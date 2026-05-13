#include "SessionManager.h"
#include "Session.h"

void SessionManager::Login(const std::shared_ptr<Session>& s, const std::string& userName) {
    asio::post(strand_, [this, s]() {
        conn_user_sessions_[s->GetUserName()] = s;
    });
}

void SessionManager::join(const std::shared_ptr<Session>& s) {
    asio::post(strand_, [this, s]() {
        sessions_.insert(s);
    });
}

void SessionManager::leave(const std::shared_ptr<Session>& s) {
    asio::post(strand_, [this, s]() {
        conn_user_sessions_.erase(s->GetUserName());
        sessions_.erase(s);
    });
}

void SessionManager::broadcast(const std::shared_ptr<const std::string>& msg) {
    asio::post(strand_, [this, msg]() {
        for (auto& s : sessions_) {
            s->Deliver(msg);
        }
    });
}

void SessionManager::SendFriend(const std::shared_ptr<const std::string>& msg, const std::string& friendName) {
    asio::post(strand_, [this, msg, friendName]() {
        auto it = conn_user_sessions_.find(friendName);

        // 친구가 접속 중이면 해당 세션에도 메시지 전달
        if (it != conn_user_sessions_.end()) {
            it->second->Deliver(msg);
        }

    });
}