#pragma once
#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>

#include "../MongoDBManager/MongoDBManager.h"
#include "../Test_Friends.h"

namespace asio = boost::asio;

class Session;

class SessionManager {
public:
    using executor_type = asio::io_context::executor_type;
    using strand_type   = asio::strand<executor_type>;

    SessionManager(asio::io_context& io, MongodbManager& mongo)
        : strand_(asio::make_strand(io)), mongo_(mongo) {}

    void Login(const std::shared_ptr<Session>& s, const std::string& userName);
    void join(const std::shared_ptr<Session>& s);
    void leave(const std::shared_ptr<Session>& s);
    
    void broadcast(const std::shared_ptr<const std::string>& msg);
    void SendFriend(const std::shared_ptr<const std::string>& msg, const std::string& friendName);

    strand_type& strand() { return strand_; }

private:
    strand_type strand_;
    MongodbManager& mongo_;

    std::set<std::shared_ptr<Session>> sessions_;

    // 현재 접속중인 유저 세션
    std::unordered_map<std::string, std::shared_ptr<Session>> conn_user_sessions_;
};