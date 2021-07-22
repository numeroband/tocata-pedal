#pragma once

#include "hal.h"

#include <cstdint>
#include <bitset>
#include <array>

namespace tocata {

class Switches
{
public:
    static constexpr size_t kNumSwitches = 6;
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
    static constexpr uint32_t kDebounceMs = 50;

    int _sm;
    Mask _state{};
    Mask _debouncing{};
    Mask _debouncing_state{};
    uint32_t _debouncing_start;
    const HWConfigSwitches& _config;
    Delegate& _delegate;
};

} // namespace tocata
