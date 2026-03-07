#include <libremidi/libremidi.hpp>
#include <functional>
#include <span>
#include <cstdint>

namespace tocata::midi {

class VirtualMidi {
public:
    using Callback = std::function<void(std::span<const uint8_t>)>;

    VirtualMidi(uint8_t port) : _midi_in{libremidi::input_configuration{
        .on_message = [this](const libremidi::message& message) {
            if (_callback) {
                _callback({message.begin(), message.end()});
            }
        }, 
        .ignore_sysex = false,
    }} 
    {
        std::string port_name{"TocataMIDI " + std::to_string(port + 1)};
        _midi_in.open_virtual_port(port_name);
        _midi_out.open_virtual_port(port_name);
    }

    void setCallback(Callback callback) {
        _callback = callback;
    }

    void send(std::span<const uint8_t> data) {
        _midi_out.send_message(data.data(), data.size());
    }

private:
    libremidi::midi_out _midi_out{};
    libremidi::midi_in _midi_in;
    Callback _callback;
};

}

