#include "midi_usb.h"
#include "hal.h"
#include <array>
#include "midi_sysex.h"

namespace tocata
{

namespace
{

// Total length of a non-SysEx message starting with `status`, or 0 for bytes
// that can't start one (stray data bytes, an unmatched F7, undefined status).
size_t messageLength(uint8_t status)
{
  if (status < 0x80) {
    return 0;
  }
  if (status < 0xF0) {
    uint8_t type = status & 0xF0;
    return (type == 0xC0 || type == 0xD0) ? 2 : 3;
  }
  switch (status) {
    case 0xF1:
    case 0xF3:
      return 2;
    case 0xF2:
      return 3;
    case 0xF6:
    case 0xF8:
    case 0xFA:
    case 0xFB:
    case 0xFC:
    case 0xFE:
    case 0xFF:
      return 1;
    default:
      return 0;
  }
}

}

void MidiUsb::sendProgram(uint8_t channel, uint8_t program)
{
  usb_midi_write(0xC0 | (channel & 0x0F), program);
}

void MidiUsb::sendControl(uint8_t channel, uint8_t control, uint8_t value)
{
  usb_midi_write(0xB0 | (channel & 0x0F), control, value);
}

void MidiUsb::sendSysEx(std::span<const uint8_t> sysex)
{
  // Never cut a SysEx that is already on its way out: the host would see it
  // truncated. If the previous one still can't be flushed, drop this one.
  sendBytes();
  if (writePending() != 0) {
    return;
  }
  std::copy(sysex.begin(), sysex.end(), _write_buffer.begin());
  _write_size = sysex.size();
  _write_offset = 0;
  sendBytes();
}

void MidiUsb::run()
{
  // A reply that didn't fit in the USB FIFO must go out in full before the
  // next request is handled, so hold off reading until it has.
  sendBytes();
  while (writePending() == 0) {
    uint8_t byte;
    if (usb_midi_stream_read(&byte, 1) == 0) {
      break;
    }

    // A new F0 always starts a fresh SysEx, discarding any partial one whose
    // F7 never arrived, so a stale fragment can't corrupt the next request.
    if (byte == 0xF0) {
      _read_buffer[0] = byte;
      _read_offset = 1;
      continue;
    }

    if (_read_offset > 0) {
      if (byte >= 0xF8) {
        continue;  // Real-time bytes may be interleaved in a SysEx.
      }
      if (byte & 0x80 && byte != 0xF7) {
        _read_offset = 0;  // Any other status byte aborts the SysEx.
      } else if (_read_offset == _read_buffer.size()) {
        // Over-long: drop it. Its remaining data bytes and F7 are discarded
        // below as stray bytes.
        _read_offset = 0;
        continue;
      } else {
        _read_buffer[_read_offset++] = byte;
        if (byte == 0xF7) {
          size_t size = _read_offset;
          _read_offset = 0;
          _callback({_read_buffer.data(), size}, _write_buffer, *this);
        }
        continue;
      }
    }

    size_t length = messageLength(byte);
    if (length == 0) {
      continue;
    }
    _read_buffer[0] = byte;
    size_t got = 1;
    while (got < length) {
      auto bytes_read = usb_midi_stream_read(_read_buffer.data() + got, length - got);
      if (bytes_read == 0) {
        break;
      }
      got += bytes_read;
    }
    if (got == length) {
      _callback({_read_buffer.data(), length}, _write_buffer, *this);
    }
  }
}

void MidiUsb::sendBytes()
{
  if (writePending() == 0) {
    return;
  }

  size_t sent;
  while (writePending() && (sent = usb_midi_write(_write_buffer.data() + _write_offset, writePending()))) {
    _write_offset += sent;
  }

  if (writePending() == 0) {
    _write_offset = 0;
    _write_size = 0;
  }
}

}
