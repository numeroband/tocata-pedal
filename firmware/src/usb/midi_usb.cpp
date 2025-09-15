#include "midi_usb.h"
#include "hal.h"
#include <array>
#include "midi_sysex.h"

namespace tocata
{

void MidiUsb::sendProgram(uint8_t channel, uint8_t program)
{
  usb_midi_write(0xC0 | (channel & 0x0F), program);
}

void MidiUsb::sendControl(uint8_t channel, uint8_t control, uint8_t value)
{
  usb_midi_write(0xB0 | (channel & 0x0F), control, value);
}

void MidiUsb::sendSysEx(std::span<uint8_t> sysex)
{
  printf("sending sysex %zu bytes [%02X %02X ... %02X]\n", 
    sysex.size(), sysex[0], sysex[1], sysex[sysex.size() - 1]);
  auto total = 0;
  uint8_t retries = 0;
  while (total < sysex.size()) {
    auto sent = usb_midi_write(sysex.data() + total, sysex.size() - total);
    total += sent;
    // printf("sent sysex %zu/%zu bytes\n", total, sysex.size());
    if (sent == 0) {
      if (++retries > 5) {
        return;
      }
    } else {
      retries = 0;
    }
    usb_run();
  }
}

void MidiUsb::run() 
{
#ifdef HAL_PICO
  std::array<uint8_t, 591> packet;
  uint32_t available = 0;
  while ((available = tud_midi_available())) {
    auto bytes_read = tud_midi_stream_read(packet.data(), available);
    if (bytes_read > 0) {
      printf("MIDI IN(%u): %02X %02X %02X\n", 
        bytes_read, packet[0], packet[1], packet[2]);
      _callback({packet.data(), bytes_read}, *this);
    }
  }
#endif
}

}