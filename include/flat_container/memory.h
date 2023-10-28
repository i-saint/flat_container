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

// type traits

template<typename T>
constexpr bool is_pod_v = std::is_trivial_v<T>;

template<class Iter, class T, class = void>
constexpr bool is_iterator_v = false;
template<class Iter, class T>
constexpr bool is_iterator_v<Iter, T, typename std::enable_if_t<std::is_same_v<std::remove_const_t<typename std::iterator_traits<Iter>::value_type>, T>> > = true;


template <class T, class = void>
constexpr bool is_reallocatable_v = false;
template <class T>
constexpr bool is_reallocatable_v<T, std::enable_if_t<T::is_reallocatable>> = true;

template <class T, class = void>
constexpr bool have_buffer_v = false;
template <class T>
constexpr bool have_buffer_v<T, std::enable_if_t<T::have_inner_buffer>> = true;

template <class T, class = void>
constexpr bool is_remote_memory_v = false;
template <class T>
constexpr bool is_remote_memory_v<T, std::enable_if_t<T::is_remote_memory>> = true;

// std::construct_at() requires c++20 so define our own.
template<class T, class... Args>
inline constexpr T* _construct_at(T* p, Args&&... args)
{
    return new (p) T(std::forward<Args>(args)...);
}

// existing: count of existing (constructed) objects in dst
template<class T>
inline void _copy_construct(T* first, T* last, T* dst, size_t existing = 0)
{
    if constexpr (is_pod_v<T>) {
        std::memcpy(dst, first, sizeof(T) * std::distance(first, last));
    }
    else {
        size_t n = std::distance(first, last);
        auto end_assign = dst + std::min(n, existing);
        auto end_construct = dst + n;
        while (dst < end_assign) {
            *dst++ = *first++;
        }
        while (dst < end_construct) {
            _construct_at<T>(dst++, *first++);
        }
    }
}

// existing: count of existing (constructed) objects in dst
template<class T>
constexpr void _move_construct(T* first, T* last, T* dst, size_t existing = 0)
{
    if constexpr (is_pod_v<T>) {
        std::memcpy(dst, first, sizeof(T) * std::distance(first, last));
    }
    else {
        size_t n = std::distance(first, last);
        auto end_assign = dst + std::min(n, existing);
        auto end_construct = dst + n;
        while (dst < end_assign) {
            *dst++ = std::move(*first++);
        }
        while (dst < end_construct) {
            _construct_at<T>(dst++, std::move(*first++));
        }
    }
}
template<class T>
inline void _swap_content(T* data1, size_t size1, T* data2, size_t size2)
{
    if constexpr (is_pod_v<T>) {
        size_t swap_count = std::max(size1, size2);
        for (size_t i = 0; i < swap_count; ++i) {
            std::swap(data1[i], data2[i]);
        }
    }
    else {
        size_t swap_count = std::min(size1, size2);
        for (size_t i = 0; i < swap_count; ++i) {
            std::swap(data1[i], data2[i]);
        }
        if (size1 < size2) {
            for (size_t i = size1; i < size2; ++i) {
                _construct_at<T>(&data1[i], std::move(data2[i]));
                std::destroy_at(&data2[i]);
            }
        }
        if (size2 < size1) {
            for (size_t i = size2; i < size1; ++i) {
                _construct_at<T>(&data2[i], std::move(data1[i]));
                std::destroy_at(&data1[i]);
            }
        }
    }
}


// memory models

// typical dynamic memory model
template<class T, class Allocator = std::allocator<T>>
class dynamic_memory
{
public:
    using value_type = T;
    using allocator_type = Allocator;
    static constexpr bool is_reallocatable = true;

    dynamic_memory() = default;
    dynamic_memory(const dynamic_memory& r) { operator=(r); }
    dynamic_memory(dynamic_memory&& r) noexcept { operator=(std::move(r)); }

    ~dynamic_memory()
    {
        std::destroy(data_, data_ + size_);
        _deallocate(data_, capacity_);

        // clear for debug
        capacity_ = size_ = 0;
        data_ = nullptr;
    }

