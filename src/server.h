#pragma once

#include "config.h"

#include <ESPAsyncWebServer.h>

namespace tocata {

class Server
{
public:
    Server(const char* hostname) : _hostname(hostname) {}
    void begin(Config& config);

private:
    static Server* _singleton;

    void startWifi(Config& config);
    void startHttp();

    AsyncWebServer _server{80};
    const char* _hostname;
};

}