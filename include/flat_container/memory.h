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
constexpr bool is_memory_view_v = false;
template <class T>
constexpr bool is_memory_view_v<T, std::enable_if_t<T::is_memory_view>> = true;


template<class T>
inline void _move_and_destroy(T* first, T* last, T* dst)
{
    if constexpr (is_pod_v<T>) {
        std::memcpy(dst, first, sizeof(T) * std::distance(first, last));
    }
    else {
        std::move(first, last, dst);
        std::destroy(first, last);
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

protected:
    size_t& _capacity() { return capacity_; }
    size_t _capacity() const { return capacity_; }
    size_t& _size() { return size_; }
    size_t _size() const { return size_; }
    value_type*& _data() { return data_; }
    value_type* _data() const { return data_; }
    void _copy_on_write() {}

    void _resize_capacity(size_t new_capaity)
    {
        if (capacity_ == new_capaity) {
            return;
        }

        value_type* new_data = allocator_type().allocate(new_capaity);
        _move_and_destroy(data_, data_ + size_, new_data);
        allocator_type().deallocate(data_, capacity_);

        data_ = new_data;
        capacity_ = new_capaity;
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

    void _deallocate(void* addr)
    {
        if (addr != buffer_) {
            allocator_type().deallocate(data_, capacity_);
        }
    }

    void _resize_capacity(size_t new_capaity)
    {
        if (capacity_ == new_capaity) {
            return;
        }

        value_type* new_data = _allocate(new_capaity);
        _move_and_destroy(data_, data_ + size_, new_data);
        _deallocate(data_);

        data_ = new_data;
        capacity_ = new_capaity;
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
class memory_view
{
public:
    using value_type = T;
    static constexpr bool is_memory_view = true;

    memory_view() = default;
    memory_view(void* data, size_t capacity, size_t size = 0) : capacity_(capacity), size_(size), data_((T*)data) {}

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

