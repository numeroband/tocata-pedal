#include "config.h"

#include <SPIFFS.h>

namespace tocata {
bool Config::load()
{
    File file = SPIFFS.open(kConfigFile);
    if (!file.available())
    {
        Serial.println("config file not available");
        return false;
    }

    DeserializationError error = deserializeJson(_doc, file);
    file.close();
    if (error != DeserializationError::Ok)
    {
        Serial.print("Deserialization failed with code ");
        Serial.println(error.code());
        _doc.clear();
        return false;
    }

    return true;
}

void Config::save() const
{
    File file = SPIFFS.open(kConfigFile, FILE_WRITE);
    serializeJson(_doc, file);
    file.close();
}

void Config::Program::init(uint8_t number, const JsonObject& doc)
{
    _number = number;
    _available = !doc.isNull();
    if (!_available)
    {
        return;
    }

    _name = doc["name"];
    _num_switches = doc["fs"].size();

    for (uint8_t i = 0; i < _num_switches; ++i)
    {
        _switches[i].init(i, doc["fs"][i].as<JsonObject>());
    }
}

void Config::Footswitch::init(uint8_t number, const JsonObject& doc) 
{
    _available = !doc.isNull();
    if (!_available)
    {
        return;
    }

    _number = number;
    strncpy(_name, doc["name"], sizeof(_name));
    _name[kMaxNameSize] = '\0';
    _enabled = doc["on"].as<bool>();
    _actions = doc["actions"].as<JsonArray>();
}

}