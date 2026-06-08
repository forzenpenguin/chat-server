/*
* ASIO/client BLUEPRINT
* 1. The Setup Step
*	- The Concept: Create the engine, create a blank socket, and set up a translator (resolver).
*	- The Code Mindset: "Create io_context, tcp::socket, and tcp::resolver."
* 2. The Resolve and Connect Step
*	- The Concept: Tell the resolver to look up the address. When it finishes, pass those address results to the socket and say, "Hey OS, connect to this address. Call my lambda when the handshake is complete."
*	- The Code Mindset: resolver.async_resolve(...) triggers socket.async_connect(...).
* 3. The Read/Write Loop Step
*	- The Concept: Tell the OS: "Read whatever data comes into this socket into my buffer. Wake me up via this lambda when you've got something."
*	- The Critical Trick: Just like the server's accept loop, if you expect multiple pieces of data, your read callback must call async_read or async_read_some again to keep listening.
* 4. The Trigger Step
*	-The Concept: Call io_context.run().
*/


#include <iostream>
#include <boost/asio.hpp>
#include <vector>
#include <ctime>
#include <memory>

using namespace std;
using boost::asio::ip::tcp;

class client {
private:
	boost::asio::io_context& io;
	tcp::socket socket_;
	tcp::resolver resolver_;
public:
	client(boost::asio::io_context& io_context) : io(io_context), socket_(io), resolver_(io) {}
	void start(const string& host, unsigned short port) {
		resolver_.async_resolve(host, to_string(port), [this](const boost::system::error_code& ec, tcp::resolver::results_type results) {
			if (!ec) {
				boost::asio::async_connect(socket_, results, bind(&client::handle_connect, this, placeholders::_1, placeholders::_2));
			}
			else {
				cerr << "Resolve error: " << ec.message() << endl;
			}
			});
	};
	void handle_connect(const boost::system::error_code& ec, const tcp::endpoint& /*endpoint*/) {
		if (!ec) {
			vector<char> buffer_(1024);
			cout << "Connected to server." << endl;
			boost::system::error_code read_ec;
			socket_.async_read_some(boost::asio::buffer(buffer_), [this, buffer_](const boost::system::error_code& read_ec, size_t len) {
				if (!read_ec) {
					cout << "Received: " << string(buffer_.data(), len) << endl;
				}
				else if (read_ec == boost::asio::error::eof) {
					cout << "Connection closed by server." << endl;
				}
				else {
					cerr << "Read error: " << read_ec.message() << endl;
				}
				});
		}
		else {
			cerr << "Connect error: " << ec.message() << endl;
		}
	};

};


int main() {
	cout << "Client is starting..." << endl;
	try {
		boost::asio::io_context io_context;
		client myClient(io_context);
		myClient.start("10.40.83.48", 1234);
		io_context.run();
	}
	catch (exception& e) {
		cerr << "Exception: " << e.what() << endl;
	}
	return 0;
};