#!/usr/bin/env python3
"""Generate a Tocata Pedal restore JSON from a Quad Cortex's user setlists.

Run with the pyquadcortex venv:

    ~/qc_venv/bin/python tools/qc_sync/sync_quad_cortex.py --channel 1

Then, separately and manually:

    node ../web/src/api/cli.mjs restore tocata-backup.json
"""

import argparse
import json
import logging
import sys
import time

QC_MINI_PRODUCT_ID = 35119  # pyquadcortex.hid_ids.PRODUCT_ID only knows the regular QC

# CC#32 (bank LSB) selects the setlist: 0 = Factory Library, 1 = "My Presets",
# 2..n = the user setlists in device order. That order is NOT alphabetical and is
# not carried by any field of FolderInfo -- it is the order the device pushes the
# folder listings in, in response to a File READ, which is also the order the unit's
# Directory screen shows them in.
#
# Verified against a Quad Cortex Mini: the push order (My Presets, Pearl Jam,
# dnotes 90s, LISTA1) matched banks 1-4 exactly, banks 5+ were ignored, and
# creating a fifth setlist put it at push index 4 and made it answer to bank 5.
FACTORY_BANK = 0

SCENE_SELECT_CC = 43
BANK_MSB_CC = 0
BANK_LSB_CC = 32
EXPRESSION_1_CC = 1

MAX_PRG_NAME_SIZE = 30
MAX_FS_NAME_SIZE = 8
NUM_PROGRAMS = 99
NUM_SETLISTS = 26

# The 6 footswitch LED colors the pedal's enum supports (web/src/api/Parsers.mjs
# `colors`), given as the literal CSS color values the web UI renders them with
# (web/src/components/Footswitches.js:54 uses the enum string directly as a CSS
# backgroundColor) -- used as the reference palette for nearest-color matching.
FOOTSWITCH_COLOR_RGB = {
    "blue": (0x00, 0x00, 0xFF),
    "purple": (0x80, 0x00, 0x80),
    "red": (0xFF, 0x00, 0x00),
    "yellow": (0xFF, 0xFF, 0x00),
    "green": (0x00, 0x80, 0x00),
    "turquoise": (0x40, 0xE0, 0xD0),
}

log = logging.getLogger("sync_quad_cortex")


class _MillisecondFormatter(logging.Formatter):
    """A Formatter whose %(asctime)s includes milliseconds, e.g. 14:23:01.123."""

    converter = time.localtime

    def formatTime(self, record, datefmt=None):
        ct = self.converter(record.created)
        base = time.strftime("%Y-%m-%d %H:%M:%S", ct)
        return f"{base}.{int(record.msecs):03d}"


def configure_logging(verbose: bool) -> None:
    """INFO+ (or DEBUG+ with --verbose) to stdout, WARNING+ to stderr, both timestamped."""
    log.setLevel(logging.DEBUG if verbose else logging.INFO)
    log.propagate = False

    formatter = _MillisecondFormatter("%(asctime)s [%(levelname)s] %(message)s")

    stdout_handler = logging.StreamHandler(sys.stdout)
    stdout_handler.setFormatter(formatter)
    stdout_handler.setLevel(logging.DEBUG if verbose else logging.INFO)
    stdout_handler.addFilter(lambda record: record.levelno < logging.WARNING)

    stderr_handler = logging.StreamHandler(sys.stderr)
    stderr_handler.setFormatter(formatter)
    stderr_handler.setLevel(logging.WARNING)

    log.addHandler(stdout_handler)
    log.addHandler(stderr_handler)


