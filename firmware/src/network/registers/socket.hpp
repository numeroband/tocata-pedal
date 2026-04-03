#pragma once

#include "types.hpp"

namespace tocata::registers {

using Sn_MR         = SocketReg<0x0000>; // (SOCKET n Mode Register) R=W 0x00
using Sn_PSR        = SocketReg<0x0004>; // (SOCKET n Prefer Source IPv6 Address Register) R=W 0x00
using Sn_CR         = SocketReg<0x0010>; // (SOCKET n Command Register) RW,AC 0x00
using Sn_IR         = SocketReg<0x0020>; // (SOCKET n Interrupt Register) WO 0x00
using Sn_IMR        = SocketReg<0x0024>; // (SOCKET n Interrupt Mask Register) R=W 0xFF
using Sn_IRCLR      = SocketReg<0x0028>; // (Sn_IR Clear Register) WO 0xFF
using Sn_SR         = SocketReg<0x0030>; // (SOCKET n Status Register) RO 0x00
using Sn_ESR        = SocketReg<0x0031>; // (SOCKET n Extension Status Register) RO 0x00
using Sn_PNR        = SocketReg<0x0100>; // (SOCKET n IP Protocol Number Register) R=W 0x00
using Sn_TOSR       = SocketReg<0x0104>; // (SOCKET n IP Type Of Service Register) R=W 0x00
using Sn_TTLR       = SocketReg<0x0108>; // (SOCKET n IP Time To Live Register) R=W 0x80
using Sn_FRGR       = SocketReg<0x010C, 2>;
using Sn_FRGR0      = SocketReg<0x010C>; // (SOCKET n Fragment Offset in IP Header Register) R=W 0x40
using Sn_FRGR1      = SocketReg<0x010D>; // R=W 0x00
using Sn_MSSR       = SocketReg<0x0110, 2>;
using Sn_MSSR0      = SocketReg<0x0110>; // (SOCKET n Maximum Segment Size Register) RW 0x00
using Sn_MSSR1      = SocketReg<0x0111>; // _RW 0x00
using Sn_PORTR      = SocketReg<0x0114, 2>;
using Sn_PORTR0     = SocketReg<0x0114>; // (SOCKET n Source Port Register) R=W 0x00
using Sn_PORTR1     = SocketReg<0x0115>; // R=W 0x00
using Sn_DHAR       = SocketReg<0x0118, 6>;
using Sn_DHAR0      = SocketReg<0x0118>; // (SOCKET n Destination Hardware Address Register) RW 0x00
using Sn_DHAR1      = SocketReg<0x0119>; // RW 0x00
using Sn_DHAR2      = SocketReg<0x011A>; // RW 0x00
using Sn_DHAR3      = SocketReg<0x011B>; // RW 0x00
using Sn_DHAR4      = SocketReg<0x011C>; // RW 0x00
using Sn_DHAR5      = SocketReg<0x011D>; // RW 0x00
using Sn_DIPR       = SocketReg<0x0120, 4>;
using Sn_DIPR0      = SocketReg<0x0120>; // (SOCKET n Destination IPv4 Address Register) RW 0x00
using Sn_DIPR1      = SocketReg<0x0121>; // RW 0x00
using Sn_DIPR2      = SocketReg<0x0122>; // RW 0x00
using Sn_DIPR3      = SocketReg<0x0123>; // RW 0x00
using Sn_DIP6R      = SocketReg<0x0130, 16>; 
using Sn_DIP6R0     = SocketReg<0x0130>; // (SOCKET n Destination IPv6 Address Register) RW 0x00
using Sn_DIP6R1     = SocketReg<0x0131>; // RW 0x00
using Sn_DIP6R2     = SocketReg<0x0132>; // RW 0x00
using Sn_DIP6R3     = SocketReg<0x0133>; // RW 0x00
using Sn_DIP6R4     = SocketReg<0x0134>; // RW 0x00
using Sn_DIP6R5     = SocketReg<0x0135>; // RW 0x00
using Sn_DIP6R6     = SocketReg<0x0136>; // RW 0x00
using Sn_DIP6R7     = SocketReg<0x0137>; // RW 0x00
using Sn_DIP6R8     = SocketReg<0x0138>; // RW 0x00
using Sn_DIP6R9     = SocketReg<0x0139>; // RW 0x00
using Sn_DIP6R10    = SocketReg<0x013A>; // RW 0x00
using Sn_DIP6R11    = SocketReg<0x013B>; // RW 0x00
using Sn_DIP6R12    = SocketReg<0x013C>; // RW 0x00
using Sn_DIP6R13    = SocketReg<0x013D>; // RW 0x00
using Sn_DIP6R14    = SocketReg<0x013E>; // RW 0x00
using Sn_DIP6R15    = SocketReg<0x013F>; // RW 0x00
using Sn_DPORTR     = SocketReg<0x0140, 2>;
using Sn_DPORTR0    = SocketReg<0x0140>; // (SOCKET n Destination Port Register) RW 0x00
using Sn_DPORTR1    = SocketReg<0x0141>; // RW 0x00
using Sn_MR2        = SocketReg<0x0144>; // (SOCKET n Mode Register 2) R=W 0x00
using Sn_RTR        = SocketReg<0x0180, 2>;
using Sn_RTR0       = SocketReg<0x0180>; // (SOCKET n Retransmission Time Register) RW 0x00
using Sn_RTR1       = SocketReg<0x0181>; // RW 0x00
using Sn_RCR        = SocketReg<0x0184>; // (SOCKET n Retransmission Count Register) RW 0x00
using Sn_KPALVTR    = SocketReg<0x0188>; // (SOCKET n Keep Alive Time Register) R=W 0x00
using Sn_TX_BSR     = SocketReg<0x0200>; // (SOCKET n TX Buffer Size Register) R=W 0x02
using Sn_TX_FSR     = SocketReg<0x0204, 2>;
using Sn_TX_FSR0    = SocketReg<0x0204>; // (SOCKET n TX Free Size Register) RO 0x00
using Sn_TX_FSR1    = SocketReg<0x0205>; // RO 0x00
using Sn_TX_RD      = SocketReg<0x0208, 2>;
using Sn_TX_RD0     = SocketReg<0x0208>; // (SOCKET n TX Read Pointer Register) RO 0x00
using Sn_TX_RD1     = SocketReg<0x0209>; // RO 0x00
using Sn_TX_WR      = SocketReg<0x020C, 2>;
using Sn_TX_WR0     = SocketReg<0x020C>; // (SOCKET n TX Write Pointer Register) RW 0x00
using Sn_TX_WR1     = SocketReg<0x020D>; // RW 0x00
using Sn_RX_BSR     = SocketReg<0x0220>; // (SOCKET n RX Buffer Size Register) R=W 0x02
using Sn_RX_RSR     = SocketReg<0x0224, 2>;
using Sn_RX_RSR0    = SocketReg<0x0224>; // (SOCKET n RX Received Size Register) RO 0x00
using Sn_RX_RSR1    = SocketReg<0x0225>; // RO 0x00
using Sn_RX_RD      = SocketReg<0x0228, 2>;
using Sn_RX_RD0     = SocketReg<0x0228>; // (SOCKET n RX Read Pointer Register) RW 0x00
using Sn_RX_RD1     = SocketReg<0x0229>; // RW 0x00
using Sn_RX_WR      = SocketReg<0x022C, 2>;
using Sn_RX_WR0     = SocketReg<0x022C>; // (SOCKET n RX Write Pointer Register) RO 0x00
using Sn_RX_WR1     = SocketReg<0x022D>; // RO 0x00

constexpr uint8_t Sn_CR_OPEN  = 0x01;
constexpr uint8_t Sn_CR_CLOSE = 0x10;
constexpr uint8_t Sn_CR_SEND6 = 0xA0;
constexpr uint8_t Sn_CR_RECV  = 0x40;

constexpr uint8_t SOCK_CLOSED = 0;

constexpr uint8_t Sn_IR_SENDOK  = (1 << 4);
constexpr uint8_t Sn_IR_TIMEOUT = (1 << 3);
constexpr uint8_t Sn_IR_RECV    = (1 << 2);
constexpr uint8_t Sn_IR_DISCON  = (1 << 1);
constexpr uint8_t Sn_IR_CON     = (1 << 0);

constexpr uint8_t Sn_MR_UDP6  = 0x0A;
constexpr uint8_t Sn_MR_MULTI = (1 << 7);

}