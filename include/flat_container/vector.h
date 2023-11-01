#pragma once
#include "vector_base.h"

namespace ist {

template<class T, class Memory>
class basic_vector : public vector_base<Memory>
{
using super = vector_base<Memory>;
public:
    using typename super::value_type;
    using typename super::pointer;
    using typename super::const_pointer;
    using typename super::reference;
    using typename super::const_reference;
    using typename super::iterator;
    using typename super::const_iterator;
    using typename super::memory_type;

    constexpr basic_vector() = default;
    constexpr basic_vector(basic_vector&& r) noexcept = default;
    constexpr basic_vector& operator=(basic_vector&& r) noexcept = default;
    constexpr basic_vector(const basic_vector& r) = default;
    constexpr basic_vector& operator=(const basic_vector& r) = default;

    template<bool cond = !has_remote_memory_v<super>, fc_require(cond)>
    constexpr explicit basic_vector(size_t n) { resize(n); }

    template<bool cond = !has_remote_memory_v<super>, fc_require(cond)>
    constexpr basic_vector(size_t n, const_reference v) { resize(n, v); }

    template<bool cond = !has_remote_memory_v<super>, fc_require(cond)>
    constexpr basic_vector(std::initializer_list<value_type> r) { assign(r); }

    template<class Iter, bool cond = !has_remote_memory_v<super> && is_iterator_of_v<Iter, value_type>, fc_require(cond)>
    constexpr basic_vector(Iter first, Iter last) { assign(first, last); }

    template<bool cond = has_remote_memory_v<super>, fc_require(cond)>
    constexpr basic_vector(const void* data, size_t capacity, size_t size = 0)
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
    using super::begin;
    using super::cbegin;
    using super::end;
    using super::cend;
    using super::at;
    using super::operator[];
    using super::front;
    using super::back;

    // as_const()
    constexpr const basic_vector& as_const() const { return *this; }

    // resize()
    constexpr void resize(size_t n)
    {
        _copy_on_write();
        _resize(n, [&](pointer addr) { _construct_at(addr); });
    }
    constexpr void resize(size_t n, const_reference v)
    {
        _copy_on_write();
        _resize(n, [&](pointer addr) { _construct_at(addr, v); });
    }

    // push_back()
    constexpr void push_back(const_reference v)
    {
        _copy_on_write();
        _expand(1, [&](pointer addr) { _construct_at(addr, v); });
    }
    constexpr void push_back(value_type&& v)
    {
        _copy_on_write();
        _expand(1, [&](pointer addr) { _construct_at(addr, std::move(v)); });
    }

    // emplace_back()
    template< class... Args >
    constexpr reference emplace_back(Args&&... args)
    {
        _copy_on_write();
        _expand(1, [&](pointer addr) { _construct_at(addr, std::forward<Args>(args)...); });
        return back();
    }

    // pop_back()
    constexpr void pop_back()
    {
        _copy_on_write();
        _shrink(1);
    }

    // assign()
    template<class Iter, fc_require(is_iterator_of_v<Iter, value_type>)>
    constexpr void assign(Iter first, Iter last)
    {
        _copy_on_write();
        size_t n = std::distance(first, last);
        _assign(n, [&](pointer dst) { _copy_range(first, last, dst); });
    }
    constexpr void assign(std::initializer_list<value_type> list)
    {
        _copy_on_write();
        _assign(list.size(), [&](pointer dst) { _copy_range(list.begin(), list.end(), dst); });
    }
    constexpr void assign(size_t n, const_reference v)
    {
        _copy_on_write();
        _assign(n, [&](pointer dst) { _fill_range(dst, n, v); });
    }

    // assign_range()
    template<class Container>
    constexpr void insert_range(Container&& v)
    {
        _copy_on_write();
        _assign(std::size(v), [&](pointer dst) { _move_range(std::begin(v), std::end(v), dst); });
    }

