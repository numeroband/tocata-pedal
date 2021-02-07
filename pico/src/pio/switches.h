#pragma once

#include <cstdint>

#include "hardware/pio.h"

namespace tocata {

class Switches
{
public:
    void init();
    void run();

private:
    static constexpr int kStateMachineId = 0;
    static constexpr uint kFirstInputPin = 2;
    static constexpr uint kFirstOutputPin = 6;

    PIO _pio = pio0;
    int _sm = kStateMachineId;
    uint32_t _state = 0;
};

} // namespace tocata
