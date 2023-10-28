#pragma once
#include "memory.h"
#include "span.h"

namespace ist {


// std::construct_at() requires c++20 so define our own.
template<class T, class... Args>
inline constexpr T* _construct_at(T* p, Args&&... args)
{
    return new (p) T(std::forward<Args>(args)...);
}
// std::destroy, std::destroy_at is c++17 but define our own for consistency with construct_at.
template<class T>
inline constexpr void _destroy_at(T* p)
{
    std::destroy_at(p);
}
template<class Iter>
inline constexpr void _destroy(const Iter first, const Iter last)
{
    std::destroy(first, last);
}


template<class Memory>
class vector_base : public Memory
{
using super = Memory;
public:
    using value_type = typename super::value_type;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using iterator = pointer;
    using const_iterator = const_pointer;


    vector_base() {}
    vector_base(const vector_base& r) { operator=(r); }
    vector_base(vector_base&& r) noexcept { operator=(std::move(r)); }
    template<bool view = is_memory_view_v<super>, fc_require(view)>
    constexpr vector_base(void* data, size_t capacity, size_t size = 0)
        : super(data, capacity, size)
    {
    }
    ~vector_base()
    {
        _shrink(this->_size());
    }

    // operator=()
    vector_base& operator=(const vector_base& r)
    {
        this->_copy_on_write();
        _assign(r.size(), [&](pointer dst) { _copy_range(dst, r.begin(), r.end()); });
        return *this;
    }
    vector_base& operator=(vector_base&& r)
    {
        swap(r);
        return *this;
    }

    // swap()
    constexpr void swap(vector_base& r)
    {
        if constexpr (have_buffer_v<super>) {
            if constexpr (is_reallocatable_v<super>) {
                if (this->_capacity() > this->buffer_capacity() && r._capacity() > r.buffer_capacity()) {
                    std::swap(this->_capacity(), r._capacity());
                    std::swap(this->_size(), r._size());
                    std::swap(this->_data(), r._data());
                }
                else {
                    this->_swap_content(r);
                }
            }
            else {
                this->_swap_content(r);
            }
        }
        else {
            std::swap(this->_capacity(), r._capacity());
            std::swap(this->_size(), r._size());
            std::swap(this->_data(), r._data());
        }
    }

    // reserve()
    void reserve(size_t n)
    {
        if constexpr (is_reallocatable_v<super>) {
            if (n > this->_capacity()) {
                this->_copy_on_write();
                size_t new_capaity = std::max<size_t>(n, this->_capacity() * 2);
                this->_resize_capacity(new_capaity);
            }
        }
    }

    // shrink_to_fit()
    void shrink_to_fit()
    {
        if constexpr (is_reallocatable_v<super>) {
            if (this->_size() != this->_capacity()) {
                this->_copy_on_write();
                this->_resize_capacity(this->_size());
            }
        }
    }

    // clear()
    constexpr void clear()
    {
        if (this->_size() > 0) {
            this->_copy_on_write();
            _shrink(this->_size());
        }
    }

    // capacity() / size() / size_bytes() / empty()
    constexpr size_t capacity() const noexcept
    {
        return this->_capacity();
    }
    constexpr size_t size() const noexcept
    {
        return this->_size();
    }
    constexpr size_t size_bytes() const noexcept
    {
        return sizeof(value_type) * this->_size();
    }
    constexpr bool empty() const noexcept
    {
        return this->_size() == 0;
    }

    // data() / cdata()
    constexpr pointer data() noexcept
    {
        this->_copy_on_write();
        return this->_data();
    }
    constexpr const_pointer data() const noexcept
    {
        return this->_data();
    }
    constexpr const_pointer cdata() const noexcept
    {
        return this->_data();
 }

    // begin() / cbegin()
    constexpr iterator begin() noexcept
    {
        this->_copy_on_write();
        return this->_data();
    }
    constexpr const_iterator begin() const noexcept
    {
        return this->_data();
    }
    constexpr const_iterator cbegin() const noexcept
    {
        return this->_data();
    }

    // end() / cend()
    constexpr iterator end() noexcept
    {
        this->_copy_on_write();
        return this->_data() + this->_size();
    }
    constexpr const_iterator end() const noexcept
    {
        return this->_data() + this->_size();
    }
    constexpr const_iterator cend() const noexcept
    {
        return this->_data() + this->_size();
    }

    // at()
    constexpr reference at(size_t i)
    {
        _boundary_check(i);
        this->_copy_on_write();
        return this->_data()[i];
    }
    constexpr const_reference at(size_t i) const
    {
        _boundary_check(i);
        return this->_data()[i];
    }

