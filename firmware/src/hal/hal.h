#pragma once

#include <cstdint>

namespace tocata {

static constexpr uint8_t kMaxDisplays = 2;

struct HWConfigI2C
{
    uint8_t index;
    uint8_t sda_pin;
    uint8_t scl_pin;		
};

struct HWConfigSwitches
{
    int state_machine_id;
    uint8_t first_input_pin;
    uint8_t first_output_pin;
};

struct HWConfigLeds
{
    int state_machine_id;
    uint8_t data_pin;
};

struct HWConfigExpression
{
    uint8_t adc_pin;
};

struct HWConfig
{
    HWConfigSwitches switches;
    HWConfigLeds leds;
    HWConfigI2C display[kMaxDisplays];
    HWConfigExpression expression;
}; 

}

#ifdef PICO_BUILD
#include "hal_pico.h"
#else
#include "hal_host.h"
#endif
