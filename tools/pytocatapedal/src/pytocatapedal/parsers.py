"""Packed-struct (de)serialization, mirroring web/src/api/Parsers.mjs.

The wire schemes below (field order/sizes/offsets) are a direct, byte-exact
port of the JS scheme tables and must stay in sync with them and with the
firmware's `config/` structs (see the root CLAUDE.md's cross-cutting
contract). They operate on plain dicts with the same camelCase field names
the JS side uses; the public parse_*/serialize_* functions at the bottom
convert those dicts to/from the typed dataclasses in models.py.
"""
import logging
import struct as _struct
from typing import Optional, Tuple

from .models import Config, Program, Setlist, from_wire, to_wire

log = logging.getLogger(__name__)

MAX_PRG_NAME_SIZE = 30
MAX_FS_NAME_SIZE = 8
MAX_NAMES_RESPONSE = 16
MAX_ACTIONS = 5
MAX_SWITCHES = 8
MAX_PROGRAMS = 99  # Program::kMaxPrograms

MESSAGE_TYPES = [None, "PC", "CC", "NO", "NF"]
COLORS = [None, "blue", "purple", "red", "yellow", "green", "turquoise"]
MODES = ["default", "scene"]
FS_MODES = ["stomp", "momentary", "scene", "program"]


def _is_empty(value):
    if value is None:
        return True
    if isinstance(value, (list, tuple)):
        return len(value) == 0
    if value is False:
        return True
    return False


# --- primitive parsers: (buffer, offset, *args) -> (value, new_offset) ---


def _parse_uint(size):
    fmt = {1: "<B", 2: "<H", 4: "<I"}[size]

    def parser(buf, offset):
        (value,) = _struct.unpack_from(fmt, buf, offset)
        return value, offset + size

    return parser


def _parse_bool(buf, offset):
    return buf[offset] != 0, offset + 1


def _parse_enum(buf, offset, values):
    idx = buf[offset]
    return (values[idx] if 0 <= idx < len(values) else None), offset + 1


def _parse_str(buf, offset, size):
    raw = bytes(buf[offset : offset + size + 1])
    end = raw.find(0)
    if end == -1:
        end = len(raw)
    return raw[:end].decode("latin-1"), offset + size + 1


def _parse_array(buf, offset, num_elems, parser_type, *args):
    ret = []
    for _ in range(num_elems):
        parsed, offset = PARSERS[parser_type](buf, offset, *args)
        ret.append(parsed)
    return ret, offset


def _parse_narray(buf, offset, *args):
    count = buf[offset]
    offset += 1
    ret, offset = _parse_array(buf, offset, *args)
    return ret[:count], offset


def _parse_struct(buf, offset, scheme):
    ret = {}
    for field_spec in scheme["fields"]:
        field, parser_type, *args = field_spec
        try:
            parsed, offset = PARSERS[parser_type](buf, offset, *args)
        except Exception:
            log.exception("error parsing field %r", field)
            continue
        if _is_empty(parsed):
            continue
        if isinstance(field, (list, tuple)):
            for f, v in zip(field, parsed):
                ret[f] = v
        else:
            ret[field] = parsed
    valid = scheme.get("valid")
    return (None if (valid and not valid(ret)) else ret), offset


def _parse_compact(buf, offset, scheme):
    compact_parser, *compact_args = scheme["parser"]
    value, new_offset = PARSERS[compact_parser](buf, offset, *compact_args)
    scratch = bytearray(new_offset - offset)
    ret = []
    for bits, parser_type, *args in scheme["fields"]:
        try:
            mask = (1 << bits) - 1
            sub_value = value & mask
            value >>= bits
            SERIALIZERS[compact_parser](scratch, 0, sub_value, *compact_args)
            parsed, _ = PARSERS[parser_type](scratch, 0, *args)
            if not _is_empty(parsed):
                ret.append(parsed)
        except Exception:
            log.exception("error parsing compact field")
    return ret, new_offset


def _parse_u32buffer(buf, offset):
    (length,) = _struct.unpack_from("<I", buf, offset)
    offset += 4
    return bytes(buf[offset : offset + length]), offset + length


PARSERS = {
    "uint8": _parse_uint(1),
    "uint16": _parse_uint(2),
    "uint32": _parse_uint(4),
    "bool": _parse_bool,
    "enum": _parse_enum,
    "str": _parse_str,
    "array": _parse_array,
    "nArray": _parse_narray,
    "struct": _parse_struct,
    "compact": _parse_compact,
    "u32Buffer": _parse_u32buffer,
}


# --- primitive serializers: (buffer, offset, value, *args) -> new_offset ---


def _serialize_uint(size):
    fmt = {1: "<B", 2: "<H", 4: "<I"}[size]
    mask = (1 << (size * 8)) - 1

    def serializer(buf, offset, value):
        _struct.pack_into(fmt, buf, offset, (value or 0) & mask)
        return offset + size

    return serializer


