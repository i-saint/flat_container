#pragma once
#include <vector>
#include <algorithm>
#include <utility>
#include <initializer_list>
#include "vector.h"

namespace ist {

// flat map (std::map-like sorted vector)
template <
    class Key,
    class Value,
    class Compare = std::less<>,
    class Container = std::vector<std::pair<Key, Value>, std::allocator<std::pair<Key, Value>>>
>
class basic_map
{
public:
    using key_type               = Key;
    using mapped_type            = Value;
    using value_type             = std::pair<const key_type, mapped_type>;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;
    using key_compare            = Compare;
    using reference              = value_type&;
    using const_reference        = const value_type&;
    using pointer                = value_type*;
    using const_pointer          = const value_type*;
    using container_type         = Container;
    using iterator               = typename container_type::iterator;
    using const_iterator         = typename container_type::const_iterator;

    basic_map() {}
    basic_map(basic_map&& v) noexcept { operator=(std::move(v)); }
    basic_map(container_type&& v) noexcept { operator=(std::move(v)); }
    basic_map(const basic_map& v) { operator=(v); }
    basic_map(const container_type& v) { operator=(v); }

    template <class Iter, bool cond = !has_remote_memory_v<container_type> && is_iterator_of_v<Iter, value_type>, fc_require(cond)>
    basic_map(Iter first, Iter last)
    {
        insert(first, last);
    }
    template <bool cond = !has_remote_memory_v<container_type>, fc_require(cond)>
    basic_map(std::initializer_list<value_type> list)
    {
        insert(list);
    }

    template<bool cond = has_remote_memory_v<container_type>, fc_require(cond)>
    basic_map(void* data, size_t capacity, size_t size = 0)
        : data_(data, capacity, size)
    {
    }

    basic_map& operator=(const basic_map& v)
    {
        data_ = v.data_;
        return *this;
    }
    basic_map& operator=(const container_type& v)
    {
        data_ = v.data_;
        _sort();
        return *this;
    }

    basic_map& operator=(basic_map&& v) noexcept
    {
        swap(v);
        return *this;
    }
    basic_map& operator=(container_type&& v) noexcept
    {
        swap(v);
        return *this;
    }

    void swap(basic_map& v) noexcept
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

    bool operator==(const basic_map& v) const noexcept { return data_ == v.data_; }
    bool operator!=(const basic_map& v) const noexcept { return data_ != v.data_; }

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
    constexpr const basic_map& as_const() const { return *this; }

    // lower_bound()
    iterator lower_bound(const key_type& k)
    {
        return std::lower_bound(begin(), end(), k, _key_compare());
    }
    const_iterator lower_bound(const key_type& k) const
    {
        return std::lower_bound(begin(), end(), k, _key_compare());
    }
    template <class K>
    iterator lower_bound(const K& k)
    {
        return std::lower_bound(begin(), end(), k, _key_compare());
    }
    template <class K>
    const_iterator lower_bound(const K& k) const
    {
        return std::lower_bound(begin(), end(), k, _key_compare());
    }

    // upper_bound()
    iterator upper_bound(const key_type& k)
    {
        return std::upper_bound(begin(), end(), k, _key_compare());
    }
    const_iterator upper_bound(const key_type& k) const
    {
        return std::upper_bound(begin(), end(), k, _key_compare());
    }
    template <class K>
    iterator upper_bound(const K& k)
    {
        return std::upper_bound(begin(), end(), k, _key_compare());
    }
    template <class K>
    const_iterator upper_bound(const K& k) const
    {
        return std::upper_bound(begin(), end(), k, _key_compare());
    }

    // equal_range()
    std::pair<iterator, iterator> equal_range(const key_type& k)
    {
        return std::equal_range(begin(), end(), k, _key_compare());
    }
    std::pair<const_iterator, const_iterator> equal_range(const key_type& k) const
    {
        return std::equal_range(begin(), end(), k, _key_compare());
    }
    template <class K>
    std::pair<iterator, iterator> equal_range(const K& k)
    {
        return std::equal_range(begin(), end(), k, _key_compare());
    }
    template <class K>
    std::pair<const_iterator, const_iterator> equal_range(const K& k) const
    {
        return std::equal_range(begin(), end(), k, _key_compare());
    }

    // find()
    iterator find(const key_type& k)
    {
        auto it = lower_bound(k);
        return (it != end() && _equals(it->first, k)) ? it : end();
    }
    const_iterator find(const key_type& k) const
    {
        auto it = lower_bound(k);
        return (it != end() && _equals(it->first, k)) ? it : end();
    }
    template <class K>
    iterator find(const K& k)
    {
        auto it = lower_bound<K>(k);
        return (it != end() && _equals(it->first, k)) ? it : end();
    }
    template <class K>
    const_iterator find(const K& k) const
    {
        auto it = lower_bound<K>(k);
        return (it != end() && _equals(it->first, k)) ? it : end();
    }

    // count()
    size_t count(const key_type& k) const
    {
        return _count_keys(equal_range(k), k);
    }
    template <class K>
    size_t count(const K& k) const
    {
        return _count_keys(equal_range(k), k);
    }

