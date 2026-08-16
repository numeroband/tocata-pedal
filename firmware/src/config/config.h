#pragma once

#include <cstring>
#include <cstdint>

namespace tocata {

// Builds the "/XX" path of a file from its id. The filesystem packs the id into
// 7 bits of a single flash byte, so only ids 0x00..0x7D are addressable: /00 is
// the config, /01../63 the 99 programs and /64../7D the 26 setlists.
inline void copyFilePath(uint8_t file_id, char* path)
{
    static const char hex[] = "0123456789ABCDEF";
    path[0] = '/';
    path[1] = hex[file_id >> 4];
    path[2] = hex[file_id & 0x0F];
    path[3] = '\0';
}

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

// Bit 3 of the low nibble of the bytes that pack a 4-bit channel next to a
// small enum (Actions::Action and Program below). All 16 channel values are
// legal MIDI channels, so the "follow the device-wide channel" marker has no
// room in the channel nibble itself; the enums it shares a byte with use at
// most values 0..4, leaving this bit free. Every byte written by earlier
// firmware has it clear, so old data keeps its explicit channel.
static constexpr uint8_t kGlobalChannelMask = 0x08;

class Actions
{
public:
    static constexpr size_t kMaxActions = 5;

    void run(MidiSender& midi, uint8_t global_channel) const;
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

        void run(MidiSender& midi, uint8_t global_channel) const;
        bool operator==(const Action& other);

    private:
        Type type() const { return Type(_channel_and_type & 0x07); }
        bool globalChannel() const { return _channel_and_type & kGlobalChannelMask; }
        // `global_channel` comes from Config::MidiConfig, a full unclamped byte,
        // so mask it here rather than trusting every send backend to do it.
        uint8_t channel(uint8_t global_channel) const
        {
            return (globalChannel() ? global_channel : (_channel_and_type >> 4)) & 0x0F;
        }

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
        void run(MidiSender& midi, bool active, uint8_t global_channel) const;
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

    void run(MidiSender& midi, uint8_t global_channel) const;
    void sendExpression(MidiSender& midi, uint8_t value, uint8_t global_channel) const;
    Footswitch& footswitch(uint8_t id) { return _switches[id]; }
    const Footswitch& footswitch(uint8_t id) const { return _switches[id]; }
    uint8_t numFootswitches() const { return _num_switches; }
    const char* name() const { return _name; }
    Mode mode() const { return Mode(_channel_and_mode & 0x07); }
    // Effective mode of a single switch: kScene programs force every switch to
    // scene; kDefault programs defer to the switch's own stored mode.
    Footswitch::Mode switchMode(uint8_t id) const
    {
        if (id >= _num_switches) { return Footswitch::kStomp; }
        return (mode() == kScene) ? Footswitch::kScene : footswitch(id).mode();
    }
    uint8_t defaultScene() const;
    uint8_t expression() const { return _expression; }
    bool expressionGlobalChannel() const { return _channel_and_mode & kGlobalChannelMask; }
    uint8_t expressionChannel(uint8_t global_channel) const
    {
        return (expressionGlobalChannel() ? global_channel : (_channel_and_mode >> 4)) & 0x0F;
    }
    bool expressionEnabled() const { return _expression < 128; }
    bool available() const { return _name[0]; }
    void save(uint8_t id) const;
    bool operator==(const Program& other);

protected:
    friend class Storage;
    static void initAll();

private:
    static void remove(uint8_t id, bool check);
    // Offset by one: file id 0 ("/00") belongs to Config.
    static void copyPath(uint8_t id, char* path) { copyFilePath(id + 1, path); }
    void invalidate() { _name[0] = 0; }

    char _name[kMaxNameLength + 1] = "";
    uint8_t _num_switches;
    Footswitch _switches[kNumSwitches];
    Actions _actions;
    Mode _channel_and_mode; // Expression channel in first most significant 4 bits
    uint8_t _expression;
} __attribute__((packed));

// An ordered subset of the programs, used to drive program-change navigation
// during a performance. Setlist 0 is not stored: it is the synthetic "All"
// setlist, a 1:1 mapping over every program, which is what the pedal uses when
// no setlist has been selected.
class Setlist
{
public:
    // Files /64../7D -- everything the file-id encoding leaves after the config
    // and the 99 programs (see copyFilePath).
    static constexpr uint8_t kMaxSetlists = 26;

    // Copies the name of setlist `id` and returns its program count. Reads only
    // the 32-byte header, so it is cheap enough to probe every slot. A return of
    // 0 means missing, unnamed or empty -- i.e. not selectable.
    static uint8_t copyName(uint8_t id, char* name);
    static void remove(uint8_t id);
    static void removeAll();

    Setlist() { loadAll(); }

    // Loads setlist `id`, falling back to "All" when the file is missing,
    // unnamed or empty. Returns whether the stored setlist was usable.
    bool load(uint8_t id);
    void loadAll();

    bool available() const { return _name[0]; }
    const char* name() const { return _name; }
    uint8_t numPrograms() const { return _num_programs; }
    uint8_t program(uint8_t pos) const
    {
        if (pos >= _num_programs) { return 0; }
        const uint8_t id = _programs[pos];
        return (id < Program::kMaxPrograms) ? id : 0;
    }
    // Position of `program_id` within the setlist, or -1 when it isn't in it.
    int16_t find(uint8_t program_id) const;
    void save(uint8_t id) const;
    bool operator==(const Setlist& other) const;

private:
    static constexpr uint8_t kFirstFileId = 0x64;
    static constexpr size_t kHeaderSize = Program::kMaxNameLength + 2;

    static void copyPath(uint8_t id, char* path) { copyFilePath(kFirstFileId + id, path); }
    void invalidate() { _name[0] = 0; }

    char _name[Program::kMaxNameLength + 1] = "";
    uint8_t _num_programs = 0;
    uint8_t _programs[Program::kMaxPrograms] = {};
} __attribute__((packed));

}