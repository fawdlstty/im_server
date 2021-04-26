#ifndef __IM_SERVER_BASE_T_HPP__
#define __IM_SERVER_BASE_T_HPP__



#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>

#include "../xfinal/xfinal/xfinal.hpp"

#include "../taskpool/taskpool_t.hpp"
#include "im_connect_t.hpp"



class im_server_base_t {
public:
	im_server_base_t (size_t _threads): m_pool (std::min (std::max (_threads, (size_t) 1u), (size_t) std::thread::hardware_concurrency ())) {
		m_event.on ("open", [this] (xfinal::websocket &ws) {
			if (!m_open_callback)
				return;
			auto _connect_t = std::make_shared<im_connect_t> (ws.shared_from_this ());
			auto _fut = m_pool.run (m_open_callback, _connect_t);
			m_pool.after_run (std::move (_fut), [this, _connect_t] (std::optional<int64_t> _ocid) {
				if (_ocid.has_value ()) {
					int64_t _cid = _ocid.value ();
					std::unique_lock<std::recursive_mutex> ul { m_mtx };
					_connect_t->_set_cid (_cid);
					m_conns [_cid] = _connect_t;
				} else {
					_connect_t->close ();
				}
			});
		});
		m_event.on ("message", [this] (xfinal::websocket &ws) {
			std::unique_lock<std::recursive_mutex> ul { m_mtx };
			int64_t _cid = *ws.get_user_data<std::shared_ptr<int64_t>> ("cid");
			auto _p = m_conns.find (_cid);
			if (_p != m_conns.end ()) {
				if (ws.message_code () == 1) {
					std::string _recv = std::string (ws.messages ());
					m_string_message_callback (_p->second, _recv);
				} else {
					auto _recv = ws.messages ();
					m_pool.run (m_binary_message_callback, _p->second, (const uint8_t *) _recv.data (), _recv.size ());
				}
			}
		});
		m_event.on ("close", [this] (xfinal::websocket &ws) {
			std::unique_lock<std::recursive_mutex> ul { m_mtx };
			int64_t _cid = *ws.get_user_data<std::shared_ptr<int64_t>> ("cid");
			auto _p = m_conns.find (_cid);
			if (_p != m_conns.end ()) {
				if (m_close_callback) {
					m_pool.run (m_close_callback, _p->second);
				}
				m_conns.erase (_p);
			}
		});
	}

	void on_open_callback (std::function<std::optional<int64_t> (std::shared_ptr<im_connect_t>)> _callback) { m_open_callback = _callback; }
	void on_string_message_callback (std::function<void (std::shared_ptr<im_connect_t>, std::string)> _callback) { m_string_message_callback = _callback; }
	void on_binary_message_callback (std::function<void (std::shared_ptr<im_connect_t>, const uint8_t *, size_t)> _callback) { m_binary_message_callback = _callback; }
	void on_close_callback (std::function<void (std::shared_ptr<im_connect_t>)> _callback) { m_close_callback = _callback; }
	xfinal::websocket_event &get_ref () { return m_event; }
	void reset () {
		std::unique_lock<std::recursive_mutex> ul { m_mtx };
		if (m_close_callback) {
			for (auto _p = m_conns.begin (); _p != m_conns.end (); ++_p)
				_p->second->close ();
		}
		m_conns.clear ();
	}

	bool send_string (int64_t _cid, std::string _data) {
		std::unique_lock<std::recursive_mutex> ul { m_mtx };
		auto _p = m_conns.find (_cid);
		if (_p == m_conns.end ())
			return false;
		auto _conn = _p->second;
		ul.unlock ();
		return _conn->send_string (_data);
	}
	bool send_binary (int64_t _cid, const uint8_t *_data, size_t _size) {
		std::unique_lock<std::recursive_mutex> ul { m_mtx };
		auto _p = m_conns.find (_cid);
		if (_p == m_conns.end ())
			return false;
		auto _conn = _p->second;
		ul.unlock ();
		return _conn->send_binary (_data, _size);
	}

	std::string get_online_cids () {
		std::stringstream _ss;
		std::unique_lock<std::recursive_mutex> ul { m_mtx };
		_ss << '[';
		bool _first = true;
		for (auto _p = m_conns.begin (); _p != m_conns.end (); ++_p) {
			if (_first) {
				_first = false;
			} else {
				_ss << ',';
			}
			_ss << _p->first;
		}
		return _ss.str ();
	}

private:
	xfinal::websocket_event															m_event;
	std::recursive_mutex															m_mtx;
	std::map<int64_t, std::shared_ptr<im_connect_t>>								m_conns;
	int64_t																			m_inc_cid = 0;
	taskpool_t																		m_pool;

	std::function<std::optional<int64_t> (std::shared_ptr<im_connect_t>)>			m_open_callback;
	std::function<void (std::shared_ptr<im_connect_t>, std::string)>				m_string_message_callback;
	std::function<void (std::shared_ptr<im_connect_t>, const uint8_t *, size_t)>	m_binary_message_callback;
	std::function<void (std::shared_ptr<im_connect_t>)>								m_close_callback;
};



#endif //__IM_SERVER_BASE_T_HPP__
