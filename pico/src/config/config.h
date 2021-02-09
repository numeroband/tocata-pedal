#pragma once

#include <cstring>
#include <cstdint>

namespace tocata {

class MidiUsb;

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

    void run(MidiUsb& midi) const;
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

        void run(MidiUsb& midi) const;
        bool operator==(const Action& other);

    private:
        Type _type = kNone;
        uint8_t _values[2];
    } __attribute__((packed));

private:
    uint8_t _num_actions;
    Action _actions[kMaxActions];
} __attribute__((packed));

class Program
{
public:
    static constexpr size_t kNumSwitches = 6;
    static constexpr uint8_t kMaxNameLength = 30;
    static constexpr uint8_t kMaxPrograms = 99;
    static constexpr size_t kMaxJsonSize = 1024;
    static constexpr size_t kMaxPathSize = 4;
    static constexpr uint8_t kInvalidId = 255;

    class Footswitch
    {
    public:
        enum Color : uint8_t
        {
            kBlue, 
            kPurple, 
            kRed, 
            kYellow, 
            kGreen, 
            kTurquoise,
        };

        static constexpr size_t kMaxNameSize = 5;

        const char* name() const { return _name; }
        bool momentary() const { return _momentary; }
        bool enabled() const { return _enabled; }
        bool available() const { return _name[0]; }
        void run(MidiUsb& midi) const;
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

    void run(MidiUsb& midi) const;
    Footswitch& footswitch(uint8_t id) { return _switches[id]; }
    const Footswitch& footswitch(uint8_t id) const { return _switches[id]; }
    uint8_t numFootswitches() const { return _num_switches; }
    const char* name() const { return _name; }
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
} __attribute__((packed));

}