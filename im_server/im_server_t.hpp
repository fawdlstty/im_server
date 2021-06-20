#ifndef __IM_SERVER_T_HPP__
#define __IM_SERVER_T_HPP__



#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <tuple>

#include "../taskpool/taskpool_t.hpp"
#include "../xfinal/xfinal/xfinal.hpp"
#include "im_connect_t.hpp"



class im_server_t {
	class _session_storage: public xfinal::session_storage {
	public:
		bool init () { return true; }
		void save (xfinal::session &session) {}
		void remove (xfinal::session &session) {}
		void remove (std::string const &uuid) {}
	};

	enum class _find_type_t { stay, remove };

public:
	im_server_t (size_t _io_thread_num, size_t _process_thread_num): m_io_thread_num (_io_thread_num), m_tpool (_process_thread_num) {
		m_event.on ("open", [this] (xfinal::websocket &ws) {
			if (!m_open_cb)
				return;
			auto _conn = std::make_shared<im_connect_t> (ws.shared_from_this ());
			std::optional<int64_t> _ouid = m_open_cb (_conn);
			if (_ouid.has_value ()) {
				int64_t _uid = _ouid.value ();
				_add_connect (_uid, _conn);
			} else {
				_conn->_set_uid (-1);
				fa::future_t<bool> &&_fut = _conn->send_string (R"({"type":"auth","result":false,"reason":"auth fail"})");
				m_tpool.async_after_run (std::move (_fut), [_conn] (bool &&) { _conn->close (); });
			}
		});
		m_event.on ("message", [this] (xfinal::websocket &ws) {
			std::optional<std::shared_ptr<im_connect_t>> _conn = _get_connect (ws, _find_type_t::stay);
			if (!_conn.has_value ())
				return;
			if (ws.message_code () == 1) {
				std::string _recv = std::string (ws.messages ());
				m_tpool.async_run (m_string_message_cb, _conn.value (), _recv);
			} else if (ws.message_code () == 2) {
				std::string_view _view = ws.messages ();
				span_t<uint8_t> _v { (uint8_t *) _view.data (), _view.size () };
				m_tpool.async_run (m_binary_message_cb, _conn.value (), _v);
			}
		});
		m_event.on ("close", [this] (xfinal::websocket &ws) {
			std::optional<std::shared_ptr<im_connect_t>> _conn = _get_connect (ws, _find_type_t::remove);
			if (!_conn.has_value ())
				return;
			if (m_close_cb)
				m_tpool.async_run (m_close_cb, _conn.value ()->get_uid ());
		});
	}

	fa::future_t<bool> send_client_string (int64_t _uid, const std::string &_data) {
		auto _conn = get_connect (_uid);
		if (!_conn.has_value ()) {
			return fa::future_t<bool>::from_value (false);
		} else {
			return _conn.value ()->send_string (_data);
		}
	}
	fa::future_t<bool> send_client_binary (int64_t _uid, span_t<uint8_t> _data) {
		auto _conn = get_connect (_uid);
		if (!_conn.has_value ()) {
			return fa::future_t<bool>::from_value (false);
		} else {
			return _conn.value ()->send_binary (_data);
		}
	}
	std::optional<std::tuple<std::string, uint16_t>> get_client_remote_info (int64_t _uid) {
		auto _conn = get_connect (_uid);
		return _conn.has_value () ? std::make_optional (_conn.value ()->remote_info ()) : std::nullopt;
	}
	void close_client (int64_t _uid) {
		auto _conn = get_connect (_uid);
		if (_conn.has_value ())
			_conn.value ()->close ();
	}

	fa::future_t<int> send_all_client_string (const std::string &_data) {
		std::unique_lock<std::recursive_mutex> ul2 { m_mtx_uid };
		std::vector<int64_t> _v;
		_v.assign (m_conn_uids.begin (), m_conn_uids.end ());
		ul2.unlock ();
		std::vector<fa::future_t<bool>> _vfut;
		_vfut.reserve (_v.size ());
		for (int64_t _uid : _v) {
			auto _conn = get_connect (_uid);
			if (_conn.has_value ())
				_vfut.emplace_back (_conn.value ()->send_string (_data));
		}
		fa::future_t<std::vector<bool>> _fut0 = m_tpool.async_wait_all (std::move (_vfut));
		fa::future_t<int> _fut1 = m_tpool.async_after_run (std::move (_fut0), [] (std::vector<bool> &&_vsend) -> int {
			int _count = 0;
			for (bool _b : _vsend)
				_count += _b ? 1 : 0;
			return _count;
		});
		return _fut1;
	}
	fa::future_t<int> send_all_client_binary (span_t<uint8_t> _data) {
		std::unique_lock<std::recursive_mutex> ul2 { m_mtx_uid };
		std::vector<int64_t> _v;
		_v.assign (m_conn_uids.begin (), m_conn_uids.end ());
		ul2.unlock ();
		std::vector<fa::future_t<bool>> _vfut;
		_vfut.reserve (_v.size ());
		for (int64_t _uid : _v) {
			auto _conn = get_connect (_uid);
			if (_conn.has_value ())
				_vfut.emplace_back (_conn.value ()->send_binary (_data));
		}
		fa::future_t<std::vector<bool>> _fut0 = m_tpool.async_wait_all (std::move (_vfut));
		fa::future_t<int> _fut1 = m_tpool.async_after_run (std::move (_fut0), [] (std::vector<bool> &&_vsend) -> int {
			int _count = 0;
			for (bool _b : _vsend)
				_count += _b ? 1 : 0;
			return _count;
		});
		return _fut1;
	}
	void close_all_client () {
		std::unique_lock<std::recursive_mutex> ul { m_mtx };
		while (!m_conns.empty ())
			m_conns.begin ()->second->close ();
	}

