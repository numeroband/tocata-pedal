# pytocatapedal

Python port of the Tocata Pedal's MIDI SysEx API (`web/src/api/*.mjs`) and its
Node CLI (`web/src/api/cli.mjs`). Same wire protocol, same JSON shapes, no
Node/npm required.

## Install

```bash
pip install -e tools/pytocatapedal[dev]
```

Requires a working [`python-rtmidi`](https://pypi.org/project/python-rtmidi/)
install (needs a C compiler and the platform's MIDI backend headers on some
systems -- see that package's docs if the wheel build fails).

## CLI

```bash
pytocatapedal <command> [args]
```

Transport is configured via environment variables (same names as the Node
CLI, so a shell setup works for either):

| Variable               | Default          | Notes                              |
| ---------------------- | ---------------- | ----------------------------------- |
| `TOCATA_TRANSPORT`     | `midi`           | only `midi` is implemented          |
| `TOCATA_MIDI_DEVICE`   | `Tocata Pedal`   | MIDI port name to connect to        |
| `TOCATA_MIDI_CHANNEL`  | broadcast/any    | SysEx channel, `0`-`15`             |

Commands (get/backup commands print JSON to stdout, or write it to `path` if
given):

```
get-config [path]              set-config <path>              del-config
get-names
get-program <id> [path]        set-program <id> <path>        del-program <id>
get-setlists
get-setlist <id> [path]        set-setlist <id> <path>        del-setlist <id>
restart                        bootrom
backup [path]                  restore <path>                 factory
flash <file.uf2>
read <addr> <length> <path>    write <addr> <path>            erase <addr> <length>
uf2-info <path>
```

A backup file written by `pytocatapedal backup` can be restored with
`node web/src/api/cli.mjs restore` and vice versa -- both produce the same
JSON shape.

## Library

```python
from pytocatapedal import Api, TransportMidi

transport = TransportMidi("Tocata Pedal")
api = Api(transport)
api.connect()
config = api.get_config()
```

## Quad Cortex sync

`sync-quad-cortex` reads a connected Neural DSP Quad Cortex's user setlists
over USB and restores them straight to the pedal -- no intermediate `restore`
step needed. It needs the `quad-cortex` extra (pulls in
[pyquadcortex](https://pypi.org/project/pyquadcortex/) and `hidapi`, both USB
HID-based and not needed for the rest of the package):

```bash
pip install -e tools/pytocatapedal[quad-cortex]
pytocatapedal sync-quad-cortex --channel 1
```

It works around `pyquadcortex` only knowing the regular Quad Cortex's product
id: it enumerates HID for Neural DSP's vendor id (`0x152A`), matches the
attached unit against its own model table (regular QC or Mini), and overrides
`pyquadcortex.hid_ids.PRODUCT_ID` accordingly before connecting --
`--qc-model qc|mini` forces this when auto-detection can't decide (e.g. both
are attached) or finds nothing.

The mapping to Tocata's schema, per the Quad Cortex's documented MIDI
implementation ("Incoming MIDI Messages" / "Incoming MIDI CC List" in its
manual):

- **One program per QC preset** -- its `actions` send the preset-recall
  sequence `CC#32` (setlist bank), `CC#0` (0-127 vs 128-255 group), then `PC`
  (preset within that group), and the expression pedal is enabled and mapped
  to the QC's own `CC#1` ("Expression Pedal 1"). All of these follow the
  pedal's global MIDI channel (`--pedal-channel`, which defaults to
  `--channel`) rather than a channel baked into each action, so re-pointing
  the whole rig at another channel is a single config edit instead of
  re-syncing.
- **One footswitch per QC scene** (always 8, `mode: 'scene'`) -- its
  `onActions` sends `CC#43` with the scene's 0-7 value, and its name is the
  scene label (truncated to 8 chars). An empty scene label leaves the name
  empty (unavailable), except the preset's `default_scene`, which gets the
  literal name `"DEFAULT"` instead so it stays available even when the QC
  preset never labeled it; that footswitch also starts `enabled`. Colors are
  the nearest CSS-distance match to Tocata's 6-color enum.
- **`--program-mode-switch <0-7>`** replaces one footswitch with
  `mode: 'program'` (a pure program-change trigger) instead of a scene
  switch, but only for presets where that scene has no label -- a labeled
  scene is left as-is and a warning is logged instead of overwriting it.
- **One setlist per QC setlist**, referencing its programs by id.

**CC#32 setlist-bank values come from the device's own folder-push order.**
`0` = Factory Library, `1` = "My Presets", `2`..n = the user setlists in
device order -- the only place the device exposes that order is the sequence
it pushes folder listings in when answering a `File{READ}`, which is also the
order its Directory screen shows. `--qc-data-out <path>` saves the raw
collected snapshot (including the detected bank order); `--qc-data-in <path>`
replays a saved snapshot without touching USB again, useful for retrying the
pedal sync after a partial failure.

## Quad Cortex tuner bridge

`pytocatapedal-qc-bridge` drives the pedal's tuner display from a rack-mounted
Quad Cortex's own tuner, over a live MIDI connection -- no SysEx, no config
protocol, just a CC in and a stream of Note On out. It needs the
`quad-cortex` extra (same as `sync-quad-cortex`):

```bash
pip install -e tools/pytocatapedal[quad-cortex]
pytocatapedal-qc-bridge "<midi port name>" --channel 1
```

`<midi port name>` is used for both input and output (the pedal's own MIDI
port, in the intended deployment). The bridge:

- listens for CC#45 on `--channel` (1-16, default 1): **>=64 enables** the
  Quad Cortex's tuner meter feed, **<64 disables** it. This is the exact CC
  the pedal's own tuner mode sends/expects (`kTunerModeCc` in
  `firmware/src/controller.h` -- it used to be hardcoded to CC 47, a firmware
  bug fixed alongside this tool so the two sides agree).
