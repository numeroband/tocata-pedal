#include <config.h>
#include "config_legacy.h"
#include "hal.h"

#include <midi_sender.h>
#include <filesystem.h>

#define _log(...) 
#define _logln(...) 

namespace tocata {

#define FAKE_CONFIG 0

#if FAKE_CONFIG
static Config sConfig = {};
static Program sPrograms[Program::kMaxPrograms] = {};
#endif

void Storage::init()
{
#if !FAKE_CONFIG
    auto start = millis();

    TocataFS.init(true);

    if (Config::init())
    {
        Program::initAll();
    }

    auto end = millis();
    printf("Storage init in ms: %u\n", end - start);
    printf("Usage: %u / %u\n", (uint32_t)TocataFS.usedBytes(), (uint32_t)TocataFS.totalBytes());
#endif
    printf("Config size: %u, Program size: %u, Setlist size: %u, Footswitch size: %u, Actions size: %u, Action size: %u\n",
        (uint32_t)sizeof(Config),
        (uint32_t)sizeof(Program),
        (uint32_t)sizeof(Setlist),
        (uint32_t)sizeof(Program::Footswitch),
        (uint32_t)sizeof(Actions),
        (uint32_t)sizeof(Actions::Action));
}

void Storage::factoryReset()
{
    printf("Factory reset: erasing config, all programs and all setlists\n");
    Config::remove(false);
    Program::initAll();
    Setlist::removeAll();
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

    _midi = {};
    _expression = {};

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

    migrate(bytes_read);

    return available();
}

void Config::migrate(size_t bytes_read)
{
    // Fields not present in `bytes_read` keep the defaults set at the top of
    // load(); here we decide whether the bytes we did read are trustworthy.
    if (bytes_read == sizeof(*this))
    {
        // Current format (v1): everything was read.
        return;
    }

    if (bytes_read == sizeof(ConfigV0))
    {
        // Legacy v0 file: MIDI channel was read; expression stays at defaults.
        return;
    }

    // Unknown/corrupt size: discard everything, fall back to defaults.
    _logln(F("Invalid config file"));
    _midi = {};
    _expression = {};
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
    current.load();
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
        && _midi == other._midi
        && _expression == other._expression
    );
}

void Actions::Action::run(MidiSender& midi, uint8_t global_channel) const
{
    switch (type())
    {
    case kProgramChange:
        midi.sendProgram(channel(global_channel), _values[0]);
        break;
    case kControlChange:
        midi.sendControl(channel(global_channel), _values[0], _values[1]);
        break;
    default:
        break;
    }
}

bool Actions::Action::operator==(const Actions::Action& other)
{
    return (true
        && _channel_and_type == other._channel_and_type
        && memcmp(_values, other._values, sizeof(_values)) == 0
    );
}