    // insert()
    template<class Iter, fc_require(is_iterator_of_v<Iter, value_type>)>
    constexpr iterator insert(const_iterator pos, Iter first, Iter last)
    {
        _copy_on_write();
        size_t n = std::distance(first, last);
        return _insert(pos, n, [&](pointer dst) { _copy_range( first, last, dst); });
    }
    constexpr iterator insert(const_iterator pos, std::initializer_list<value_type> list)
    {
        _copy_on_write();
        return _insert(pos, list.size(), [&](pointer dst) { _copy_range(list.begin(), list.end(), dst); });
    }
    constexpr iterator insert(const_iterator pos, const_reference v)
    {
        _copy_on_write();
        return _insert(pos, 1, [&](pointer dst) { _fill_range(dst, 1, v); });
    }
    constexpr iterator insert(const_iterator pos, value_type&& v)
    {
        _copy_on_write();
        return _insert(pos, 1, [&](pointer dst) { _move_one(dst, std::move(v)); });
    }

    // insert_range()
    template<class Container>
    constexpr iterator insert_range(const_iterator pos, Container&& v)
    {
        _copy_on_write();
        return _insert(pos, std::size(v), [&](pointer dst) { _move_range(std::begin(v), std::end(v), dst); });
    }

    // emplace()
    template< class... Args >
    constexpr iterator emplace(const_iterator pos, Args&&... args)
    {
        _copy_on_write();
        return _insert(pos, 1, [&](pointer dst) { _emplace_one(dst, std::forward<Args>(args)...); });
    }

    // erase()
    constexpr iterator erase(const_iterator first, const_iterator last)
    {
        _copy_on_write();
        size_t s = std::distance(first, last);
        std::move(iterator(last), end(), iterator(first));
        _shrink(s);
        return iterator(first);
    }
    constexpr iterator erase(const_iterator pos)
    {
        _copy_on_write();
        return erase(pos, pos + 1);
    }

protected:
    using super::_data;
    using super::_size;
    using super::_capacity;
    using super::_copy_on_write;

    using super::_copy_range;
    using super::_fill_range;
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
inline bool operator==(const basic_vector<T, M1>& l, const basic_vector<T, M2>& r)
{
    return l.size() == r.size() && std::equal(l.begin(), l.end(), r.begin());
}
template<class T, class M1, class M2>
inline bool operator!=(const basic_vector<T, M1>& l, const basic_vector<T, M2>& r)
{
    return l.size() != r.size() || !std::equal(l.begin(), l.end(), r.begin());
}
template<class T, class M1, class M2>
inline bool operator<(const basic_vector<T, M1>& l, const basic_vector<T, M2>& r)
{
    return std::lexicographical_compare(l.begin(), l.end(), r.begin(), r.end());
}
template<class T, class M1, class M2>
inline bool operator>(const basic_vector<T, M1>& l, const basic_vector<T, M2>& r)
{
    return r < l;
}
template<class T, class M1, class M2>
inline bool operator<=(const basic_vector<T, M1>& l, const basic_vector<T, M2>& r)
{
    return !(r < l);
}
template<class T, class M1, class M2>
inline bool operator>=(const basic_vector<T, M1>& l, const basic_vector<T, M2>& r)
{
    return !(l < r);
}


template<class T, class Allocator = std::allocator<T>>
using vector = basic_vector<T, dynamic_memory<T, Allocator>>;

template<class T, size_t Capacity>
using fixed_vector = basic_vector<T, fixed_memory<T, Capacity>>;

template<class T, size_t Capacity, class Allocator = std::allocator<T>>
using small_vector = basic_vector<T, small_memory<T, Capacity, Allocator>>;

template<class T>
using remote_vector = basic_vector<T, remote_memory<T>>;

template<class T, class Allocator = std::allocator<T>>
using shared_vector = basic_vector<T, shared_memory<T, Allocator>>;

} // namespace ist