	std::string get_online_clients () {
		std::stringstream _ss;
		std::unique_lock<std::recursive_mutex> ul2 { m_mtx_uid };
		for (int64_t _uid : m_conn_uids)
			_ss << _uid << ',';
		ul2.unlock ();
		std::string _ret = _ss.str ();
		if (!_ret.empty ())
			_ret.erase (_ret.begin () + _ret.size () - 1);
		return _ret;
	}

	bool init (uint16_t _port, std::string _prefix = "/") {
		if (m_server)
			m_server->stop ();
		m_server = std::make_shared<xfinal::http_server> (m_io_thread_num);
		m_server->set_disable_auto_create_directories (true);
		m_server->set_session_storager (std::make_unique<_session_storage> ());
		std::string _s_port = std::to_string (_port);
		if (!m_server->listen ("0.0.0.0", _s_port)) {
			m_server = nullptr;
			return false;
		}
		m_server->router (_prefix, m_event);
		return true;
	}

	std::shared_ptr<xfinal::http_server> get_server () { return m_server; }

	std::optional<std::shared_ptr<im_connect_t>> get_connect (int64_t _uid) {
		return _get_connect (_uid, _find_type_t::stay);
	}

	void run (bool block_this_thread = true) {
		if (block_this_thread) {
			m_server->run ();
		} else {
			std::thread ([this] () {
				m_server->run ();
			}).detach ();
		}
	}

	void stop () {
		close_all_client ();
		m_server->stop ();
	}

private:
	void _add_connect (int64_t _uid, std::shared_ptr<im_connect_t> _conn) {
		_conn->_set_uid (_uid);
		std::unique_lock<std::recursive_mutex> ul { m_mtx, std::defer_lock };
		ul.lock ();
		m_conns [_uid] = _conn;
		ul.unlock ();
		//
		std::unique_lock<std::recursive_mutex> ul2 { m_mtx_uid, std::defer_lock };
		ul2.lock ();
		auto _iter = std::lower_bound (m_conn_uids.begin (), m_conn_uids.end (), _uid);
		if (_iter == m_conn_uids.end ()) {
			m_conn_uids.push_back (_uid);
		} else if (*_iter != _uid) {
			m_conn_uids.insert (_iter, _uid);
		}
		ul2.unlock ();
	}
	std::optional<std::shared_ptr<im_connect_t>> _get_connect (xfinal::websocket &ws, _find_type_t _find_type) {
		int64_t _uid = *ws.get_user_data<std::shared_ptr<int64_t>> ("uid");
		if (_uid == -1)
			return std::nullopt;
		return (_get_connect (_uid, _find_type));
	}
	std::optional<std::shared_ptr<im_connect_t>> _get_connect (int64_t _uid, _find_type_t _find_type) {
		std::unique_lock<std::recursive_mutex> ul { m_mtx };
		auto _p = m_conns.find (_uid);
		if (_p == m_conns.end ())
			return std::nullopt;
		auto _ret = _p->second;
		if (_find_type == _find_type_t::remove) {
			m_conns.erase (_p);
			ul.unlock ();
			//
			std::unique_lock<std::recursive_mutex> ul2 { m_mtx_uid, std::defer_lock };
			ul2.lock ();
			auto _iter = std::lower_bound (m_conn_uids.begin (), m_conn_uids.end (), _uid);
			if (*_iter == _uid)
				m_conn_uids.erase (_iter);
			ul2.unlock ();
		}
		return _ret;
	}

	size_t																	m_io_thread_num;
	std::shared_ptr<xfinal::http_server>									m_server;
	xfinal::websocket_event													m_event;
	std::recursive_mutex													m_mtx;
	std::map<int64_t, std::shared_ptr<im_connect_t>>						m_conns;
	std::recursive_mutex													m_mtx_uid;
	std::vector<int64_t>													m_conn_uids;
	int64_t																	m_inc_uid = 0;

public:
	fa::taskpool_t															m_tpool;
	std::function<std::optional<int64_t> (std::shared_ptr<im_connect_t>)>	m_open_cb;
	std::function<void (std::shared_ptr<im_connect_t>, std::string)>		m_string_message_cb;
	std::function<void (std::shared_ptr<im_connect_t>, span_t<uint8_t>)>	m_binary_message_cb;
	std::function<void (int64_t)>											m_close_cb;
};



#endif //__IM_SERVER_T_HPP__
