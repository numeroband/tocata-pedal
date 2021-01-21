#pragma once

#include "fsmidi.h"

#include <Arduino.h>
#include <ArduinoJson.h>

#include <string>

namespace tocata {

class Storage
{
public:
    static void begin();
};

class Config
{
public:
    static constexpr size_t kMaxJsonSize = 256;

    class Wifi
    {
    public:
        bool parse(const JsonObjectConst& obj);
        const char* ssid() const { return _ssid; }
        const char* key() const { return _key; }
        bool available() const { return _available; }
        void serialize(JsonObject& obj) const;
        bool operator==(const Wifi& other);

    private:
        char _ssid[64];
        char _key[64];
        bool _available = false;
    };

    static void remove() { remove(true); };

    Config() { load(); };
    Config(const JsonObjectConst& obj) { parse(obj); }

    bool load();
    bool parse(const JsonObjectConst& obj);
    bool available() const { return _available; }
    const Wifi& wifi() const { return _wifi; }
    void serialize(JsonObject& obj) const;
    void save() const;
    bool operator==(const Config& other);

protected:
    friend class Storage;
    static bool init();

private:
    static constexpr const char* kPath = "/00";

    static void remove(bool check);

    Wifi _wifi;
    bool _available;
};

class Actions
{
public:
    static constexpr size_t kMaxActions = 5;

    void run(FsMidi& midi) const;
    uint8_t parse(const JsonArrayConst& array);
    void serialize(JsonArray& array) const;
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

        void run(FsMidi& midi) const;
        bool parse(const JsonObjectConst& obj);
        void serialize(JsonObject& obj) const;
        bool operator==(const Action& other);

    private:
        Type _type = kNone;
        uint8_t _values[2];
    };

private:
    Action _actions[kMaxActions];
    uint8_t _num_actions;
};

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

        void parse(uint8_t id, const JsonObjectConst& doc);
        const char* name() const { return _name; }
        bool momentary() const { return _momentary; }
        bool enabled() const { return _enabled; }
        void enable(bool value) { _enabled = value; }
        void toggle() { _enabled = !_enabled; }
        uint8_t id() const { return _id; }
        bool available() const { return _available; }
        void serialize(JsonObject& obj) const;
        void run(FsMidi& midi) const;
        void reset() { _enabled = _default; }
        bool operator==(const Footswitch& other);

    private:
        Actions _on_actions;
        Actions _off_actions;
        char _name[kMaxNameSize + 1];
        Color _color;
        bool _available = false;
        bool _enabled;
        bool _default;
        bool _momentary;
        uint8_t _id;
    };

    static uint8_t copyName(uint8_t id, char* name);
    static void remove(uint8_t id) { remove(id, true); };

    Program() : _available(false), _id(kInvalidId) {}
    Program(uint8_t id) { load(id); }
    Program(uint8_t id, const JsonObjectConst& obj) { parse(id, obj); };

    bool load(uint8_t id);
    bool parse(uint8_t id, const JsonObjectConst& obj);

    void run(FsMidi& midi) const;
    Footswitch& footswitch(uint8_t id) { return _switches[id]; }
    uint8_t numFootswitches() const { return _num_switches; }
    const char* name() const { return _name; }
    uint8_t id() const { return _id; }
    bool available() const { return _available; }
    void serialize(JsonObject& obj) const;
    void save() const;
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

    char _name[kMaxNameLength + 1];
    Footswitch _switches[kNumSwitches];
    Actions _actions;
    bool _available;
    uint8_t _id;
    uint8_t _num_switches;
};

}