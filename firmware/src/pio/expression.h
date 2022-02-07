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
    void incFilter();
    void decFilter();
    uint8_t getValue() { return _currentValue; };
    void setCallback(Callback callback) { _callback = callback; }
    void resetMax() { _maxRaw = _currentRaw; }
    void resetMin() { _minRaw = _currentRaw; }

private:
    static constexpr int16_t kMinValue = 0;
    static constexpr int16_t kMaxValue = 127;
    static constexpr int16_t kMaxFilter = 3;
    static constexpr int16_t kDefaultFilter = 1;

    uint8_t calculateValue();

    const HWConfigExpression& _config;
    uint16_t _currentRaw;
    uint8_t _currentValue = 0;
    int16_t  _filterRadius = kDefaultFilter;
    int16_t  _filterCenter = kMinValue - _filterRadius;
    uint16_t _maxRaw;
    uint16_t _minRaw;
    Callback _callback = nullptr;
};

} // namespace tocata