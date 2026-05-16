#include "switches.h"

#include <cstdio>

namespace tocata {

void Switches::init()
{
    switches_init(_config, kNumSwitches);
}

void Switches::run()
{
    uint32_t now = millis();
    Mask raw_sample = switches_value(_config);


    Mask changed_this_tick;

    for (size_t i = 0; i < kNumSwitches; ++i) {
        // Check if this specific button is currently in a lockout period
        if (now - _last_change_time[i] < kDebounceMs) {
            continue; 
        }

        // Detect immediate change
        if (raw_sample[i] != _stable_states[i]) {
            _stable_states[i] = raw_sample[i];
            _last_change_time[i] = now; // Start the lockout timer
            changed_this_tick.set(i);
        }
    }

    // Fire callback if any button changed and is now stable
    if (changed_this_tick.any() && _callback) {
        _callback(_stable_states, changed_this_tick);
    }
}

} // namespace tocata
