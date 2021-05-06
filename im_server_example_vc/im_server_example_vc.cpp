#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <optional>
#include <string>

#include <im_server/im_server.hpp>
#include "../taskpool/taskpool_t.hpp"
//#include <im_server/im_server_t.hpp>



fa::taskpool_t s_pool { 1 };
im_server_t s_server { 1, 1 };

int main () {
	s_server.on_open_callback ([] (std::shared_ptr<im_connect_t> _conn) -> fa::future_t<std::optional<int64_t>>&& {
		std::string _val = _conn->get_param ("xx");
		auto [_ip, _port] = _conn->remote_info ();
		std::cout << "connect: " << _ip << "[" << _port << "] xx[" << _val << "]" << std::endl;
		return common_t::get_valued_future<std::optional<int64_t>> (1);
	});
	s_server.on_string_message_callback ([&] (std::shared_ptr<im_connect_t> _conn, std::string _data) {
		std::cout << "recv string: " << _data << std::endl;
		if (_data == "close") {
			//std::thread ([&] () {
			//	s_server.stop ();
			//}).detach ();
			auto _fut = s_pool.async_wait (std::chrono::seconds (3));
			s_pool.async_after_run (std::move (_fut), [] () {
				s_server.close_client (1);
			});
		}
	});
	s_server.on_binary_message_callback ([] (std::shared_ptr<im_connect_t> _conn, std::vector<uint8_t> _data) {
		std::cout << "recv binary(size): " << _data.size () << std::endl;
	});
	s_server.on_close_callback ([] (int64_t _cid) {
		std::cout << "disconnect" << std::endl;
	});
	if (!s_server.init (8080, "/ws")) {
		std::cout << "listen failed.\n";
		return 0;
	}
	s_server.run ();
	return 0;
}
