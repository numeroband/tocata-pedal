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
    static constexpr uint8_t kMaxPrograms = 99;

    class Wifi
    {
    public:
        Wifi();
        const char* ssid() const { return _doc["wifi"]["ssid"]; }
        const char* key() const { return _doc["wifi"]["key"]; }
        bool available() const { return ssid() && key(); }

    private:
        DynamicJsonDocument _doc{128};
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
        char _name[kMaxNameSize + 1];
        bool _available = false;
        bool _enabled;
        uint8_t _number;
    };

    class Program
    {
    public:
        static constexpr uint8_t kMaxNameLength = 30;

        static bool copyPath(uint8_t id, char* path, size_t size) { 
            return snprintf_P(path, size, PSTR("/prg.%u.json"), id) == size;
        }

        Program(uint8_t number);
        Footswitch& footswitch(uint8_t number) { return _switches[number]; }
        uint8_t numFootswitches() const { return _num_switches; }
        const char* name() const { return _name; }
        uint8_t number() const { return _number; }
        bool available() const { return _available; }

    private:
        std::array<Footswitch, kNumSwitches> _switches;
        char _name[kMaxNameLength + 1];
        bool _available = false;
        uint8_t _number = 0;
        uint8_t _num_switches = 0;
    };

    void begin();
    void restore();
    static const char* namesPath() { return "/names.txt"; };
    static const char* configPath() { return "/config.json"; };

    Wifi wifi() { return {}; }
    Program program(uint8_t number) { return {number}; };
};

}