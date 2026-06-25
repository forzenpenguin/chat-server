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


int increment_instance = 0;
class chat_room;
class chat_session : public enable_shared_from_this<chat_session> {
public:
	typedef shared_ptr<chat_session> pointer;
	static pointer create(boost::asio::io_context& io_context, chat_room& room_ptr) {
		return make_shared<chat_session>(io_context, room_ptr);
	};
	void start();
	void self_send(string msg, string username_buffer);
	tcp::socket& socket() {
		return socket_;
	};
	int get_instance_number() {
		return instance_number;
	};
	chat_session(boost::asio::io_context& io_context, chat_room& room_ptr) : socket_(io_context), room(room_ptr) {
		instance_number = increment_instance;
	};
	string get_username() {
		return username;
	};
	void set_username(const string username_param) {
		username = username_param;
		cout << "username: " << username << endl;
	}
private:
	tcp::socket socket_;
	string send_buffer_;
	array<char, 1024> recv_buffer_;
	chat_room& room;
	int instance_number = 0;
	string username = "NULL";
};

class chat_room {
public:
	void join(shared_ptr<chat_session> session) {
		sessions_.push_back(session);
		//auto usernameBuffer = make_shared<string>(session->get_username());
		for (auto& session : sessions_) {
			boost::asio::async_write(session->socket(), boost::asio::buffer( "USER JOINED"), [](const boost::system::error_code& error, size_t bytes_transferred) {
				if (error)
					cerr << "<JOININGERROR>: " << error.message() << endl;
				});
		}
	};
	void leave(shared_ptr<chat_session> session) {
		auto left_msg = make_shared<string>(session->get_username() + " LEFT");
		for (auto& session : sessions_) {
			boost::asio::async_write(session->socket(), boost::asio::buffer(*left_msg), [left_msg](const boost::system::error_code& error, size_t bytes_transferred) {
				if (error)
					cerr << "<lEAVINGERROR>: " << error.message() << endl;
				});
		}
		sessions_.erase(remove(sessions_.begin(), sessions_.end(), session), sessions_.end());
	};
	void broadcast(string msg, int& instance_num_param, string username_buffer) {
		for (auto& session : sessions_) {
			if (session->get_instance_number() == instance_num_param)
				continue;
			if (session->socket().is_open()) {
				session->self_send(msg, username_buffer);
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
			self->room.broadcast(self->send_buffer_, self->instance_number, self->username);
			cout << "received: " << string(self->recv_buffer_.data(), bytes_transferred) << endl;
			self->start();
		}
		else if (error == boost::asio::error::eof ||
			error == boost::asio::error::connection_reset)
		{
			boost::asio::post(self->socket_.get_executor(), [self]() {
				self->room.leave(self);
				});
		}
		else {
			cerr << "<ReadingError> " << error.message() << endl;
		}
		});
};
void chat_session::self_send(string msg, string username_buffer) {
	auto shared_msg = make_shared<string>(username_buffer + ": " + msg);
	boost::asio::async_write(socket_, boost::asio::buffer(*shared_msg), [self = shared_from_this(), shared_msg](const boost::system::error_code& error, size_t bytes_transferred) {
		if (!error) {
			cout << "Sent: " << string(*shared_msg) << endl;
		}
		else {
			cerr << "<SendError> " << error.message() << endl;
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
				increment_instance++;
				boost::asio::async_read(new_session->socket(), boost::asio::buffer(temUsernameHolder), boost::asio::transfer_at_least(1),[this, new_session](const boost::system::error_code& ec, size_t bytes_transferred) {
					if (!ec) {
						new_session->set_username(string(temUsernameHolder.data(), bytes_transferred));
					}
					else {
						cerr << "<USERNAME> " << ec.message() << endl;
					}
				});
				room_.join(new_session);
				if (room_.get_sessions().size() == 1) {
					boost::asio::async_write(new_session->socket(), boost::asio::buffer("<SERVER> Chat Room Started!"), [](const boost::system::error_code& error, size_t bytes_transfered) {
						if (error) {
							cerr << "<SERVER> " << error.message() << endl;

						}
						});
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
	array<char, 100> temUsernameHolder;
};

int main() {
	cout << "server " << endl;
	boost::asio::io_context io_context;
	chat_room room;
	chat_server new_server(io_context, room, "10.184.251.48", 1234);
	io_context.run();
	return 0;
}