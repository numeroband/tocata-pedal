#pragma once

#include "hal.h"
#include "poll_timer.h"

#include <cstdint>

extern "C" {
#include <stdio.h>

#include "port_common.h"

#include "wizchip_conf.h"
#include "wizchip_spi.h"

#include "dhcp.h"

#include "timer.h"

#include "pico/unique_id.h"
}


/**
    ----------------------------------------------------------------------------------------------------
    Macros
    ----------------------------------------------------------------------------------------------------
*/

/* Buffer */
#define ETHERNET_BUF_MAX_SIZE (1024 * 2)

/* Socket */
#define SOCKET_DHCP 7

/* Retry count */
#define DHCP_RETRY_COUNT 5

/**
    ----------------------------------------------------------------------------------------------------
    Variables
    ----------------------------------------------------------------------------------------------------
*/
/* Network */
static wiz_NetInfo g_net_info = {.dhcp = NETINFO_STATIC};
//     .mac = {0x02}, // MAC address
//     .ip = {},                     // IP address
//     .sn = {},                    // Subnet Mask
//     .gw = {},                     // Gateway
// #if _WIZCHIP_ > W5500
//     .lla = {
//         0xfe, 0x80, 0x00, 0x00,
//         0x00, 0x00, 0x00, 0x00,
//         0x02, 0x08, 0xdc, 0xff,
//         0xfe, 0x57, 0x57, 0x25
//     },             // Link Local Address
//     .gua = {
//         0x00, 0x00, 0x00, 0x00,
//         0x00, 0x00, 0x00, 0x00,
//         0x00, 0x00, 0x00, 0x00,
//         0x00, 0x00, 0x00, 0x00
//     },             // Global Unicast Address
//     .sn6 = {
//         0xff, 0xff, 0xff, 0xff,
//         0xff, 0xff, 0xff, 0xff,
//         0x00, 0x00, 0x00, 0x00,
//         0x00, 0x00, 0x00, 0x00
//     },             // IPv6 Prefix
//     .gw6 = {
//         0x00, 0x00, 0x00, 0x00,
//         0x00, 0x00, 0x00, 0x00,
//         0x00, 0x00, 0x00, 0x00,
//         0x00, 0x00, 0x00, 0x00
//     },             // Gateway IPv6 Address
//     .dns = {},                         // DNS server
//     .dns6 = {
//         0x20, 0x01, 0x48, 0x60,
//         0x48, 0x60, 0x00, 0x00,
//         0x00, 0x00, 0x00, 0x00,
//         0x00, 0x00, 0x88, 0x88
//     },             // DNS6 server
//     .ipmode = NETINFO_STATIC_ALL,                // this 'ipmode' is never used in this project.
//     .dhcp = NETINFO_STATIC, // NETINFO_DHCP,
// #else
//     .dns = {8, 8, 8, 8},                         // DNS server
//     .dhcp = NETINFO_DHCP,
// #endif
// };

