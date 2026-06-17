#include <iostream>
#include <boost/asio.hpp>
#include <vector>
#include <ctime>
#include <memory>
#include <string>

using namespace std;
using boost::asio::ip::tcp;

class mediator : public enable_shared_from_this<mediator> {
public:
	typedef shared_ptr<mediator> pointer;
	static pointer create(boost::asio::io_context& io) {
		return make_shared<mediator>(io);
	}
	mediator(boost::asio::io_context& io) : socket_(io), strand_(boost::asio::make_strand(io)) {}
	tcp::socket& socket() {
		return socket_;
	}
	void send_respond(const string respond) {
		auto res = make_shared<string>(respond);
		boost::asio::post(strand_, [self = shared_from_this(), res]() {
			boost::asio::async_write(self->socket_, boost::asio::buffer(*res), [res](const boost::system::error_code& error, size_t bytes_transferred) {
				if (error) {
					cerr << "SendingError: " << error.message() << endl;
				}
				});
			});
	};
private:
	tcp::socket socket_;
	boost::asio::strand<boost::asio::io_context::executor_type> strand_;
};

class sender {
public:
	sender(mediator::pointer med_param) : med(med_param) {};
	void operator()() {
		cout << "Type: ";
		getline(cin, res_msg);
		med->send_respond(res_msg);
	};
private:
	string res_msg = "";
	mediator::pointer med;
};


class client {
public:
	client(boost::asio::io_context& io, const string username_param) : io_context_(io), resolver_(io_context_), username(username_param) { buffer_.resize(1024); }
	void connect(const string host, unsigned short port) {
		auto new_mediator = mediator::create(io_context_);
		resolver_.async_resolve(host, std::to_string(port),
			[this, new_mediator](const boost::system::error_code& error, tcp::resolver::results_type endpoints) {
				if (!error) {
					boost::asio::async_connect(new_mediator->socket(), endpoints, [this, new_mediator](const boost::system::error_code error, const tcp::endpoint& endpoint) {
						if (!error) {
							boost::asio::async_write(new_mediator->socket(), boost::asio::buffer(username), [this](const boost::system::error_code& error, size_t bytes_transferred) {
								if (error)
									cerr << "<USERNAME> " << error.message() << endl;
								else
									cout << "username: " << username << "bytes: " << bytes_transferred << endl;
								});
							read_msg(new_mediator);
						}
						else {
							cerr << "ConnectionError: " << error.message() << endl;
						}
						}
					);

				}
				else {
					cerr << "ResolverError: " << error.message() << endl;
				}
			});
	}
	void read_msg(mediator::pointer med) {
		boost::asio::async_read(med->socket(), boost::asio::buffer(buffer_), boost::asio::transfer_at_least(1), [this, med](const boost::system::error_code& error, size_t bytes_transferred) {
			if (!error) {
				cout << "Message: " << string(this->buffer_.data(), bytes_transferred) << endl;
				sender sender1(med);
				thread(sender1).detach();
				read_msg(med);
			}
			else {
				cerr << "ReadError: " << error.message() << endl;
			}
			});
	};
private:
	boost::asio::io_context& io_context_;
	tcp::resolver resolver_;
	vector<char>buffer_;
	string username = "";
};


int main(int argc, char* argv[]) {
	if (argc < 2) {
		cerr << "parameters required" << argv[0] << "usename" << endl;
		return 1;
	}
	string username = argv[1];
	cout << "=========================Chat Room============================" << endl;
	try {
		boost::asio::io_context io_context;
		client myClient(io_context, username);
		myClient.connect("10.134.87.48", 1234);
		io_context.run();
	}
	catch (exception& e) {
		cerr << "Exception: " << e.what() << endl;
	}
	return 0;
};