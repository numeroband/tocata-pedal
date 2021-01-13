#include "server.h"

#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <Update.h>

namespace tocata {

using namespace std::placeholders;

extern const uint8_t index_html_gz_start[] asm("_binary_html_index_html_gz_start");
extern const uint8_t index_html_gz_end[] asm("_binary_html_index_html_gz_end");
static const size_t index_html_len = index_html_gz_end - index_html_gz_start;
static const uint8_t* index_html = index_html_gz_start;

Server* Server::_singleton;

Server::Server(
    const char* hostname, 
    ConfigUpdated config_updated,
    ProgramUpdated program_updated) :
_hostname(hostname), 
_config_updated(config_updated),
_program_updated(program_updated),
_program_handler("/api/programs", std::bind(&Server::updateProgram, this, _1, _2), Program::kMaxJsonSize),
_config_handler("/api/config", std::bind(&Server::updateConfig, this, _1, _2), Config::kMaxJsonSize)
{
}

void Server::begin()
{
    assert(!_singleton);    
    _singleton = this;

    auto start = millis();
    memset(_names, ' ', sizeof(_names));
    Program program;
    for (uint8_t id = 0; id < Program::kMaxPrograms; ++id)
    {
        if (program.load(id))
        {
            memcpy(_names + (id * Program::kMaxNameLength), program.name(), strlen(program.name()));
        }
    }
    auto end = millis();
    Serial.print(F("Names cached: "));
    Serial.println(end - start);

    startWifi();
}

void Server::loop()
{
}

void Server::startWifi()
{
    Config config{};
    auto wifi_config = config.wifi();
    _ap = !wifi_config.available();
    if (_ap)
    {
        Serial.println(F("No wifi config"));
        WiFi.softAP(_hostname);
        Serial.println(F("WiFi AP started")); 
        startHttp();
    }
    else
    {
        _ap = false;
        Serial.print(F("Connecting to "));
        Serial.println(wifi_config.ssid());
        WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
            Serial.print(F("Connected with IP ")); 
            Serial.println(WiFi.localIP());
            _singleton->startHttp();
        }, SYSTEM_EVENT_STA_GOT_IP);
        WiFi.begin(wifi_config.ssid(), wifi_config.key());
    }
}

void Server::startHttp()
{
    _server.addHandler(&_program_handler);
    _server.addHandler(&_config_handler);
    _server.on("/api/restart", HTTP_POST, std::bind(&Server::restart, this, _1));
    _server.on("/api/config", HTTP_GET, std::bind(&Server::getConfig, this, _1));
    _server.on("/api/config", HTTP_DELETE, std::bind(&Server::removeConfig, this, _1));
    _server.on("/api/programs", HTTP_GET, std::bind(&Server::getPrograms, this, _1));
    _server.on("/api/programs", HTTP_DELETE, std::bind(&Server::removeProgram, this, _1));
    _server.onFileUpload(std::bind(&Server::firmwareUpload, this, _1, _2, _3, _4, _5, _6));
    _server.onNotFound(std::bind(&Server::sendIndex, this, _1));
 
    _server.begin();
    MDNS.begin(_hostname);
    MDNS.addService("http", "tcp", 80);

    Serial.println(F("HTTP server started"));
}

