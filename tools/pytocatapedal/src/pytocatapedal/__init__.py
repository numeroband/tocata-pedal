"""Python port of the Tocata Pedal API (web/src/api/*.mjs) and its CLI."""
from importlib.metadata import version as _version

from .api import Api, Command, NUM_PROGRAMS, NUM_SETLISTS
from .midi_sysex import ANY_CHANNEL, NO_CHANNEL, from_sysex, to_sysex
from .models import (
    Action,
    Backup,
    Color,
    Config,
    Footswitch,
    FsMode,
    Midi,
    Mode,
    MessageType,
    Program,
    Setlist,
    from_wire,
    to_wire,
)
from .protocol import Protocol
from .transport_midi import TransportMidi
from .uf2 import UF2

__version__ = _version("pytocatapedal")

__all__ = [
    "Api",
    "Command",
    "NUM_PROGRAMS",
    "NUM_SETLISTS",
    "ANY_CHANNEL",
    "NO_CHANNEL",
    "from_sysex",
    "to_sysex",
    "Action",
    "Backup",
    "Color",
    "Config",
    "Footswitch",
    "FsMode",
    "Midi",
    "Mode",
    "MessageType",
    "Program",
    "Setlist",
    "from_wire",
    "to_wire",
    "Protocol",
    "TransportMidi",
    "UF2",
    "__version__",
]
