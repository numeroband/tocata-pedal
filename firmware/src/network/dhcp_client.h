#pragma once

#include "hal.h"
#include "poll_timer.h"

#include <cstdint>

extern "C" {
#include <stdio.h>

#include "port_common.h"

#include "wizchip_conf.h"
#include "wizchip_spi.h"

#include "pico/unique_id.h"
}


namespace tocata {

/* Network */
static wiz_NetInfo g_net_info = {.dhcp = NETINFO_STATIC};

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

                network_initialize(g_net_info);

                /* Get network information */
                print_network_information(g_net_info);
                _ready = true;
                _callback(g_net_info.ip);

                _state = State::Running;
                break;
            case State::Running:
                break;
            default:
                return;
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
    Callback _callback;
    bool _ready = false;
    bool _connected = false;
};

}
