#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

namespace tocata {

class Config
{
public:
    class Wifi
    {
    public:
        bool available() const { return !_doc.isNull(); }
        const char* ssid() const { return _doc["ssid"]; }
        const char* key() const { return _doc["key"]; }

    friend Config;
    protected:
        Wifi(JsonObject doc) : _doc(doc) {}

    private:
        JsonObject _doc;
    };

    bool load();
    void save() const;
    Wifi wifi() { return {_doc["wifi"].as<JsonObject>()}; }

private:
    static constexpr const char* kConfigFile = "/config.json";
    DynamicJsonDocument _doc{1024};
};

}