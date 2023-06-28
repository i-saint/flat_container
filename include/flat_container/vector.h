#pragma once
#include "foundation.h"

namespace ist {

template<class T, class Memory>
class basic_vector : public Memory
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

    constexpr basic_vector() {}
    constexpr basic_vector(const basic_vector& r) noexcept { operator=(r); }
    constexpr basic_vector(basic_vector&& r) noexcept { operator=(std::move(r)); }

    template<bool view = is_memory_view_v<super>, fc_require(!view)>
    constexpr explicit basic_vector(size_t n) { resize(n); }

    template<bool view = is_memory_view_v<super>, fc_require(!view)>
    constexpr basic_vector(size_t n, const T& v) { resize(n, v); }

    template<bool view = is_memory_view_v<super>, fc_require(!view)>
    constexpr basic_vector(std::initializer_list<T> r) { assign(r); }

    template<class Iter, bool view = is_memory_view_v<super>, fc_require(!view), fc_require(is_iterator_v<Iter>)>
    constexpr basic_vector(Iter first, Iter last) { assign(first, last); }

    template<bool view = is_memory_view_v<super>, fc_require(view)>
    constexpr basic_vector(void* data, size_t capacity, size_t size = 0)
        : super(data, capacity, size)
    {
    }

    ~basic_vector() { clear(); }

    constexpr void swap(basic_vector& r)
    {
        if constexpr (is_trivially_swappable_v<super>) {
            std::swap(capacity_, r.capacity_);
            std::swap(size_, r.size_);
            std::swap(data_, r.data_);
        }
        else {
            size_t size1 = size_;
            size_t size2 = r.size_;
            size_t swap_count = std::min(size1, size2);
            for (size_t i = 0; i < swap_count; ++i) {
                std::swap(data_[i], r[i]);
            }
            if (size1 < size2) {
                for (size_t i = size1; i < size2; ++i) {
                    new (data_ + i) T(std::move(r[i]));
                    r[i].~T();
                }
            }
            if (size2 < size1) {
                for (size_t i = size2; i < size1; ++i) {
                    new (r.data_ + i) T(std::move(data_[i]));
                    data_[i].~T();
                }
            }
            std::swap(size_, r.size_);
        }
    }

    constexpr basic_vector& operator=(const basic_vector& r) noexcept
    {
        if constexpr (is_memory_view_v<super>) {
            capacity_ = r.capacity_;
            size_ = r.size_;
            data_ = r.data_;
        }
        else {
            assign(r.begin(), r.end());
        }
        return *this;
    }
    constexpr basic_vector& operator=(basic_vector&& r) noexcept
    {
        if constexpr (is_trivially_swappable_v<super>) {
            swap(r);
        }
        else {
            assign_impl(r.size(), [&](T* dst) { move_construct(dst, r.begin(), r.end()); });
            r.clear();
        }
        return *this;
    }

    using super::capacity;
    using super::size;
    using super::data;
    using super::reserve;
    using super::shrink_to_fit;

    constexpr bool empty() const noexcept { return size_ > 0; }
    constexpr bool full() const noexcept { return size_ == capacity_; }
    constexpr iterator begin() noexcept { return data_; }
    constexpr const_iterator begin() const noexcept { return data_; }
    constexpr const_iterator cbegin() const noexcept { return data_; }
    constexpr iterator end() noexcept { return data_ + size_; }
    constexpr const_iterator end() const noexcept { return data_ + size_; }
    constexpr const_iterator cend() const noexcept { return data_ + size_; }
    constexpr T& at(size_t i) { boundary_check(i); return data_[i]; }
    constexpr const T& at(size_t i) const { boundary_check(i); return data_[i]; }
    constexpr T& operator[](size_t i) { boundary_check(i); return data_[i]; }
    constexpr const T& operator[](size_t i) const { boundary_check(i); return data_[i]; }
    constexpr T& front() { boundary_check(1); return data_[0]; }
    constexpr const T& front() const { boundary_check(1); return data_[0]; }
    constexpr T& back() { boundary_check(1); return data_[size_ - 1]; }
    constexpr const T& back() const { boundary_check(1); return data_[size_ - 1]; }

    constexpr void clear()
    {
        shrink(size_);
    }

    constexpr void resize(size_t n)
    {
        resize_impl(n, [&](T* addr) { new (addr) T(); });
    }
    constexpr void resize(size_t n, const T& v)
    {
        resize_impl(n, [&](T* addr) { new (addr) T(v); });
    }

    constexpr void push_back(const T& v)
    {
        expand(1, [&](T* addr) { new (addr) T(v); });
    }
    constexpr void push_back(T&& v)
    {
        expand(1, [&](T* addr) { new (addr) T(std::move(v)); });
    }

    template< class... Args >
    constexpr T& emplace_back(Args&&... args)
    {
        expand(1, [&](T* addr) { new (addr) T(std::forward<Args>(args)...); });
        return back();
    }

    constexpr void pop_back()
    {
        shrink(1);
    }

    template<class Iter, fc_require(is_iterator_v<Iter>)>
    constexpr void assign(Iter first, Iter last)
    {
        size_t n = std::distance(first, last);
        assign_impl(n, [&](T* dst) { copy_construct(dst, first, last); });
    }
    constexpr void assign(std::initializer_list<value_type> list)
    {
        assign_impl(list.size(), [&](T* dst) { copy_construct(dst, list.begin(), list.end()); });
    }
    constexpr void assign(size_t n, const T& v)
    {
        assign_impl(n, [&](T* dst) { copy_construct(dst, v, n); });
    }

