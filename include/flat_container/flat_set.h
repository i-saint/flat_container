#pragma once
#include <vector>
#include <algorithm>
#include <initializer_list>


// flat set (aka sorted vector)
template <
    class Key,
    class Compare = std::less<>,
    class Allocator = std::allocator<Key>
>
class flat_set
{
public:
    using key_type               = Key;
    using value_type             = Key;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;
    using key_compare            = Compare;
    using value_compare          = Compare;
    using allocator_type         = Allocator;
    using reference              = Key&;
    using const_reference        = const Key&;
    using pointer                = typename std::allocator_traits<allocator_type>::pointer;
    using const_pointer          = typename std::allocator_traits<allocator_type>::const_pointer;
    using vector_type            = std::vector<value_type, allocator_type>;
    using iterator               = typename vector_type::iterator;
    using const_iterator         = typename vector_type::const_iterator;
    using reverse_iterator       = typename vector_type::reverse_iterator;
    using const_reverse_iterator = typename vector_type::const_reverse_iterator;



    flat_set() {}
    flat_set(const flat_set& v)
        : m_data(v.m_data)
    {
    }
    flat_set(flat_set&& v) noexcept { swap(v); }

    flat_set& operator=(const flat_set& v)
    {
        m_data = v.m_data;
        return *this;
    }
    flat_set& operator=(flat_set&& v) noexcept
    {
        swap(v);
        return *this;
    }

    void swap(flat_set& v) noexcept
    {
        m_data.swap(v.m_data);
    }
    // v must be sorted or call sort() after swap()
    void swap(vector_type& v) noexcept
    {
        m_data.swap(v);
    }

    // representation

    vector_type& get() { return m_data; }
    const vector_type& get() const { return m_data; }

    vector_type&& extract() { return std::move(m_data); }

    // compare

    bool operator==(const flat_set& v) const { return m_data == v.m_data; }
    bool operator!=(const flat_set& v) const { return m_data != v.m_data; }

    // resize & clear

    void reserve(size_type v) { m_data.reserve(v); }

    // resize() may break the order. sort() should be called in that case.
    void resize(size_type v) { m_data.resize(v); }

    void clear() { m_data.clear(); }

    void shrink_to_fit() { m_data.shrink_to_fit(); }

    // for the case m_data is directry modified (resize(), swap(), get(), etc)
    void sort() { std::sort(begin(), end(), key_compare()); }

    // size & data

    size_type empty() const { return m_data.empty(); }

    size_type size() const { return m_data.size(); }

    pointer data() { return m_data.data(); }
    const_pointer data() const { return m_data.data(); }

    // iterator

    iterator begin() noexcept { return m_data.begin(); }
    const_iterator begin() const noexcept { return m_data.begin(); }
    constexpr const_iterator cbegin() const noexcept { return m_data.cbegin(); }

    iterator end() noexcept { return m_data.end(); }
    const_iterator end() const noexcept { return m_data.end(); }
    constexpr const_iterator cend() const noexcept { return m_data.cend(); }

    reverse_iterator rbegin() noexcept { return m_data.rbegin(); }
    const_reverse_iterator rbegin() const noexcept { return m_data.rbegin(); }
    constexpr const_reverse_iterator crbegin() const noexcept { return m_data.crbegin(); }

    reverse_iterator rend() noexcept { return m_data.rend(); }
    const_reverse_iterator rend() const noexcept { return m_data.rend(); }
    constexpr const_reverse_iterator crend() const noexcept { return m_data.crend(); }

    // search

    iterator lower_bound(const value_type& v)
    {
        return std::lower_bound(begin(), end(), v, key_compare());
    }
    const_iterator lower_bound(const value_type& v) const
    {
        return std::lower_bound(begin(), end(), v, key_compare());
    }
    template <class V, class C = Compare, class = typename C::is_transparent>
    iterator lower_bound(const V& v)
    {
        return std::lower_bound(begin(), end(), v, C());
    }
    template <class V, class C = Compare, class = typename C::is_transparent>
    const_iterator lower_bound(const V& v) const
    {
        return std::lower_bound(begin(), end(), v, C());
    }

