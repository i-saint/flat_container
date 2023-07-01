#pragma once
#include "foundation.h"

namespace ist {

template<class T, class Memory>
class basic_raw_vector : public Memory
{
using super = Memory;
public:
    // T must be pod
    static_assert(is_pod_v<T>);

    constexpr basic_raw_vector() = default;
    constexpr basic_raw_vector(const basic_raw_vector& r) = default;
    constexpr basic_raw_vector(basic_raw_vector&& r) noexcept = default;
    constexpr basic_raw_vector& operator=(const basic_raw_vector& r) = default;
    constexpr basic_raw_vector& operator=(basic_raw_vector&& r) noexcept = default;

    template<bool view = is_memory_view_v<super>, fc_require(!view)>
    constexpr explicit basic_raw_vector(size_t n) { resize(n); }

    template<bool view = is_memory_view_v<super>, fc_require(!view)>
    constexpr basic_raw_vector(size_t n, const T& v) { resize(n, v); }

    template<bool view = is_memory_view_v<super>, fc_require(!view)>
    constexpr basic_raw_vector(std::initializer_list<T> r) { assign(r); }

    template<class Iter, bool view = is_memory_view_v<super>, fc_require(!view), fc_require(is_iterator_v<Iter>)>
    constexpr basic_raw_vector(Iter first, Iter last) { assign(first, last); }

    template<bool view = is_memory_view_v<super>, fc_require(view)>
    constexpr basic_raw_vector(void* data, size_t capacity, size_t size = 0)
        : super(data, capacity, size)
    {
    }

    using super::capacity;
    using super::size;
    using super::size_bytes;
    using super::data;
    using super::reserve;
    using super::shrink_to_fit;
    using super::clear;

    using super::empty;
    using super::full;
    using super::begin;
    using super::cbegin;
    using super::end;
    using super::cend;
    using super::at;
    using super::operator[];
    using super::front;
    using super::back;

    constexpr void resize(size_t n)
    {
        _resize(n, [&](T*) {}); // new elements are uninitialized
    }
    constexpr void resize(size_t n, const T& v)
    {
        _resize(n, [&](T* addr) { *addr = v; });
    }

    constexpr void push_back(const T& v)
    {
        _expand(1, [&](T* addr) { *addr = v; });
    }

    template< class... Args >
    constexpr T& emplace_back(Args&&... args)
    {
        _expand(1, [&](T* addr) { *addr = T(std::forward<Args>(args)...); });
        return back();
    }

    constexpr void pop_back()
    {
        _shrink(1);
    }

    template<class Iter, fc_require(is_iterator_v<Iter>)>
    constexpr void assign(Iter first, Iter last)
    {
        size_t n = std::distance(first, last);
        _assign(n, [&](T* dst) { _copy_range(dst, first, last); });
    }
    constexpr void assign(std::initializer_list<T> list)
    {
        _assign(list.size(), [&](T* dst) { _copy_range(dst, list.begin(), list.end()); });
    }
    constexpr void assign(size_t n, const T& v)
    {
        _assign(n, [&](T* dst) { _copy_n(dst, v, n); });
    }

    template<class Iter, fc_require(is_iterator_v<Iter>)>
    constexpr T* insert(T* pos, Iter first, Iter last)
    {
        size_t n = std::distance(first, last);
        return _insert(pos, n, [&](T* addr) { _copy_range(addr, first, last); });
    }
    constexpr T* insert(T* pos, std::initializer_list<T> list)
    {
        return _insert(pos, list.size(), [&](T* addr) { _copy_range(addr, list.begin(), list.end()); });
    }
    constexpr T* insert(T* pos, const T& v)
    {
        return _insert(pos, 1, [&](T* addr) { _copy_n(addr, v, 1); });
    }
    template< class... Args >
    constexpr T* emplace(T* pos, Args&&... args)
    {
        return _insert(pos, 1, [&](T* addr) { _emplace_one(addr, std::forward<Args>(args)...); });
    }

    constexpr T* erase(T* first, T* last)
    {
        _copy_range(first, last, end());
        _shrink(std::distance(first, last));
        return first;
    }
    constexpr T* erase(T* pos)
    {
        return erase(pos, pos + 1);
    }

protected:
    using super::_copy_range;
    using super::_copy_n;
    using super::_move_range;
    using super::_move_one;
    using super::_emplace_one;

    using super::_shrink;
    using super::_expand;
    using super::_resize;
    using super::_insert;
    using super::_assign;
};

template<class T, class M1, class M2>
bool operator==(const basic_raw_vector<T, M1>& l, const basic_raw_vector<T, M2>& r)
{
    return l.size() == r.size() && std::memcmp(l.data(), r.data(), l.size_bytes()) == 0;
}
template<class T, class M1, class M2>
bool operator!=(const basic_raw_vector<T, M1>& l, const basic_raw_vector<T, M2>& r)
{
    return l.size() != r.size() || std::memcmp(l.data(), r.data(), l.size_bytes()) != 0;
}
template<class T, class M1, class M2>
bool operator<(const basic_raw_vector<T, M1>& l, const basic_raw_vector<T, M2>& r)
{
    return std::lexicographical_compare(l.begin(), l.end(), r.begin(), r.end());
}
template<class T, class M1, class M2>
bool operator>(const basic_raw_vector<T, M1>& l, const basic_raw_vector<T, M2>& r)
{
    return r < l;
}
template<class T, class M1, class M2>
bool operator<=(const basic_raw_vector<T, M1>& l, const basic_raw_vector<T, M2>& r)
{
    return !(r < l);
}
template<class T, class M1, class M2>
bool operator>=(const basic_raw_vector<T, M1>& l, const basic_raw_vector<T, M2>& r)
{
    return !(l < r);
}


template<class T>
using raw_vector = basic_raw_vector<T, dynamic_memory<T>>;

template<class T, size_t Capacity>
using fixed_raw_vector = basic_raw_vector<T, fixed_memory<T, Capacity>>;

template<class T, size_t Capacity>
using sbo_raw_vector = basic_raw_vector<T, sbo_memory<T, Capacity>>;

template<class T>
using raw_vector_view = basic_raw_vector<T, memory_view<T>>;

} // namespace ist


namespace std {

template<class T, class M>
void swap(ist::basic_raw_vector<T, M>& l, ist::basic_raw_vector<T, M>& r) noexcept
{
    l.swap(r);
}

} // namespace std