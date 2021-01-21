#include "config.h"

#ifdef TOCATA_USE_SPIFFS
#include <SPIFFS.h>
#define TocataFS SPIFFS
#else
#include "filesystem.h"
#endif // TOCATA_USE_SPIFFS

namespace tocata {

void Storage::begin()
{
    auto start = millis();

    TocataFS.begin(true);

    if (Config::init())
    {
        Program::initAll();
    }

    auto end = millis();
    Serial.print(F("Storage init in ms: "));
    Serial.println(end - start);
    Serial.print(F("Usage: "));
    Serial.print(TocataFS.usedBytes());
    Serial.print('/');
    Serial.println(TocataFS.totalBytes());
    Serial.print(F("Config size: "));
    Serial.print(sizeof(Config));
    Serial.print(F(", Program size: "));
    Serial.print(sizeof(Program));
    Serial.print(F(", Footswitch size: "));
    Serial.print(sizeof(Program::Footswitch));
    Serial.print(F(", Actions size: "));
    Serial.print(sizeof(Actions));
    Serial.print(F(", Action size: "));
    Serial.println(sizeof(Actions::Action));
}

bool Config::init()
{
    if (TocataFS.exists(kPath))
    {
        return false;
    }
    else
    {
        remove(false);
        return true;
    }
}

void Config::remove(bool check)
{
    if (check)
    {
        Config current{};
        if (!current.available())
        {
            return;
        }
    }

    auto file = TocataFS.open(kPath, FILE_WRITE);
    if (!file)
    {
        Serial.println(F("Cannot open config file to init"));
        return;
    }
    file.close();
}

bool Config::load()
{    
    _available = false;

    auto file = TocataFS.open(kPath, FILE_READ);
    if (!file)
    {
        Serial.println(F("Cannot open config file to load"));
        return false;
    }
    size_t bytes_read = file.read((uint8_t*)this, sizeof(*this));
    file.close();

    if (bytes_read == 0)
    {
        return false;
    }

    if (bytes_read != sizeof(*this))
    {
        Serial.println(F("Invalid config file"));
        _available = false;
    }

    return _available;
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

    Config current{};
    if (current == *this)
    {
        return;
    }

    auto file = TocataFS.open(kPath, FILE_WRITE);
    if (!file)
    {
        Serial.println(F("Cannot open config file to write"));
        return;
    }

    size_t written = file.write((uint8_t*)this, sizeof(*this));
    file.close();

    if (written != sizeof(*this))
    {
        Serial.println(F("Cannot write config to file"));
        TocataFS.remove(kPath);
    }
}

bool Config::operator==(const Config& other)
{
    return (true
        && _available == other._available
        && _wifi == other._wifi
    );
}

void Actions::Action::run(FsMidi& midi) const
{
    switch (_type)
    {
    case kProgramChange:
        midi.sendProgram(_values[0]);
        break;
    case kControlChange:
        midi.sendControl(_values[0], _values[1]);
        break;
    default:
        break;
    }
}

bool Actions::Action::parse(const JsonObjectConst& obj)
{
    _type = kNone;
    memset(_values, 0, sizeof(_values));

    const String& type = obj["type"];
    if (type == "PC")
    {
        _type = kProgramChange;
        _values[0] = obj["program"].as<uint8_t>();
    }
    else if (type == "CC")
    {
        _type = kControlChange;
        _values[0] = obj["control"].as<uint8_t>();
        _values[1] = obj["value"].as<uint8_t>();
    }
    else
    {
        return false;
    }
    return true;
}

void Actions::Action::serialize(JsonObject& obj) const
{
    switch (_type)
    {
    case kProgramChange:
        obj["type"] = "PC";
        obj["program"] = _values[0];
        break;
    case kControlChange:
        obj["type"] = "CC";
        obj["control"] = _values[0];
        obj["value"] = _values[1];
        break;
    default:
        break;
    }
}

bool Actions::Action::operator==(const Actions::Action& other)
{
    return (true
        && _type == other._type
        && memcmp(_values, other._values, sizeof(_values)) == 0
    );
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

    if (array.isNull())
    {
        return _num_actions;
    }

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

bool Actions::operator==(const Actions& other)
{
    if (_num_actions != other._num_actions)
    {
        return false;
    }

    for (uint32_t i = 0; i < _num_actions; ++i)
    {
        if (!(_actions[i] == other._actions[i]))
        {
            return false;
        }
    }

    return true;
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

bool Config::Wifi::operator==(const Config::Wifi& other)
{
    return (true
        && _available == other._available
        && strncmp(_ssid, other._ssid, sizeof(_ssid)) == 0
        && strncmp(_key, other._key, sizeof(_key)) == 0
    );
}

void Program::initAll()
{
    for (uint32_t id = 0; id < kMaxPrograms; ++id)
    {
        remove(id, false);
    }
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

uint8_t Program::copyName(uint8_t id, char* name)
{
    char path[kMaxPathSize];
    copyPath(id, path);

    auto file = TocataFS.open(path, FILE_READ);
    if (!file)
    {
        Serial.println(F("Cannot open to copy name"));
        return 0;
    }
    size_t bytes_read = file.read((uint8_t*)name, kMaxNameLength);
    file.close();

    if (bytes_read == 0)
    {
        return 0;
    }

    if (bytes_read != kMaxNameLength)
    {
        Serial.println(F("Invalid program file"));
        return 0;
    }

    return strnlen(name, kMaxNameLength);
}

void Program::remove(uint8_t id, bool check)
{
    if (id >= kMaxPrograms)
    {
        Serial.print(F("Invalid program to remove "));
        Serial.println(id);
        return;
    }

    if (check)
    {
        Program current{id};
        if (!current.available())
        {
            return;
        }
    }

    char path[kMaxPathSize];
    copyPath(id, path);

    auto file = TocataFS.open(path, FILE_WRITE);
    if (!file)
    {
        Serial.print(F("Cannot open program file to init"));
        Serial.println(path);
        return;
    }
    file.close();
}

bool Program::load(uint8_t id)
{
    if (id > kMaxPrograms)
    {
        _available = false;
        _id = id;

        Serial.print(F("Invalid program to load "));
        Serial.println(id);

        return false;
    }

    char path[kMaxPathSize];
    copyPath(id, path);

    auto file = TocataFS.open(path, FILE_READ);
    if (!file)
    {
        Serial.print(F("Cannot open program to load "));
        Serial.println(path);
    }
    size_t bytes_read = file.read((uint8_t*)this, sizeof(*this));
    file.close();

    _id = id;

    if (bytes_read == 0)
    {
        return false;
    }

    if (bytes_read != sizeof(*this))
    {
        Serial.print(F("Invalid program file "));
        Serial.println(path);
        _available = false;
        return false;
    }

    if (!_available)
    {
        return false;
    }

    for (uint32_t i = 0; i < _num_switches; ++i)
    {
        Footswitch& fs = _switches[i];
        if (fs.available())
        {
            fs.reset();
        }
    }

    return true;
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

    Program current{_id};
    if (current == *this)
    {
        return;
    }

    char path[kMaxPathSize];
    copyPath(_id, path);
    auto file = TocataFS.open(path, FILE_WRITE);
    if (!file)
    {
        Serial.print(F("Cannot open program file to write "));
        Serial.println(path);
        return;
    }

    size_t written = file.write((uint8_t*)this, sizeof(*this));
    file.close();

    if (written != sizeof(*this))
    {
        Serial.print(F("Cannot write program to file "));
        Serial.println(path);
    }
}

bool Program::operator==(const Program& other)
{
    if (!(true
        && _available == other._available
        && _id == other._id
        && _num_switches == other._num_switches
        && strncmp(_name, other._name, sizeof(_name)) == 0
        && _actions == other._actions
    )) return false;

    for (uint32_t i = 0; i < _num_switches; ++i)
    {
        if (!(_switches[i] == other._switches[i]))
        {
            return false;
        }
    }

    return true;
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

    JsonArray on_actions = obj["onActions"].to<JsonArray>();
    _on_actions.serialize(on_actions);
    JsonArray off_actions = obj["offActions"].to<JsonArray>();
    _off_actions.serialize(off_actions);
}

bool Program::Footswitch::operator==(const Program::Footswitch& other)
{
    return ((!_available && !other._available) || (true
        && _available == other._available
        && _id == other._id
        && _color == other._color
        && _default == other._default
        && _momentary == other._momentary
        && _on_actions == other._on_actions
        && _off_actions == other._off_actions
        && strncmp(_name, other._name, sizeof(_name)) == 0
    ));
}

}