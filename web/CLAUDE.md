# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

The **browser configuration tool** for the Tocata Pedal (the firmware lives in `../firmware`). It's a Create React App single-page app, deployed to GitHub Pages (`numeroband.github.io/tocata-pedal`), that talks to the pedal over **Web MIDI SysEx** to edit programs, system config, back up/restore, and flash firmware — no install, runs from the browser. The same API/transport code (`src/api/*.mjs`) is reused by a Node CLI and a desktop simulator.

## Commands

```bash
# Dev server (CRA, http://localhost:3000):
npm start

# Production build into build/ (note the legacy OpenSSL flag, required by CRA 5 on modern Node):
npm run build

# Deploy to GitHub Pages (runs predeploy: fetch matching firmware UF2 into public/, then build):
npm run deploy

# Tests (CRA / react-scripts, jest):
npm test

# CLI against a connected pedal (Node `midi` transport, not Web MIDI):
node src/api/cli.mjs <command> [args]      # get-config, set-program, get-setlists, set-setlist, backup, flash, read, ... see cli.mjs
node src/api/cli.mjs flash <file.uf2>      # stream firmware over USB MIDI (no BOOTSEL/SWD needed)

# Local simulator (fake pedal backed by ~/.tocata-sim, serves the built app on :8080):
npm run sim                                # needs `npm run build` first
```

`npm run firmware` downloads `TocataPedal-<version>.uf2` from the GitHub release matching `package.json` `version` into `public/`, so the in-app firmware updater ships a build that matches the app version. Bump `version` in `package.json` in lockstep with firmware releases.

## Architecture

### Layered API: transport → protocol → API → React

The core lives in `src/api/` as framework-agnostic `.mjs` modules so it can run in the browser, in Node (CLI), and in the sim:

- **[Api.mjs](src/api/Api.mjs)** — the high-level surface: `getConfig`/`setProgram`/`getSetlist`/`flashFirmware`/`backup`/etc. Numeric `Command` constants here mirror the firmware's `config_protocol.h` enum (`GET_CONFIG=3`, `SET_PROGRAM=8`, `GET_SETLISTS=0x0A`, `MEM_READ=0x10`, …). Requests are serialized through an internal queue so only one is in flight at a time. There are up to **99 programs** and **26 setlists**.
- **[Protocol.mjs](src/api/Protocol.mjs)** — frames messages with a 4-byte header (`uint16 length`, `uint8 command`, `uint8 status`), reassembles fragmented responses, and rejects on non-zero status. Picks the transport by duck-typing the object passed in (`requestMIDIAccess` → Web MIDI, `createReadStream` → Node).
- **[TransportMidi.mjs](src/api/TransportMidi.mjs)** (browser, Web MIDI) / **[TransportNodeMidi.mjs](src/api/TransportNodeMidi.mjs)** (Node `midi` package) — find the input/output ports named "Tocata Pedal" and move raw bytes. Both wrap payloads in SysEx via **[MidiSysEx.mjs](src/api/MidiSysEx.mjs)**.
- **[Parsers.mjs](src/api/Parsers.mjs)** — a declarative struct (de)serializer. Schemes describe the on-the-wire layout (`config`, `program`, `footswitch`, `action`, `setlist`, …) and `parseX`/`serializeX` walk them. **This layout must stay byte-compatible with the firmware's packed structs** — changing a field order/type here without matching the firmware breaks the protocol.

### SysEx framing (must match firmware)

[MidiSysEx.mjs](src/api/MidiSysEx.mjs) implements the 7-bit packing used on the wire: manufacturer prefix `F0 00 2F 7F`, an optional 1-byte channel, then the payload bit-packed 8→7. This mirrors `../firmware/src/midi_sysex.h`. The channel can be `kSysExAnyChannel` (0x7F, broadcast) or a specific channel; the URL `?channel=` / env `TOCATA_MIDI_CHANNEL` selects it so multiple pedals can be addressed independently. The device name defaults to "Tocata Pedal" and is overridable via `?device=` / `TOCATA_MIDI_DEVICE`.

### Footswitch mode model (mirrors firmware)

Programs have a program-level `mode` (`default` / `scene`) and each footswitch has its own `mode` (`stomp` / `momentary` / `scene` / `program`) — see the `mode`/`fsMode` enums in [Parsers.mjs](src/api/Parsers.mjs). Legacy programs saved as whole-program `scene` are expanded on read into `mode: 'default'` with every switch forced to `scene` (see `getProgram` in [Api.mjs](src/api/Api.mjs)), so the per-switch editor renders them correctly and re-saving preserves the behavior. Keep this in sync with the firmware's two-level mode resolution (`Program::switchMode`). `program` mode has no on/off MIDI actions — it's a pure trigger the firmware uses to enter program-change mode — so the editor hides both Actions cards and strips any actions when a switch is set to it.

### Setlists (no UI yet)

A setlist is a named, ordered subset of the programs (`name` + `programs: [id, …]`, max 99) that the pedal uses to drive program-change navigation. Wire ids are **0-based, 0..25**, mirroring programs; the pedal displays them +1 and reserves its own index 0 for the built-in "All" setlist, which is synthetic and never stored — so there is no id 0 meaning "All" on this side. `getSetlistNames` pages like `getProgramNames` but trims to `NUM_SETLISTS`, because the device always replies with a full 16-slot page (the program version does *not* trim, so it returns 112 entries with a stale tail — pre-existing).

