# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Firmware for the **Tocata Pedal**, a MIDI footswitch built on the Raspberry Pi Pico family. The same source tree builds two very different binaries from one CMake project, controlled by `PICO_BOARD`:

- **Embedded** (`PICO_BOARD=pico2` or `pico`): the on-pedal firmware, using the Pico SDK 2.2.0, TinyUSB, U8G2, and a Wiznet W6100 stack.
- **Host simulator** (`PICO_BOARD=none`): a desktop executable that compiles the same controller against an SDL/libremidi-backed HAL for development on macOS. On macOS this also builds a second target, `TocataMidi`, a standalone helper that bridges multicast MIDI to virtual MIDI ports and talks to a Behringer WING for primary/secondary failover.

Out-of-tree build dirs already exist: `build/` (pico2, ninja) and `build-macos/` (none, Xcode).

## Build & flash

The repo is wired for the **Raspberry Pi Pico VS Code extension**, which expects tools under `~/.pico-sdk/` (SDK 2.2.0, toolchain 14_2_Rel1, cmake 3.28.6, ninja 1.12.1, picotool 2.0.0). The shell `PATH` injection lives in [.vscode/settings.json](.vscode/settings.json); if running outside VS Code, export `PICO_SDK_PATH` and `PICO_TOOLCHAIN_PATH` from there first.

```bash
# Embedded build (Pico 2 / W6100):
~/.pico-sdk/ninja/v1.12.1/ninja -C build

# Flash via picotool (USB BOOTSEL):
~/.pico-sdk/picotool/2.0.0/picotool/picotool load build/src/TocataPedal.uf2 -fx

# Flash via SWD (CMSIS-DAP + OpenOCD), matches the VS Code "Flash" task:
~/.pico-sdk/openocd/0.12.0+dev/openocd \
  -s ~/.pico-sdk/openocd/0.12.0+dev/scripts \
  -f interface/cmsis-dap.cfg -f target/rp2350.cfg \
  -c "adapter speed 5000; program build/src/TocataPedal.elf verify reset exit"

# Host build (macOS, Xcode generator already in build-macos/):
cmake --build build-macos --target TocataPedal
# TocataMidi (multicast-MIDI ↔ virtual-MIDI ↔ WING bridge, macOS only):
cmake --build build-macos --target TocataMidi
./build-macos/src/tocata-midi/TocataMidi.app/Contents/MacOS/TocataMidi <iface> [num_ports] [primary|secondary]
```

To configure from scratch: `cmake -S . -B build -G Ninja` (embedded, defaults to `pico2`) or `cmake -S . -B build-macos -G Xcode -DPICO_BOARD=none` (host).

There is no test suite; verification is done on hardware or via the host build.

## Architecture

### One source, two targets via the HAL

[src/hal/hal.h](src/hal/hal.h) declares board-agnostic structs (`HWConfig*`) and switches between [hal_pico.h](src/hal/hal_pico.h) and [hal_host.h](src/hal/hal_host.h) on `PICO_BUILD`. Every platform-specific call (flash, GPIO, PIO, USB, I2C/SPI, board LED, `millis`/`sleep_ms`) goes through free functions in that header. **Code under `src/` outside `hal/` must not include Pico SDK headers directly** — add a new HAL function instead.

`src/CMakeLists.txt` also branches on `PICO_SDK`: on embedded it links the SDK + `tinyusb` and generates PIO headers; on host it links `libremidi` + `SDL2-static` (added under `lib/CMakeLists.txt`).

### Board variants (pin maps live in main)

[src/main.cpp](src/main.cpp) is the single place where each physical board's pin assignments are encoded. The `RASPBERRYPI_PICO2` block configures the long pedal (10 switches, 8 LEDs, SPI OLED, W6100 ethernet, expression jack with detect pin). The `RASPBERRYPI_PICO` block is the original short pedal (I2C OLED, no ethernet, fewer pins). `is_pedal_long()` in the HAL is the runtime feature flag derived from these. When adding a new board, extend `HWConfig` and both `main.cpp` branches together.

### Controller is the app

[src/controller.h](src/controller.h) / [controller.cpp](src/controller.cpp) owns every subsystem (`UsbDevice`, `Switches`, `Expression`, `Leds`, `Display`, `Network`, `Config`) and drives them from a cooperative `run()` loop called by `main()`. There are no threads or RTOS; everything is non-blocking polling. The display runs at ~20 Hz via `PollTimer`; everything else runs every iteration. The controller is a state machine over modes (footswitch / setup / program-change / tuner), each with its own switch + LED callback set.

### Persistence

[src/config/](src/config/) implements a tiny FAT-like layout over a fixed flash partition (`kFlashPartitionOffset`, `kFlashPartitionSize` in the HAL). `Config` holds device-level state; `Program` holds one of up to 99 footswitch programs (8 switches × on/off action lists, expression channel, colors, mode). Structs that hit flash are `__attribute__((packed))` and versioned — preserve layout when editing.

### USB protocol

[src/usb/web_usb.h](src/usb/web_usb.h) defines the binary control protocol the browser config tool (`web/` at the repo root) speaks over WebUSB: numbered `Command`s (`kGetConfig`, `kSetProgram`, `kMemRead`, `kFlashErase`, etc.) with packed request/response structs. The MIDI side ([src/midi_sysex.h](src/midi_sysex.h)) implements 7-bit SysEx encoding under the manufacturer prefix `F0 00 2F 7F`.

### Networking & redundancy

When ethernet is available, [src/network/](src/network/) brings up the W6100 over SPI and exposes a `MulticastMidi` sender that joins an IPv6 multicast group (RTP-MIDI-style). The same `mc_midi.hpp` is compiled into the host `TocataMidi` helper, which additionally runs a TCP session against a Behringer WING ([src/tocata-midi/wing_session.hpp](src/tocata-midi/wing_session.hpp)) and toggles the local ethernet output based on the `IO_ALTSW` node, implementing primary/secondary failover between two paired pedals.

### PIO

[src/pio/](src/pio/) drives the WS2812B LED chain via the `.pio` program ([ws2812b.pio](src/pio/ws2812b.pio), compiled to a header by `pico_generate_pio_header`). Switches and the expression pedal were previously PIO-driven but are now polled GPIO/ADC (see recent commit `dd9074e Removed PIO programs for switches`); the `pio/switches.cpp` and `pio/expression.cpp` files remain as the polling implementations.

## Version

Bumped in the top-level [CMakeLists.txt](CMakeLists.txt) (`TOCATA_PEDAL_VERSION_*`); fed into both targets via `VERSION_MAJOR/MINOR/SUBMINOR` compile definitions, and into `pico_set_program_version` for the UF2 metadata.
