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
using namespace std;
using boost::asio::ip::tcp;


class chat_handler : public enable_shared_from_this<chat_handler> {
public:
	typedef shared_ptr<chat_handler> pointer;
	static pointer create(boost::asio::io_context& io_context) {
		return pointer(new chat_handler(io_context));
	}
	tcp::socket& socket() {
		return socket_;
	}
	void listening(vector<shared_ptr<chat_handler>>* clients) {
		socket_.async_read_some(boost::asio::buffer(message_buffer_), [self = shared_from_this(), clients](const boost::system::error_code& error, size_t bytes_transferred) {
			if (!error) {
				string msg(self->message_buffer_.data(), bytes_transferred);
				self->broadcast(clients, msg);
				self->listening(clients);
			}
			else {
				cerr << "Error reading message: " << error.message() << endl;
			}
			});
	};
	void broadcast(vector<shared_ptr<chat_handler>>* clients_, const string& msg) {
		for (auto& client : *clients_) {
			boost::asio::async_write(client->socket(), boost::asio::buffer(msg),
				[self = shared_from_this()](const boost::system::error_code& error, size_t bytes_transferred) {
					if (!error) {
						cout << "Message broadcasted to clients: " << bytes_transferred << " bytes" << endl;
					}
					else {
						cerr << "Error broadcasting message: " << error.message() << endl;
					}
				});
		}
	}
private:
	chat_handler(boost::asio::io_context& io) : socket_(io) {}
	tcp::socket socket_;
	std::array<char, 1024> message_buffer_;
};
class chat_server {
public:
	chat_server(boost::asio::io_context& io_context, const string& host, unsigned short port) : io_context_(io_context), acceptor_(io_context_, tcp::endpoint(boost::asio::ip::make_address(host), port)) {
		start_accept();
	};
	void start_accept() {
		chat_handler::pointer new_client = chat_handler::create(io_context_);
		acceptor_.async_accept(new_client->socket(), [this, new_client](const boost::system::error_code& error) {
				if (!error) {
					clients.push_back(new_client);
					new_client->listening(&clients);
				}
				start_accept();
				});
	}
private:
	boost::asio::io_context& io_context_;
	tcp::acceptor acceptor_;
	std::vector<std::shared_ptr<chat_handler>> clients;
};

int main() {
	// 4. the trigger step:
	boost::asio::io_context io_context;
	chat_server srv(io_context, "10.40.83.48", 1234);
	io_context.run();
	return 0;
}