#pragma once
#include <vector>
#include <algorithm>
#include <initializer_list>
#include "vector.h"

namespace ist {

// flat set (aka sorted vector)
template <
    class Key,
    class Compare = std::less<>,
    class Container = std::vector<Key, std::allocator<Key>>
>
class basic_set
{
public:
    using key_type               = Key;
    using value_type             = Key;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;
    using key_compare            = Compare;
    using value_compare          = Compare;
    using reference              = Key&;
    using const_reference        = const Key&;
    using pointer                = Key*;
    using const_pointer          = const Key*;
    using container_type         = Container;
    using iterator               = typename container_type::iterator;
    using const_iterator         = typename container_type::const_iterator;


    basic_set() {}
    basic_set(const basic_set& v) { operator=(v); }
    basic_set(const container_type& v) { operator=(v); }
    basic_set(basic_set&& v) noexcept { operator=(std::move(v)); }
    basic_set(container_type&& v) noexcept { operator=(std::move(v)); }

    template <class Iter, bool cond = !has_remote_memory_v<container_type> && is_iterator_of_v<Iter, value_type>, fc_require(cond)>
    basic_set(Iter first, Iter last)
    {
        insert(first, last);
    }
    template <bool cond = !has_remote_memory_v<container_type>, fc_require(cond)>
    basic_set(std::initializer_list<value_type> list)
    {
        insert(list);
    }

    template<bool cond = has_remote_memory_v<container_type>, fc_require(cond)>
    basic_set(void* data, size_t capacity, size_t size = 0)
        : data_(data, capacity, size)
    {
    }

    basic_set& operator=(const basic_set& v)
    {
        data_ = v.data_;
        return *this;
    }
    basic_set& operator=(const container_type& v)
    {
        data_ = v.data_;
        _sort();
        return *this;
    }

    basic_set& operator=(basic_set&& v) noexcept
    {
        swap(v);
        return *this;
    }
    basic_set& operator=(container_type&& v) noexcept
    {
        swap(v);
        return *this;
    }

    void swap(basic_set& v) noexcept
    {
        data_.swap(v.data_);
    }
    void swap(container_type& v) noexcept
    {
        data_.swap(v);
        _sort();
    }

    const container_type& get() const { return data_; }
    container_type&& extract() { return std::move(data_); }

    bool operator==(const basic_set& v) const { return data_ == v.data_; }
    bool operator!=(const basic_set& v) const { return data_ != v.data_; }


    void reserve(size_type v) { data_.reserve(v); }
    void clear() { data_.clear(); }
    void shrink_to_fit() { data_.shrink_to_fit(); }

    size_type empty() const noexcept { return data_.empty(); }
    size_type size() const noexcept { return data_.size(); }
    size_type capacity() const noexcept { return data_.capacity(); }

    // data() / cdata()
    pointer data() noexcept { return data_.data(); }
    const_pointer data() const noexcept { return data_.data(); }
    const_pointer cdata() const noexcept { return data_.data(); }

    // begin() / cbegin()
    iterator begin() noexcept { return data_.begin(); }
    const_iterator begin() const noexcept { return data_.begin(); }
    constexpr const_iterator cbegin() const noexcept { return data_.cbegin(); }

    // end() / cend()
    iterator end() noexcept { return data_.end(); }
    const_iterator end() const noexcept { return data_.end(); }
    constexpr const_iterator cend() const noexcept { return data_.cend(); }


    // as_const()
    constexpr const basic_set& as_const() const { return *this; }

    // lower_bound()
    iterator lower_bound(const value_type& k)
    {
        return std::lower_bound(begin(), end(), k, key_compare());
    }
    const_iterator lower_bound(const value_type& k) const
    {
        return std::lower_bound(begin(), end(), k, key_compare());
    }
    template <class K>
    iterator lower_bound(const K& k)
    {
        return std::lower_bound(begin(), end(), k, key_compare());
    }
    template <class K>
    const_iterator lower_bound(const K& k) const
    {
        return std::lower_bound(begin(), end(), k, key_compare());
    }

    // upper_bound()
    iterator upper_bound(const value_type& k)
    {
        return std::upper_bound(begin(), end(), k, key_compare());
    }
    const_iterator upper_bound(const value_type& k) const
    {
        return std::upper_bound(begin(), end(), k, key_compare());
    }
    template <class K>
    iterator upper_bound(const K& k)
    {
        return std::upper_bound(begin(), end(), k, key_compare());
    }
    template <class K>
    const_iterator upper_bound(const K& k) const
    {
        return std::upper_bound(begin(), end(), k, key_compare());
    }

    // equal_range()
    std::pair<iterator, iterator> equal_range(const value_type& k)
    {
        return std::equal_range(begin(), end(), k, key_compare());
    }
    std::pair<const_iterator, const_iterator> equal_range(const value_type& k) const
    {
        return std::equal_range(begin(), end(), k, key_compare());
    }
    template <class K>
    std::pair<iterator, iterator> equal_range(const K& k)
    {
        return std::equal_range(begin(), end(), k, key_compare());
    }
    template <class K>
    std::pair<const_iterator, const_iterator> equal_range(const K& k) const
    {
        return std::equal_range(begin(), end(), k, key_compare());
    }

