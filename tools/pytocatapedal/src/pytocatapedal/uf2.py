"""UF2 firmware image parsing, mirroring web/src/api/UF2.mjs."""
import struct as _struct
from dataclasses import dataclass

BLOCK_SIZE = 512

FIRST_MAGIC = 0x0A324655
SECOND_MAGIC = 0x9E5D5157
FINAL_MAGIC = 0x0AB16F30


@dataclass
class Block:
    flags: int
    family_id: int
    address: int
    payload: bytes


class UF2:
    FAMILY_RP2040 = 0xE48BFF56
    FAMILY_RP2XXX_ABSOLUTE = 0xE48BFF57
    FAMILY_RP2035_ARM_S = 0xE48BFF59

    def __init__(self, buffer: bytes):
        if len(buffer) == 0 or (len(buffer) % BLOCK_SIZE) != 0:
            raise ValueError("Invalid UF2 size")
        self.buffer = buffer
        num_blocks = len(buffer) // BLOCK_SIZE
        self.flash_start = float("inf")
        self.flash_end = 0
        self.blocks = []
        for i in range(num_blocks):
            block = self._get_block(i)
            if block.family_id != UF2.FAMILY_RP2XXX_ABSOLUTE:
                self.flash_start = min(self.flash_start, block.address)
                self.flash_end = max(self.flash_end, block.address + len(block.payload))
            self.blocks.append(block)

    def _get_block(self, idx: int) -> Block:
        offset = idx * BLOCK_SIZE
        chunk = self.buffer[offset : offset + BLOCK_SIZE]

        first_magic = _struct.unpack_from("<I", chunk, 0)[0]
        if first_magic != FIRST_MAGIC:
            raise ValueError(f"{idx}: Invalid block first magic")
        second_magic = _struct.unpack_from("<I", chunk, 4)[0]
        if second_magic != SECOND_MAGIC:
            raise ValueError(f"{idx}: Invalid block second magic")
        flags = _struct.unpack_from("<I", chunk, 8)[0]
        if (flags & ~0x8000) != 0x2000:
            raise ValueError(f"{idx}: Invalid block flags (0x{flags:x})")
        address = _struct.unpack_from("<I", chunk, 12)[0]
        payload_size = _struct.unpack_from("<I", chunk, 16)[0]
        if payload_size > 476:
            raise ValueError(f"{idx}: Invalid block payload size")
        family_id = _struct.unpack_from("<I", chunk, 28)[0]
        if family_id not in (UF2.FAMILY_RP2040, UF2.FAMILY_RP2035_ARM_S, UF2.FAMILY_RP2XXX_ABSOLUTE):
            raise ValueError(f"{idx}: Invalid block family id (0x{family_id:x})")
        final_magic = _struct.unpack_from("<I", chunk, 508)[0]
        if final_magic != FINAL_MAGIC:
            raise ValueError(f"{idx}: Invalid block final magic")

        return Block(
            flags=flags,
            family_id=family_id,
            address=address,
            payload=bytes(chunk[32 : 32 + payload_size]),
        )
