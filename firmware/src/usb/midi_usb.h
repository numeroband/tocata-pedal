#pragma once

#include <cstdint>
#include <functional>
#include <cstdint>
#include "midi_sender.h"
#include "midi_sysex.h"

namespace tocata
{
class MidiUsb : public MidiSender
{
public:
  void init() {}
  void run();
	void sendProgram(uint8_t channel, uint8_t program) override;
	void sendControl(uint8_t channel, uint8_t control, uint8_t value) override;
	void sendSysEx(std::span<const uint8_t> sysex) override;
  void setCallback(Callback callback) { _callback = callback; }

private:
  void sendBytes();
  size_t writePending() { return _write_size - _write_offset; }
  size_t readAvailable() { return _buffer.size() - _read_offset; }

  Callback _callback{};
  size_t _write_size{0};
  size_t _write_offset{0};
  size_t _read_offset{0};
  std::array<uint8_t, kMidiSysExMaxSize> _buffer;
};

}


