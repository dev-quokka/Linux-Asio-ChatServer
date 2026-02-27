#include "SessionManager.h"
#include "Session.h"

void SessionManager::join(const std::shared_ptr<Session>& s) {
        sessions_.insert(s);
        std::cout << "Client joined" << "\n";
    }

    void SessionManager::leave(const std::shared_ptr<Session>& s) {
        sessions_.erase(s);
        std::cout << "Client left" << "\n";
    }

    void SessionManager::broadcast(const std::shared_ptr<const std::string>& msg) {
        std::cout << "Broadcasting message: " << *msg << "\n";
        for (auto& s : sessions_) s->Deliver(msg);
    }