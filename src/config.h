#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

#include <string>

namespace tocata {

class Config
{
public:
    class SubConfig
    {
    public:
        bool available() const { return !_doc.isNull(); }

    protected:
        SubConfig(JsonObject doc) : _doc(doc) {}
        JsonObject _doc;
    };

    class Wifi : public SubConfig
    {
    public:
        Wifi(JsonObject doc) : SubConfig(doc) {}
        const char* ssid() const { return _doc["ssid"]; }
        const char* key() const { return _doc["key"]; }
    };

    class Footswitch : public SubConfig
    {
    public:
        Footswitch(uint8_t number, JsonObject doc) : SubConfig(doc), _number(number) {}
        const char* name() const { return _doc["name"]; }
        bool enabled() const { return _doc["enabled"]; }
        void enable(bool value) { if (available()) _doc["enabled"] = value; }
        uint8_t number() const { return _number; }
        void initState() { _doc["enabled"] = _doc["initial"].as<bool>(); }

    private:
        uint8_t _number;
    };

    class Program : public SubConfig
    {
    public:
        Program(uint8_t number, JsonObject doc);
        Footswitch footswitch(uint8_t number) { return {number, _doc["fs"][number].as<JsonObject>()}; }
        uint8_t numFootswitches() const { return _doc["fs"].size(); }
        const char* name() const { return _doc["name"]; }
        uint8_t number() const { return _number; }

    private:
        uint8_t _number;
    };

    bool load();
    void save() const;
    Wifi wifi() { return {_doc["wifi"].as<JsonObject>()}; }
    Program program(uint8_t number) { return {number, _doc["programs"][String(number)].as<JsonObject>()}; }

private:
    static constexpr const char* kConfigFile = "/config.json";
    DynamicJsonDocument _doc{1024};
};

}