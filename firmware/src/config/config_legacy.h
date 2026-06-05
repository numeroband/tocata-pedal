#pragma once

#include <cstdint>

// On-flash layouts for older Config versions, kept only so config.cpp can
// recognize them by size during migration. Do NOT include this anywhere else.

namespace tocata {

// v0: 1-byte version + 1-byte MIDI channel (no expression calibration).
struct ConfigV0
{
    uint8_t _version;
    uint8_t _channel;
} __attribute__((packed));

} // namespace tocata
