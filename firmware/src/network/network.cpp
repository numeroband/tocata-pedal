#include "network.h"
#include "hal.h"

#include <cstdint>
#include <cstdio>


namespace tocata {

void Network::init(uint8_t midi_port) {
    if (!_config.available) {
        return;
    }

    printf("Initializing ethernet...\n");
    _eth.init();
    _midi.init(midi_port);
}

void Network::reinitMidi(uint8_t midi_port) {
    if (!_config.available) {
        return;
    }
    _midi.reinit(midi_port);
}

void Network::run() {
    if (!_config.available) {
        return;
    }

    bool connected = _eth.ready();
    if (connected != _connected) {
        _connected = connected;
        printf("Ethernet %sconnected\n", connected ? "" : "dis");
        if (_connected && _onLinkUp) {
            _onLinkUp();
        }
    }

    if (_connected) {
        _midi.run();
    }
}

Ethernet* EthernetUDP6::gEth;

} // namespace tocata
