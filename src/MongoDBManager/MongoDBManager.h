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

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

struct ChatLogItem {
    int64_t     cur_ms;      // 서버 기준 현재 시간 (클라 기준 시간 X)
    std::string sender;     // 전달자 (소켓 번호)
    std::string message;    // 채팅 내용
};

class MongodbManager{
public:
    MongodbManager(std::string uri, std::string db_name, std::string coll_name)
    : uri_(std::move(uri)), db_name_(std::move(db_name)), coll_name_(std::move(coll_name)) {}

    ~MongodbManager() { Stop(); }

    void Start();
    void Stop();

    void Enqueue(ChatLogItem item);

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
    std::optional<mongocxx::client> client_;
};