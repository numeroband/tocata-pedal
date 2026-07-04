#pragma once
#include "midi_port.hpp"
#include <CoreMIDI/CoreMIDI.h>
#include <CoreFoundation/CoreFoundation.h>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace tocata::midi {

// Attaches to an existing system MIDI source/destination (matched by
// substring against its display name) instead of creating a new virtual
// port, so TocataMidi can bridge multicast MIDI directly into a device
// other software already expects to talk to.
class SystemMidi : public MidiPort {
public:
    // Throws std::runtime_error (message includes currently-available
    // source/destination names) if name_substr matches neither a source
    // nor a destination endpoint currently present on the system.
    SystemMidi(const std::string& name_substr) {
        auto src = findSource(name_substr);
        auto dst = findDestination(name_substr);

        if (!src && !dst) {
            std::string msg = "SystemMidi: no device matching \"" + name_substr + "\" found.\n";
            msg += "Available sources:\n";
            for (auto& n : listSources()) msg += "  " + n + "\n";
            msg += "Available destinations:\n";
            for (auto& n : listDestinations()) msg += "  " + n + "\n";
            throw std::runtime_error(msg);
        }

        std::string client_name{"TocataMIDI->" + name_substr};
        CFStringRef name = CFStringCreateWithCString(
            kCFAllocatorDefault, client_name.c_str(), kCFStringEncodingUTF8);
        MIDIClientCreate(name, nullptr, nullptr, &_client);

        if (src) {
            _source = *src;
            MIDIInputPortCreate(_client, name, &SystemMidi::readProc, this, &_in_port);
            MIDIPortConnectSource(_in_port, _source, nullptr);
        }
        if (dst) {
            _destination = *dst;
            MIDIOutputPortCreate(_client, name, &_out_port);
        }

        CFRelease(name);
    }

    ~SystemMidi() override {
        if (_in_port) {
            if (_source) MIDIPortDisconnectSource(_in_port, _source);
            MIDIPortDispose(_in_port);
        }
        if (_out_port) MIDIPortDispose(_out_port);
        if (_client) MIDIClientDispose(_client);
        // Note: _source / _destination are NOT disposed here -- they are
        // owned by the system / another application, not by SystemMidi.
    }

    SystemMidi(const SystemMidi&) = delete;
    SystemMidi& operator=(const SystemMidi&) = delete;

    void setCallback(Callback callback) override {
        _callback = std::move(callback);
    }

    void send(std::span<const uint8_t> data) override {
        if (!_destination) return;
        std::vector<uint8_t> buffer(sizeof(MIDIPacketList) + data.size() + 256);
        MIDIPacketList* pkt_list = reinterpret_cast<MIDIPacketList*>(buffer.data());
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        MIDIPacket* pkt = MIDIPacketListInit(pkt_list);
        pkt = MIDIPacketListAdd(pkt_list, buffer.size(), pkt, 0, data.size(), data.data());
        if (pkt) {
            MIDISend(_out_port, _destination, pkt_list);
        }
#pragma clang diagnostic pop
    }

    // Display names of all current CoreMIDI sources / destinations, in
    // MIDIGetSource/MIDIGetDestination enumeration order.
    static std::vector<std::string> listSources() {
        std::vector<std::string> out;
        ItemCount n = MIDIGetNumberOfSources();
        for (ItemCount i = 0; i < n; ++i) {
            out.push_back(displayName(MIDIGetSource(i)));
        }
        return out;
    }

    static std::vector<std::string> listDestinations() {
        std::vector<std::string> out;
        ItemCount n = MIDIGetNumberOfDestinations();
        for (ItemCount i = 0; i < n; ++i) {
            out.push_back(displayName(MIDIGetDestination(i)));
        }
        return out;
    }

    // Prints a labeled listing of listSources()/listDestinations(). Backs
    // --list-devices. Numbering is cosmetic only -- device selection is
    // always by substring match, not by these indices.
    static void printAvailableDevices() {
        printf("Sources (input to TocataMidi):\n");
        auto sources = listSources();
        for (size_t i = 0; i < sources.size(); ++i) {
            printf("  %zu: %s\n", i, sources[i].c_str());
        }
        printf("Destinations (output from TocataMidi):\n");
        auto destinations = listDestinations();
        for (size_t i = 0; i < destinations.size(); ++i) {
            printf("  %zu: %s\n", i, destinations[i].c_str());
        }
    }

private:
    static std::string displayName(MIDIObjectRef endpoint) {
        CFStringRef cf_name = nullptr;
        MIDIObjectGetStringProperty(endpoint, kMIDIPropertyDisplayName, &cf_name);
        if (!cf_name) return {};
        char buf[256];
        CFStringGetCString(cf_name, buf, sizeof(buf), kCFStringEncodingUTF8);
        CFRelease(cf_name);
        return std::string{buf};
    }

    static std::optional<MIDIEndpointRef> findSource(const std::string& name_substr) {
        ItemCount n = MIDIGetNumberOfSources();
        for (ItemCount i = 0; i < n; ++i) {
            MIDIEndpointRef ep = MIDIGetSource(i);
            if (displayName(ep).find(name_substr) != std::string::npos) {
                return ep;
            }
        }
        return std::nullopt;
    }

    static std::optional<MIDIEndpointRef> findDestination(const std::string& name_substr) {
        ItemCount n = MIDIGetNumberOfDestinations();
        for (ItemCount i = 0; i < n; ++i) {
            MIDIEndpointRef ep = MIDIGetDestination(i);
            if (displayName(ep).find(name_substr) != std::string::npos) {
                return ep;
            }
        }
        return std::nullopt;
    }

    static void readProc(const MIDIPacketList* pkt_list, void* refCon, void* /*srcConnRefCon*/) {
        auto* self = static_cast<SystemMidi*>(refCon);
        const MIDIPacket* packet = &pkt_list->packet[0];
        for (UInt32 i = 0; i < pkt_list->numPackets; ++i) {
            if (self->_callback && packet->length > 0) {
                self->_callback({packet->data, packet->length});
            }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
            packet = MIDIPacketNext(packet);
#pragma clang diagnostic pop
        }
    }

    MIDIClientRef _client{0};
    MIDIPortRef _in_port{0};
    MIDIPortRef _out_port{0};
    MIDIEndpointRef _source{0};
    MIDIEndpointRef _destination{0};
    Callback _callback;
};

}
