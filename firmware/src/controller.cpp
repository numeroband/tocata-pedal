#include "controller.h"

#include "hal.h"

#include <algorithm>
#include <functional>
#include <midi_sysex.h>

using namespace std::placeholders;

namespace tocata {

void Controller::init()
{
    _usb.init();
    _usb.midi().setCallback(std::bind(&Controller::midiCallback, this, _1, _2, _3));
    _display.init();
    //
    Storage::init();
    _config.load();
    _buttons.init();
    _exp.init();
    _exp.setCalibration(_config.expression().minRaw(), _config.expression().maxRaw());
    _exp.run();
    _leds.init();

    PollTimer leds_timer;
    for (uint8_t i = 0; is_pedal_long() && i < 128; ++i)
    {
        while (!leds_timer.expired()) {
            _usb.run();
        }
        leds_timer.restart(15000);
        for (uint8_t led = 0; led < Leds::kMaxLeds; ++led)
        {
            _leds.setColor(led, i, i, i);
        }
        _leds.refresh();
        _leds.run();
    }
    footswitchMode(false);

    _network.init(_config.midi().channel());
    _network.midi().setCallback(std::bind(&Controller::midiCallback, this, _1, _2, _3));
    _network.setOnLinkUp([this]{
        sendIdentityReply(_network.midi());
        sendExpression(_exp.getValue());
    });

    sendIdentityReply(_usb.midi());

    sendExpression(_exp.getValue());
}

void Controller::sendIdentityReply(MidiSender& sender)
{
    // MIDI Universal Non-Realtime SysEx — Identity Reply (F0 7E <ch> 06 02)
    const uint8_t reply[] = {
        0xF0,
        0x7E,                               // Universal Non-Realtime
        _config.midi().channel(),           // Device ID
        0x06, 0x02,                         // General Information / Identity Reply
        0x00, 0x2F, 0x7F,                   // Manufacturer ID
        0x00, 0x00,                         // Device family code
        0x00, 0x00,                         // Device family member code
        VERSION_MAJOR, VERSION_MINOR, VERSION_SUBMINOR, 0x00,  // Software revision
        0xF7,
    };
    sender.sendSysEx(reply);
}

void Controller::run() 
{    
    _usb.run();
    _buttons.run();
    _exp.run();
    _network.run();
    _leds.run();

    if (_display_timer.expired())
    {
        static uint32_t display_runs = 0;
        static uint32_t total_time = 0;
        auto start = millis();
         _display.run();
        total_time += millis() - start;
        if (++display_runs >= 30) {  // ~1s at 30 Hz
            printf("display average: %u\n", (total_time * 1000) / display_runs);
            total_time = 0;
            display_runs = 0;
        }
        _display_timer.restart(33333);  // 33333us interval = exact 30 Hz
    }
}

void Controller::footswitchCallback(Switches::Mask status, Switches::Mask modified)
{
    auto activated = status & modified;

    if (activated[swMap(kProgramSwitch)])
    {
        _saved_program_id = _program_id;
        _saved_switches_state = _switches_state;
        _restore_state = true;
        changeProgramMode();
        return;
    }

    for (uint8_t sw = 0; sw < Program::kNumSwitches; ++sw)
    {
        if (modified[swMap(sw)])
        {
            changeSwitch(sw, status[swMap(sw)], true);
        }
    }
    constexpr uint8_t inc_one = Program::kNumSwitches;
    constexpr uint8_t dec_one = Program::kNumSwitches + 1;

    if (activated[swMap(inc_one)]) {
        loadProgram((_program_id + 1) % 99, false, false);
        footswitchMode();
    } else if (activated[swMap(dec_one)]) {
        loadProgram((_program_id + 99 - 1) % 99, false, false);
        footswitchMode();
    } else {
        _leds.refresh();
    }
}

void Controller::programCallback(Switches::Mask status, Switches::Mask modified)
{
    auto activated = status & modified;

    if (activated[swMap(kIncOneSwitch)]) {
        loadProgram((_program_id + 1) % 99, false, false);
    } else if (activated[swMap(kIncTenSwitch)]) {
        loadProgram((_program_id + 10) % 99, false, false);
    } else if (activated[swMap(kDecOneSwitch)]) {
        loadProgram((_program_id + 99 - 1) % 99, false, false);
    } else if (activated[swMap(kDecTenSwitch)]) {
        loadProgram((_program_id + 99 - 10) % 99, false, false);
    } else if (activated[swMap(kSetupSwitch)]) {
        setupMode();
    } else if (activated[swMap(kLoadSwitch)]) {
        // LOAD always reloads the selected program and (re)sends its MIDI.
        _restore_state = false;
        footswitchMode(true);
    } else if (activated[swMap(kExitProgramSwitch)]) {
        // EXIT leaves change-program mode and restores the program/scene that
        // was live before entering, without sending MIDI (the amp is already on it).
        footswitchMode(false);
    } else if (activated[swMap(kTunerSwitch)]) {
        tunerMode();
    }
}

void Controller::midiCallback(std::span<const uint8_t> packet, std::span<uint8_t> buffer, MidiSender& sender)
{
    uint8_t channel = _config.midi().channel();
    while (!packet.empty()) {
        uint8_t msg_channel = packet[0] & 0x0F;
        uint8_t msg_type = packet[0] & 0xF0;
        if (msg_channel == channel && msg_type == 0xC0) {
            uint8_t value = packet[1];
            if (_tuner_mode) {
                Program target;
                target.load(value);
                _saved_program_id = value;
                defaultSwitchesState(target, _saved_switches_state);
                _restore_state = true;
            } else {
                footswitchMode(false);
                loadProgram(value, false, true);
            }
        } else if (msg_channel == channel && msg_type == 0xB0 && packet[1] == 43) {
            if (_tuner_mode) {
                // _saved_program_id already reflects whatever program is pending for
                // exit (the live one, or a program deferred by an earlier PC while
                // tuning) -- only the scene within that program changes here, so the
                // saved stomp toggles are preserved.
                Program target;
                target.load(_saved_program_id);
                applySceneToState(target, _saved_switches_state, packet[2]);
                _restore_state = true;
            } else {
                footswitchMode(false);
                changeSwitch(packet[2], true, false);
                changeSwitch(packet[2], false, false);
                sleep_ms(1);
                _leds.refresh();
            }
        } else if (msg_channel == channel && msg_type == 0xB0 && packet[1] == 47) {
            uint8_t value = packet[2];
            if (value > 0 && !_tuner_mode) {
                tunerMode();
            } else if (value == 0 && _tuner_mode) {
                exitTunerMode(false);
            }
        } else if (msg_channel == channel && msg_type == 0x90) {
            int8_t velocity = packet[2];
            uint8_t note = packet[1];
            if (velocity > 63) {
                ++note;
                velocity -= 128;
            }
            displayTuner(note, velocity);
        } else if (msg_channel == channel && msg_type == 0x80 && packet[1] == 0) {
            displayTuner(0, 0);
        } else if (packet[0] == 0xF0) {
            // MIDI Universal Non-Realtime Identity Request: F0 7E <id> 06 01 F7
            if (packet.size() == 6 &&
                packet[1] == 0x7E &&
                (packet[2] == 0x7F || packet[2] == channel) &&
                packet[3] == 0x06 && packet[4] == 0x01 &&
                packet[5] == 0xF7)
            {
                sendIdentityReply(sender);
                return;
            }
            auto response = _usb.config().processSysEx(packet, buffer, channel);
            if (response.size() > 0) {
                sender.sendSysEx(response);
            }
        }

        size_t next = 1;
        while (next < packet.size() && !(packet[next] & 0x80)) {
            ++next;
        }
        packet = packet.subspan(next);
    }
}

void Controller::setupCallback(Switches::Mask status, Switches::Mask modified)
{
    auto activated = status & modified;

    if (activated[swMap(kExpMaxSwitch)]) {        
        _exp.resetMax();
    } else if (activated[swMap(kExpMinSwitch)]) {
        _exp.resetMin();
    } else if (activated[swMap(kExpEnabledSwitch)]) {
        _expEnabled = !_expEnabled;
        _display.setFootswitch(kExpEnabledSwitch, _expEnabled ? "XPOFF" : "XPON");
    } else if (activated[swMap(kIncExpFilterSwitch)]) {
        _exp.incFilter();
    } else if (activated[swMap(kDecExpFilterSwitch)]) {
        _exp.decFilter();
    } else if (activated[swMap(kIncChannelSwitch)]) {
        _pendingChannel = (_pendingChannel + 1) % 16;
        setExpValue(_exp.getValue());
    } else if (activated[swMap(kDecChannelSwitch)]) {
        _pendingChannel = (_pendingChannel + 15) % 16;
        setExpValue(_exp.getValue());
    } else if (activated[swMap(kExitSwitch)]) {
        if (_pendingChannel != _config.midi().channel()) {
            _config.midi().setChannel(_pendingChannel);
            _network.reinitMidi(_pendingChannel);
            sendIdentityReply(_usb.midi());
            sendIdentityReply(_network.midi());
        }
        _config.expression().setMinRaw(_exp.getMinRaw());
        _config.expression().setMaxRaw(_exp.getMaxRaw());
        _config.save();
        footswitchMode();
    }
}

void Controller::setExpValue(uint8_t value) {
    constexpr uint8_t kNumDigits = 3;
    constexpr uint8_t kStart = sizeof(EXP_VALUE_PREFIX) - 1;

    if (value == Expression::kDisconnected)
    {
        for (uint8_t i = 0; i < kNumDigits; ++i)
        {
            _expValue[kStart + i] = '-';
        }
    }
    else
    {
        uint8_t remainder = value;
        for (uint8_t i = 0; i < kNumDigits; ++i)
        {
            uint8_t digit = remainder % 10;
            remainder /= 10;
            _expValue[kStart + (kNumDigits - 1 - i)] = '0' + digit;
        }
    }

    // MIDI channel
    constexpr uint8_t kChannelStart = sizeof(CHANNEL_PREFIX) - 1;
    auto channel = _pendingChannel + 1;
    _expValue[kChannelStart] = (channel > 9) ? '1' : ' ';
    _expValue[kChannelStart + 1] = '0' + (channel % 10);

    _display.setText(_expValue);
    _display.refresh();
}

void Controller::setupMode()
{
    _pendingChannel = _config.midi().channel();
    loadProgram(_program_id, false, false);
    _display.setBlink(false);
    setExpValue(_exp.getValue());
    _display.clearSwitches();
    _display.setFootswitch(kIncChannelSwitch, "CHAN+");
    _display.setFootswitch(kDecChannelSwitch, "CHAN-");
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
    _display.clearSwitches();
    _display.setFootswitch(kIncOneSwitch, " +1");
    _display.setFootswitch(kIncTenSwitch, " +10");
    _display.setFootswitch(kSetupSwitch, "SETUP");
    _display.setFootswitch(kTunerSwitch, "TUNER");
    _display.setFootswitch(kDecOneSwitch, " -1");
    _display.setFootswitch(kDecTenSwitch, " -10");
    _display.setFootswitch(kLoadSwitch, "LOAD");
    _display.setFootswitch(kExitProgramSwitch, "EXIT");
    _switches_state.reset();
    _buttons.setCallback(std::bind(&Controller::programCallback, this, _1, _2));
}

void Controller::tunerMode() {
    _tuner_mode = true;
    // Only capture the live state when there isn't already a pending restore.
    // When tuner is entered from the change-program menu, _restore_state is
    // already set and _saved_* hold the real live program/switch state (captured
    // on the "..." press); entering the menu rebuilt _switches_state from program
    // defaults, so recapturing here would clobber the live state with defaults.
    if (!_restore_state) {
        _saved_program_id = _program_id;
        _saved_switches_state = _switches_state;
        _restore_state = true;
    }

    for (uint8_t idx = 0; idx < Program::kNumSwitches; ++idx) {
        _display.setFootswitch(idx, nullptr);
    }
    displayTuner(0, 0);
    _switches_state.reset();
    _buttons.setCallback([this](auto status, auto modified) {
        auto activated = status & modified;
        if (!activated.any()) {
            return;
        }
        exitTunerMode(true);
    });

    auto channel = _config.midi().channel();
    _network.midi().sendControl(channel, 47, 127);;
    _usb.midi().sendControl(channel, 47, 127);
}

void Controller::exitTunerMode(bool send_midi)
{
    _tuner_mode = false;
    if (send_midi) {
        auto channel = _config.midi().channel();
        _network.midi().sendControl(channel, 47, 0);
        _usb.midi().sendControl(channel, 47, 0);
    }
    footswitchMode(false);
}

void Controller::footswitchMode(bool send_midi)
{
    _display.setTuner(false);
    _display.setBlink(false);
    const std::bitset<Program::kNumSwitches>* restore = nullptr;
    if (_restore_state) {
        _program_id = _saved_program_id;
        restore = &_saved_switches_state;
    }
    _restore_state = false;
    // When restoring the saved state the program is already live; re-running its
    // MIDI actions would re-trigger them and create an inconsistency.
    loadProgram(_program_id, send_midi && restore == nullptr, true, restore);
    _buttons.setCallback(std::bind(&Controller::footswitchCallback, this, _1, _2));
    _exp.setCallback(std::bind(&Controller::sendExpression, this, _1));
}

void Controller::displayTuner(uint8_t note, int64_t cents)
{
    if (!_tuner_mode) {
        return;
    }
    _display.setTuner(true, note, cents);

    // Full-scale tuning offset.
    static constexpr int kMinCents = -64;
    static constexpr int kMaxCents = 63;

    const uint8_t columns = _leds.kNumLeds / 2;  // 4 (long) or 3 (short)
    struct RGB { uint8_t r, g, b; };
    RGB col[4] = {};  // per-column color; up to 4 columns

    // Wider dead zone: the middle column(s) show solid green here.
    const bool in_tune = (cents >= -4 && cents <= 3);

    if (note < 24) {
        // No valid note: leave everything off.
    } else if (in_tune) {
        if (columns >= 4) {
            col[1] = col[2] = {0, kLedMax, 0};
        } else {
            col[1] = {0, kLedMax, 0};
        }
    } else {
        // Continuous moving bar: map cents to a position across the column
        // index range and split intensity between the two nearest columns
        // with a triangular falloff. Integer math, scaled by D = range size.
        const int c = std::min(std::max(static_cast<int>(cents), kMinCents), kMaxCents);
        const int D = kMaxCents - kMinCents;  // 127
        const int N = columns - 1;
        for (uint8_t i = 0; i < columns; ++i) {
            const int diff = (c - kMinCents) * N - static_cast<int>(i) * D;
            const int adiff = diff < 0 ? -diff : diff;
            const int level = kLedMax - (kLedMax * adiff + D / 2) / D;
            if (level > 0) {
                col[i] = {_gamma_table[level], 0, 0};
            }
        }
    }

    for (uint8_t i = 0; i < _leds.kNumLeds; ++i)
    {
        const RGB& c = col[i % columns];
        _leds.setColor(i, c.r, c.g, c.b);
    }
    _leds.refresh();
}

void Controller::changeSwitch(uint8_t id, bool active, bool send_midi)
{
    if (id >= _program.numFootswitches())
    {
        return;
    }

    const auto& fs = _program.footswitch(id);
    auto sw_mode = _program.switchMode(id);
    bool is_scene = (sw_mode == Program::Footswitch::kScene);
    bool momentary = (sw_mode == Program::Footswitch::kMomentary);
    if (!fs.available() || (!momentary && !active))
    {
        return;
    }

    if (is_scene)
    {
        // Mutual exclusion only among scene switches: turn off the previously
        // active scene switch (if any), leaving stomp switches untouched.
        if (_fs_id != id && _fs_id < _program.numFootswitches() &&
            _program.switchMode(_fs_id) == Program::Footswitch::kScene &&
            _switches_state[_fs_id])
        {
            const auto& prev = _program.footswitch(_fs_id);
            _switches_state[_fs_id] = false;
            _leds.setColor(_fs_id, prev.color(), false);
            if (send_midi) {
                prev.run(_network.midi(), false);
                prev.run(_usb.midi(), false);
            }
        }
        _switches_state[id] = true;
        _fs_id = id;
    } else {
        _switches_state[id] = momentary ? (active ^ fs.enabled()) : !_switches_state[id];
    }

    if (send_midi) {
        fs.run(_network.midi(), _switches_state[id]);
        fs.run(_usb.midi(), _switches_state[id]);
    }
    _leds.setColor(id, fs.color(), _switches_state[id]);
}

void Controller::sendExpression(uint8_t value)
{
    if (value == Expression::kDisconnected) { return; } // nothing to send
    if (_expEnabled && _program.available() && _program.expressionEnabled())
    {
        _program.sendExpression(_network.midi(), value);
        _program.sendExpression(_usb.midi(), value);
    }
}

void Controller::configChanged()
{
    _network.reinitMidi(_config.midi().channel());
}

void Controller::programChanged(uint8_t id)
{
    if (id == _program_id)
    {
        loadProgram(id, false, true);
    }
}

void Controller::defaultSwitchesState(const Program& program, std::bitset<Program::kNumSwitches>& state) const
{
    state.reset();
    uint8_t scene = program.defaultScene();
    for (uint8_t id = 0; id < program.numFootswitches(); ++id)
    {
        const auto& fs = program.footswitch(id);
        if (!fs.available())
        {
            continue;
        }
        bool is_scene = (program.switchMode(id) == Program::Footswitch::kScene);
        state[id] = is_scene ? (id == scene) : fs.enabled();
    }
}

void Controller::applySceneToState(const Program& program, std::bitset<Program::kNumSwitches>& state, uint8_t scene_id) const
{
    // Select one scene within the scene group (mutual exclusion among scene
    // switches), leaving stomp switches untouched.
    for (uint8_t id = 0; id < program.numFootswitches(); ++id)
    {
        if (program.switchMode(id) == Program::Footswitch::kScene)
        {
            state[id] = (id == scene_id);
        }
    }
}

void Controller::loadProgram(uint8_t id, bool send_midi, bool display_switches,
                             const std::bitset<Program::kNumSwitches>* restore_state)
{
    printf("loadProgram %u\n", id);
    // Turn off the currently active scene switch (if any) before switching away.
    if (send_midi && _fs_id < _program.numFootswitches() &&
        _program.switchMode(_fs_id) == Program::Footswitch::kScene &&
        _switches_state[_fs_id])
    {
        _program.footswitch(_fs_id).run(_usb.midi(), false);
        _program.footswitch(_fs_id).run(_network.midi(), false);
    }
    _program_id = id;
    _fs_id = 0;
    _program.load(id);

    displayProgram(display_switches);

    if (send_midi && _program.available())
    {
        _program.run(_usb.midi());
        _program.run(_network.midi());
        sendExpression(_exp.getValue());
    }

    if (display_switches)
    {
        // Restore the saved snapshot (scene + stomp toggles) or fall back to the
        // program's default state.
        if (restore_state)
        {
            _switches_state = *restore_state;
        }
        else
        {
            defaultSwitchesState(_program, _switches_state);
        }
    }

    // Track the active scene = first scene switch currently on, else the default.
    _fs_id = _program.defaultScene();
    if (display_switches)
    {
        for (uint8_t sid = 0; sid < _program.numFootswitches(); ++sid)
        {
            if (_program.switchMode(sid) == Program::Footswitch::kScene && _switches_state[sid])
            {
                _fs_id = sid;
                break;
            }
        }
    }

    for (uint8_t lid = 0; lid < _leds.kNumLeds; ++lid)
    {
        if (!display_switches)
        {
            _leds.setColor(lid, kWhite, false);
            continue;
        }

        const auto& fs = _program.footswitch(lid);
        if (lid >= _program.numFootswitches() || !fs.available())
        {
            _leds.setColor(lid, kNone, false);
        }
        else
        {
            _leds.setColor(lid, fs.color(), _switches_state[lid]);
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
    _display.setFootswitch(kProgramSwitch, "...");
}

}
