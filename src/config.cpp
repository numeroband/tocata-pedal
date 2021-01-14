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

void Action::run(FsMidi& midi) const
{
    switch (_type)
    {
    case kProgramChange:
        midi.sendProgram(_value1);
        break;
    case kControlChange:
        midi.sendControl(_value1, _value2);
        break;
    default:
        break;
    }
}

bool Action::parse(const JsonObjectConst& obj)
{
    const String& type = obj["type"];
    if (type == "PC")
    {
        _value1 = obj["program"].as<uint8_t>();
    }
    else if (type == "CC")
    {
        _value1 = obj["control"].as<uint8_t>();
        _value2 = obj["value"].as<uint8_t>();
    }
    else
    {
        return false;
    }
    return true;
}

void Action::serialize(JsonObject& obj) const
{
    switch (_type)
    {
    case kProgramChange:
        obj["type"] = "PC";
        obj["program"] = _value1;
        break;
    case kControlChange:
        obj["type"] = "CC";
        obj["control"] = _value1;
        obj["value"] = _value2;
        break;
    default:
        break;
    }
}

void Actions::run(FsMidi& midi) const
{
    for (uint8_t i = 0; i < _num_actions; ++i)
    {
        _actions[i].run(midi);
    }
}

uint8_t Actions::parse(const JsonArrayConst& array)
{    
    _num_actions = 0;
    auto array_size = array.size();

    for (uint8_t i = 0; i < array_size; ++i)
    {
        if (_num_actions < kMaxActions &&
            _actions[_num_actions].parse(array[i].as<JsonObjectConst>()))
        {
            ++_num_actions;
        }
    }

    return _num_actions;
}

void Actions::serialize(JsonArray& array) const
{
    for (uint8_t i = 0; i < _num_actions; ++i)
    {
        JsonObject obj = array[i].to<JsonObject>();
        _actions[i].serialize(obj);
    }
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

void Program::run(FsMidi& midi) const
{
    _actions.run(midi);
    for (uint8_t i = 0; i < _num_switches; ++i)
    {
        const Footswitch& fs = _switches[i];
        if (fs.available())
        {
            fs.run(midi);
        }
    }
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

    _actions.parse(obj["actions"].as<JsonArrayConst>());
    
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

    JsonArray actions = obj["actions"].to<JsonArray>();
    _actions.serialize(actions);
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

void Program::Footswitch::run(FsMidi& midi) const
{
    if (_enabled)
    {
        _on_actions.run(midi);
    }    
    else
    {
        _off_actions.run(midi);
    }    
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

    _default = obj["enabled"].as<bool>();
    _enabled = _default;
    _momentary = obj["momentary"].as<bool>();

    const String& color = obj["color"].as<String>();
    if (color == "blue") { _color = kBlue; } 
    else if (color == "purple") { _color = kPurple; } 
    else if (color == "red") { _color = kRed; } 
    else if (color == "yellow") { _color = kYellow; } 
    else if (color == "green") { _color = kGreen; } 
    else if (color == "turquoise") { _color = kTurquoise; }
    else { return; }

    _on_actions.parse(obj["onActions"].as<JsonArrayConst>());
    _off_actions.parse(obj["offActions"].as<JsonArrayConst>());

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

    if (_default)
    {
        obj["enabled"] = true;
    }

    if (_momentary)
    {
        obj["momentary"] = true;
    }

    JsonArray actions = obj["onActions"].to<JsonArray>();
    _on_actions.serialize(actions);
    actions = obj["offActions"].to<JsonArray>();
    _on_actions.serialize(actions);
}

}