#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

#include <string>

namespace tocata {

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

    private:
        char _ssid[64];
        char _key[64];
        bool _available = false;
    };

    static void remove();

    Config() { load(); };
    Config(const JsonObjectConst& obj) { parse(obj); }

    bool load();
    bool parse(const JsonObjectConst& obj);
    bool available() const { return _available; }
    const Wifi& wifi() const { return _wifi; }
    void serialize(JsonObject& obj) const;
    void save() const;

private:
    static constexpr const char* kPath = "/config.json";

    Wifi _wifi;
    bool _available;
};

class Program
{
public:
    static constexpr size_t kNumSwitches = 6;
    static constexpr uint8_t kMaxNameLength = 30;
    static constexpr uint8_t kMaxPrograms = 99;
    static constexpr size_t kMaxJsonSize = 512;
    static constexpr size_t kMaxPathSize = 16;
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
        bool enabled() const { return _enabled; }
        void enable(bool value) { _enabled = value; }
        void toggle() { _enabled = !_enabled; }
        uint8_t id() const { return _id; }
        bool available() const { return _available; }
        void serialize(JsonObject& obj) const;

    private:
        char _name[kMaxNameSize + 1];
        Color _color;
        bool _available = false;
        bool _enabled;
        bool _momentary;
        uint8_t _id;
    };

    static void remove(uint8_t id);

    Program() : _available(false), _id(kInvalidId) {}
    Program(uint8_t id) { load(id); }
    Program(uint8_t id, const JsonObjectConst& obj) { parse(id, obj); };

    bool load(uint8_t id);
    bool parse(uint8_t id, const JsonObjectConst& obj);

    Footswitch& footswitch(uint8_t id) { return _switches[id]; }
    uint8_t numFootswitches() const { return _num_switches; }
    const char* name() const { return _name; }
    uint8_t id() const { return _id; }
    bool available() const { return _available; }
    void serialize(JsonObject& obj) const;
    void save() const;

private:
    static void copyPath(uint8_t id, char* path) { 
        snprintf_P(path, kMaxPathSize, PSTR("/prg.%u.json"), id);
    }

    std::array<Footswitch, kNumSwitches> _switches;
    char _name[kMaxNameLength + 1];
    bool _available;
    uint8_t _id;
    uint8_t _num_switches = 0;
};

}