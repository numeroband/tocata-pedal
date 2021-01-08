#include "server.h"

#include <ESPmDNS.h>
#include <SPIFFS.h>

namespace tocata {

extern const uint8_t index_html_gz_start[] asm("_binary_data_index_html_gz_start");
extern const uint8_t index_html_gz_end[] asm("_binary_data_index_html_gz_end");
static const size_t index_html_len = index_html_gz_end - index_html_gz_start;
static const uint8_t* index_html = index_html_gz_start;

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
    _server.on("/config.json", [](AsyncWebServerRequest *request) {
       request->send(SPIFFS, "/config.json", String());
    });
    _server.onNotFound([](AsyncWebServerRequest *request) {
        if (request->url().lastIndexOf('.') < 0) 
        {
            AsyncWebServerResponse *response = request->beginResponse(
                String("text/html"),
                index_html_len,
                [](uint8_t *buffer, size_t max_len, size_t already_sent) {
                    size_t remaining = index_html_len - already_sent;
                    size_t to_send = std::min(remaining, max_len);
                    memcpy(buffer, index_html + already_sent, to_send);

                    return to_send;
                }
            );
            response->addHeader("Content-Encoding", "gzip");
            response->addHeader("Content-Disposition", "inline; filename=\"index.html\"");
            request->send(response);  
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