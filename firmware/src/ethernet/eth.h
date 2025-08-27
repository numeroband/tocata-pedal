#pragma once

#include <cstdint>
#include <midi_sender.h>

namespace tocata {

class Ethernet : public MidiSender {
public:
    void init();
    void run();
	void sendProgram(uint8_t channel, uint8_t program) override;
	void sendControl(uint8_t channel, uint8_t control, uint8_t value) override;

private:
    void runDHCP();

    uint32_t _last_run = 0;
    bool _init = false;
    uint32_t _init_millis = 0;
    uint32_t _dhcp_init_ms;
};

}