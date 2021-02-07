#include <config.h>

#include "filesystem.h"
#include "bsp/board.h"

#define _log(...) 
#define _logln(...) 

namespace tocata {

#define FAKE_CONFIG 1

#if FAKE_CONFIG
static Config sConfig = {};
static Program sPrograms[Program::kMaxPrograms] = {};
#endif

void Storage::begin()
{
    auto start = board_millis();

    TocataFS.begin(true);

    if (Config::init())
    {
        Program::initAll();
    }

    auto end = board_millis();
    printf("Storage init in ms: %u\n", end - start);
    printf("Usage: %u / %u\n", TocataFS.usedBytes(), TocataFS.totalBytes());
    printf("Config size: %u, Program size: %u, Footswitch size: %u, Actions size: %u, Action size: %u\n",
        sizeof(Config),
        sizeof(Program),
        sizeof(Program::Footswitch),
        sizeof(Actions),
        sizeof(Actions::Action));
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
#if FAKE_CONFIG
    memset(&sConfig, 0, sizeof(Config));
    return;
#endif

    if (check)
    {
        Config current{};
        if (!current.available())
        {
            return;
        }
    }

    File file = TocataFS.open(kPath, FILE_WRITE);
    if (!file)
    {
        _logln(F("Cannot open config file to init"));
        return;
    }
    file.close();
}

bool Config::load()
{    
#if FAKE_CONFIG
    memcpy(this, &sConfig, sizeof(Config));
    return available();
#endif

    _wifi = {};

    File file = TocataFS.open(kPath, FILE_READ);
    if (!file)
    {
        _logln(F("Cannot open config file to load"));
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
        _logln(F("Invalid config file"));
        _wifi = {};
    }

    return available();
}

void Config::save() const
{
#if FAKE_CONFIG
    memcpy(&sConfig, this, sizeof(Config));
    return;
#endif

    if (!available())
    {
        return;
    }

    Config current{};
    if (current == *this)
    {
        return;
    }

    File file = TocataFS.open(kPath, FILE_WRITE);
    if (!file)
    {
        _logln(F("Cannot open config file to write"));
        return;
    }

    size_t written = file.write((uint8_t*)this, sizeof(*this));
    file.close();

    if (written != sizeof(*this))
    {
        _logln(F("Cannot write config to file"));
        TocataFS.remove(kPath);
    }
}

bool Config::operator==(const Config& other)
{
    return (true
        && _wifi == other._wifi
    );
}

void Actions::Action::run(MidiUsb& midi) const
{
    switch (_type)
    {
    case kProgramChange:
        // midi.sendProgram(_values[0]);
        break;
    case kControlChange:
        // midi.sendControl(_values[0], _values[1]);
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

void Actions::run(MidiUsb& midi) const
{
    for (uint8_t i = 0; i < _num_actions; ++i)
    {
        _actions[i].run(midi);
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

bool Config::Wifi::operator==(const Config::Wifi& other)
{
    return (true
        && available() == other.available()
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

void Program::run(MidiUsb& midi) const
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
#if FAKE_CONFIG
    memcpy(name, sPrograms[id]._name, kMaxNameLength + 1);
    return strnlen(name, kMaxNameLength);
#endif    

    char path[kMaxPathSize];
    copyPath(id, path);

    File file = TocataFS.open(path, FILE_READ);
    if (!file)
    {
        _logln(F("Cannot open to copy name"));
        return 0;
    }
    size_t bytes_read = file.read((uint8_t*)name, kMaxNameLength + 1);
    file.close();

    if (bytes_read == 0)
    {
        return 0;
    }

    if (bytes_read != kMaxNameLength)
    {
        _logln(F("Invalid program file"));
        return 0;
    }

    return strnlen(name, kMaxNameLength);
}

void Program::remove(uint8_t id, bool check)
{
#if FAKE_CONFIG
    memset(&sPrograms[id], 0, sizeof(Program));
#endif

    if (id >= kMaxPrograms)
    {
        _log(F("Invalid program to remove "));
        _logln(id);
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

    File file = TocataFS.open(path, FILE_WRITE);
    if (!file)
    {
        _log(F("Cannot open program file to init"));
        _logln(path);
        return;
    }
    file.close();
}

bool Program::load(uint8_t id)
{
#if FAKE_CONFIG
    memcpy(this, &sPrograms[id], sizeof(Program));
    return available();
#endif

    invalidate();

    if (id > kMaxPrograms)
    {
        _log(F("Invalid program to load "));
        _logln(id);
        return false;
    }

    char path[kMaxPathSize];
    copyPath(id, path);

    File file = TocataFS.open(path, FILE_READ);
    if (!file)
    {
        _log(F("Cannot open program to load "));
        _logln(path);
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
        _log(F("Invalid program file "));
        _logln(path);
        return false;
    }

    return available();
}

void Program::save(uint8_t id) const
{
#if FAKE_CONFIG
    memcpy(&sPrograms[id], this, sizeof(Program));
    return;
#endif

    if (!available())
    {
        return;
    }

    Program current{id};
    if (current == *this)
    {
        return;
    }

    char path[kMaxPathSize];
    copyPath(id, path);
    File file = TocataFS.open(path, FILE_WRITE);
    if (!file)
    {
        _log(F("Cannot open program file to write "));
        _logln(path);
        return;
    }

    size_t written = file.write((uint8_t*)this, sizeof(*this));
    file.close();

    if (written != sizeof(*this))
    {
        _log(F("Cannot write program to file "));
        _logln(path);
    }
}

bool Program::operator==(const Program& other)
{
    if (!(true
        && available() == other.available()
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

void Program::Footswitch::run(MidiUsb& midi) const
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

bool Program::Footswitch::operator==(const Program::Footswitch& other)
{
    return ((!available() && !other.available()) || (true
        && available() == other.available()
        && _color == other._color
        && _enabled == other._enabled
        && _momentary == other._momentary
        && _on_actions == other._on_actions
        && _off_actions == other._off_actions
        && strncmp(_name, other._name, sizeof(_name)) == 0
    ));
}

}