#include "midi_usb.h"
#include "hal.h"

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

void MidiUsb::run() 
{
#ifdef HAL_PICO
  uint8_t packet[4];
  while (tud_midi_available()) {
    if (tud_midi_packet_read(packet)) {
      printf("MIDI IN: %02X %02X %02X %02X\n", 
        packet[0], packet[1], packet[2], packet[3]);
      _callback(packet + 1);
    }
  }
#endif
}

}