`backup`/`restore`/`factory` in [Api.mjs](src/api/Api.mjs) and `readAll`/`updateAll` in [components/Api.js](src/components/Api.js) cover setlists, so the Backup/Restore screen round-trips them even though nothing renders them. **Restore is a full device sync**: slots absent from the JSON are deleted, so restoring a backup taken before setlists existed clears all 26. The **Programs editor deliberately has no setlist UI yet** — adding one means a new screen plus a `Navigation.js` entry.

### React app

[App.js](src/App.js) sets up the Material UI v4 theme (auto light/dark) and a fixed nav of four screens — **Programs**, **Configuration**, **Backup/Restore**, **Firmware** — each a component under [src/components/](src/components). [components/Api.js](src/components/Api.js) is the React-facing wrapper: it instantiates the `Api` against `navigator` (Web MIDI), reads URL params (`transport`, `device`, `channel`), and exposes connect / read-all / update-all / flash helpers with progress callbacks. Web MIDI requires a secure context (HTTPS or localhost) and a Chromium-based browser.

### Simulator

[sim/](sim) is a separate Express app (`sim/index.mjs`) that serves the production `build/` and implements the pedal's REST/file API against `~/.tocata-sim`, with [sim/Sim.mjs](sim/Sim.mjs) emulating the device. It's an older REST-style path (see [setupProxy.js](src/setupProxy.js), which proxies `/api` to `:8080` in dev) and is independent of the MIDI transport used against real hardware. `sim-package.json` / `tocata-sim.bat` package it as a standalone distributable (`npm run tocata-sim`).

## Quad Cortex sync script (`python/`)

[python/sync_quad_cortex.py](python/sync_quad_cortex.py) is a standalone Python script (not part of the npm toolchain) that generates a `cli.mjs restore`-compatible backup JSON from a connected Neural DSP Quad Cortex's user setlists. Run it with a venv that has [pyquadcortex](https://pypi.org/project/pyquadcortex/) installed:

```bash
python sync_quad_cortex.py --channel 1
```

It connects over USB (working around a pyquadcortex bug by overriding `hid_ids.PRODUCT_ID` for the Quad Cortex Mini), reads the folders under `pyquadcortex.USER_SETLIST_ROOT`, and reads each preset's full `BinaryPreset` (name, 8 scene labels/colors, `default_scene`). It writes two files: a raw `qc_data.json` snapshot (reusable via `--qc-data-in` to regenerate without touching hardware again) and the final backup (`--out`, default `tocata-backup.json`).

A `File{READ}` makes the device enumerate its **whole** tree — ~800 folders over ~15s — but the setlists are the front of that stream: measured on a QC Mini they arrive first, contiguously, ~10ms apart, each already complete, with the first non-setlist folder ~50ms later. The cost is the device's ~6.5s delay before it answers at all, not the enumeration. So `read_setlist_folders` waits for that burst and stops as soon as the enumeration moves past it (`--collect-timeout` is a ceiling, not a duration); it falls back to whatever it recorded if the burst never ends that way. Don't replace this with a fixed-duration `collect` — the window has to cover a device latency that isn't ours to predict.

The mapping to Tocata's schema, per the Quad Cortex's documented MIDI implementation ("Incoming MIDI Messages" / "Incoming MIDI CC List" in its manual):

- **One program per QC preset** — its `actions` send the preset-recall sequence `CC#32` (setlist bank), `CC#0` (0-127 vs 128-255 group), then `PC` (preset within that group), all on `--channel`. Expression pedal is enabled and mapped to the QC's own `CC#1` ("Expression Pedal 1").
- **One footswitch per QC scene** (always 8, `mode: 'scene'`) — its `onActions` sends `CC#43` with the scene's 0-7 value. The footswitch matching the preset's `default_scene` starts `enabled`. Colors are the nearest CSS-distance match to Tocata's 6-color enum (QC's ARGB vs. `blue`/`purple`/`red`/`yellow`/`green`/`turquoise`).
- **One setlist per QC setlist**, referencing its programs by id.

**CC#32 setlist-bank values come from the device's own folder-push order.** `0` = Factory Library, `1` = "My Presets", `2`..n = the user setlists in device order. That order is not alphabetical and no `FolderInfo` field carries it — the only place the device exposes it is the order it pushes folder listings in when answering a `File{READ}`, which is also the order its Directory screen shows. `collect_qc_data` records first-arrival order and stores the resulting `bank` in `qc_data.json`; a `qc_data.json` predating this has no `bank` and the script errors rather than guessing.

Verified against a Quad Cortex Mini: push order matched banks 1-4 exactly, banks past the last setlist were ignored (the PC applied within the current setlist instead), and creating a fifth setlist put it last in push order and made it answer to bank `5`. Replaying every generated program's recall actions over USB MIDI landed the unit on the right preset 13/13.

The script only **generates** the JSON — running the destructive `node src/api/cli.mjs restore <file>` against a real pedal is a separate, manual step.

## Relationship to firmware

This app is one half of a contract with `../firmware`: the command numbers ([Api.mjs](src/api/Api.mjs) ↔ `config_protocol.h`), the packed struct layouts ([Parsers.mjs](src/api/Parsers.mjs) ↔ `config/` structs), and the SysEx encoding ([MidiSysEx.mjs](src/api/MidiSysEx.mjs) ↔ `midi_sysex.h`) must all change together. When editing the protocol, update both trees and keep `package.json` `version` aligned with the firmware release whose UF2 the updater downloads.
