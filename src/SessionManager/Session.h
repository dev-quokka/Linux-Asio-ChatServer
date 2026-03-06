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
    Session(tcp::socket socket, SessionManager& room, MongodbManager& mongo)
        : socket_(std::move(socket))
        , room_(room)
        , mongo_(mongo) 
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

    const std::string& GetUserName() { return user_name_; }

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
                    
                    bool need_message =
                        parsed.type == ChatType::World ||
                        parsed.type == ChatType::DM ||
                        parsed.type == ChatType::Guild ||
                        parsed.type == ChatType::Party;

                    bool need_target =
                        parsed.type == ChatType::Login ||
                        parsed.type == ChatType::Friend ||
                        parsed.type == ChatType::DM; // DM은 대상 필수

                    if (need_message && parsed.message.empty()) { Read(); return; }
                    if (need_target && parsed.target.empty())   { Read(); return; }

                    if(parsed.type == ChatType::Login) {
                        user_name_ = parsed.target;
                        room_.Login(self, user_name_);
                    }
                    else if (parsed.type == ChatType::Friend) {
                        const std::string friendName = parsed.target;

                        // 콜백 시점에 세션이 이미 종료될 수 있으므로 weak_ptr로 캡처
                        auto self_weak = weak_from_this();

                        mongo_.RequestFriendChatLogs(user_name_, friendName,
                            [self_weak](std::vector<ChatLogItem> friendLogs) {
                                if (auto self = self_weak.lock()) {
                                    asio::post(self->strand_,
                                        [self, logs = std::move(friendLogs)]() {
                                            std::string friendChatLogs;
                                            friendChatLogs.reserve(DM_CHAT_COUNT * (MAX_CHAT_LEN + 32) + 64);

                                            friendChatLogs += "[DM_HISTORY_BEGIN]\n";

                                            for (const auto& log : logs) {
                                                friendChatLogs += "[" + log.sender + "] : " + log.message + "\n";
                                            }

                                            friendChatLogs += "[DM_HISTORY_END]\n";

                                            self->Deliver(std::make_shared<const std::string>(std::move(friendChatLogs)));
                                        }
                                    );
                                }
                            }   
                        );
                    }
                    else if (parsed.type == ChatType::World) {
                        std::string newMsg = "[World][" + user_name_ + "] : " + parsed.message + "\n";

                        auto msg = std::make_shared<const std::string>(std::move(newMsg));
                        room_.broadcast(msg);
                        parsed.target = "main";
                    }
                    else if (parsed.type == ChatType::DM) {
                        std::string newMsg = "[DM][" + user_name_ + "] : " + parsed.message + "\n";
                        auto msg = std::make_shared<const std::string>(std::move(newMsg));
                        Deliver(msg);

                        room_.SendFriend(msg, parsed.target);
                    }

                    // 로그로 남길 채팅인지 판단 (월드, DM, 길드, 파티 채팅만 로그로 남김)
                    bool should_log =
                        parsed.type == ChatType::World ||
                        parsed.type == ChatType::DM ||
                        parsed.type == ChatType::Guild ||
                        parsed.type == ChatType::Party;

                    if (should_log) {
                        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch()).count();

                        ChatLogItem item;
                        item.type   = parsed.type;
                        item.cur_ms = static_cast<int64_t>(now);
                        item.sender = user_name_;
                        item.message = parsed.message;
                        item.target  = parsed.target;

                        mongo_.Enqueue(std::move(item));
                    }
                    
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