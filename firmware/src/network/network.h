#pragma once

#include "midi_sender.h"
#include <cstdint>

#ifdef PICO_BUILD

#include "apple_midi.h"

namespace tocata {

class Network {
public:
    void init();
    void run();
    MidiSender& midi() { return _midi; }
    const MidiSender& midi() const { return _midi; }

private:
    void postInit();
    void runDHCP();

// #ifdef CYW43_WL_GPIO_LED_PIN
#if 0
    bool _supported = true;
#else
    bool _supported = false;
#endif
    uint32_t _last_run = 0;
    bool _init = false;
    uint32_t _init_millis;
    AppleMidi& _midi = AppleMidi::sharedInstance();
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
