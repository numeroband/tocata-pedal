#include "midi_usb.h"
#include "hal.h"
#include <array>
#include "midi_sysex.h"

namespace tocata
{

void MidiUsb::sendProgram(uint8_t channel, uint8_t program)
{
  _write_offset = 0;
  usb_midi_write(0xC0 | (channel & 0x0F), program);
}

void MidiUsb::sendControl(uint8_t channel, uint8_t control, uint8_t value)
{
  _write_offset = 0;
  usb_midi_write(0xB0 | (channel & 0x0F), control, value);
}

void MidiUsb::sendSysEx(std::span<const uint8_t> sysex)
{
  printf("sending sysex %zu bytes [%02X %02X ... %02X]\n", 
    sysex.size(), sysex[0], sysex[1], sysex[sysex.size() - 1]);
  std::copy(sysex.begin(), sysex.end(), _buffer.begin());
  _write_size = sysex.size();
  _write_offset = 0;
  sendBytes();
}

void MidiUsb::run() 
{
#ifdef HAL_PICO
  uint32_t available = 0;
  while ((available = tud_midi_available())) {
    _write_size = _write_offset = 0;
    auto bytes_read = tud_midi_stream_read(_buffer.data() + _read_offset, readAvailable());
    if (bytes_read == 0) {
      break;
    }
    printf("MIDI IN(%u): %02X %02X..%02X\n", 
      bytes_read, 
      _buffer[_read_offset + 0], 
      _buffer[_read_offset + 1], 
      _buffer[_read_offset + bytes_read - 1]);
    
    if (_read_offset == 0 && _buffer[0] != 0xF0) {
      _callback({_buffer.data(), bytes_read}, _buffer, *this);
      break;      
    }
    
    _read_offset += bytes_read;
    if (_buffer[_read_offset - 1] == 0xF7) {
      _callback({_buffer.data(), _read_offset}, _buffer, *this);
      _read_offset = 0;
    }
  }
  sendBytes();
#endif
}

void MidiUsb::sendBytes()
{
  if (writePending() == 0) {
    return;
  }

  size_t sent;
  while ((sent = usb_midi_write(_buffer.data() + _write_offset, writePending()))) {
    printf("MIDI OUT(%u): %02X %02X..%02X\n", 
      (uint32_t)sent, 
      _buffer[_write_offset + 0], 
      _buffer[_write_offset + 1], 
      _buffer[_write_offset + sent - 1]);

    _write_offset += sent;
  }

  if (writePending() == 0) {
    printf("midi sysex with %zu bytes sent\n", _write_size);
    _write_offset = 0;
    _write_size = 0;
  }
}

}