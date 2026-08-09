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
  (preset within that group), all on `--channel`. Expression pedal is enabled
  and mapped to the QC's own `CC#1` ("Expression Pedal 1").
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
