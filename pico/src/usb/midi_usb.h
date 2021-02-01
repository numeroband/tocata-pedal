#pragma once

#include <cstdint>

namespace tocata
{
class MidiUsb
{
public:
  void init() {}
  void run();

private:
  // Store example melody as an array of note values
  static constexpr uint8_t note_sequence[] =
  {
    74,78,81,86,90,93,98,102,57,61,66,69,73,78,81,85,88,92,97,100,97,92,88,85,81,78,
    74,69,66,62,57,62,66,69,74,78,81,86,90,93,97,102,97,93,90,85,81,78,73,68,64,61,
    56,61,64,68,74,78,81,86,90,93,98,102
  };

  uint32_t _note_pos = 0;
  uint32_t _start_ms = 0;
};

}


