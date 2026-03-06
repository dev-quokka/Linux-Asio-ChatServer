#include "../MongoDBManager/MongoDBManager.h"

mongocxx::instance& MongodbManager::GlobalInstance() {
    static mongocxx::instance inst{};
    return inst;
}

bsoncxx::document::value MongodbManager::MakeChatDoc(const ChatLogItem& item, const std::string& roomKey){
    bsoncxx::builder::basic::document d{};

    d.append(
        kvp("roomKey", roomKey),
        kvp("cur_ms",  item.cur_ms),
        kvp("sender",  item.sender),
        kvp("message", item.message),
        kvp("target", item.target)
    );

    return d.extract();
}

std::string MongodbManager::MakeRoomKeyByChatItem(const ChatLogItem& item) {
    if (item.target == "") { // 타겟 없을 때
        std::cerr << "타겟 or ID 없음. sender=" << item.sender << "\n";
        return "";
    }
    else {
        switch (item.type) {
            case ChatType::World:
                return MakeWorldRoomKey(item.target);
            case ChatType::DM:
                return MakeDmRoomKey(item.sender, item.target);
            case ChatType::Guild:
                return MakeGuildRoomKey(item.target);
            case ChatType::Party:
                return MakePartyRoomKey(item.target);
            default:
                return "";
        }
    }
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

void MongodbManager::RequestFriendChatLogs(const std::string& myName,const std::string& friendName,
    std::function<void(std::vector<ChatLogItem>)> onComplete)
{
    {
        std::lock_guard<std::mutex> lk(read_mtx);
        read_d.push_back(ReadRequest{myName, friendName, std::move(onComplete)});
    }

    read_cv.notify_one();
}

void MongodbManager::Start() {
    if (running_.exchange(true)) return;

    GlobalInstance();

    // read client는 read worker에서만 사용하지만,
    // 인덱스 설정을 위해 여기서 먼저 생성해도 괜찮음
    read_client_.emplace(mongocxx::uri{uri_});
    SetIndex();

    write_worker = std::thread([this]() { WriteWorkerLoop(); });
    read_worker = std::thread([this]() { ReadWorkerLoop(); });

    std::cout << "MongoDB started" << "\n";
}

void MongodbManager::Stop() {
    if (!running_.exchange(false)) return;

    write_cv.notify_all();
    read_cv.notify_all();

    if (write_worker.joinable()) write_worker.join();
    if (read_worker.joinable()) read_worker.join();
}

void MongodbManager::Enqueue(ChatLogItem item) {
    {
        std::lock_guard<std::mutex> lk(write_mtx);
        write_d.push_back(std::move(item));
    }

    write_cv.notify_one();
}

void MongodbManager::WriteWorkerLoop() {
    try {
        write_client_.emplace(mongocxx::uri{uri_});
        auto db = (*write_client_)[db_name_];
        auto coll = db[coll_name_];

        while (running_) {
            ChatLogItem item;
            
            {
                std::unique_lock<std::mutex> lk(write_mtx);
                write_cv.wait(lk, [&] { return !running_ || !write_d.empty(); });

                if (!running_ && write_d.empty()) break;

                item = std::move(write_d.front());
                write_d.pop_front();
            }

            std::string roomKey = MakeRoomKeyByChatItem(item);
            
            auto doc = MakeChatDoc(item, roomKey);  
            coll.insert_one(doc.view());
        }
    } catch (const std::exception& e) {
        std::cerr << "[MongoLogger] Worker error: " << e.what() << "\n";
    }
}

void MongodbManager::ReadWorkerLoop() {
    try {
        if (!read_client_) {
            read_client_.emplace(mongocxx::uri{uri_});
        }

        while (running_) {
            ReadRequest req;

            {
                std::unique_lock<std::mutex> lk(read_mtx);
                read_cv.wait(lk, [&] { return !running_ || !read_d.empty(); });

                if (!running_ && read_d.empty()) break;

                req = std::move(read_d.front());
                read_d.pop_front();
            }

            auto logs = GetFriendChatLogs(req.myName, req.friendName);

            if (req.onComplete) {
                req.onComplete(std::move(logs));
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "[MongoDB] ReadWorker error: " << e.what() << "\n";
    }
}

std::vector<ChatLogItem> MongodbManager::GetFriendChatLogs(const std::string& myName, const std::string& friendName) {
    std::vector<ChatLogItem> fcl;

    try {
        if (!read_client_) {
            read_client_.emplace(mongocxx::uri{uri_});
        }

        auto db   = (*read_client_)[db_name_];
        auto coll = db[coll_name_];

        const std::string roomKey = MakeDmRoomKey(myName, friendName);
        auto filter = make_document(kvp("roomKey", roomKey));

        mongocxx::options::find opt;
        opt.sort(make_document(kvp("cur_ms", -1)));
        opt.limit(static_cast<std::int64_t>(DM_CHAT_COUNT));

        auto colls = coll.find(filter.view(), opt);

        for (const auto& tc : colls) {
            ChatLogItem item{};
            item.type = ChatType::DM;
            item.target = friendName;

            if (auto v = tc["cur_ms"]; v) {
                item.cur_ms = v.get_int64().value;
            }

            if (auto v = tc["sender"]; v && v.type() == bsoncxx::type::k_string)
                item.sender = std::string(v.get_string().value);

            if (auto v = tc["message"]; v && v.type() == bsoncxx::type::k_string)
                item.message = std::string(v.get_string().value);

            fcl.push_back(std::move(item));
        }

        std::reverse(fcl.begin(), fcl.end());
    }
    catch (const std::exception& e) {
        std::cerr << "[MongoDB] GetFriendChatLogs error: " << e.what() << "\n";
    }

    return fcl;
}