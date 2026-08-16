"""Command-line interface, mirroring web/src/api/cli.mjs.

    pytocatapedal <command> [args]

Talks to a pedal over USB MIDI (python-rtmidi). Configure the transport with
the same environment variables the Node CLI uses, so a shell setup carries
over between the two:
    TOCATA_TRANSPORT      only "midi" is supported (default: midi)
    TOCATA_MIDI_DEVICE    MIDI port name (default: "Tocata Pedal")
    TOCATA_MIDI_CHANNEL   SysEx channel, 0-15 (default: broadcast/any)
"""
import argparse
import json
import os
import sys
import time

from .api import Api
from .midi_sysex import ANY_CHANNEL
from .models import Backup, Config, Program, Setlist, from_wire, to_wire
from .quad_cortex import QC_PRODUCT_IDS, build_backup, collect_qc_data, configure_logging
from .transport_midi import TransportMidi
from .uf2 import UF2


def _int_auto_base(value: str) -> int:
    return int(value, 0)


def _read_json(path):
    with open(path) as f:
        return json.load(f)


def _write_json_or_print(data, path):
    text = json.dumps(data, indent=2)
    if path:
        with open(path, "w") as f:
            f.write(text)
    else:
        print(text)


def _build_transport() -> TransportMidi:
    transport_kind = os.environ.get("TOCATA_TRANSPORT", "midi")
    if transport_kind != "midi":
        sys.exit(f"Wrong transportClass {transport_kind}")
    device = os.environ.get("TOCATA_MIDI_DEVICE", "Tocata Pedal")
    channel_env = os.environ.get("TOCATA_MIDI_CHANNEL")
    channel = int(channel_env) if channel_env is not None else ANY_CHANNEL
    return TransportMidi(device, channel)


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="pytocatapedal")
    sub = parser.add_subparsers(dest="command", required=True)

    sub.add_parser("get-config").add_argument("path", nargs="?")
    sub.add_parser("set-config").add_argument("path")
    sub.add_parser("del-config")
    sub.add_parser("get-names")

    p = sub.add_parser("get-program")
    p.add_argument("id", type=int)
    p.add_argument("path", nargs="?")

    p = sub.add_parser("set-program")
    p.add_argument("id", type=int)
    p.add_argument("path")

    p = sub.add_parser("del-program")
    p.add_argument("id", type=int)

    sub.add_parser("get-setlists")

    p = sub.add_parser("get-setlist")
    p.add_argument("id", type=int)
    p.add_argument("path", nargs="?")

    p = sub.add_parser("set-setlist")
    p.add_argument("id", type=int)
    p.add_argument("path")

    p = sub.add_parser("del-setlist")
    p.add_argument("id", type=int)

    sub.add_parser("restart")
    sub.add_parser("bootrom")

    sub.add_parser("backup").add_argument("path", nargs="?")

    p = sub.add_parser("restore")
    p.add_argument("path")

    sub.add_parser("factory")

    p = sub.add_parser("flash")
    p.add_argument("path")

    p = sub.add_parser("read")
    p.add_argument("addr", type=_int_auto_base)
    p.add_argument("length", type=_int_auto_base)
    p.add_argument("path")

    p = sub.add_parser("write")
    p.add_argument("addr", type=_int_auto_base)
    p.add_argument("path")

    p = sub.add_parser("erase")
    p.add_argument("addr", type=_int_auto_base)
    p.add_argument("length", type=_int_auto_base)

    p = sub.add_parser("uf2-info")
    p.add_argument("path")

    p = sub.add_parser("sync-quad-cortex")
    p.add_argument("--channel", type=int, required=True,
                    help="Quad Cortex MIDI channel (1-16)")
    p.add_argument("--pedal-channel", type=int, default=None,
                    help="Tocata pedal's own MIDI channel (1-16); defaults to "
                         "--channel. Every generated action follows this channel "
                         "(Config.midi.channel) rather than a channel baked into "
                         "it, so it must match the Quad Cortex's own channel "
                         "(--channel) for the synced programs to reach it.")
    p.add_argument("--program-mode-switch", type=int, default=None,
                    help="footswitch index (0-7) to configure as 'program' mode "
                         "instead of 'scene' mode, for presets where that scene "
                         "has no label; a labeled scene is left as-is and a "
                         "warning is logged instead of overwriting it")
    p.add_argument("--collect-timeout", type=float, default=30.0,
                    help="ceiling on the wait for the device's setlist listings "
                         "(it typically answers in ~7s); not a fixed delay")
    p.add_argument("--qc-model", choices=["auto", *QC_PRODUCT_IDS], default="auto",
                    help="which Quad Cortex to open; 'auto' picks the model that is "
                         "actually attached over USB")
    p.add_argument("--qc-data-out", default=None,
                    help="optionally save the raw Quad Cortex snapshot to this path")
    p.add_argument("--qc-data-in", default=None,
                    help="skip live USB collection, reuse a saved --qc-data-out snapshot")
    p.add_argument("-v", "--verbose", action="store_true",
                    help="also log per-preset/per-folder DEBUG detail")

    return parser


