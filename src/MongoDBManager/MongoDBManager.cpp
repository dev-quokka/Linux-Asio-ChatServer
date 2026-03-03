#include "../MongoDBManager/MongoDBManager.h"

mongocxx::instance& MongodbManager::GlobalInstance() {
    static mongocxx::instance inst{};
    return inst;
}

void MongodbManager::SetIndex() {
    if(!read_client_){
        std::cerr << "MongoDB 클라이언트가 초기화되지 않았습니다. 인덱스를 설정 실패." << "\n";
        return;
    }

    auto db = (*read_client_)[db_name_];
    auto coll = db[coll_name_];

    coll.create_index(make_document(
        kvp("roomKey", 1),
        kvp("cur_ms", -1)
    ));
}

void MongodbManager::Start() {
    if (running_.exchange(true)) return;

    GlobalInstance();
    
    read_client_.emplace(mongocxx::uri{uri_});
    SetIndex();

    worker_ = std::thread([this]() { WorkerLoop(); });
    std::cout << "MongoDB started" << "\n";
}

void MongodbManager::Stop() {
    if (!running_.exchange(false)) return;

    cv_.notify_all();
    if (worker_.joinable()) worker_.join();
}

void MongodbManager::Enqueue(ChatLogItem item) {
    {
        std::lock_guard<std::mutex> lk(mtx_);
        q_.push_back(std::move(item));
    }

    cv_.notify_one();
}

void MongodbManager::WorkerLoop() {
    try {
        write_client_.emplace(mongocxx::uri{uri_});
        auto db = (*write_client_)[db_name_];
        auto coll = db[coll_name_];

        while (running_) {
            ChatLogItem item;
            
            {
                std::unique_lock<std::mutex> lk(mtx_);
                cv_.wait(lk, [&] { return !running_ || !q_.empty(); });

                if (!running_ && q_.empty()) break;

                item = std::move(q_.front());
                q_.pop_front();
            }

            auto doc = make_document(
                kvp("cur_ms", item.cur_ms),
                kvp("sender", item.sender),
                kvp("message", item.message)
            );

            auto res = coll.insert_one(doc.view());
            res;

            std::cout << "Inserted: " << bsoncxx::to_json(doc.view()) << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "[MongoLogger] Worker error: " << e.what() << "\n";
    }
}

const std::vector<ChatLogItem>& GetFriendChatLogs(const std::string& myName, const std::string& friendName) {
    // MongoDB에서 myName과 관련된 친구 채팅 로그를 가져와서 전달

}