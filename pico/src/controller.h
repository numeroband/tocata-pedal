#pragma once

#include "switches.h"
#include "usb_device.h"
#include "display.h"
#include "config.h"
#include "hal.h"

namespace tocata {

class Controller : public Switches::Delegate, public WebUsb::Delegate
{
public:
    Controller(const HWConfig& config) : _usb(*this), _buttons(config.switches, *this), _display(config.display) {}
    void init();
    void run();

private:
    void switchesChanged(Switches::Mask status, Switches::Mask modified) override;
    void configChanged() override;
    void programChanged(uint8_t id) override;

    void updateProgram(uint8_t id);
    void updateConfig();
    void loadProgram(uint8_t id);

    UsbDevice _usb;
    Switches _buttons;
    Display _display;
    Program _program{};
    uint32_t _last_display_update;
    uint8_t _program_id;
    uint8_t _counter = 0;
};

}
