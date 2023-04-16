#pragma once

#include "hal.h"

#include <cstdint>
#include <bitset>
#include <array>
#include <functional>

namespace tocata {

class Switches
{
public:
    static constexpr size_t kNumSwitches = 10;
    using Mask = std::bitset<kNumSwitches>;
    using SwitchesChanged = std::function<void(Mask status, Mask modified)>;

    Switches(const HWConfigSwitches& config) : _config(config) {}

    void init();
    void run();
    void setCallback(SwitchesChanged callback) { _callback = callback; }

private:
    static constexpr uint32_t kDebounceMs = 50;

    int _sm;
    Mask _state{};
    Mask _debouncing{};
    Mask _debouncing_state{};
    uint32_t _debouncing_start;
    const HWConfigSwitches& _config;
    SwitchesChanged _callback{};
};

} // namespace tocata
