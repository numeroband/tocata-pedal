#include "switches.h"
#include "switch_matrix.pio.h"
#include <cstdio>

namespace tocata {

void Switches::init()
{
    uint offset = pio_add_program(_pio, &switch_matrix_program);
    switch_matrix_program_init(_pio, _sm, offset, kFirstInputPin, kFirstOutputPin);
}

void Switches::run()
{
    if (!pio_sm_is_rx_fifo_empty(_pio, _sm))
    {
        uint32_t old_state = _state;
        _state = ~pio_sm_get(_pio, _sm);
        if (_state == old_state)
        {
            // Spurious read
            return;
        }
        printf("Switches change %02X -> %02X\n", old_state, _state);
    }
}

} // namespace tocata
