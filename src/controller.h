#pragma once

#include "buttons.h"
#include "fsmidi.h"
#include "display.h"
#include "config.h"
#include "server.h"

namespace tocata {

// static constexpr size_t kIns = 2;
// static constexpr size_t kOuts = 1;

class Controller
{
public:
    using Buttons6 = Buttons<3, 2>;
    
    Controller(const Buttons6::InArray& in_gpios, const Buttons6::OutArray& out_gpios);
    void begin();
    void loop();

private:

    static constexpr const char* kHostname = "TocataPedal";

    void buttonsChanged(Buttons6::Mask status, Buttons6::Mask modified);
    void midiConnected();
    void midiDisconnected();
    void updateProgram(uint8_t number);

    Buttons6 _buttons;
    FsMidi _midi;
    Display _display;
    Config _config;
    Config::Program _program{255};
    Server _server;
    uint8_t _counter = 0;
};

}
