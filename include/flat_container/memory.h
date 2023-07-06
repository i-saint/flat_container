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
constexpr bool is_mapped_memory_v = false;
template <typename T>
constexpr bool is_mapped_memory_v<T, std::enable_if_t<T::is_mapped_memory>> = true;


template<class T>
class dynamic_memory
{
public:
    using value_type = T;
    static const bool is_dynamic_memory = true;

protected:
    T* _allocate(size_t size)
    {
        if (size == 0) {
            return nullptr;
        }
        return (T*)std::malloc(sizeof(T) * size);
    }

    void _deallocate(void *addr)
    {
        std::free(addr);
    }

    template<class Move>
    void _reallocate(size_t new_capaity, Move&& move)
    {
        if (capacity_ == new_capaity) {
            return;
        }
        T* new_data = _allocate(new_capaity);
        move(new_data);

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

    constexpr size_t buffer_capacity() noexcept { return buffer_capacity_; }

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

    template<class Move>
    void _reallocate(size_t new_capaity, Move&& move)
    {
        if (capacity_ == new_capaity) {
            return;
        }
        T* new_data = _allocate(new_capaity);
        if (new_data == data_) {
            return;
        }
        move(new_data);

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
public:
    using value_type = T;
    static const bool is_mapped_memory = true;

    mapped_memory() {}
    mapped_memory(void* data, size_t capacity, size_t size = 0) : capacity_(capacity), size_(size), data_((T*)data) {}
    ~mapped_memory() {}

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

} // namespace ist

