#include "switches.h"

#include <cstdio>

namespace tocata {

void Switches::init()
{
    switches_init(_config);
}

void Switches::run()
{
    uint32_t now = millis();
    if (switches_changed(_config))
    {
        Mask old_state = _state;
        _state = switches_value(_config);
        Mask changed = _state ^ old_state;
        Mask old_debouncing = _debouncing;
        Mask start_debouncing = ~old_debouncing & changed;
        _debouncing = old_debouncing | start_debouncing;
        _debouncing_state = _debouncing_state | (_state & start_debouncing);

        if (start_debouncing.any())
        {
            _debouncing_start = now;
        }
    }

    if (!_debouncing.any() || (now - _debouncing_start) < kDebounceMs)
    {
        return;
    }

    if (_callback)
    {
        _callback(_debouncing_state, _debouncing);
    }
    printf("%u: sw %03X(%03X)\n", 
        now, 
        static_cast<uint16_t>(_debouncing_state.to_ulong()), 
        static_cast<uint16_t>(_debouncing.to_ulong()));

    // If the current state is different than the state at the start of the debouncing 
    // we start a new debounce with the new state
    _debouncing = _debouncing_state ^ (_state & _debouncing);
    _debouncing_state = (_state & _debouncing);
    _debouncing_start = now;
}

} // namespace tocata
