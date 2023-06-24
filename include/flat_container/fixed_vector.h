#pragma once
#include <initializer_list>
#include <type_traits>
#include <cstddef>


namespace impl {

template<class T, size_t Capacity>
class fixed_memory
{
public:
    static const bool is_trivially_swappable = false;

    fixed_memory() {}
    ~fixed_memory() {}
    constexpr static size_t capacity() { return capacity_; }
    constexpr size_t size() const { return size_; }
    constexpr T* data() { return (T*)buffer_; }
    constexpr const T* data() const { return (T*)buffer_; }

public:
    static const size_t capacity_ = Capacity;
    size_t size_ = 0;
    union {
        T data_[0]; // for debug
        std::byte buffer_[sizeof(T) * Capacity]; // uninitialized
    };
};

template<class T>
class memory_view
{
public:
    static const bool is_memory_view = true;
    static const bool is_trivially_swappable = true;

    memory_view() {}
    memory_view(void* data, size_t capacity) : capacity_(capacity), data_((T*)data) {}
    constexpr size_t capacity() { return capacity_; }
    constexpr size_t size() const { return size_; }
    constexpr T* data() { return data_; }
    constexpr const T* data() const { return data_; }

public:
    size_t capacity_ = 0;
    size_t size_ = 0;
    T* data_ = nullptr;
};


template <typename T, typename = void>
constexpr bool is_memory_view_v = false;
template <typename T>
constexpr bool is_memory_view_v<T, std::void_t<decltype(T::is_memory_view)>> = true;


template<class T, class Memory>
class fixed_vector_impl : public Memory
{
using super = Memory;
public:
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using iterator = pointer;
    using const_iterator = const_pointer;

    constexpr fixed_vector_impl() {}
    constexpr fixed_vector_impl(const fixed_vector_impl& r) noexcept { operator=(r); }
    constexpr fixed_vector_impl(fixed_vector_impl&& r) noexcept { operator=(std::move(r)); }

    template<bool view = is_memory_view_v<Memory>, std::enable_if_t<!view, bool> = true>
    constexpr explicit fixed_vector_impl(size_t n) { resize(n); }

    template<bool view = is_memory_view_v<Memory>, std::enable_if_t<!view, bool> = true>
    constexpr fixed_vector_impl(size_t n, const T& v) { resize(n, v); }

    template<bool view = is_memory_view_v<Memory>, std::enable_if_t<!view, bool> = true>
    constexpr fixed_vector_impl(std::initializer_list<T> r) { assign(r); }

    template<class ForwardIter, bool view = is_memory_view_v<Memory>, std::enable_if_t<!view, bool> = true>
    constexpr fixed_vector_impl(ForwardIter first, ForwardIter last) { assign(first, last); }

    template<bool view = is_memory_view_v<Memory>, std::enable_if_t<view, bool> = true>
    constexpr fixed_vector_impl(void* data, size_t capacity)
        : Memory(data, capacity)
    {
    }

    ~fixed_vector_impl() { clear(); }

    constexpr void swap(fixed_vector_impl& r)
    {
        if constexpr (super::is_trivially_swappable) {
            std::swap(capacity_, r.capacity_);
            std::swap(size_, r.size_);
            std::swap(data_, r.data_);
        }
        else {
            size_t size1 = size();
            size_t size2 = r.size();
            size_t min = std::min(size1, size2);
            for (size_t i = 0; i < min; ++i) {
                std::swap(data()[i], r[i]);
            }
            if (size1 < size2) {
                for (size_t i = size1; i < size2; ++i) {
                    new (data() + i) T(std::move(r[i]));
                }
                size_ = size2;
                r.resize(size1);
            }
            if (size2 < size1) {
                for (size_t i = size2; i < size1; ++i) {
                    new (r.data() + i) T(std::move(data()[i]));
                }
                r.size_ = size1;
                resize(size2);
            }
        }
    }
    constexpr fixed_vector_impl& operator=(const fixed_vector_impl& r) noexcept
    {
        assign(r.begin(), r.end());
        return *this;
    }
    constexpr fixed_vector_impl& operator=(fixed_vector_impl&& r) noexcept
    {
        if constexpr (super::is_trivially_swappable) {
            swap(r);
        }
        else {
            resize(r.size());
            std::move(r.begin(), r.end(), begin());
            r.clear();
        }
        return *this;
    }

    using super::capacity;
    using super::size;
    using super::data;

