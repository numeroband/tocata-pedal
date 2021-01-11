#include "config.h"

#include <SPIFFS.h>

namespace tocata {

void Config::begin()
{
    if (!SPIFFS.exists(namesPath()))
    {
        restore();
    }

}

void Config::restore()
{
    Serial.println("Initializing file system");
    SPIFFS.format();    
    uint8_t emptyName[Program::kMaxNameLength];
    memset(emptyName, ' ', sizeof(emptyName));
    File names = SPIFFS.open(namesPath(), FILE_WRITE);
    for (uint8_t i = 0; i < kMaxPrograms; ++i)
    {
        names.write(emptyName, sizeof(emptyName));
    }    
}

Config::Wifi::Wifi()
{
    if (!SPIFFS.exists(configPath()))
    {
        return;
    }
    File file = SPIFFS.open(configPath());
    DeserializationError error = deserializeJson(_doc, file);
    file.close();
    if (error != DeserializationError::Ok)
    {
        Serial.print("Config deserialization failed with code ");
        Serial.println(error.code());   
        return;
    }
}

Config::Program::Program(uint8_t number) : _number(number)
{
    if (number > kMaxPrograms)
    {
        return;
    }

    char path[16];
    copyPath(number, path, sizeof(path));

    if (!SPIFFS.exists(path))
    {
        return;
    }
    File file = SPIFFS.open(path);
    DynamicJsonDocument doc{512};
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    if (error != DeserializationError::Ok)
    {
        Serial.print("Program deserialization failed with code ");
        Serial.println(error.code());   
        return;
    }

    _available = !doc.isNull();
    if (!_available)
    {
        return;
    }

    strncpy(_name, doc["name"], sizeof(_name));
    _name[kMaxNameLength] = '\0';
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
    _enabled = doc["enabled"].as<bool>();
}

}