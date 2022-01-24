#include "hal.h"
#ifndef HAL_PICO

#include "display_sim.h"
#include "application.h"

#define ASIO_STANDALONE
 
#include <web_usb.h>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp> 
#include <libremidi/libremidi.hpp>

#include <functional>
#include <queue>
#include <fstream>

#include <filesystem>
#include <CoreFoundation/CoreFoundation.h>

namespace tocata {

uint8_t MemFlash[2 * 1024 * 1024];

static libremidi::midi_out midi{};
static DisplaySim display;
static Application app{display};

void i2c_write(uint8_t addr, const uint8_t *src, size_t len)
{
    if (display.processTransfer(src, len))
    {
      return;
    }

    printf("Invalid I2C %u bytes to %02X: ", (uint32_t)len, addr);
    for (uint8_t i = 0; i < len; ++i)
    {
    	printf("%02X ", src[i]);
    }
    printf("\n");
}

bool switches_changed(const HWConfigSwitches& config) 
{
  return app.switchesChanged();
}

void leds_refresh(const HWConfigLeds& config, uint32_t* leds, size_t num_leds)
{
  auto adjust = [](uint32_t v32) -> uint8_t {
    uint8_t v8 = static_cast<uint8_t>(v32);
    return (v8 == 0) ? 0 : (v8 + 127);
  };

  for (uint8_t i = 0; i < num_leds; ++i) {
    uint32_t led = leds[i];
    uint8_t r = adjust(led >> 16);
    uint8_t g = adjust(led >> 24);
    uint8_t b = adjust(led >> 8);
    app.setLedColor(i, r, g, b);
  }
}

uint32_t switches_value(const HWConfigSwitches& config) 
{
  return app.switchesValue();
}

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::bind;

class WebSocket
{
public:
  typedef websocketpp::server<websocketpp::config::asio> server;

  void init()
  {
    try {
        auto url = CFBundleCopyResourceURL(CFBundleGetMainBundle(), CFSTR("web"), nullptr, nullptr);
        if (url)
        {
          auto path_cfstr = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
          _http_root = CFStringGetCStringPtr(path_cfstr, kCFStringEncodingUTF8);
          CFRelease(path_cfstr);
          CFRelease(url);
        } else {
          _http_root = std::filesystem::current_path().c_str();
          _http_root += "/build/src/TocataPedal.app/Resources/web";
        }

        std::cout << "HTTP root: " << _http_root << std::endl;
        
        // Set logging settings
        _server.clear_access_channels(websocketpp::log::alevel::all);
        _server.set_access_channels(websocketpp::log::alevel::connect);

        _server.init_asio();

        _server.set_open_handler([this](websocketpp::connection_hdl hdl)
        {
          _connection = hdl;
          WebUsb::singleton().connected(true);
          _connected = true;
        });

        _server.set_close_handler([this](websocketpp::connection_hdl hdl) 
        {
          auto con = _server.get_con_from_hdl(hdl);
          std::cout << "close code: " << con->get_remote_close_code() << " (" 
            << websocketpp::close::status::get_string(con->get_remote_close_code()) 
            << "), close reason: " << con->get_remote_close_reason() << std::endl;;
          _connected = false;
          WebUsb::singleton().connected(false);
        });

        _server.set_message_handler([this](websocketpp::connection_hdl hdl, server::message_ptr msg)
        {
          _messages.push(msg->get_payload());
        });

        _server.set_http_handler(bind(&WebSocket::http, this, _1));

        // Listen on port 9002
        _server.listen(9002);

        // Start the server accept loop
        _server.start_accept();
    } catch (websocketpp::exception const & e) {
        std::cout << e.what() << std::endl;
    } catch (...) {
        std::cout << "other exception" << std::endl;
    }
  }

  void disconnect() {
    if (!_connected) { return; }
    std::cout << "Closing connection" << std::endl;
    _connected = false;
    _server.pause_reading(_connection);
    while (!_messages.empty())
    {
      _messages.pop();
    }
    _server.close(_connection, websocketpp::close::status::going_away, "restarting");
  }

  void run() { _server.poll(); }

  uint32_t readAvailable()
  {
    if (_messages.empty())
    {
      return 0;
    }

    return static_cast<uint32_t>(_messages.front().size());
  }

  uint32_t read(void* buffer, uint32_t bufsize)
  {
    if (_messages.empty())
    {
      return 0;
    }

    auto& msg = _messages.front();
    uint32_t msg_size = static_cast<uint32_t>(msg.size());
    assert(msg_size <= bufsize);
    msg.copy(static_cast<char*>(buffer), msg_size);
    _messages.pop();
    
    return msg_size;
  }

  uint32_t write(const void* buffer, uint32_t bufsize)
  {
    if (!_connected) { return 0; }

    try {
        _server.send(_connection, buffer, bufsize, websocketpp::frame::opcode::binary);
        return bufsize;
    } catch (websocketpp::exception const & e) {
        std::cout << "Send failed because: "
                  << "(" << e.what() << ")" << std::endl;
        return 0;
    }
  }

  uint32_t writeAvailable() { return 64; }

