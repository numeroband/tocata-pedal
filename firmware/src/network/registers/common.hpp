#pragma once

#include "types.hpp"

namespace tocata::registers {

using CIDR      = CommonReg<0x0000, 2>;
using CIDR0     = CommonReg<0x0000>; // (Chip Identification Register) RO 0x61
using CIDR1     = CommonReg<0x0001>; // RO 0x00
using VER       = CommonReg<0x0002, 2>;
using VER0      = CommonReg<0x0002>; // (Chip Version Register) RO 0x46
using VER1      = CommonReg<0x0003>; // RO 0x61
using SYSR      = CommonReg<0x2000>; // (System Status Register) RO 0xEU
using SYCR0     = CommonReg<0x2004>; // (System Config Register 0) WO 0x80
using SYCR1     = CommonReg<0x2005>; // R=W 0x80
using TCNTR0    = CommonReg<0x2016>; // (Tick Counter Register) RO 0x00
using TCNTR1    = CommonReg<0x2017>; // RO 0x00
using TCNTCLR   = CommonReg<0x2020>; // (TCNTR Clear Register) WO 0x00
using IR        = CommonReg<0x2100>; // (Interrupt Register) RO 0x00
using SIR       = CommonReg<0x2101>; // (SOCKET Interrupt Register) RO 0x00
using SLIR      = CommonReg<0x2102>; // (SOCKET-less Interrupt Register) RO 0x00
using IMR       = CommonReg<0x2104>; // (Interrupt Mask Register) R=W 0x00
using IRCLR     = CommonReg<0x2108>; // (IR Clear Register) WO 0x00
using SIMR      = CommonReg<0x2114>; // (SOCKET Interrupt Mask Register) R=W 0x00
using SLIMR     = CommonReg<0x2124>; // (SOCKET-less Interrupt Mask Register) R=W 0x00
using SLIRCLR   = CommonReg<0x2128>; // (SLIR Clear Register) WO 0x00
using SLPSR     = CommonReg<0x212C>; // (SOCKET-less Prefer Source IPv6 Address Register) R=W 0x00
using SLCR      = CommonReg<0x2130>; // (SOCKET-less Command Register) RW,AC 0x00
using PHYSR     = CommonReg<0x3000>; // (PHY Status Register) RO 0x00
using PHYRAR    = CommonReg<0x3008>; // (PHY Register Address Register) R=W 0x00
using PHYDIR0   = CommonReg<0x300C>; // (PHY Data Input Register) R=W 0x00
using PHYDIR1   = CommonReg<0x300D>; // R=W 0x00
using PHYDOR    = CommonReg<0x3010, 2>;
using PHYDOR0   = CommonReg<0x3010>; // (PHY Data Output Register) RO 0x00
using PHYDOR1   = CommonReg<0x3011>; // RO 0x00
using PHYACR    = CommonReg<0x3014>; // (PHY Access Control Register) RW,AC 0x00
using PHYDIVR   = CommonReg<0x3018>; // (PHY Division Register) R=W 0x01
using PHYCR0    = CommonReg<0x301C>; // (PHY Control Register 0) WO 0x00
using PHYCR1    = CommonReg<0x301D>; // R=W 0x40
using NET4MR    = CommonReg<0x4000>; // (Network IPv4 Mode Register) R=W 0x00
using NET6MR    = CommonReg<0x4004>; // (Network IPv6 Mode Register) R=W 0x00
using NETMR     = CommonReg<0x4008>; // (Network Mode Register) R=W 0x00
using NETMR2    = CommonReg<0x4009>; // (Network Mode Register 2) R=W 0x00
using PTMR      = CommonReg<0x4100>; // (PPP Link Control Protocol Request Timer Register) R=W 0x28
using PMNR      = CommonReg<0x4104>; // (PPP Link Control Protocol Magic number Register) R=W 0x00
using PHAR0     = CommonReg<0x4108>; // (PPPoE Hardware Address Register on PPPoE) R=W 0x00
using PHAR1     = CommonReg<0x4109>; // R=W 0x00
using PHAR2     = CommonReg<0x410A>; // R=W 0x00
using PHAR3     = CommonReg<0x410B>; // R=W 0x00
using PHAR4     = CommonReg<0x410C>; // R=W 0x00
using PHAR5     = CommonReg<0x410D>; // R=W 0x00
using PSIDR0    = CommonReg<0x4110>; // (PPPoE Session ID Register) R=W 0x00
using PSIDR1    = CommonReg<0x4111>; // R=W 0x00
using PMRUR0    = CommonReg<0x4114>; // (PPPoE Maximum Receive Unit Register) R=W 0xFF
using PMRUR1    = CommonReg<0x4115>; // R=W 0xFF
using SHAR      = CommonReg<0x4120, 6>;
using SHAR0     = CommonReg<0x4120>; // (Source Hardware Address Register) R=W 0x00
using SHAR1     = CommonReg<0x4121>; // R=W 0x00
using SHAR2     = CommonReg<0x4122>; // R=W 0x00
using SHAR3     = CommonReg<0x4123>; // R=W 0x00
using SHAR4     = CommonReg<0x4124>; // R=W 0x00
using SHAR5     = CommonReg<0x4125>; // R=W 0x00
using GAR0      = CommonReg<0x4130>; // (Gateway IP Address Register) R=W 0x00
using GAR1      = CommonReg<0x4131>; // R=W 0x00
using GAR2      = CommonReg<0x4132>; // R=W 0x00
using GAR3      = CommonReg<0x4133>; // R=W 0x00
using SUBR0     = CommonReg<0x4134>; // (Subnet Mask Register) R=W 0x00
using SUBR1     = CommonReg<0x4135>; // R=W 0x00
using SUBR2     = CommonReg<0x4136>; // R=W 0x00
using SUBR3     = CommonReg<0x4137>; // R=W 0x00
using SIPR0     = CommonReg<0x4138>; // (IPv4 Source Address Register) R=W 0x00
using SIPR1     = CommonReg<0x4139>; // R=W 0x00
using SIPR2     = CommonReg<0x413A>; // R=W 0x00
using SIPR3     = CommonReg<0x413B>; // R=W 0x00
using LLAR      = CommonReg<0x4140, 16>;
using LLAR0     = CommonReg<0x4140>; // (Link Local Address Register) R=W 0x00
using LLAR1     = CommonReg<0x4141>; // R=W 0x00
using LLAR2     = CommonReg<0x4142>; // R=W 0x00
using LLAR3     = CommonReg<0x4143>; // R=W 0x00
using LLAR4     = CommonReg<0x4144>; // R=W 0x00
using LLAR5     = CommonReg<0x4145>; // R=W 0x00
using LLAR6     = CommonReg<0x4146>; // R=W 0x00
using LLAR7     = CommonReg<0x4147>; // R=W 0x00
using LLAR8     = CommonReg<0x4148>; // R=W 0x00
using LLAR9     = CommonReg<0x4149>; // R=W 0x00
using LLAR10    = CommonReg<0x414A>; // R=W 0x00
using LLAR11    = CommonReg<0x414B>; // R=W 0x00
using LLAR12    = CommonReg<0x414C>; // R=W 0x00
using LLAR13    = CommonReg<0x414D>; // R=W 0x00
using LLAR14    = CommonReg<0x414E>; // R=W 0x00
using LLAR15    = CommonReg<0x414F>; // R=W 0x00
using GUAR0     = CommonReg<0x4150>; // (Global Unicast Address Register) R=W 0x00
using GUAR1     = CommonReg<0x4151>; // R=W 0x00
using GUAR2     = CommonReg<0x4152>; // R=W 0x00
using GUAR3     = CommonReg<0x4153>; // R=W 0x00
using GUAR4     = CommonReg<0x4154>; // R=W 0x00
using GUAR5     = CommonReg<0x4155>; // R=W 0x00
using GUAR6     = CommonReg<0x4156>; // R=W 0x00
using GUAR7     = CommonReg<0x4157>; // R=W 0x00
using GUAR8     = CommonReg<0x4158>; // R=W 0x00
using GUAR9     = CommonReg<0x4159>; // R=W 0x00
using GUAR10    = CommonReg<0x415A>; // R=W 0x00
using GUAR11    = CommonReg<0x415B>; // R=W 0x00
using GUAR12    = CommonReg<0x415C>; // R=W 0x00
using GUAR13    = CommonReg<0x415D>; // R=W 0x00
using GUAR14    = CommonReg<0x415E>; // R=W 0x00
using GUAR15    = CommonReg<0x415F>; // R=W 0x00
using SUB6R0    = CommonReg<0x4160>; // (IPv6 Subnet Prefix Register) R=W 0x00
using SUB6R1    = CommonReg<0x4161>; // R=W 0x00
using SUB6R2    = CommonReg<0x4162>; // R=W 0x00
using SUB6R3    = CommonReg<0x4163>; // R=W 0x00
using SUB6R4    = CommonReg<0x4164>; // R=W 0x00
using SUB6R5    = CommonReg<0x4165>; // R=W 0x00
using SUB6R6    = CommonReg<0x4166>; // R=W 0x00
using SUB6R7    = CommonReg<0x4167>; // R=W 0x00
using SUB6R8    = CommonReg<0x4168>; // R=W 0x00
using SUB6R9    = CommonReg<0x4169>; // R=W 0x00
using SUB6R10   = CommonReg<0x416A>; // R=W 0x00
using SUB6R11   = CommonReg<0x416B>; // R=W 0x00
using SUB6R12   = CommonReg<0x416C>; // R=W 0x00
using SUB6R13   = CommonReg<0x416D>; // R=W 0x00
using SUB6R14   = CommonReg<0x416E>; // R=W 0x00
using SUB6R15   = CommonReg<0x416F>; // R=W 0x00
using GA6R0     = CommonReg<0x4170>; // (IPv6 Gateway Address Register) R=W 0x00
using GA6R1     = CommonReg<0x4171>; // R=W 0x00
using GA6R2     = CommonReg<0x4172>; // R=W 0x00
using GA6R3     = CommonReg<0x4173>; // R=W 0x00
using GA6R4     = CommonReg<0x4174>; // R=W 0x00
using GA6R5     = CommonReg<0x4175>; // R=W 0x00
using GA6R6     = CommonReg<0x4176>; // R=W 0x00
using GA6R7     = CommonReg<0x4177>; // R=W 0x00
using GA6R8     = CommonReg<0x4178>; // R=W 0x00
using GA6R9     = CommonReg<0x4179>; // R=W 0x00
using GA6R10    = CommonReg<0x417A>; // R=W 0x00
using GA6R11    = CommonReg<0x417B>; // R=W 0x00
using GA6R12    = CommonReg<0x417C>; // R=W 0x00
using GA6R13    = CommonReg<0x417D>; // R=W 0x00
using GA6R14    = CommonReg<0x417E>; // R=W 0x00
using GA6R15    = CommonReg<0x417F>; // R=W 0x00
using SLDIP6R0  = CommonReg<0x4180>; // (SOCKET-less Destination IP Address Register) R=W 0x00
using SLDIP6R1  = CommonReg<0x4181>; // R=W 0x00
using SLDIP6R2  = CommonReg<0x4182>; // R=W 0x00
using SLDIP6R3  = CommonReg<0x4183>; // R=W 0x00
using SLDIP6R4  = CommonReg<0x4184>; // R=W 0x00
using SLDIP6R5  = CommonReg<0x4185>; // R=W 0x00
using SLDIP6R6  = CommonReg<0x4186>; // R=W 0x00
using SLDIP6R7  = CommonReg<0x4187>; // R=W 0x00
using SLDIP6R8  = CommonReg<0x4188>; // R=W 0x00
using SLDIP6R9  = CommonReg<0x4189>; // R=W 0x00
using SLDIP6R10 = CommonReg<0x418A>; // R=W 0x00
using SLDIP6R11 = CommonReg<0x418B>; // R=W 0x00
using SLDIP6R12 = CommonReg<0x418C>; // R=W 0x00
using SLDIP6R13 = CommonReg<0x418D>; // R=W 0x00
using SLDIP6R14 = CommonReg<0x418E>; // R=W 0x00
using SLDIP6R15 = CommonReg<0x418F>; // R=W 0x00
using SLDHAR0   = CommonReg<0x4190>; // (SOCKET-less Destination Hardware Address Register) RO 0x00
using SLDHAR1   = CommonReg<0x4191>; // RO 0x00
using SLDHAR2   = CommonReg<0x4192>; // RO 0x00
using SLDHAR3   = CommonReg<0x4193>; // RO 0x00
using SLDHAR4   = CommonReg<0x4194>; // RO 0x00
using SLDHAR5   = CommonReg<0x4195>; // RO 0x00
using PINGIDR0  = CommonReg<0x4198>; // (PING ID Register) R=W 0x00
using PINGIDR1  = CommonReg<0x4199>; // R=W 0x00
using PINGSEQR0 = CommonReg<0x419C>; // (PING Sequence-number Register) R=W 0x00
using PINGSEQR1 = CommonReg<0x419D>; // R=W 0x00
using UIPR0     = CommonReg<0x41A0>; // (Unreachable IP Address Register) RO 0x00
using UIPR1     = CommonReg<0x41A1>; // RO 0x00
using UIPR2     = CommonReg<0x41A2>; // RO 0x00
using UIPR3     = CommonReg<0x41A3>; // RO 0x00
using UPORTR0   = CommonReg<0x41A4>; // (Unreachable Port Register) RO 0x00
using UPORTR1   = CommonReg<0x41A5>; // RO 0x00
using UIP6R0    = CommonReg<0x41B0>; // (Unreachable IPv6 Address Register) RO 0x00
using UIP6R1    = CommonReg<0x41B1>; // RO 0x00
using UIP6R2    = CommonReg<0x41B2>; // RO 0x00
using UIP6R3    = CommonReg<0x41B3>; // RO 0x00
using UIP6R4    = CommonReg<0x41B4>; // RO 0x00
using UIP6R5    = CommonReg<0x41B5>; // RO 0x00
using UIP6R6    = CommonReg<0x41B6>; // RO 0x00
using UIP6R7    = CommonReg<0x41B7>; // RO 0x00
using UIP6R8    = CommonReg<0x41B8>; // RO 0x00
using UIP6R9    = CommonReg<0x41B9>; // RO 0x00
using UIP6R10   = CommonReg<0x41BA>; // RO 0x00
using UIP6R11   = CommonReg<0x41BB>; // RO 0x00
using UIP6R12   = CommonReg<0x41BC>; // RO 0x00
using UIP6R13   = CommonReg<0x41BD>; // RO 0x00
using UIP6R14   = CommonReg<0x41BE>; // RO 0x00
using UIP6R15   = CommonReg<0x41BF>; // RO 0x00
using UPORT6R0  = CommonReg<0x41C0>; // (Unreachable IPv6 Port Register) RO 0x00
using UPORT6R1  = CommonReg<0x41C1>; // RO 0x00
using INTPTMR0  = CommonReg<0x41C5>; // (Interrupt Pending Time Register) R=W 0x00
using INTPTMR1  = CommonReg<0x41C6>; // R=W 0x00
using PLR       = CommonReg<0x41D0>; // (Prefix Length Register) RO 0x00
using PFR       = CommonReg<0x41D4>; // (Prefix Flag Register) RO 0x00
using VLTR0     = CommonReg<0x41D8>; // (Valid Life Time Register) RO 0x00
using VLTR1     = CommonReg<0x41D9>; // RO 0x00
using VLTR2     = CommonReg<0x41DA>; // RO 0x00
using VLTR3     = CommonReg<0x41DB>; // RO 0x00
using PLTR0     = CommonReg<0x41DC>; // (Preferred Life Time Register) RO 0x00
using PLTR1     = CommonReg<0x41DD>; // RO 0x00
using PLTR2     = CommonReg<0x41DE>; // RO 0x00
using PLTR3     = CommonReg<0x41DF>; // RO 0x00
using PAR0      = CommonReg<0x41E0>; // (Prefix Address Register) RO 0x00
using PAR1      = CommonReg<0x41E1>; // RO 0x00
using PAR2      = CommonReg<0x41E2>; // RO 0x00
using PAR3      = CommonReg<0x41E3>; // RO 0x00
using PAR4      = CommonReg<0x41E4>; // RO 0x00
using PAR5      = CommonReg<0x41E5>; // RO 0x00
using PAR6      = CommonReg<0x41E6>; // RO 0x00
using PAR7      = CommonReg<0x41E7>; // RO 0x00
using PAR8      = CommonReg<0x41E8>; // RO 0x00
using PAR9      = CommonReg<0x41E9>; // RO 0x00
using PAR10     = CommonReg<0x41EA>; // RO 0x00
using PAR11     = CommonReg<0x41EB>; // RO 0x00
using PAR12     = CommonReg<0x41EC>; // RO 0x00
using PAR13     = CommonReg<0x41ED>; // RO 0x00
using PAR14     = CommonReg<0x41EE>; // RO 0x00
using PAR15     = CommonReg<0x41EF>; // RO 0x00
using ICMP6BLKR = CommonReg<0x41F0>; // (ICMPv6 Block Register) R=W 0x00
using CHPLCKR   = CommonReg<0x41F4>; // (Chip Lock Register) WO 0x00
using NETLCKR   = CommonReg<0x41F5>; // (Network Lock Register) WO 0x00
using PHYLCKR   = CommonReg<0x41F6>; // (PHY Lock Register) WO 0x00
using RTR0      = CommonReg<0x4200>; // (Retransmission Time Register) R=W 0x07
using RTR1      = CommonReg<0x4201>; // R=W 0xD0
using RCR       = CommonReg<0x4204>; // (Retransmission Count Register) R=W 0x08
using SLRTR0    = CommonReg<0x4208>; // (SOCKET-less Retransmission Time Register) R=W 0x07
using SLRTR1    = CommonReg<0x4209>; // R=W 0xD0
using SLRCR     = CommonReg<0x420C>; // (SOCKET-less Retransmission Count Register) R=W 0x00
using SLHOPR    = CommonReg<0x420F>; // (Hop limit Register) R=W 0x80

static constexpr uint8_t NETLCKR_UNLOCK = 0x3A;
static constexpr uint8_t NETLCKR_LOCK   = 0xC5;

static constexpr uint8_t PHYACR_WRITE = 0x01;
static constexpr uint8_t PHYACR_READ = 0x02;

static constexpr uint8_t PHYRAR_BMSR = 0x01;
static constexpr uint16_t BMSR_LINK_STATUS = 0x2000;

}