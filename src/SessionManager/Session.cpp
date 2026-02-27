// #include "Session.h"
// #include "ChatServer.h"

// Session::Session(tcp::socket socket, Server& server)
//     : socket_(std::move(socket)), server_(server) {}

// void Session::Start() { Read(); }

// void Session::Deliver(const std::shared_ptr<const std::string>& msg) {
//     outbox_.push_back(msg);
//     if (writing_) return;
//     writing_ = true;
//     Write();
// }

// void Session::Read() {
//     auto self = shared_from_this();
//     socket_.async_read_some(asio::buffer(buf_),
//         [this, self](const boost::system::error_code& ec, std::size_t n) {
//             if (ec) { server_.leave(self); return; }
//             auto msg = std::make_shared<const std::string>(buf_.data(), n);
//             server_.broadcast(msg);
//             Read();
//         });
// }

// void Session::Write() {
//     auto self = shared_from_this();
//     if (outbox_.empty()) { writing_ = false; return; }

//     auto msg = outbox_.front();
//     asio::async_write(socket_, asio::buffer(*msg),
//         [this, self](const boost::system::error_code& ec, std::size_t) {
//             if (ec) { server_.leave(self); return; }
//             outbox_.pop_front();
//             Write();
//         });
// }