void Actions::run(MidiSender& midi, uint8_t global_channel) const
{
    for (uint8_t i = 0; i < _num_actions; ++i)
    {
        _actions[i].run(midi, global_channel);
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

bool Config::MidiConfig::operator==(const Config::MidiConfig& other)
{
    return (true
        && available() == other.available()
        && _channel == other._channel
    );
}

bool Config::ExpressionConfig::operator==(const Config::ExpressionConfig& other)
{
    return (true
        && _minRaw == other._minRaw
        && _maxRaw == other._maxRaw
    );
}

void Program::initAll()
{
    for (uint32_t id = 0; id < kMaxPrograms; ++id)
    {
        remove(id, false);
    }
}

void Program::run(MidiSender& midi, uint8_t global_channel) const
{
    _actions.run(midi, global_channel);
    for (uint8_t i = 0; i < _num_switches; ++i)
    {
        const Footswitch& fs = _switches[i];
        if (fs.available())
        {
            fs.run(midi, fs.enabled(), global_channel);
        }
    }
}

void Program::sendExpression(MidiSender& midi, uint8_t value, uint8_t global_channel) const
{
    midi.sendControl(expressionChannel(global_channel), expression(), value);
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

uint8_t Program::defaultScene() const
{
    // First enabled scene switch wins; else the first available scene switch
    // becomes the default active scene; else 0 (a safe sentinel for programs
    // with no scene switches -- switchMode() never reports that switch as scene).
    int first_scene = -1;
    for (uint8_t fs_id = 0; fs_id < numFootswitches(); ++fs_id)
    {
        if (switchMode(fs_id) != Footswitch::kScene)
        {
            continue;
        }
        auto& fs = footswitch(fs_id);
        if (!fs.available())
        {
            continue;
        }
        if (fs.enabled())
        {
            return fs_id;
        }
        if (first_scene < 0)
        {
            first_scene = fs_id;
        }
    }
    return (first_scene < 0) ? 0 : uint8_t(first_scene);
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
        && _channel_and_mode == other._channel_and_mode
        && _expression == other._expression
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

void Program::Footswitch::run(MidiSender& midi, bool active, uint8_t global_channel) const
{
    if (active)
    {
        _on_actions.run(midi, global_channel);
    }
    else
    {
        _off_actions.run(midi, global_channel);
    }
}

void Setlist::loadAll()
{
    static constexpr char kAllName[] = "All";

    memset(_name, 0, sizeof(_name));
    memcpy(_name, kAllName, sizeof(kAllName));
    _num_programs = Program::kMaxPrograms;
    for (uint8_t pos = 0; pos < Program::kMaxPrograms; ++pos)
    {
        _programs[pos] = pos;
    }
}

uint8_t Setlist::copyName(uint8_t id, char* name)
{
    name[0] = 0;

    if (id >= kMaxSetlists)
    {
        return 0;
    }

    char path[Program::kMaxPathSize];
    copyPath(id, path);

    File file = TocataFS.open(path, FILE_READ);
    if (!file)
    {
        return 0;
    }

    uint8_t header[kHeaderSize];
    size_t bytes_read = file.read(header, sizeof(header));
    file.close();

    if (bytes_read != sizeof(header))
    {
        _log(F("Invalid setlist file "));
        _logln(path);
        return 0;
    }

    memcpy(name, header, Program::kMaxNameLength + 1);
    name[Program::kMaxNameLength] = 0;

    const uint8_t num_programs = header[Program::kMaxNameLength + 1];
    if (!name[0] || num_programs == 0 || num_programs > Program::kMaxPrograms)
    {
        name[0] = 0;
        return 0;
    }

    return num_programs;
}

bool Setlist::load(uint8_t id)
{
    invalidate();
    _num_programs = 0;

    bool usable = false;
    if (id < kMaxSetlists)
    {
        char path[Program::kMaxPathSize];
        copyPath(id, path);

        File file = TocataFS.open(path, FILE_READ);
        if (file)
        {
            size_t bytes_read = file.read((uint8_t*)this, sizeof(*this));
            file.close();
            usable = bytes_read == sizeof(*this)
                && available()
                && _num_programs > 0
                && _num_programs <= Program::kMaxPrograms;
        }
    }

    if (!usable)
    {
        // Never leave the caller holding an unusable setlist: navigation wraps
        // modulo numPrograms(), and "All" is the natural fallback.
        loadAll();
        return false;
    }

    _name[Program::kMaxNameLength] = 0;
    return true;
}

int16_t Setlist::find(uint8_t program_id) const
{
    for (uint8_t pos = 0; pos < _num_programs; ++pos)
    {
        if (_programs[pos] == program_id)
        {
            return int16_t(pos);
        }
    }

    return -1;
}

void Setlist::remove(uint8_t id)
{
    if (id >= kMaxSetlists)
    {
        _log(F("Invalid setlist to remove "));
        _logln(id);
        return;
    }

    char path[Program::kMaxPathSize];
    copyPath(id, path);
    // Unlike programs, setlists are never pre-created, so deleting frees the
    // slot outright instead of leaving an empty file behind. That keeps the file
    // table small on the short pedal, where one 64K block holds just 127 files.
    TocataFS.remove(path);
}

void Setlist::removeAll()
{
    for (uint8_t id = 0; id < kMaxSetlists; ++id)
    {
        remove(id);
    }
}

void Setlist::save(uint8_t id) const
{
    if (id >= kMaxSetlists || !available() || _num_programs == 0)
    {
        return;
    }

    Setlist current;
    if (current.load(id) && current == *this)
    {
        return;
    }

    char path[Program::kMaxPathSize];
    copyPath(id, path);

    File file = TocataFS.open(path, FILE_WRITE);
    if (!file)
    {
        _log(F("Cannot open setlist file to write "));
        _logln(path);
        return;
    }

    size_t written = file.write((const uint8_t*)this, sizeof(*this));
    file.close();

    if (written != sizeof(*this))
    {
        _log(F("Cannot write setlist to file "));
        _logln(path);
        TocataFS.remove(path);
    }
}

bool Setlist::operator==(const Setlist& other) const
{
    return (true
        && _num_programs == other._num_programs
        && strncmp(_name, other._name, sizeof(_name)) == 0
        && memcmp(_programs, other._programs, _num_programs) == 0
    );
}

bool Program::Footswitch::operator==(const Program::Footswitch& other)
{
    return ((!available() && !other.available()) || (true
        && available() == other.available()
        && _color == other._color
        && _enabled == other._enabled
        && _mode == other._mode
        && _on_actions == other._on_actions
        && _off_actions == other._off_actions
        && strncmp(_name, other._name, sizeof(_name)) == 0
    ));
}

}