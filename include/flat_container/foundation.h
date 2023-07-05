#pragma once
#include <cstddef>
#include <initializer_list>
#include <type_traits>
#include <iterator>
#include <type_traits>
#include <memory>

#ifdef _DEBUG
#   if !defined(FC_ENABLE_CAPACITY_CHECK)
#       define FC_ENABLE_CAPACITY_CHECK
#   endif
#endif

#define fc_require(...) std::enable_if_t<__VA_ARGS__, bool> = true

namespace ist {

template <typename T, typename = void>
constexpr bool is_dynamic_memory_v = false;
template <typename T>
constexpr bool is_dynamic_memory_v<T, std::enable_if_t<T::is_dynamic_memory>> = true;

template <typename T, typename = void>
constexpr bool is_sbo_memory_v = false;
template <typename T>
constexpr bool is_sbo_memory_v<T, std::enable_if_t<T::is_sbo_memory>> = true;

template <typename T, typename = void>
constexpr bool is_fixed_memory_v = false;
template <typename T>
constexpr bool is_fixed_memory_v<T, std::enable_if_t<T::is_fixed_memory>> = true;

template <typename T, typename = void>
constexpr bool is_memory_view_v = false;
template <typename T>
constexpr bool is_memory_view_v<T, std::enable_if_t<T::is_memory_view>> = true;

template<typename T, typename = void>
constexpr bool is_iterator_v = false;
template<typename T>
constexpr bool is_iterator_v<T, typename std::enable_if_t<!std::is_same_v<typename std::iterator_traits<T>::value_type, void>>> = true;

template<typename T>
constexpr bool is_pod_v = std::is_trivially_constructible_v<T> && std::is_trivially_copyable_v<T>;


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


template<class T>
class dynamic_memory
{
public:
    using value_type = T;
    static const bool is_dynamic_memory = true;

    dynamic_memory() {}
    ~dynamic_memory() { _deallocate(data_); }

    void reserve(size_t n)
    {
        if (n <= capacity_) {
            return;
        }
        size_t new_capaity = std::max<size_t>(n, capacity_ * 2);
        _reallocate(new_capaity);
    }

    void shrink_to_fit()
    {
        _reallocate(size_);
    }

protected:
    T* _allocate(size_t size)
    {
        return (T*)std::malloc(sizeof(T) * size);
    }
    void _deallocate(void *addr)
    {
        std::free(addr);
    }

    void _reallocate(size_t new_capaity)
    {
        T* new_data = _allocate(new_capaity);
        if constexpr (is_pod_v<T>) {
            std::memcpy(new_data, data_, sizeof(T) * size_);
        }
        else {
            for (size_t i = 0; i < size_; ++i) {
                _construct_at<T>(&new_data[i], std::move(data_[i]));
                _destroy_at(&data_[i]);
            }
        }

        _deallocate(data_);
        data_ = new_data;
        capacity_ = new_capaity;
    }

    size_t capacity_ = 0;
    size_t size_ = 0;
    T* data_ = nullptr;
};

// dynamic memory with small buffer optimization
template<class T, size_t Capacity>
class sbo_memory
{
public:
    using value_type = T;
    static const bool is_sbo_memory = true;
    static const size_t fixed_capacity = Capacity;

    sbo_memory() {}
    ~sbo_memory() { _deallocate(data_); }

    constexpr size_t buffer_capacity() noexcept { return buffer_capacity_; }

    void reserve(size_t n)
    {
        if (n <= capacity_) {
            return;
        }
        size_t new_capaity = std::max<size_t>(n, capacity_ * 2);
        _reallocate(new_capaity);
    }

    void shrink_to_fit()
    {
        _reallocate(size_);
    }

protected:
    T* _allocate(size_t size)
    {
        if (size <= buffer_capacity_) {
            return (T*)buffer_;
        }
        else {
            return (T*)std::malloc(sizeof(T) * size);
        }
    }
    void _deallocate(void* addr)
    {
        if (addr != buffer_) {
            std::free(addr);
        }
    }

    void _reallocate(size_t new_capaity)
    {
        T* new_data = _allocate(new_capaity);
        if (new_data == data_) {
            return;
        }

        if constexpr (is_pod_v<T>) {
            std::memcpy(new_data, data_, sizeof(T) * size_);
        }
        else {
            for (size_t i = 0; i < size_; ++i) {
                _construct_at<T>(new_data + i, std::move(data_[i]));
                _destroy_at(&data_[i]);
            }
        }

        _deallocate(data_);
        data_ = new_data;
        capacity_ = new_capaity;
    }

    static const size_t buffer_capacity_ = Capacity;
    size_t capacity_ = Capacity;
    size_t size_ = 0;
    T* data_ = (T*)buffer_;
    std::byte buffer_[sizeof(T) * Capacity]; // uninitialized in intention
};

template<class T, size_t Capacity>
class fixed_memory
{
public:
    using value_type = T;
    static const bool is_fixed_memory = true;
    static const size_t fixed_capacity = Capacity;

