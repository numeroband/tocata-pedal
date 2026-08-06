#include "midi_port.hpp"
#include "mc_midi.hpp"
#include "wing_session.hpp"
#include "player_connection.hpp"
#include <cstdio>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace tocata::midi;
using namespace tocata::wing;

static constexpr const char* kPlayerUri = "ws://localhost:9999";
static constexpr const char* kVirtualPrefix = "TocataMIDI";

// Interface whose link-local scope the multicast groups are joined on. There is
// no portable default, so pick the one that is right on the machine each
// backend runs on; override with --iface.
#if defined(__APPLE__)
static constexpr const char* kDefaultIface = "en7";
#else
static constexpr const char* kDefaultIface = "eth0";
#endif

struct Args {
    std::string iface = kDefaultIface;
    std::optional<std::string> role;
    std::vector<std::string> devices = {"TocataMIDI 1"};
    bool list_devices = false;
    bool show_help = false;
    bool valid = true;
};

static std::vector<std::string> splitCommaList(const std::string& s) {
    std::vector<std::string> out;
    size_t start = 0;
    while (start <= s.size()) {
        size_t comma = s.find(',', start);
        if (comma == std::string::npos) {
            out.push_back(s.substr(start));
            break;
        }
        out.push_back(s.substr(start, comma - start));
        start = comma + 1;
    }
    return out;
}

static void printUsage() {
    printf("Usage: TocataMidi [--iface %s] [--role primary|secondary]\n", kDefaultIface);
    printf("                   [--devices \"Name1,Name2,...\"] [--list-devices] [--help]\n");
    printf("\n");
    printf("Each --devices entry names one bridged port. An entry starting with\n");
    printf("\"%s\" creates a new virtual MIDI port using that exact name;\n", kVirtualPrefix);
    printf("any other entry is matched (by substring) against an existing system\n");
    printf("MIDI source/destination to attach to. Use --list-devices to see what's\n");
    printf("currently available.\n");
}

static Args parseArgs(int argc, const char* argv[]) {
    Args args;
    bool devices_set = false;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&](const char* flag) -> std::optional<std::string> {
            if (i + 1 >= argc) {
                printf("Missing value for %s\n", flag);
                args.valid = false;
                return std::nullopt;
            }
            return std::string{argv[++i]};
        };

        if (a == "--iface") {
            if (auto v = next("--iface")) args.iface = *v;
        } else if (a == "--role") {
            if (auto v = next("--role")) args.role = *v;
        } else if (a == "--devices") {
            if (auto v = next("--devices")) {
                args.devices = splitCommaList(*v);
                devices_set = true;
            }
        } else if (a == "--list-devices") {
            args.list_devices = true;
        } else if (a == "--help" || a == "-h") {
            args.show_help = true;
        } else {
            printf("Unknown argument: %s\n", a.c_str());
            args.valid = false;
        }
    }

    if (!args.list_devices && !args.show_help) {
        if (devices_set && (args.devices.empty() || args.devices.size() > 10)) {
            printf("Invalid --devices count: %zu (must be 1-10)\n", args.devices.size());
            args.valid = false;
        }
        for (auto& d : args.devices) {
            if (d.empty()) {
                printf("--devices entries must not be empty\n");
                args.valid = false;
                break;
            }
        }
        if (args.role && *args.role != "primary" && *args.role != "secondary") {
            printf("Invalid --role: %s (expected primary|secondary)\n", args.role->c_str());
            args.valid = false;
        }
    }

    return args;
}

int main(int argc, const char* argv[]) {
    Args args = parseArgs(argc, argv);

    if (args.show_help) {
        printUsage();
        return 0;
    }
    if (args.list_devices) {
        SystemMidi::printAvailableDevices();
        return 0;
    }
    if (!args.valid) {
        printUsage();
        return 1;
    }

    bool mc_out_disabled = args.role.has_value();
    bool primary = (mc_out_disabled && *args.role == "primary");

    try {
        asio::io_context io_context;
        // Both are built only with --role: WingSession's discovery socket binds
        // UDP 2222 on construction, which would otherwise occupy the WING
        // discovery port and stop a second TocataMidi from starting at all.
        std::optional<WingSession> wing_session;
        std::optional<PlayerConnection> player_connection;

        if (mc_out_disabled) {
            wing_session.emplace(io_context);
            player_connection.emplace(io_context, kPlayerUri);
            wing_session->setParserCallback([primary, &mc_out_disabled, &player_connection](auto hash, auto value) {
                if (hash == node::IO_ALTSW) {
                    bool not_alt = !std::get<int32_t>(value);
                    mc_out_disabled = primary ^ not_alt;
                    player_connection->setBackup(mc_out_disabled);
                } else if (hash == node::MGRP1_MUTE) {
                    player_connection->setMuted(std::get<int32_t>(value) != 0);
                }
            });
            wing_session->setSessionCallback([&io_context](bool connected) {
                if (connected) {
                    printf("Connected to WING\n");
                } else {
                    printf("Disconnected from WING\n");
                }
            });

            printf("Connecting to WING as %s...\n", args.role->c_str());
            wing_session->start();
        } else {
            printf("No redundancy. Ethernet out enabled.\n");
        }

        std::vector<std::unique_ptr<MidiPort>> virt_ports;
        std::vector<MulticastMidi> mc_ports;
        const size_t num_ports = args.devices.size();
        virt_ports.reserve(num_ports);
        mc_ports.reserve(num_ports);

        for (size_t i = 0; i < num_ports; ++i) {
            const std::string& name = args.devices[i];

            std::unique_ptr<MidiPort> port = name.rfind(kVirtualPrefix, 0) == 0
                ? std::unique_ptr<MidiPort>(std::make_unique<VirtualMidi>(io_context, name))
                : std::unique_ptr<MidiPort>(std::make_unique<SystemMidi>(io_context, name));

            auto& mc_port = mc_ports.emplace_back(io_context, uint8_t(i), args.iface.c_str(), mc_out_disabled);
            auto* port_ptr = port.get();
            port_ptr->setCallback(std::bind(&MulticastMidi::send, &mc_port, std::placeholders::_1));
            mc_port.setCallback(std::bind(&MidiPort::send, port_ptr, std::placeholders::_1));

            virt_ports.push_back(std::move(port));
        }

        io_context.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
