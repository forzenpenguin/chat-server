/*
* ASIO/server BLUEPRINT
* 1. Setup:
*	- io_context: The core I/O object that manages asynchronous operations.
*	- Acceptor: Listens for
* 2. Listing loop:
*	- You need a function that constantly loops to accept new guests.
*	- The Concept: Create a blank "socket room" (connection object), and tell the OS: "When someone knocks, put them in this room and call my handle_accept function."
*	- The Critical Trick: Inside handle_accept, you must immediately call your listen function again so the guard goes back to the door for the next guest.
* 3. The Object Lifetime Guard Step
*	- Asynchronous code means your functions return immediately while the OS works in the background. If your connection object dies while the OS is writing to it, the program crashes.
*	- The Concept: The connection object must hold onto itself using shared_from_this() until the OS says, "I'm done sending the data."
* 4. The Trigger Step
*	- Nothing actually happens until you turn the key.
*	- The Concept: Call io_context.run(). This starts the background loop that listens for the OS to say "I'm done!" and executes your callbacks.
*/

#include <iostream>
#include <thread>
#include <boost/asio.hpp>
#include <functional>
#include <vector>
#include <ctime>
#include <memory>
#include <chrono>
using namespace std;
using boost::asio::ip::tcp;

class chat_room;
class chat_session : public enable_shared_from_this<chat_session> {
public:
	typedef shared_ptr<chat_session> pointer;
	static pointer create(boost::asio::io_context& io_context, chat_room& room_ptr) {
		return make_shared<chat_session>(io_context, room_ptr);
	};
	void start();
	void self_send(string msg) {
		auto shared_msg = make_shared<string>(msg);
		boost::asio::async_write(socket_,boost::asio::buffer(*shared_msg), [self = shared_from_this(), shared_msg](const boost::system::error_code& error, size_t bytes_transferred) {
			if (!error) {
				cout << "Sent: " << self->send_buffer_ << endl;
			}
			else {
				cerr << "ServerSendError: " << error.message() << endl;
			}
			});
	};
	tcp::socket& socket() {
		return socket_;
	};
	chat_session(boost::asio::io_context& io_context, chat_room& room_ptr) : socket_(io_context), room(room_ptr) {};
private:
	tcp::socket socket_;
	string send_buffer_;
	array<char, 1024> recv_buffer_;
	chat_room& room;
};

class chat_room {
public:
	void join(shared_ptr<chat_session> session) {
		sessions_.push_back(session);
	};
	void leave(shared_ptr<chat_session> session) {
		sessions_.erase(remove(sessions_.begin(), sessions_.end(), session), sessions_.end());
	};
	void broadcast(string msg) {
		for (auto& session : sessions_) {
			if (session->socket().is_open()) {
				session->self_send(msg);
			}
			else {
				leave(session);
			}
		}
	};
	vector<shared_ptr<chat_session>>& get_sessions() {
		return sessions_;
	}

private:
	vector<shared_ptr<chat_session>> sessions_;
};

void chat_session::start() {
	boost::asio::async_read(socket_, boost::asio::buffer(recv_buffer_), boost::asio::transfer_at_least(1), [self = shared_from_this()](const boost::system::error_code& error, size_t bytes_transferred) {
		if (!error) {
			self->send_buffer_ = string(self->recv_buffer_.data(), bytes_transferred);
			self->room.broadcast(self->send_buffer_);
		}
		else {
			cerr << "ServerReadingError: " << error.message() << endl;
		}
		});
};

class chat_server {
public:
	chat_server(boost::asio::io_context& io_context, chat_room& room, const string host, unsigned short port) : io_context_(io_context), acceptor_(io_context_, tcp::endpoint(boost::asio::ip::make_address(host), port)), room_(room) {
		start_accept();
	};
	void start_accept() {
		auto new_session = chat_session::create(io_context_, room_);
		acceptor_.async_accept(new_session->socket(), [this, new_session](const boost::system::error_code& error) {
			if (!error) {
				room_.join(new_session);
				if (room_.get_sessions().size() == 1) {
					boost::asio::async_write(new_session->socket(), boost::asio::buffer("<SERVER> Chat Room Started!"), [](const boost::system::error_code& error, size_t bytes_transfered) {
						if (!error) {
							cout << "sent!!!!!!!!!: BTYES:  " << bytes_transfered << endl;
						}
						else {
							cerr << "SERVER: " << error.message() << endl;
						}
						});
					//new_session->self_send("SERVER: Chat Room Started!");
				};
				new_session->start();
			}
			start_accept();
			});
	};
private:
	boost::asio::io_context& io_context_;
	tcp::acceptor acceptor_;
	chat_room& room_;
};

int main() {
	cout << "server " << endl;
	boost::asio::io_context io_context;
	chat_room room;
	chat_server new_server(io_context, room, "10.134.87.48", 1234);
	io_context.run();
	return 0;
}