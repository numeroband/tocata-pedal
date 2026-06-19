---
name: hardware-debug
description: Debug Tocata Pedal firmware on the real Pico hardware by reading UART logs and sending live USB MIDI, then reflash without needing BOOTSEL. Use this whenever a bug report or test involves incoming MIDI (Program Change, Control Change, Note on/off) and needs to be reproduced or verified on the physical pedal — the host simulator (build-macos) cannot receive plain MIDI messages, only SysEx, so it is not a substitute for this. Also use it whenever the user mentions flashing the pedal, the device being stuck/frozen/not booting, or wants to watch printf/debug output while testing.
---

# Hardware debug loop for the Tocata Pedal

The host simulator build (`PICO_BOARD=none`, `build-macos/.../TocataPedal.app`) is convenient, but its virtual "Tocata Pedal" MIDI port only relays SysEx (the WebUSB-over-MIDI bridge used by the web config tool). It does **not** process plain incoming Note/CC/Program-Change messages — `MidiUsb::run()`'s `tud_midi_*` polling loop is `#ifdef HAL_PICO` only ([src/hal/hal_host.cpp](../../src/hal/hal_host.cpp), [src/usb/midi_usb.cpp](../../src/usb/midi_usb.cpp)). So if a task involves reproducing or verifying behavior triggered by incoming MIDI, the host build will silently do nothing and you'll draw the wrong conclusion. Test on the real pedal instead, using the loop below: read UART logs, send live MIDI, rebuild, reflash, repeat.

## Setup

```bash
python3 -m venv /tmp/midienv   # one-time; reuse the venv after
/tmp/midienv/bin/pip install pyserial python-rtmidi
```

Plain `cat`/`stty` on the pedal's USB CDC serial port were unreliable in practice (returned no data even though the device was alive) — use `pyserial` instead.

## Reading UART logs

The pedal exposes a USB CDC serial port at `/dev/tty.usbmodem*` (the exact suffix number can change between reconnects — re-glob it after every flash/restart/power-cycle). Baud is 115200, no special line-ending handling needed.

```python
import serial, time
port = "/dev/tty.usbmodem104"  # re-check with `ls /dev/tty.usbmodem*`
ser = serial.Serial(port, 115200, timeout=0.5)
ser.read(8192)  # flush whatever was buffered before you started caring
# ... do something that should produce output ...
time.sleep(0.6)  # give the device a moment to print and flush
print(ser.read(8192).decode(errors="replace"))
```

Firmware printf output (e.g. `loadProgram %u`, the periodic `display average:` line, LED state dumps) shows up here. The periodic `display average:` line proves the device's main loop is alive even when nothing else is printing — if you stop seeing it, the device has likely hung or disconnected.

If you add temporary `printf` debug statements to `controller.cpp` (or elsewhere) to trace a bug, **remove them before considering the task done** — rebuild and reflash the clean version once you've confirmed the fix, and diff the file to make sure no debug lines were left behind.

## Reading the pedal's config (channel, programs, scenes)

Before sending any MIDI, pull the pedal's actual configuration instead of guessing or asking the user to recall it from memory:

```bash
cd firmware
node ../web/src/api/cli.mjs backup /tmp/pedal_backup.json
```

This dumps the live config over the same USB-MIDI/WebUSB-sysex channel used for flashing — no separate setup. The resulting JSON has `version`, `midi.channel` (0-indexed — see the channel note below), and `programs`, an array where each entry has `name`, `mode` (`"scene"` or `"stomp"`), and `fs` (an 8-slot array of footswitch configs, `null` for unused slots). For `"scene"` mode programs, exactly one `fs` entry should carry `"enabled": true` — that's the program's currently-active/default scene, the same value `Program::defaultScene()` computes in firmware. Inspect this before testing a scene-related bug so you know which scene index to expect, instead of inferring it from trial-and-error CC sends:

```python
import json
d = json.load(open("/tmp/pedal_backup.json"))
print("channel:", d["midi"]["channel"])
p = d["programs"][2]
print(p["name"], p["mode"])
for i, fs in enumerate(p["fs"]):
    if fs:
        print(i, fs["name"], fs.get("enabled", False))
```

## Sending MIDI to the pedal

The pedal is a class-compliant USB MIDI device. Find it by name, not by assuming a fixed index — port enumeration order isn't guaranteed, and on the host simulator the same name can appear twice (it opens two virtual ports), while on real hardware it usually appears once.

```python
import rtmidi
mo = rtmidi.MidiOut()
ports = mo.get_ports()
idx = ports.index("Tocata Pedal")
mo.open_port(idx)
mo.send_message([status_byte, data1, data2])
```

**Before sending anything, confirm the pedal's configured MIDI channel.** Don't assume channel 1 (wire value 0) and don't just ask the user to recall it from memory — read it directly off the device with the backup command below, which is faster and avoids off-by-one mistakes. The channel is stored and reported 0-indexed; the pedal's own display shows it 1-indexed (e.g. `midi.channel: 1` in the backup means the display reads "channel 2"). Build the status byte as `base_status | channel`, using the 0-indexed value as-is — e.g. Program Change on a pedal configured for `channel: 1` is `0xC0 | 1`.

Common messages for this firmware (see [src/controller.cpp](../../src/controller.cpp) `midiCallback`):
- Program Change: `[0xC0 | ch, program_number]` (program_number is 0-indexed)
- Control Change: `[0xB0 | ch, cc_number, value]`
- CC 47 is this firmware's tuner-mode toggle (nonzero = enter, 0 = exit)
- CC 43 is the scene-switch command

To correlate a MIDI send with its effect, open the serial port and flush it *before* sending, then read again after a short sleep — that way you see only what the message actually triggered.

## Flashing the pedal

This firmware can be reflashed entirely over the existing USB MIDI connection — no BOOTSEL button, no SWD probe needed, as long as the pedal is currently running and responsive.

```bash
cd firmware
cmake --build build                                     # defaults to pico2/embedded
node ../web/src/api/cli.mjs flash build/src/TocataPedal.uf2
```

This streams `flashErase`/`memWrite`/`memRead` progress and finishes with `Restarting`. The device drops off USB during the restart — after it finishes, `sleep 2`-ish and re-glob `/dev/tty.usbmodem*` and the rtmidi port list before sending anything else; don't reuse a `Serial`/`MidiOut` handle opened before the restart.

## If the device gets stuck

Flashing or sending MIDI to an unresponsive pedal can itself hang (the command just sits there with no output — check with `ps aux | grep cli.mjs` if a flash/restart seems to be taking too long). In order of escalation:

1. `node ../web/src/api/cli.mjs restart` — try this first, it's the least disruptive.
2. If that also hangs, kill the stuck node process and ask the user to physically power-cycle the pedal (unplug/replug). This needs the user's hands on the hardware — don't try to work around it.
3. Once it's back, re-verify `/dev/tty.usbmodem*` and the MIDI port list before resuming.

If you need a known-good build to hand off for manual flashing (e.g. via BOOTSEL drag-and-drop, in case the over-MIDI path itself is what's broken), a fresh `cmake --build build` produces `build/src/TocataPedal.uf2` — point the user at that path.
