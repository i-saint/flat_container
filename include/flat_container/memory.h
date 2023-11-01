#pragma once
#include <cstddef>
#include <initializer_list>
#include <type_traits>
#include <iterator>
#include <type_traits>
#include <memory>
#include <functional>
#include <atomic>

#ifdef _DEBUG
#   if !defined(FC_ENABLE_CAPACITY_CHECK)
#       define FC_ENABLE_CAPACITY_CHECK
#   endif
#endif

#define fc_require(...) std::enable_if_t<__VA_ARGS__, bool> = true

namespace ist {

// type traits

// std::remove_cvref_t requires C++20 so define our own.
template<class T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template<typename T>
constexpr bool is_pod_v = std::is_trivial_v<T>;

template<class Iter, class T, class = void>
constexpr bool is_iterator_of_v = false;
template<class Iter, class T>
constexpr bool is_iterator_of_v<Iter, T, typename std::enable_if_t<std::is_same_v<std::remove_const_t<typename std::iterator_traits<Iter>::value_type>, T>> > = true;


template <class T, class = void>
constexpr bool has_resize_capacity_v = false;
template <class T>
constexpr bool has_resize_capacity_v<T, std::enable_if_t<T::has_resize_capacity>> = true;

template <class T, class = void>
constexpr bool has_inner_buffer_v = false;
template <class T>
constexpr bool has_inner_buffer_v<T, std::enable_if_t<T::has_inner_buffer>> = true;

template <class T, class = void>
constexpr bool has_remote_memory_v = false;
template <class T>
constexpr bool has_remote_memory_v<T, std::enable_if_t<T::has_remote_memory>> = true;

template <class T, class = void>
constexpr bool has_copy_on_write_v = false;
template <class T>
constexpr bool has_copy_on_write_v<T, std::enable_if_t<T::has_copy_on_write>> = true;


// std::construct_at() requires c++20 so define our own.
template<class T, class... Args>
inline constexpr T* _construct_at(T* p, Args&&... args)
{
    return new (p) T(std::forward<Args>(args)...);
}

template<class T>
inline void _destroy(T* first, T* last)
{
    if constexpr (is_pod_v<T>) {
        if (first < last) {
            std::destroy(first, last);
        }
    }
}
template<class T>
inline void _destroy_at(T* dst)
{
    if constexpr (is_pod_v<T>) {
        std::destroy_at(dst);
    }
}


// dst_size: count of existing (constructed) objects in dst
template<class T>
inline void _copy_content(T* src, size_t src_size, T* dst, size_t dst_capacity, size_t& dst_size)
{
    size_t n = std::min(src_size, dst_capacity);
    if constexpr (is_pod_v<T>) {
        std::memcpy(dst, src, sizeof(T) * n);
    }
    else {
        size_t num_copy = std::min(n, dst_size);
        size_t num_new = dst_size >= n ? 0 : n - dst_size;
        size_t pos = 0;

        std::copy_n(src, num_copy, dst);
        pos += num_copy;
        std::uninitialized_copy_n(src + pos, num_new, dst + pos);
        pos += num_new;
        _destroy(dst + pos, dst + dst_size);
    }
    dst_size = n;
}
template<class T>
inline void _copy_content(T* src, size_t src_size, T* dst, size_t dst_capacity)
{
    size_t dst_size = 0;
    _copy_content(src, src_size, dst, dst_capacity, dst_size);
}

// dst_size: count of existing (constructed) objects in dst
// ** elements of src will be moved and destroyed **
template<class T>
inline void _move_content(T* src, size_t src_size, T* dst, size_t dst_capacity, size_t& dst_size)
{
    size_t n = std::min(src_size, dst_capacity);
    if constexpr (is_pod_v<T>) {
        std::memcpy(dst, src, sizeof(T) * n);
    }
    else {
        size_t num_move = std::min(n, dst_size);
        size_t num_new = dst_size >= n ? 0 : n - dst_size;
        size_t pos = 0;

        std::move(src, src + num_move, dst);
        pos += num_move;
        std::uninitialized_move_n(src + pos, num_new, dst + pos);
        pos += num_new;
        _destroy(dst + pos, dst + dst_size);
        _destroy(src, src + src_size);
    }
    dst_size = n;
}
template<class T>
inline void _move_content(T* src, size_t src_size, T* dst, size_t dst_capacity)
{
    size_t dst_size = 0;
    _move_content(src, src_size, dst, dst_capacity, dst_size);
}

template<class T>
inline void _swap_content(T* data1, size_t& size1, T* data2, size_t& size2)
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
                _destroy_at(&data2[i]);
            }
        }
        if (size2 < size1) {
            for (size_t i = size2; i < size1; ++i) {
                _construct_at<T>(&data2[i], std::move(data1[i]));
                _destroy_at(&data1[i]);
            }
        }
    }
    std::swap(size1, size2);
}


