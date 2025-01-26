#pragma once

#include <cstdint>
#include <functional>

namespace tocata
{
class MidiUsb
{
public:
  using Callback = std::function<void(const uint8_t*)>;
  void init() {}
  void run();
	void sendProgram(uint8_t channel, uint8_t program);
	void sendControl(uint8_t channel, uint8_t control, uint8_t value);
  void setCallback(Callback callback) { _callback = callback; }

private:
  Callback _callback{};
};

}


