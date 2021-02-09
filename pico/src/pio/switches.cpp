#include "switches.h"
#include "switch_matrix.pio.h"
#include <cstdio>

namespace tocata {

void Switches::init(const HWConfig& config)
{
    _sm = config.state_machine_id;
    uint offset = pio_add_program(_pio, &switch_matrix_program);
    switch_matrix_program_init(_pio, _sm, offset, config.first_input_pin, config.first_output_pin);
}

void Switches::run()
{
    if (!pio_sm_is_rx_fifo_empty(_pio, _sm))
    {
        Mask old_state = _state;
        _state = ~pio_sm_get(_pio, _sm);
        if (_state == old_state)
        {
            // Spurious read
            return;
        }
        _delegate.switchesChanged(_state, _state ^ old_state);
    }
}

} // namespace tocata