// memory models

// typical dynamic memory model
template<class T, class Allocator = std::allocator<T>>
class dynamic_memory
{
public:
    using value_type = T;
    using allocator_type = Allocator;
    static constexpr bool has_resize_capacity = true;

    dynamic_memory() = default;
    dynamic_memory(const dynamic_memory& r) { operator=(r); }
    dynamic_memory(dynamic_memory&& r) noexcept { operator=(std::move(r)); }

    ~dynamic_memory()
    {
        _destroy(data_, data_ + size_);
        _deallocate(data_, capacity_);

        // clear for debug
        capacity_ = size_ = 0;
        data_ = nullptr;
    }

    dynamic_memory& operator=(const dynamic_memory& r)
    {
        if (r.size_ > capacity_) {
            _resize_capacity(r.size_);
        }
        _copy_content(r.data_, r.size_, data_, capacity_, size_);
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
    size_t _capacity() const { return capacity_; }
    size_t& _size() { return size_; }
    size_t _size() const { return size_; }
    value_type* _data() const { return data_; }

    void _resize_capacity(size_t new_capaity)
    {
        if (capacity_ == new_capaity) {
            return;
        }

        value_type* new_data = _allocate(new_capaity);
        _move_content(data_, size_, new_data, new_capaity);
        _deallocate(data_, capacity_);

        data_ = new_data;
        capacity_ = new_capaity;
        size_ = std::min(size_, new_capaity);
    }

private:
    value_type* _allocate(size_t size)
    {
        return size == 0 ? nullptr : allocator_type().allocate(size);
    }

    void _deallocate(value_type* data, size_t size)
    {
        allocator_type().deallocate(data, size);
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
    static constexpr bool has_inner_buffer = true;
    static_assert(alignof(value_type) <= 16);

    fixed_memory() = default;
    fixed_memory(const fixed_memory& r) { operator=(r); }
    fixed_memory(fixed_memory&& r) noexcept { operator=(std::move(r)); }

    ~fixed_memory()
    {
        _destroy(data_, data_ + size_);
        size_ = 0;
    }

    fixed_memory& operator=(const fixed_memory& r)
    {
        _copy_content(r.data_, r.size_, data_, capacity_, size_);
        return *this;
    }

    fixed_memory& operator=(fixed_memory&& r) noexcept
    {
        swap(r);
        return *this;
    }

    void swap(fixed_memory& r)
    {
        _swap_content(data_, size_, r.data_, r.size_);
    }

    constexpr size_t buffer_capacity() noexcept { return Capacity; }

protected:
    size_t _capacity() const { return capacity_; }
    size_t& _size() { return size_; }
    size_t _size() const { return size_; }
    value_type* _data() const { return data_; }

private:
    static constexpr size_t capacity_ = Capacity;
    size_t size_ = 0;
    value_type* data_ = reinterpret_cast<value_type*>(buffer_); // for debug and align

    // uninitialized in intention
    char buffer_[sizeof(value_type) * Capacity];
};

// dynamic memory with small buffer optimization.
// if required size is smaller than internal buffer, internal buffer is used. otherwise, it behaves like dynamic_memory.
// (many of std::string implementations use this strategy)
template<class T, size_t Capacity, class Allocator = std::allocator<T>>
class small_memory
{
public:
    using value_type = T;
    using allocator_type = Allocator;
    static constexpr bool has_resize_capacity = true;
    static constexpr bool has_inner_buffer = true;
    static_assert(alignof(value_type) <= 16);

    small_memory() = default;
    small_memory(const small_memory& r) { operator=(r); }
    small_memory(small_memory&& r) noexcept { operator=(std::move(r)); }

    ~small_memory()
    {
        _destroy(data_, data_ + size_);
        _deallocate(data_, capacity_);

        // clear for debug
        capacity_ = size_ = 0;
        data_ = nullptr;
    }

    small_memory& operator=(const small_memory& r)
    {
        if (r.size_ > capacity_) {
            _resize_capacity(r.size_);
        }
        _copy_content(r.data_, r.size_, data_, capacity_, size_);
        return *this;
    }

    small_memory& operator=(small_memory&& r) noexcept
    {
        swap(r);
        return *this;
    }

    void swap(small_memory& r)
    {
        if ((char*)data_ != buffer_ && (char*)r.data_ != r.buffer_) {
            std::swap(capacity_, r.capacity_);
            std::swap(size_, r.size_);
            std::swap(data_, r.data_);
        }
        else {
            size_t max_size = std::max(size_, r.size_);
            if (capacity_ < max_size) {
                _resize_capacity(max_size);
            }
            if (r.capacity_ < max_size) {
                r._resize_capacity(max_size);
            }
            _swap_content(data_, size_, r.data_, r.size_);
        }
    }

    constexpr size_t buffer_capacity() noexcept { return Capacity; }

protected:
    size_t _capacity() const { return capacity_; }
    size_t& _size() { return size_; }
    size_t _size() const { return size_; }
    value_type* _data() const { return data_; }

    void _resize_capacity(size_t new_capaity)
    {
        new_capaity = std::max(new_capaity, Capacity);
        if (capacity_ == new_capaity) {
            return;
        }

        value_type* new_data = _allocate(new_capaity);
        _move_content(data_, size_, new_data, new_capaity);
        _deallocate(data_, capacity_);

        data_ = new_data;
        capacity_ = new_capaity;
        size_ = std::min(size_, new_capaity);
    }

private:
    value_type* _allocate(size_t size)
    {
        if (size <= Capacity) {
            return (value_type*)buffer_;
        }
        else {
            return allocator_type().allocate(size);
        }
    }

    void _deallocate(value_type* data, size_t size)
    {
        if ((char*)data != buffer_) {
            allocator_type().deallocate(data, size);
        }
    }

private:
    size_t capacity_ = Capacity;
    size_t size_ = 0;
    value_type* data_ = reinterpret_cast<value_type*>(buffer_);

    // uninitialized in intention
    size_t pad_[1];
    char buffer_[sizeof(value_type) * Capacity];
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
    using release_handler = std::function<void(value_type* data, size_t size, size_t capacity)>;
    static constexpr bool has_remote_memory = true;


    remote_memory() = default;
    remote_memory(remote_memory&& r) noexcept { operator=(std::move(r)); }

    // movable but non-copyable
    remote_memory(const remote_memory& r) = delete;
    remote_memory& operator=(const remote_memory& r) = delete;

    remote_memory(const void* data, size_t capacity, size_t size = 0, release_handler&& on_release = destroy_elements)
        : capacity_(capacity), size_(size), data_((value_type*)data), on_release_(on_release)
    {}

    ~remote_memory()
    {
        dispose();
    }

    remote_memory& operator=(remote_memory&& r) noexcept
    {
        dispose();
        swap(r);
        return *this;
    }

    void swap(remote_memory& r)
    {
        std::swap(capacity_, r.capacity_);
        std::swap(size_, r.size_);
        std::swap(data_, r.data_);
    }


    bool valid() const
    {
        return data_ != nullptr;
    }

    void dispose()
    {
        if (on_release_) {
            on_release_(data_, size_, capacity_);
        }
        capacity_ = size_ = 0;
        data_ = nullptr;
    }

    static void destroy_elements(value_type* data, size_t size, size_t /*capacity*/)
    {
        _destroy(data, data + size);
    }

protected:
    size_t _capacity() const { return capacity_; }
    size_t& _size() { return size_; }
    size_t _size() const { return size_; }
    value_type* _data() const { return data_; }

private:
    size_t capacity_ = 0;
    size_t size_ = 0;
    value_type* data_ = nullptr;
    release_handler on_release_;
};


// reference counted shared memory.
// it will make a copy when non-const member function is called (copy-on-write).
template<class T, class Allocator = std::allocator<T>>
class shared_memory
{
public:
    using value_type = T;
    using allocator_type = Allocator;
    using release_handler = std::function<void(value_type* data, size_t size, size_t capacity)>;

    static constexpr bool has_resize_capacity = true;
    static constexpr bool has_remote_memory = true;
    static constexpr bool has_copy_on_write = true;

    shared_memory()
    {
        control_ = new control_block();
        control_->on_release_ = &_release_owned_memory;
    }
    shared_memory(const shared_memory& r) { operator=(r); }
    shared_memory(shared_memory&& r) noexcept { operator=(std::move(r)); }

    shared_memory(const void* data, size_t capacity, size_t size = 0, release_handler&& on_release = {})
        : shared_memory()
    {
        control_ = new control_block();
        control_->data_ = (value_type*)data;
        control_->capacity_ = capacity;
        control_->size_ = size;
        control_->on_release_ = on_release;
    }

    ~shared_memory()
    {
        dispose();
    }

    shared_memory& operator=(const shared_memory& r)
    {
        _decref();
        control_ = r.control_;
        control_->ref_count_++;
        return *this;
    }

    shared_memory& operator=(shared_memory&& r) noexcept
    {
        dispose();
        swap(r);
        return *this;
    }

    void swap(shared_memory& r)
    {
        std::swap(control_, r.control_);
    }


    bool valid() const
    {
        return control_ != nullptr;
    }

    void dispose()
    {
        _decref();
        control_ = nullptr;
    }

    size_t ref_count() const
    {
        return control_->ref_count_;
    }

protected:
    size_t _capacity() const { return control_->capacity_; }
    size_t& _size() { return control_->size_; }
    size_t _size() const { return control_->size_; }
    value_type* _data() const { return control_->data_; }

    void _copy_on_write()
    {
        if (control_->ref_count_ > 1) {
            auto old = control_;
            auto control_ = new control_block();
            control_->on_release_ = &_release_owned_memory;
            if (old->data_) {
                _resize_capacity(old->size_);
                _copy_content(old->data_, old->size_, control_->data_, control_->capacity_, control_->size_);
            }
        }
    }

    void _resize_capacity(size_t new_capaity)
    {
        auto& data_ = control_->data_;
        auto& capacity = control_->capacity_;
        auto& size_ = control_->size_;
        if (capacity == new_capaity) {
            return;
        }

        value_type* new_data = _allocate(new_capaity);
        _move_content(data_, size_, new_data, new_capaity);
        _deallocate(data_, capacity);

        data_ = new_data;
        capacity = new_capaity;
        size_ = std::min(size_, new_capaity);
    }

private:
    static void _release_owned_memory(value_type* data, size_t size, size_t capacity)
    {
        _destroy(data, data + size);
        allocator_type().deallocate(data, capacity);
    }

    value_type* _allocate(size_t size)
    {
        return size == 0 ? nullptr : allocator_type().allocate(size);
    }

    void _deallocate(value_type* data, size_t size)
    {
        allocator_type().deallocate(data, size);
    }

    void _decref()
    {
        if (control_ && --control_->ref_count_ == 0) {
            if (control_->on_release_) {
                control_->on_release_(control_->data_, control_->size_, control_->capacity_);
            }

            // clear for debug
            control_->capacity_ = control_->size_ = 0;
            control_->data_ = nullptr;

            delete control_;
            control_ = nullptr;
        }
    }

private:
    struct control_block
    {
        std::atomic<size_t> ref_count_{ 1 };
        release_handler on_release_;
        size_t capacity_ = 0;
        size_t size_ = 0;
        value_type* data_ = nullptr;
    };

    control_block* control_ = nullptr;
};


} // namespace ist

