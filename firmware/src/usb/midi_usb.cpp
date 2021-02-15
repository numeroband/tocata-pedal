#include "midi_usb.h"
#include "hal.h"

namespace tocata
{

void MidiUsb::sendProgram(uint8_t program)
{
  usb_midi_write(0xC0, program);
}

void MidiUsb::sendControl(uint8_t control, uint8_t value)
{
  usb_midi_write(0xB0, control, value);
}

}