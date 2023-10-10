#pragma once
#include <cstring>
#include <iostream>
#include <functional>

namespace ist {

class memory_view_streambuf : public std::streambuf
{
    using super = std::streambuf;
public:
    using underflow_handler = std::function<bool(char*&, size_t&, char*&)>;
    using overflow_handler = std::function<bool(char*&, size_t&)>;
    using destroy_handler = std::function<void()>;

    memory_view_streambuf() = default;
    memory_view_streambuf(const void* data, size_t size);
    ~memory_view_streambuf() override;
    // movable but non-copyable
    memory_view_streambuf(memory_view_streambuf&& v) noexcept = default;
    memory_view_streambuf& operator=(memory_view_streambuf&& v) noexcept = default;
    memory_view_streambuf(const memory_view_streambuf& v) = delete;
    memory_view_streambuf& operator=(const memory_view_streambuf& v) = delete;

    // overrides
    pos_type seekoff(off_type off,
        std::ios::seekdir dir,
        std::ios::openmode mode = std::ios::in | std::ios::out) override;
    pos_type seekpos(pos_type pos, std::ios::openmode mode = std::ios::in | std::ios::out) override;
    int underflow() override;
    int overflow(int c) override;
    std::streamsize xsgetn(char* ptr, std::streamsize count) override;
    std::streamsize xsputn(const char* ptr, std::streamsize count) override;

    // extensions
    void swap(memory_view_streambuf& v) noexcept;
    char* data() noexcept { return data_; }
    const char* data() const noexcept { return data_; }
    size_t size() const noexcept { return size_; }

    void reset(const void* data, size_t size);
    void set_underflow_handler(underflow_handler&& f) { on_underflow_ = std::move(f); }
    void set_overflow_handler(overflow_handler&& f) { on_overflow_ = std::move(f); }
    void set_destroy_handler(destroy_handler&& f) { on_destroy_ = std::move(f); }

public:
    char* data_{};
    size_t size_ = 0;
    underflow_handler on_underflow_;
    overflow_handler on_overflow_;
    destroy_handler on_destroy_;
};

class memory_view_stream : public std::iostream
{
    using super = std::iostream;
public:
    using underflow_handler = memory_view_streambuf::underflow_handler;
    using overflow_handler = memory_view_streambuf::overflow_handler;
    using destroy_handler = memory_view_streambuf::destroy_handler;

    memory_view_stream();
    memory_view_stream(const void* data, size_t size);
    // movable but non-copyable
    memory_view_stream(memory_view_stream&& v) noexcept;
    memory_view_stream& operator=(memory_view_stream&& v) noexcept;
    memory_view_stream(const memory_view_stream& v) = delete;
    memory_view_stream& operator=(const memory_view_stream& rhs) = delete;

    void swap(memory_view_stream& v) noexcept;
    memory_view_streambuf* rdbuf() const noexcept { return const_cast<memory_view_streambuf*>(&buf_); }
    char* data() noexcept { return buf_.data(); }
    const char* data() const noexcept { return buf_.data(); }
    size_t size() const noexcept { return buf_.size(); }

