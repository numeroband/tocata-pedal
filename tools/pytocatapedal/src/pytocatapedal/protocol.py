"""Message framing/reassembly, mirroring web/src/api/Protocol.mjs.

Every message on the wire is a 4-byte header (uint16 LE length, uint8
command, uint8 status) followed by `length` payload bytes. Unlike the JS
version -- which serializes concurrent callers through an internal Promise
queue -- this is a synchronous, blocking port: a single lock around
send+receive is enough since callers issue one request at a time.
"""
import struct as _struct
import threading
from typing import Tuple

LENGTH_OFFSET = 0
COMMAND_OFFSET = 2
STATUS_OFFSET = 3
MSG_HEADER_SIZE = 4


class Protocol:
    def __init__(self, transport, connection_event=None):
        self.transport = transport
        if connection_event is not None:
            self.transport.connection_event = connection_event
        self._buffer = bytearray()
        self._lock = threading.Lock()

    @property
    def connected(self) -> bool:
        return self.transport.connected

    def connect(self, reconnect: bool = False):
        return self.transport.reconnect() if reconnect else self.transport.connect()

    def version(self):
        return self.transport.version()

    def _receive(self) -> Tuple[int, bytes]:
        total_length = None
        while total_length is None or len(self._buffer) < total_length:
            self._buffer += self.transport.receive()
            if len(self._buffer) > 2:
                length = _struct.unpack_from("<H", self._buffer, LENGTH_OFFSET)[0]
                total_length = MSG_HEADER_SIZE + length

        command = self._buffer[COMMAND_OFFSET]
        status = self._buffer[STATUS_OFFSET]
        # Mirrors the JS side: this is every trailing buffered byte, not just
        # `length` of them -- harmless, since struct parsing only consumes as
        # many bytes as its scheme declares.
        data = bytes(self._buffer[MSG_HEADER_SIZE:])
        self._buffer = self._buffer[total_length:] if len(self._buffer) > total_length else bytearray()

        if status != 0:
            raise RuntimeError(f"Response with status {status}")
        return command, data

    def send_request(self, command: int, data: bytes = b"") -> Tuple[int, bytes]:
        with self._lock:
            header = _struct.pack("<HBB", len(data), command, 0)
            self.transport.send(header + bytes(data))
            return self._receive()