  void http(websocketpp::connection_hdl hdl)
  {
    // Upgrade our connection handle to a full connection_ptr
    server::connection_ptr con = _server.get_con_from_hdl(hdl);
    std::ifstream file;
    std::string filename = con->get_resource();
    std::string response;
    auto query = filename.find('?');
    if (query != std::string::npos)
    {
      filename = filename.substr(0, query);
    }

    if (filename.rfind("/tocata-pedal", 0) == 0)
    {
      filename = filename.substr(sizeof("tocata-pedal"));
    }    
    if (filename.empty() || filename == "/") {
        filename = "/index.html";
    }
    
    filename = _http_root + filename;

    file.open(filename.c_str(), std::ios::in);
    if (!file) {
        // 404 error
        std::stringstream ss;

        ss << "<!doctype html><html><head>"
           << "<title>Error 404 (Resource not found)</title><body>"
           << "<h1>Error 404</h1>"
           << "<p>The requested URL " << filename << " was not found on this server.</p>"
           << "</body></head></html>";

        con->set_body(ss.str());
        con->set_status(websocketpp::http::status_code::not_found);
        return;
    }
    file.seekg(0, std::ios::end);
    response.reserve(file.tellg());
    file.seekg(0, std::ios::beg);
    response.assign((std::istreambuf_iterator<char>(file)),
                    std::istreambuf_iterator<char>());
    con->set_body(response);
    con->set_status(websocketpp::http::status_code::ok);

  }
private:
  server _server;
  std::queue<std::string> _messages;
  websocketpp::connection_hdl _connection;
  std::string _http_root;
  bool _connected = false;
};

static WebSocket ws;
static FILE* flash;

void flash_init() 
{
  flash = fopen("/tmp/tocata_flash", "a");
  assert(flash);  
  fclose(flash);
  flash = fopen("/tmp/tocata_flash", "r+");
  assert(flash);  
  fseek(flash, kFlashSize - 1, SEEK_SET);
  int c = fgetc(flash);
  c = (c == EOF) ? 0xFF : c;
  fseek(flash, kFlashSize - 1, SEEK_SET);
  int new_c = fputc(c, flash);  
  assert(c == new_c);
  fflush(flash);
}

void flash_read(uint32_t flash_offs, void *dst, size_t count) 
{
  assert(flash_offs + count <= kFlashSize);
  fseek(flash, flash_offs, SEEK_SET);
  size_t ret = fread(dst, count, 1, flash);
  assert(ret == 1);
    // printf("flash_read 0x%08X %u bytes\n", flash_offs, (uint32_t)count);
    // memcpy(dst, MemFlash + flash_offs, count);
}

void flash_write(uint32_t flash_offs, const void *data, size_t count) 
{
    assert(flash_offs + count <= kFlashSize);
    assert(flash_offs % kFlashPageSize == 0);
    assert(count % kFlashPageSize == 0);
    uint8_t* buffer = new uint8_t[count];
    flash_read(flash_offs, buffer, count);
    for (size_t i = 0; i < count; ++i)
    {
        buffer[i] &= static_cast<const uint8_t*>(data)[i];
    }
    fseek(flash, flash_offs, SEEK_SET);
    size_t ret = fwrite(buffer, count, 1, flash);
    assert(ret == 1);
    fflush(flash);
    delete[] buffer;

    // printf("flash_write 0x%08X %u bytes\n", flash_offs, (uint32_t)count);
    // for (size_t i = 0; i < count; ++i)
    // {
    //     MemFlash[flash_offs + i] &= static_cast<const uint8_t*>(data)[i];
    // }
}

void flash_erase(uint32_t flash_offs, size_t count) 
{
    assert(flash_offs + count <= kFlashSize);
    assert(flash_offs % kFlashSectorSize == 0);
    assert(count % kFlashSectorSize == 0);

    uint8_t* buffer = new uint8_t[count];
    memset(buffer, 0xFF, count);    
    fseek(flash, flash_offs, SEEK_SET);
    size_t ret = fwrite(buffer, count, 1, flash);
    assert(ret == 1);
    fflush(flash);
    delete[] buffer;
    
    // printf("flash_erase 0x%08X %u bytes\n", flash_offs, (uint32_t)count);
    // memset(MemFlash + flash_offs, 0xFF, count);
}

void usb_init() { 
  ws.init(); 
  flash_init();
  midi.open_virtual_port("Tocata Pedal");
}

void usb_run() { 
  ws.run();
  if (!app.run()) {
    exit(0);
  }
}
uint32_t usb_vendor_available() { return ws.readAvailable(); }
uint32_t usb_vendor_read(void* buffer, uint32_t bufsize) { return ws.read(buffer, bufsize); }
uint32_t usb_vendor_write_available() { return ws.writeAvailable(); }
uint32_t usb_vendor_write(const void* buffer, uint32_t bufsize) { return ws.write(buffer,bufsize); }

void usb_midi_write(const unsigned char* message, size_t size) {
  midi.send_message(message, size);
}

void board_reset() {
  ws.disconnect();
}

}

#endif // HAL_HOST