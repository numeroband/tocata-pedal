#include "midi_usb.h"

#include <pico/time.h>
#include <tusb.h>

namespace tocata
{

void MidiUsb::sendProgram(uint8_t program)
{
  uint8_t message[] = { 0xC0, program };
  tud_midi_write(0, message, sizeof(message));
}

void MidiUsb::sendControl(uint8_t control, uint8_t value)
{
  tudi_midi_write24(0, 0xB0, control, value);
}

}