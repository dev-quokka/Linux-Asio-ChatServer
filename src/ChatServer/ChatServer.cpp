#include "../ChatServer/ChatServer.h"

void Server::Accept() {
    acceptor_.async_accept([this](boost::system::error_code ec, tcp::socket socket) {
        if (!ec) {
            auto s = std::make_shared<Session>(std::move(socket), room_, mongo_);
            room_.join(s);
            s->Start();
        }
        Accept();
    });
}