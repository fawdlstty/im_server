#ifndef __COMMON_T_HPP__
#define __COMMON_T_HPP__



#include <future>

#include "../taskpool/taskpool_t.hpp"



struct common_t {
	static fa::future_t<void> &&get_valued_future () {
		fa::promise_t<void> _promise;
		_promise.set_value ();
		return std::move (_promise.get_future ());
	}

	template<typename T>
	static fa::future_t<T> &&get_valued_future (T &&t) {
		fa::promise_t<T> _promise;
		_promise.set_value (std::move (t));
		return std::move (_promise.get_future ());
	}
};



#endif //__COMMON_T_HPP__
