#pragma once

#include "hal.h"
#include "config.h"

#include <cstdint>
#include <bitset>

namespace tocata {

class Leds
{
public:
    static constexpr uint8_t kNumLeds = TOCATA_PEDAL_LONG ? 10 : 6;

    static constexpr uint8_t fixMapping(uint8_t index) {
        constexpr uint8_t mapping[] = {
#if TOCATA_PEDAL_LONG
            4, 3, 2, 1, 0, 5, 6, 7, 8, 9,
#else
            2, 1, 0, 3, 4, 5,
#endif
        };
        return mapping[index];
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

    uint32_t _state[kNumLeds] = {};
    const HWConfigLeds& _config;
    bool _fix_mapping = leds_fix_mapping();
};

} // namespace tocata
