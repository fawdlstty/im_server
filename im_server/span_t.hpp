#ifndef __SPAN_T_HPP__
#define __SPAN_T_HPP__



#include <string_view>



template<typename T>
class span_t {
public:
	span_t (T *_ptr, size_t _size): m_ptr (_ptr), m_size (_size) {}
	span_t (std::string_view _view): m_ptr ((T *) _view.data ()), m_size (_view.size () / sizeof (T)) {}
	T *data () { return m_ptr; }
	size_t size () { return m_size; }
	T &operator[] (size_t _i) { return m_ptr [_i]; }

private:
	T		*m_ptr;
	size_t	m_size;
};



#endif //__SPAN_T_HPP__
