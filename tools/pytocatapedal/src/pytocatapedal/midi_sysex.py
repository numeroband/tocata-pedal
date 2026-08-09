"""SysEx framing: manufacturer envelope plus 8-bit -> 7-bit packing.

Mirrors web/src/api/MidiSysEx.mjs and firmware/src/midi_sysex.h byte-for-byte.
Both sides must change together -- see the root CLAUDE.md's cross-cutting
contract.
"""
from typing import Optional

SYSEX_PREFIX = bytes([0xF0, 0x00, 0x2F, 0x7F])
SYSEX_SUFFIX = bytes([0xF7])
SYSEX_MIN_SIZE = len(SYSEX_PREFIX) + len(SYSEX_SUFFIX)

ANY_CHANNEL = 0x7F
NO_CHANNEL = -1


def to_sysex(buffer: bytes, channel: int = ANY_CHANNEL) -> bytes:
    has_channel = channel != NO_CHANNEL
    channel_size = 1 if has_channel else 0
    total_bits = len(buffer) * 8
    bytes_required = SYSEX_MIN_SIZE + channel_size + (total_bits + 6) // 7
    sysex = bytearray(bytes_required)
    sysex[: len(SYSEX_PREFIX)] = SYSEX_PREFIX
    offset = len(SYSEX_PREFIX)

    if has_channel:
        sysex[offset] = channel
        offset += 1

    sysex[offset] = 0
    bits = 0

    for b in buffer:
        bits += 1
        sysex[offset] = (sysex[offset] | (b >> bits)) & 0xFF
        offset += 1
        sysex[offset] = (b << (7 - bits)) & 0x7F
        if bits == 7:
            bits = 0
            offset += 1
            sysex[offset] = 0

    if bits != 0:
        offset += 1

    sysex[offset : offset + len(SYSEX_SUFFIX)] = SYSEX_SUFFIX

    return bytes(sysex)


def from_sysex(sysex: bytes, channel: int = ANY_CHANNEL) -> Optional[bytes]:
    has_channel = channel != NO_CHANNEL
    channel_size = 1 if has_channel else 0

    if len(sysex) < SYSEX_MIN_SIZE + channel_size:
        return None
    if bytes(sysex[: len(SYSEX_PREFIX)]) != SYSEX_PREFIX:
        return None

    if has_channel:
        stored_channel = sysex[len(SYSEX_PREFIX)]
        if channel != ANY_CHANNEL and stored_channel != ANY_CHANNEL and stored_channel != channel:
            return None

    content_start = len(SYSEX_PREFIX) + channel_size
    bytes_required = ((len(sysex) - SYSEX_MIN_SIZE - channel_size) * 7) // 8
    content = sysex[content_start : len(sysex) - len(SYSEX_SUFFIX)]
    buffer = bytearray(bytes_required)
    offset = 0
    bits = 0

    for i in range(bytes_required):
        bits += 1
        b = content[offset] << bits
        offset += 1
        b |= content[offset] >> (7 - bits)
        if bits == 7:
            bits = 0
            offset += 1
        buffer[i] = b & 0xFF

    return bytes(buffer)
