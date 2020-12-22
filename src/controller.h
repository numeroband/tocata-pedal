#pragma once

#include "buttons.h"
#include "fsmidi.h"
#include "display.h"
#include "config.h"
#include "server.h"

namespace tocata {

static constexpr size_t kIns = 2;
static constexpr size_t kOuts = 1;

class Controller
{
public:
    Controller(const std::array<uint8_t, kIns>& in_gpios, const std::array<uint8_t, kOuts>& out_gpios);
    void begin();
    void loop();

private:
    static constexpr const char* kHostname = "TocataPedal";

    void buttonsChanged(std::bitset<kIns * kOuts> status, std::bitset<kIns * kOuts> modified);
    void midiConnected();
    void midiDisconnected();

    Buttons<kIns, kOuts> _buttons;
    FsMidi _midi;
    Display _display;
    Config _config;
    Server _server{kHostname};
    uint8_t _counter = 0;
};

}
