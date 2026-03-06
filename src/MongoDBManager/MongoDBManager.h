#pragma once

#include <iostream>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#include <mongocxx/v_noabi/mongocxx/instance.hpp>
#include <mongocxx/v_noabi/mongocxx/client.hpp>
#include <mongocxx/v_noabi/mongocxx/uri.hpp>

#include <bsoncxx/v_noabi/bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/v_noabi/bsoncxx/json.hpp>

#include <bsoncxx/v_noabi/bsoncxx/types.hpp>
#include <bsoncxx/v_noabi/bsoncxx/document/view.hpp>
#include <bsoncxx/v_noabi/bsoncxx/document/element.hpp>
#include <bsoncxx/v_noabi/bsoncxx/builder/stream/document.hpp>

#include "../ChatInfo.h"
#include "../FriendInfo.h"

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

class MongodbManager{
public:
    MongodbManager(std::string uri, std::string db_name, std::string coll_name)
    : uri_(std::move(uri)), db_name_(std::move(db_name)), coll_name_(std::move(coll_name)) {}

    ~MongodbManager() { Stop(); }

    void Start();
    void Stop();
    
    void SetIndex();
    void Enqueue(ChatLogItem item);

    // 친구와 채팅한 기록을 불러와 전달하는 함수
    std::vector<ChatLogItem> GetFriendChatLogs(const std::string& myName, const std::string& friendName);
    
     // 비동기 read 요청 함수 추가
    void RequestFriendChatLogs(const std::string& myName, const std::string& friendName,
        std::function<void(std::vector<ChatLogItem>)> onComplete
    );

    bsoncxx::document::value MakeChatDoc(const ChatLogItem& item, const std::string& roomKey);
    std::string MakeRoomKeyByChatItem(const ChatLogItem& item);
    
private:
    void WriteWorkerLoop();
    void ReadWorkerLoop(); 
    
    std::string uri_;
    std::string db_name_;
    std::string coll_name_;

    // write
    std::mutex write_mtx;
    std::condition_variable write_cv;
    std::deque<ChatLogItem> write_d;
    std::thread write_worker;

    // read
    std::mutex read_mtx;
    std::condition_variable read_cv;
    std::deque<ReadRequest> read_d;
    std::thread read_worker;

    std::atomic<bool> running_{false};

    // mongocxx는 instance가 1개만 존재해야 하므로, static 함수로 관리
    static mongocxx::instance& GlobalInstance();

    // mongocxx::client는 기본 생성자 없음, optional로 감싸서 필요할 때 생성
    // unique_ptr도 가능하지만, optional로 힙 할당 없이 관리
    std::optional<mongocxx::client> read_client_;
    std::optional<mongocxx::client> write_client_;
};

