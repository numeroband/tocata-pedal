#pragma once

#define ASIO_STANDALONE
#include <asio.hpp>
#include <iostream>
#include <array>
#include <functional>

using asio::ip::udp;
using namespace std::chrono_literals;

namespace tocata::wing {

class WingDiscovery {
public:
  using Callback = std::function<void(bool connected, const std::string& address)>;
  WingDiscovery(asio::io_context& io)
    : socket_(io, udp::endpoint(udp::v4(), PORT))
    , timer_(io)
  {
    // Allow broadcast and reuse address
    socket_.set_option(asio::socket_base::broadcast(true));
    socket_.set_option(asio::socket_base::reuse_address(true));

    broadcast_endpoint_ = udp::endpoint(
      asio::ip::address_v4::broadcast(), PORT
    );
  }

  void setCallback(Callback callback) {
    callback_ = callback;
  }

  void start() {
    do_send();
  }

private:
  static constexpr uint16_t PORT = 2222;
  static constexpr auto BROADCAST_IP = "255.255.255.255";
  static constexpr auto MESSAGE = "WING?";
  static constexpr auto RESPONSE = "WING,";
  static constexpr size_t BUFFER_SIZE = 1024;

  void start_timeout() {
    timer_.expires_after(6s);
    timer_.async_wait([this](const asio::error_code& ec) {
      if (ec == asio::error::operation_aborted) {
        return;
      }
      socket_.close();
      if (callback_) {
        std::cout << "timer " << (ec ? ec.message() : std::string{"Timeout discovering WING"}) << std::endl;
        callback_(false, ec ? ec.message() : std::string{"Timeout discovering WING"});
      }
    });
  }

  void do_send() {
    socket_.async_send_to(
      asio::buffer(MESSAGE, std::strlen(MESSAGE)),
      broadcast_endpoint_,
      [this](const asio::error_code& ec, std::size_t bytes_sent) {
        if (!ec) {
          do_receive();
          start_timeout();
        } else if (callback_) {
          callback_(false, ec.message());
          socket_.close();
        }
      }
    );
  }

  void do_receive() {
    socket_.async_receive_from(
      asio::buffer(recv_buffer_),
      sender_endpoint_,
      [this](const asio::error_code& ec, std::size_t bytes_received) {
        if (!ec) {
          std::string msg(recv_buffer_.data(), bytes_received);
          if (!msg.starts_with(RESPONSE)) {
            // Keep listening for more responses
            do_receive();
            return;
          }

          if (callback_) {
            callback_(true, sender_endpoint_.address().to_string());
          }
        } else if (ec != asio::error::operation_aborted) {
          if (callback_) {
            callback_(false, ec.message());
          }
        }
        timer_.cancel();
        socket_.close();
      }
    );
  }

  udp::socket socket_;
  udp::endpoint broadcast_endpoint_;
  udp::endpoint sender_endpoint_;
  asio::steady_timer timer_;
  std::array<char, BUFFER_SIZE> recv_buffer_;
  Callback callback_;
};

}

