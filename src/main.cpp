#include "ChatServer/ChatServer.h"

int main(int argc, char* argv[]) {
    try {
        uint16_t port = 9000;
        if (argc >= 2) {
            port = static_cast<uint16_t>(std::stoi(argv[1]));
        }

        asio::io_context io;
        Server server(io, port);

        io.run();

    } catch (const std::exception& e) {
        std::cerr << "[Fatal] " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}