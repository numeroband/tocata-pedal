#pragma once

#include <cstdint>
#include <functional>
#include <span>

namespace tocata {

class MidiSender {
public:
	using Callback = std::function<void(std::span<const uint8_t>, std::span<uint8_t>, MidiSender& sender)>;
	virtual void sendProgram(uint8_t channel, uint8_t program) = 0;
	virtual void sendControl(uint8_t channel, uint8_t control, uint8_t value) = 0;
	virtual void sendSysEx(std::span<const uint8_t> sysex) = 0;
	virtual void setCallback(Callback callback) = 0;
};

}
