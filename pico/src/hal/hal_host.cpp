#include "hal.h"
#ifndef HAL_PICO

#define ASIO_STANDALONE
 
#include <web_usb.h>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp> 
#include <functional>
#include <queue>

namespace tocata {

uint8_t MemFlash[2 * 1024 * 1024];

class WebSocket
{
public:
  typedef websocketpp::server<websocketpp::config::asio> server;

  void init()
  {
    try {
        // Set logging settings
        _server.set_access_channels(websocketpp::log::alevel::all);
        _server.clear_access_channels(websocketpp::log::alevel::frame_payload);

        _server.init_asio();

        _server.set_open_handler([this](websocketpp::connection_hdl hdl)
        {
          _connection = hdl;
          WebUsb::singleton().connected(true);
        });

        _server.set_message_handler([this](websocketpp::connection_hdl hdl, server::message_ptr msg)
        {
          printf("queuing %u bytes\n", (uint32_t)msg->get_payload().length());
          _messages.push(msg->get_payload());
        });

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
    try {
        printf("sending %u bytes\n", bufsize);
        _server.send(_connection, buffer, bufsize, websocketpp::frame::opcode::binary);
        return bufsize;
    } catch (websocketpp::exception const & e) {
        std::cout << "Send failed because: "
                  << "(" << e.what() << ")" << std::endl;
        return 0;
    }
  }

  uint32_t writeAvailable() { return 64; }

private:
  server _server;
  std::queue<std::string> _messages;
  websocketpp::connection_hdl _connection;
};

static WebSocket ws;

void usb_init() { ws.init(); }
void usb_run() { ws.run(); }
uint32_t usb_vendor_available() { return ws.readAvailable(); }
uint32_t usb_vendor_read(void* buffer, uint32_t bufsize) { return ws.read(buffer, bufsize); }
uint32_t usb_vendor_write_available() { return ws.writeAvailable(); }
uint32_t usb_vendor_write(const void* buffer, uint32_t bufsize) { return ws.write(buffer,bufsize); }

}

#endif // HAL_PICO