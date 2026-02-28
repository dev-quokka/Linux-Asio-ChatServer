#include "ChatServer/ChatServer.h"
#include <thread>
#include <vector>

// MongoDB C++ Driver (테스트용)
#include <mongocxx/v_noabi/mongocxx/instance.hpp>
#include <mongocxx/v_noabi/mongocxx/client.hpp>
#include <mongocxx/v_noabi/mongocxx/uri.hpp>
#include <bsoncxx/v_noabi/bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/v_noabi/bsoncxx/json.hpp>

int main(int argc, char* argv[]) {
    try {
        uint16_t port = 9000;
        if (argc >= 2) {
            port = static_cast<uint16_t>(std::stoi(argv[1]));
        }

        // MongoDB C++ Driver (테스트용)
        {
            mongocxx::instance inst{};
            mongocxx::client conn{ mongocxx::uri{"mongodb://localhost:27017"} };

            auto db = conn["chat"];
            auto collection = db["test"];

            bsoncxx::builder::stream::document document{};
            document << "msg" << "hello from asio server"<< '\n';

            auto result = collection.insert_one(document.view());

            if (result) {
                std::cout << "Insert 테스트 성공!" << '\n';
            } else {
                std::cout << "Insert 테스트 실패." << '\n';
            }
        }

        asio::io_context io;
        Server server(io, port);

        // CPU 코어 기반으로 적당한 수의 워커스레드 생성
        unsigned int n = std::thread::hardware_concurrency();
        if (n == 0) n = 4;
        
        n = std::min(n, 4u);

        std::vector<std::thread> workers;
        workers.reserve(n);

        for (unsigned int i = 0; i < n; ++i) {
            workers.emplace_back([&io]() { io.run(); });
        }

        for (auto& t : workers) t.join();

    } catch (const std::exception& e) {
        std::cerr << "[Fatal] " << e.what() << "\n";
        return 1;
    }
    return 0;
}