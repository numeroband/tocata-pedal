#pragma once

#include <cstring>
#include <cstdint>

namespace tocata {

class MidiSender;

class Storage
{
public:
    static void init();
};

class Config
{
public:
    class Wifi
    {
    public:
        const char* ssid() const { return _ssid; }
        const char* key() const { return _key; }
        bool available() const { return _ssid[0]; }
        bool operator==(const Wifi& other);

    private:
        char _ssid[64] = "";
        char _key[64];
    } __attribute__((packed));

    static void remove() { remove(true); };

    Config() { load(); };

    bool load();
    bool available() const { return _wifi.available(); }
    const Wifi& wifi() const { return _wifi; }
    void save() const;
    bool operator==(const Config& other);

protected:
    friend class Storage;
    static bool init();

private:
    static constexpr const char* kPath = "/00";

    static void remove(bool check);

    Wifi _wifi;
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
        kStomp = 0,
        kScene = 1,
    };

    class Footswitch
    {
    public:
        static constexpr size_t kMaxNameSize = 8;

        const char* name() const { return _name; }
        bool momentary() const { return _momentary; }
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
        bool _momentary;
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