#pragma once

extern "C" {
    #include "wizchip_conf.h"
}

#include <hardware/gpio.h>

#include <array>
#include <cstdint>

#define TCT_UDP_DEBUG 0

namespace tocata {

using IP6Address = std::array<uint8_t, 16>;

class EthernetUDP6
{
private:
    enum State {
        kIdle,
        kSending,
        kReceiving,
    };

public:
    void begin(uint16_t port)
    {
        beginAny({}, port);
    }

    int beginMulticast(const IP6Address& multicast_ip, uint16_t multicast_port)
    {
        return beginAny(multicast_ip, multicast_port);
    }

    bool beginPacket(const IP6Address& addr, uint16_t port)
    {
        if (_socket < 0) { return false; } 
        changeState(kSending);

        setSn_DIP6R(_socket, (uint8_t*)addr.data());
        setSn_DPORTR(_socket, port);

#if TCT_UDP_DEBUG
        auto ip = addr;
        printf("\n[%d] TX to %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x port %u", 
            _socket, 
            ip[0], ip[1], ip[2], ip[3], 
            ip[4], ip[5], ip[6], ip[7], 
            ip[8], ip[9], ip[10], ip[11], 
            ip[12], ip[13], ip[14], ip[15], 
            port);
#endif
        return true;
    }

    int32_t receive() {
        uint16_t available = getSn_RX_RSR(_socket);
        if (available == 0) {
            return 0;
        }
        /* First read 2 bytes of PACKET INFO in SOCKETn RX buffer*/
        uint8_t head[2];
        wiz_recv_data(_socket, head, 2);    
        setSn_CR(_socket, Sn_CR_RECV);
        while (getSn_CR(_socket));
        uint16_t pack_len = head[0] & 0x07;
        pack_len = (pack_len << 8) + head[1];
        uint8_t packet_info = head[0] & 0xF8;
        constexpr uint8_t kIPV6 = 1 << 7;
        if ((packet_info & kIPV6) == 0) {
            setSn_CR(_socket, Sn_CR_RECV);
            while (getSn_CR(_socket));
            return -1;
        }
        wiz_recv_data(_socket, _remote_addr.data(), _remote_addr.size());
        setSn_CR(_socket, Sn_CR_RECV);
        while (getSn_CR(_socket));
        wiz_recv_data(_socket, (uint8_t*)&_remote_port, sizeof(_remote_port));
        setSn_CR(_socket, Sn_CR_RECV);
        while (getSn_CR(_socket));

        return pack_len;
    }

    size_t parsePacket()
    {
        if (_socket < 0) { return 0; }
        auto total_interrupts = _total_interrupts;
        if (total_interrupts != _processed_interrupts) {
#if TCT_UDP_DEBUG
            printf("[%d] total_ints %u processed %u\n", _socket, total_interrupts, _processed_interrupts);
#endif
            _data_available = true;
            _processed_interrupts = total_interrupts;
        }

        if (!_data_available) {
            return 0;
        }

        int32_t received = receive();
        if (received < 0) {
            printf("[%d] recvfrom() failed: %d\n", _socket, received);
            changeState(kIdle);
            return 0;
        }

        if (received == 0) {
            _data_available = false;
            return 0;
        }

        changeState(kReceiving);
        _buffer_size = size_t(received);

#if TCT_UDP_DEBUG
        auto ip = _remote_addr.data();
        printf("\n[%d] RX from %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x port %u %u bytes\n", 
            _socket, 
            ip[0], ip[1], ip[2], ip[3], 
            ip[4], ip[5], ip[6], ip[7], 
            ip[8], ip[9], ip[10], ip[11], 
            ip[12], ip[13], ip[14], ip[15], 
            _remote_port, _buffer_size);
#endif
        return _buffer_size;
    };

    size_t available()
    {
        return kReceiving ? _buffer_size - _buffer_offset : 0;
    };

    int read()
    {
        uint8_t b;
        read(&b, sizeof(b));
        return b;
    }

    size_t read(uint8_t* buffer, size_t size)
    {
        if (_socket < 0) { return 0; }
        auto avail = available();
        if (avail == 0) { return 0; }
        if (size > avail) {
            size = avail;
        }
        if (buffer) {
            wiz_recv_data(_socket, buffer, size);
        } else {
            wiz_recv_ignore(_socket, size);
        }
        setSn_CR(_socket, Sn_CR_RECV);
        while (getSn_CR(_socket));

        _buffer_offset += size;
        #if TCT_UDP_DEBUG
        printf("[%d] read %u bytes", _socket, size);
        for (size_t i = 0; buffer && i < size; ++i) {
            if ((i % 16) == 0) {
                printf("\n[%d] RX: ", _socket);
            }
            printf("%02X ", buffer[i]);
        }
        printf("\n");
        #endif
        if (!available()) {
            changeState(kIdle);
        }
        return size;
    };

    void write(uint8_t buffer)
    {
        write(&buffer, sizeof(buffer));
    };

    void write(uint8_t* buffer, size_t size)
    {
        if (_socket < 0) { return; }
        if (_state != kSending) { return; }

        auto maxsize = getSn_TxMAX(_socket);
        if (size > maxsize) {
            size = maxsize;            
        }

        while (getSn_TX_FSR(_socket) < size) {}
        wiz_send_data(_socket, buffer, size);

#if TCT_UDP_DEBUG
        for (size_t i = 0; i < size; ++i) {
            if ((i % 16) == 0) {
                printf("\n[%d] TX:", _socket);
            }
            printf("%02X ", buffer[i]);
        }
        printf("\n");
#endif
    };

