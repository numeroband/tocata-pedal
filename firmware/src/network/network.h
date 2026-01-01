#pragma once

#include "hal.h"
#include "midi_sender.h"
#include "dhcp_client.h"
#include <cstdint>

#ifdef PICO_BUILD

#include "apple_midi.h"

namespace tocata {

class Network {
public:
    Network(const HWConfigEthernet& config) : _config{config} {}
    void init();
    void run();
    MidiSender& midi() { return _midi; }
    const MidiSender& midi() const { return _midi; }

private:
    const HWConfigEthernet& _config;
    AppleMidi& _midi = AppleMidi::sharedInstance();
    DHCPClient _dhcp;
};

} // namespace tocata

#else // PICO_BUILD

namespace tocata {
class Network {
private:
    class DummyMidi : public MidiSender {
	    void sendProgram(uint8_t channel, uint8_t program) override {}
	    void sendControl(uint8_t channel, uint8_t control, uint8_t value) override {}
    	void sendSysEx(std::span<const uint8_t> sysex) override {}
        void setCallback(Callback callback) override {}
    };
public:
    void init() {}
    void run() {}
    MidiSender& midi() { return _midi; }
    const MidiSender& midi() const { return _midi; }
    DummyMidi _midi;
};
}

#endif // PICO_BUILD
