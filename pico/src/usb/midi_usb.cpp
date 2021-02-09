#include "midi_usb.h"

#include <pico/time.h>
#include <tusb.h>

namespace tocata
{

void MidiUsb::run(void)
{
  // send note every 1000 ms
  uint32_t now = to_ms_since_boot(get_absolute_time());
  if (now - _start_ms < 1000) return; // not enough time
  _start_ms = now;

  // Previous positions in the note sequence.
  int previous = _note_pos - 1;

  // If we currently are at position 0, set the
  // previous position to the last note in the sequence.
  if (previous < 0) previous = sizeof(note_sequence) - 1;

  // Send Note On for current position at full velocity (127) on channel 1.
  tudi_midi_write24(0, 0x90, note_sequence[_note_pos], 127);

  // Send Note Off for previous note.
  tudi_midi_write24(0, 0x80, note_sequence[previous], 0);

  // Increment position
  _note_pos++;

  // If we are at the end of the sequence, start over.
  if (_note_pos >= sizeof(note_sequence)) _note_pos = 0;
}

}