    void reset(const void* data, size_t size) { buf_.reset(data, size); }
    void set_underflow_handler(underflow_handler&& f) { buf_.set_underflow_handler(std::move(f)); }
    void set_overflow_handler(overflow_handler&& f) { buf_.set_overflow_handler(std::move(f)); }
    void set_destroy_handler(destroy_handler&& f) { buf_.set_destroy_handler(std::move(f)); }

private:
    memory_view_streambuf buf_;
};


#pragma region impl
inline memory_view_streambuf::memory_view_streambuf(const void* data, size_t size)
{
    reset(data, size);
}

inline memory_view_streambuf::~memory_view_streambuf()
{
    if (on_destroy_) {
        on_destroy_();
    }
}

inline memory_view_streambuf::pos_type
memory_view_streambuf::seekoff(off_type off, std::ios::seekdir dir, std::ios::openmode mode)
{
    char* head = data_;
    char* tail = head + size_;

    if (mode & std::ios::out)
    {
        char* current = this->pptr();

        if (dir == std::ios::beg)
            current = head + off;
        else if (dir == std::ios::cur)
            current += off;
        else if (dir == std::ios::end)
            current = tail - off;

        this->setp(current, tail);
        return pos_type(current - head);
    }
    else if (mode & std::ios::in)
    {
        char* current = this->gptr();
        if (dir == std::ios::beg)
            current = head + off;
        else if (dir == std::ios::cur)
            current += off;
        else if (dir == std::ios::end)
            current = tail - off;

        this->setg(current, current, tail);
        return pos_type(current - head);
    }
    return pos_type(-1);
}

inline memory_view_streambuf::pos_type
memory_view_streambuf::seekpos(pos_type pos, std::ios::openmode mode)
{
    return seekoff(pos, std::ios::beg, mode);
}

inline int
memory_view_streambuf::underflow()
{
    char* head = data_;
    char* tail = head + size_;
    char* cur = this->gptr();
    if (cur < tail) {
        // buffer is not exhausted yet. just update position.
        int ret = *cur++;
        this->setg(cur, cur, tail);
        return ret;
    }
    else {
        // call underflow handler. update buffer and position if handled properly.
        if (on_underflow_ && on_underflow_(data_, size_, cur)) {
            this->setg(cur, cur, tail);
            return 0;
        }
        else {
            return traits_type::eof();
        }
    }
}

inline int
memory_view_streambuf::overflow(int c)
{
    char* head = data_;
    char* tail = head + size_;
    char* cur = this->pptr();
    if (cur < tail) {
        // buffer is not exhausted yet. just update position.
        *cur++ = (char)c;
        this->setp(cur, tail);
    }
    else {
        // call overflow handler. update buffer and position if handled properly.
        size_t old = size_;
        if (on_overflow_ && on_overflow_(data_, size_)) {
            cur = data_ + old;
            *cur++ = (char)c;
            this->setp(cur, data_ + size_);
        }
        else {
            return traits_type::eof();
        }
    }
    return c;
}

// std::streambuf implements xsgetn() and xsputn().
// but MSVC's has a problem when count > INT_MAX.
// so we implement our own.

inline std::streamsize
memory_view_streambuf::xsgetn(char* dst, std::streamsize count)
{
    char* head = data_;
    char* tail = head + size_;
    char* src = this->gptr();

    size_t remain = count;
    while (remain) {
        size_t n = std::min(remain, size_t(tail - src));
        std::memcpy(dst, src, n);
        dst += n;
        src += n;
        remain -= n;
        this->setg(src, src, tail);

        if (remain) {
            if (underflow() == traits_type::eof()) {
                return count - remain;
            }
            head = data_;
            tail = data_ + size_;
            src = this->gptr();
        }
    }
    return count;
}

inline std::streamsize
memory_view_streambuf::xsputn(const char* src, std::streamsize count)
{
    char* head = data_;
    char* tail = data_ + size_;
    char* dst = this->pptr();

    size_t remain = count;
    while (remain) {
        size_t n = std::min(remain, size_t(tail - dst));
        std::memcpy(dst, src, n);
        src += n;
        dst += n;
        remain -= n;
        this->setp(dst, tail);

        if (remain) {
            if (overflow(*src++) == traits_type::eof()) {
                return count - remain;
            }
            --remain;
            head = data_;
            tail = data_ + size_;
            dst = this->pptr();
        }
    }
    return count;
}

inline void
memory_view_streambuf::swap(memory_view_streambuf& v) noexcept
{
    super::swap(v);
    std::swap(data_, v.data_);
    std::swap(size_, v.size_);
    std::swap(on_underflow_, v.on_underflow_);
    std::swap(on_overflow_, v.on_overflow_);
    std::swap(on_destroy_, v.on_destroy_);
}


inline void
memory_view_streambuf::reset(const void* data, size_t size)
{
    data_ = (char*)data;
    size_ = size;
    this->setp(data_, data_ + size_);
    this->setg(data_, data_, data_ + size_);
}


inline memory_view_stream::memory_view_stream()
    : super(&buf_)
{
}

inline memory_view_stream::memory_view_stream(const void* data, size_t size)
    : super(&buf_)
{
    reset(data, size);
}

inline memory_view_stream::memory_view_stream(memory_view_stream&& v) noexcept
    : super(&buf_)
{
}

inline memory_view_stream&
memory_view_stream::operator=(memory_view_stream&& v) noexcept
{
    swap(v);
    return *this;
}

inline void
memory_view_stream::swap(memory_view_stream& v) noexcept
{
    super::swap(v);
    buf_.swap(v.buf_);
}
#pragma endregion impl

} // namespace ist
