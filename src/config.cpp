#include "config.h"

#include <SPIFFS.h>

namespace tocata {

void Config::remove()
{
    if (SPIFFS.exists(kPath))
    {
        SPIFFS.remove(kPath);
    }
}

bool Config::load()
{
    if (!SPIFFS.exists(kPath))
    {
        return false;
    }

    DynamicJsonDocument doc{kMaxJsonSize};
    File file = SPIFFS.open(kPath, FILE_READ);
    if (!file)
    {
        Serial.println(F("Cannot open config file to read"));
        return false;
    }

    DeserializationError error = deserializeJson(doc, file);
    file.close();
    if (error != DeserializationError::Ok)
    {
        Serial.print(F("Config deserialization failed with code "));
        Serial.println(error.code());   
        return false;
    }

    return parse(doc.as<JsonObjectConst>());
}

bool Config::parse(const JsonObjectConst& obj)
{
    _available = false;

    if (obj.isNull())
    {
        return false;
    }

    _wifi.parse(obj["wifi"]);
    _available = true;
    return true;
}

void Config::serialize(JsonObject& obj) const
{
    if (!_available)
    {
        return;
    }

    if (_wifi.available())
    {
        JsonObject wifi_obj = obj["wifi"].to<JsonObject>();
        _wifi.serialize(wifi_obj);
    }
}

void Config::save() const
{
    if (!_available)
    {
        return;
    }

    DynamicJsonDocument doc{kMaxJsonSize};
    JsonObject obj = doc.to<JsonObject>();
    serialize(obj);
    File file = SPIFFS.open(kPath, FILE_WRITE);
    if (!file)
    {
        Serial.println(F("Cannot open config file to write"));
        return;
    }
    serializeJson(doc, file);
    file.close();
}

bool Config::Wifi::parse(const JsonObjectConst& obj)
{
    _available = false;
    
    if (obj.isNull())
    {
        return false;
    }
    
    const String& ssid = obj["ssid"];
    unsigned int ssid_length = ssid.length();
    if (ssid_length == 0 || ssid_length >= sizeof(_ssid))
    {
        return false;
    }
    ssid.toCharArray(_ssid, sizeof(_ssid));

    const String& key = obj["key"];
    unsigned int key_length = key.length();
    if (key_length == 0 || key_length >= sizeof(_key))
    {
        return false;
    }
    key.toCharArray(_key, sizeof(_key));
    _available = true;
    
    return true;
}

void Config::Wifi::serialize(JsonObject& obj) const
{
    obj["ssid"] = _ssid;
    obj["key"] = _key;
}

void Program::remove(uint8_t id)
{
    if (id >= kMaxPrograms)
    {
        return;
    }

    char path[kMaxPathSize];
    copyPath(id, path);
    if (SPIFFS.exists(path))
    {
        SPIFFS.remove(path);
    }
}

bool Program::load(uint8_t id)
{
    _available = false;
    _id = id;

    if (id > kMaxPrograms)
    {
        return false;
    }

    char path[kMaxPathSize];
    copyPath(id, path);

    if (!SPIFFS.exists(path))
    {
        return false;
    }
    File file = SPIFFS.open(path, FILE_READ);
    if (!file)
    {
        Serial.println(F("Cannot open program file to read"));
        return false;
    }
    DynamicJsonDocument doc{kMaxJsonSize};
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    if (error != DeserializationError::Ok)
    {
        Serial.print(F("Program deserialization failed with code "));
        Serial.println(error.code());   
        return false;
    }

    return parse(id, doc.as<JsonObjectConst>());
}

bool Program::parse(uint8_t id, const JsonObjectConst& obj)
{
    _available = false;
    _id = id;

    if (id > kMaxPrograms)
    {
        return false;
    }

    if (obj.isNull())
    {
        return false;
    }

    if (obj["name"].isNull())
    {
        return false;
    }

    const String& name = obj["name"];
    unsigned int name_length = name.length();
    if (name_length == 0 || name_length > kMaxNameLength)
    {
        return false;
    }
    name.toCharArray(_name, sizeof(_name));

    _num_switches = obj["fs"].size();
    if (_num_switches > kNumSwitches)
    {
        return false;
    }

    for (uint8_t i = 0; i < _num_switches; ++i)
    {
        const JsonArrayConst& switches = obj["fs"];
        _switches[i].parse(i, switches[i].as<JsonObjectConst>());
    }
    
    _available = true;

    return true;
}

void Program::serialize(JsonObject& obj) const
{
    if (!_available)
    {
        return;
    }
    
    obj["name"] = _name;
    JsonArray switches = obj.createNestedArray("fs");
    for (uint8_t i = 0; i < _num_switches; ++i)
    {
        const Footswitch& fs = _switches[i];
        if (fs.available())
        {
            JsonObject fs_doc = switches[i].to<JsonObject>();
            fs.serialize(fs_doc);
        }
    }
}

void Program::save() const
{
    if (!_available)
    {
        return;
    }
    
    DynamicJsonDocument doc{kMaxJsonSize};
    JsonObject obj = doc.to<JsonObject>();
    serialize(obj);

    char path[kMaxPathSize];
    copyPath(_id, path);
    File file = SPIFFS.open(path, FILE_WRITE);
    if (!file)
    {
        Serial.println(F("Cannot open program file to write"));
        return;
    }

    serializeJson(doc, file);
    file.close();
}

void Program::Footswitch::parse(uint8_t id, const JsonObjectConst& obj) 
{
    _available = false;
    _id = id;

    if (obj.isNull())
    {
        return;
    }

    if (obj["name"].isNull())
    {
        return;
    }

    const String& name = obj["name"];
    unsigned int name_length = name.length();
    if (name_length == 0 || name_length > kMaxNameLength)
    {
        return;
    }
    name.toCharArray(_name, sizeof(_name));

    _enabled = obj["enabled"].as<bool>();
    _momentary = obj["momentary"].as<bool>();

    const String& color = obj["color"].as<String>();
    if (color == "blue") { _color = kBlue; } 
    else if (color == "purple") { _color = kPurple; } 
    else if (color == "red") { _color = kRed; } 
    else if (color == "yellow") { _color = kYellow; } 
    else if (color == "green") { _color = kGreen; } 
    else if (color == "turquoise") { _color = kTurquoise; }
    else { return; }

    _available = true;
}

void Program::Footswitch::serialize(JsonObject& obj) const
{
    obj["name"] = _name;
    switch (_color)
    {
        case kBlue:
            obj["color"] = "blue";
            break;
        case kPurple:
            obj["color"] = "purple";
            break;
        case kRed:
            obj["color"] = "red";
            break;
        case kYellow:
            obj["color"] = "yellow";
            break;
        case kGreen:
            obj["color"] = "green";
            break;
        case kTurquoise:
            obj["color"] = "turquoise";
            break;
        default:
            break;
    }

    if (_enabled)
    {
        obj["enabled"] = true;
    }

    if (_momentary)
    {
        obj["momentary"] = true;
    }
}

}