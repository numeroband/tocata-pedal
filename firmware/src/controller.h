#pragma once

#include "switches.h"
#include "expression.h"
#include "leds.h"
#include "usb_device.h"
#include "display.h"
#include "config.h"
#include "network.h"
#include "hal.h"
#include "poll_timer.h"

#include <cmath>

#define CHANNEL_PREFIX "CH "
#define CHANNEL_TEXT CHANNEL_PREFIX "16"
#define EXP_VALUE_PREFIX CHANNEL_TEXT " - EXP = "
#define EXP_VALUE_TEXT EXP_VALUE_PREFIX "127"

namespace tocata {

class Controller : public ConfigProtocol::Delegate
{
public:
    Controller(const HWConfig& config) :
        _usb(*this),
        _buttons(config.switches),
        _sw_map{config.switches.map},
        _exp(config.expression),
        _leds(config.leds),
        _display(config.displayI2C, config.displaySPI, _switches_state),
        _network(config.ethernet)
    {
        // Gamma-correct (gamma = 2.2) lookup table: _gamma_table[level] is the
        // duty cycle that looks as bright to the eye as a linear ramp at
        // `level`, computed once here so displayTuner() avoids a runtime pow().
        for (int level = 0; level <= kLedMax; ++level) {
            _gamma_table[level] = static_cast<uint8_t>(kLedMax * std::pow(static_cast<float>(level) / kLedMax, 2.2f) + 0.5f);
        }
    }
    void init();
    void run();

private:
    void footswitchCallback(Switches::Mask status, Switches::Mask modified);
    void programCallback(Switches::Mask status, Switches::Mask modified);
    void setupCallback(Switches::Mask status, Switches::Mask modified);
    void setlistCallback(Switches::Mask status, Switches::Mask modified);
    void midiCallback(std::span<const uint8_t> packet, std::span<uint8_t> buffer, MidiSender& sender);

    void configChanged() override;
    void programChanged(uint8_t id) override;

    void sendIdentityReply(MidiSender& sender);
    void factoryReset();
    void footswitchMode(bool send_midi = true);
    void setupMode();
    void changeProgramMode();
    void setlistMode();
    bool selectSetlist(uint8_t setlist_id);
    void displaySetlistSelection();
    void tunerMode();
    void exitTunerMode(bool send_midi);
    void changeSwitch(uint8_t id, bool active, bool send_midi);
    bool stompLike(const Program& program, uint8_t id) const;
    void setSwitchEnabled(uint8_t id, bool enable);
    void sendExpression(uint8_t value);
    void updateProgram(uint8_t id);
    void updateConfig();
    void loadProgram(uint8_t id, bool send_midi, bool display_switches,
                     const std::bitset<Program::kNumSwitches>* restore_state = nullptr);
    void loadPosition(uint8_t pos, bool send_midi, bool display_switches,
                      const std::bitset<Program::kNumSwitches>* restore_state = nullptr);
    void movePosition(int8_t delta);
    void defaultSwitchesState(const Program& program, std::bitset<Program::kNumSwitches>& state) const;
    void applySceneToState(const Program& program, std::bitset<Program::kNumSwitches>& state, uint8_t scene_id) const;
    void displayProgram(bool display_switches);
    void setExpValue(uint8_t value);
    void displayTuner(uint8_t note, int64_t cents);
    uint8_t swMap(uint8_t sw) { return _sw_map[sw]; }

    UsbDevice _usb;
    Switches _buttons;
    const uint8_t* _sw_map;
    Expression _exp;
    Leds _leds;
    Display _display;
    Network _network;
    Config _config{};
    Program _program{};
    // The active setlist drives program-change navigation. It defaults to the
    // synthetic "All" setlist (every program, in order), which reproduces the
    // pre-setlist behavior exactly, and is not persisted across reboots.
    Setlist _setlist{};
    static constexpr uint8_t kAllSetlist = 0;   // cursor value for "All"
    uint8_t _setlist_id = kAllSetlist;          // 0 = All, 1..26 = Setlist id 0..25
    uint8_t _setlist_pos = 0;                   // cursor within _setlist
    uint8_t _program_id = 0;
    uint8_t _fs_id = 0;
    uint8_t _program_sw_id = Program::kInvalidId;  // cached id of the kProgram switch for
                                                    // the current program, or kInvalidId
    uint8_t _saved_program_id = 0;
    uint8_t _saved_setlist_pos = 0;
    // Setlist menu: the ids that are actually selectable, rebuilt on entry so
    // empty slots are never reachable. Entry 0 is always kAllSetlist.
    uint8_t _sel_setlists[Setlist::kMaxSetlists + 1] = {};
    uint8_t _num_sel_setlists = 0;
    uint8_t _sel_setlist_idx = 0;
    // Display::setText keeps the pointer rather than a copy, so the browsed name
    // has to outlive the call.
    char _sel_setlist_name[Program::kMaxNameLength + 1] = "";
    std::bitset<Program::kNumSwitches> _saved_switches_state{};
    bool _restore_state = false;
    uint8_t _counter = 0;
    std::bitset<Program::kNumSwitches> _switches_state{};
    char _expValue[sizeof(EXP_VALUE_TEXT)]{EXP_VALUE_TEXT};
    bool _tuner_mode = false;
    uint8_t _pendingChannel = 0;

    static constexpr int kLedMax = 128;  // max LED intensity, also used by displayTuner()
    uint8_t _gamma_table[kLedMax + 1] = {};

    static constexpr uint8_t kIncOneSwitch = 0;
    static constexpr uint8_t kIncTenSwitch = 1;
    static constexpr uint8_t kSetupSwitch = 2;
    static constexpr uint8_t kTunerSwitch = 3;
    const uint8_t kDecOneSwitch = uint8_t(_leds.kNumLeds / 2);
    const uint8_t kDecTenSwitch = uint8_t(_leds.kNumLeds / 2 + 1);
    const uint8_t kLoadSwitch = uint8_t(_leds.kNumLeds - 1);
    const uint8_t kExitProgramSwitch = uint8_t(_leds.kNumLeds / 2 + 2);

    static constexpr uint8_t kExpMaxSwitch = 0;
    static constexpr uint8_t kIncExpFilterSwitch = 1;
    static constexpr uint8_t kIncChannelSwitch = 2;
    const uint8_t kSetlistsSwitch = uint8_t(_leds.kNumLeds / 2 - 1);
    const uint8_t kExpMinSwitch = uint8_t(_leds.kNumLeds / 2);
    const uint8_t kDecExpFilterSwitch = uint8_t(_leds.kNumLeds / 2 + 1);
    const uint8_t kDecChannelSwitch = uint8_t(_leds.kNumLeds - 2);
    const uint8_t kExitSwitch = uint8_t(_leds.kNumLeds - 1);

    PollTimer _display_timer{};
};

}
