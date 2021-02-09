#pragma once

#include "hardware/pio.h"

#include <cstdint>
#include <bitset>

namespace tocata {

class Switches
{
public:
    struct HWConfig
    {
        int state_machine_id;
        uint first_input_pin;
        uint first_output_pin;
    };

    static constexpr size_t kNumSwitches = 8;
    using Mask = std::bitset<kNumSwitches>;

    class Delegate
    {
    public:
        virtual void switchesChanged(Mask status, Mask modified) = 0;
    };

    Switches(Delegate& delegate) : _delegate(delegate) {}

    void init(const HWConfig& config);
    void run();

private:
    PIO _pio = pio0;
    int _sm;
    Mask _state{};
    Delegate& _delegate;
};

} // namespace tocata