    // operator[]
    constexpr reference operator[](size_t i)
    {
        _boundary_check(i);
        this->_copy_on_write();
        return this->_data()[i];
    }
    constexpr const_reference operator[](size_t i) const
    {
        _boundary_check(i);
        return this->_data()[i];
    }

    // front()
    constexpr reference front()
    {
        _boundary_check(1);
        this->_copy_on_write();
        return this->_data()[0];
    }
    constexpr const_reference front() const
    {
        _boundary_check(1);
        return this->_data()[0];
    }

    // back()
    constexpr reference back()
    {
        _boundary_check(1);
        this->_copy_on_write();
        return this->_data()[this->_size() - 1] ;
    }
    constexpr const_reference back() const
    {
        _boundary_check(1);
        return this->_data()[this->_size() - 1];
    }

    // span<value_type>
    constexpr operator span<value_type>() const noexcept
    {
        return { this->_data(), this->_size() };
    }

protected:
    template<class Iter, class MaybeOverlapped = std::false_type, fc_require(is_iterator_v<Iter, value_type>)>
    constexpr void _copy_range(iterator dst, Iter first, Iter last, MaybeOverlapped = {})
    {
        if constexpr (is_pod_v<value_type>) {
            if constexpr (std::is_pointer_v<Iter> && !MaybeOverlapped::value) {
                std::memcpy(dst, first, sizeof(value_type) * std::distance(first, last));
            }
            else {
                std::copy(first, last, dst);
            }
        }
        else {
            size_t n = std::distance(first, last);
            auto end_assign = std::min(dst + n, this->_data() + this->_size());
            auto end_new = dst + n;
            while (dst < end_assign) {
                *dst++ = *first++;
            }
            while (dst < end_new) {
                _construct_at<value_type>(dst++, *first++);
            }
        }
    }
    constexpr void _copy_n(iterator dst, const_reference v, size_t n)
    {
        if constexpr (is_pod_v<value_type>) {
            auto end_assign = dst + n;
            while (dst < end_assign) {
                *dst++ = v;
            }
        }
        else {
            auto end_assign = std::min(dst + n, this->_data() + this->_size());
            auto end_new = dst + n;
            while (dst < end_assign) {
                *dst++ = v;
            }
            while (dst < end_new) {
                _construct_at<value_type>(dst++, v);
            }
        }
    }
    template<class Iter>
    constexpr void _move_range(iterator dst, Iter first, Iter last)
    {
        if constexpr (is_pod_v<value_type>) {
            _copy_range(dst, first, last);
        }
        else {
            size_t n = std::distance(first, last);
            auto end_assign = std::min(dst + n, this->_data() + this->_size());
            auto end_new = dst + n;
            while (dst < end_assign) {
                *dst++ = std::move(*first++);
            }
            while (dst < end_new) {
                _construct_at<value_type>(dst++, std::move(*first++));
            }
        }
    }
    constexpr void _move_one(iterator dst, value_type&& v)
    {
        if constexpr (is_pod_v<value_type>) {
            *dst = v;
        }
        else {
            if (dst >= this->_data() + this->_size()) {
                _construct_at<value_type>(dst, std::move(v));
            }
            else {
                *dst = std::move(v);
            }
        }
    }
    template< class... Args >
    constexpr void _emplace_one(iterator dst, Args&&... args)
    {
        if constexpr (is_pod_v<value_type>) {
            *dst = value_type(std::forward<Args>(args)...);
        }
        else {
            if (dst >= this->_data() + this->_size()) {
                _construct_at<value_type>(dst, std::forward<Args>(args)...);
            }
            else {
                *dst = value_type(std::forward<Args>(args)...);
            }
        }
    }

    constexpr void _swap_content(vector_base& r)
    {
        size_t max_size = std::max(this->_size(), r._size());
        this->reserve(max_size);
        r.reserve(max_size);

        if constexpr (is_pod_v<value_type>) {
            size_t swap_count = max_size;
            for (size_t i = 0; i < swap_count; ++i) {
                std::swap(this->_data()[i], r[i]);
            }
            std::swap(this->_size(), r._size());
        }
        else {
            size_t size1 = this->_size();
            size_t size2 = r._size();
            size_t swap_count = std::min(size1, size2);
            for (size_t i = 0; i < swap_count; ++i) {
                std::swap(this->_data()[i], r[i]);
            }
            if (size1 < size2) {
                for (size_t i = size1; i < size2; ++i) {
                    _construct_at<value_type>(this->_data() + i, std::move(r[i]));
                    _destroy_at(&r[i]);
                }
            }
            if (size2 < size1) {
                for (size_t i = size2; i < size1; ++i) {
                    _construct_at<value_type>(r._data() + i, std::move(this->_data()[i]));
                    _destroy_at(&this->_data()[i]);
                }
            }
            std::swap(this->_size(), r._size());
        }
    }

