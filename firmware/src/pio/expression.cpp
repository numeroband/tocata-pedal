#include "expression.h"

namespace tocata {

void Expression::init() {
    _minRaw = 0;
    _maxRaw = (1 << 12) - 1;
    expression_init(_config);
}

void Expression::run() {
    _currentRaw = expression_read();

    uint8_t value = calculateValue();
    if (value != _currentValue) {
        _currentValue = value;
        if (_callback) {
            _callback(_currentValue);
        }
    }
}

uint8_t Expression::calculateValue() {
    if (_currentRaw < _minRaw) {
        _filterCenter = kMinValue - _filterRadius;
        return static_cast<uint8_t>(kMinValue);
    }

    if (_currentRaw >= _maxRaw) {
        _filterCenter = kMaxValue + _filterRadius;
        return static_cast<uint8_t>(kMaxValue);
    }

    float floatValue = static_cast<float>(_currentRaw - _minRaw);
    floatValue /= static_cast<float>((_maxRaw - _minRaw));
    floatValue *= static_cast<float>((kMaxValue - kMinValue) + 1);
    int16_t intValue = static_cast<int16_t>(floatValue);

    if (intValue > (_filterCenter + _filterRadius)) {
        _filterCenter = intValue - _filterRadius;        
    } else if (intValue < (_filterCenter - _filterRadius)) {
        _filterCenter = intValue + _filterRadius;
    } else {
        intValue = _currentValue;
    }

    return static_cast<uint8_t>(intValue);
}

void Expression::incFilter() { 
    _filterRadius += (_filterRadius < kMaxFilter) ? 1 : 0;
    printf("Exp filter: %d\n", _filterRadius);
}

void Expression::decFilter() { 
    _filterRadius -= (_filterRadius > 0) ? 1 : 0; 
    printf("Exp filter: %d\n", _filterRadius);
}

} // namespace tocata