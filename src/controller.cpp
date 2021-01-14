#include "controller.h"

namespace tocata {

using namespace std::placeholders;

Controller::Controller(const Buttons6::InArray& in_gpios, const Buttons6::OutArray& out_gpios) 
    : _buttons(in_gpios, out_gpios), _midi(kHostname), _server(kHostname, 
    std::bind(&Controller::updateConfig, this),
    std::bind(&Controller::updateProgram, this, _1)) {}

void Controller::begin() 
{
    _display.begin();
    
    loadProgram(0);

    _midi.setOnConnect(std::bind(&Controller::midiConnected, this));
    _midi.setOnDisconnect(std::bind(&Controller::midiDisconnected, this));
    _buttons.setCallback(std::bind(&Controller::buttonsChanged, this, _1, _2));

    _buttons.begin();
    _midi.begin();
    _server.begin();
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
    _server.loop();
}

void Controller::buttonsChanged(Buttons6::Mask status, Buttons6::Mask modified)
{
    static bool toggle;

    auto activated = status & modified;
    if (activated.any())
    {
        loadProgram((_program.id() + 1) % 4);
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

void Controller::loadProgram(uint8_t id)
{    
    _program.load(id);
    _display.setProgram(_program);
    if (_program.available())
    {
        _program.run(_midi);
    }
}

void Controller::updateProgram(uint8_t id)
{
    if (id == _program.id())
    {
        loadProgram(id);
    }
}

void Controller::updateConfig()
{
    loadProgram(0);
}

}
