#pragma once

#include "config.h"

#include <ESPAsyncWebServer.h>

#include <functional>

namespace tocata {

class Server
{
public:
    using ConfigUpdated = std::function<void()>;
    Server(const char* hostname, ConfigUpdated config_updated) : _hostname(hostname), _config_updated(config_updated) {}
    void begin(Config& config);

private:
    static Server* _singleton;

    void startWifi(Config& config);
    void startHttp();

    AsyncWebServer _server{80};
    const char* _hostname;
    ConfigUpdated _config_updated;

};

}