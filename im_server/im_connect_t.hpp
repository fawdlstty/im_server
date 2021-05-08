#ifndef __IM_CONNECT_T_HPP__
#define __IM_CONNECT_T_HPP__



#include <cstdint>
#include <future>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "../taskpool/taskpool_t.hpp"
#include "../xfinal/xfinal/xfinal.hpp"
#include "common_t.hpp"



class im_connect_t {
public:
	im_connect_t (std::shared_ptr<xfinal::websocket> _ws): m_ws (_ws) {}

	bool is_connecting () { return m_ws->socket ().is_open (); }

	fa::future_t<bool> &&send_string (const std::string &_data) {
		if (!is_connecting ()) {
			return std::move (common_t::get_valued_future (false));
		} else {
			std::shared_ptr<fa::promise_t<bool>> _promise = std::make_shared<fa::promise_t<bool>> ();
			m_ws->write_string (_data, [_promise] (bool _success, std::error_code _ec) { _promise->set_value (_success); });
			return std::move (_promise->get_future ());
		}
	}

	fa::future_t<bool> &&send_binary (const std::vector<uint8_t> &_data) {
		if (!is_connecting ()) {
			return std::move (common_t::get_valued_future (false));
		} else {
			std::shared_ptr<fa::promise_t<bool>> _promise = std::make_shared<fa::promise_t<bool>> ();
			std::string _data2 (_data.begin (), _data.end ());
			m_ws->write_binary (_data2, [_promise] (bool _success, std::error_code _ec) { _promise->set_value (_success); });
			return std::move (_promise->get_future ());
		}
	}

	std::tuple<std::string, uint16_t> remote_info () {
		auto _ep = m_ws->socket ().remote_endpoint ();
		return { _ep.address ().to_string (), _ep.port () };
	}

	std::string get_param (std::string _key) { return m_ws->key_params ()[_key]; }

	void close () { m_ws->close (); }

	int64_t get_uid () { return m_uid; }

	void _set_uid (int64_t _uid) {
		m_uid = _uid;
		m_ws->set_user_data ("uid", std::make_shared<int64_t> (_uid));
	}

private:
	std::shared_ptr<xfinal::websocket> m_ws;
	int64_t m_uid = -1;
};



#endif //__IM_CONNECT_T_HPP__
