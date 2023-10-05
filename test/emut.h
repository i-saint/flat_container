#pragma once
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/val.h>
#include <emscripten/bind.h>

namespace emut {

using emscripten::val;

template <typename T> constexpr bool is_std_string = false;
template <typename... Args> constexpr bool is_std_string<std::basic_string<Args...>> = true;

template <typename T> constexpr bool is_std_unique_ptr = false;
template <typename... Args> constexpr bool is_std_unique_ptr<std::unique_ptr<Args...>> = true;

template <typename T> constexpr bool is_std_shared_ptr = false;
template <typename... Args> constexpr bool is_std_shared_ptr<std::shared_ptr<Args...>> = true;

template <typename T, typename = void>
constexpr bool is_ranged = false;
template <typename T>
constexpr bool is_ranged<T, std::void_t<decltype(std::begin(std::declval<T&>())), decltype(std::end(std::declval<T&>()))>> = true;


template<typename T>
static inline val raw_ptr(const T* p)
{
    using namespace emscripten;
    return p ?
        val::take_ownership(internal::_emval_take_value(internal::TypeID<T>::get(), &p)) :
        val::null();
}
template<typename T>
static inline val raw_ptr(const std::shared_ptr<T>& p)
{
    return raw_ptr(p.get());
}
template<typename T>
static inline val raw_ptr(const std::unique_ptr<T>& p)
{
    return raw_ptr(p.get());
}

template<class T>
static inline val make_val(const T& v)
{
    if constexpr (std::is_arithmetic_v<T>) {
        if constexpr (std::is_integral_v<T> && sizeof(T) >= 8) {
            // 64 bit int require explicit conversion
            return val((double)v);
        }
        else {
            return val(v);
        }
    }
    else if constexpr (is_std_string<T>) {
        return val(v);
    }
    else if constexpr (is_std_unique_ptr<T> || is_std_shared_ptr<T> || std::is_pointer_v<T>) {
        return raw_ptr(v);
    }
    else {
        return raw_ptr(&v);
    }
}

class Iterator;
using IteratorPtr = std::shared_ptr<Iterator>;
class Iterable;
using IterablePtr = std::shared_ptr<Iterable>;

class Iterator
{
    friend class Iterable;
public:
    template<typename... Args>
    static val create(Args&&... args)
    {
        auto* r = new Iterator(std::forward<Args>(args)...);
        r->on_done_ = [r]() { delete r; };
        return raw_ptr(r);
    }

    val next()
    {
        return next_();
    }

private:
    template<class Iter>
    Iterator(Iter begin, Iter end)
    {
        static_assert(sizeof(Iter) <= sizeof(void*)); // とりえあえずポインタと同サイズのみ受け付ける
        static_assert(std::is_trivially_destructible_v<Iter>); // デストラクタの面倒は見ない

        Iter* holder = new (buf_) Iter[2]{ begin, end };
        current_ = val::object();
        current_.set("done", false);

        next_ = [this, holder]() {
            Iter& p = holder[0];
            Iter& end = holder[1];
            if (p != end) {
                current_.set("value", make_val(*p++));
                return current_;
            }
            else {
                val tmp = std::move(current_);
                tmp.set("done", true);
                on_done_();
                return tmp;
            }
        };
    }

    template<class T, std::enable_if_t<is_ranged<T>, int> = 0>
    Iterator(const T& container)
        : Iterator(std::begin(container), std::end(container))
    {
    }

private:
    std::function<val()> next_;
    std::function<void()> on_done_;
    char buf_[sizeof(void*) * 2];
    val current_;
};

class Iterable
{
public:
    template<typename... Args>
    static val create(Args&&... args)
    {
        auto* r = new Iterable(std::forward<Args>(args)...);
        r->iter_.on_done_ = [r]() { delete r; };
        return raw_ptr(r);
    }

    val iterator()
    {
        return raw_ptr(&iter_);
    }

private:
    template<typename... Args>
    Iterable(Args&&... args)
        : iter_(std::forward<Args>(args)...)
    {
    }

    Iterator iter_;
};

} // namespace emut
#endif // __EMSCRIPTEN__
