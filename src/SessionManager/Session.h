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
    Session(tcp::socket socket, SessionManager& room, MongodbManager& mongo, std::string userName)
        : socket_(std::move(socket))
        , room_(room)
        , mongo_(mongo)
        , user_name_(userName)   
        , strand_(asio::make_strand(socket_.get_executor())) {}

    void Start() {
        asio::dispatch(strand_, [self = shared_from_this()]() {
            self->Read();
        });
    }

    void Deliver(const std::shared_ptr<const std::string>& msg) {
        asio::post(strand_, [self = shared_from_this(), msg]() {
            self->outbox_.push_back(msg);
            if (self->writing_) return;
            self->writing_ = true;
            self->Write();
        });
    }

private:
    void Read() {
        auto self = shared_from_this();

        asio::async_read_until(socket_, inbuf_, '\n',
            asio::bind_executor(strand_,
                [this, self](boost::system::error_code ec, std::size_t) {
                    if (ec) {
                        room_.leave(self);
                        return;
                    }

                    std::istream is(&inbuf_);
                    std::string line;
                    std::getline(is, line);

                    if (!line.empty() && line.back() == '\r') line.pop_back();

                    auto parsed = ParseLine(line);
                    if (parsed.message.empty()) {
                        Read();
                        return;
                    }

                    const auto socketNum = socket_.native_handle();
                    auto now = std::chrono::time_point_cast<std::chrono::milliseconds>(
                               std::chrono::system_clock::now())
                               .time_since_epoch()
                               .count();

                    if(parsed.type == ChatType::Login) {
                        std::string newMsg = "[유저ID: " + user_name_ + "] 님이 로그인했습니다.\n";
                        room_.Login(self, user_name_);
                    }
                    else if (parsed.type == ChatType::Friend) {
                        std::string newMsg = "[유저ID: " + user_name_ + "] 님이 [친구ID: " + parsed.target.value_or("") + "] 채팅방에 입장했습니다.\n";
                        
                        const auto friendLogs = mongo_.GetFriendChatLogs(user_name_, parsed.target.value_or(""));

                        
                    }
                    else if (parsed.type == ChatType::World) {
                        std::string newMsg = "[유저ID: " + user_name_ + "] : " + parsed.message + "\n";

                        auto msg = std::make_shared<const std::string>(std::move(newMsg));
                        room_.broadcast(msg);
                    }
                    else if (parsed.type == ChatType::DM) {
                        std::string newMsg = "[친구ID: " + user_name_ + "] : " + parsed.message + "\n";

                        auto msg = std::make_shared<const std::string>(std::move(newMsg));
                        room_.SendFriend(msg, parsed.target.value_or(""));
                    }

                    // MongoDB에 채팅 로그 저장
                    auto now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count();

                    ChatLogItem item;
                    item.type = parsed.type;
                    item.cur_ms = static_cast<int64_t>(now);
                    item.sender = user_name_;
                    item.message = line;
                    item.target = parsed.target; 

                    mongo_.Enqueue(std::move(item));

                    Read();
                }
            )
        );
    }

    void Write() {
        auto self = shared_from_this();
        if (outbox_.empty()) { writing_ = false; return; }

        auto msg = outbox_.front();
        asio::async_write(socket_, asio::buffer(*msg),
            asio::bind_executor(strand_,
                [this, self](boost::system::error_code ec, std::size_t) {
                    if (ec) {
                        room_.leave(self);
                        return;
                    }
                    outbox_.pop_front();
                    Write();
                }
            )
        );
    }

    tcp::socket socket_;
    SessionManager& room_;
    MongodbManager& mongo_;

    // 세션 전용 strand
    asio::strand<tcp::socket::executor_type> strand_;

    std::string user_name_;

    boost::asio::streambuf inbuf_;
    std::deque<std::shared_ptr<const std::string>> outbox_;
    bool writing_ = false;
};