#pragma once
#include <iostream>
#include <memory>
#include <set>

class Session;

class SessionManager {
public:
    void join(const std::shared_ptr<Session>& s);
    void leave(const std::shared_ptr<Session>& s);
    void broadcast(const std::shared_ptr<const std::string>& msg);

private:
    std::set<std::shared_ptr<Session>> sessions_;
};


