import struct

import pytest

from pytocatapedal.uf2 import FINAL_MAGIC, FIRST_MAGIC, SECOND_MAGIC, UF2


def build_block(address, payload, family_id, flags=0x2000):
    block = bytearray(512)
    struct.pack_into("<I", block, 0, FIRST_MAGIC)
    struct.pack_into("<I", block, 4, SECOND_MAGIC)
    struct.pack_into("<I", block, 8, flags)
    struct.pack_into("<I", block, 12, address)
    struct.pack_into("<I", block, 16, len(payload))
    struct.pack_into("<I", block, 28, family_id)
    block[32 : 32 + len(payload)] = payload
    struct.pack_into("<I", block, 508, FINAL_MAGIC)
    return bytes(block)


def test_parses_blocks_and_computes_flash_range():
    b0 = build_block(0x10000000, b"\x01" * 256, UF2.FAMILY_RP2040)
    b1 = build_block(0x10000100, b"\x02" * 100, UF2.FAMILY_RP2040)
    # An absolute-family block sits outside the flashable range and must be
    # excluded from flash_start/flash_end.
    b2 = build_block(0x00000000, b"\x03" * 4, UF2.FAMILY_RP2XXX_ABSOLUTE)
    uf2 = UF2(b0 + b1 + b2)

    assert len(uf2.blocks) == 3
    assert uf2.blocks[0].address == 0x10000000
    assert uf2.blocks[0].payload == b"\x01" * 256
    assert uf2.blocks[2].family_id == UF2.FAMILY_RP2XXX_ABSOLUTE
    assert uf2.flash_start == 0x10000000
    assert uf2.flash_end == 0x10000100 + 100


def test_rejects_bad_size():
    with pytest.raises(ValueError):
        UF2(b"\x00" * 100)


def test_rejects_bad_magic():
    block = bytearray(build_block(0x10000000, b"\x01", UF2.FAMILY_RP2040))
    block[0] = 0
    with pytest.raises(ValueError):
        UF2(bytes(block))


def test_rejects_oversized_payload():
    block = bytearray(build_block(0x10000000, b"\x01" * 10, UF2.FAMILY_RP2040))
    struct.pack_into("<I", block, 16, 477)
    with pytest.raises(ValueError):
        UF2(bytes(block))


def test_rejects_bad_flags():
    block = build_block(0x10000000, b"\x01", UF2.FAMILY_RP2040, flags=0x1000)
    with pytest.raises(ValueError):
        UF2(block)
