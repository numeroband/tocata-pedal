#pragma once

#include <cstdint>

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

#ifdef CYW43_WL_GPIO_LED_PIN
    bool _supported = true;
#else
    bool _supported = false;
#endif
    uint32_t _last_run = 0;
    bool _init = false;
    uint32_t _init_millis;
    AppleMidi& _midi = AppleMidi::sharedInstance();
};

}