- while enabled, forwards every detected-pitch push from the Quad Cortex as a
  MIDI Note On, encoding note + cents-off exactly the way the pedal's
  firmware decodes it (`controller.cpp`'s incoming `0x90` handler: velocity
  is a signed cents-times-two offset folded into note, not an independent
  value; see `freq_to_midi_note()` in `qc_bridge.py`). `freq == 0` (no pitch
  detected) becomes `note=0, velocity=0`, which the firmware already treats
  as "clear the tuner display".
- connects to the Quad Cortex once at startup and holds that connection for
  the whole run -- avoids re-running pyquadcortex's connect handshake (up to
  ~30s) on every CC toggle, at the cost of holding the Quad Cortex's USB HID
  interface exclusively (Cortex Control can't be opened) while the bridge is
  running.
- like `sync-quad-cortex`, auto-detects the regular Quad Cortex vs. the Mini
  over USB; `--qc-model qc|mini` forces it when auto-detection can't decide.

The Quad Cortex's own tuner-meter fields (`Tuner.enable_meter`/`Tuner.meter`)
aren't exposed by any public `pyquadcortex` method -- that project's own docs
call the meter permanently unsupported, based on a full-size Quad Cortex on
an older firmware that refused the write. That's stale for at least the Quad
Cortex Mini, which streams the meter fine; `qc_bridge.py` drives it directly
through `pyquadcortex`'s transport layer (`qc._t.send`/`qc._t.collect`) since
`pyquadcortex` isn't ours to change.

## Tests

```bash
pytest tools/pytocatapedal/tests
```

These cover the pure-data logic only (SysEx pack/unpack, struct
parse/serialize round-trips, UF2 block parsing) -- no pedal hardware needed.

## Protocol compatibility

This package must stay byte-compatible with the firmware and with
`web/src/api/*.mjs` -- see the root `CLAUDE.md`'s cross-cutting contract.
`midi_sysex.py`, `parsers.py`, and `uf2.py` are direct, field-for-field ports
of `MidiSysEx.mjs`, `Parsers.mjs`, and `UF2.mjs`; if the protocol changes,
update all of them together.