    dynamic_memory& operator=(const dynamic_memory& r)
    {
        _resize_capacity(r.size_);
        _copy_construct(r.data_, r.data_ + r.size_, data_, size_);
        size_ = r.size_;
        return *this;
    }

    dynamic_memory& operator=(dynamic_memory&& r) noexcept
    {
        swap(r);
        return *this;
    }

    void swap(dynamic_memory& r)
    {
        std::swap(capacity_, r.capacity_);
        std::swap(size_, r.size_);
        std::swap(data_, r.data_);
    }

protected:
    size_t& _capacity() { return capacity_; }
    size_t _capacity() const { return capacity_; }
    size_t& _size() { return size_; }
    size_t _size() const { return size_; }
    value_type*& _data() { return data_; }
    value_type* _data() const { return data_; }
    void _copy_on_write() {}

    value_type* _allocate(size_t size)
    {
        return size == 0 ? nullptr : allocator_type().allocate(size);
    }

    void _deallocate(value_type* addr, size_t size)
    {
        allocator_type().deallocate(addr, size);
    }

    void _resize_capacity(size_t new_capaity)
    {
        if (capacity_ == new_capaity) {
            return;
        }

        value_type* new_data = _allocate(new_capaity);
        _move_construct(data_, data_ + std::min(size_, new_capaity), new_data);
        std::destroy(data_, data_ + size_);
        _deallocate(data_, capacity_);

        data_ = new_data;
        capacity_ = new_capaity;
        size_ = std::min(size_, capacity_);
    }

private:
    size_t capacity_ = 0;
    size_t size_ = 0;
    value_type* data_ = nullptr;
};

// fixed size memory block
template<class T, size_t Capacity>
class fixed_memory
{
public:
    using value_type = T;
    static constexpr bool have_inner_buffer = true;
    constexpr size_t buffer_capacity() noexcept { return Capacity; }

    fixed_memory() = default;
    fixed_memory(const fixed_memory& r) { operator=(r); }
    fixed_memory(fixed_memory&& r) noexcept { operator=(std::move(r)); }

    ~fixed_memory()
    {
        std::destroy(_data(), _data() + _size());
        size_ = 0;
    }

    fixed_memory& operator=(const fixed_memory& r)
    {
        _copy_construct(r._data(), r._data() + r._size(), _data(), _size());
        size_ = r.size_;
        return *this;
    }

    fixed_memory& operator=(fixed_memory&& r) noexcept
    {
        swap(r);
        return *this;
    }

    void swap(fixed_memory& r)
    {
        _swap_content(_data(), _size(), r._data(), r._size());
        std::swap(size_, r.size_);
    }

protected:
    size_t _capacity() const { return Capacity; }
    size_t& _size() { return size_; }
    size_t _size() const { return size_; }
    value_type* _data() const { return (value_type*)buffer_; }
    void _copy_on_write() {}

private:
    size_t size_ = 0;
    char buffer_[sizeof(value_type) * Capacity]; // uninitialized in intention
};

// dynamic memory with small buffer optimization.
// if required size is smaller than internal buffer, internal buffer is used. otherwise, it allocates dynamic memory.
// so, it behaves like hybrid of dynamic_memory and fixed_memory.
// (many of std::string implementations use this strategy)
template<class T, size_t Capacity, class Allocator = std::allocator<T>>
class sbo_memory
{
public:
    using value_type = T;
    using allocator_type = Allocator;
    static constexpr bool is_reallocatable = true;
    static constexpr bool have_inner_buffer = true;

    constexpr size_t buffer_capacity() noexcept { return Capacity; }

    sbo_memory() = default;
    sbo_memory(const sbo_memory& r) { operator=(r); }
    sbo_memory(sbo_memory&& r) noexcept { operator=(std::move(r)); }

    ~sbo_memory()
    {
        std::destroy(data_, data_ + size_);
        _deallocate(data_, capacity_);

        // clear for debug
        capacity_ = size_ = 0;
        data_ = nullptr;
    }

