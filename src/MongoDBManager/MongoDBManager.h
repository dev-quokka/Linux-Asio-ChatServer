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
    const std::vector<ChatLogItem>& GetFriendChatLogs(const std::string& myName, const std::string& friendName);

private:
    void WorkerLoop();

    std::string uri_;
    std::string db_name_;
    std::string coll_name_;

    std::mutex mtx_;
    std::condition_variable cv_;
    std::deque<ChatLogItem> q_;
    std::atomic<bool> running_{false};
    std::thread worker_;

    // mongocxx는 instance가 1개만 존재해야 하므로, static 함수로 관리
    static mongocxx::instance& GlobalInstance();

    // mongocxx::client는 기본 생성자 없음, optional로 감싸서 필요할 때 생성
    // unique_ptr도 가능하지만, optional로 힙 할당 없이 관리
    std::optional<mongocxx::client> read_client_;
    std::optional<mongocxx::client> write_client_;
};

