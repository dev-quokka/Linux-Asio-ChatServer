cmake_minimum_required(VERSION 3.16)
project(chat_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

find_package(mongocxx CONFIG REQUIRED)
find_package(bsoncxx  CONFIG REQUIRED)

add_executable(chat_server
    src/main.cpp
    src/ChatServer/ChatServer.cpp
    src/SessionManager/SessionManager.cpp
    src/MongoDBManager/MongoDBManager.cpp
)

target_link_libraries(chat_server
  PRIVATE
    Boost::system
    Threads::Threads
    mongo::mongocxx_static
    mongo::bsoncxx_static
)