    template<class Construct>
    constexpr void _assign(size_t n, Construct&& construct)
    {
        this->reserve(n);
        _capacity_check(n);
        construct(this->_data());
        if constexpr (!is_pod_v<value_type>) {
            if (n < this->_size()) {
                _destroy(this->_data() + n, this->_data() + this->_size());
            }
        }
        this->_size() = n;
    }

    constexpr void _shrink(size_t n)
    {
        size_t new_size = this->_size() - n;
        _capacity_check(new_size);
        if constexpr (!is_pod_v<value_type>) {
            _destroy(this->_data() + new_size, this->_data() + this->_size());
        }
        this->_size() = new_size;
    }

    template<class Construct>
    constexpr void _expand(size_t n, Construct&& construct)
    {
        size_t new_size = this->_size() + n;
        this->reserve(new_size);
        _capacity_check(new_size);
        construct(this->_data() + this->_size());
        this->_size() = new_size;
    }

    template<class Construct>
    constexpr void _resize(size_t n, Construct&& construct)
    {
        if (n < this->_size()) {
            _shrink(this->_size() - n);
        }
        else if (n > this->_size()) {
            size_t exn = n - this->_size();
            _expand(exn, [&](pointer addr) {
                for (size_t i = 0; i < exn; ++i) {
                    construct(addr + i);
                }
                });
        }
    }

    template<class Construct>
    constexpr iterator _insert(iterator pos, size_t s, Construct&& construct)
    {
        size_t d = std::distance(this->_data(), pos);
        this->reserve(this->_size() + s);
        pos = this->_data() + d; // for the case realloc happened
        _move_backward(pos, this->_data() + this->_size(), this->_data() + this->_size() + s);
        construct(pos);
        this->_size() += s;
        return pos;
    }

    constexpr iterator _move_backward(iterator first, iterator last, iterator dst)
    {
        size_t n = std::distance(first, last);
        if (n == 0) {
            return dst;
        }
        if constexpr (is_pod_v<value_type>) {
            auto end_assign = dst - n;
            while (dst > end_assign) {
                *(--dst) = *(--last);
            }
        }
        else {
            auto end_new = this->_data() + this->_size();
            auto end_assign = dst - n;
            while (dst > end_new) {
                _construct_at<value_type>(--dst, std::move(*(--last)));
            }
            while (dst > end_assign) {
                *(--dst) = std::move(*(--last));
            }
        }
        return dst;
    }

    constexpr void _capacity_check(size_t n) const
    {
#ifdef FC_ENABLE_CAPACITY_CHECK
        if (n > this->_capacity()) {
            throw std::out_of_range("out of capacity");
        }
#endif
    }

    constexpr void _boundary_check(size_t n) const
    {
#ifdef FC_ENABLE_CAPACITY_CHECK
        if (n > this->_size()) {
            throw std::out_of_range("out of range");
        }
#endif
    }

private:
};


template<class T>
class constant_iterator
{
public:
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T*;
    using reference = T&;

    using ptrdiff_t = std::ptrdiff_t;

    constant_iterator(T& v, ptrdiff_t count = 0) : ref_(v), count_(count) {}
    constant_iterator(const constant_iterator&) = default;
    constant_iterator(constant_iterator&&) = default;
    constant_iterator& operator=(const constant_iterator&) = default;
    constant_iterator& operator=(constant_iterator&&) = default;

    reference operator*() { return ref_; }
    pointer operator->() { return &ref_; }
    constant_iterator& operator++() { ++count_; return *this; }
    constant_iterator& operator--() { --count_; return *this; }
    constant_iterator operator++(int) { auto ret = *this; ++count_; return ret; }
    constant_iterator operator--(int) { auto ret = *this; --count_; return ret; }
    constant_iterator& operator+=(ptrdiff_t n) { count_ += n; return *this; }
    constant_iterator& operator-=(ptrdiff_t n) { count_ -= n; return *this; }
    constant_iterator operator+(ptrdiff_t n) const { return { ref_, count_ + n }; }
    constant_iterator operator-(ptrdiff_t n) const { return { ref_, count_ - n }; }
    difference_type operator-(const constant_iterator& r) const { return count_ - r.count_; }
    bool operator<(const constant_iterator& r) const { return count_ < r.count_; }
    bool operator>(const constant_iterator& r) const { return count_ > r.count_; }
    bool operator==(const constant_iterator& r) const { return count_ == r.count_; }
    bool operator!=(const constant_iterator& r) const { return count_ != r.count_; }

    T& ref_;
    ptrdiff_t count_;
};
template<class T>
inline auto make_constant_iterator(T& v) {
    return constant_iterator<T>{v};
}

} // namespace ist