    sbo_memory& operator=(const sbo_memory& r)
    {
        _resize_capacity(r.size_);
        _copy_construct(r.data_, r.data_ + r.size_, data_, size_);
        size_ = r.size_;
        return *this;
    }

    sbo_memory& operator=(sbo_memory&& r) noexcept
    {
        swap(r);
        return *this;
    }

    void swap(sbo_memory& r)
    {
        if ((char*)data_ != buffer_ && (char*)r.data_ != r.buffer_) {
            std::swap(capacity_, r.capacity_);
            std::swap(size_, r.size_);
            std::swap(data_, r.data_);
        }
        else {
            this->_swap_content(r);
        }
    }


protected:
    size_t& _capacity() { return capacity_; }
    size_t _capacity() const { return capacity_; }
    size_t& _size() { return size_; }
    size_t _size() const { return size_; }
    value_type*& _data() { return data_; }
    value_type* _data() const { return data_; }
    void _copy_on_write() {}

    value_type* _allocate(size_t size)
    {
        if (size <= Capacity) {
            return (value_type*)buffer_;
        }
        else {
            return allocator_type().allocate(size);
        }
    }

    void _deallocate(value_type* addr, size_t size)
    {
        if ((char*)addr != buffer_) {
            allocator_type().deallocate(addr, size);
        }
    }

    void _resize_capacity(size_t new_capaity)
    {
        new_capaity = std::max(new_capaity, Capacity);
        if (capacity_ == new_capaity) {
            return;
        }

        value_type* new_data = _allocate(new_capaity);
        _move_construct(data_, data_ + std::min(size_, new_capaity), new_data);
        std::destroy(data_, data_ + size_);
        _deallocate(data_, capacity_);

        data_ = new_data;
        capacity_ = new_capaity;
        size_ = std::min(size_, capacity_);
    }

private:
    size_t capacity_ = Capacity;
    size_t size_ = 0;
    value_type* data_ = (value_type*)buffer_;
    size_t pad_[1]; // align
    char buffer_[sizeof(T) * Capacity]; // uninitialized in intention
};

// "wrap" existing memory block.
// similar to std::span, but it takes ownership.
// that means, unlike std::span, mapped container have resize(), push_back() and insert().
// assigning mapped container copies each elements instead of swapping data pointer.
// mapped containers destroys elements in destructor.
// calling detach() waives ownership.
template<class T>
class remote_memory
{
public:
    using value_type = T;
    static constexpr bool is_remote_memory = true;

    remote_memory() = default;
    remote_memory(const remote_memory& r) { operator=(r); }
    remote_memory(remote_memory&& r) noexcept { operator=(std::move(r)); }

    remote_memory(void* data, size_t capacity, size_t size = 0)
        : capacity_(capacity), size_(size), data_((T*)data) {}

    ~remote_memory()
    {
        std::destroy(data_, data_ + size_);
        capacity_ = size_ = 0;
        data_ = nullptr;
    }

    remote_memory& operator=(const remote_memory& r)
    {
        _copy_construct(r.data_, r.data_ + r.size_, data_, size_);
        capacity_ = r.capacity_;
        size_ = r.size_;
        return *this;
    }

    remote_memory& operator=(remote_memory&& r) noexcept
    {
        swap(r);
        return *this;
    }

    void swap(remote_memory& r)
    {
        std::swap(capacity_, r.capacity_);
        std::swap(size_, r.size_);
        std::swap(data_, r.data_);
    }

    // detach memory without destroying existing elements
    void detach()
    {
        capacity_ = size_ = 0;
        data_ = nullptr;
    }

protected:
    size_t& _capacity() { return capacity_; }
    size_t _capacity() const { return capacity_; }
    size_t& _size() { return size_; }
    size_t _size() const { return size_; }
    T*& _data() { return data_; }
    T* _data() const { return data_; }

    void _copy_on_write() {}

private:
    size_t capacity_ = 0;
    size_t size_ = 0;
    T* data_ = nullptr;
};

} // namespace ist