    fixed_memory() {}
    ~fixed_memory() {}

    constexpr void reserve(size_t n) {}
    constexpr void shrink_to_fit() {}

protected:
    static const size_t capacity_ = Capacity;
    size_t size_ = 0;
    union {
        T data_[0]; // for debug
        std::byte buffer_[sizeof(T) * Capacity]; // uninitialized in intention
    };
};

template<class T>
class mapped_memory
{
template<class X> friend class memory_boilerplate;
public:
    using value_type = T;
    static const bool is_memory_view = true;

    mapped_memory() {}
    mapped_memory(void* data, size_t capacity, size_t size = 0) : capacity_(capacity), size_(size), data_((T*)data) {}
    ~mapped_memory() {}

    constexpr void reserve(size_t n) {}
    constexpr void shrink_to_fit() {}

    // detach data from this view.
    // (if detach() or swap() are not called, elements are destroyed by destrunctor)
    constexpr void detach()
    {
        capacity_ = size_ = 0;
        data_ = nullptr;
    }

protected:
    size_t capacity_ = 0;
    size_t size_ = 0;
    T* data_ = nullptr;
};


template<class Memory>
class memory_boilerplate : public Memory
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


    memory_boilerplate() {}
    memory_boilerplate(const memory_boilerplate& r) { operator=(r); }
    memory_boilerplate(memory_boilerplate&& r) noexcept { operator=(std::move(r)); }
    template<bool view = is_memory_view_v<super>, fc_require(view)>
    constexpr memory_boilerplate(void* data, size_t capacity, size_t size = 0)
        : super(data, capacity, size)
    {
    }
    ~memory_boilerplate() { clear(); }

    memory_boilerplate& operator=(const memory_boilerplate& r)
    {
        if constexpr (is_memory_view_v<super>) {
            this->capacity_ = r.capacity_;
            this->size_ = r.size_;
            this->data_ = r.data_;
            return *this;
        }
        else {
            _assign(r.size(), [&](pointer dst) { _copy_range(dst, r.begin(), r.end()); });
        }
        return *this;
    }
    memory_boilerplate& operator=(memory_boilerplate&& r)
    {
        swap(r);
        return *this;
    }

    constexpr void swap(memory_boilerplate& r)
    {
        if constexpr (is_dynamic_memory_v<super> || is_memory_view_v<super>) {
            std::swap(this->capacity_, r.capacity_);
            std::swap(this->size_, r.size_);
            std::swap(this->data_, r.data_);
        }
        else if constexpr (is_sbo_memory_v<super>) {
            if (this->capacity_ > this->buffer_capacity_ && r.capacity_ > r.buffer_capacity_) {
                std::swap(this->capacity_, r.capacity_);
                std::swap(this->size_, r.size_);
                std::swap(this->data_, r.data_);
            }
            else {
                this->_swap_content(r);
            }
        }
        else if constexpr (super::is_fixed_memory) {
            this->_swap_content(r);
        }
    }

    constexpr void clear()
    {
        _shrink(this->size_);
    }

    constexpr size_t capacity() noexcept { return this->capacity_; }
    constexpr size_t size() const noexcept { return this->size_; }
    constexpr size_t size_bytes() const noexcept { return sizeof(value_type) * this->size_; }
    constexpr pointer data() noexcept { return this->data_; }
    constexpr const_pointer data() const noexcept { return this->data_; }

