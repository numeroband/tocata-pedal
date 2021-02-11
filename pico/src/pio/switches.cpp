#include "switches.h"

#include <cstdio>

namespace tocata {

void Switches::init()
{
    switches_init(_config);
}

void Switches::run()
{
    if (switches_changed(_config))
    {
        Mask old_state = _state;
        _state = switches_value(_config);
        if (_state == old_state)
        {
            // Spurious read
            return;
        }
        _delegate.switchesChanged(_state, _state ^ old_state);
    }
}

} // namespace tocata
