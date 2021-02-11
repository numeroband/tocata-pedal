#include "controller.h"

#include "hal.h"

namespace tocata {

void Controller::init()
{
    _usb.init();
    _display.init();
    Storage::init();

    loadProgram(0);

    _buttons.init();
}

void Controller::run() 
{
    _usb.run();
    _buttons.run();

    uint32_t now = millis();
    if (now - _last_display_update > 70)
    {
        _display.run();
        _last_display_update = now;
    }
}

void Controller::switchesChanged(Switches::Mask status, Switches::Mask modified)
{
    static bool toggle;

    auto activated = status & modified;
    if (activated.any())
    {
        loadProgram((_program_id + 1) % 4);
        toggle = !toggle;
        _usb.midi().sendControl(55, toggle ? 127 : 0);
    }
}

void Controller::configChanged()
{
}

void Controller::programChanged(uint8_t id)
{
    if (id == _program_id)
    {
        loadProgram(id);
    }
}

void Controller::loadProgram(uint8_t id)
{    
    _program_id = id;
    _program.load(id);
    _display.setProgram(id, _program);
    if (_program.available())
    {
        _program.run(_usb.midi());
    }
}

}