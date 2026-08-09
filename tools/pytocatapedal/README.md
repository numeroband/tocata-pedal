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
