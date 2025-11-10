#pragma once

#include <stdint.h>
#include <cassert>
#include "hal.h"
#include <array>
#include <vector>

namespace tocata {

class DisplaySim
{
public:
    virtual bool processTransfer(const uint8_t* transfer, uint32_t len) = 0;
    virtual void setControlData(bool isData) = 0;
    virtual void refresh(uint32_t* screen, size_t hw_cols, uint32_t* colors) = 0;
    virtual size_t numRows() const = 0;
    virtual size_t numColumns() const = 0;
};

}
