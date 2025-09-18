#pragma once

#include "switches.h"
#include "expression.h"
#include "leds.h"
#include "usb_device.h"
#include "display.h"
#include "config.h"
#include "network.h"
#include "hal.h"

#define EXP_VALUE_PREFIX "EXP = "
#define EXP_VALUE_TEXT EXP_VALUE_PREFIX "127"

namespace tocata {

class Controller : public WebUsb::Delegate
{
public:
    Controller(const HWConfig& config) : 
        _usb(*this), 
        _buttons(config.switches), 
        _exp(config.expression),
        _leds(config.leds), 
        _display(config.display, _switches_state) {}
    void init();
    void run();

private:
    void footswitchCallback(Switches::Mask status, Switches::Mask modified);
    void programCallback(Switches::Mask status, Switches::Mask modified);
    void setupCallback(Switches::Mask status, Switches::Mask modified);
    void midiCallback(std::span<const uint8_t> packet, std::span<uint8_t> buffer, MidiSender& sender);

    void configChanged() override;
    void programChanged(uint8_t id) override;

    void footswitchMode(bool send_midi = true);
    void setupMode();
    void changeProgramMode();
    void changeSwitch(uint8_t id, bool active, bool send_midi);
    void sendExpression(uint8_t value);
    void updateProgram(uint8_t id);
    void updateConfig();
    void loadProgram(uint8_t id, bool send_midi, bool display_switches);
    void displayProgram(bool display_switches);
    void setExpValue(uint8_t value);
    void displayTuner(uint8_t note, int64_t cents);

    UsbDevice _usb;
    Switches _buttons;
    Expression _exp;
    Leds _leds;
    Display _display;
    Program _program{};
    Network _network{};
    uint32_t _last_display_update;
    uint8_t _program_id = 0;
    uint8_t _fs_id = 0;
    uint8_t _counter = 0;
    bool _expEnabled = true;
    std::bitset<Program::kNumSwitches> _switches_state{};
    char _expValue[sizeof(EXP_VALUE_TEXT)]{EXP_VALUE_TEXT};

    static constexpr uint8_t kIncOneSwitch = 0;
    static constexpr uint8_t kIncTenSwitch = 1;
    static constexpr uint8_t kSetupSwitch = 2;
    const uint8_t kDecOneSwitch = _leds.kNumLeds / 2;
    const uint8_t kDecTenSwitch = _leds.kNumLeds / 2 + 1;
    const uint8_t kLoadSwitch = _leds.kNumLeds / 2 + 2;

    static constexpr uint8_t kExpMaxSwitch = 0;
    static constexpr uint8_t kIncExpFilterSwitch = 1;
    static constexpr uint8_t kExpEnabledSwitch = 2;
    const uint8_t kExpMinSwitch = uint8_t(_buttons.kNumSwitches / 2);
    const uint8_t kDecExpFilterSwitch = uint8_t(_buttons.kNumSwitches / 2 + 1);
    const uint8_t kExitSwitch = uint8_t(_buttons.kNumSwitches / 2 + 2);
};

}
