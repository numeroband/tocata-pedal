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
node src/api/cli.mjs <command> [args]      # get-config, set-program, backup, flash, read, ... see cli.mjs
node src/api/cli.mjs flash <file.uf2>      # stream firmware over USB MIDI (no BOOTSEL/SWD needed)

# Local simulator (fake pedal backed by ~/.tocata-sim, serves the built app on :8080):
npm run sim                                # needs `npm run build` first
```

`npm run firmware` downloads `TocataPedal-<version>.uf2` from the GitHub release matching `package.json` `version` into `public/`, so the in-app firmware updater ships a build that matches the app version. Bump `version` in `package.json` in lockstep with firmware releases.

## Architecture

### Layered API: transport → protocol → API → React

The core lives in `src/api/` as framework-agnostic `.mjs` modules so it can run in the browser, in Node (CLI), and in the sim:

- **[Api.mjs](src/api/Api.mjs)** — the high-level surface: `getConfig`/`setProgram`/`flashFirmware`/`backup`/etc. Numeric `Command` constants here mirror the firmware's `config_protocol.h` enum (`GET_CONFIG=3`, `SET_PROGRAM=8`, `MEM_READ=0x10`, …). Requests are serialized through an internal queue so only one is in flight at a time. There are up to **99 programs**.
- **[Protocol.mjs](src/api/Protocol.mjs)** — frames messages with a 4-byte header (`uint16 length`, `uint8 command`, `uint8 status`), reassembles fragmented responses, and rejects on non-zero status. Picks the transport by duck-typing the object passed in (`requestMIDIAccess` → Web MIDI, `createReadStream` → Node).
- **[TransportMidi.mjs](src/api/TransportMidi.mjs)** (browser, Web MIDI) / **[TransportNodeMidi.mjs](src/api/TransportNodeMidi.mjs)** (Node `midi` package) — find the input/output ports named "Tocata Pedal" and move raw bytes. Both wrap payloads in SysEx via **[MidiSysEx.mjs](src/api/MidiSysEx.mjs)**.
- **[Parsers.mjs](src/api/Parsers.mjs)** — a declarative struct (de)serializer. Schemes describe the on-the-wire layout (`config`, `program`, `footswitch`, `action`, …) and `parseX`/`serializeX` walk them. **This layout must stay byte-compatible with the firmware's packed structs** — changing a field order/type here without matching the firmware breaks the protocol.

### SysEx framing (must match firmware)

[MidiSysEx.mjs](src/api/MidiSysEx.mjs) implements the 7-bit packing used on the wire: manufacturer prefix `F0 00 2F 7F`, an optional 1-byte channel, then the payload bit-packed 8→7. This mirrors `../firmware/src/midi_sysex.h`. The channel can be `kSysExAnyChannel` (0x7F, broadcast) or a specific channel; the URL `?channel=` / env `TOCATA_MIDI_CHANNEL` selects it so multiple pedals can be addressed independently. The device name defaults to "Tocata Pedal" and is overridable via `?device=` / `TOCATA_MIDI_DEVICE`.

### Footswitch mode model (mirrors firmware)

Programs have a program-level `mode` (`default` / `scene`) and each footswitch has its own `mode` (`stomp` / `momentary` / `scene`) — see the `mode`/`fsMode` enums in [Parsers.mjs](src/api/Parsers.mjs). Legacy programs saved as whole-program `scene` are expanded on read into `mode: 'default'` with every switch forced to `scene` (see `getProgram` in [Api.mjs](src/api/Api.mjs)), so the per-switch editor renders them correctly and re-saving preserves the behavior. Keep this in sync with the firmware's two-level mode resolution (`Program::switchMode`).

### React app

[App.js](src/App.js) sets up the Material UI v4 theme (auto light/dark) and a fixed nav of four screens — **Programs**, **Configuration**, **Backup/Restore**, **Firmware** — each a component under [src/components/](src/components). [components/Api.js](src/components/Api.js) is the React-facing wrapper: it instantiates the `Api` against `navigator` (Web MIDI), reads URL params (`transport`, `device`, `channel`), and exposes connect / read-all / update-all / flash helpers with progress callbacks. Web MIDI requires a secure context (HTTPS or localhost) and a Chromium-based browser.

### Simulator

[sim/](sim) is a separate Express app (`sim/index.mjs`) that serves the production `build/` and implements the pedal's REST/file API against `~/.tocata-sim`, with [sim/Sim.mjs](sim/Sim.mjs) emulating the device. It's an older REST-style path (see [setupProxy.js](src/setupProxy.js), which proxies `/api` to `:8080` in dev) and is independent of the MIDI transport used against real hardware. `sim-package.json` / `tocata-sim.bat` package it as a standalone distributable (`npm run tocata-sim`).

## Relationship to firmware

This app is one half of a contract with `../firmware`: the command numbers ([Api.mjs](src/api/Api.mjs) ↔ `config_protocol.h`), the packed struct layouts ([Parsers.mjs](src/api/Parsers.mjs) ↔ `config/` structs), and the SysEx encoding ([MidiSysEx.mjs](src/api/MidiSysEx.mjs) ↔ `midi_sysex.h`) must all change together. When editing the protocol, update both trees and keep `package.json` `version` aligned with the firmware release whose UF2 the updater downloads.
