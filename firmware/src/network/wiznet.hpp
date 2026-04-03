#pragma once

#include "wiznet_registers.hpp"
#include "wiznet_spi.hpp"
#include "hal_pico.h"

#include <cstdint>

namespace tocata {

using namespace registers;

struct MACAddress : public std::array<uint8_t, 6> {
    void print() const {
        auto& mac = *this;
        printf("%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    }
};

struct IP6Address : public std::array<uint8_t, 16> {
    void print() const {
        auto& ip = *this;
        printf("%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x", 
            ip[0], ip[1], ip[2], ip[3], 
            ip[4], ip[5], ip[6], ip[7], 
            ip[8], ip[9], ip[10], ip[11], 
            ip[12], ip[13], ip[14], ip[15]);
    }
};

class Ethernet
{
public:
    void init() {
        _spi.init();
        set<NETLCKR>(NETLCKR_UNLOCK);
        MACAddress mac;
        macFromSerial(mac.data());
        set<SHAR>(mac.data());
        IP6Address ip;
        macToLinkLocalAddress(mac.data(), ip.data());
        set<LLAR>(ip.data());
        set<NETLCKR>(NETLCKR_LOCK);

        uint16_t chipId = get<CIDR>();
        if (chipId != 0x6100) {
            printf("Wrong chip ID: %0x04X\n");
            return;
        }
        _initialized = true;
        print();
    }

    void print() {
        MACAddress mac;
        get<SHAR>(mac.data());
        printf("----\nMAC: ");
        mac.print();
        IP6Address ip;
        get<LLAR>(ip.data());
        printf("\nLLA: ");
        ip.print();
        printf("\n----\n");
    }

    bool ready() {
        return _initialized && (mdioRead(PHYRAR_BMSR) & BMSR_LINK_STATUS);
    }

    template<typename Reg>
        requires(Reg::type == RegisterType::Common)
    typename Reg::value_type get() {
        return _spi.read<typename Reg::value_type>(Reg::address, Reg::block());
    }

    template<typename Reg>
        requires(Reg::type == RegisterType::Common)
    void set(typename Reg::value_type value) {
        _spi.write(Reg::address, Reg::block(), value);
    }

    template<typename Reg>
        requires(Reg::type == RegisterType::Common)
    void get(void* buffer) {
        _spi.read(Reg::address, Reg::block(), buffer, Reg::size);
    }

    template<typename Reg>
        requires(Reg::type == RegisterType::Common)
    void set(const void* buffer) {
        _spi.write(Reg::address, Reg::block(), buffer, Reg::size);
    }

    template<typename Reg>
        requires(Reg::type != RegisterType::Common)
    typename Reg::value_type get(uint8_t sn) {
        return _spi.read<typename Reg::value_type>(Reg::address, Reg::block(sn));
    }

    template<typename Reg>
        requires(Reg::type != RegisterType::Common)
    void set(uint8_t sn, typename Reg::value_type value) {
        _spi.write(Reg::address, Reg::block(sn), value);
    }

    template<typename Reg>
        requires(Reg::type != RegisterType::Common)
    void get(uint8_t sn, void* buffer) {
        _spi.read(Reg::address, Reg::block(sn), buffer, Reg::size);
    }

    template<typename Reg>
        requires(Reg::type != RegisterType::Common)
    void set(uint8_t sn, const void* buffer) {
        _spi.write(Reg::address, Reg::block(sn), buffer, Reg::size);
    }

    void sendData(uint8_t sn, const void* data, uint16_t len) {
        if (len == 0) { return; }
        uint16_t ptr = get<Sn_TX_WR>(sn);
        _spi.write(ptr, block<RegisterType::TxBuffer>(sn), data, len);
        ptr += len;
        set<Sn_TX_WR>(sn, ptr);
    }

    void recvData(uint8_t sn, void* data, uint16_t len) {
        if (len == 0) { return; }
        uint16_t ptr = get<Sn_RX_RD>(sn);
        if (data) {
            _spi.read(ptr, block<RegisterType::RxBuffer>(sn), data, len);
        }
        ptr += len;
        set<Sn_RX_RD>(sn, ptr);
    }

private:
    static void macToLinkLocalAddress(const uint8_t* mac, uint8_t* outIpv6) {
        // 1. Set the Link-Local prefix: fe80::/64
        // First 8 bytes: fe80:0000:0000:0000
        outIpv6[0] = 0xfe;
        outIpv6[1] = 0x80;
        for (int i = 2; i < 8; ++i) {
            outIpv6[i] = 0x00;
        }

        // 2. Modified EUI-64 (Interface Identifier)
        // Copy first 3 bytes of MAC
        outIpv6[8] = mac[0];
        outIpv6[9] = mac[1];
        outIpv6[10] = mac[2];

        // 3. Flip the Universal/Local bit (7th bit of the first byte)
        outIpv6[8] ^= 0x02;

        // 4. Insert ff:fe in the middle
        outIpv6[11] = 0xff;
        outIpv6[12] = 0xfe;

        // 5. Copy the remaining 3 bytes of MAC
        outIpv6[13] = mac[3];
        outIpv6[14] = mac[4];
        outIpv6[15] = mac[5];
    }

    static void macFromSerial(uint8_t* mac) {
        // Create a struct to hold the ID
        pico_unique_board_id_t board_id;

        // Fetch the ID from the hardware (Flash chip)
        pico_get_unique_board_id(&board_id);
        memcpy(mac, board_id.id + 2, 6);
        mac[0] = 0x02;
    }

    uint16_t mdioRead(uint8_t phyregaddr) {
        set<PHYRAR>(phyregaddr);
        set<PHYACR>(PHYACR_READ);
        while (get<PHYACR>()); //wait for command complete
        return get<PHYDOR>();
    }
    
    EthernetSPI _spi;
    bool _initialized = false;
};

}