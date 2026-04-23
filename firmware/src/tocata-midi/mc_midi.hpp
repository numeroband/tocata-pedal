#pragma once

#define ASIO_STANDALONE
#include <asio.hpp>
#include <cstdint>
#include <iostream>

using asio::ip::udp;

constexpr short multicast_port = 30001;

namespace tocata::midi {

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
    std::array<uint8_t, 512> data;
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
        return {data.data(), data_size(size)};
    }

    uint8_t* bytes() { return reinterpret_cast<uint8_t*>(this); }
};

class MulticastMidi {
public:
    using Callback = std::function<void(std::span<const uint8_t>)>;

    MulticastMidi(asio::io_context& io_context, uint8_t port, const char* iface, bool& out_disabled)
        : socket_(io_context),
          multicast_address_{from_port(port, iface)},
          multicast_endpoint_(asio::ip::make_address(multicast_address_), multicast_port),
          _out_disabled{out_disabled} {
        
        // 1. Open the socket with IPv6
        socket_.open(udp::v6());

        // 2. Allow multiple processes to bind to the same port
        socket_.set_option(udp::socket::reuse_address(true));

        // 3. Bind to the wildcard address and the specific port
        socket_.bind(udp::endpoint(udp::v6(), multicast_port));

        socket_.set_option(asio::ip::multicast::enable_loopback{false});

        // 4. Join the multicast group
        socket_.set_option(asio::ip::multicast::join_group(
            asio::ip::make_address(multicast_address_)));

        // Start the async loops
        start_receive();
    }

    void send(std::span<const uint8_t> data) {
        if (_out_disabled) {
            return;
        }

        std::vector<uint8_t> packet_bytes(Packet::total_size(data.size()));
        Packet& packet = *reinterpret_cast<Packet*>(packet_bytes.data());
        packet.header = {.sequence = _sequence++};
        memcpy(packet.data.data(), data.data(), data.size());
        socket_.async_send_to(
            asio::buffer(packet_bytes), multicast_endpoint_,
            [](asio::error_code ec, std::size_t bytes) {
                if (ec) {
                    std::cerr << "\n[Error sending bytes " << bytes << "]" << std::endl;
                }
            });
    }

    void setCallback(Callback callback) { _callback = callback; }

private:
    static std::string from_port(uint8_t port, const char* iface) {
        return "ff02::1:70CA:7A0" + std::to_string(port) + "%" + iface;
    }

    void start_receive() {
        socket_.async_receive_from(
            asio::buffer((uint8_t*)&_packet, sizeof(_packet)), remote_endpoint_,
            [this](asio::error_code ec, std::size_t bytes_recvd) {
                if (ec) {
                    std::cerr << "\n[Error on endpoint] " << remote_endpoint_ << "]: " << std::endl;
                    return;
                }

                if (_callback && _packet.validate(bytes_recvd)) {
                    _callback(_packet.span(bytes_recvd));
                } else {
                    std::cerr << "MC invalid packet sizes " << bytes_recvd << std::endl;
                }
                
                start_receive();
            });
    }

private:
    Callback _callback{};
    bool& _out_disabled;
    static constexpr uint16_t kPort = 30001;
    uint8_t _sequence = 0;
    Packet _packet;
    udp::socket socket_;
    std::string multicast_address_;
    udp::endpoint multicast_endpoint_;
    udp::endpoint remote_endpoint_;
};

}