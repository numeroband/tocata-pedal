#include "virt_midi.hpp"
#include "mc_midi.hpp"

using namespace tocata::midi;

int main(int argc, const char* argv[]) {
    const char* iface = argc > 1 ? argv[1] : "en7";
    uint8_t num_ports = argc > 2 ? uint8_t(atoi(argv[2])) : 1;
    if (num_ports == 0 || num_ports > 10) {
        printf("Invalid num ports: %u\n", num_ports);
        return -1;
    }

    try {
        asio::io_context io_context;        
        std::vector<VirtualMidi> virt_ports;
        std::vector<MulticastMidi> mc_ports;
        for (uint8_t i = 0; i < num_ports; ++i) {
            auto& virt_port = virt_ports.emplace_back(i);
            auto& mc_port = mc_ports.emplace_back(io_context, i, iface);
            virt_port.setCallback(std::bind(&MulticastMidi::send, &mc_port, std::placeholders::_1));
            mc_port.setCallback(std::bind(&VirtualMidi::send, &virt_port, std::placeholders::_1));
        }

        io_context.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }



    return 0;
}