    // find()
    iterator find(const value_type& k)
    {
        auto it = lower_bound(k);
        return (it != end() && _equals(*it, k)) ? it : end();
    }
    const_iterator find(const value_type& k) const
    {
        auto it = lower_bound(k);
        return (it != end() && _equals(*it, k)) ? it : end();
    }
    template <class K>
    iterator find(const K& k)
    {
        auto it = lower_bound(k);
        return (it != end() && _equals(*it, k)) ? it : end();
    }
    template <class K>
    const_iterator find(const K& k) const
    {
        auto it = lower_bound(k);
        return (it != end() && _equals(*it, k)) ? it : end();
    }

    // count()
    size_t count(const value_type& k) const
    {
        return _count_keys(equal_range(k), k);
    }
    template <class K>
    size_t count(const K& k) const
    {
        return _count_keys(equal_range(k), k);
    }

    // contains()
    bool contains(const value_type& k) const
    {
        return find(k) != end();
    }
    template<class K>
    bool contains(const K& k) const
    {
        return find(k) != end();
    }


    // insert()
    std::pair<iterator, bool> insert(const value_type& v)
    {
        auto it = lower_bound(v);
        if (it == end() || !_equals(*it, v)) {
            return { data_.insert(it, v), true };
        }
        else {
            return { it, false };
        }
    }
    std::pair<iterator, bool> insert(value_type&& v)
    {
        auto it = lower_bound(v);
        if (it == end() || !_equals(*it, v)) {
            return { data_.insert(it, v), true };
        }
        else {
            return { it, false };
        }
    }
    template<class Iter, fc_require(is_iterator_of_v<Iter, value_type>)>
    void insert(Iter first, Iter last)
    {
        for (auto i = first; i != last; ++i) {
            insert(*i);
        }
    }
    void insert(std::initializer_list<value_type> list)
    {
        insert(list.begin(), list.end());
    }

    // erase()
    iterator erase(const value_type& v)
    {
        if (auto it = find(v); it != end()) {
            return data_.erase(it);
        }
        else {
            return end();
        }
    }
    iterator erase(const_iterator pos)
    {
        return data_.erase(pos);
    }
    iterator erase(const_iterator first, const_iterator last)
    {
        return data_.erase(first, last);
    }

private:
    void _sort() { std::sort(begin(), end(), key_compare()); }

    template<class IterPair, class K>
    static size_t _count_keys(const IterPair& pair, const K& key)
    {
        size_t r = 0;
        for (auto first = pair.first; first != pair.second; ++first) {
            if (*first == key) {
                ++r;
            }
        }
        return r;
    }

    static bool _equals(const value_type& a, const value_type& b)
    {
        return !Compare()(a, b) && !Compare()(b, a);
    }
    template <class K1, class V2>
    static bool _equals(const K1& a, const V2& b)
    {
        return !Compare()(a, b) && !Compare()(b, a);
    }

private:
    container_type data_;
};

template<class K, class Comp, class Cont1, class Cont2>
bool operator==(const basic_set<K, Comp, Cont1>& l, const basic_set<K, Comp, Cont2>& r)
{
    return l.size() == r.size() && std::equal(l.begin(), l.end(), r.begin());
}
template<class K, class Comp, class Cont1, class Cont2>
bool operator!=(const basic_set<K, Comp, Cont1>& l, const basic_set<K, Comp, Cont2>& r)
{
    return l.size() != r.size() || !std::equal(l.begin(), l.end(), r.begin());
}
template<class K, class Comp, class Cont1, class Cont2>
bool operator<(const basic_set<K, Comp, Cont1>& l, const basic_set<K, Comp, Cont2>& r)
{
    return std::lexicographical_compare(l.begin(), l.end(), r.begin(), r.end());
}
template<class K, class Comp, class Cont1, class Cont2>
bool operator>(const basic_set<K, Comp, Cont1>& l, const basic_set<K, Comp, Cont2>& r)
{
    return r < l;
}
template<class K, class Comp, class Cont1, class Cont2>
bool operator<=(const basic_set<K, Comp, Cont1>& l, const basic_set<K, Comp, Cont2>& r)
{
    return !(r < l);
}
template<class K, class Comp, class Cont1, class Cont2>
bool operator>=(const basic_set<K, Comp, Cont1>& l, const basic_set<K, Comp, Cont2>& r)
{
    return !(l < r);
}


template <class Key, class Compare = std::less<>, class Allocator = std::allocator<Key>>
using flat_set = basic_set<Key, Compare, std::vector<Key, Allocator>>;

template <class Key, size_t Capacity, class Compare = std::less<>>
using fixed_set = basic_set<Key, Compare, fixed_vector<Key, Capacity>>;

template <class Key, size_t Capacity, class Compare = std::less<>, class Allocator = std::allocator<Key>>
using small_set = basic_set<Key, Compare, small_vector<Key, Capacity, Allocator>>;

template <class Key, class Compare = std::less<>>
using remote_set = basic_set<Key, Compare, remote_vector<Key>>;

template <class Key, class Compare = std::less<>, class Allocator = std::allocator<Key>>
using shared_set = basic_set<Key, Compare, shared_vector<Key, Allocator>>;

} // namespace ist
