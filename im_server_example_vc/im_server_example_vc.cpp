#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <string>

#include <im_server/im_server.hpp>



int main () {
	im_server_t _server { 1 };
	_server.on_open_callback ([] (std::shared_ptr<im_connect_t> _conn, int64_t _cid) {
		std::string _val = _conn->get_param ("xx");
		auto [_ip, _port] = _conn->remote_info ();
		std::cout << "connect: " << _ip << "[" << _port << "] xx[" << _val << "]" << std::endl;
	});
	_server.on_string_message_callback ([&] (std::shared_ptr<im_connect_t> _conn, int64_t _cid, std::string _data) {
		std::cout << "recv string: " << _data << std::endl;
		if (_data == "close") {
			std::thread ([&] () {
				_server.stop ();
			}).detach ();
		}
	});
	_server.on_binary_message_callback ([] (std::shared_ptr<im_connect_t> _conn, int64_t _cid, const uint8_t *_data, size_t _size) {
		std::cout << "recv binary(size): " << _size << std::endl;
	});
	_server.on_close_callback ([] (std::shared_ptr<im_connect_t> _conn, int64_t _cid) {
		std::cout << "disconnect" << std::endl;
	});
	_server.start (8080, "/ws");
	return 0;
}
