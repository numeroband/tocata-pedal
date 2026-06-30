#include <CoreMIDI/CoreMIDI.h>
#include <CoreFoundation/CoreFoundation.h>
#include <functional>
#include <span>
#include <string>
#include <vector>
#include <cstdint>

namespace tocata::midi {

class VirtualMidi {
public:
    using Callback = std::function<void(std::span<const uint8_t>)>;

    VirtualMidi(uint8_t port) {
        std::string port_name{"TocataMIDI " + std::to_string(port + 1)};
        CFStringRef name = CFStringCreateWithCString(
            kCFAllocatorDefault, port_name.c_str(), kCFStringEncodingUTF8);

        MIDIClientCreate(name, nullptr, nullptr, &_client);
        // Virtual source: other apps read from it (our output).
        MIDISourceCreate(_client, name, &_source);
        // Virtual destination: other apps send to it (our input).
        MIDIDestinationCreate(_client, name, &VirtualMidi::readProc, this, &_destination);

        CFRelease(name);
    }

    ~VirtualMidi() {
        if (_destination) MIDIEndpointDispose(_destination);
        if (_source) MIDIEndpointDispose(_source);
        if (_client) MIDIClientDispose(_client);
    }

    VirtualMidi(const VirtualMidi&) = delete;
    VirtualMidi& operator=(const VirtualMidi&) = delete;

    VirtualMidi(VirtualMidi&& other) noexcept
        : _client{other._client}, _source{other._source},
          _destination{other._destination}, _callback{std::move(other._callback)} {
        other._client = 0;
        other._source = 0;
        other._destination = 0;
    }

    void setCallback(Callback callback) {
        _callback = std::move(callback);
    }

    void send(std::span<const uint8_t> data) {
        std::vector<uint8_t> buffer(sizeof(MIDIPacketList) + data.size() + 256);
        MIDIPacketList* pkt_list = reinterpret_cast<MIDIPacketList*>(buffer.data());
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        MIDIPacket* pkt = MIDIPacketListInit(pkt_list);
        pkt = MIDIPacketListAdd(pkt_list, buffer.size(), pkt, 0, data.size(), data.data());
        if (pkt) {
            MIDIReceived(_source, pkt_list);
        }
#pragma clang diagnostic pop
    }

private:
    static void readProc(const MIDIPacketList* pkt_list, void* refCon, void* /*srcConnRefCon*/) {
        auto* self = static_cast<VirtualMidi*>(refCon);
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
    MIDIEndpointRef _source{0};
    MIDIEndpointRef _destination{0};
    Callback _callback;
};

}
