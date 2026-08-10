#pragma once

#define ASIO_STANDALONE
#include <asio.hpp>
#include <array>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <vector>

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
    // Worst-case encoded SysEx on this protocol is ~592 bytes (config
    // protocol's kBuffSize=512 raw, 7-bit packed plus the F0..F7/channel
    // envelope -- see midi_sysex.h and config_protocol.h's kBuffSize).
    // 600 covers that with a little room; must match network/mc_midi.hpp's
    // Packet::message, since a mismatch would truncate on receive again.
    std::array<uint8_t, 600> data;
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
        // Mem-init order must match declaration order below, or gcc warns
        // (-Wreorder): _out_disabled is declared first, then the sockets and
        // the address/endpoint pair.
        : _out_disabled{out_disabled},
          rx_socket_(io_context),
          tx_socket_(io_context),
          multicast_address_{from_port(port, iface)},
          multicast_endpoint_(asio::ip::make_address(multicast_address_), multicast_port) {

        // Receiving and sending need two sockets, because the bind that makes
        // receiving correct is the one that makes sending impossible:
        //
        // The receive socket binds to the group address, NOT the wildcard.
        // Every bridged port uses the same UDP port and differs only by group,
        // and on Linux the group-membership check happens per interface, not
        // per socket -- so with a wildcard bind every group's datagrams reach
        // every socket and port 0 re-emits port 1's MIDI into its own device.
        // Binding to the group makes the kernel filter by destination address.
        //
        // But BSD refuses to send from a socket whose local address is a
        // multicast address (the source address would be invalid): on macOS
        // every send from a group-bound socket fails with EOPNOTSUPP. Hence a
        // separate, unbound send socket. Its ephemeral source port is fine --
        // nothing on the wire filters on it.

        // 1. Open both sockets with IPv6
        rx_socket_.open(udp::v6());
        tx_socket_.open(udp::v6());

        // 2. Allow multiple processes to bind to the same port
        rx_socket_.set_option(udp::socket::reuse_address(true));

        // 3. Bind the receive socket to the group address and join the group
        rx_socket_.bind(udp::endpoint(multicast_endpoint_.address(), multicast_port));
        rx_socket_.set_option(asio::ip::multicast::join_group(
            asio::ip::make_address(multicast_address_)));

        // 4. Don't hear our own transmissions. IPV6_MULTICAST_LOOP is a
        // send-side option, so it belongs on the send socket. The destination
        // endpoint carries the "%<iface>" scope id, which is what selects the
        // outbound interface, so there is nothing else to configure here.
        tx_socket_.set_option(asio::ip::multicast::enable_loopback{false});

        // Start the async loops
        start_receive();
    }

    void send(std::span<const uint8_t> data) {
        if (_out_disabled) {
            return;
        }

        // Owned by the handler, not the stack: async_send_to only queues the
        // operation, so the buffer has to outlive this function.
        auto packet_bytes =
            std::make_shared<std::vector<uint8_t>>(Packet::total_size(data.size()));
        Packet& packet = *reinterpret_cast<Packet*>(packet_bytes->data());
        packet.header = {.sequence = _sequence++};
        memcpy(packet.data.data(), data.data(), data.size());
        tx_socket_.async_send_to(
            asio::buffer(*packet_bytes), multicast_endpoint_,
            [packet_bytes, endpoint = multicast_endpoint_](
                asio::error_code ec, std::size_t /*bytes*/) {
                if (ec) {
                    std::cerr << "\n[Error sending to " << endpoint << "]: "
                              << ec.message() << std::endl;
                }
            });
    }

    void setCallback(Callback callback) { _callback = callback; }

private:
    static std::string from_port(uint8_t port, const char* iface) {
        return "ff02::1:70CA:7A0" + std::to_string(port) + "%" + iface;
    }

    void start_receive() {
        rx_socket_.async_receive_from(
            asio::buffer((uint8_t*)&_packet, sizeof(_packet)), remote_endpoint_,
            [this](asio::error_code ec, std::size_t bytes_recvd) {
                if (ec) {
                    std::cerr << "\n[Error receiving from " << remote_endpoint_ << "]: "
                              << ec.message() << std::endl;
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
    udp::socket rx_socket_;
    udp::socket tx_socket_;
    std::string multicast_address_;
    udp::endpoint multicast_endpoint_;
    udp::endpoint remote_endpoint_;
};

}