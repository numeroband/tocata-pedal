#include "virt_midi.hpp"
#include "mc_midi.hpp"
#include "wing_session.hpp"

using namespace tocata::midi;
using namespace tocata::wing;

int main(int argc, const char* argv[]) {
    const char* iface = argc > 1 ? argv[1] : "en7";
    uint8_t num_ports = argc > 2 ? uint8_t(atoi(argv[2])) : 1;
    if (num_ports == 0 || num_ports > 10) {
        printf("Invalid num ports: %u\n", num_ports);
        return -1;
    }
    bool mc_out_disabled = (argc > 3);
    auto role = (argc > 3) ? argv[3] : nullptr;
    bool primary = (mc_out_disabled && std::string{role} == "primary");

    try {
        asio::io_context io_context;
        WingSession wing_session{io_context};

        if (mc_out_disabled) {
            wing_session.setParserCallback([primary, role, &mc_out_disabled](auto hash, auto value) {
                if (hash == node::IO_ALTSW) {
                    bool not_alt = !std::get<int32_t>(value);
                    bool new_mc_out_disabled = primary ^ not_alt;
                    if (new_mc_out_disabled == mc_out_disabled) {
                        return;
                    }
                    mc_out_disabled = new_mc_out_disabled;
                    printf("Role: %s, eth out: %s\n", role,
                        mc_out_disabled ? "disabled" : "enabled");
                }
            });
            wing_session.setSessionCallback([&io_context](bool connected) {
                if (connected) {
                    printf("Connected to WING\n");
                } else {
                    printf("Disconnected from WING\n");
                }
            });

            printf("Connecting to WING as %s...\n", role);
            wing_session.start();
        } else {
            printf("No redundancy. Ethernet out enabled.\n");
        }

        std::vector<VirtualMidi> virt_ports;
        std::vector<MulticastMidi> mc_ports;
        for (uint8_t i = 0; i < num_ports; ++i) {
            auto& virt_port = virt_ports.emplace_back(i);
            auto& mc_port = mc_ports.emplace_back(io_context, i, iface, mc_out_disabled);
            virt_port.setCallback(std::bind(&MulticastMidi::send, &mc_port, std::placeholders::_1));
            mc_port.setCallback(std::bind(&VirtualMidi::send, &virt_port, std::placeholders::_1));
        }

        io_context.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }



    return 0;
}