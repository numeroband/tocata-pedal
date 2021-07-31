#pragma once

#include "switches.h"
#include "leds.h"
#include "usb_device.h"
#include "display.h"
#include "config.h"
#include "hal.h"

namespace tocata {

class Controller : public WebUsb::Delegate
{
public:
    Controller(const HWConfig& config) : 
        _usb(*this), 
        _buttons(config.switches), 
        _leds(config.leds), 
        _display(config.display, _switches_state) {}
    void init();
    void run();

private:
    static constexpr uint8_t kIncOneSwitch = 0;
    static constexpr uint8_t kIncTenSwitch = 1;
    static constexpr uint8_t kSetupSwitch = 2;
    static constexpr uint8_t kDecOneSwitch = Program::kNumSwitches / 2;
    static constexpr uint8_t kDecTenSwitch = Program::kNumSwitches / 2 + 1;
    static constexpr uint8_t kLoadSwitch = Program::kNumSwitches / 2 + 2;

    void footswitchCallback(Switches::Mask status, Switches::Mask modified);
    void programCallback(Switches::Mask status, Switches::Mask modified);

    void configChanged() override;
    void programChanged(uint8_t id) override;

    void changeProgramMode();
    void changeSwitch(uint8_t id, bool active);
    void updateProgram(uint8_t id);
    void updateConfig();
    void loadProgram(uint8_t id, bool send_midi, bool display_switches);
    void displayProgram(bool display_switches);

    UsbDevice _usb;
    Switches _buttons;
    Leds _leds;
    Display _display;
    Program _program{};
    uint32_t _last_display_update;
    uint8_t _program_id;
    uint8_t _counter = 0;
    std::bitset<Program::kNumSwitches> _switches_state = {};
};

}
