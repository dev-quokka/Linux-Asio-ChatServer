#pragma once
#include <boost/asio.hpp>
#include <deque>
#include <memory>
#include <string>

#include "SessionManager.h"

namespace asio = boost::asio;
using asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket, SessionManager& room)
        : socket_(std::move(socket)), room_(room) {}

    void Start() {
        Read();
    }

    void Deliver(const std::shared_ptr<const std::string>& msg) {
        outbox_.push_back(msg);
        if (writing_) return;
        writing_ = true;
        Write();
    }

private:
    void Read() {
        auto self = shared_from_this();

        // '\n'까지 읽기
        asio::async_read_until(socket_, inbuf_, '\n',
            [this, self](boost::system::error_code ec, std::size_t /*bytes*/) {
                if (ec) {
                    room_.leave(self);
                    return;
                }

                std::istream is(&inbuf_);
                std::string line;
                std::getline(is, line);

                // '\r' 제거
                if (!line.empty() && line.back() == '\r')
                    line.pop_back();

                const auto socketNum = socket_.native_handle();
                std::string newMsg =
                    "[소켓번호: " + std::to_string((long long)socketNum) + "] : " + line + "\n";

                auto msg = std::make_shared<const std::string>(std::move(newMsg));
                room_.broadcast(msg);

                Read();
            });
    }

    void Write() {
        auto self = shared_from_this();
        if (outbox_.empty()) { writing_ = false; return; }

        auto msg = outbox_.front();
        asio::async_write(socket_, asio::buffer(*msg),
            [this, self](auto ec, std::size_t) {
                if (ec) { room_.leave(self); return; }
                outbox_.pop_front();
                Write();
            });
    }

    tcp::socket socket_;
    SessionManager& room_;
    boost::asio::streambuf inbuf_;

    std::deque<std::shared_ptr<const std::string>> outbox_;
    bool writing_ = false;
};