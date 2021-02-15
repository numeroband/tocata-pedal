#pragma once

#include <cstdint>

namespace tocata
{
class MidiUsb
{
public:
  void init() {}
  void run() {}
	void sendProgram(uint8_t program);
	void sendControl(uint8_t control, uint8_t value);
};

}


