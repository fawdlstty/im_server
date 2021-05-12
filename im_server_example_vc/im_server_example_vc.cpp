#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <optional>
#include <string>

#include <im_server/im_server.hpp>
#include "../taskpool/taskpool_t.hpp"
#include "../taskpool/objectpool_t.hpp"
////#include <im_server/im_server_t.hpp>

#define ORMPP_ENABLE_MYSQL
#include "../ormpp/mysql.hpp"
#include "../ormpp/dbng.hpp"

#ifdef _MSC_VER
#pragma comment (lib, "Crypt32.lib")
#endif



fa::taskpool_t s_pool { 1 };
im_server_t s_server { 1, 1 };



struct tb_person {
	int64_t id;
	std::string name;
	int age;
};
REFLECTION (tb_person, id, name, age)

int main () {
	ormpp::dbng<ormpp::mysql> _sql;
	bool _is_conn = _sql.connect ("127.0.0.1", "username", "password", "dbname", 3306);
	tb_person p { 0, "kangkang", 18 };
	int x = _sql.insert (p);
	x = x;





	if (!s_server.init (8080, "/ws")) {
		std::cout << "listen failed.\n";
		return 0;
	}
	s_server.on_open_callback ([] (std::shared_ptr<im_connect_t> _conn) -> fa::future_t<std::optional<int64_t>> {
		//std::string _val = _conn->get_param ("xx");
		//auto [_ip, _port] = _conn->remote_info ();
		//std::cout << "connect: " << _ip << "[" << _port << "] xx[" << _val << "]" << std::endl;
		return fa::future_t<std::optional<int64_t>>::from_value (1);
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
	s_server.on_binary_message_callback ([] (std::shared_ptr<im_connect_t> _conn, span_t<uint8_t> _data) {
		std::cout << "recv binary(size): " << _data.size () << std::endl;
	});
	s_server.on_close_callback ([] (int64_t _cid) {
		std::cout << "disconnect" << std::endl;
	});
	s_server.run ();
	return 0;
}
