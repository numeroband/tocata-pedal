#include "controller.h"

namespace tocata {

Controller::Controller(const Buttons6::InArray& in_gpios, const Buttons6::OutArray& out_gpios) 
    : _buttons(in_gpios, out_gpios), _midi(kHostname), _server(kHostname, [this]{ updateConfig();}) {}

void Controller::begin() 
{
    _display.begin();
    
    updateConfig();

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
    static unsigned long last;
    unsigned long now = millis();  
    if (now - last > 70)
    {
        _display.loop();
        last = now;
    }    
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

void Controller::updateConfig()
{
    if (_config.load())
    {
        Serial.println("Config loaded");
    }
    else
    {
        Serial.println("Config couldn't be loaded");
        _config.save();
    }
    _program = _config.program(_program.number());
    _display.setProgram(_program);
}

}
