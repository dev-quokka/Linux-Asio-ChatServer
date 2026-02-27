#include "SessionManager.h"
#include "Session.h"

void SessionManager::join(const std::shared_ptr<Session>& s) {
    asio::post(strand_, [this, s]() {
        sessions_.insert(s);
        std::cout << "Client joined" <<'n';
    });
}

void SessionManager::leave(const std::shared_ptr<Session>& s) {
    asio::post(strand_, [this, s]() {
        sessions_.erase(s);
        std::cout << "Client left" <<'n';
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