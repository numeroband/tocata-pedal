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

Config::Program::Program(uint8_t number, JsonObject doc) : SubConfig(doc), _number(number)
{
    uint8_t num_switches = numFootswitches();
    for (uint8_t i = 0; i < num_switches; ++i)
    {
        footswitch(i).initState();
    }
}

}