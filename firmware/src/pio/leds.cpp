#include "leds.h"

#include <cstdio>

namespace tocata {

void Leds::init()
{
    leds_init(_config);
    refresh();
}

void Leds::setColor(uint8_t led, Color color, bool active)
{
    const RGB& rgb = kColors[color];
    const uint8_t r = brightness(rgb.r, active);
    const uint8_t g = brightness(rgb.g, active);
    const uint8_t b = brightness(rgb.b, active);
    _state[_config.mapping[led]] = (g << 24) | (r << 16) | (b << 8);
}

void Leds::refresh()
{
    leds_refresh(_config, _state, kNumLeds);
}

} // namespace tocata
