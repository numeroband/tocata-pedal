"""Generate a Tocata Pedal backup from a Quad Cortex's user setlists.

Ported from the former web/python/sync_quad_cortex.py -- see cli.py's
`sync-quad-cortex` command, which drives this module's `collect_qc_data()`
and `build_backup()` and feeds the result straight into `Api.restore()`
instead of writing an intermediate JSON file.
"""

import logging
import sys
import time

NEURAL_DSP_VENDOR_ID = 0x152A
# pyquadcortex.hid_ids only knows the regular Quad Cortex (its PRODUCT_ID is 0x880A); the
# Mini is a different product id it has no constant for, so we supply both here and
# override hid_ids.PRODUCT_ID with whichever unit is actually attached. That override
# works because pyquadcortex/session.py reads hid_ids.PRODUCT_ID at connect time, not at
# import time.
QC_PRODUCT_IDS = {
    "qc": (0x880A, "Quad Cortex"),
    "mini": (0x892F, "Quad Cortex Mini"),  # 35119
}

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
NUM_FOOTSWITCHES = 8

# Label given to the footswitch designated as the program-mode trigger via
# --program-mode-switch; that switch has no onActions/offActions (mode "program"
# is a pure trigger, see web/CLAUDE.md's "Footswitch mode model"), so it doesn't
# carry a QC scene label.
PROGRAM_MODE_FS_NAME = "..."

# The 6 footswitch LED colors the pedal's enum supports (models.Color) -- used
# as the reference palette for nearest-color matching against the QC's ARGB.
FOOTSWITCH_COLOR_RGB = {
    "blue": (0x00, 0x00, 0xFF),
    "purple": (0x80, 0x00, 0x80),
    "red": (0xFF, 0x00, 0x00),
    "yellow": (0xFF, 0xFF, 0x00),
    "green": (0x00, 0x80, 0x00),
    "turquoise": (0x40, 0xE0, 0xD0),
}

log = logging.getLogger("pytocatapedal.quad_cortex")


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


def detect_product_id(model: str):
    """(product_id, label) for the QC to open, or None if nothing was detected.

    ``model`` is "auto" or a key of QC_PRODUCT_IDS. An explicit (non-"auto") model is
    returned without touching USB at all -- it must work even on a host where HID
    enumeration is unreliable. "auto" enumerates HID for Neural DSP's vendor id; this
    does NOT open the device, so it still finds a unit that Cortex Control is holding
    exclusively (which would make the later pyquadcortex.connect() fail regardless).
    """
    if model != "auto":
        return QC_PRODUCT_IDS[model]

    import hid

    found = set()
    for d in hid.enumerate():
        if d["vendor_id"] != NEURAL_DSP_VENDOR_ID:
            continue
        found.add(d["product_id"])

    known = {pid: label for pid, label in QC_PRODUCT_IDS.values()}
    known_found = {pid for pid in found if pid in known}

    for pid in sorted(found - known_found):
        log.warning(
            "a Neural DSP USB device with unrecognized product id %#x was seen; "
            "add it to QC_PRODUCT_IDS if it's a new Quad Cortex model.",
            pid,
        )

    if len(known_found) > 1:
        names = ", ".join(known[pid] for pid in sorted(known_found))
        sys.exit(
            f"error: multiple Quad Cortex models detected over USB ({names}). "
            f"Pick one with --qc-model {{{'|'.join(QC_PRODUCT_IDS)}}}."
        )
    if len(known_found) == 1:
        pid = next(iter(known_found))
        return pid, known[pid]
    return None


def collect_qc_data(timeout: float, model: str = "auto") -> dict:
    import pyquadcortex
    import pyquadcortex.hid_ids as hid_ids
    from pyquadcortex.proto import ProductionAutomation_pb2 as pa

    detected = detect_product_id(model)
    if detected is not None:
        product_id, product_label = detected
        hid_ids.PRODUCT_ID = product_id
        log.info("detected %s (product id %#x) over USB", product_label, product_id)
    else:
        product_id, product_label = hid_ids.PRODUCT_ID, "unknown model"
        log.warning(
            "no Neural DSP device enumerated over USB; falling back to pyquadcortex's "
            "default product id %#x -- if this is a Quad Cortex Mini, pass "
            "--qc-model mini",
            product_id,
        )

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
        return {
            "setlist_root": root,
            "device": {"model": product_label, "product_id": product_id},
            "setlists": setlists,
        }


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
    # An empty name makes the firmware treat the footswitch as unavailable
    # (Footswitch::available() is `_name[0]`, see firmware/src/config/config.h)
    # -- exactly what an empty QC scene label should mean here. The default
    # scene is the exception: it's named explicitly so it stays available
    # even when the QC preset never labeled it.
    stripped = label.strip()
    name = stripped or ("DEFAULT" if is_default else "")
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


def program_mode_footswitch() -> dict:
    return {
        "name": PROGRAM_MODE_FS_NAME,
        # Explicit "no color" (colors[0] in web/src/api/Parsers.mjs) rather than
        # an omitted key: the enum serializer does `colors.findIndex(v => v ===
        # value)`, and findIndex on undefined returns -1, which wraps to a
        # bogus 255 on the wire instead of a real "unset" value.
        "color": None,
        "mode": "program",
    }


def preset_recall_actions(index: int, bank: int, channel: int) -> list:
    group, pc = (0, index) if index < 128 else (1, index - 128)
    return [
        {"type": "CC", "channel": channel, "values": [BANK_LSB_CC, bank]},
        {"type": "CC", "channel": channel, "values": [BANK_MSB_CC, group]},
        {"type": "PC", "channel": channel, "values": [pc, 0]},
    ]


def build_program(preset: dict, bank: int, channel: int, program_mode_switch: int = None) -> dict:
    scenes = preset["scene_labels"]
    colors = preset["scene_colors"]
    default_scene = preset["default_scene"]
    # Only turn program_mode_switch into the program-mode trigger if the QC
    # preset left that scene unlabeled. A labeled scene is a real, in-use QC
    # scene -- overwriting it with the trigger would make it unreachable, so
    # we keep the proper scene footswitch there instead and warn that this
    # preset doesn't get a program-mode trigger.
    program_mode_index = None
    if program_mode_switch is not None and program_mode_switch < len(scenes):
        scene_label = scenes[program_mode_switch].strip()
        if scene_label:
            log.warning(
                f"preset {preset['name']!r}: footswitch {program_mode_switch} has "
                f"QC scene {scene_label!r}; not using program mode for this preset "
                f"to avoid overwriting it.",
            )
        else:
            program_mode_index = program_mode_switch
    fs = [
        program_mode_footswitch() if i == program_mode_index
        else scene_footswitch(i, scenes[i], colors[i], i == default_scene, channel)
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


def build_backup(qc_data: dict, channel: int, pedal_channel: int, program_mode_switch: int = None) -> dict:
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
            programs.append(build_program(preset, bank, channel, program_mode_switch))
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
