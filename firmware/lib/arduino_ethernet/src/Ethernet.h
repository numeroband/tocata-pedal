#pragma once

#include "Arduino.h"

#include "socket.h"
#include "wizchip_conf.h"

#include <array>
#include <cstdint>

#define DEFAULT_CONTROL_PORT 5004
#define TCT_UDP_DEBUG 0

class EthernetUDP
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
        if (_socket >= 0) {
            stop();
        }

        int next_socket = firstAvailableSocket(this);
        if (next_socket < 0) {
            printf("No sockets available\n");
            return;
        }

        _socket = socket(next_socket, Sn_MR_UDP, port, SOCK_IO_NONBLOCK);
        if (_socket < 0)
        {
            printf("[%d] socket() failed: %d\n", next_socket, _socket);
            return;
        }
        changeState(kIdle);

        printf("[%d] UDP socket opened\n", _socket);
    }

    bool beginPacket(uint32_t addr, uint16_t port)
    {
        if (_socket < 0) { return false; } 
        changeState(kSending);
        _remote_addr = addr;
        _remote_port = port;
        return true;
    }

    bool beginPacket(IPAddress addr, uint16_t port)
    {
        return beginPacket(uint32_t(addr), port);
    }

    size_t parsePacket()
    {
        if (_socket < 0) { return 0; }
        auto total_interrupts = _total_interrupts;
        if (total_interrupts == _processed_interrupts) {
            return 0;
        }

#if TCT_UDP_DEBUG
        printf("[%d] total_ints %u processed %u\n", total_interrupts, _processed_interrupts);
#endif
        _processed_interrupts = total_interrupts;
        int32_t received = recvfrom(_socket, _buffer.data(), _buffer.size(), _remote_addr.raw_address(), &_remote_port);
        if (received < 0) {
            printf("[%d] recvfrom() failed: %d\n", _socket, received);
            changeState(kIdle);
            return 0;
        }

        if (received == 0) {
            return 0;
        }

        changeState(kReceiving);
        _buffer_size = size_t(received);

#if TCT_UDP_DEBUG
        for (size_t i = 0; i < _buffer_size; ++i) {
            if ((i % 16) == 0) {
                printf("\n[%d] RX: ", _socket);
            }
            printf("%02X ", _buffer[i]);
        }
        printf("\n");
#endif
        return _buffer_size;
    };

    size_t available()
    {
        return kReceiving ? _buffer_size - _buffer_offset : 0;
    };

    int read()
    {
        if (_state != kReceiving || !available()) { return -1; }
#if TCT_UDP_DEBUG
        printf("[%d] read byte\n", _socket);
#endif        
        auto ret = int(_buffer[_buffer_offset++]);
        if (!available()) {
            changeState(kIdle);
        }
        return ret;
    }

    size_t read(byte* buffer, size_t size)
    {
        if (_socket < 0) { return 0; }
        auto avail = available();
        if (avail == 0) { return 0; }
        if (size > avail) {
            size = avail;
        }
        memcpy(buffer, _buffer.data() + _buffer_offset, size);
        _buffer_offset += size;
        #if TCT_UDP_DEBUG
        printf("[%d] read %u bytes\n", _socket, size);
        #endif
        if (!available()) {
            changeState(kIdle);
        }
        return size;
    };

    void write(uint8_t buffer)
    {
        if (_socket < 0) { return; }
        if (_state != kSending) { return; }
        if (_buffer_size < _buffer.size()) {
            _buffer[_buffer_size++] = buffer;
        }
    };

    void write(uint8_t* buffer, size_t size)
    {
        if (_socket < 0) { return; }
        if (_state != kSending) { return; }
        auto avail = _buffer.size() - _buffer_size;
        if (size > avail) {
            size = avail;
        }
        memcpy(_buffer.data() + _buffer_size, buffer, size);
        _buffer_size += size;
    };

    void endPacket() {
        if (_state != kSending || _buffer_size == 0) {
            return;
        }

#if TCT_UDP_DEBUG
        for (size_t i = 0; i < _buffer_size; ++i) {
            if ((i % 16) == 0) {
                printf("\n[%d] TX:", _socket);
            }
            printf("%02X ", _buffer[i]);
        }
        printf("\n");
#endif

        int8_t ret = sendto(_socket, _buffer.data(), _buffer_size, _remote_addr.raw_address(), _remote_port);
        if (ret < 0) {
            printf("[%d] sendto() failed: %d\n", _socket, ret);
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
        close(uint8_t(_socket));
        usedSockets()[_socket] = nullptr;
        _socket = -1;
        changeState(kIdle);
    };

    IPAddress remoteIP() { return _remote_addr; }
    uint16_t  remotePort() { return _remote_port; }

private:
    static constexpr size_t kMaxSockets = 8;
    static constexpr size_t kMaxPacketSize = 2048;

    static std::array<EthernetUDP*, kMaxSockets>& usedSockets() {
        static std::array<EthernetUDP*, kMaxSockets> used_sockets{};
        return used_sockets;        
    }

    static int firstAvailableSocket(EthernetUDP* client) {
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

    void changeState(State state) {
        _buffer_size = 0;
        _buffer_offset = 0;
        _state = state;
    }

    static void interruptInitialize(uint8_t socket)
    {
        uint32_t reg_val = SIK_RECEIVED; 
        int ret_val = ctlsocket(socket, CS_SET_INTMASK, (void *)&reg_val);

        auto& used_sockets = usedSockets();
        uint16_t intr_mask = 0;
        for (size_t i = 0; i < used_sockets.size(); ++i) {
            if (used_sockets[i]) {
                intr_mask |= IK_SOCK_0 << i;
            }
        }
        wizchip_setinterruptmask(intr_kind(intr_mask));

        gpio_set_irq_enabled_with_callback(PIN_INT, GPIO_IRQ_EDGE_FALL, true, interruptCallback);
    }

    static void interruptCallback(uint gpio, uint32_t events)
    {
        intr_kind intr_mask = wizchip_getinterrupt();
        uint16_t clr_intr_mask = 0;
        auto& used_sockets = usedSockets();
        for (size_t i = 0; i < used_sockets.size(); ++i) {
            uint16_t sock_bit = IK_SOCK_0 << i;
            if (intr_mask & sock_bit) {
                clr_intr_mask |= sock_bit;
                uint32_t reg_val = SIK_RECEIVED; 
                ctlsocket(i, CS_CLR_INTERRUPT, (void *)&reg_val);
                if (used_sockets[i]) {
                    used_sockets[i]->_total_interrupts++;
                }
            }
        }
        wizchip_clrinterrupt(intr_kind(clr_intr_mask));
    }
    
    int _socket{-1};
    std::array<uint8_t, kMaxPacketSize> _buffer;
    size_t _buffer_size{0};
    size_t _buffer_offset{0};
    State _state{kIdle};
    uint32_t _processed_interrupts{0};
    uint32_t _total_interrupts{0};
    IPAddress _remote_addr{};
    uint16_t _remote_port{};
};
