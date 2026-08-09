"""Bridge a Quad Cortex's tuner meter to the Tocata Pedal's tuner display.

Intended deployment: a Quad Cortex racked up alongside a Mac or Raspberry Pi
running this bridge, with a Tocata Pedal out front connected over MIDI. The
pedal's own tuner mode sends CC#45 (127 entering, 0 exiting, see
firmware/src/controller.cpp's kTunerModeCc) and expects a stream of MIDI Note
On messages to drive its on-screen tuner needle. This bridge listens for that
CC on a MIDI port, toggles the Quad Cortex's tuner meter feed accordingly
(via pyquadcortex's transport directly -- pyquadcortex's own public API has
no method for it, see docs/roadmap.md in that project, which is stale for at
least the Quad Cortex Mini), and forwards every detected-pitch push as a Note
On using the same note+velocity encoding the pedal's firmware decodes
(controller.cpp's incoming 0x90 handler).
"""

import argparse
import logging
import math
import sys
import threading
import time

import rtmidi

from .quad_cortex import QC_PRODUCT_IDS, detect_product_id
from .transport_midi import TransportMidi

TUNER_MODE_CC = 45
TUNER_ENABLE_THRESHOLD = 64   # CC value >= 64 enables, < 64 disables
COLLECT_WINDOW_SECONDS = 0.1  # Transport.collect()'s poll loop sleeps 100ms
                               # per iteration regardless of a smaller
                               # `seconds`, so this already matches its real
                               # granularity
_A4 = 440.0

log = logging.getLogger("pytocatapedal.qc_bridge")


class _MillisecondFormatter(logging.Formatter):
    """A Formatter whose %(asctime)s includes milliseconds, e.g. 14:23:01.123."""

    converter = time.localtime

    def formatTime(self, record, datefmt=None):
        ct = self.converter(record.created)
        base = time.strftime("%Y-%m-%d %H:%M:%S", ct)
        return f"{base}.{int(record.msecs):03d}"


def configure_logging(verbose: bool) -> None:
    """INFO+ (or DEBUG+ with --verbose) to stdout, WARNING+ to stderr, both timestamped."""
    log.setLevel(logging.DEBUG if verbose else logging.INFO)
    log.propagate = False

    formatter = _MillisecondFormatter("%(asctime)s [%(levelname)s] %(message)s")

    stdout_handler = logging.StreamHandler(sys.stdout)
    stdout_handler.setFormatter(formatter)
    stdout_handler.setLevel(logging.DEBUG if verbose else logging.INFO)
    stdout_handler.addFilter(lambda record: record.levelno < logging.WARNING)

    stderr_handler = logging.StreamHandler(sys.stderr)
    stderr_handler.setFormatter(formatter)
    stderr_handler.setLevel(logging.WARNING)

    log.addHandler(stdout_handler)
    log.addHandler(stderr_handler)


def _lround(x: float) -> int:
    """Round half away from zero, like C++'s std::lround (Python's round() rounds half to even)."""
    return int(math.floor(x + 0.5)) if x >= 0 else int(math.ceil(x - 0.5))


def freq_to_midi_note(freq: float) -> tuple:
    """Detected pitch (Hz) -> (note, velocity), matching the pedal firmware's decode.

    velocity is not an independent "how in tune" value: it's cents-off-pitch
    times two, signed, clamped to [-64, 63], and reconstructed together with
    note into one continuous pitch value. A negative cents offset decrements
    note and biases velocity into [64, 127] -- exactly undone by the
    firmware's incoming Note On handler (controller.cpp: `if (velocity > 63)
    { ++note; velocity -= 128; }`). freq == 0 (no pitch detected) maps to
    (0, 0), which the firmware treats as "clear the tuner display" (note < 24
    draws nothing).
    """
    if freq == 0:
        return 0, 0
    midi_float = 12.0 * math.log2(freq / _A4) + 69.0
    note = _lround(midi_float)
    cents = (midi_float - note) * 100.0
    velocity_signed = max(-64, min(63, _lround(cents * 2.0)))
    if velocity_signed < 0:
        note -= 1
        velocity = velocity_signed + 128
    else:
        velocity = velocity_signed
    if not 0 <= note <= 127:
        # MIDI data bytes are 7-bit. Real guitar pitches never leave 0..127
        # (E2 ~= note 40 up through high harmonics), so this is cheap
        # insurance against a stray reading rather than an expected path.
        log.debug("freq_to_midi_note(%r): note %d out of MIDI range, clamping", freq, note)
        note = max(0, min(127, note))
    return note, velocity


