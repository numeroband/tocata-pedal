#pragma once

#include <cstdint>
#include <array>

namespace tocata::registers {

#include <cstdint>
enum class RegisterType : uint8_t {
    Common,
    Socket,
    TxBuffer,
    RxBuffer,
};

template<RegisterType type>
inline uint8_t block(uint8_t sn = 0) {
    if constexpr (type == RegisterType::Socket) {
        return uint8_t((1 + 4 * sn) << 3);
    } else if constexpr (type == RegisterType::TxBuffer) {
        return uint8_t((2 + 4 * sn) << 3);
    } else if constexpr (type == RegisterType::RxBuffer) {
        return uint8_t((3 + 4 * sn) << 3);
    } else {
        return 0;
    }
}

template <size_t SIZE>
struct RegisterValueType { using type = void; };
template <> struct RegisterValueType<1> { using type = uint8_t; };
template <> struct RegisterValueType<2> { using type = uint16_t; };
template <> struct RegisterValueType<4> { using type = uint32_t; };

template<uint16_t ADDR, size_t SIZE, RegisterType TYPE>
struct Register {
    static constexpr uint16_t address = ADDR;
    static constexpr size_t size = SIZE;
    static constexpr RegisterType type = TYPE;
    using value_type = typename RegisterValueType<SIZE>::type;
    static uint8_t block(uint8_t sn = 0) { return registers::block<TYPE>(sn); }
};

template<uint16_t ADDR, size_t SIZE = 1>
using CommonReg = Register<ADDR, SIZE, RegisterType::Common>;

template<uint16_t ADDR, size_t SIZE = 1>
using SocketReg = Register<ADDR, SIZE, RegisterType::Socket>;

template<uint16_t ADDR, size_t SIZE = 1>
using TxBufferReg = Register<ADDR, SIZE, RegisterType::TxBuffer>;

template<uint16_t ADDR, size_t SIZE = 1>
using RxBufferReg = Register<ADDR, SIZE, RegisterType::RxBuffer>;

}