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

    if (!_detection_delay) {
        // Immediate path: fire on any change, no added latency.
        if (changed_this_tick.any() && _callback) {
            _callback(_stable_states, changed_this_tick);
        }
        return;
    }

    // Detection-delay path: hold changes for up to kDetectionDelayMs, coalescing
    // any further changes so two near-simultaneous presses arrive in one callback.
    // The window starts on the first change and does not extend, bounding the added
    // latency to kDetectionDelayMs.
    if (changed_this_tick.any()) {
        if (_pending_changed.none()) {
            _window_start = now;
        }
        _pending_changed |= changed_this_tick;
    }

    if (_pending_changed.any() && (now - _window_start) >= kDetectionDelayMs) {
        Mask to_report = _pending_changed;
        _pending_changed.reset();
        if (_callback) {
            _callback(_stable_states, to_report);
        }
    }
}

Switches::Mask Switches::rawMask() const
{
    return Mask(switches_value(_config));
}

} // namespace tocata
