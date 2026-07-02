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
    static constexpr size_t kMaxSwitches = 10;
    const uint8_t kNumSwitches = is_pedal_long() ? 10 : 6;
    using Mask = std::bitset<kMaxSwitches>;
    using SwitchesChanged = std::function<void(Mask status, Mask modified)>;

    Switches(const HWConfigSwitches& config) : _config(config) {}

    void init();
    void run();
    // Installing a new callback (i.e. switching modes) always turns the detection
    // delay off; a mode that wants it must re-enable it after this call.
    void setCallback(SwitchesChanged callback) { _callback = callback; setDetectionDelay(false); }
    // While enabled, switch changes are held for up to kDetectionDelayMs and any
    // further changes in that window are coalesced into a single callback, so two
    // near-simultaneous presses arrive together. Disabled = immediate, zero latency.
    void setDetectionDelay(bool enable) { _detection_delay = enable; _pending_changed.reset(); }
    Mask rawMask() const;

private:
    static constexpr uint32_t kDebounceMs = 200;
    static constexpr uint32_t kDetectionDelayMs = 75;

    // int _sm;
    // Mask _state{};
    // Mask _debouncing{};
    // Mask _debouncing_state{};
    // uint32_t _debouncing_start;


    const HWConfigSwitches& _config;
    SwitchesChanged _callback{};
    Mask _stable_states;
    std::array<uint32_t, kMaxSwitches> _last_change_time; // Fixed-size array for lockout timestamps
    uint32_t _lockout_duration;
    bool _detection_delay = false;
    Mask _pending_changed{};      // changes accumulated during the current detection window
    uint32_t _window_start = 0;   // millis() of the first change in the current window
};

} // namespace tocata
