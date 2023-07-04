#pragma once
#include <cstddef>
#include <initializer_list>
#include <type_traits>
#include <iterator>
#include <type_traits>

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
constexpr bool is_pod_v = std::is_trivially_constructible_v<T>;


template<class ThisT, class T>
class memory_boilerplate
{
public:
    using value_type = T;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using iterator = pointer;
    using const_iterator = const_pointer;

    constexpr size_t capacity() noexcept { return get().capacity_; }
    constexpr size_t size() const noexcept { return get().size_; }
    constexpr size_t size_bytes() const noexcept { return sizeof(T) * get().size_; }
    constexpr pointer data() noexcept { return get().data_; }
    constexpr const_pointer data() const noexcept { return get().data_; }

    constexpr bool empty() const noexcept { return get().size_ == 0; }
    constexpr iterator begin() noexcept { return get().data_; }
    constexpr const_iterator begin() const noexcept { return get().data_; }
    constexpr const_iterator cbegin() const noexcept { return get().data_; }
    constexpr iterator end() noexcept { return get().data_ + get().size_; }
    constexpr const_iterator end() const noexcept { return get().data_ + get().size_; }
    constexpr const_iterator cend() const noexcept { return get().data_ + get().size_; }
    constexpr reference at(size_t i) { _boundary_check(i); return get().data_[i]; }
    constexpr const_reference at(size_t i) const { _boundary_check(i); return get().data_[i]; }
    constexpr reference operator[](size_t i) { _boundary_check(i); return get().data_[i]; }
    constexpr const_reference operator[](size_t i) const { _boundary_check(i); return get().data_[i]; }
    constexpr reference front() { _boundary_check(1); return get().data_[0]; }
    constexpr const_reference front() const { _boundary_check(1); return get().data_[0]; }
    constexpr reference back() { _boundary_check(1); return get().data_[get().size_ - 1]; }
    constexpr const_reference back() const { _boundary_check(1); return get().data_[get().size_ - 1]; }

