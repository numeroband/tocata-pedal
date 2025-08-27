#pragma once

#include <cstdint>

namespace tocata {

class MidiSender {
public:
	virtual void sendProgram(uint8_t channel, uint8_t program) = 0;
	virtual void sendControl(uint8_t channel, uint8_t control, uint8_t value) = 0;
};

}
