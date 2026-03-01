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
        , mongo_("mongodb://127.0.0.1:27017", "chat", "logs")
    {
        mongo_.Start();
        Accept();
        std::cout << "[Server] Listening on 0.0.0.0:" << port << "\n";
    }

private:
    void Accept();

    tcp::acceptor acceptor_;
    SessionManager room_;
    MongodbManager mongo_;
};