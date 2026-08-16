"""High-level API surface, mirroring web/src/api/Api.mjs.

Every method is a plain blocking call -- Protocol's internal lock already
serializes access to the MIDI link, so unlike the JS Api there's no
Promise/FIFO request queue to maintain here.
"""
import logging
from enum import IntEnum
from typing import List, Optional

from .models import Backup, Config, FsMode, Mode, Program, Setlist
from .parsers import (
    parse_addr_payload,
    parse_config,
    parse_names,
    parse_program,
    parse_setlist,
    serialize_addr_length,
    serialize_addr_payload,
    serialize_config,
    serialize_program,
    serialize_setlist,
)
from .protocol import Protocol
from .uf2 import UF2

log = logging.getLogger(__name__)


class Command(IntEnum):
    RESTART = 1
    BOOT_ROM = 2
    GET_CONFIG = 3
    SET_CONFIG = 4
    DEL_CONFIG = 5
    GET_NAMES = 6
    GET_PROGRAM = 7
    SET_PROGRAM = 8
    DEL_PROGRAM = 9
    GET_SETLISTS = 0x0A
    GET_SETLIST = 0x0B
    SET_SETLIST = 0x0C
    DEL_SETLIST = 0x0D
    MEM_READ = 0x10
    MEM_WRITE = 0x11
    FLASH_ERASE = 0x12


NUM_PROGRAMS = 99
# Setlist 0 on the pedal is the built-in "All" setlist and is not stored, so
# these ids address the 26 editable setlists (files /64../7D).
NUM_SETLISTS = 26

_CHUNK_SIZE = 500


