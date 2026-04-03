#pragma once

#include "udp6.hpp"
#include <midi_sender.h>
#include <poll_timer.h>
#include <cstdint>

namespace tocata::mcmidi {

    struct Header {
        static constexpr uint32_t kMagic = 0x70CA7A00;
        uint32_t magic = kMagic;
        uint8_t version = 1;
        uint8_t sequence = 0;
        uint8_t reserved[2];
        bool validate() const {
            return magic == kMagic && version == 1;
        }
    };

    struct Packet {
        Header header;
        std::array<uint8_t, 512> message;
        bool validate(size_t size) const {
            return size > sizeof(header) && header.validate();

        }

        static constexpr size_t data_size(size_t size) {
            return size - sizeof(header);
        }

        static constexpr size_t total_size(size_t data_size) {
            return data_size + sizeof(header);
        }

        std::span<const uint8_t> span(size_t size) const {
            return {message.data(), data_size(size)};
        }

        uint8_t* bytes() { return reinterpret_cast<uint8_t*>(this); }
    };
}

namespace tocata {

class MulticastMidi : public MidiSender {
public:
    MulticastMidi(Ethernet& eth) : _socket{eth} {}
    void init() {
        _socket.beginMulticast(kAddr, kPort);
    }

    void run() {
        auto available = _socket.parsePacket();
        if (available == 0) {
            return;
        }
        if (available > sizeof(_packet)) {
            printf("Ignoring MC MIDI packet with %zu bytes\n", available);
            _socket.read(nullptr, available);
            return;
        }
        auto bytes_read = _socket.read(_packet.bytes(), sizeof(_packet));
        if (!_packet.validate(bytes_read))
        {
            return;
        }

        if (_callback) {
            _callback(_packet.span(bytes_read), _packet.message, *this);
        }
    }

	void sendProgram(uint8_t channel, uint8_t program) override {
        _packet.header = {.sequence = _sequence++};
        _packet.message[0] = 0xC0 | (channel & 0x0F);
        _packet.message[1] = program;
        sendPacket(2);
    }

	void sendControl(uint8_t channel, uint8_t control, uint8_t value) override {
        _packet.header = {.sequence = _sequence++};
        _packet.message[0] = 0xB0 | (channel & 0x0F);
        _packet.message[1] = control;
        _packet.message[2] = value;
        sendPacket(3);
    }

	void sendSysEx(std::span<const uint8_t> sysex) override {
        _packet.header = {.sequence = _sequence++};
        if (sysex.data() != _packet.message.data()) {
            memcpy(_packet.message.data(), sysex.data(), sysex.size());
        }
        sendPacket(sysex.size());
    }

    void setCallback(Callback callback) override {
        _callback = callback;
    }

private:
    void sendPacket(uint8_t data_size) {
        _socket.beginPacket(kAddr, kPort);
        _socket.write(_packet.bytes(), _packet.total_size(data_size));
        _socket.endPacket();
    }

    EthernetUDP6 _socket;
    Callback _callback{};
    static constexpr IP6Address kAddr = {
        0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01, 0x70, 0xCA, 0x7A, 0};
    static constexpr uint16_t kPort = 30001;
    uint8_t _sequence = 0;
    mcmidi::Packet _packet;
};

}