#include "network.h"
#include "hal.h"

#include <cstdint>
#include <cstdio>


namespace tocata {

void Network::init() {
    if (!_config.available) {
        return;
    }
    printf("Initializing ethernet...\n");

    _dhcp.init([this](const uint8_t* ip) {
        if (ip) {
            // ::Ethernet.setLocalIP({ip});
            printf("IP ready!!!\n");
            _midi.init();
        } else {            
        }
    });
}

void Network::run() {
    if (!_config.available) {
        return;
    }

    _dhcp.run();
    if (_dhcp.ready()) {
        _midi.run();
    }
}

} // namespace tocata
