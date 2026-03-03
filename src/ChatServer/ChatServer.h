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
        : acceptor_(io, tcp::endpoint(tcp::v4(), port))
        , mongo_("mongodb://127.0.0.1:27017", "chat", "logs")
        , room_(io, mongo_)
    {
        mongo_.Start();
        Accept();
        std::cout << "[Server] Listening on 0.0.0.0:" << port << "\n";
    }

    void Init();

private:
    void Accept();

    tcp::acceptor acceptor_;
    MongodbManager mongo_;
    SessionManager room_;
};