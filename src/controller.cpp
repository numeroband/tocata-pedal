#include "controller.h"

namespace tocata {

Controller::Controller(const Buttons6::InArray& in_gpios, const Buttons6::OutArray& out_gpios) 
    : _buttons(in_gpios, out_gpios), _midi(kHostname), _program(_config.program(0)) {}

void Controller::begin() 
{
    _display.begin();

    if (_config.load())
    {
        Serial.println("Config loaded");
    }
    else
    {
        Serial.println("Config not loaded");
    }

    _program = _config.program(1);
    _display.setProgram(_program);
    _midi.setOnConnect(std::bind(&Controller::midiConnected, this));
    _midi.setOnDisconnect(std::bind(&Controller::midiDisconnected, this));
    _buttons.setCallback(std::bind(&Controller::buttonsChanged, this, std::placeholders::_1, std::placeholders::_2));

    _buttons.begin();
    _midi.begin();
    _server.begin(_config);
}

void Controller::loop() {
    _midi.loop();
    _buttons.loop();
    _counter = (millis() / 1000) % 128;
}

void Controller::buttonsChanged(Buttons6::Mask status, Buttons6::Mask modified)
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
