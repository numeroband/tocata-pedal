# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

The **Tocata Pedal** project: a MIDI footswitch built on the Raspberry Pi Pico, plus the browser tool used to configure it. The repo is two independent halves, each with its own `CLAUDE.md` — read the one for the half you're working in:

- **[firmware/](firmware/CLAUDE.md)** — the on-pedal firmware (Pico SDK / C++), which also builds a host simulator and a multicast-MIDI bridge. See [firmware/CLAUDE.md](firmware/CLAUDE.md).
- **[web/](web/CLAUDE.md)** — the browser configuration tool (Create React App) that talks to the pedal over Web MIDI SysEx to edit programs, back up/restore, and flash firmware. See [web/CLAUDE.md](web/CLAUDE.md).

## The cross-cutting contract

The two halves speak a shared binary protocol, and three things must change **together** across both trees:

- **Command numbers** — `web/src/api/Api.mjs` ↔ `firmware/src/usb/config_protocol.h`.
- **Packed struct layouts** — `web/src/api/Parsers.mjs` ↔ `firmware/src/config/` structs (config, program, footswitch).
- **SysEx framing** — `web/src/api/MidiSysEx.mjs` ↔ `firmware/src/midi_sysex.h` (prefix `F0 00 2F 7F`, 8→7-bit packing).

Also keep `web/package.json` `version` aligned with the firmware release whose UF2 the in-app updater downloads. When touching the protocol, update both sides and check the per-half `CLAUDE.md` for details.
