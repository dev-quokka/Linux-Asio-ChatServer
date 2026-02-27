#pragma once
#include <boost/asio.hpp>
#include <iostream>
#include <memory>

#include "../SessionManager/Session.h"
#include "../SessionManager/SessionManager.h"

namespace asio = boost::asio;
using asio::ip::tcp;

class Server {
public:
    Server(asio::io_context& io, uint16_t port)
        : acceptor_(io, tcp::endpoint(tcp::v4(), port)), room_(io)
    {
        Accept();
        std::cout << "[Server] Listening on 0.0.0.0:" << port << "\n";
    }

private:
    void Accept() {
        acceptor_.async_accept([this](boost::system::error_code ec, tcp::socket socket) {
            if (!ec) {
                auto s = std::make_shared<Session>(std::move(socket), room_);
                room_.join(s);
                s->Start();
            }
            Accept();
        });
    }

    tcp::acceptor acceptor_;
    SessionManager room_;
};