cmake_minimum_required(VERSION 3.16)
project(chat_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS system)

add_executable(chat_server
    src/main.cpp
    src/SessionManager/SessionManager.cpp
)

target_link_libraries(chat_server
    Boost::system
)