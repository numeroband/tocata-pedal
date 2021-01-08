#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

#include <string>

namespace tocata {

class Config
{
public:
    static constexpr size_t kNumSwitches = 6;

    class Wifi
    {
    public:
        Wifi(const JsonObject& doc) : _available(!doc.isNull()), _ssid(doc["ssid"]), _key(doc["key"]) {}
        const char* ssid() const { return _ssid; }
        const char* key() const { return _key; }
        bool available() const { return _available; }

    private:
        bool _available;
        const char* _ssid;
        const char* _key;
    };

    class Footswitch
    {
    public:
        static constexpr size_t kMaxNameSize = 5;

        void init(uint8_t number, const JsonObject& doc);
        const char* name() const { return _name; }
        bool enabled() const { return _enabled; }
        void enable(bool value) { _enabled = value; }
        void toggle() { _enabled = !_enabled; }
        uint8_t number() const { return _number; }
        bool available() const { return _available; }

    private:
        JsonArray _actions;
        char _name[kMaxNameSize + 1];
        bool _available = false;
        bool _enabled;
        uint8_t _number;
    };

    class Program
    {
    public:
        Program() {}
        Program(uint8_t number, const JsonObject& doc) { init(number, doc); }
        void init(uint8_t number, const JsonObject& doc);
        Footswitch& footswitch(uint8_t number) { return _switches[number]; }
        uint8_t numFootswitches() const { return _num_switches; }
        const char* name() const { return _name; }
        uint8_t number() const { return _number; }
        bool available() const { return _available; }

    private:
        const char* _name;
        std::array<Footswitch, kNumSwitches> _switches;
        bool _available = false;
        uint8_t _number = 0;
        uint8_t _num_switches;
    };

    bool load();
    void save() const;
    Wifi wifi() { return {_doc["wifi"].as<JsonObject>()}; }
    Program program(uint8_t number) { return {number, _doc["programs"][number].as<JsonObject>()}; };

private:
    static constexpr const char* kConfigFile = "/config.json";
    StaticJsonDocument<30 * 1024> _doc;
};

}