def read_setlist_folders(qc, pa, root: str, timeout: float):
    """The user setlist folder listings, in the order the device pushes them.

    A single File READ makes the device enumerate its ENTIRE tree -- ~800 folders
    (factory library, captures, IRs, every plugin's artist folders) over roughly
    fifteen seconds. We want only the handful under ``root``, and measurement on a
    Quad Cortex Mini shows they are exactly the front of that stream: the setlists
    arrive first, contiguously, ~10ms apart, each already carrying its COMPLETE
    file list, and the first non-setlist folder follows ~50ms later. The expensive
    part is not the enumeration but the device's ~6.5s delay before it says
    anything at all.

    So rather than collecting for a fixed number of seconds -- which has to be
    long enough to cover a latency that isn't ours to predict -- wait for the
    burst and stop as soon as the enumeration moves past it. ``timeout`` is only a
    ceiling on that wait, not a duration we sit through.

    The recording happens in the ``match`` predicate because that is the only
    per-message hook: the transport evaluates it against every inbound message of
    the type, in arrival order, so returning True on "seen a setlist, now seeing
    something else" both ends the wait and leaves us holding the whole burst.
    """
    best, order, default_key = {}, [], [None]
    start = time.monotonic()

    def record(m):
        f = m.folder
        if not f.key:
            return False
        if not f.key.startswith(root):
            # A non-setlist folder: the burst is over IF we actually got one.
            if order:
                log.debug(
                    "folder enumeration moved past the setlist burst at %r "
                    "after %.3fs",
                    f.key,
                    time.monotonic() - start,
                )
            return bool(order)
        if f.key not in best:
            order.append(f.key)
            log.debug(
                "setlist folder #%d seen: key=%r name=%r files=%d (t=%.3fs)",
                len(order),
                f.key,
                f.name,
                len(f.files),
                time.monotonic() - start,
            )
        if f.HasField("is_user_default") and f.is_user_default:
            default_key[0] = f.key
        prev = best.get(f.key)
        if prev is None or len(f.files) > len(prev.files):
            best[f.key] = f
        return False

    log.info(
        "requesting File READ and waiting up to %.1fs for the setlist burst under %r",
        timeout,
        root,
    )
    try:
        qc._t.await_broadcast(
            pa.FileMessage,
            lambda: qc._t.send(pa.FileMessage(action=pa.MessageAction.READ)),
            timeout=timeout,
            match=record,
        )
    except TimeoutError:
        # The burst never ended the way we expect (a device whose enumeration
        # order differs, or one slower than `timeout`). Whatever `record` saw over
        # the full wait is still correct and correctly ordered -- this degrades to
        # the fixed-window behavior rather than failing.
        if not order:
            log.warning(
                "folder enumeration timed out after %.1fs with no setlists seen at all",
                timeout,
            )
            raise
        log.warning(
            f"folder enumeration did not settle within {timeout}s; "
            f"using the {len(order)} setlist(s) seen so far.",
        )
    log.info(
        "folder enumeration finished: %d setlist(s) in %.3fs, default=%r",
        len(order),
        time.monotonic() - start,
        default_key[0],
    )
    return best, order, default_key[0]


def collect_qc_data(timeout: float) -> dict:
    import pyquadcortex
    import pyquadcortex.hid_ids as hid_ids
    from pyquadcortex.proto import ProductionAutomation_pb2 as pa

    hid_ids.PRODUCT_ID = QC_MINI_PRODUCT_ID
    log.debug("overrode pyquadcortex product id to %d (Quad Cortex Mini)", QC_MINI_PRODUCT_ID)

    log.info("connecting to Quad Cortex over USB...")
    with pyquadcortex.connect() as qc:
        log.info("connected")
        root = pyquadcortex.USER_SETLIST_ROOT

        # `order` is the arrival order, which is the CC#32 bank order (see
        # FACTORY_BANK above) and is the only place the device exposes it.
        best, order, default_key = read_setlist_folders(qc, pa, root, timeout)

        if default_key is not None and order and order[0] != default_key:
            log.warning(
                f"expected the device-default setlist ({default_key!r}) "
                f"to arrive first, but {order[0]!r} did -- CC#32 bank numbers "
                f"below are probably wrong.",
            )

        setlists = []
        for bank, key in enumerate(order, start=FACTORY_BANK + 1):
            folder = best[key]
            log.info(
                "reading setlist %r (key=%r, bank=%d): %d preset(s)",
                folder.name,
                key,
                bank,
                len(folder.files),
            )
            presets = []
            for pd in folder.files:
                if not (pd.HasField("name") and pd.name):
                    continue
                t0 = time.monotonic()
                bp = qc.read_preset(key, pd.index)
                log.debug(
                    "read preset index=%d name=%r in %.3fs",
                    pd.index,
                    bp.name,
                    time.monotonic() - t0,
                )
                presets.append({
                    "name": bp.name,
                    "index": pd.index,
                    "scene_labels": list(bp.scene_labels),
                    "scene_colors": list(bp.scene_colors),
                    "default_scene": bp.default_scene if bp.HasField("default_scene") else 0,
                })
            setlists.append({
                "key": key,
                "name": folder.name,
                "bank": bank,
                "presets": presets,
            })

        log.info(
            "collection complete: %d setlist(s), %d preset(s) total",
            len(setlists),
            sum(len(s["presets"]) for s in setlists),
        )
        return {"setlist_root": root, "setlists": setlists}


def nearest_footswitch_color(argb: int) -> str:
    r = (argb >> 16) & 0xFF
    g = (argb >> 8) & 0xFF
    b = argb & 0xFF
    return min(
        FOOTSWITCH_COLOR_RGB,
        key=lambda name: sum(
            (a - b2) ** 2 for a, b2 in zip((r, g, b), FOOTSWITCH_COLOR_RGB[name])
        ),
    )


def scene_footswitch(scene_index: int, label: str, color_argb: int, is_default: bool,
                      channel: int) -> dict:
    letter = chr(ord("A") + scene_index)
    name = label.strip() or letter
    return {
        "name": name[:MAX_FS_NAME_SIZE],
        "color": nearest_footswitch_color(color_argb),
        "enabled": is_default,
        "mode": "scene",
        "onActions": [
            {"type": "CC", "channel": channel, "values": [SCENE_SELECT_CC, scene_index]},
        ],
        "offActions": [],
    }


