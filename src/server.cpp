#include "server.h"

#include <ESPmDNS.h>
#include <SPIFFS.h>

namespace tocata {

Server* Server::_singleton;

void Server::begin(Config& config)
{
    assert(!_singleton);    
    _singleton = this;

    startWifi(config);
}

void Server::startWifi(Config& config)
{
    auto wifi_config = config.wifi();
    if (wifi_config.available())
    {
        Serial.print("Connecting to ");
        Serial.println(wifi_config.ssid());
        WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
            Serial.print("Connected with IP "); 
            Serial.println(WiFi.localIP());
            _singleton->startHttp();
        }, SYSTEM_EVENT_STA_GOT_IP);
        WiFi.begin(wifi_config.ssid(), wifi_config.key());
    }
    else
    {
        Serial.println("No wifi config");
        WiFi.softAP(_hostname);
        Serial.println("WiFi AP started"); 
        _singleton->startHttp();
    }
}

void Server::startHttp()
{
    _server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html"); 
    _server.serveStatic("/static/js", SPIFFS, "/"); 
    _server.serveStatic("/static/css", SPIFFS, "/"); 
    _server.onNotFound([](AsyncWebServerRequest *request) {
        if (request->url().lastIndexOf('.') < 0) 
        {
            request->send(SPIFFS, "/index.html", String());
        }
        else
        {
            request->send(404, "text/plain", "Not found");
        }
    });
 
   _server.begin();
    MDNS.begin(_hostname);
    MDNS.addService("http", "tcp", 80);

   Serial.println("HTTP server started");
}

}