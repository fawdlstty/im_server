#ifndef __IM_OFFLINE_SERVER_T_HPP__
#define __IM_OFFLINE_SERVER_T_HPP__



#include <chrono>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/core.h>

#include "im_server_t.hpp"



//enum class im_msg_type_t {
//	command = 0,
//	advertising = 1,
//	text = 10,
//	image = 11,
//	audio = 12,
//	video = 13,
//	invalid = 20,
//	send_success = 21,
//	read = 22,
//};
//
//enum class im_msg_flag_t {
//	none = 0,
//	on_recv_need_reply = 1,
//	on_read_need_reply = 2,
//	store = 4,
//	force_notify = 8,
//};

/*
* IM登录流程
* recver->服务器：连接，鉴权
* sender->服务器：{ "type": "init" }
* 服务器->数据仓储：读取待发送信息
* 服务器->recver：[{...}, {...}, ...]
* 
* 
* 
* IM消息流程
* sender->服务器：{ "seq": 消息序列号(避免重复), "recver_uid": 接收者ID, "type": "消息类型", "content": "消息内容" }
* 服务器->数据仓储：存档消息
* 服务器->sender：{ "type": "reply", "result": "success", "seq": 序列号, "msg_id": 消息ID }
* 服务器->recver：{ "type": "消息类型", "sender": 发送者ID, "msg_id": 消息ID, "content": "消息内容" }
* 
* recver->服务器：{ "type": "arrive", "msg_ids": [ 消息ID列表 ] }
* 服务器->数据仓储：更新消息状态
* 服务器->sender：{ "type": "reply", "msg_ids": [ 消息ID列表 ] }
* 
* recver->服务器：{ "type": "read", "msg_ids": [ 消息ID列表 ] }
* 服务器->数据仓储：更新消息状态
* 服务器->sender：{ "type": "reply", "msg_ids": [ 消息ID列表 ] }
*/



class im_offline_server_t {
public:
	im_offline_server_t (): m_server (1, 1) {}

	bool init (uint16_t _port, std::string _prefix = "/") {
		return m_server.init (_port, "/ws");
	}

	std::shared_ptr<xfinal::http_server> get_server () {
		return m_server.get_server ();
	}