    // contains()
    bool contains(const Key& k) const
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
        auto it = lower_bound(v.first);
        if (it == end() || !_equals(it->first, v.first)) {
            return { data_.insert(it, v), true };
        }
        else {
            return { it, false };
        }
    }
    std::pair<iterator, bool> insert(value_type&& v)
    {
        auto it = lower_bound(v.first);
        if (it == end() || !_equals(it->first, v.first)) {
            return { data_.insert(it, std::move(v)), true };
        }
        else {
            return { it, false };
        }
    }
    template<class Iter>
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
    iterator erase(const key_type& v)
    {
        if (auto it = find(v); it != end()) {
            return data_.erase(it);
        }
        else {
            return end();
        }
    }
    iterator erase(iterator pos)
    {
        return data_.erase(pos);
    }
    iterator erase(iterator first, iterator last)
    {
        return data_.erase(first, last);
    }

    // at()
    mapped_type& at(const key_type& v)
    {
        if (auto it = find(v); it != end()) {
            return it->second;
        }
        else {
            throw std::out_of_range("flat_map::at()");
        }
    }
    const mapped_type& at(const key_type& v) const
    {
        if (auto it = find(v); it != end()) {
            return it->second;
        }
        else {
            throw std::out_of_range("flat_map::at()");
        }
    }

    // operator[]
    mapped_type& operator[](const key_type& v)
    {
        if (auto it = find(v); it != end()) {
            return it->second;
        }
        else {
            return insert(value_type{ v, {} }).first->second;
        }
    }
    mapped_type& operator[](key_type&& v)
    {
        if (auto it = find(v); it != end()) {
            return it->second;
        }
        else {
            return insert(value_type{ v, {} }).first->second;
        }
    }


private:
    void _sort()
    {
        std::sort(begin(), end(), [](auto& a, auto& b) { return key_compare()(a.first, b.first); });
    }

    template<class IterPair, class K>
    static size_t _count_keys(const IterPair& pair, const K& key)
    {
        size_t r = 0;
        for (auto first = pair.first; first != pair.second; ++first) {
            if (first->first == key) {
                ++r;
            }
        }
        return r;
    }

    struct _key_compare
    {
        template<class K>
        bool operator()(const value_type& v, const K& k) const
        {
            return Compare()(v.first, k);
        }
        template<class K>
        bool operator()(const K& k, const value_type& v) const
        {
            return Compare()(k, v.first);
        }
    };

    static bool _equals(const key_type& a, const key_type& b)
    {
        return !Compare()(a, b) && !Compare()(b, a);
    }
    template <class K1, class K2>
    static bool _equals(const K1& a, const K2& b)
    {
        return !Compare()(a, b) && !Compare()(b, a);
    }

private:
    container_type data_;
};

template<class K, class V, class Comp, class Cont1, class Cont2>
bool operator==(const basic_map<K, V, Comp, Cont1>& l, const basic_map<K, V, Comp, Cont2>& r)
{
    return l.size() == r.size() && std::equal(l.begin(), l.end(), r.begin());
}
template<class K, class V, class Comp, class Cont1, class Cont2>
bool operator!=(const basic_map<K, V, Comp, Cont1>& l, const basic_map<K, V, Comp, Cont2>& r)
{
    return l.size() != r.size() || !std::equal(l.begin(), l.end(), r.begin());
}
template<class K, class V, class Comp, class Cont1, class Cont2>
bool operator<(const basic_map<K, V, Comp, Cont1>& l, const basic_map<K, V, Comp, Cont2>& r)
{
    return std::lexicographical_compare(l.begin(), l.end(), r.begin(), r.end());
}
template<class K, class V, class Comp, class Cont1, class Cont2>
bool operator>(const basic_map<K, V, Comp, Cont1>& l, const basic_map<K, V, Comp, Cont2>& r)
{
    return r < l;
}
template<class K, class V, class Comp, class Cont1, class Cont2>
bool operator<=(const basic_map<K, V, Comp, Cont1>& l, const basic_map<K, V, Comp, Cont2>& r)
{
    return !(r < l);
}
template<class K, class V, class Comp, class Cont1, class Cont2>
bool operator>=(const basic_map<K, V, Comp, Cont1>& l, const basic_map<K, V, Comp, Cont2>& r)
{
    return !(l < r);
}


template <class Key, class Value, class Compare = std::less<>, class Allocator = std::allocator<std::pair<Key, Value>>>
using flat_map = basic_map<Key, Value, Compare, std::vector<std::pair<Key, Value>, Allocator>>;

template <class Key, class Value, size_t Capacity, class Compare = std::less<>>
using fixed_map = basic_map<Key, Value, Compare, fixed_vector<std::pair<Key, Value>, Capacity>>;

template <class Key, class Value, size_t Capacity, class Compare = std::less<>, class Allocator = std::allocator<std::pair<Key, Value>>>
using small_map = basic_map<Key, Value, Compare, small_vector<std::pair<Key, Value>, Capacity, Allocator>>;

template <class Key, class Value, class Compare = std::less<>>
using remote_map = basic_map<Key, Value, Compare, remote_vector<std::pair<Key, Value>>>;

template<class Key, class Value, class Compare = std::less<>, class Allocator = std::allocator<std::pair<Key, Value>>>
using shared_map = basic_map<Key, Value, Compare, shared_vector<std::pair<Key, Value>, Allocator>>;

} // namespace ist
