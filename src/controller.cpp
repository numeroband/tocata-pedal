#include "controller.h"

namespace tocata {

Controller::Controller(const std::array<uint8_t, kIns>& in_gpios, const std::array<uint8_t, kOuts>& out_gpios) 
    : _buttons(in_gpios, out_gpios), _midi(kHostname), _display() {}

void Controller::begin() 
{
    if (_config.load())
    {
        Serial.println("Config loaded");
    }
    else
    {
        Serial.println("Config not loaded");
    }

    _midi.setOnConnect(std::bind(&Controller::midiConnected, this));
    _midi.setOnDisconnect(std::bind(&Controller::midiDisconnected, this));
    _buttons.setCallback(std::bind(&Controller::buttonsChanged, this, std::placeholders::_1, std::placeholders::_2));

    _buttons.begin();
    _midi.begin();
    _display.begin();
    _server.begin(_config);
}

void Controller::loop() {
    _midi.loop();
    _buttons.loop();
    _display.setProgram(_counter);
    _counter = (millis() / 1000) % 128;
}

void Controller::buttonsChanged(std::bitset<kIns * kOuts> status, std::bitset<kIns * kOuts> modified)
{
    static bool toggle;

    auto activated = status & modified;
    if (activated.any())
    {
        toggle = !toggle;
        _midi.sendControl(55, toggle ? 127 : 0);
    }
}

void Controller::midiConnected()
{
    _display.setConnected(true);
}

void Controller::midiDisconnected()
{
    _display.setConnected(false);
}

}
