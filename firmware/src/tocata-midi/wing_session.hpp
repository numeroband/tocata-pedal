#pragma once

#include "wing_discovery.hpp"
#include "wing_parser.hpp"
#define ASIO_STANDALONE
#include "asio.hpp"

using asio::ip::tcp;
using namespace std::chrono_literals;

namespace tocata::wing {

namespace node {
  constexpr WingParser::Hash IO_ALTSW = 0xF9CE1576;
};

class WingSession {
public:
  using Callback = std::function<void(bool connected)>;

  WingSession(asio::io_context& io_context)
    : _discovery{io_context}
    , _socket{io_context}
    , _timer{io_context}
    , _resolver{io_context}
  {
    _discovery.setCallback([this](bool connected, const std::string& address) {
      if (!connected) {
        if (_callback) {
          _callback(false);
        }
        return;
      }

      _resolver.async_resolve(
        address.c_str(), PORT,
        [this](const asio::error_code& ec, tcp::resolver::results_type results) {
          if (ec) { return log_error("resolve", ec); }
          connect(std::move(results));
        });
    });
  }

  void setParserCallback(WingParser::Callback callback) {
    _parser.setCallback(callback);
  }

  void setSessionCallback(Callback callback) {
    _callback = callback;
  }

  void start() {
    _discovery.start();
  }

  void sendRequest(uint32_t hash, std::function<void()> callback) {
    struct Request {
      uint8_t node_hash = 0xd7;
      uint32_t hash;
      uint8_t request = 0xdc;
      Request(uint32_t hash) : hash{asio::detail::socket_ops::host_to_network_long(hash)} {}
    } __attribute((packed));

    struct RequestWithChannel {
      uint8_t channel_change = 0xdf;
      uint8_t channel = 0xd1;
      Request request;
      RequestWithChannel(uint32_t hash) : request{hash} {}
    } __attribute((packed));

    RequestWithChannel request{hash};
    void* buffer = _channel_sent ? static_cast<void*>(&request.request) : static_cast<void*>(&request);
    size_t size = _channel_sent ? sizeof(request.request) : sizeof(request);
    _channel_sent = true;

    asio::async_write(
      _socket,
      asio::buffer(buffer, size),
      [callback](const asio::error_code& ec, std::size_t bytes) {
        if (ec) { return log_error("write", ec); }
        callback();
      });
  }

private:
  static constexpr const char* PORT = "2222";

  void connect(tcp::resolver::results_type results)
  {
    asio::async_connect(
      _socket, results,
      [this](const asio::error_code& ec, const tcp::endpoint& ep) {
        if (ec) { return log_error("connect", ec); }
        // Disable Nagle's algorithm so every send is flushed immediately.
        asio::error_code opt_ec;
        _socket.set_option(tcp::no_delay(true), opt_ec);
        if (opt_ec) { return log_error("set_option(no_delay)", opt_ec); }

        if (_callback) { _callback(true); }

        read();
        sendAndSchedule();
      });
  }

  void read()
  {
    _socket.async_read_some(
      asio::buffer(&_read_buffer, sizeof(_read_buffer)),
      [this](const asio::error_code& ec, std::size_t /*n*/) {
        if (ec) { return log_error("read", ec); }
        _parser.parse(_read_buffer);
        read();   // re-arm immediately
      });
  }

  void sendAndSchedule() {
    sendRequest(0xf9ce1576, [this]() { 
      schedule_send(); 
    });
  }

  void schedule_send()
  {
    _timer.expires_after(6s);
    _timer.async_wait([this](const asio::error_code& ec) {
      if (ec) { return log_error("timer", ec); }
      sendAndSchedule();
    });
  }

  static void log_error(const char* where, const asio::error_code& ec)
  {
    std::fprintf(stderr, "[error] %s: %s\n", where, ec.message().c_str());
  }

  WingDiscovery _discovery;
  tcp::socket _socket;
  asio::steady_timer _timer;
  tcp::resolver _resolver;
  uint8_t _read_buffer;
  WingParser _parser;
  bool _channel_sent = false;
  Callback _callback;
};

}