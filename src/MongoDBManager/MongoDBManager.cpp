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

            std::string roomKey = MakeRoomKeyByChatItem(item);
            
            auto doc = MakeChatDoc(item, roomKey);  
            coll.insert_one(doc.view());

            std::cout << "Inserted: " << bsoncxx::to_json(doc.view()) << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "[MongoLogger] Worker error: " << e.what() << "\n";
    }
}

std::vector<ChatLogItem> MongodbManager::GetFriendChatLogs(const std::string& myName, const std::string& friendName) {

    // 유저가 특정 친구와의 채팅방에 들어왔을때 이전 채팅 로그를 전달하기 위해 호출되는 함수
    // MongoDB에서 myName과 관련된 친구 채팅 로그를 가져와서 전달
    std::vector<ChatLogItem> fcl;
    
    try {
        if (!read_client_) {
            GlobalInstance();
            read_client_.emplace(mongocxx::uri{uri_});
        }

        auto db   = (*read_client_)[db_name_];
        auto coll = db[coll_name_];

        const std::string roomKey = MakeDmRoomKey(myName, friendName);
        auto filter = make_document(kvp("roomKey", roomKey));

        mongocxx::options::find opt;
        opt.sort(make_document(kvp("cur_ms", -1)));              // 최신순
        opt.limit(static_cast<std::int64_t>(DM_CHAT_COUNT));     // 최근 DM (DM_CHAT_COUNT) 설정 개수 까지 가져오기

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

        // 최신순으로 가져왔으니 화면/전송용으로 reverse 정렬
        std::reverse(fcl.begin(), fcl.end());

    } catch (const std::exception& e) {
        std::cerr << "[MongoDB] GetFriendChatLogs error: " << e.what() << "\n";
    }

    return fcl;
}