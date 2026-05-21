#pragma once

#include <cctype>
#include <queue>
#include <sstream>
#include <string>
#define ASIO_STANDALONE
#include "asio.hpp"

using asio::ip::tcp;

namespace tocata::midi {

class HttpClient {
public:
  HttpClient(asio::io_context& io_context)
    : _io_context{io_context}
    , _socket{io_context}
    , _resolver{io_context}
  {}

  void get(const std::string& host, const std::string& port, const std::string& path) {
    _queue.push({host, port, path});
    if (!_busy) {
      start_next();
    }
  }

private:
  struct Req { std::string host, port, path; };

  void start_next() {
    _busy = true;
    const Req& req = _queue.front();

    std::ostringstream r;
    r << "GET " << req.path << " HTTP/1.1\r\n"
      << "Host: " << req.host << ":" << req.port << "\r\n"
      << "Connection: keep-alive\r\n"
      << "\r\n";
    _request_str = r.str();

    if (_socket.is_open() && req.host == _connected_host && req.port == _connected_port) {
      send();
    } else {
      if (_socket.is_open()) {
        asio::error_code ec;
        _socket.close(ec);
      }
      resolve();
    }
  }

  void resolve() {
    const Req& req = _queue.front();
    _resolver.async_resolve(req.host, req.port,
      [this](const asio::error_code& ec, tcp::resolver::results_type results) {
        if (ec) { log_error("resolve", ec); return on_done(false); }
        connect(std::move(results));
      });
  }

  void connect(tcp::resolver::results_type results) {
    asio::async_connect(_socket, results,
      [this](const asio::error_code& ec, const tcp::endpoint&) {
        if (ec) { log_error("connect", ec); return on_done(false); }
        _connected_host = _queue.front().host;
        _connected_port = _queue.front().port;
        send();
      });
  }

  void send() {
    asio::async_write(_socket, asio::buffer(_request_str),
      [this](const asio::error_code& ec, std::size_t) {
        if (ec) { log_error("send", ec); return on_done(false); }
        read_status();
      });
  }

  void read_status() {
    asio::async_read_until(_socket, _response_buf, "\r\n",
      [this](const asio::error_code& ec, std::size_t) {
        if (ec) { log_error("read_status", ec); return on_done(false); }
        std::istream stream(&_response_buf);
        std::string http_version;
        unsigned int status_code = 0;
        std::string status_message;
        stream >> http_version >> status_code;
        std::getline(stream, status_message);
        if (status_code / 100 != 2) {
          std::fprintf(stderr, "[error] http status %u%s\n", status_code, status_message.c_str());
        }
        read_headers();
      });
  }

  void read_headers() {
    asio::async_read_until(_socket, _response_buf, "\r\n\r\n",
      [this](const asio::error_code& ec, std::size_t) {
        if (ec) { log_error("read_headers", ec); return on_done(false); }
        std::istream stream(&_response_buf);
        std::string line;
        std::size_t content_length = 0;
        while (std::getline(stream, line) && line != "\r" && !line.empty()) {
          // Case-insensitive search for content-length header
          std::string lower = line;
          for (char& c : lower) { c = char(std::tolower(unsigned(c))); }
          const std::string prefix = "content-length: ";
          if (lower.substr(0, prefix.size()) == prefix) {
            content_length = std::stoul(line.substr(prefix.size()));
          }
        }
        // Any bytes still in the buffer are body bytes read ahead by async_read_until.
        std::size_t buffered = _response_buf.size();
        _response_buf.consume(buffered);  // discard already-buffered body bytes
        std::size_t remaining = (content_length > buffered) ? content_length - buffered : 0;
        if (remaining > 0) {
          read_body(remaining);
        } else {
          on_done(true);
        }
      });
  }

  void read_body(std::size_t remaining) {
    asio::async_read(_socket, _response_buf, asio::transfer_exactly(remaining),
      [this](const asio::error_code& ec, std::size_t) {
        if (ec) { log_error("read_body", ec); return on_done(false); }
        _response_buf.consume(_response_buf.size());
        on_done(true);
      });
  }

  void on_done(bool reusable) {
    _queue.pop();
    _busy = false;
    if (!reusable && _socket.is_open()) {
      asio::error_code ec;
      _socket.close(ec);
      _connected_host.clear();
      _connected_port.clear();
    }
    if (!_queue.empty()) {
      start_next();
    }
  }

  static void log_error(const char* where, const asio::error_code& ec) {
    std::fprintf(stderr, "[error] http %s: %s\n", where, ec.message().c_str());
  }

  asio::io_context& _io_context;
  tcp::socket _socket;
  tcp::resolver _resolver;
  asio::streambuf _response_buf;
  std::string _request_str;
  std::string _connected_host;
  std::string _connected_port;
  bool _busy = false;
  std::queue<Req> _queue;
};

} // namespace tocata::midi
