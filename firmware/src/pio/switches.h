#pragma once

#include "hal.h"

#include <cstdint>
#include <bitset>

namespace tocata {

class Switches
{
public:
    static constexpr size_t kNumSwitches = 8;
    using Mask = std::bitset<kNumSwitches>;

    class Delegate
    {
    public:
        virtual void switchesChanged(Mask status, Mask modified) = 0;
    };

    Switches(const HWConfigSwitches& config, Delegate& delegate) : _config(config), _delegate(delegate) {}

    void init();
    void run();

private:
    int _sm;
    Mask _state{};
    const HWConfigSwitches& _config;
    Delegate& _delegate;
};

} // namespace tocata
