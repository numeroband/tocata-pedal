#pragma once

#include <memory>
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
  {}

  void get(const std::string& host, const std::string& port, const std::string& path) {
    auto req = std::make_shared<Request>(_io_context, host, port, path);
    req->start();
  }

private:
  struct Request : std::enable_shared_from_this<Request> {
    tcp::socket socket;
    tcp::resolver resolver;
    asio::streambuf response_buf;
    std::string host;
    std::string port;
    std::string request_str;

    Request(asio::io_context& io, const std::string& h, const std::string& p, const std::string& path)
      : socket{io}
      , resolver{io}
      , host{h}
      , port{p}
    {
      std::ostringstream req;
      req << "GET " << path << " HTTP/1.1\r\n"
          << "Host: " << h << ":" << p << "\r\n"
          << "Connection: close\r\n"
          << "\r\n";
      request_str = req.str();
    }

    void start() {
      auto self = shared_from_this();
      resolver.async_resolve(host, port,
        [self](const asio::error_code& ec, tcp::resolver::results_type results) {
          if (ec) { return log_error("resolve", ec); }
          self->connect(std::move(results));
        });
    }

    void connect(tcp::resolver::results_type results) {
      auto self = shared_from_this();
      asio::async_connect(socket, results,
        [self](const asio::error_code& ec, const tcp::endpoint&) {
          if (ec) { return log_error("connect", ec); }
          self->send();
        });
    }

    void send() {
      auto self = shared_from_this();
      asio::async_write(socket, asio::buffer(request_str),
        [self](const asio::error_code& ec, std::size_t) {
          if (ec) { return log_error("send", ec); }
          self->read_status();
        });
    }

    void read_status() {
      auto self = shared_from_this();
      asio::async_read_until(socket, response_buf, "\r\n",
        [self](const asio::error_code& ec, std::size_t) {
          if (ec) { return log_error("read", ec); }
          std::istream stream(&self->response_buf);
          std::string http_version;
          unsigned int status_code = 0;
          std::string status_message;
          stream >> http_version >> status_code;
          std::getline(stream, status_message);
          if (status_code / 100 != 2) {
            std::fprintf(stderr, "[error] http status %u%s\n", status_code, status_message.c_str());
          }
        });
    }

    static void log_error(const char* where, const asio::error_code& ec) {
      std::fprintf(stderr, "[error] http %s: %s\n", where, ec.message().c_str());
    }
  };

  asio::io_context& _io_context;
};

} // namespace tocata::midi
