#pragma once

#include "hal.h"
#include "config.h"

#include <cstdint>
#include <bitset>

namespace tocata {

class Leds
{
public:
    static constexpr uint8_t kMaxLeds = 8;
    const uint8_t kNumLeds = is_pedal_long() ? 8 : 6;

    static uint8_t fixMapping(uint8_t index) {
        if (is_pedal_long()) {
            constexpr uint8_t mapping[] = {3, 2, 1, 0, 4, 5, 6, 7,};
            return mapping[index];
        } else {
            constexpr uint8_t mapping[] = {2, 1, 0, 3, 4, 5,};
            return mapping[index];
        }
    }

    Leds(const HWConfigLeds& config) : _config(config) {}

    void init();
    void setColor(uint8_t led, Color color, bool active);
    void refresh();

private:
    struct RGB
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };

    static constexpr uint8_t kFull = 128;
    static constexpr uint8_t kHalf = 64;
    static constexpr uint8_t kOff = 0;
    static constexpr RGB kColors[] = {
        [kNone]      = { kOff,  kOff,  kOff  },
        [kBlue]      = { kOff,  kOff,  kFull }, 
        [kPurple]    = { kHalf, kOff,  kHalf }, 
        [kRed]       = { kFull, kOff,  kOff  }, 
        [kYellow]    = { kHalf, kHalf, kOff  }, 
        [kGreen]     = { kOff,  kFull, kOff  }, 
        [kTurquoise] = { kOff,  kHalf, kHalf },
        [kWhite] = { kHalf,  kHalf, kHalf },
    };

    static inline uint8_t brightness(uint8_t orig, bool active)
    {
        return active ? orig : (orig / kHalf);
    }

    uint32_t _state[kMaxLeds] = {};
    const HWConfigLeds& _config;
    bool _fix_mapping = leds_fix_mapping();
};

} // namespace tocata
