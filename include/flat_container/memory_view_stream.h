#pragma once
#include <iostream>
#include <functional>

namespace ist {

class memory_view_streambuf : public std::streambuf
{
    using super = std::streambuf;
public:
    using overflow_handler = std::function<bool(char*&, size_t&, char*&)>;
    using underflow_handler = std::function<bool(char*&, size_t&, char*&)>;

    memory_view_streambuf() = default;
    memory_view_streambuf(memory_view_streambuf&& v) noexcept = default;
    memory_view_streambuf& operator=(memory_view_streambuf&& v) noexcept = default;
    memory_view_streambuf(const memory_view_streambuf& v) = delete;
    memory_view_streambuf& operator=(const memory_view_streambuf& v) = delete;
    memory_view_streambuf(const void* data, size_t size) { reset(data, size); }

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
    char* data() { return data_; }
    const char* data() const { return data_; }
    size_t size() const { return size_; }

    void reset(const void* data, size_t size)
    {
        data_ = (char*)data;
        size_ = size;
    }
    void set_underflow_handler(underflow_handler&& f) { on_underflow_ = std::move(f); }
    void set_overflow_handler(overflow_handler&& f) { on_overflow_ = std::move(f); }

public:
    char* data_{};
    size_t size_ = 0;
    underflow_handler on_underflow_;
    overflow_handler on_overflow_;
};

class memory_view_stream : public std::iostream
{
    using super = std::iostream;
public:
    using overflow_handler = memory_view_streambuf::overflow_handler;
    using underflow_handler = memory_view_streambuf::underflow_handler;

    memory_view_stream();
    memory_view_stream(memory_view_stream&& v) noexcept;
    memory_view_stream& operator=(memory_view_stream&& v) noexcept;
    memory_view_stream(const memory_view_stream& v) = delete;
    memory_view_stream& operator=(const memory_view_stream& rhs) = delete;
    memory_view_stream(const void* data, size_t size);

    memory_view_streambuf* rdbuf() const { return const_cast<memory_view_streambuf*>(&buf_); }
    char* data() { return buf_.data(); }
    const char* data() const { return buf_.data(); }
    size_t size() const { return buf_.size(); }

    void reset(const void* data, size_t size) { buf_.reset(data, size); }
    void set_underflow_handler(underflow_handler&& f) { buf_.set_underflow_handler(std::move(f)); }
    void set_overflow_handler(overflow_handler&& f) { buf_.set_overflow_handler(std::move(f)); }

private:
    memory_view_streambuf buf_;
};


#pragma region impl
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

// called when read buffer is exhausted
inline int
memory_view_streambuf::underflow()
{
    char* head = data_;
    char* tail = head + size_;
    char* current = this->gptr();
    if (current < tail)
    {
        // buffer is not exhausted yet actually. just update position.
        int ret = *current++;
        this->setg(current, current, tail);
        return ret;
    }
    else
    {
        return traits_type::eof();
    }
}

// called when write buffer is exhausted
inline int
memory_view_streambuf::overflow(int c)
{
    char* head = data_;
    char* tail = head + size_;
    char* current = this->gptr();
    if (current < tail)
    {
        // buffer is not exhausted yet actually. just update position.
        *current++ = (char)c;
        this->setp(current, current, tail);
    }
    else
    {
        // call overflow handler. update buffer and position if handled properly.
        if (on_overflow_ && on_overflow_(data_, size_, current)) {
            *current++ = (char)c;
            this->setp(current, current, data_ + size_);
        }
        else {
            return traits_type::eof();
        }
    }
    return c;
}

// implementation of read()
inline std::streamsize
memory_view_streambuf::xsgetn(char* ptr, std::streamsize count)
{
    char* head = data_;
    char* tail = head + size_;
    char* current = this->gptr();
    auto remaining = std::streamsize(tail - current);
    count = std::min(count, remaining);
    std::memcpy(ptr, current, count);

    current += count;
    this->setg(current, current, tail);
    return count;
}

// implementation of write()
inline std::streamsize
memory_view_streambuf::xsputn(const char* ptr, std::streamsize count)
{
    char* head = data_;
    char* tail = head + size_;
    char* current = this->pptr();
    if (current + count > tail)
    {
        if (on_overflow_ && on_overflow_(data_, size_, current)) {
            this->setp(current, current, data_ + size_);
        }
    }
    std::memcpy(current, ptr, count);

    current += count;
    this->setp(current, tail);
    return count;
}


memory_view_stream::memory_view_stream()
    : super(&buf_)
{
}

memory_view_stream::memory_view_stream(memory_view_stream&& v) noexcept
    : super(&buf_)
{
}

memory_view_stream& memory_view_stream::operator=(memory_view_stream&& v) noexcept
{
    swap(v);
    return *this;
}

memory_view_stream::memory_view_stream(const void* data, size_t size)
    : super(&buf_)
{
    reset(data, size);
}
#pragma endregion impl

} // namespace ist
