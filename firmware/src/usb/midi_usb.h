#pragma once

#include <cstdint>
#include <functional>
#include "midi_sender.h"

namespace tocata
{
class MidiUsb : public MidiSender
{
public:
  using Callback = std::function<void(std::span<uint8_t>, MidiSender&)>;
  void init() {}
  void run();
	void sendProgram(uint8_t channel, uint8_t program) override;
	void sendControl(uint8_t channel, uint8_t control, uint8_t value) override;
	void sendSysEx(std::span<uint8_t> sysex) override;
  void setCallback(Callback callback) { _callback = callback; }

private:
  Callback _callback{};
};

}