def _serialize_bool(buf, offset, value):
    buf[offset] = 1 if value else 0
    return offset + 1


def _serialize_enum(buf, offset, value, values):
    try:
        idx = values.index(value)
    except ValueError:
        idx = -1
    buf[offset] = idx & 0xFF
    return offset + 1


def _serialize_str(buf, offset, value, size):
    encoded = (value or "").encode("latin-1", errors="ignore")[:size]
    buf[offset : offset + len(encoded)] = encoded
    buf[offset + len(encoded)] = 0
    return offset + size + 1


def _serialize_array(buf, offset, value, num_elems, parser_type, *args):
    # Slots beyond len(value) (up to the scheme's fixed max capacity) are
    # padding that nArray's leading count byte tells the reader to ignore --
    # the exact byte values written here don't matter, only that something
    # valid gets written. (The JS side leaves these `undefined` rather than
    # `null`, which trips its enum serializer's -1-wraps-to-255 fallback and
    # so writes different, equally-unread, padding bytes -- not something
    # this port needs to reproduce.)
    for i in range(num_elems):
        item = value[i] if (value is not None and i < len(value)) else None
        offset = SERIALIZERS[parser_type](buf, offset, item, *args)
    return offset


def _serialize_narray(buf, offset, value, *args):
    buf[offset] = len(value) if value else 0
    offset += 1
    return _serialize_array(buf, offset, value, *args)


def _serialize_struct(buf, offset, value, scheme):
    valid = scheme.get("valid")
    if not value or (valid and not valid(value)):
        value = {}
    for field_spec in scheme["fields"]:
        field, parser_type, *args = field_spec
        field_value = [value.get(f) for f in field] if isinstance(field, (list, tuple)) else value.get(field)
        offset = SERIALIZERS[parser_type](buf, offset, field_value, *args)
    return offset


def _serialize_compact(buf, offset, values, scheme):
    compact_parser, *compact_args = scheme["parser"]
    compact_value = 0
    total_bits = 0
    scratch = bytearray(8)
    for i, (bits, parser_type, *args) in enumerate(scheme["fields"]):
        item = values[i] if values and i < len(values) else None
        SERIALIZERS[parser_type](scratch, 0, item, *args)
        mask = (1 << bits) - 1
        field_value, _ = PARSERS[compact_parser](scratch, 0, *compact_args)
        compact_value |= (field_value & mask) << total_bits
        total_bits += bits
    return SERIALIZERS[compact_parser](buf, offset, compact_value, *compact_args)


def _serialize_u32buffer(buf, offset, value):
    payload = value or b""
    _struct.pack_into("<I", buf, offset, len(payload))
    offset += 4
    buf[offset : offset + len(payload)] = payload
    return offset + len(payload)


SERIALIZERS = {
    "uint8": _serialize_uint(1),
    "uint16": _serialize_uint(2),
    "uint32": _serialize_uint(4),
    "bool": _serialize_bool,
    "enum": _serialize_enum,
    "str": _serialize_str,
    "array": _serialize_array,
    "nArray": _serialize_narray,
    "struct": _serialize_struct,
    "compact": _serialize_compact,
    "u32Buffer": _serialize_u32buffer,
}


# --- wire schemes (byte-exact mirror of Parsers.mjs) ---

MIDI_SCHEME = {"fields": [("channel", "uint8")]}
# Config::ExpressionConfig -- present so set-config writes all 6 bytes the firmware
# reinterpret_casts as a Config; sending only version+channel would leave the
# pedal's expression calibration to whatever trailed in the request buffer.
EXPRESSION_CAL_SCHEME = {"fields": [("minRaw", "uint16"), ("maxRaw", "uint16")]}
CONFIG_SCHEME = {
    "fields": [
        ("version", "uint8"),
        ("midi", "struct", MIDI_SCHEME),
        ("expression", "struct", EXPRESSION_CAL_SCHEME),
    ]
}

# Bit 3 of the low nibble marks "follow Config.midi.channel" (the pedal's global
# MIDI channel) instead of the explicit channel in the high nibble; see
# kGlobalChannelMask in firmware/src/config/config.h. The flag is parsed/serialized
# as 'uint8', not 'bool': _is_empty() treats False as empty and _serialize_compact
# only pushes non-empty positional values, so a false flag would shift the
# channel into its slot. Numeric 0 survives.
TYPE_AND_CHANNEL_SCHEME = {
    "parser": ("uint8",),
    "fields": [(3, "enum", MESSAGE_TYPES), (1, "uint8"), (4, "uint8")],
}
ACTION_SCHEME = {
    "fields": [
        (("type", "globalChannel", "channel"), "compact", TYPE_AND_CHANNEL_SCHEME),
        ("values", "array", 2, "uint8"),
    ]
}

