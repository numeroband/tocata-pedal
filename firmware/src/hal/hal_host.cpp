#include "hal.h"
#ifndef HAL_PICO

#define ASIO_STANDALONE
 
#include <web_usb.h>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp> 
#include <functional>
#include <queue>
#include <display_sim.h>

namespace tocata {

uint8_t MemFlash[2 * 1024 * 1024];

static DisplaySim display;
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

class WebSocket
{
public:
  typedef websocketpp::server<websocketpp::config::asio> server;

  void init()
  {
    try {
        // Set logging settings
        _server.clear_access_channels(websocketpp::log::alevel::all);
        _server.set_access_channels(websocketpp::log::alevel::connect);

        _server.init_asio();

        _server.set_open_handler([this](websocketpp::connection_hdl hdl)
        {
          _connection = hdl;
          WebUsb::singleton().connected(true);
        });

        _server.set_message_handler([this](websocketpp::connection_hdl hdl, server::message_ptr msg)
        {
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

void usb_init() { ws.init(); flash_init(); }
void usb_run() { 
  ws.run(); 
  // display.refresh(); 
}
uint32_t usb_vendor_available() { return ws.readAvailable(); }
uint32_t usb_vendor_read(void* buffer, uint32_t bufsize) { return ws.read(buffer, bufsize); }
uint32_t usb_vendor_write_available() { return ws.writeAvailable(); }
uint32_t usb_vendor_write(const void* buffer, uint32_t bufsize) { return ws.write(buffer,bufsize); }

}

#endif // HAL_PICO