#include "controller.h"

#include "hal.h"

#include <functional>

using namespace std::placeholders;

namespace tocata {

void Controller::init()
{
    _usb.init();
    _usb.midi().setCallback(std::bind(&Controller::midiCallback, this, _1));
    _display.init();
    Storage::init();
    _leds.init();
    footswitchMode();
    _buttons.init();
    _exp.init();
}

void Controller::run() 
{
    _usb.run();
    _buttons.run();
    _exp.run();

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
            changeSwitch(sw, status[sw], true);
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
    } else if (activated[kSetupSwitch]) {
        setupMode();
    } else if (activated[kLoadSwitch]) {
        footswitchMode();
    }
}

void Controller::midiCallback(const uint8_t* packet)
{
    if (packet[0] == 0xC0) {
        loadProgram(packet[1], false, true);
    } else if (packet[0] == 0xB0 && packet[1] == 43) {
        changeSwitch(packet[2], true, false);
        changeSwitch(packet[2], false, false);
        _leds.refresh();
    }
}

void Controller::setupCallback(Switches::Mask status, Switches::Mask modified)
{
    auto activated = status & modified;

    if (activated[kExpMaxSwitch]) {        
        _exp.resetMax();
    } else if (activated[kExpMinSwitch]) {
        _exp.resetMin();
    } else if (activated[kExpEnabledSwitch]) {
        _expEnabled = !_expEnabled;
        _display.setFootswitch(kExpEnabledSwitch, _expEnabled ? "XPOFF" : "XPON");
    } else if (activated[kIncExpFilterSwitch]) {
        _exp.incFilter();
    } else if (activated[kDecExpFilterSwitch]) {
        _exp.decFilter();
    } else if (activated[kExitSwitch]) {
        footswitchMode();
    }
}

void Controller::setExpValue(uint8_t value) {
    constexpr uint8_t kNumDigits = 3;
    constexpr uint8_t kStart = sizeof(_expValue) - kNumDigits - 1;

    uint8_t remainder = value;

    for (uint8_t i = 0; i < kNumDigits; ++i)
    {
        uint8_t digit = remainder % 10;
        remainder /= 10;
        _expValue[kStart + (kNumDigits - 1 - i)] = '0' + digit;
    }

    _display.setText(_expValue);
}

void Controller::setupMode()
{
    loadProgram(_program_id, false, false);
    _display.setBlink(false);
    setExpValue(_exp.getValue());
    _display.clearSwitches();
    _display.setFootswitch(kExpMaxSwitch, "XPMAX");
    _display.setFootswitch(kExpMinSwitch, "XPMIN");
    _display.setFootswitch(kIncExpFilterSwitch, "FILT+");
    _display.setFootswitch(kDecExpFilterSwitch, "FILT-");
    _display.setFootswitch(kExpEnabledSwitch, _expEnabled ? "XPOFF" : "XPON");
    _display.setFootswitch(kExitSwitch, "EXIT");
    _switches_state.reset();
    _buttons.setCallback(std::bind(&Controller::setupCallback, this, _1, _2));
    _exp.setCallback(std::bind(&Controller::setExpValue, this, _1));
}

void Controller::changeProgramMode()
{
    _display.setBlink(true); 
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

void Controller::footswitchMode()
{
    _display.setBlink(false); 
    loadProgram(_program_id, true, true);
    _buttons.setCallback(std::bind(&Controller::footswitchCallback, this, _1, _2));
    _exp.setCallback(std::bind(&Controller::sendExpression, this, _1));
}

void Controller::changeSwitch(uint8_t id, bool active, bool send_midi)
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
    if (send_midi) {
        fs.run(_usb.midi(), _switches_state[id]);
    }
    _leds.setColor(id, fs.color(), _switches_state[id]);
}

void Controller::sendExpression(uint8_t value) 
{
    if (_expEnabled && _program.available() && _program.expressionEnabled())
    {
        _program.sendExpression(_usb.midi(), value);
    }
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
    displayProgram(display_switches);

    if (send_midi && _program.available())
    {
        _program.run(_usb.midi());
        sendExpression(_exp.getValue());
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