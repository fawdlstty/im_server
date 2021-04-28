#ifndef __IM_CONNECT_T_HPP__
#define __IM_CONNECT_T_HPP__



#include <cstdint>
#include <string>
#include <tuple>

#include "../xfinal/xfinal/xfinal.hpp"



class im_connect_t {
public:
	im_connect_t (std::shared_ptr<xfinal::websocket> _ws): m_ws (_ws) {}

	bool is_connecting () { return m_ws->socket ().is_open (); }

	bool send_string (std::string _data) {
		if (!is_connecting ())
			return false;
		m_ws->write_string (_data);
		return true;
	}

	bool send_binary (const uint8_t *_data, size_t _size) {
		if (!is_connecting ())
			return false;
		std::string _tmp { (const char *) _data, _size };
		m_ws->write_binary (_tmp);
		return true;
	}

	std::tuple<std::string, uint16_t> remote_info () {
		auto _ep = m_ws->socket ().remote_endpoint ();
		return { _ep.address ().to_string (), _ep.port () };
	}

	std::string get_param (std::string _key) { return m_ws->key_params ()[_key]; }

	void close () { m_ws->close (); }

	int64_t get_cid () { return m_cid; }

	void _set_cid (int64_t _cid) {
		m_cid = _cid;
		m_ws->set_user_data ("cid", std::make_shared<int64_t> (_cid));
	}

private:
	std::shared_ptr<xfinal::websocket> m_ws;
	int64_t m_cid = -1;
};



#endif //__IM_CONNECT_T_HPP__