    constexpr void clear()
    {
        _shrink(get().size_);
    }

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
            T* end_assign = std::min(dst + n, get().data_ + get().size_);
            T* end_new = dst + n;
            while (dst < end_assign) {
                *dst++ = *first++;
            }
            while (dst < end_new) {
                new (dst++) T(*first++);
            }
        }
    }
    constexpr void _copy_n(iterator dst, const T& v, size_t n)
    {
        if constexpr (is_pod_v<value_type>) {
            T* end_assign = dst + n;
            while (dst < end_assign) {
                *dst++ = v;
            }
        }
        else {
            T* end_assign = std::min(dst + n, get().data_ + get().size_);
            T* end_new = dst + n;
            while (dst < end_assign) {
                *dst++ = v;
            }
            while (dst < end_new) {
                new (dst++) T(v);
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
            T* end_assign = std::min(dst + n, get().data_ + get().size_);
            T* end_new = dst + n;
            while (dst < end_assign) {
                *dst++ = std::move(*first++);
            }
            while (dst < end_new) {
                new (dst++) T(std::move(*first++));
            }
        }
    }
    constexpr void _move_one(iterator dst, T&& v)
    {
        if constexpr (is_pod_v<value_type>) {
            *dst = v;
        }
        else {
            if (dst >= get().data_ + get().size_) {
                new (dst) T(std::move(v));
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
            *dst = T(std::forward<Args>(args)...);
        }
        else {
            if (dst >= get().data_ + get().size_) {
                new (dst) T(std::forward<Args>(args)...);
            }
            else {
                *dst = T(std::forward<Args>(args)...);
            }
        }
    }


    template<class R>
    constexpr void _swap_content(R& r)
    {
        size_t max_size = std::max(get().size_, r.size_);
        get().reserve(max_size);
        r.reserve(max_size);

        if constexpr (is_pod_v<value_type>) {
            size_t swap_count = max_size;
            for (size_t i = 0; i < swap_count; ++i) {
                std::swap(get().data_[i], r[i]);
            }
            std::swap(get().size_, r.size_);
        }
        else {
            size_t size1 = get().size_;
            size_t size2 = r.size_;
            size_t swap_count = std::min(size1, size2);
            for (size_t i = 0; i < swap_count; ++i) {
                std::swap(get().data_[i], r[i]);
            }
            if (size1 < size2) {
                for (size_t i = size1; i < size2; ++i) {
                    new (get().data_ + i) value_type(std::move(r[i]));
                    r[i].~value_type();
                }
            }
            if (size2 < size1) {
                for (size_t i = size2; i < size1; ++i) {
                    new (r.data_ + i) value_type(std::move(get().data_[i]));
                    get().data_[i].~value_type();
                }
            }
            std::swap(get().size_, r.size_);
        }
    }

    template<class Construct>
    constexpr void _assign(size_t n, Construct&& construct)
    {
        get().reserve(n);
        _capacity_check(n);
        construct(get().data_);
        if constexpr (!is_pod_v<value_type>) {
            for (size_t i = n; i < get().size_; ++i) {
                get().data_[i].~value_type();
            }
        }
        get().size_ = n;
    }

    constexpr void _shrink(size_t n)
    {
        size_t new_size = get().size_ - n;
        _capacity_check(new_size);
        if constexpr (!is_pod_v<value_type>) {
            for (size_t i = new_size; i < get().size_; ++i) {
                get().data_[i].~value_type();
            }
        }
        get().size_ = new_size;
    }

    template<class Construct>
    constexpr void _expand(size_t n, Construct&& construct)
    {
        size_t new_size = get().size_ + n;
        get().reserve(new_size);
        _capacity_check(new_size);
        construct(get().data_ + get().size_);
        get().size_ = new_size;
    }

    template<class Construct>
    constexpr void _resize(size_t n, Construct&& construct)
    {
        if (n < get().size_) {
            _shrink(get().size_ - n);
        }
        else if (n > get().size_) {
            size_t exn = n - get().size_;
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
        size_t d = std::distance(get().data_, pos);
        get().reserve(get().size_ + s);
        pos = get().data_ + d; // for the case realloc happened
        _move_backward(pos, get().data_ + get().size_, get().data_ + get().size_ + s);
        construct(pos);
        get().size_ += s;
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
            value_type* end_new = get().data_ + get().size_;
            value_type* end_assign = dst - n;
            while (dst > end_new) {
                new (--dst) value_type(std::move(*(--last)));
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
        if (n > get().capacity_) {
            throw std::out_of_range("out of capacity");
        }
#endif
    }

    constexpr void _boundary_check(size_t n) const
    {
#ifdef FC_ENABLE_CAPACITY_CHECK
        if (n > get().size_) {
            throw std::out_of_range("out of range");
        }
#endif
    }

private:
    constexpr ThisT& get() noexcept { return *(ThisT*)this; }
    constexpr const ThisT& get() const noexcept { return *(ThisT*)this; }
};

template<class T>
class dynamic_memory : public memory_boilerplate<dynamic_memory<T>, T>
{
template<class X, class Y> friend class memory_boilerplate;
public:
    static const bool is_dynamic_memory = true;

    dynamic_memory() {}
    dynamic_memory(const dynamic_memory& r) { operator=(r); }
    dynamic_memory(dynamic_memory&& r) noexcept { operator=(std::move(r)); }
    ~dynamic_memory() { release(); }

    dynamic_memory& operator=(const dynamic_memory& r)
    {
        this->_assign(r.size(), [&](T* dst) { this->_copy_range(dst, r.begin(), r.end()); });
        return *this;
    }
    dynamic_memory& operator=(dynamic_memory&& r)
    {
        swap(r);
        return *this;
    }

    constexpr void swap(dynamic_memory& r)
    {
        std::swap(capacity_, r.capacity_);
        std::swap(size_, r.size_);
        std::swap(data_, r.data_);
    }

    void reserve(size_t n)
    {
        if (n <= capacity_) {
            return;
        }
        size_t new_capaity = std::max<size_t>(n, capacity_ * 2);
        reallocate(new_capaity);
    }

    void shrink_to_fit()
    {
        reallocate(size_);
    }

protected:
    T* allocate(size_t size)
    {
        return (T*)std::malloc(sizeof(T) * size);
    }
    void deallocate(void *addr)
    {
        std::free(addr);
    }

    void reallocate(size_t new_capaity)
    {
        T* new_data = allocate(new_capaity);
        if constexpr (is_pod_v<T>) {
            std::memcpy(new_data, data_, sizeof(T) * size_);
        }
        else {
            for (size_t i = 0; i < size_; ++i) {
                new (new_data + i) T(std::move(data_[i]));
                data_[i].~T();
            }
        }

        deallocate(data_);
        capacity_ = new_capaity;
        data_ = new_data;
    }

    void release()
    {
        this->clear();
        deallocate(data_);

        capacity_ = size_ = 0;
        data_ = nullptr;
    }

    size_t capacity_ = 0;
    size_t size_ = 0;
    T* data_ = nullptr;
};

// dynamic memory with small buffer optimization
template<class T, size_t Capacity>
class sbo_memory : public memory_boilerplate<sbo_memory<T, Capacity>, T>
{
template<class X, class Y> friend class memory_boilerplate;
public:
    static const bool is_sbo_memory = true;
    static const size_t fixed_capacity = Capacity;

#pragma warning(disable:26495)
    sbo_memory() {}
#pragma warning(default:26495)
    sbo_memory(const sbo_memory& r) { operator=(r); }
    sbo_memory(sbo_memory&& r) noexcept { operator=(std::move(r)); }
    ~sbo_memory() { release(); }

    sbo_memory& operator=(const sbo_memory& r)
    {
        this->_assign(r.size(), [&](T* dst) { this->_copy_range(dst, r.begin(), r.end()); });
        return *this;
    }
    sbo_memory& operator=(sbo_memory&& r)
    {
        swap(r);
        return *this;
    }

    constexpr void swap(sbo_memory& r)
    {
        if constexpr (capacity_ > buffer_capacity_ && r.capacity_ > r.buffer_capacity_) {
            std::swap(capacity_, r.capacity_);
            std::swap(size_, r.size_);
            std::swap(data_, r.data_);
        }
        else {
            this->_swap_content(r);
        }
    }

    constexpr size_t buffer_capacity() noexcept { return buffer_capacity_; }

    void reserve(size_t n)
    {
        if (n <= capacity_) {
            return;
        }
        size_t new_capaity = std::max<size_t>(n, capacity_ * 2);
        reallocate(new_capaity);
    }

    void shrink_to_fit()
    {
        reallocate(size_);
    }

protected:
    T* allocate(size_t size)
    {
        if (size <= buffer_capacity_) {
            return (T*)buffer_;
        }
        else {
            return (T*)std::malloc(sizeof(T) * size);
        }
    }
    void deallocate(void* addr)
    {
        if (addr != buffer_) {
            std::free(addr);
        }
    }

    void reallocate(size_t new_capaity)
    {
        T* new_data = allocate(new_capaity);
        if (new_data == data_) {
            return;
        }

        if constexpr (is_pod_v<T>) {
            std::memcpy(new_data, data_, sizeof(T) * size_);
        }
        else {
            for (size_t i = 0; i < size_; ++i) {
                new (new_data + i) T(std::move(data_[i]));
                data_[i].~T();
            }
        }

        deallocate(data_);
        capacity_ = new_capaity;
        data_ = new_data;
    }

    void release()
    {
        this->clear();
        deallocate(data_);

        capacity_ = size_ = 0;
        data_ = nullptr;
    }

    static const size_t buffer_capacity_ = Capacity;
    size_t capacity_ = Capacity;
    size_t size_ = 0;
    T* data_ = (T*)buffer_;
    std::byte buffer_[sizeof(T) * Capacity]; // uninitialized in intention
};

template<class T, size_t Capacity>
class fixed_memory : public memory_boilerplate<fixed_memory<T, Capacity>, T>
{
template<class X, class Y> friend class memory_boilerplate;
public:
    static const bool is_fixed_memory = true;
    static const size_t fixed_capacity = Capacity;

    // suppress warning for uninitialized members
#pragma warning(disable:26495)
    fixed_memory() {}
#pragma warning(default:26495)
    fixed_memory(const fixed_memory& r) { operator=(r); }
    fixed_memory(fixed_memory&& r) noexcept { operator=(std::move(r)); }
    ~fixed_memory() { this->clear(); }

    constexpr fixed_memory& operator=(const fixed_memory& r)
    {
        this->_assign(r.size(), [&](T* dst) { this->_copy_range(dst, r.begin(), r.end()); });
        return *this;
    }
    constexpr fixed_memory& operator=(fixed_memory&& r)
    {
        swap(r);
        return *this;
    }

    constexpr void swap(fixed_memory& r)
    {
        this->_swap_content(r);
    }

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
class memory_view : public memory_boilerplate<memory_view<T>, T>
{
template<class X, class Y> friend class memory_boilerplate;
public:
    static const bool is_memory_view = true;

    memory_view() {}
    memory_view(const memory_view& r) { operator=(r); }
    memory_view(memory_view&& r) noexcept { operator=(std::move(r)); }
    memory_view(void* data, size_t capacity, size_t size = 0) : capacity_(capacity), size_(size), data_((T*)data) {}
    ~memory_view() { this->clear(); }

    memory_view& operator=(const memory_view& r)
    {
        capacity_ = r.capacity_;
        size_ = r.size_;
        data_ = r.data_;
        return *this;
    }
    memory_view& operator=(memory_view&& r)
    {
        swap(r);
        return *this;
    }

    constexpr void swap(memory_view& r)
    {
        std::swap(capacity_, r.capacity_);
        std::swap(size_, r.size_);
        std::swap(data_, r.data_);
    }

    constexpr void reserve(size_t n) {}
    constexpr void shrink_to_fit() {}

    // detach data from this view.
    // (if detach() or swap() are not called, elements are destroyed by ~memory_view())
    constexpr void detach()
    {
        *this = {};
    }

protected:
    size_t capacity_ = 0;
    size_t size_ = 0;
    T* data_ = nullptr;
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

