#pragma once
#include <boost/asio.hpp>
#include <array>
#include <iostream>
#include <memory>
#include <set>
#include <string>

#include "../SessionManager/Session.h"

namespace asio = boost::asio;
using asio::ip::tcp;

class Server {
public:
    Server(asio::io_context& io, uint16_t port)
        : acceptor_(io, tcp::endpoint(tcp::v4(), port)) {
        Accept();
    }

private:
    void Accept() {
        acceptor_.async_accept([this](auto ec, tcp::socket socket) {
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