#ifndef __IM_CONNECT_T_HPP__
#define __IM_CONNECT_T_HPP__



#include <cstdint>

#include "../xfinal/xfinal/xfinal.hpp"



class im_connect_t {
public:
	im_connect_t (std::shared_ptr<xfinal::websocket> _ws, int64_t _cid): m_ws (_ws) {
		m_ws->set_user_data ("cid", std::make_shared<int64_t> (_cid));
	}

	void send_string (std::string _data) { m_ws->write_string (_data); }
	void send_binary (const uint8_t *_data, size_t _size) {
		std::string _tmp { (const char *) _data, _size };
		m_ws->write_binary (_tmp);
	}

private:
	std::shared_ptr<xfinal::websocket> m_ws;
};



#endif //__IM_CONNECT_T_HPP__
