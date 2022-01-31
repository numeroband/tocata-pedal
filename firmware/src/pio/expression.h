#pragma once

#include "hal.h"
#include "config.h"

#include <cstdint>
#include <bitset>
#include <functional>

namespace tocata {

class Expression {
public:
    using Callback = std::function<void(uint16_t)>;

    Expression(const HWConfigExpression& config) : _config(config) {}
    void init();
    void run();
    uint8_t getValue() { return _currentValue; };
    void setCallback(Callback callback) { _callback = callback; }
    void resetMax() { _maxRaw = _currentRaw; }
    void resetMin() { _minRaw = _currentRaw; }

private:
    static constexpr uint8_t kMinValue = 0;
    static constexpr uint8_t kMaxValue = 127;

    uint8_t calculateValue();

    const HWConfigExpression& _config;
    uint16_t _currentRaw;
    uint8_t _currentValue;
    uint16_t _maxRaw;
    uint16_t _minRaw;
    Callback _callback = nullptr;
};

} // namespace tocata