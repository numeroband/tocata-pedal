"""python-rtmidi backed MIDI transport, mirroring web/src/api/TransportNodeMidi.mjs.

python-rtmidi is the direct analog of the RtMidi-backed `midi` npm package
the Node CLI uses.
"""
import queue
from typing import Optional

import rtmidi

from .midi_sysex import ANY_CHANNEL, from_sysex, to_sysex


class TransportMidi:
    def __init__(self, device_name: str, channel: int = ANY_CHANNEL, connection_event=None):
        self.device_name = device_name
        self.channel = channel
        self.connection_event = connection_event
        self.midi_in: Optional[rtmidi.MidiIn] = None
        self.midi_out: Optional[rtmidi.MidiOut] = None
        self._queue: "queue.Queue[bytes]" = queue.Queue()

    @property
    def connected(self) -> bool:
        return self.midi_in is not None

    def version(self):
        # Hardcoded stub, matching the JS transport (never read from the device).
        return {"major": 1, "minor": 2, "subminor": 3}

    def connect(self):
        self.midi_in = rtmidi.MidiIn()
        self.midi_out = rtmidi.MidiOut()

        input_index = self._find_port(self.midi_in, self.device_name)
        output_index = self._find_port(self.midi_out, self.device_name)
        if input_index is None or output_index is None:
            raise RuntimeError(f"Cannot find MIDI device '{self.device_name}'")

        self.midi_in.set_callback(self._on_message)
        self.midi_in.open_port(input_index)
        self.midi_in.ignore_types(sysex=False, timing=True, active_sense=True)
        self.midi_out.open_port(output_index)

        if self.connection_event:
            self.connection_event(True)

    def reconnect(self):
        return self.connect()

    def send(self, data: bytes):
        sysex = to_sysex(bytes(data), self.channel)
        self.midi_out.send_message(list(sysex))

    def receive(self) -> bytes:
        return self._queue.get()

    def _on_message(self, event, _data=None):
        message, _delta_time = event
        buffer = from_sysex(bytes(message), self.channel)
        if buffer is None:
            return
        self._queue.put(buffer)

    @staticmethod
    def _find_port(port, name: str) -> Optional[int]:
        count = port.get_port_count()
        for i in range(count):
            if port.get_port_name(i) == name:
                return i
        # rtmidi backends (notably ALSA) sometimes suffix port names
        # (e.g. "Tocata Pedal:0"), so fall back to a prefix match.
        for i in range(count):
            if port.get_port_name(i).startswith(name):
                return i
        return None