void Server::sendIndex(AsyncWebServerRequest *request)
{
    if (request->url().lastIndexOf('.') >= 0) 
    {
        request->send(404);
        return;
    }

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

void Server::restart(AsyncWebServerRequest *request)
{
    request->send(200, "application/json", "true");
    Serial.println(F("Restarting in 1 second..."));
    delay(1);
    ESP.restart();
}

void Server::getConfig(AsyncWebServerRequest *request)
{
    Config config{};
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    DynamicJsonDocument doc{Config::kMaxJsonSize};
    JsonObject root = doc.to<JsonObject>();
    config.serialize(root);
    serializeJson(root, *response);
    request->send(response);
}    

void Server::updateConfig(AsyncWebServerRequest *request, JsonVariant &json)
{
    const JsonObjectConst& obj = json.as<JsonObjectConst>();
    Config config{obj};
    if (!config.available())
    {
        request->send(400);
        return;
    }
    config.save();
    request->send(200, "application/json", "true");
    _config_updated();
}

void Server::removeConfig(AsyncWebServerRequest *request)
{
    Config::remove();
    request->send(200, "application/json", "true");
    _config_updated();
}

void Server::getPrograms(AsyncWebServerRequest *request)
{
    uint8_t id;
    if (!getProgramId(request, &id, true))
    {
        return;
    }

    if (id != Program::kInvalidId)
    {
        getProgram(request, id);
        return;
    }

    request->send("text/plain", sizeof(_names), [this](uint8_t *buffer, size_t max_len, size_t already_sent)
    {
        memcpy(buffer, _names + already_sent, max_len);
        return max_len;
    });
}

void Server::getProgram(AsyncWebServerRequest *request, uint8_t id)
{
    Program program{id};
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    DynamicJsonDocument doc{Program::kMaxJsonSize};
    JsonObject root = doc.to<JsonObject>();
    program.serialize(root);
    serializeJson(root, *response);
    request->send(response);
}

void Server::updateProgram(AsyncWebServerRequest *request, JsonVariant &json)
{
    uint8_t id;
    if (!getProgramId(request, &id))
    {
        return;
    }

    const JsonObjectConst& obj = json.as<JsonObjectConst>();
    Program program{id, obj};
    if (!program.available())
    {
        request->send(400);
        return;
    }
    program.save();
    addProgramName(program);
    request->send(200, "application/json", "true");
    _program_updated(id);
}

void Server::removeProgram(AsyncWebServerRequest *request)
{
    uint8_t id;
    if (!getProgramId(request, &id))
    {
        return;
    }

    Program::remove(id);
    removeProgramName(id);
    request->send(200, "application/json", "true");
    _program_updated(id);
}

void Server::firmwareUpload(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) 
{
    if (request->url() != "/api/firmware")
    {
        request->send(404);
        return;
    }

    // handle upload and update
    if (!index)
    {            
        Serial.print(F("Updating firmware: "));
        Serial.println(filename.c_str());
        if (!Update.begin(1024 * 1024))
        {
            Serial.print(F("Not enough space"));
            request->send(400);
        }
    }

    if (len)
    {
        size_t remaining = len;
        while (remaining) 
        {
            size_t written = Update.write(data, remaining);
            if (written == 0)
            {
                Serial.print(F("ERROR writing firmware"));
                Update.abort();
                request->send(500);
                return;
            }
            remaining -= written;
            data += written;
        }
    }

    if (final)
    {
        if (!Update.end())
        {
            Serial.print(F("ERROR ending firmware update"));
            request->send(500);
            return;
        }
        request->send(200, "text/plain", "Firmware updated");
        Serial.println(F("Firmware updated. Rebooting in 1 second..."));
        delay(1000);
        ESP.restart();
    }
}

void Server::addProgramName(const Program& program)
{
    uint8_t* name = _names + (program.id() * Program::kMaxNameLength);
    size_t name_len = strnlen(program.name(), Program::kMaxNameLength);
    memcpy(name, program.name(), name_len);
    memset(name + name_len, ' ', Program::kMaxNameLength - name_len);
}

void Server::removeProgramName(uint8_t id)
{
    uint8_t* name = _names + (id * Program::kMaxNameLength);
    memset(name, ' ', Program::kMaxNameLength);
}

bool Server::getProgramId(AsyncWebServerRequest *request, uint8_t* id, bool noParamValid)
{
    auto param = request->getParam("id");
    if (!param)
    {        
        if (noParamValid)
        {
            *id = Program::kInvalidId;
            return true;
        }
        else
        {
            request->send(400);
            return false;
        }        
    }

    *id = static_cast<uint8_t>(param->value().toInt());
    if (*id >= Program::kMaxPrograms)
    {
        request->send(400);
        return false;
    }

    return true;
}

}