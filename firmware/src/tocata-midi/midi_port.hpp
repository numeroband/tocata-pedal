#pragma once
#include <functional>
#include <span>
#include <cstdint>

namespace tocata::midi {

class MidiPort {
public:
    using Callback = std::function<void(std::span<const uint8_t>)>;

    virtual ~MidiPort() = default;

    virtual void send(std::span<const uint8_t> data) = 0;
    virtual void setCallback(Callback callback) = 0;
};

}