    constexpr bool empty() const noexcept { return this->size_ == 0; }
    constexpr iterator begin() noexcept { return this->data_; }
    constexpr const_iterator begin() const noexcept { return this->data_; }
    constexpr const_iterator cbegin() const noexcept { return this->data_; }
    constexpr iterator end() noexcept { return this->data_ + this->size_; }
    constexpr const_iterator end() const noexcept { return this->data_ + this->size_; }
    constexpr const_iterator cend() const noexcept { return this->data_ + this->size_; }
    constexpr reference at(size_t i) { _boundary_check(i); return this->data_[i]; }
    constexpr const_reference at(size_t i) const { _boundary_check(i); return this->data_[i]; }
    constexpr reference operator[](size_t i) { _boundary_check(i); return this->data_[i]; }
    constexpr const_reference operator[](size_t i) const { _boundary_check(i); return this->data_[i]; }
    constexpr reference front() { _boundary_check(1); return this->data_[0]; }
    constexpr const_reference front() const { _boundary_check(1); return this->data_[0]; }
    constexpr reference back() { _boundary_check(1); return this->data_[this->size_ - 1]; }
    constexpr const_reference back() const { _boundary_check(1); return this->data_[this->size_ - 1]; }

protected:
    template<class Iter>
    constexpr void _copy_range(iterator dst, Iter first, Iter last)
    {
        if constexpr (is_pod_v<value_type>) {
            if constexpr (std::is_pointer_v<Iter>) {
                std::memcpy(dst, first, sizeof(value_type) * std::distance(first, last));
            }
            else {
                std::copy(first, last, dst);
            }
        }
        else {
            size_t n = std::distance(first, last);
            auto end_assign = std::min(dst + n, this->data_ + this->size_);
            auto end_new = dst + n;
            while (dst < end_assign) {
                *dst++ = *first++;
            }
            while (dst < end_new) {
                _construct_at<value_type>(dst++, *first++);
            }
        }
    }
    constexpr void _copy_n(iterator dst, const value_type& v, size_t n)
    {
        if constexpr (is_pod_v<value_type>) {
            auto end_assign = dst + n;
            while (dst < end_assign) {
                *dst++ = v;
            }
        }
        else {
            auto end_assign = std::min(dst + n, this->data_ + this->size_);
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
            auto end_assign = std::min(dst + n, this->data_ + this->size_);
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
            if (dst >= this->data_ + this->size_) {
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
            if (dst >= this->data_ + this->size_) {
                _construct_at<value_type>(dst, std::forward<Args>(args)...);
            }
            else {
                *dst = value_type(std::forward<Args>(args)...);
            }
        }
    }

    constexpr void _swap_content(memory_boilerplate& r)
    {
        size_t max_size = std::max(this->size_, r.size_);
        this->reserve(max_size);
        r.reserve(max_size);

        if constexpr (is_pod_v<value_type>) {
            size_t swap_count = max_size;
            for (size_t i = 0; i < swap_count; ++i) {
                std::swap(this->data_[i], r[i]);
            }
            std::swap(this->size_, r.size_);
        }
        else {
            size_t size1 = this->size_;
            size_t size2 = r.size_;
            size_t swap_count = std::min(size1, size2);
            for (size_t i = 0; i < swap_count; ++i) {
                std::swap(this->data_[i], r[i]);
            }
            if (size1 < size2) {
                for (size_t i = size1; i < size2; ++i) {
                    _construct_at<value_type>(this->data_ + i, std::move(r[i]));
                    _destroy_at(&r[i]);
                }
            }
            if (size2 < size1) {
                for (size_t i = size2; i < size1; ++i) {
                    _construct_at<value_type>(r.data_ + i, std::move(this->data_[i]));
                    _destroy_at(&this->data_[i]);
                }
            }
            std::swap(this->size_, r.size_);
        }
    }

    template<class Construct>
    constexpr void _assign(size_t n, Construct&& construct)
    {
        this->reserve(n);
        _capacity_check(n);
        construct(this->data_);
        if constexpr (!is_pod_v<value_type>) {
            if (n < this->size_) {
                _destroy(this->data_ + n, this->data_ + this->size_);
            }
        }
        this->size_ = n;
    }

    constexpr void _shrink(size_t n)
    {
        size_t new_size = this->size_ - n;
        _capacity_check(new_size);
        if constexpr (!is_pod_v<value_type>) {
            _destroy(this->data_ + new_size, this->data_ + this->size_);
        }
        this->size_ = new_size;
    }

    template<class Construct>
    constexpr void _expand(size_t n, Construct&& construct)
    {
        size_t new_size = this->size_ + n;
        this->reserve(new_size);
        _capacity_check(new_size);
        construct(this->data_ + this->size_);
        this->size_ = new_size;
    }

    template<class Construct>
    constexpr void _resize(size_t n, Construct&& construct)
    {
        if (n < this->size_) {
            _shrink(this->size_ - n);
        }
        else if (n > this->size_) {
            size_t exn = n - this->size_;
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
        size_t d = std::distance(this->data_, pos);
        this->reserve(this->size_ + s);
        pos = this->data_ + d; // for the case realloc happened
        _move_backward(pos, this->data_ + this->size_, this->data_ + this->size_ + s);
        construct(pos);
        this->size_ += s;
        return pos;
    }

    constexpr iterator _move_backward(iterator first, iterator last, iterator dst)
    {
        size_t n = std::distance(first, last);
        if (n == 0) {
            return dst;
        }
        if constexpr (is_pod_v<value_type>) {
            value_type* end_assign = dst - n;
            while (dst > end_assign) {
                *(--dst) = *(--last);
            }
        }
        else {
            value_type* end_new = this->data_ + this->size_;
            value_type* end_assign = dst - n;
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
        if (n > this->capacity_) {
            throw std::out_of_range("out of capacity");
        }
#endif
    }

    constexpr void _boundary_check(size_t n) const
    {
#ifdef FC_ENABLE_CAPACITY_CHECK
        if (n > this->size_) {
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

