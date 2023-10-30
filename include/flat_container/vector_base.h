#pragma once
#include "memory.h"
#include "span.h"

namespace ist {

template<class Memory>
class vector_base : public Memory
{
using super = Memory;
public:

public:
    using value_type = typename super::value_type;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using iterator = pointer;
    using const_iterator = const_pointer;
    using memory_type = Memory;


    vector_base() = default;
    constexpr vector_base(vector_base&& r) noexcept = default;
    constexpr vector_base& operator=(vector_base&& r) noexcept = default;
    constexpr vector_base(const vector_base& r) = default;
    constexpr vector_base& operator=(const vector_base& r) = default;

    template<bool cond = has_remote_memory_v<super>, fc_require(cond)>
    constexpr vector_base(const void* data, size_t capacity, size_t size = 0)
        : super(data, capacity, size)
    {
    }

    // reserve()
    void reserve(size_t n)
    {
        if constexpr (has_resize_capacity_v<super>) {
            if (n > _capacity()) {
                _copy_on_write();
                size_t new_capaity = std::max<size_t>(n, _capacity() * 2);
                super::_resize_capacity(new_capaity);
            }
        }
    }

    // shrink_to_fit()
    void shrink_to_fit()
    {
        if constexpr (has_resize_capacity_v<super>) {
            if (_size() != _capacity()) {
                _copy_on_write();
                super::_resize_capacity(_size());
            }
        }
    }

    // clear()
    constexpr void clear()
    {
        if (_size() > 0) {
            _copy_on_write();
            _shrink(_size());
        }
    }

    // capacity()
    constexpr size_t capacity() const noexcept
    {
        return _capacity();
    }
    // size()
    constexpr size_t size() const noexcept
    {
        return _size();
    }
    // size_bytes()
    constexpr size_t size_bytes() const noexcept
    {
        return sizeof(value_type) * _size();
    }
    // empty()
    constexpr bool empty() const noexcept
    {
        return _size() == 0;
    }

    // data() / cdata()
    constexpr pointer data() noexcept
    {
        _copy_on_write();
        return _data();
    }
    constexpr const_pointer data() const noexcept
    {
        return _data();
    }
    constexpr const_pointer cdata() const noexcept
    {
        return _data();
 }

    // begin() / cbegin()
    constexpr iterator begin() noexcept
    {
        _copy_on_write();
        return _data();
    }
    constexpr const_iterator begin() const noexcept
    {
        return _data();
    }
    constexpr const_iterator cbegin() const noexcept
    {
        return _data();
    }

    // end() / cend()
    constexpr iterator end() noexcept
    {
        _copy_on_write();
        return _data() + _size();
    }
    constexpr const_iterator end() const noexcept
    {
        return _data() + _size();
    }
    constexpr const_iterator cend() const noexcept
    {
        return _data() + _size();
    }

    // at()
    constexpr reference at(size_t i)
    {
        _boundary_check(i);
        _copy_on_write();
        return _data()[i];
    }
    constexpr const_reference at(size_t i) const
    {
        _boundary_check(i);
        return _data()[i];
    }

    // operator[]
    constexpr reference operator[](size_t i)
    {
        _boundary_check(i);
        _copy_on_write();
        return _data()[i];
    }
    constexpr const_reference operator[](size_t i) const
    {
        _boundary_check(i);
        return _data()[i];
    }

    // front()
    constexpr reference front()
    {
        _boundary_check(1);
        _copy_on_write();
        return _data()[0];
    }
    constexpr const_reference front() const
    {
        _boundary_check(1);
        return _data()[0];
    }

    // back()
    constexpr reference back()
    {
        _boundary_check(1);
        _copy_on_write();
        return _data()[_size() - 1] ;
    }
    constexpr const_reference back() const
    {
        _boundary_check(1);
        return _data()[_size() - 1];
    }

    // span<value_type>
    constexpr operator span<value_type>() const noexcept
    {
        return { _data(), _size() };
    }

protected:
    using super::_data;
    using super::_size;
    using super::_capacity;

    void _copy_on_write()
    {
        if constexpr (has_copy_on_write_v<super>) {
            super::_copy_on_write();
        }
    }

    template<class Iter, class MaybeOverlapped = std::false_type>
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
            auto end_assign = std::min(dst + n, _data() + _size());
            auto end_construct = dst + n;
            while (dst < end_assign) {
                *dst++ = *first++;
            }
            while (dst < end_construct) {
                _construct_at<value_type>(dst++, *first++);
            }
        }
    }
    constexpr void _fill_range(iterator dst, const_reference v, size_t n)
    {
        if constexpr (is_pod_v<value_type>) {
            auto end_assign = dst + n;
            while (dst < end_assign) {
                *dst++ = v;
            }
        }
        else {
            auto end_assign = std::min(dst + n, _data() + _size());
            auto end_construct = dst + n;
            while (dst < end_assign) {
                *dst++ = v;
            }
            while (dst < end_construct) {
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
            auto end_assign = std::min(dst + n, _data() + _size());
            auto end_construct = dst + n;
            while (dst < end_assign) {
                *dst++ = std::move(*first++);
            }
            while (dst < end_construct) {
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
            if (dst >= _data() + _size()) {
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
            if (dst >= _data() + _size()) {
                _construct_at<value_type>(dst, std::forward<Args>(args)...);
            }
            else {
                *dst = value_type(std::forward<Args>(args)...);
            }
        }
    }

    template<class Construct>
    constexpr void _assign(size_t n, Construct&& construct)
    {
        this->reserve(n);
        _capacity_check(n);
        construct(_data());
        _destroy(_data() + n, _data() + _size());
        _size() = n;
    }

    constexpr void _shrink(size_t n)
    {
        size_t new_size = _size() - n;
        _capacity_check(new_size);
        _destroy(_data() + new_size, _data() + _size());
        _size() = new_size;
    }

    template<class Construct>
    constexpr void _expand(size_t n, Construct&& construct)
    {
        size_t new_size = _size() + n;
        this->reserve(new_size);
        _capacity_check(new_size);
        construct(_data() + _size());
        _size() = new_size;
    }

    template<class Construct>
    constexpr void _resize(size_t n, Construct&& construct)
    {
        if (n < _size()) {
            _shrink(_size() - n);
        }
        else if (n > _size()) {
            size_t exn = n - _size();
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
        size_t d = std::distance(_data(), pos);
        this->reserve(_size() + s);
        pos = _data() + d; // for the case realloc happened
        _move_backward(pos, _data() + _size(), _data() + _size() + s);
        construct(pos);
        _size() += s;
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
            auto end_construct = _data() + _size();
            auto end_assign = dst - n;
            while (dst > end_construct) {
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
        if (n > _capacity()) {
            throw std::out_of_range("out of capacity");
        }
#endif
    }

    constexpr void _boundary_check(size_t n) const
    {
#ifdef FC_ENABLE_CAPACITY_CHECK
        if (n > _size()) {
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
