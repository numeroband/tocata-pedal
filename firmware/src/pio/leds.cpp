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
    setColor(led, 
        brightness(rgb.r, active),
        brightness(rgb.g, active),
        brightness(rgb.b, active));
}

void Leds::setColor(uint8_t led, uint8_t r, uint8_t g, uint8_t b)
{
    led = _config.map[led];
    _state[led] = (g << 24) | (r << 16) | (b << 8);
}

void Leds::refresh()
{
    _refresh_pending = true;
}

void Leds::run()
{
    if (!_refresh_pending || !_timer.expired()) {
        return;
    }

    _refresh_pending = false;
    leds_refresh(_config, _state, kNumLeds);
    _timer.start(1);
}

} // namespace tocata
