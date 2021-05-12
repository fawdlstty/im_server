#ifndef __IM_TYPED_SERVER_T_HPP__
#define __IM_TYPED_SERVER_T_HPP__



#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/core.h>

#include "im_server_t.hpp"



enum class im_msg_type_t {
	command = 0,
	advertising = 1,
	text = 10,
	image = 11,
	audio = 12,
	video = 13,
	invalid = 20,
	send_success = 21,
	readed = 22,
};

enum class im_msg_flag_t {
	none = 0,
	on_recv_need_reply = 1,
	on_read_need_reply = 2,
	store = 4,
	force_notify = 8,
};

class im_typed_server_t {
public:
	im_typed_server_t (size_t _process_thread_num, uint16_t _port): m_server (1, _process_thread_num), m_port (_port) {}

	bool run (bool block_this_thread = true) {
		if (!m_server.init (m_port, "/ws"))
			return false;

		m_server.on_string_message_callback ([this] (std::shared_ptr<im_connect_t> _conn, std::string _data) {
			try {
				nlohmann::json _j = nlohmann::json::parse (_data);
				int64_t _sender = _conn->get_uid ();
				int64_t _recver = _j ["recver"].get<int64_t> ();
				int64_t _hash = _j ["msg_hash"].get<int64_t> ();
				std::string _type = _j ["type"].get<std::string> ();
				std::string _content = _j ["content"].get<std::string> ();
				//
				if (_recver <= 0) {
					_conn->send_string (R"({"type":"error","detail":"recver must greater than 0"})");
				} else if (_type == "text" || _type == "image" || _type == "audio" || _type == "video") {
					auto _fut = m_allow_transfer (_sender, _recver);
					m_tpool.async_after_run (std::move (_fut), [] (bool _allow) {
						// TODO
					});
					if (!) {

					}
				} else {
					_conn->send_string (fmt::format (R"({{"type":"invalid",""}})"));
				}
			} catch (std::exception &e) {
				_conn->send_string (R"({"type":"error","detail":"format error"})");
			} catch (...) {
				_conn->send_string (R"({"type":"error","detail":"format error"})");
			}
		});

		m_server.run (block_this_thread);
		return true;
	}

	void set_allow_transfer (std::function<fa::future_t<bool> (int64_t _sender_uid, int64_t _recver_uid)> _cb) {
		m_allow_transfer = _cb;
	}

	void on_open_callback (std::function<fa::future_t<std::optional<int64_t>> (std::shared_ptr<im_connect_t>)> _cb) {
		//im_server_t::on_open_callback (_cb);
	}

private:
	im_server_t m_server;
	uint16_t m_port;
	fa::taskpool_t m_tpool { 1 };

	//std::function<void (int64_t _msg_id, int64_t _send_uid, int64_t _recv_uid, im_msg_type_t _msg_type, im_msg_flag_t _msg_flag, std::span<uint8_t> _msg)> m_on_message;
	std::function<fa::future_t<bool> (int64_t _sender_uid, int64_t _recver_uid)> m_allow_transfer;
};



#endif //__IM_TYPED_SERVER_T_HPP__