class _Bridge:
    """Owns the MIDI-out port and Quad Cortex connection, and forwards the
    tuner meter feed to Note On while CC#TUNER_MODE_CC keeps it enabled.
    """

    def __init__(self, qc, pa, midi_out, channel: int):
        self._qc = qc
        self._pa = pa
        self._midi_out = midi_out
        self._channel = channel
        self._enabled = threading.Event()
        self._stop = threading.Event()
        self._worker = threading.Thread(target=self._forward_loop, daemon=True)

    def start(self):
        self._worker.start()

    def stop(self):
        self._stop.set()
        if self._enabled.is_set():
            self._set_enabled(False)
        self._worker.join(timeout=2.0)

    def on_midi_in(self, event, _data=None):
        """rtmidi callback: reacts only to CC#TUNER_MODE_CC on our channel."""
        message, _delta_time = event
        if len(message) < 3:
            return
        status, data1, data2 = message[0], message[1], message[2]
        if (status & 0xF0) != 0xB0 or (status & 0x0F) != self._channel:
            return
        if data1 != TUNER_MODE_CC:
            return
        self._set_enabled(data2 >= TUNER_ENABLE_THRESHOLD)

    def _set_enabled(self, enabled: bool):
        if enabled == self._enabled.is_set():
            return  # already in that state; the device write would be a no-op
        log.info("tuner meter %s", "enabled" if enabled else "disabled")
        self._qc._t.send(self._pa.TunerMessage(
            action=self._pa.MessageAction.UPDATE, enable_meter=enabled))
        if enabled:
            self._enabled.set()
        else:
            self._enabled.clear()

    def _forward_loop(self):
        while not self._stop.is_set():
            if not self._enabled.wait(timeout=0.5):
                continue
            batch = self._qc._t.collect(
                self._pa.TunerMessage, lambda: None, COLLECT_WINDOW_SECONDS,
                match=lambda m: m.HasField("meter"))
            if not self._enabled.is_set():
                continue  # disabled mid-collect; drop this batch
            for m in batch:
                note, velocity = freq_to_midi_note(m.meter)
                self._midi_out.send_message([0x90 | self._channel, note, velocity])
                log.debug("meter=%.3fHz -> note=%d velocity=%d", m.meter, note, velocity)


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="pytocatapedal-qc-bridge",
        description="Bridge a Quad Cortex's tuner meter to MIDI Note On, "
                     "toggled by CC#45 (>=64 enable, <64 disable) -- the same "
                     "CC the Tocata Pedal's own tuner mode uses.")
    parser.add_argument("port", help="MIDI port name, used for both input and output")
    parser.add_argument("--channel", type=int, default=1,
                         help="MIDI channel, 1-16 (default: 1)")
    parser.add_argument("--qc-model", choices=["auto", *QC_PRODUCT_IDS], default="auto",
                         help="which Quad Cortex to open; 'auto' picks the model that "
                              "is actually attached over USB")
    parser.add_argument("-v", "--verbose", action="store_true",
                         help="also log per-note DEBUG detail")
    return parser


def _open_midi(port_name: str):
    midi_in = rtmidi.MidiIn()
    midi_out = rtmidi.MidiOut()
    in_index = TransportMidi._find_port(midi_in, port_name)
    out_index = TransportMidi._find_port(midi_out, port_name)
    if in_index is None or out_index is None:
        sys.exit(f"error: cannot find MIDI port {port_name!r}")
    midi_in.open_port(in_index)
    midi_out.open_port(out_index)
    return midi_in, midi_out


def main():
    args = _build_parser().parse_args()
    if not (1 <= args.channel <= 16):
        sys.exit("error: --channel must be 1-16")
    configure_logging(args.verbose)
    channel = args.channel - 1

    midi_in, midi_out = _open_midi(args.port)
    log.info("opened MIDI port %r", args.port)

    import pyquadcortex
    import pyquadcortex.hid_ids as hid_ids
    from pyquadcortex.proto import ProductionAutomation_pb2 as pa

    detected = detect_product_id(args.qc_model)
    if detected is not None:
        product_id, product_label = detected
        hid_ids.PRODUCT_ID = product_id
        log.info("detected %s (product id %#x) over USB", product_label, product_id)
    else:
        product_id, product_label = hid_ids.PRODUCT_ID, "unknown model"
        log.warning(
            "no Neural DSP device enumerated over USB; falling back to pyquadcortex's "
            "default product id %#x -- if this is a Quad Cortex Mini, pass --qc-model mini",
            product_id,
        )

    log.info("connecting to Quad Cortex over USB...")
    with pyquadcortex.connect() as qc:
        log.info("connected to %s", product_label)
        bridge = _Bridge(qc, pa, midi_out, channel)
        midi_in.set_callback(bridge.on_midi_in)
        bridge.start()
        log.info("listening on %r (channel %d) for CC#%d -- Ctrl+C to exit",
                  args.port, args.channel, TUNER_MODE_CC)
        try:
            while True:
                time.sleep(0.5)
        except KeyboardInterrupt:
            pass
        finally:
            log.info("shutting down")
            bridge.stop()
            midi_in.close_port()
            midi_out.close_port()


if __name__ == "__main__":
    main()