    constexpr bool empty() const { return size() > 0; }
    constexpr bool full() const { return size() == capacity(); }
    constexpr iterator begin() { return data(); }
    constexpr iterator end() { return data() + size(); }
    constexpr const_iterator begin() const { return data(); }
    constexpr const_iterator end() const { return data() + size(); }
    constexpr T& operator[](size_t i) { capacity_check(i); return data()[i]; }
    constexpr const T& operator[](size_t i) const { capacity_check(i); return data()[i]; }
    constexpr T& front() { return data()[0]; }
    constexpr const T& front() const { return data()[0]; }
    constexpr T& back() { return data()[size() - 1]; }
    constexpr const T& back() const { return data()[size() - 1]; }

    // just for compatibility
    constexpr void reserve(size_t n) {}
    constexpr void shrink_to_fit() {}

    constexpr void clear()
    {
        resize(0);
    }

    constexpr void resize(size_t n)
    {
        resize_impl(n, [&](T* addr, size_t) { new (addr) T(); });
    }
    constexpr void resize(size_t n, const T& v)
    {
        resize_impl(n, [&](T* addr, size_t) { new (addr) T(v); });
    }

    constexpr void push_back(const T& v)
    {
        resize_impl(size() + 1, [&](T* addr, size_t) { new (addr) T(v); });
    }
    constexpr void push_back(T&& v)
    {
        resize_impl(size() + 1, [&](T* addr, size_t) { new (addr) T(std::move(v)); });
    }

    template< class... Args >
    constexpr T& emplace_back(Args&&... args)
    {
        resize_impl(size() + 1, [&](T* addr, size_t) { new (addr) T(std::forward<Args>(args)...); });
        return back();
    }

    constexpr void pop_back()
    {
        resize(size() - 1);
    }

    template<class ForwardIter>
    constexpr void assign(ForwardIter first, ForwardIter last)
    {
        size_t n = std::distance(first, last);
        resize(n);
        std::copy(first, last, begin());
    }
    constexpr void assign(std::initializer_list<value_type> list)
    {
        resize(list.size());
        std::copy(list.begin(), list.begin(), begin());
    }
    constexpr void assign(size_t n, const T& v)
    {
        resize(n);
        for (auto& dst : *this)
            dst = v;
    }

    template<class ForwardIter>
    constexpr iterator insert(iterator pos, ForwardIter first, ForwardIter last)
    {
        size_t n = std::distance(first, last);
        return insert_impl(pos, n, [&](T* addr) { std::copy(first, last, addr); });
    }
    constexpr iterator insert(iterator pos, const T& v)
    {
        return insert_impl(pos, 1, [&](T* addr) { *addr = v; });
    }
    constexpr iterator insert(iterator pos, T&& v)
    {
        return insert_impl(pos, 1, [&](T* addr) { *addr = std::move(v); });
    }
    constexpr iterator insert(iterator pos, std::initializer_list<value_type> list)
    {
        return insert_impl(pos, list.size(), [&](T* addr) { std::copy(list.begin(), list.end(), addr); });
    }

    constexpr iterator erase(iterator first, iterator last)
    {
        size_t s = std::distance(first, last);
        std::move(last, end(), first);
        resize(size() - s);
        return first;
    }
    constexpr iterator erase(iterator pos)
    {
        return erase(pos, pos + 1);
    }

private:
    using super::capacity_;
    using super::size_;
    using super::data_;

    // Construct: [](T*)
    template<class Construct>
    constexpr void resize_impl(size_t n, Construct&& construct)
    {
        capacity_check(n);
        size_t old_size = size();
        for (size_t i = n; i < old_size; ++i) {
            data()[i].~T();
        }
        for (size_t i = old_size; i < n; ++i) {
            construct(data() + i, i);
        }
        size_ = n;
    }

    template<class Construct>
    constexpr iterator insert_impl(iterator pos, size_t s, Construct&& construct)
    {
        size_t old_size = size();
        size_t d = std::distance(begin(), pos);
        resize(old_size + s);
        if (d != old_size) {
            std::move_backward(begin() + d, begin() + old_size, end());
        }
        construct(begin() + d);
        return begin() + d;
    }


    constexpr void capacity_check(size_t n)
    {
#ifdef _DEBUG
        if (n > capacity()) {
            throw std::out_of_range("fixed_vector: out of range");
        }
#endif
    }
};

} // namespace impl


template<class T, size_t Capacity>
using fixed_vector = impl::fixed_vector_impl<T, impl::fixed_memory<T, Capacity>>;

template<class T>
using vector_view = impl::fixed_vector_impl<T, impl::memory_view<T>>;
