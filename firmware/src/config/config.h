#pragma once

#include <cstring>
#include <cstdint>

namespace tocata {

class MidiSender;

class Storage
{
public:
    static void init();
    static void factoryReset();
};

class Config
{
public:
    class MidiConfig
    {
    public:
        uint8_t channel() const { return _channel; }
        void setChannel(uint8_t channel) { _channel = channel; }
        bool available() const { return true; }
        bool operator==(const MidiConfig& other);

    private:
        uint8_t _channel = 0;
    } __attribute__((packed));

    class ExpressionConfig
    {
    public:
        static constexpr uint16_t kDefaultMinRaw = 0;
        static constexpr uint16_t kDefaultMaxRaw = 4095; // 12-bit ADC

        uint16_t minRaw() const { return _minRaw; }
        uint16_t maxRaw() const { return _maxRaw; }
        void setMinRaw(uint16_t v) { _minRaw = v; }
        void setMaxRaw(uint16_t v) { _maxRaw = v; }
        bool operator==(const ExpressionConfig& other);

    private:
        uint16_t _minRaw = kDefaultMinRaw;
        uint16_t _maxRaw = kDefaultMaxRaw;
    } __attribute__((packed));

    static void remove() { remove(true); };

    bool load();
    bool available() const { return _midi.available(); }
    const MidiConfig& midi() const { return _midi; }
    MidiConfig& midi() { return _midi; }
    const ExpressionConfig& expression() const { return _expression; }
    ExpressionConfig& expression() { return _expression; }
    void save() const;
    bool operator==(const Config& other);

protected:
    friend class Storage;
    static bool init();

private:
    static constexpr const char* kPath = "/00";
    static constexpr uint8_t kVersion = 1;

    static void remove(bool check);
    void migrate(size_t bytes_read);

    uint8_t _version = kVersion;
    MidiConfig _midi;
    ExpressionConfig _expression;
} __attribute__((packed));

class Actions
{
public:
    static constexpr size_t kMaxActions = 5;

    void run(MidiSender& midi) const;
    bool operator==(const Actions& other);

    class Action
    {
    public:
        enum Type : uint8_t
        {
            kNone,
            kProgramChange,
            kControlChange,
            kNoteOn,
            kNoteOff,
        };

        void run(MidiSender& midi) const;
        bool operator==(const Action& other);

    private:
        Type type() const { return Type(_channel_and_type & 0x0F); }
        uint8_t channel() const { return _channel_and_type >> 4; }

        uint8_t _channel_and_type = kNone;
        uint8_t _values[2];
    } __attribute__((packed));

private:
    uint8_t _num_actions;
    Action _actions[kMaxActions];
} __attribute__((packed));

enum Color : uint8_t
{
    kNone,
    kBlue, 
    kPurple, 
    kRed, 
    kYellow, 
    kGreen, 
    kTurquoise,
    kWhite,
};

class Program
{
public:
    static constexpr size_t kNumSwitches = 8;
    static constexpr uint8_t kMaxNameLength = 30;
    static constexpr uint8_t kMaxPrograms = 99;
    static constexpr size_t kMaxPathSize = 4;
    static constexpr uint8_t kInvalidId = 255;

    enum Mode : uint8_t
    {
        kDefault = 0,  // each switch picks its own mode (see Footswitch::Mode)
        kScene = 1,    // forces every switch to behave as Footswitch::kScene
    };

    class Footswitch
    {
    public:
        static constexpr size_t kMaxNameSize = 8;

        // Per-switch mode. Stored in the byte formerly named `_momentary`, so the
        // legacy bool maps perfectly: false -> kStomp, true -> kMomentary.
        enum Mode : uint8_t
        {
            kStomp = 0,      // independent on/off toggle
            kMomentary = 1,  // on while pressed, off when released
            kScene = 2,      // mutually exclusive among scene switches
            kProgram = 3,    // pure trigger: enters program-change mode, no actions
        };

        const char* name() const { return _name; }
        Mode mode() const { return _mode; }
        bool momentary() const { return _mode == kMomentary; }
        bool enabled() const { return _enabled; }
        Color color() const { return _color; }
        bool available() const { return _name[0]; }
        void run(MidiSender& midi, bool active) const;
        bool operator==(const Footswitch& other);

    private:
        Actions _on_actions;
        Actions _off_actions;
        char _name[kMaxNameSize + 1] = "";
        Color _color;
        bool _enabled;
        Mode _mode;
    } __attribute__((packed));

    static uint8_t copyName(uint8_t id, char* name);
    static void remove(uint8_t id) { remove(id, true); };

    Program() {}
    Program(uint8_t id) { load(id); }

    bool load(uint8_t id);

    void run(MidiSender& midi) const;
    void sendExpression(MidiSender& midi, uint8_t value) const;
    Footswitch& footswitch(uint8_t id) { return _switches[id]; }
    const Footswitch& footswitch(uint8_t id) const { return _switches[id]; }
    uint8_t numFootswitches() const { return _num_switches; }
    const char* name() const { return _name; }
    Mode mode() const { return Mode(_channel_and_mode & 0x0F); }
    // Effective mode of a single switch: kScene programs force every switch to
    // scene; kDefault programs defer to the switch's own stored mode.
    Footswitch::Mode switchMode(uint8_t id) const
    {
        if (id >= _num_switches) { return Footswitch::kStomp; }
        return (mode() == kScene) ? Footswitch::kScene : footswitch(id).mode();
    }
    uint8_t defaultScene() const;
    uint8_t expression() const { return _expression; }
    uint8_t expressionChannel() const { return _channel_and_mode >> 4; }
    bool expressionEnabled() const { return _expression < 128; }
    bool available() const { return _name[0]; }
    void save(uint8_t id) const;
    bool operator==(const Program& other);

protected:
    friend class Storage;
    static void initAll();

private:
    static void remove(uint8_t id, bool check);
    static void copyPath(uint8_t id, char* path) 
    { 
        path[0] = '/';
        path[1] = '0' + ((id + 1) / 10);
        path[2] = '0' + ((id + 1) % 10);
        path[3] = '\0';
    }
    void invalidate() { _name[0] = 0; }

    char _name[kMaxNameLength + 1] = "";
    uint8_t _num_switches;
    Footswitch _switches[kNumSwitches];
    Actions _actions;
    Mode _channel_and_mode; // Expression channel in first most significant 4 bits
    uint8_t _expression;
} __attribute__((packed));

}