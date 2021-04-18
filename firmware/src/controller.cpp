#include "controller.h"

#include "hal.h"

namespace tocata {

void Controller::init()
{
    _usb.init();
    _display.init();
    Storage::init();
    _leds.init();
    _buttons.init();

    loadProgram(0);
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
    auto activated = status & modified;

    if (activated[0] && activated[3]) // Hardcoded
    {
        loadProgram((_program_id - 1) % 99);
        return;
    }

    if (activated[1] && activated[4]) // Hardcoded
    {
        loadProgram((_program_id + 1) % 99);
        return;
    }

    for (uint8_t sw = 0; sw < Switches::kNumSwitches; ++sw)
    {
        if (modified[sw])
        {
            changeSwitch(sw, status[sw]);
        }
    }
    _leds.refresh();
}

void Controller::changeSwitch(uint8_t id, bool active)
{
    const auto& fs = _program.footswitch(id);
    bool is_scene = (_program.mode() == Program::kScene);
    bool momentary = !is_scene && fs.momentary();
    if (id >= _program.numFootswitches() || !fs.available() || (!momentary && !active))
    {
        return;
    }

    if (is_scene)
    {
        _switches_state.reset();
        for (uint8_t id = 0; id < Switches::kNumSwitches; ++id)
        {
            const auto& fs = _program.footswitch(id);
            if (id >= _program.numFootswitches() || !fs.available())
            {
                _leds.setColor(id, kNone, false);
            }
            else
            {
                _leds.setColor(id, fs.color(), false);
            }
        }
    }

    _switches_state[id] = momentary ? (active ^ fs.enabled()) : !_switches_state[id];
    fs.run(_usb.midi(), _switches_state[id]);
    _leds.setColor(id, fs.color(), _switches_state[id]);
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

    bool is_scene = (_program.mode() == Program::kScene);
    for (uint8_t id = 0; id < Switches::kNumSwitches; ++id)
    {
        const auto& fs = _program.footswitch(id);
        if (id >= _program.numFootswitches() || !fs.available())
        {
            _leds.setColor(id, kNone, false);
        }
        else
        {
            _switches_state[id] = is_scene ? (id == 0) : fs.enabled();
            _leds.setColor(id, fs.color(), _switches_state[id]);
        }
    }
    _leds.refresh();
}

}