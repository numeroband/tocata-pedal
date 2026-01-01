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
static wiz_NetInfo g_net_info = {
    .mac = {0x00, 0x08, 0xDC, 0x12, 0x34, 0x56}, // MAC address
    .ip = {192, 168, 11, 2},                     // IP address
    .sn = {255, 255, 255, 0},                    // Subnet Mask
    .gw = {192, 168, 11, 1},                     // Gateway
#if _WIZCHIP_ > W5500
    .lla = {
        0xfe, 0x80, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x02, 0x08, 0xdc, 0xff,
        0xfe, 0x57, 0x57, 0x25
    },             // Link Local Address
    .gua = {
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    },             // Global Unicast Address
    .sn6 = {
        0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    },             // IPv6 Prefix
    .gw6 = {
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    },             // Gateway IPv6 Address
    .dns = {8, 8, 8, 8},                         // DNS server
    .dns6 = {
        0x20, 0x01, 0x48, 0x60,
        0x48, 0x60, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x88, 0x88
    },             // DNS6 server
    .ipmode = NETINFO_STATIC_ALL,                // this 'ipmode' is never used in this project.
    .dhcp = NETINFO_DHCP,
#else
    .dns = {8, 8, 8, 8},                         // DNS server
    .dhcp = NETINFO_DHCP,
#endif
};
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
        wizchip_spi_initialize();
        wizchip_cris_initialize();

        wizchip_reset();
        wizchip_initialize();
        wizchip_check();

        _timer.start(3000);
        _state = State::WaitForPost;
        _callback = callback;

        return true;
    }

    void run() {
        uint8_t retval = 0;
        switch (_state) {
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
        Uninitialized,
        WaitForPost,
        Running,
    };

    State _state = State::Uninitialized;
    PollTimer _timer{};
    uint8_t _dhcp_retry = 0;
    Callback _callback;
    bool _ready = false;
};

}
