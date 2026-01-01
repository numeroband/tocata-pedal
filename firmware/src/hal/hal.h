#pragma once

#include <cstdint>

namespace tocata {

struct HWConfigI2C
{
    bool available;
    uint8_t sda_pin;
    uint8_t scl_pin;		
};

struct HWConfigDisplayI2C : HWConfigI2C {};

struct HWConfigDisplaySPI
{
    bool available;
    uint8_t clk_pin;
    uint8_t tx_pin;
    uint8_t dc_pin;
    uint8_t reset_pin;
};

struct HWConfigEthernet
{
    bool available;
    uint8_t rx_pin;
    uint8_t cs_pin;
    uint8_t clk_pin;
    uint8_t tx_pin;
    uint8_t reset_pin;
    uint8_t irq_pin;
};

struct HWConfigSwitches
{
    int state_machine_id;
    uint8_t first_input_pin;
    uint8_t map[10];
};

struct HWConfigLeds
{
    int state_machine_id;
    uint8_t data_pin;
    uint8_t map[8];
};

struct HWConfigExpression
{
    uint8_t adc_pin;
    uint8_t connected_pin;
};

struct HWConfig
{
    HWConfigSwitches switches;
    HWConfigLeds leds;
    HWConfigDisplayI2C displayI2C;
    HWConfigDisplaySPI displaySPI;
    HWConfigExpression expression;
    HWConfigEthernet ethernet;
}; 

}

#ifdef PICO_BUILD
#include "hal_pico.h"
#else
#include "hal_host.h"
#endif
