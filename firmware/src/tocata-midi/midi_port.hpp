#pragma once
#define ASIO_STANDALONE
#include <asio.hpp>
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

// Platform backends, dispatched the same way hal.h picks between hal_pico.h and
// hal_host.h. Both export tocata::midi::VirtualMidi and tocata::midi::SystemMidi
// with identical signatures, so main.cpp keeps a single call site.
#if defined(__APPLE__)
#include "coremidi_port.hpp"
#elif defined(__linux__)
#include "alsa_port.hpp"
#else
#error "TocataMidi: no MIDI backend for this platform"
#endif
