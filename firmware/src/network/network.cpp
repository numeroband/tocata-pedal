#include "network.h"
#include <cstdint>
#include <cstdio>
#include "hal.h"

extern "C" {
#include "wizchip_spi.h"
#include "dhcp.h"
#include "timer.h"
}

unsigned long t1 = millis();
int8_t isConnected = 0;

//////////////

/* ----------------------------------------------------------------------------------------------------
 * Macros
 * ----------------------------------------------------------------------------------------------------
 */
/* Clock */
#define PLL_SYS_KHZ (133 * 1000)

/* Buffer */
#define ETHERNET_BUF_MAX_SIZE (1024 * 2)

/* Socket */
#define SOCKET_DHCP 7

/* Retry count */
#define DHCP_RETRY_COUNT 5

/**
 * ----------------------------------------------------------------------------------------------------
 * Variables
 * ----------------------------------------------------------------------------------------------------
 */
/* Network */
static wiz_NetInfo g_net_info =
    {
        .mac = {0x00, 0x08, 0xDC, 0x12, 0x34, 0x56}, // MAC address
        .ip = {192, 168, 2, 19},                     // IP address
        .sn = {255, 255, 255, 0},                    // Subnet Mask
        .gw = {192, 168, 2, 1},                     // Gateway
        .dns = {192, 168, 2, 1},                         // DNS server
        .dhcp = NETINFO_DHCP,
        // .dhcp = NETINFO_STATIC,
};
static uint8_t g_ethernet_buf[ETHERNET_BUF_MAX_SIZE] = {
    0,
}; // common buffer

/* DHCP */
static uint8_t g_dhcp_get_ip_flag = 0;

/* Timer */
static volatile uint16_t g_msec_cnt = 0;

/**
 * ----------------------------------------------------------------------------------------------------
 * Functions
 * ----------------------------------------------------------------------------------------------------
 */
/* Clock */
static void set_clock_khz(void);

/* DHCP */
static void wizchip_dhcp_init(void);
static void wizchip_dhcp_assign(void);
static void wizchip_dhcp_conflict(void);

/* Timer */
static void repeating_timer_callback(void);

///

namespace tocata {

void Network::init() {
    printf("Initializing ethernet...\n");

    set_clock_khz();
    wizchip_spi_initialize();
    wizchip_cris_initialize();    
    wizchip_reset();
    wizchip_initialize();
    wizchip_check();

    _init_millis = millis();
}

void Network::postInit() {
    wizchip_1ms_timer_initialize(repeating_timer_callback);

    if (g_net_info.dhcp == NETINFO_DHCP) // DHCP
    {
        wizchip_dhcp_init();
    }
    else // static
    {
        network_initialize(g_net_info);
        ::Ethernet.setLocalIP({g_net_info.ip});

        /* Get network information */
        print_network_information(g_net_info);
    }
    printf("eth init finished\n");
}

void Network::run() {
    if (!_init) {
      if ((millis() - _init_millis) < 3000) {
          return;
      }
      _init = true;
      postInit();
    }

    if (g_dhcp_get_ip_flag) {
      _midi.run();
    } else {
        runDHCP();
        if (g_dhcp_get_ip_flag) {
          _midi.init();
        }
    }
}

void Network::runDHCP() {

    /* Initialize */
    static uint8_t retval = 0;
    static uint8_t dhcp_retry = 0;

    /* Assigned IP through DHCP */
    if (g_net_info.dhcp != NETINFO_DHCP)
    {
      g_dhcp_get_ip_flag = 1;
    }

    if (g_dhcp_get_ip_flag)
    {
      return;
    }

    retval = DHCP_run();

    if (retval == DHCP_IP_LEASED)
    {
        if (g_dhcp_get_ip_flag == 0)
        {
            printf(" DHCP success\n");

            g_dhcp_get_ip_flag = 1;
            // DHCP_stop();
        }
    }
    else if (retval == DHCP_FAILED)
    {
        g_dhcp_get_ip_flag = 0;
        dhcp_retry++;

        if (dhcp_retry <= DHCP_RETRY_COUNT)
        {
            printf(" DHCP timeout occurred and retry %d\n", dhcp_retry);
        }
    }

    if (dhcp_retry > DHCP_RETRY_COUNT)
    {
        printf(" DHCP failed\n");

        DHCP_stop();
    }
}

} // namespace tocata

/**
 * ----------------------------------------------------------------------------------------------------
 * Functions
 * ----------------------------------------------------------------------------------------------------
 */
/* Clock */
static void set_clock_khz(void)
{
    // set a system clock frequency in khz
    set_sys_clock_khz(PLL_SYS_KHZ, true);

    // configure the specified clock
    clock_configure(
        clk_peri,
        0,                                                // No glitchless mux
        CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, // System PLL on AUX mux
        PLL_SYS_KHZ * 1000,                               // Input frequency
        PLL_SYS_KHZ * 1000                                // Output (must be same as no divider)
    );
}

/* DHCP */
static void wizchip_dhcp_init(void)
{
    printf(" DHCP client running\n");

    DHCP_init(SOCKET_DHCP, g_ethernet_buf);

    reg_dhcp_cbfunc(wizchip_dhcp_assign, wizchip_dhcp_assign, wizchip_dhcp_conflict);
}

static void wizchip_dhcp_assign(void)
{
    getIPfromDHCP(g_net_info.ip);
    getGWfromDHCP(g_net_info.gw);
    getSNfromDHCP(g_net_info.sn);
    getDNSfromDHCP(g_net_info.dns);

    Ethernet.setLocalIP({g_net_info.ip});
    g_net_info.dhcp = NETINFO_DHCP;

    /* Network initialize */
    network_initialize(g_net_info); // apply from DHCP

    print_network_information(g_net_info);
    printf(" DHCP leased time : %ld seconds\n", getDHCPLeasetime());
}

static void wizchip_dhcp_conflict(void)
{
    printf(" Conflict IP from DHCP\n");
}

/* Timer */
static void repeating_timer_callback(void)
{
    g_msec_cnt += 1;

    if (g_msec_cnt >= 1000 - 1)
    {
        g_msec_cnt = 0;

        DHCP_time_handler();
    }
}