inline void mac_to_lla(const uint8_t* mac, uint8_t* outIpv6) {
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

inline void mac_from_serial(uint8_t* mac) {
    // Create a struct to hold the ID
    pico_unique_board_id_t board_id;

    // Fetch the ID from the hardware (Flash chip)
    pico_get_unique_board_id(&board_id);
    memcpy(mac, board_id.id + 2, 6);
    mac[0] = 0x02;
}

static uint8_t g_ethernet_buf[ETHERNET_BUF_MAX_SIZE] = {
    0,
}; // common buffer

/* DHCP */
static uint8_t g_dhcp_get_ip_flag = 0;

/* Timer */
static uint16_t g_msec_cnt = 0;

/**
    ----------------------------------------------------------------------------------------------------
    Functions
    ----------------------------------------------------------------------------------------------------
*/

/* DHCP */
static void wizchip_dhcp_init(void);
static void wizchip_dhcp_assign(void);
static void wizchip_dhcp_conflict(void);

/* Timer */
static void repeating_timer_callback(void);

/**
    ----------------------------------------------------------------------------------------------------
    Functions
    ----------------------------------------------------------------------------------------------------
*/

/* DHCP */
static void wizchip_dhcp_init(void) {
    printf(" DHCP client running\n");

    DHCP_init(SOCKET_DHCP, g_ethernet_buf);

    reg_dhcp_cbfunc(wizchip_dhcp_assign, wizchip_dhcp_assign, wizchip_dhcp_conflict);
}

static void wizchip_dhcp_assign(void) {
    getIPfromDHCP(g_net_info.ip);
    getGWfromDHCP(g_net_info.gw);
    getSNfromDHCP(g_net_info.sn);
    getDNSfromDHCP(g_net_info.dns);

    g_net_info.dhcp = NETINFO_DHCP;

    /* Network initialize */
    network_initialize(g_net_info); // apply from DHCP

    print_network_information(g_net_info);
    printf(" DHCP leased time : %ld seconds\n", getDHCPLeasetime());
}

static void wizchip_dhcp_conflict(void) {
    printf(" Conflict IP from DHCP\n");
}

/* Timer */
static void repeating_timer_callback(void) {
    g_msec_cnt++;

    if (g_msec_cnt >= 1000 - 1) {
        g_msec_cnt = 0;

        DHCP_time_handler();
    }
}


//////////////////////////////////////////////////////////////////

namespace tocata {
    
class DHCPClient {
public:
    using Callback = std::function<void(const uint8_t*)>;

    bool init(Callback callback) {
        mac_from_serial(g_net_info.mac);
        mac_to_lla(g_net_info.mac, g_net_info.lla);

        wizchip_spi_initialize();
        wizchip_cris_initialize();

        wizchip_reset();
        wizchip_initialize();
        wizchip_check();

        _callback = callback;

        return true;
    }

    void run() {
        uint8_t retval = 0;
        /* Check PHY link status */
        uint8_t temp;
        if (ctlwizchip(CW_GET_PHYLINK, (void *)&temp) == -1) {
            printf(" Unknown PHY link status\n");

            return;
        }
        bool connected = (temp == PHY_LINK_ON);
        if (_connected && !connected) {
            _state = State::Disconnected;
            _ready = false;
            printf("Ethernet disconnected\n");
        }
        _connected = connected;
        
        switch (_state) {
            case State::Disconnected:
                if (!_connected) {
                    return;
                }
                _state = State::WaitForPost;
                _timer.start(2000);
                return;
            case State::WaitForPost:
                if (!_timer.expired()) {
                    return; 
                }

                wizchip_1ms_timer_initialize(repeating_timer_callback);

                if (g_net_info.dhcp == NETINFO_DHCP) { // DHCP
                    wizchip_dhcp_init();
                } else { // static
                    network_initialize(g_net_info);

                    /* Get network information */
                    print_network_information(g_net_info);
                    _ready = true;
                    _callback(g_net_info.ip);                    
                }
                _state = State::Running;
                break;
            case State::Running:
                break;
            default:
                return;
        }

        /* Assigned IP through DHCP */
        if (g_net_info.dhcp == NETINFO_DHCP) {
            retval = DHCP_run();

            if (retval == DHCP_IP_LEASED) {
                if (g_dhcp_get_ip_flag == 0) {
                    printf(" DHCP success\n");

                    g_dhcp_get_ip_flag = 1;
                    _ready = true;
                    _callback(g_net_info.ip);
                }
            } else if (retval == DHCP_FAILED) {
                g_dhcp_get_ip_flag = 0;
                _dhcp_retry++;

                if (_dhcp_retry <= DHCP_RETRY_COUNT) {
                    printf(" DHCP timeout occurred and retry %d\n", _dhcp_retry);
                }
            }

            if (_dhcp_retry > DHCP_RETRY_COUNT) {
                printf(" DHCP failed\n");

                DHCP_stop();
            }

        }
    }

    bool ready() const { return _ready; }

private:
    enum class State {
        Disconnected,
        WaitForPost,
        Running,
    };

    State _state = State::Disconnected;
    PollTimer _timer{};
    uint8_t _dhcp_retry = 0;
    Callback _callback;
    bool _ready = false;
    bool _connected = false;
};

}
