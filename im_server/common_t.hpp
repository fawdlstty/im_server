#ifndef __COMMON_T_HPP__
#define __COMMON_T_HPP__



#include <future>



struct common_t {
	static std::future<void> &&get_valued_future () {
		std::promise<void> _promise;
		_promise.set_value ();
		return std::move (_promise.get_future ());
	}

	template<typename T>
	static std::future<T> &&get_valued_future (T &&t) {
		std::promise<T> _promise;
		_promise.set_value (std::move (t));
		return std::move (_promise.get_future ());
	}
};



#endif //__COMMON_T_HPP__
