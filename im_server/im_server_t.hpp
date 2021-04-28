#ifndef __IM_SERVER_T_HPP__
#define __IM_SERVER_T_HPP__



#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <thread>

#include "../xfinal/xfinal/xfinal.hpp"

#include "im_server_base_t.hpp"



class _session_storage: public xfinal::session_storage {
public:
	bool init () { return true; }
	void save (xfinal::session &session) {}
	void remove (xfinal::session &session) {}
	void remove (std::string const &uuid) {}
};

class im_server_t {
public:
	im_server_t (size_t _threads): m_base (_threads) {}

	void on_open_callback (std::function<std::optional<int64_t> (std::shared_ptr<im_connect_t>)> _callback) { m_base.on_open_callback (_callback); }
	void on_string_message_callback (std::function<void (std::shared_ptr<im_connect_t>, std::string)> _callback) { m_base.on_string_message_callback (_callback); }
	void on_binary_message_callback (std::function<void (std::shared_ptr<im_connect_t>, const uint8_t *, size_t)> _callback) { m_base.on_binary_message_callback (_callback); }
	void on_close_callback (std::function<void (std::shared_ptr<im_connect_t>)> _callback) { m_base.on_close_callback (_callback); }
	bool send_string (int64_t _cid, std::string _data) { return m_base.send_string (_cid, _data); }
	bool send_binary (int64_t _cid, const uint8_t *_data, size_t _size) { return m_base.send_binary (_cid, _data, _size); }

	bool start (uint16_t _port, std::string _prefix = "/", bool block_this_thread = true) {
		if (m_server)
			m_server->stop ();
		m_base.reset ();
		m_server = std::make_unique<xfinal::http_server> (1);
		m_server->set_disable_auto_create_directories (true);
		m_server->set_session_storager (std::make_unique<_session_storage> ());
		std::string _s_port = std::to_string (_port);
		if (!m_server->listen ("0.0.0.0", _s_port)) {
			m_server = nullptr;
			return false;
		}
		m_server->router (_prefix, m_base.get_ref ());
		if (block_this_thread) {
			m_server->run ();
		} else {
			std::thread ([this] () {
				m_server->run ();
			}).detach ();
		}
		return true;
	}

	void stop () {
		m_base.reset ();
		m_server->stop ();
	}

private:
	std::unique_ptr<xfinal::http_server> m_server;
	im_server_base_t m_base;
};



#endif //__IM_SERVER_T_HPP__
