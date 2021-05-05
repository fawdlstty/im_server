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

#include "../xfinal/xfinal/xfinal.hpp"
#include "../taskpool/taskpool_t.hpp"




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
	im_server_t (size_t _io_thread_num, size_t _process_thread_num): m_io_thread_num (m_io_thread_num), m_pool (_process_thread_num) {
		m_event.on ("open", [this] (xfinal::websocket &ws) {
			if (!m_open_callback)
				return;
			auto _connect_t = std::make_shared<im_connect_t> (ws.shared_from_this ());
			auto _fut = m_pool.async_run (m_open_callback, _connect_t);
			m_pool.async_after_run (std::move (_fut), [this, _connect_t] (std::optional<int64_t> _ocid) {
				if (_ocid.has_value ()) {
					int64_t _cid = _ocid.value ();
					_add_connect (_cid, _connect_t);
				} else {
					auto _fut = _connect_t->send_string ("[im_server] auth failed.");
					m_pool.async_after_run (std::move (_fut), [_connect_t] (bool) { _connect_t->close (); });
				}
			});
		});
		m_event.on ("message", [this] (xfinal::websocket &ws) {
			std::optional<std::shared_ptr<im_connect_t>> _conn = _get_connect (ws);
			if (!_conn.has_value ())
				return;
			if (ws.message_code () == 1) {
				std::string _recv = std::string (ws.messages ());
				m_pool.async_run (m_string_message_callback, _conn.value (), _recv);
			} else if (ws.message_code () == 2) {
				std::string_view _view = ws.messages ();
				std::vector<uint8_t> _v (_view.begin (), _view.end ());
				m_pool.async_run (m_binary_message_callback, _conn.value (), std::move (_v));
			}
		});
		m_event.on ("close", [this] (xfinal::websocket &ws) {
			std::optional<std::shared_ptr<im_connect_t>> _conn = _get_connect (ws, _find_type_t::remove);
			if (!_conn.has_value ())
				return;
			if (m_close_callback)
				m_pool.async_run (m_close_callback, _conn.value ()->get_cid ());
		});
	}

	void on_open_callback (std::function<std::optional<int64_t> (std::shared_ptr<im_connect_t>)> _callback) {
		m_open_callback = _callback;
	}
	void on_string_message_callback (std::function<void (std::shared_ptr<im_connect_t>, std::string)> _callback) {
		m_string_message_callback = _callback;
	}
	void on_binary_message_callback (std::function<void (std::shared_ptr<im_connect_t>, std::vector<uint8_t>)> _callback) {
		m_binary_message_callback = _callback;
	}
	void on_close_callback (std::function<void (int64_t)> _callback) {
		m_close_callback = _callback;
	}

	std::future<bool> &&send_client_string (int64_t _cid, const std::string &_data) {
		auto _conn = _get_connect (_cid);
		if (!_conn.has_value ()) {
			return common_t::get_valued_future (false);
		} else {
			return _conn.value ()->send_string (_data);
		}
	}
	std::future<bool> &&send_client_binary (int64_t _cid, const std::vector<uint8_t> &_data) {
		auto _conn = _get_connect (_cid);
		if (!_conn.has_value ()) {
			return common_t::get_valued_future (false);
		} else {
			return _conn.value ()->send_binary (_data);
		}
	}
	std::optional<std::tuple<std::string, uint16_t>> get_client_remote_info (int64_t _cid) {
		auto _conn = _get_connect (_cid);
		return _conn.has_value () ? std::make_optional (_conn.value ()->remote_info ()) : std::nullopt;
	}
	void close_client (int64_t _cid) {
		auto _conn = _get_connect (_cid);
		if (_conn.has_value ())
			_conn.value ()->close ();
	}

	std::future<int> &&send_all_client_string (const std::string &_data) {
		std::unique_lock<std::recursive_mutex> ul2 { m_mtx_cid };
		std::vector<int64_t> _v;
		_v.assign (m_conn_cids.begin (), m_conn_cids.end ());
		ul2.unlock ();
		std::vector<std::future<bool>> _vfut;
		_vfut.reserve (_v.size ());
		for (int64_t _cid : _v) {
			auto _conn = _get_connect (_cid);
			if (_conn.has_value ())
				_vfut.emplace_back (_conn.value ()->send_string (_data));
		}
		std::future<std::vector<bool>> _fut0 = m_pool.async_wait_all (std::move (_vfut));
		std::future<int> _fut1 = m_pool.async_after_run (std::move (_fut0), [] (std::vector<bool> _vsend) -> int {
			int _count = 0;
			for (bool _b : _vsend)
				_count += _b ? 1 : 0;
			return _count;
		});
		return std::move (_fut1);
	}
	std::future<int> &&send_all_client_binary (const std::vector<uint8_t> &_data) {
		std::unique_lock<std::recursive_mutex> ul2 { m_mtx_cid };
		std::vector<int64_t> _v;
		_v.assign (m_conn_cids.begin (), m_conn_cids.end ());
		ul2.unlock ();
		std::vector<std::future<bool>> _vfut;
		_vfut.reserve (_v.size ());
		for (int64_t _cid : _v) {
			auto _conn = _get_connect (_cid);
			if (_conn.has_value ())
				_vfut.emplace_back (_conn.value ()->send_binary (_data));
		}
		std::future<std::vector<bool>> _fut0 = m_pool.async_wait_all (std::move (_vfut));
		std::future<int> _fut1 = m_pool.async_after_run (std::move (_fut0), [] (std::vector<bool> _vsend) -> int {
			int _count = 0;
			for (bool _b : _vsend)
				_count += _b ? 1 : 0;
			return _count;
		});
		return std::move (_fut1);
	}
	void close_all_client () {
		std::unique_lock<std::recursive_mutex> ul { m_mtx };
		while (!m_conns.empty ())
			m_conns.begin ()->second->close ();
	}

	std::string get_online_clients () {
		std::stringstream _ss;
		std::unique_lock<std::recursive_mutex> ul2 { m_mtx_cid };
		for (int64_t _cid : m_conn_cids)
			_ss << _cid << ',';
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
	void _add_connect (int64_t _cid, std::shared_ptr<im_connect_t> _conn) {
		_conn->_set_cid (_cid);
		std::unique_lock<std::recursive_mutex> ul { m_mtx, std::defer_lock };
		ul.lock ();
		m_conns [_cid] = _conn;
		ul.unlock ();
		//
		std::unique_lock<std::recursive_mutex> ul2 { m_mtx_cid, std::defer_lock };
		ul2.lock ();
		auto _iter = std::lower_bound (m_conn_cids.begin (), m_conn_cids.end (), _cid);
		if (*_iter != _cid)
			m_conn_cids.insert (_iter, _cid);
		ul2.unlock ();
	}
	std::optional<std::shared_ptr<im_connect_t>> _get_connect (xfinal::websocket &ws, _find_type_t _find_type = _find_type_t::stay) {
		int64_t _cid = *ws.get_user_data<std::shared_ptr<int64_t>> ("cid");
		return (_get_connect (_cid, _find_type));
	}
	std::optional<std::shared_ptr<im_connect_t>> _get_connect (int64_t _cid, _find_type_t _find_type = _find_type_t::stay) {
		std::unique_lock<std::recursive_mutex> ul { m_mtx };
		auto _p = m_conns.find (_cid);
		if (_p == m_conns.end ())
			return std::nullopt;
		auto _ret = _p->second;
		if (_find_type == _find_type_t::remove) {
			m_conns.erase (_p);
			ul.unlock ();
			//
			std::unique_lock<std::recursive_mutex> ul2 { m_mtx_cid, std::defer_lock };
			ul2.lock ();
			auto _iter = std::lower_bound (m_conn_cids.begin (), m_conn_cids.end (), _cid);
			if (*_iter == _cid)
				m_conn_cids.erase (_iter);
			ul2.unlock ();
		}
		return _ret;
	}

	size_t																		m_io_thread_num;
	std::shared_ptr<xfinal::http_server>										m_server;
	xfinal::websocket_event														m_event;
	std::recursive_mutex														m_mtx;
	std::map<int64_t, std::shared_ptr<im_connect_t>>							m_conns;
	std::recursive_mutex														m_mtx_cid;
	std::vector<int64_t>														m_conn_cids;
	int64_t																		m_inc_cid = 0;
	taskpool_t																	m_pool;

	std::function<std::optional<int64_t> (std::shared_ptr<im_connect_t>)>		m_open_callback;
	std::function<void (std::shared_ptr<im_connect_t>, std::string)>			m_string_message_callback;
	std::function<void (std::shared_ptr<im_connect_t>, std::vector<uint8_t>)>	m_binary_message_callback;
	std::function<void (int64_t)>												m_close_callback;
};



#endif //__IM_SERVER_T_HPP__
