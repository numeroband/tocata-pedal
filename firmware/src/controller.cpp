#include "controller.h"

#include "hal.h"

#include <functional>

using namespace std::placeholders;

namespace tocata {

void Controller::init()
{
    _usb.init();
    _display.init();
    Storage::init();
    _leds.init();
    _buttons.setCallback(std::bind(&Controller::footswitchCallback, this, _1, _2));
    _buttons.init();
    loadProgram(0, true, true);
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

void Controller::footswitchCallback(Switches::Mask status, Switches::Mask modified)
{
    auto activated = status & modified;

    if (activated.count() > 1)
    {
        changeProgramMode();
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

void Controller::programCallback(Switches::Mask status, Switches::Mask modified)
{
    auto activated = status & modified;

    if (activated[kIncOneSwitch]) {
        loadProgram((_program_id + 1) % 99, false, false);
    } else if (activated[kIncTenSwitch]) {
        loadProgram((_program_id + 10) % 99, false, false);
    } else if (activated[kDecOneSwitch]) {
        loadProgram((_program_id + 99 - 1) % 99, false, false);
    } else if (activated[kDecTenSwitch]) {
        loadProgram((_program_id + 99 - 10) % 99, false, false);
    } else if (activated[kLoadSwitch]) {
        loadProgram(_program_id, true, true);
        _buttons.setCallback(std::bind(&Controller::footswitchCallback, this, _1, _2));
    }
}

void Controller::changeProgramMode()
{
    loadProgram(_program_id, false, false);
    _display.setFootswitch(kIncOneSwitch, " +1");
    _display.setFootswitch(kIncTenSwitch, " +10");
    _display.setFootswitch(kSetupSwitch, "SETUP");
    _display.setFootswitch(kDecOneSwitch, " -1");
    _display.setFootswitch(kDecTenSwitch, " -10");
    _display.setFootswitch(kLoadSwitch, "LOAD");
    _switches_state.reset();
    _buttons.setCallback(std::bind(&Controller::programCallback, this, _1, _2));
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
        loadProgram(id, true, true);
    }
}

void Controller::loadProgram(uint8_t id, bool send_midi, bool display_switches)
{   
    _program_id = id;
    _program.load(id);
    _display.setBlink(!display_switches); 
    displayProgram(display_switches);

    if (send_midi && _program.available())
    {
        _program.run(_usb.midi());
    }

    bool is_scene = (_program.mode() == Program::kScene);
    for (uint8_t id = 0; id < Switches::kNumSwitches; ++id)
    {
        if (!display_switches)
        {
            _leds.setColor(id, kWhite, false);
            continue;
        }

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

void Controller::displayProgram(bool display_switches) {
    _display.setNumber(_program_id + 1);
    _display.setText(_program.available() ? _program.name() : "<EMPTY>");
    if (!display_switches)
    {
        return;
    }

    for (uint8_t idx = 0; idx < Program::kNumSwitches; ++idx) {
        auto& fs = _program.footswitch(idx);
        _display.setFootswitch(idx, fs.available() ? fs.name() : nullptr);
    }
}

}