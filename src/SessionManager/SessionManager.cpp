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
        std::cout << "Client joined" << '\n';
    });
}

void SessionManager::leave(const std::shared_ptr<Session>& s) {
    asio::post(strand_, [this, s]() {
        conn_user_sessions_.erase(s->GetUserName());
        sessions_.erase(s);
        std::cout << "Client left" << '\n';
    });
}

void SessionManager::broadcast(const std::shared_ptr<const std::string>& msg) {
    asio::post(strand_, [this, msg]() {

        // tempSession을 만들어서 현재 세션 목록을 복사한 후, 그 복사본을 순회하면서 메시지 전달
        // 브로드캐스트 중에 세션이 join/leave 하더라도 안전하게 처리하기 위함
        auto tempSession = sessions_;
        
        std::cout << "Broadcasting message: " << *msg << "\n";
        for (auto& s : tempSession) {
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