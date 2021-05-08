#ifndef __IM_TYPED_SERVER_T_HPP__
#define __IM_TYPED_SERVER_T_HPP__



#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <span>
#include <vector>

#include "im_server_t.hpp"



enum class im_msg_type_t {
	command = 0,
	recv_reply = 1,
	read_reply = 2,
	text = 3,
	image = 4,
	audio = 5,
	video = 6,
	media_chat = 7,
	advertising = 8,
};

enum class im_msg_flag_t {
	none = 0,
	on_recv_need_reply = 1,
	on_read_need_reply = 2,
	store = 4,
	force_notify = 8,
};

class im_typed_server_t: private im_server_t {
public:
	im_typed_server_t (size_t _io_thread_num, size_t _process_thread_num): im_server_t (_io_thread_num, _process_thread_num) {}

	//void set_on_message (std::function<void (int64_t _msg_id, int64_t _send_uid, int64_t _recv_uid, im_msg_type_t _msg_type, im_msg_flag_t _msg_flag, std::span<uint8_t> _msg)> _on_message) {
	//	m_on_message = _on_message;
	//}

	void set_allow_transfer (std::function<fa::future_t<bool> &&(int64_t _msg_id, int64_t _send_uid, int64_t _recv_uid, im_msg_type_t _msg_type)> _cb) {
		m_allow_transfer = _cb;
	}

	void on_open_callback (std::function<fa::future_t<std::optional<int64_t>> &&(std::shared_ptr<im_connect_t>)> _cb) {
		im_server_t::on_open_callback (_cb);
	}

private:
	//std::function<void (int64_t _msg_id, int64_t _send_uid, int64_t _recv_uid, im_msg_type_t _msg_type, im_msg_flag_t _msg_flag, std::span<uint8_t> _msg)> m_on_message;
	std::function<fa::future_t<bool> &&(int64_t _msg_id, int64_t _send_uid, int64_t _recv_uid, im_msg_type_t _msg_type)> m_allow_transfer;
};



#endif //__IM_TYPED_SERVER_T_HPP__