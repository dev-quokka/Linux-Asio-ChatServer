#include <boost/asio.hpp>
#include <array>
#include <iostream>
#include <memory>

namespace asio = boost::asio;
using asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
public:
    // 생성자
    Session(tcp::socket socket) : socket_(std::move(socket)) {} 
    
    void start() { Read(); }

private:
    void Read() {
        auto self = shared_from_this();
        socket_.async_read_some(asio::buffer(buf_),
            [this, self](const boost::system::error_code& ec, std::size_t n) {
                if (ec) { // 연결 종료 or 에러
                    return;
                }
                Write(n);
            });
    }

    void Write(std::size_t n) {
        auto self = shared_from_this();
        asio::async_write(socket_, asio::buffer(buf_.data(), n),
            [this, self](const boost::system::error_code& ec, std::size_t n) {
                if (ec) { 
                    return;
                }
                Read();
            });
    }

    tcp::socket socket_;
    std::array<char, 4096> buf_{};
};

class Server {
public:
    Server(asio::io_context& io, uint16_t port) : acceptor_(io, tcp::endpoint(tcp::v4(), port)) {
        Accept();
        std::cout << "[Server] Listening on 0.0.0.0:" << port << "\n";
    }

private:
    void Accept() {
        acceptor_.async_accept(
            [this](const boost::system::error_code& ec, tcp::socket socket) {
                if (!ec) {
                    // 클라이언트 1명 접속하면 세션 생성
                    std::make_shared<Session>(std::move(socket))->start();
                }
                Accept(); // 다음 클라이언트 계속 받기
            });
    }

    tcp::acceptor acceptor_;
};

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