FOOTSWITCH_SCHEME = {
    "fields": [
        ("onActions", "nArray", MAX_ACTIONS, "struct", ACTION_SCHEME),
        ("offActions", "nArray", MAX_ACTIONS, "struct", ACTION_SCHEME),
        ("name", "str", MAX_FS_NAME_SIZE),
        ("color", "enum", COLORS),
        ("enabled", "bool"),
        ("mode", "enum", FS_MODES),
    ],
    "valid": lambda o: bool(o.get("name")),
}

NAMES_SCHEME = {
    "fields": [
        ("id", "uint8"),
        ("names", "nArray", MAX_NAMES_RESPONSE, "str", MAX_PRG_NAME_SIZE),
    ]
}

# Same low-nibble split as TYPE_AND_CHANNEL_SCHEME, for the expression pedal channel.
MODE_AND_CHANNEL_SCHEME = {
    "parser": ("uint8",),
    "fields": [(3, "enum", MODES), (1, "uint8"), (4, "uint8")],
}

PROGRAM_SCHEME = {
    "fields": [
        ("name", "str", MAX_PRG_NAME_SIZE),
        ("fs", "nArray", MAX_SWITCHES, "struct", FOOTSWITCH_SCHEME),
        ("actions", "nArray", MAX_ACTIONS, "struct", ACTION_SCHEME),
        (("mode", "expGlobalChannel", "expChannel"), "compact", MODE_AND_CHANNEL_SCHEME),
        ("expression", "uint8"),
    ],
    "valid": lambda o: bool(o.get("name")),
}

ID_PLUS_PROGRAM_SCHEME = {"fields": [("id", "uint8"), ("program", "struct", PROGRAM_SCHEME)]}

SETLIST_SCHEME = {
    "fields": [
        ("name", "str", MAX_PRG_NAME_SIZE),
        ("programs", "nArray", MAX_PROGRAMS, "uint8"),
    ],
    "valid": lambda o: bool(o.get("name")),
}

ID_PLUS_SETLIST_SCHEME = {"fields": [("id", "uint8"), ("setlist", "struct", SETLIST_SCHEME)]}

ADDRESS_AND_LENGTH_SCHEME = {"fields": [("address", "uint32"), ("length", "uint32")]}
ADDRESS_AND_PAYLOAD_SCHEME = {"fields": [("address", "uint32"), ("payload", "u32Buffer")]}


def _parse_buffer(buffer: bytes, scheme) -> Tuple[Optional[dict], int]:
    return PARSERS["struct"](buffer, 0, scheme)


def _serialize_buffer(value: dict, scheme) -> bytes:
    buf = bytearray(512)
    length = SERIALIZERS["struct"](buf, 0, value, scheme)
    return bytes(buf[:length])


# --- public API: parse/serialize into/from the typed models ---


def parse_config(buffer: bytes) -> Optional[Config]:
    wire, _ = _parse_buffer(buffer, CONFIG_SCHEME)
    return from_wire(Config, wire) if wire is not None else None


def serialize_config(config: Config) -> bytes:
    return _serialize_buffer(to_wire(config), CONFIG_SCHEME)


def parse_names(buffer: bytes) -> dict:
    wire, _ = _parse_buffer(buffer, NAMES_SCHEME)
    return wire or {"id": 0, "names": []}


def parse_program(buffer: bytes) -> Tuple[Optional[int], Optional[Program]]:
    wire, _ = _parse_buffer(buffer, ID_PLUS_PROGRAM_SCHEME)
    if wire is None:
        return None, None
    program_wire = wire.get("program")
    program = from_wire(Program, program_wire) if program_wire is not None else None
    return wire.get("id"), program


def serialize_program(id: int, program: Program) -> bytes:
    return _serialize_buffer({"id": id, "program": to_wire(program)}, ID_PLUS_PROGRAM_SCHEME)


def parse_setlist(buffer: bytes) -> Tuple[Optional[int], Optional[Setlist]]:
    wire, _ = _parse_buffer(buffer, ID_PLUS_SETLIST_SCHEME)
    if wire is None:
        return None, None
    setlist_wire = wire.get("setlist")
    setlist = from_wire(Setlist, setlist_wire) if setlist_wire is not None else None
    return wire.get("id"), setlist


def serialize_setlist(id: int, setlist: Setlist) -> bytes:
    return _serialize_buffer({"id": id, "setlist": to_wire(setlist)}, ID_PLUS_SETLIST_SCHEME)


def parse_addr_payload(buffer: bytes) -> Tuple[int, bytes]:
    wire, _ = _parse_buffer(buffer, ADDRESS_AND_PAYLOAD_SCHEME)
    return wire["address"], bytes(wire.get("payload") or b"")


def serialize_addr_payload(address: int, payload: bytes) -> bytes:
    return _serialize_buffer({"address": address, "payload": bytes(payload)}, ADDRESS_AND_PAYLOAD_SCHEME)


def serialize_addr_length(address: int, length: int) -> bytes:
    return _serialize_buffer({"address": address, "length": length}, ADDRESS_AND_LENGTH_SCHEME)