    void endPacket() {
        if (_state != kSending) {
            return;
        }

        setSn_CR(_socket, Sn_CR_SEND6);
        while(getSn_CR(_socket)) {}
        uint8_t ir = 0;
        while ((ir & (Sn_IR_SENDOK | Sn_IR_TIMEOUT)) == 0) {
            ir = getSn_IR(_socket);
        }
        if (ir & Sn_IR_SENDOK) {
            setSn_IRCLR(_socket, Sn_IR_SENDOK);
        } else {
            setSn_IRCLR(_socket, Sn_IR_TIMEOUT);
            printf("[%d] Timeout sending buffer!!!\n", _socket);
        }

        changeState(kIdle);
    };

    void flush()
    {
        changeState(kIdle);
    };

    void stop() {
        if (_socket < 0) {
            return;
        }
        printf("[%d] UDP socket closed\n", _socket);

        setSn_CR(_socket, Sn_CR_CLOSE);
        while(getSn_CR(_socket)) {}
        while(getSn_SR(_socket) != SOCK_CLOSED) {}

        usedSockets()[_socket] = nullptr;
        _socket = -1;
        changeState(kIdle);
    };

    const IP6Address& remoteIP() const { return _remote_addr; }
    uint16_t remotePort() const { return _remote_port; }

private:
    static constexpr size_t kMaxSockets = 8;
    static constexpr size_t kMaxPacketSize = 2048;

    static std::array<EthernetUDP6*, kMaxSockets>& usedSockets() {
        static std::array<EthernetUDP6*, kMaxSockets> used_sockets{};
        return used_sockets;        
    }

    static int firstAvailableSocket(EthernetUDP6* client) {
        auto& used_sockets = usedSockets();
        for (size_t i = 0; i < used_sockets.size(); ++i) {
            if (!used_sockets[i]) {
                used_sockets[i] = client;
                interruptInitialize(uint8_t(i));
                return int(i);
            }
        }
        return -1;
    }

    int8_t socket(int8_t socket, uint16_t port, uint8_t flag) {
        setSn_MR(socket, Sn_MR_UDP6 | flag);
        setSn_PORTR(socket, port);
        setSn_CR(socket, Sn_CR_OPEN);
        while (getSn_CR(socket) != 0) {}
        printf("Sn_SR: 0x02X\n", getSn_SR(socket));

        return socket;
    }

    int beginAny(const IP6Address& multicast_ip, uint16_t port)
    {
        if (_socket >= 0) {
            stop();
        }

        int next_socket = firstAvailableSocket(this);
        if (next_socket < 0) {
            printf("No sockets available\n");
            return -1;
        }

        uint8_t flag = 0;
        bool is_multicast = (multicast_ip[0] != 0);
        if (is_multicast)
        {
    	    uint8_t mac[6];
            for (int i = 0; i < sizeof(mac); ++i) {
                if (i < 2) {
                    mac[i] = 0x33;
                } else {
                    mac[i] = multicast_ip[i + sizeof(multicast_ip) - sizeof(mac)];
                }
            }
            auto& ip = multicast_ip;
            printf("mc ip %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\n", 
            ip[0], ip[1], ip[2], ip[3], 
            ip[4], ip[5], ip[6], ip[7], 
            ip[8], ip[9], ip[10], ip[11], 
            ip[12], ip[13], ip[14], ip[15]);
            printf("listening mac %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            setSn_DHAR(next_socket, mac);
            setSn_DIP6R(next_socket, const_cast<uint8_t*>(multicast_ip.data()));
            setSn_DPORTR(next_socket, port);
            flag |= Sn_MR_MULTI;
        }

        _socket = socket(next_socket, port, flag);
        if (_socket < 0)
        {
            printf("[%d] socket() failed: %d\n", next_socket, _socket);
            return -1;
        }
        changeState(kIdle);

        printf("[%d] UDP6 socket opened%s\n", _socket, is_multicast ? "(MC)" : "");

        return 0;
    }

    void changeState(State state) {
        _buffer_size = 0;
        _buffer_offset = 0;
        _state = state;
    }

    static void interruptInitialize(uint8_t socket)
    {
        setSn_IMR(socket, Sn_IR_RECV);

        auto& used_sockets = usedSockets();
        uint8_t intr_mask = 0;
        for (size_t i = 0; i < used_sockets.size(); ++i) {
            if (used_sockets[i]) {
                intr_mask |= (1 << i);
            }
        }
        setSIMR(intr_mask);

        static bool init = false;
        if (!init) {
            init = true;
            gpio_set_dir(PIN_INT, false);
            gpio_set_function(PIN_INT, GPIO_FUNC_SIO);
            gpio_set_irq_enabled_with_callback(PIN_INT, GPIO_IRQ_EDGE_FALL, true, interruptCallback);
        }
    }

    static void interruptCallback(uint gpio, uint32_t events)
    {
        uint8_t intr_mask = getSIR();
        uint8_t clr_intr_mask = 0;
        auto& used_sockets = usedSockets();
        for (size_t i = 0; i < used_sockets.size(); ++i) {
            uint8_t sock_bit = 1 << i;;
            if (intr_mask & sock_bit) {
                clr_intr_mask |= sock_bit;
                setSn_IRCLR(i, Sn_IR_RECV);
                if (used_sockets[i]) {
                    used_sockets[i]->_total_interrupts++;
                }
            }
        }
        setIRCLR(clr_intr_mask);
    }
    
    int _socket{-1};
    size_t _buffer_size{0};
    size_t _buffer_offset{0};
    State _state{kIdle};
    uint32_t _processed_interrupts{0};
    uint32_t _total_interrupts{0};
    bool _data_available{false};
    IP6Address _remote_addr{};
    uint16_t _remote_port{};
};

}