def preset_recall_actions(index: int, bank: int, channel: int) -> list:
    group, pc = (0, index) if index < 128 else (1, index - 128)
    return [
        {"type": "CC", "channel": channel, "values": [BANK_LSB_CC, bank]},
        {"type": "CC", "channel": channel, "values": [BANK_MSB_CC, group]},
        {"type": "PC", "channel": channel, "values": [pc, 0]},
    ]


def build_program(preset: dict, bank: int, channel: int) -> dict:
    scenes = preset["scene_labels"]
    colors = preset["scene_colors"]
    default_scene = preset["default_scene"]
    fs = [
        scene_footswitch(i, scenes[i], colors[i], i == default_scene, channel)
        for i in range(min(len(scenes), len(colors)))
    ]
    log.debug(
        "built program %r (preset index=%d, bank=%d) with %d footswitch(es)",
        preset["name"],
        preset["index"],
        bank,
        len(fs),
    )
    return {
        "name": preset["name"][:MAX_PRG_NAME_SIZE],
        "fs": fs,
        "actions": preset_recall_actions(preset["index"], bank, channel),
        "mode": "default",
        "expChannel": channel,
        "expression": EXPRESSION_1_CC,
    }


def build_backup(qc_data: dict, channel: int, pedal_channel: int) -> dict:
    log.info(
        "building backup: %d setlist(s) from qc_data, channel=%d, pedal_channel=%d",
        len(qc_data["setlists"]),
        channel,
        pedal_channel,
    )
    programs = []
    setlists = []
    for setlist in qc_data["setlists"]:
        if "bank" not in setlist:
            sys.exit(
                "error: this qc_data.json predates CC#32 bank detection and has "
                "no 'bank' field. Re-collect from the device (drop --qc-data-in)."
            )
        bank = setlist["bank"]
        log.debug("processing setlist %r (bank=%d, %d preset(s))",
                   setlist["name"], bank, len(setlist["presets"]))
        program_ids = []
        for preset in setlist["presets"]:
            if len(programs) >= NUM_PROGRAMS:
                log.warning(
                    f"dropping preset {preset['name']!r} from "
                    f"{setlist['name']!r} -- {NUM_PROGRAMS} program slots exhausted.",
                )
                continue
            program_ids.append(len(programs))
            programs.append(build_program(preset, bank, channel))
        if len(setlists) >= NUM_SETLISTS:
            log.warning(
                f"dropping setlist {setlist['name']!r} -- "
                f"{NUM_SETLISTS} setlist slots exhausted.",
            )
            continue
        setlists.append({
            "name": setlist["name"][:MAX_PRG_NAME_SIZE],
            "programs": program_ids,
        })

    log.info(
        "backup built: %d program(s), %d setlist(s)",
        len(programs),
        len(setlists),
    )
    return {
        "version": 1,
        "midi": {"channel": pedal_channel},
        "programs": programs,
        "setlists": setlists,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--channel", type=int, required=True,
                        help="Quad Cortex MIDI channel (1-16)")
    parser.add_argument("--pedal-channel", type=int, default=None,
                        help="Tocata pedal's own MIDI channel (1-16); "
                             "defaults to --channel")
    parser.add_argument("--collect-timeout", type=float, default=30.0,
                        help="ceiling on the wait for the device's setlist listings "
                             "(it typically answers in ~7s); not a fixed delay")
    parser.add_argument("--qc-data-out", default="qc_data.json")
    parser.add_argument("--qc-data-in", default=None,
                        help="skip live collection, reuse a saved qc_data.json")
    parser.add_argument("--out", default="tocata-backup.json")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="also log per-preset/per-folder DEBUG detail")
    args = parser.parse_args()

    configure_logging(args.verbose)
    log.info("sync_quad_cortex starting: %s", vars(args))

    if not (1 <= args.channel <= 16):
        parser.error("--channel must be 1-16")
    pedal_channel = args.pedal_channel if args.pedal_channel is not None else args.channel
    if not (1 <= pedal_channel <= 16):
        parser.error("--pedal-channel must be 1-16")

    if args.qc_data_in:
        log.info("loading cached Quad Cortex data from %s (skipping live collection)",
                  args.qc_data_in)
        with open(args.qc_data_in) as f:
            qc_data = json.load(f)
    else:
        qc_data = collect_qc_data(args.collect_timeout)
        with open(args.qc_data_out, "w") as f:
            json.dump(qc_data, f, indent=2)
        log.info(f"wrote {args.qc_data_out}")

    backup = build_backup(qc_data, channel=args.channel - 1, pedal_channel=pedal_channel - 1)
    with open(args.out, "w") as f:
        json.dump(backup, f, indent=2)
    log.info(f"wrote {args.out}: {len(backup['programs'])} program(s), "
             f"{len(backup['setlists'])} setlist(s)")


if __name__ == "__main__":
    main()