    iterator upper_bound(const value_type& v)
    {
        return std::upper_bound(begin(), end(), v, key_compare());
    }
    const_iterator upper_bound(const value_type& v) const
    {
        return std::upper_bound(begin(), end(), v, key_compare());
    }
    template <class V, class C = Compare, class = typename C::is_transparent>
    iterator upper_bound(const V& v)
    {
        return std::upper_bound(begin(), end(), v, C());
    }
    template <class V, class C = Compare, class = typename C::is_transparent>
    const_iterator upper_bound(const V& v) const
    {
        return std::upper_bound(begin(), end(), v, C());
    }

    std::pair<iterator, iterator> equal_range(const value_type& v)
    {
        return std::equal_range(begin(), end(), v, key_compare());
    }
    std::pair<const_iterator, const_iterator> equal_range(const value_type& v) const
    {
        return std::equal_range(begin(), end(), v, key_compare());
    }
    template <class V, class C = Compare, class = typename C::is_transparent>
    std::pair<iterator, iterator> equal_range(const V& v)
    {
        return std::equal_range(begin(), end(), v, C());
    }
    template <class V, class C = Compare, class = typename C::is_transparent>
    std::pair<const_iterator, const_iterator> equal_range(const V& v) const
    {
        return std::equal_range(begin(), end(), v, C());
    }

    iterator find(const value_type& v)
    {
        auto it = lower_bound(v);
        return (it != end() && equal(*it, v)) ? it : end();
    }
    const_iterator find(const value_type& v) const
    {
        auto it = lower_bound(v);
        return (it != end() && equal(*it, v)) ? it : end();
    }
    template <class V, class C = Compare, class = typename C::is_transparent>
    iterator find(const V& v)
    {
        auto it = lower_bound<V, C>(v);
        return (it != end() && equal<value_type, V, C>(*it, v)) ? it : end();
    }
    template <class V, class C = Compare, class = typename C::is_transparent>
    const_iterator find(const V& v) const
    {
        auto it = lower_bound<V, C>(v);
        return (it != end() && equal<value_type, V, C>(*it, v)) ? it : end();
    }

    size_t count(const value_type& v) const
    {
        return find(v) != end() ? 1 : 0;
    }
    template <class V, class C = Compare, class = typename C::is_transparent>
    size_t count(const V& v) const
    {
        return find<V, C>(v) != end() ? 1 : 0;
    }

    // insert & erase

    std::pair<iterator, bool> insert(const value_type& v)
    {
        auto it = lower_bound(v);
        if (it == end() || !equal(*it, v))
            return { m_data.insert(it, v), true };
        else
            return { it, false };
    }
    std::pair<iterator, bool> insert(value_type&& v)
    {
        auto it = lower_bound(v);
        if (it == end() || !equal(*it, v))
            return { m_data.insert(it, v), true };
        else
            return { it, false };
    }
    template <class Iter>
    void insert(Iter first, Iter last)
    {
        for (auto i = first; i != last; ++i)
            insert(*i);
    }
    void insert(std::initializer_list<value_type> list)
    {
        for (auto& v : list)
            insert(v);
    }

    iterator erase(const value_type& v)
    {
        if (auto it = find(v); it != end())
            return m_data.erase(it);
        else
            return end();
    }
    iterator erase(iterator pos)
    {
        return m_data.erase(pos);
    }
    iterator erase(iterator first, iterator last)
    {
        return m_data.erase(first, last);
    }

private:
    static bool equal(const value_type& a, const value_type& b)
    {
        return !Compare()(a, b) && !Compare()(b, a);
    }
    template <class V1, class V2, class C = Compare, class = typename C::is_transparent>
    static bool equal(const V1& a, const V2& b)
    {
        return !C()(a, b) && !C()(b, a);
    }

private:
    vector_type m_data;
};
