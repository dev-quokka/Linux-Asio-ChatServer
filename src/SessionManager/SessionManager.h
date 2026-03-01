#pragma once
#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <set>
#include <string>

#include "../MongoDBManager/MongoDBManager.h"

class Session;

namespace asio = boost::asio;

class SessionManager {
public:
    using executor_type = asio::io_context::executor_type;
    using strand_type   = asio::strand<executor_type>;

    SessionManager(asio::io_context& io)
        : strand_(asio::make_strand(io)) {}

    void join(const std::shared_ptr<Session>& s);
    void leave(const std::shared_ptr<Session>& s);
    void broadcast(const std::shared_ptr<const std::string>& msg);

    strand_type& strand() { return strand_; }
private:
    strand_type strand_;
    std::set<std::shared_ptr<Session>> sessions_;
};