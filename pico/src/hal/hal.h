#pragma once

#ifdef PICO_BUILD
#include "hal_pico.h"
#else
#include "hal_host.h"
#endif

namespace tocata {

struct HWConfig
{
    HWConfigSwitches switches;
    HWConfigI2C display;
}; 

}