    template<class Iter, fc_require(is_iterator_v<Iter>)>
    constexpr iterator insert(iterator pos, Iter first, Iter last)
    {
        size_t n = std::distance(first, last);
        return insert_impl(pos, n, [&](T* addr) { copy_construct(addr, first, last); });
    }
    constexpr iterator insert(iterator pos, const T& v)
    {
        return insert_impl(pos, 1, [&](T* addr) { copy_construct(addr, v, 1); });
    }
    constexpr iterator insert(iterator pos, T&& v)
    {
        return insert_impl(pos, 1, [&](T* addr) { move_construct(addr, std::move(v)); });
    }
    constexpr iterator insert(iterator pos, std::initializer_list<value_type> list)
    {
        return insert_impl(pos, list.size(), [&](T* addr) { copy_construct(addr, list.begin(), list.end()); });
    }

    constexpr iterator erase(iterator first, iterator last)
    {
        size_t s = std::distance(first, last);
        std::move(last, end(), first);
        shrink(s);
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

    template<class Iter, fc_require(is_iterator_v<Iter>)>
    constexpr void copy_construct(iterator dst, Iter first, Iter last)
    {
        size_t n = std::distance(first, last);
        T* end_assign = std::min(dst + n, data_ + size_);
        T* end_new = dst + n;
        while (dst < end_assign) {
            *dst++ = *first++;
        }
        while (dst < end_new) {
            new (dst++) T(*first++);
        }
    }
    constexpr void copy_construct(iterator dst, const T& v, size_t n)
    {
        auto first = make_constant_iterator(v);
        copy_construct(dst, first, first + n);
    }

    constexpr void move_construct(iterator dst, T&& v)
    {
        if (dst >= data_ + size_) {
            new (dst) T(std::move(v));
        }
        else {
            *dst = std::move(v);
        }
    }
    template<class Iter, fc_require(is_iterator_v<Iter>)>
    constexpr void move_construct(iterator dst, Iter first, Iter last)
    {
        size_t n = std::distance(first, last);
        T* end_assign = std::min(dst + n, data_ + size_);
        T* end_new = dst + n;
        while (dst < end_assign) {
            *dst++ = std::move(*first++);
        }
        while (dst < end_new) {
            new (dst++) T(std::move(*first++));
        }
    }

    template<class Construct>
    constexpr void assign_impl(size_t n, Construct&& construct)
    {
        reserve(n);
        capacity_check(n);
        T* dst = data_;
        T* end_dtor = data_ + size_;
        construct(dst);
        dst += n;
        while (dst < end_dtor) {
            (dst++)->~T();
        }
        size_ = n;
    }

    constexpr void shrink(size_t n)
    {
        size_t new_size = size_ - n;
        capacity_check(new_size);
        for (size_t i = new_size; i < size_; ++i) {
            data_[i].~T();
        }
        size_ = new_size;
    }

    template<class Construct>
    constexpr void expand(size_t n, Construct&& construct)
    {
        size_t new_size = size_ + n;
        reserve(new_size);
        capacity_check(new_size);
        for (size_t i = size_; i < new_size; ++i) {
            construct(data_ + i);
        }
        size_ = new_size;
    }

    template<class Construct>
    constexpr void resize_impl(size_t n, Construct&& construct)
    {
        if (n < size_) {
            shrink(size_ - n);
        }
        else if (n > size_) {
            expand(n - size_, std::move(construct));
        }
    }

    template<class Construct>
    constexpr iterator insert_impl(iterator pos, size_t s, Construct&& construct)
    {
        size_t d = std::distance(data_, pos);
        reserve(size_ + s);
        pos = data_ + d; // for the case realloc happened
        move_backward(pos, data_ + size_, data_ + size_ + s);
        construct(pos);
        size_ += s;
        return pos;
    }

    constexpr iterator move_backward(iterator first, iterator last, iterator dst)
    {
        size_t n = std::distance(first, last);
        if (n == 0) {
            return dst;
        }
        T* end_new = data_ + size_;
        T* end_assign = dst - n;
        while (dst > end_new) {
            new (--dst) T(std::move(*(--last)));
        }
        while (dst > end_assign) {
            *(--dst) = std::move(*(--last));
        }
        return dst;
    }

    constexpr void capacity_check(size_t n) const
    {
#ifdef FC_ENABLE_CAPACITY_CHECK
        if (n > capacity_) {
            throw std::out_of_range("fixed_vector: out of capacity");
        }
#endif
    }
    constexpr void boundary_check(size_t n) const
    {
#ifdef FC_ENABLE_CAPACITY_CHECK
        if (n > size_) {
            throw std::out_of_range("fixed_vector: out of range");
        }
#endif
    }
};


template<class T>
using vector = basic_vector<T, dynamic_memory<T>>;

template<class T, size_t Capacity>
using fixed_vector = basic_vector<T, fixed_memory<T, Capacity>>;

template<class T>
using vector_view = basic_vector<T, memory_view<T>>;

} // namespace ist
