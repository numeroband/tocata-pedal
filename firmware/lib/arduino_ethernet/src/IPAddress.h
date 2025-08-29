#pragma once

#include <cstring>
#include <cstdint>

class IPAddress
{
public:
    IPAddress() : _addr{0} {};
    IPAddress(const IPAddress& from) : _addr{from._addr} {}
    IPAddress(uint8_t first_octet, uint8_t second_octet, uint8_t third_octet, uint8_t fourth_octet){
        auto addr = raw_address();
        addr[0] = first_octet;
        addr[1] = second_octet;
        addr[2] = third_octet;
        addr[3] = fourth_octet;
    }
    IPAddress(uint32_t address) : _addr{address} {}
    IPAddress(int address) : _addr{uint32_t(address)} {}
    IPAddress(const uint8_t *address) {
        auto addr = raw_address();
        addr[0] = address[0];
        addr[1] = address[1];
        addr[2] = address[2];
        addr[3] = address[3];
    };
    operator uint32_t() const { return _addr; }
    operator uint32_t()       { return _addr; }

    // Overloaded index operator to allow getting and setting individual octets of the address
    uint8_t operator[](int index) const {
        return raw_address()[index];
    }
    uint8_t& operator[](int index) {
        return raw_address()[index];
    }

    uint8_t* raw_address() { return reinterpret_cast<uint8_t*>(&_addr); }
    const uint8_t* raw_address() const { return reinterpret_cast<const uint8_t*>(&_addr); }

    bool operator==(const IPAddress& other) const { return _addr == other._addr; }
    // bool operator!=(const IPAddress& other) const { return true; }

private:
    uint32_t _addr;
};

const IPAddress INADDR_NONE{};