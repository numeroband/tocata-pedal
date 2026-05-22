#pragma once

#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE
#endif
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <asio/steady_timer.hpp>
#include <string>
#include <cstdio>
#include <chrono>

namespace tocata::midi {

class PlayerConnection {
    using WsClient = websocketpp::client<websocketpp::config::asio_client>;

public:
    PlayerConnection(asio::io_context& io_context, std::string uri)
        : _io_context{io_context}
        , _uri{std::move(uri)}
        , _reconnect_timer{io_context}
    {
        _client.init_asio(&io_context);
        _client.clear_access_channels(websocketpp::log::alevel::all);
        _client.clear_error_channels(websocketpp::log::elevel::all);

        _client.set_open_handler([this](websocketpp::connection_hdl hdl) { on_open(hdl); });
        _client.set_fail_handler([this](websocketpp::connection_hdl hdl) { on_fail(hdl); });
        _client.set_close_handler([this](websocketpp::connection_hdl hdl) { on_close(hdl); });

        connect();
    }

    void setMuted(bool muted) {
        if (muted == _muted) return;
        _muted = muted;
        if (_connected) send_state();
    }

    void setBackup(bool backup) {
        if (backup == _backup) return;
        _backup = backup;
        if (_connected) send_state();
    }

private:
    void connect() {
        websocketpp::lib::error_code ec;
        auto con = _client.get_connection(_uri, ec);
        if (ec) {
            fprintf(stderr, "[player] connect error: %s\n", ec.message().c_str());
            schedule_reconnect();
            return;
        }
        _client.connect(con);
    }

    void schedule_reconnect() {
        _reconnect_timer.expires_after(std::chrono::seconds(1));
        _reconnect_timer.async_wait([this](const asio::error_code& ec) {
            if (!ec) connect();
        });
    }

    void send_state() {
        char buf[64];
        snprintf(buf, sizeof(buf), "{\"type\":\"LiveMode\",\"muted\":%s,\"backup\":%s}",
            _muted ? "true" : "false", _backup ? "true" : "false");
        websocketpp::lib::error_code ec;
        _client.send(_hdl, buf, websocketpp::frame::opcode::text, ec);
        if (ec) {
            fprintf(stderr, "[player] send error: %s\n", ec.message().c_str());
        }
    }

    void on_open(websocketpp::connection_hdl hdl) {
        _hdl = hdl;
        _connected = true;
        send_state();
    }

    void on_fail(websocketpp::connection_hdl) {
        _connected = false;
        schedule_reconnect();
    }

    void on_close(websocketpp::connection_hdl) {
        _connected = false;
        asio::post(_io_context, [this]() { connect(); });
    }

    asio::io_context& _io_context;
    std::string _uri;
    WsClient _client;
    websocketpp::connection_hdl _hdl;
    asio::steady_timer _reconnect_timer;
    bool _connected = false;
    bool _muted = false;
    bool _backup = false;
};

} // namespace tocata::midi