class Api:
    def __init__(self, transport):
        self.connection_event = None
        self.protocol = Protocol(transport, self._on_connection_event)

    def _on_connection_event(self, connected):
        if self.connection_event:
            self.connection_event(connected)

    @property
    def connected(self) -> bool:
        return self.protocol.connected

    def version(self):
        return self.protocol.version()

    def connect(self, reconnect: bool = False):
        return self.protocol.connect(reconnect)

    def _send_request(self, command: Command, data: bytes = b"") -> bytes:
        if not self.connected:
            raise RuntimeError("Not connected")
        _command, response = self.protocol.send_request(int(command), data)
        return response

    def get_config(self) -> Config:
        log.info("getConfig")
        data = self._send_request(Command.GET_CONFIG)
        return parse_config(data) or Config()

    def set_config(self, config: Config):
        log.info("setConfig")
        self._send_request(Command.SET_CONFIG, serialize_config(config))

    def delete_config(self):
        log.info("deleteConfig")
        self._send_request(Command.DEL_CONFIG)

    def get_program_names(self) -> List[str]:
        log.info("getProgramNames")
        names: List[str] = []
        while len(names) < NUM_PROGRAMS:
            data = self._send_request(Command.GET_NAMES, bytes([len(names)]))
            res = parse_names(data)
            names.extend(res["names"])
        return names

    def get_program(self, id: int) -> Program:
        log.info("getProgram %s", id)
        data = self._send_request(Command.GET_PROGRAM, bytes([id]))
        _pid, program = parse_program(data)
        # Legacy programs stored as whole-program scene mode force every
        # switch to scene; expand that into the per-switch model so it
        # round-trips (and a re-save preserves the scene behavior).
        if program and program.mode == Mode.SCENE:
            program.mode = Mode.DEFAULT
            for fs in program.fs or []:
                if fs:
                    fs.mode = FsMode.SCENE
        return program or Program()

    def set_program(self, id: int, program: Program):
        log.info("setProgram %s", id)
        self._send_request(Command.SET_PROGRAM, serialize_program(id, program))

    def delete_program(self, id: int):
        log.info("deleteProgram %s", id)
        self._send_request(Command.DEL_PROGRAM, bytes([id]))

    def get_setlist_names(self) -> List[str]:
        log.info("getSetlistNames")
        names: List[str] = []
        while len(names) < NUM_SETLISTS:
            data = self._send_request(Command.GET_SETLISTS, bytes([len(names)]))
            res = parse_names(data)
            names.extend(res["names"])
        # The device always replies with a full page of name slots, so the
        # last response overshoots NUM_SETLISTS.
        return names[:NUM_SETLISTS]

    def get_setlist(self, id: int) -> Setlist:
        log.info("getSetlist %s", id)
        data = self._send_request(Command.GET_SETLIST, bytes([id]))
        _sid, setlist = parse_setlist(data)
        return setlist or Setlist()

    def set_setlist(self, id: int, setlist: Setlist):
        log.info("setSetlist %s", id)
        self._send_request(Command.SET_SETLIST, serialize_setlist(id, setlist))

    def delete_setlist(self, id: int):
        log.info("deleteSetlist %s", id)
        self._send_request(Command.DEL_SETLIST, bytes([id]))

    def mem_read(self, address: int, length: int) -> bytes:
        log.info("memRead %x - %d", address, length)
        req = serialize_addr_length(address, length)
        res = self._send_request(Command.MEM_READ, req)
        _addr, payload = parse_addr_payload(res)
        return payload

    def mem_write(self, address: int, payload: bytes):
        log.info("memWrite %x - %d", address, len(payload))
        self._send_request(Command.MEM_WRITE, serialize_addr_payload(address, payload))

    def flash_erase(self, address: int, length: int):
        log.info("flashErase %x - %d", address, length)
        self._send_request(Command.FLASH_ERASE, serialize_addr_length(address, length))

    def restart(self):
        self._send_request(Command.RESTART)

    def boot_rom(self):
        self._send_request(Command.BOOT_ROM)

    def backup(self) -> Backup:
        config = self.get_config()
        names = self.get_program_names()
        programs: List[Optional[Program]] = []
        for id, name in enumerate(names):
            if name:
                while len(programs) <= id:
                    programs.append(None)
                programs[id] = self.get_program(id)
        setlist_names = self.get_setlist_names()
        setlists: List[Optional[Setlist]] = []
        for id, name in enumerate(setlist_names):
            if name:
                while len(setlists) <= id:
                    setlists.append(None)
                setlists[id] = self.get_setlist(id)
        return Backup(
            version=config.version,
            midi=config.midi,
            expression=config.expression,
            programs=programs,
            setlists=setlists,
        )

    def restore(self, backup: Backup):
        for id in range(NUM_PROGRAMS):
            program = backup.programs[id] if id < len(backup.programs) else None
            if program and program.name:
                self.set_program(id, program)
            else:
                self.delete_program(id)
        # Restore is a full device sync, so slots missing from the backup are
        # cleared -- including every setlist when the file predates setlists.
        for id in range(NUM_SETLISTS):
            setlist = backup.setlists[id] if id < len(backup.setlists) else None
            if setlist and setlist.name:
                self.set_setlist(id, setlist)
            else:
                self.delete_setlist(id)
        self.set_config(Config(version=backup.version, midi=backup.midi, expression=backup.expression))

    def factory(self):
        for id in range(NUM_PROGRAMS):
            self.delete_program(id)
        for id in range(NUM_SETLISTS):
            self.delete_setlist(id)
        self.delete_config()

    def flash_firmware(self, uf2: UF2):
        self.flash_erase(uf2.flash_start, uf2.flash_end - uf2.flash_start)
        for block in uf2.blocks:
            if block.family_id != UF2.FAMILY_RP2XXX_ABSOLUTE:
                self.mem_write(block.address, block.payload)

    @staticmethod
    def print_buffer(base: int, buffer: bytes):
        for i in range(0, len(buffer), 16):
            chunk = buffer[i : i + 16]
            line = " ".join(f"{b:02X}" for b in chunk)
            print(f"{base + i:08X}: {line}")

    def verify_firmware(self, uf2: UF2):
        for block in uf2.blocks:
            payload = self.mem_read(block.address, len(block.payload))
            if len(payload) != len(block.payload):
                raise RuntimeError(
                    f"Payload length does not match: local {len(block.payload)} != remote {len(payload)}"
                )
            for i in range(len(block.payload)):
                if block.payload[i] != payload[i]:
                    print("local")
                    self.print_buffer(block.address, block.payload)
                    print("remote")
                    self.print_buffer(block.address, payload)
                    raise RuntimeError(
                        f"Payload difference in addr 0x{block.address + i:x} "
                        f"local 0x{block.payload[i]:x} != remote 0x{payload[i]:x}"
                    )

    def read_memory(self, addr: int, length: int) -> bytes:
        result = bytearray(length)
        for offset in range(0, length, _CHUNK_SIZE):
            to_read = min(_CHUNK_SIZE, length - offset)
            chunk = self.mem_read(addr + offset, to_read)
            result[offset : offset + len(chunk)] = chunk
        return bytes(result)

    def write_memory(self, addr: int, buffer: bytes):
        length = len(buffer)
        for offset in range(0, length, _CHUNK_SIZE):
            to_write = min(_CHUNK_SIZE, length - offset)
            self.mem_write(addr + offset, buffer[offset : offset + to_write])
