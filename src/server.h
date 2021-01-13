#pragma once

#include "config.h"

#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>

#include <functional>

namespace tocata {

class Server
{
public:
    using ConfigUpdated = std::function<void()>;
    using ProgramUpdated = std::function<void(uint8_t)>;

    Server(
        const char* hostname, 
        ConfigUpdated config_updated,
        ProgramUpdated program_updated);
    void begin();
    void loop();

private:
    static Server* _singleton;
    static constexpr const char* kNamesPath = "/names.txt";

    void startWifi();
    void startHttp();
    void firmwareUpload(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final);
    void getPrograms(AsyncWebServerRequest *request);
    void getProgram(AsyncWebServerRequest *request, uint8_t id);
    void updateProgram(AsyncWebServerRequest *request, JsonVariant &json);
    void removeProgram(AsyncWebServerRequest *request);
    void getConfig(AsyncWebServerRequest *request);
    void updateConfig(AsyncWebServerRequest *request, JsonVariant &json);
    void removeConfig(AsyncWebServerRequest *request);
    void addProgramName(const Program& program);
    void removeProgramName(uint8_t id);
    void sendIndex(AsyncWebServerRequest *request);
    void restart(AsyncWebServerRequest *request);
    bool getProgramId(AsyncWebServerRequest *request, uint8_t* id, bool noParamValid = false);

    AsyncWebServer _server{80};
    const char* _hostname;
    uint8_t _names[Program::kMaxNameLength * Program::kMaxPrograms];
    ConfigUpdated _config_updated;
    ProgramUpdated _program_updated;
    AsyncCallbackJsonWebHandler _program_handler;
    AsyncCallbackJsonWebHandler _config_handler;
    bool _ap = false;
};

}