	bool run (bool block_this_thread = true) {
		m_server.on_open_callback ([this] (std::shared_ptr<im_connect_t> _conn) -> std::optional<int64_t> {
			try {
				// 鉴权
				std::string _auth_str = _conn->get_param ("auth");
				std::optional<nlohmann::json> _oj = m_check_auth (_auth_str);
				if (_oj.has_value ()) {
					int64_t _current_uid = _oj.value () ["uid"].get<int64_t> ();
					return _current_uid;
				}
			} catch (...) {
			}
			return std::nullopt;
		});

		m_server.on_string_message_callback ([this] (std::shared_ptr<im_connect_t> _conn, std::string _data) {
			int64_t _current_uid = _conn->get_uid ();
			try {
				nlohmann::json _j = nlohmann::json::parse (_data);
				std::string _type = _j ["type"].get<std::string> ();
				if (_type == "init") {
					// 初始化待处理信息
					fa::future_t<std::vector<int64_t>> _fut = m_get_pendings_count (_current_uid);
					m_tpool.async_after_run (std::move (_fut), [this, _current_uid] (std::vector<int64_t> _pending_ids) {
						_process_pendings (_current_uid, _pending_ids);
					});
				} else if (_type == "text" || _type == "image" || _type == "audio" || _type == "video") {
					std::chrono::system_clock::time_point _tp = std::chrono::system_clock::now ();
					int64_t _recver_uid = _j ["recver_uid"].get<int64_t> ();
					int64_t _seq = _j ["seq"].get<int64_t> ();
					std::string _content = _j ["content"].get<std::string> ();
					auto _fut = m_allow_transfer (_current_uid, _recver_uid);
					m_tpool.async_after_run (std::move (_fut), [=] (bool _allow) {
						if (_allow) {
							// 存档信息
							auto _fut = m_store_msg (_current_uid, _recver_uid, _seq, _type, _content);
							m_tpool.async_after_run (std::move (_fut), [=] (int64_t _msg_id) {
								// 回复sender
								_reply_success (_conn, _seq, _msg_id);
								// 发送给recver
								std::optional<std::shared_ptr<im_connect_t>> _recver_oconn = m_server.get_connect (_recver_uid);
								if (_recver_oconn.has_value ())
									_send_to_recver (_recver_oconn.value (), _tp, _type, _current_uid, _msg_id, _content);
							});
						} else {
							_conn->send_string (nlohmann::json ({ { "type", "status_reply" }, { "status", "failure" }, { "seq", _seq }, { "reason", "cannot send to recver" } }).dump ());
						}
					});
				} else if (_type == "arrive") {
					std::vector<int64_t> _msg_ids = _j ["msg_ids"].get<std::vector<int64_t>> ();
					m_process_arrive (_current_uid, _msg_ids);
				} else if (_type == "read") {
					std::vector<int64_t> _msg_ids = _j ["msg_ids"].get<std::vector<int64_t>> ();
					m_process_read (_current_uid, _msg_ids);
				}
			} catch (...) {
				_reply_failure (_conn, "json format error");
			}
		});

		m_server.run (block_this_thread);
		return true;
	}

private:
	void _process_pendings (int64_t _uid, std::vector<int64_t> _pending_ids) {
		if (_pending_ids.size () == 0)
			return;
		int64_t _pending_id = _pending_ids [0];
		_pending_ids.erase (_pending_ids.begin ());
		fa::future_t<std::optional<nlohmann::json>> _fut = m_get_pending (_uid, _pending_id);
		m_tpool.async_after_run (std::move (_fut), [this] (std::optional<nlohmann::json> _opending) {
			if (_opending.has_value ()) {
				//
			} else {

			}
		});
	}

	fa::future_t<bool> _send_to_recver (std::shared_ptr<im_connect_t> _conn, std::chrono::system_clock::time_point _tp, std::string _type, int64_t _sender_uid, int64_t _msg_id, std::string _content) {
		return _conn->send_string (nlohmann::json ({ { "type", _type }, { "sender", _sender_uid }, { "msg_id", _msg_id }, { "content", _content } }).dump ());
	}

	fa::future_t<bool> _reply_success (std::shared_ptr<im_connect_t> _conn, int64_t _seq, int64_t _msg_id) {
		return _conn->send_string (nlohmann::json ({ { "type", "reply" }, { "status", "success" }, { "seq", _seq }, { "msg_id", _msg_id } }).dump ());
	}

	fa::future_t<bool> _reply_failure (std::shared_ptr<im_connect_t> _conn, std::string _reason) {
		return _conn->send_string (nlohmann::json ({ { "type", "reply" }, { "result", "failure" }, { "reason", _reason } }).dump ());
	}

	im_server_t m_server;
	fa::taskpool_t m_tpool { 1 };

public:
	std::function<std::optional<nlohmann::json> (std::string _auth_str)> m_check_auth;
	std::function<fa::future_t<std::vector<int64_t>> (int64_t _uid)> m_get_pendings_count;
	std::function<fa::future_t<std::optional<nlohmann::json>> (int64_t _uid, int64_t _pending_id)> m_get_pending;
	std::function<fa::future_t<bool> (int64_t _sender_uid, int64_t _recver_uid)> m_allow_transfer;
	std::function<fa::future_t<int64_t> (int64_t _sender_uid, int64_t _recver_uid, int64_t _seq, std::string type, std::string content)> m_store_msg;
	std::function<fa::future_t<void> (int64_t _recver_uid, const std::vector<int64_t> &_msg_ids)> m_process_arrive;
	std::function<fa::future_t<void> (int64_t _recver_uid, const std::vector<int64_t> &_msg_ids)> m_process_read;
};



#endif //__IM_OFFLINE_SERVER_T_HPP__