def _dispatch(api: Api, args: argparse.Namespace):
    command = args.command

    if command == "get-config":
        _write_json_or_print(to_wire(api.get_config()), args.path)
    elif command == "set-config":
        api.set_config(from_wire(Config, _read_json(args.path)))
    elif command == "del-config":
        api.delete_config()
    elif command == "get-names":
        print(json.dumps(api.get_program_names(), indent=2))
    elif command == "get-program":
        _write_json_or_print(to_wire(api.get_program(args.id)), args.path)
    elif command == "set-program":
        api.set_program(args.id, from_wire(Program, _read_json(args.path)))
    elif command == "del-program":
        api.delete_program(args.id)
    elif command == "get-setlists":
        print(json.dumps(api.get_setlist_names(), indent=2))
    elif command == "get-setlist":
        _write_json_or_print(to_wire(api.get_setlist(args.id)), args.path)
    elif command == "set-setlist":
        api.set_setlist(args.id, from_wire(Setlist, _read_json(args.path)))
    elif command == "del-setlist":
        api.delete_setlist(args.id)
    elif command == "restart":
        api.restart()
        print("restarting")
    elif command == "bootrom":
        api.boot_rom()
        print("boot rom ready")
    elif command == "backup":
        _write_json_or_print(to_wire(api.backup()), args.path)
    elif command == "restore":
        api.restore(from_wire(Backup, _read_json(args.path)))
    elif command == "factory":
        api.factory()
    elif command == "flash":
        with open(args.path, "rb") as f:
            uf2 = UF2(f.read())
        print("Flashing firmware")
        api.flash_firmware(uf2)
        print("Verifying firmware")
        api.verify_firmware(uf2)
        time.sleep(1)
        print("Restarting")
        api.restart()
    elif command == "read":
        content = api.read_memory(args.addr, args.length)
        with open(args.path, "wb") as f:
            f.write(content)
    elif command == "write":
        with open(args.path, "rb") as f:
            content = f.read()
        api.write_memory(args.addr, content)
    elif command == "erase":
        api.flash_erase(args.addr, args.length)
    elif command == "uf2-info":
        with open(args.path, "rb") as f:
            uf2 = UF2(f.read())
        for index, block in enumerate(uf2.blocks):
            print(f"--- Block {index}:")
            print(f"flags=0x{block.flags:x}")
            print(f"family=0x{block.family_id:x}")
            print(f"address=0x{block.address:x}")
            print(f"size={len(block.payload)}")
            print()
    elif command == "sync-quad-cortex":
        configure_logging(args.verbose)
        if not (1 <= args.channel <= 16):
            sys.exit("error: --channel must be 1-16")
        pedal_channel = args.pedal_channel if args.pedal_channel is not None else args.channel
        if not (1 <= pedal_channel <= 16):
            sys.exit("error: --pedal-channel must be 1-16")
        if args.program_mode_switch is not None and not (0 <= args.program_mode_switch < 8):
            sys.exit("error: --program-mode-switch must be 0-7")
        if args.qc_data_in:
            qc_data = _read_json(args.qc_data_in)
        else:
            qc_data = collect_qc_data(args.collect_timeout, model=args.qc_model)
            if args.qc_data_out:
                _write_json_or_print(qc_data, args.qc_data_out)
        backup_dict = build_backup(qc_data, pedal_channel=pedal_channel - 1,
                                    program_mode_switch=args.program_mode_switch)
        api.restore(from_wire(Backup, backup_dict))
        print("synced from Quad Cortex")
    else:
        raise ValueError(f'invalid arg "{command}"')


def main():
    parser = _build_parser()
    args = parser.parse_args()
    transport = _build_transport()
    api = Api(transport)
    start = time.time()
    try:
        api.connect()
        _dispatch(api, args)
    except Exception as e:
        print("ERROR!!!", e, file=sys.stderr)
    print("time", time.time() - start)


if __name__ == "__main__":
    main()
