#ifndef __IM_SERVER_T_HPP__
#define __IM_SERVER_T_HPP__



#include <cstdint>
#include <memory>
#include <string>
#include <thread>

#include "../xfinal/xfinal/xfinal.hpp"

#include "im_base_t.hpp"



class _session_storage: public xfinal::session_storage {
public:
	bool init () { return true; }
	void save (xfinal::session &session) {}
	void remove (xfinal::session &session) {}
	void remove (std::string const &uuid) {}
};

class im_server_t {
public:
	bool start (uint16_t _port, std::string _prefix, bool block_this_thread = true) {
		if (m_server)
			m_server->stop ();
		m_base.reset ();
		m_server = std::make_unique<xfinal::http_server> (1);
		m_server->set_disable_auto_create_directories (true);
		m_server->set_session_storager (std::make_unique<_session_storage> ());
		std::string _sport = std::to_string (_port);
		if (!m_server->listen ("0.0.0.0", _sport)) {
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
	im_base_t m_base;
};



#endif //__IM_SERVER_T_HPP__
