"""Typed data model for the config/program/setlist structs.

These mirror the JS objects produced/consumed by web/src/api/Parsers.mjs.
`to_wire()`/`from_wire()` convert between these dataclasses and plain
dicts/lists using the exact same field names and nesting the JS side uses
(camelCase where the JS uses it, e.g. `expChannel`), so a JSON file written
by this package's CLI is interchangeable with one written by
web/src/api/cli.mjs (and vice versa).
"""
import dataclasses
import typing
from dataclasses import dataclass, field
from enum import Enum
from typing import List, Optional


class MessageType(str, Enum):
    PC = "PC"
    CC = "CC"
    NOTE_ON = "NO"
    NOTE_OFF = "NF"


class Color(str, Enum):
    BLUE = "blue"
    PURPLE = "purple"
    RED = "red"
    YELLOW = "yellow"
    GREEN = "green"
    TURQUOISE = "turquoise"


class Mode(str, Enum):
    DEFAULT = "default"
    SCENE = "scene"


class FsMode(str, Enum):
    STOMP = "stomp"
    MOMENTARY = "momentary"
    SCENE = "scene"
    PROGRAM = "program"


@dataclass
class Midi:
    channel: int = 0


@dataclass
class ExpressionCal:
    """Config::ExpressionConfig -- raw ADC calibration, not the per-program expression CC."""

    min_raw: int = field(default=0, metadata={"wire": "minRaw"})
    max_raw: int = field(default=4095, metadata={"wire": "maxRaw"})


@dataclass
class Config:
    version: int = 0
    midi: Midi = field(default_factory=Midi)
    expression: ExpressionCal = field(default_factory=ExpressionCal)


@dataclass
class Action:
    type: Optional[MessageType] = None
    # Follow Config.midi.channel instead of `channel` below; see kGlobalChannelMask
    # in firmware/src/config/config.h. Kept as an int (not bool) so it survives
    # parsers._is_empty(), which treats False as an absent value.
    global_channel: int = field(default=0, metadata={"wire": "globalChannel"})
    channel: int = 0
    values: List[int] = field(default_factory=lambda: [0, 0])


@dataclass
class Footswitch:
    on_actions: List[Action] = field(default_factory=list, metadata={"wire": "onActions"})
    off_actions: List[Action] = field(default_factory=list, metadata={"wire": "offActions"})
    name: str = ""
    color: Optional[Color] = None
    enabled: bool = False
    mode: Optional[FsMode] = None


@dataclass
class Program:
    name: str = ""
    fs: List[Optional[Footswitch]] = field(default_factory=list)
    actions: List[Action] = field(default_factory=list)
    mode: Mode = Mode.DEFAULT
    # Same global-channel flag as Action.global_channel, for the expression pedal.
    exp_global_channel: int = field(default=0, metadata={"wire": "expGlobalChannel"})
    exp_channel: int = field(default=0, metadata={"wire": "expChannel"})
    expression: int = 0


@dataclass
class Setlist:
    name: str = ""
    programs: List[int] = field(default_factory=list)


@dataclass
class Backup:
    """Config plus every stored program/setlist -- the shape of a backup JSON file."""

    version: int = 0
    midi: Midi = field(default_factory=Midi)
    expression: ExpressionCal = field(default_factory=ExpressionCal)
    programs: List[Optional[Program]] = field(default_factory=list)
    setlists: List[Optional[Setlist]] = field(default_factory=list)


def _unwrap_optional(tp):
    if typing.get_origin(tp) is typing.Union:
        args = [a for a in typing.get_args(tp) if a is not type(None)]
        if len(args) == 1:
            return args[0], True
    return tp, False


def to_wire(value):
    """Convert a dataclass instance (recursively) into a plain JSON-able dict/list."""
    if value is None:
        return None
    if isinstance(value, Enum):
        return value.value
    if dataclasses.is_dataclass(value):
        result = {}
        for f in dataclasses.fields(value):
            key = f.metadata.get("wire", f.name)
            result[key] = to_wire(getattr(value, f.name))
        return result
    if isinstance(value, (list, tuple)):
        return [to_wire(v) for v in value]
    return value


def from_wire(cls, data):
    """Build a dataclass instance of `cls` (recursively) from a plain dict/list."""
    return _convert_field(data, cls)


def _convert_field(raw, field_type):
    if raw is None:
        return None
    field_type, _ = _unwrap_optional(field_type)
    origin = typing.get_origin(field_type)
    if origin is list:
        (item_type,) = typing.get_args(field_type) or (object,)
        return [_convert_field(v, item_type) for v in raw]
    if isinstance(field_type, type) and issubclass(field_type, Enum):
        return field_type(raw)
    if dataclasses.is_dataclass(field_type):
        if not isinstance(raw, dict):
            return None
        hints = typing.get_type_hints(field_type)
        kwargs = {}
        for f in dataclasses.fields(field_type):
            key = f.metadata.get("wire", f.name)
            if key not in raw:
                # Absent key: the wire parser omits fields it considers
                # "empty" (see parsers._is_empty), so fall back to this
                # dataclass field's own default instead of forcing None.
                continue
            kwargs[f.name] = _convert_field(raw.get(key), hints[f.name])
        return field_type(**kwargs)
    return raw
