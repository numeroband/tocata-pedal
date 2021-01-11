#include "server.h"

#include <ESPmDNS.h>
#include <SPIFFS.h>

namespace tocata {

extern const uint8_t index_html_gz_start[] asm("_binary_html_index_html_gz_start");
extern const uint8_t index_html_gz_end[] asm("_binary_html_index_html_gz_end");
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
    _server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request) 
    {
        if (SPIFFS.exists(Config::configPath())) 
        {
            request->send(SPIFFS, Config::configPath(), String());
        }
        else
        {
            request->send(404, "application/json", "{}");
        }
    });
    _server.on("/api/programs", HTTP_GET, [](AsyncWebServerRequest *request) 
    {
        size_t num_params = request->params();
        if (num_params == 0)
        {
            request->send(SPIFFS, "/names.txt", String());
            return;
        }

        AsyncWebParameter* p = request->getParam(0);
        if (p->name() != "id")
        {
            request->send(500, "text/plain", "Invalid query");
            return;
        }

        uint8_t id = static_cast<uint8_t>(p->value().toInt());
        char path[16];
        Config::Program::copyPath(id, path, sizeof(path));
        if (SPIFFS.exists(path)) 
        {
            request->send(SPIFFS, path, String());
        }
        else
        {
            request->send(404, "application/json", "{}");
        }
    });
    _server.onFileUpload([this](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
        if (request->url() != "/api/firmware")
        {
            request->send(404, "text/plain", "Not found");
            return;
        }

        // handle upload and update
        if (!index)
        {            
            Serial.printf("Updating config: %s\n", filename.c_str());
            request->_tempFile = SPIFFS.open("/config.json", "w");
            
            if (!request->_tempFile)
            {
                Serial.print("ERROR opening /config.json");
                request->send(500, "text/plain", "Internal error");
                return;
            }
        }

        if (len)
        {
            size_t remaining = len;
            while (remaining) 
            {
                size_t written = request->_tempFile.write(data, remaining);
                if (written == 0)
                {
                    Serial.print("ERROR writing /config.json");
                    request->_tempFile.close();
                    request->send(500, "text/plain", "Internal error");
                    return;
                }
                remaining -= written;
                data += written;
            }
        }

        if (final)
        {
            request->_tempFile.close();
            request->send(200, "text/plain", "Config updated");
            Serial.println("Config updated");
            _config_updated();
        }
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