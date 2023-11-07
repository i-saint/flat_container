#include "Test.h"
#include "flat_container/flat_set.h"
#include "flat_container/flat_map.h"
#include "flat_container/raw_vector.h"
#include "flat_container/vector.h"
#include "flat_container/string.h"
#include <set>
#include <map>
#include <memory>
#include <unordered_map>
#include <random>


#if defined(_M_IX86) || defined(__i386__)
#   define fc_x84
#elif defined(_M_X64) || defined(__x86_64__)
#   define fc_x84
#   define fc_x84_64
#endif

#ifdef fc_x84
#   include <nmmintrin.h>
#endif

#ifdef __clang__
#   define fc_no_sanitize_address __attribute__((no_sanitize_address))
#elif _MSC_VER
#   define fc_no_sanitize_address __declspec(no_sanitize_address)
#endif


template<class T>
class counted : public T
{
public:
    using super = T;
    static const int& instance_count() { return instance_count_; }

    counted()
    {
        ++instance_count_;
    }
    counted(const counted& r)
        : super(r)
    {
        ++instance_count_;
    }
    counted(counted&& r) noexcept
        : super(std::move(r))
    {
        ++instance_count_;
    }
    template<class... Args>
    counted(Args&&... args)
        : super(std::forward<Args>(args)...)
    {
        ++instance_count_;
    }

    ~counted()
    {
        --instance_count_;
    }

    counted& operator=(const counted& r)
    {
        super::operator=(r);
        return *this;
    }
    counted& operator=(counted&& r) noexcept
    {
        super::operator=(std::move(r));
        return *this;
    }
    template<class... Args>
    counted& operator=(Args&&... args)
    {
        super::operator=(std::forward<Args>(args)...);
        return *this;
    }

protected:
    static int instance_count_;
};

template<class T>
int counted<T>::instance_count_ = 0;

using test::Timer;
using string = std::string;
using cstring = counted<string>;

template <class T>
constexpr bool is_std_set_v = false;
template <class... Args>
constexpr bool is_std_set_v<std::set<Args...>> = true;

template <class T>
constexpr bool is_std_map_v = false;
template <class... Args>
constexpr bool is_std_map_v<std::map<Args...>> = true;


testCase(test_fixed_vector)
{
    static_assert( ist::has_dynamic_memory_v<ist::vector<int>>);
    static_assert(!ist::has_dynamic_memory_v<ist::fixed_vector<int, 8>>);
    static_assert( ist::has_dynamic_memory_v<ist::small_vector<int, 8>>);
    static_assert(!ist::has_dynamic_memory_v<ist::remote_vector<int>>);
    static_assert( ist::has_dynamic_memory_v<ist::shared_vector<int>>);

    static_assert(!ist::has_fixed_memory_v<ist::vector<int>>);
    static_assert( ist::has_fixed_memory_v<ist::fixed_vector<int, 8>>);
    static_assert( ist::has_fixed_memory_v<ist::small_vector<int, 8>>);
    static_assert(!ist::has_fixed_memory_v<ist::remote_vector<int>>);
    static_assert(!ist::has_fixed_memory_v<ist::shared_vector<int>>);

    static_assert(!ist::has_remote_memory_v<ist::vector<int>>);
    static_assert(!ist::has_remote_memory_v<ist::fixed_vector<int, 8>>);
    static_assert(!ist::has_remote_memory_v<ist::small_vector<int, 8>>);
    static_assert( ist::has_remote_memory_v<ist::remote_vector<int>>);
    static_assert(!ist::has_remote_memory_v<ist::shared_vector<int>>);

    static_assert(!ist::has_shared_memory_v<ist::vector<int>>);
    static_assert(!ist::has_shared_memory_v<ist::fixed_vector<int, 8>>);
    static_assert(!ist::has_shared_memory_v<ist::small_vector<int, 8>>);
    static_assert(!ist::has_shared_memory_v<ist::remote_vector<int>>);
    static_assert( ist::has_shared_memory_v<ist::shared_vector<int>>);

    auto& cs_count = cstring::instance_count();
    testExpect(cs_count == 0);
    {
        // basic tests
        ist::fixed_vector<cstring, 128> data, data2, data3;
        ist::small_vector<cstring, 32> sdata;
        ist::vector<cstring> ddata;

        std::byte buf[sizeof(cstring) * 128];
        ist::remote_vector<cstring> vdata(buf, 128);

        auto make_data = [](auto& dst) {
            cstring tmp;
            for (int i = 0; i < 64; ++i) {
                tmp += ' ' + char(i);

                switch (i % 8) {
                case 0: dst.push_back(tmp); break;
                case 1: {
                    auto t = tmp;
                    dst.push_back(std::move(t));
                    break;
                }
                case 2: dst.emplace_back(tmp.c_str(), tmp.size()); break;
                case 3: dst.resize(dst.size() + 1, tmp); break;
                case 4: dst.insert(dst.begin(), tmp); break;
                case 5: dst.insert(dst.begin(), &tmp, &tmp + 1); break;
                case 6: dst.insert(dst.begin(), std::initializer_list<cstring>{tmp}); break;
                case 7: dst.erase(dst.begin() + dst.size() / 2); break;
                case 8: dst.emplace(dst.begin() + dst.size() / 2, tmp.c_str(), tmp.size()); break;
                }
            }
        };
        make_data(data);
        make_data(sdata);
        make_data(ddata);
        make_data(vdata);

        testExpect(data == sdata);
        testExpect(!(data != sdata));
        testExpect(!(data < sdata));
        testExpect(!(data > sdata));
        testExpect(data <= sdata);
        testExpect(data >= sdata);

        testExpect(data == ddata);
        testExpect(!(data != ddata));
        testExpect(!(data < ddata));
        testExpect(!(data > ddata));
        testExpect(data <= ddata);
        testExpect(data >= ddata);

        testExpect(data == vdata);
        testExpect(!(data != vdata));
        testExpect(!(data < vdata));
        testExpect(!(data > vdata));
        testExpect(data <= vdata);
        testExpect(data >= vdata);

        data.erase(data.begin() + 32, data.begin() + 40);
        testExpect(data.size() == 40);

        data2 = data;
        for (size_t i = 0; i < data.size(); ++i) {
            testExpect(data[i] == data2[i]);
        }

        data3 = std::move(data2);
        for (size_t i = 0; i < data.size(); ++i) {
            testExpect(data[i] == data3[i]);
        }

        data2.assign(data3.begin(), data3.end());
        for (size_t i = 0; i < data.size(); ++i) {
            testExpect(data2[i] == data3[i]);
        }

        data3.assign(64, data2[0]);
        for (auto& v : data3) {
            testExpect(v == data2[0]);
        }
    }
    testExpect(cs_count == 0);

    {
        // with non-copyable element
        class elem
        {
        public:
            elem(const cstring& v) : value(v) {}
            elem(const char* str, size_t len) : value(str, len) {}
            elem() = delete;
            elem(const elem&) = delete;
            elem(elem&&) = default;
            elem& operator=(const elem&) = delete;
            elem& operator=(elem&&) = default;
            bool operator==(const elem& v) const { return value == v.value; }

            cstring value;
        };

        ist::fixed_vector<elem, 128> data, data2, data3, data4;
        //std::vector<elem> data, data2, data3;

        string tmp;
        for (int i = 0; i < 64; ++i) {
            tmp += ' ' + char(i);

            switch (i % 4) {
            case 0: data.push_back(elem(tmp)); break;
            case 1: data.emplace_back(tmp.c_str(), tmp.size()); break;
            case 2: data.insert(data.begin(), elem(tmp)); break;
            case 3: data.emplace(data.begin(), tmp.c_str(), tmp.size()); break;
            }
        }

        data.erase(data.begin() + 32, data.begin() + 40);
        testExpect(data.size() == 56);

        for (size_t i = 0; i < data.size(); ++i) {
            data2.push_back(elem(data[i].value));
        }
        for (size_t i = 0; i < data.size(); ++i) {
            testExpect(data[i] == data2[i]);
        }

        data3 = std::move(data2);
        for (size_t i = 0; i < data.size(); ++i) {
            testExpect(data[i] == data3[i]);
        }

        data4.push_back(elem("abc"));
        data4.emplace_back("def");
        std::swap(data4, data3);
        testExpect(data3.size() == 2);
        for (size_t i = 0; i < data.size(); ++i) {
            testExpect(data[i] == data4[i]);
        }
    }
    testExpect(cs_count == 0);
}

testCase(test_fixed_raw_vector)
{
    // causes static assertion failure
    //ist::fixed_raw_vector<string, 128> hoge;

    {
        // basic tests
        ist::fixed_raw_vector<int, 128> data, data2, data3;
        ist::small_raw_vector<int, 32> sdata;
        ist::raw_vector<int> ddata;

        std::byte buf1[sizeof(int) * 128];
        ist::remote_raw_vector<int> vdata(buf1, 128);

        auto make_data = [](auto& dst){
            int tmp = 0;
            for (int i = 0; i < 64; ++i) {
                tmp += i;

                switch (i % 9) {
                case 0: dst.push_back(tmp); break;
                case 1: {
                    auto t = tmp;
                    dst.push_back(std::move(t));
                    break;
                }
                case 2: dst.emplace_back(tmp); break;
                case 3: dst.resize(dst.size() + 1, tmp); break;
                case 4: dst.insert(dst.begin(), tmp); break;
                case 5: dst.insert(dst.begin(), &tmp, &tmp + 1); break;
                case 6: dst.insert(dst.begin(), std::initializer_list<int>{tmp}); break;
                case 7: dst.erase(dst.begin() + dst.size() / 2); break;
                case 8: dst.emplace(dst.begin() + dst.size() / 2, tmp); break;
                }
            }
        };
        make_data(data);
        make_data(sdata);
        make_data(ddata);
        make_data(vdata);

        testExpect(data == sdata);
        testExpect(!(data != sdata));
        testExpect(!(data < sdata));
        testExpect(!(data > sdata));
        testExpect(data <= sdata);
        testExpect(data >= sdata);

        testExpect(data == ddata);
        testExpect(!(data != ddata));
        testExpect(!(data < ddata));
        testExpect(!(data > ddata));
        testExpect(data <= ddata);
        testExpect(data >= ddata);

        testExpect(data == vdata);
        testExpect(!(data != vdata));
        testExpect(!(data < vdata));
        testExpect(!(data > vdata));
        testExpect(data <= vdata);
        testExpect(data >= vdata);

        data.erase(data.begin() + 32, data.begin() + 40);
        testExpect(data.size() == 42);

        data2 = data;
        for (size_t i = 0; i < data.size(); ++i) {
            testExpect(data[i] == data2[i]);
        }

        data3 = std::move(data2);
        for (size_t i = 0; i < data.size(); ++i) {
            testExpect(data[i] == data3[i]);
        }

        data2.assign(data3.begin(), data3.end());
        for (size_t i = 0; i < data.size(); ++i) {
            testExpect(data2[i] == data3[i]);
        }

        data3.assign(64, data2[0]);
        for (auto& v : data3) {
            testExpect(v == data2[0]);
        }

        //view = view2;
    }

}


testCase(test_constant_iterator)
{
    {
        using value_t = std::iterator_traits<ist::constant_iterator<string>>::value_type;
        printf("%d\n", std::is_same_v<value_t, string> ? 1 : 0);

        string data = "abcdefg";

        auto first = ist::make_constant_iterator(data);
        auto last = first + 4;

        for (; first != last; ++first) {
            printf("%s\n", first->c_str());
        }
    }
}


testCase(test_fixed_string)
{
    {
        ist::fixed_string<32> empty;
        ist::string sint = "  666   ";
        ist::string fint = "    666.666    ";

        printf("stoi: %d\n", std::stoi(sint));
        printf("stof: %f\n", std::stof(fint));
        printf("stod: %lf\n", std::stod(fint));

        std::unordered_map<ist::string, int> hoge{{"a", 999}};
        printf("std::unordered_map<ist::string, int>: %d\n", hoge["a"]);
    }
    {
        ist::fixed_string<32> abc = "12345";
        ist::fixed_string<32> def = "67890";

#define check(exp) testExpect(exp); printf(#exp ": %d\n", (int)(exp))
        check(!(abc == def));
        check( (abc != def));
        check( (abc <  def));
        check( (abc <= def));
        check(!(abc >  def));
        check(!(abc >= def));

        check(!(abc == def.data()));
        check( (abc != def.data()));
        check( (abc <  def.data()));
        check( (abc <= def.data()));
        check(!(abc >  def.data()));
        check(!(abc >= def.data()));

        check(!(abc.data() == def));
        check( (abc.data() != def));
        check( (abc.data() <  def));
        check( (abc.data() <= def));
        check(!(abc.data() >  def));
        check(!(abc.data() >= def));
#undef check

        testExpect(abc.starts_with('1'));
        testExpect(abc.starts_with("123"));
        testExpect(abc.starts_with(std::string_view("123")));
        testExpect(abc.ends_with('5'));
        testExpect(abc.ends_with("345"));
        testExpect(abc.ends_with(std::string_view("345")));

        abc += '?';
        testExpect(abc == "12345?");

        abc += "hoge";
        testExpect(abc == "12345?hoge");

        abc += {'a', 'b', 'c'};
        testExpect(abc == "12345?hogeabc");

        abc += std::string_view("def");
        testExpect(abc == "12345?hogeabcdef");

        auto check_pos = [&](size_t pos, char v) {
            return abc[pos] == v;
        };
        auto check_npos = [&](size_t pos) {
            return pos == ist::string::npos;
        };
        testExpect(check_pos(abc.find("345"), '3'));
        testExpect(check_npos(abc.find("678")));
        testExpect(check_pos(abc.find_first_of("abcdef?"), '?'));
        testExpect(check_npos(abc.find_first_of("xyz")));
        testExpect(check_pos(abc.find_first_not_of("12345"), '?'));
        testExpect(check_npos(abc.find_first_not_of("12345?hogeabcdef")));
        testExpect(check_pos(abc.find_last_of("?12345"), '?'));
        testExpect(check_npos(abc.find_last_of("xyz")));
        testExpect(check_pos(abc.find_last_not_of("hogeabcdef"), '?'));
        testExpect(check_npos(abc.find_last_not_of("12345?hogeabcdef")));

        std::string sabc = std::string(abc);
        testExpect(check_pos(sabc.find("345"), '3'));
        testExpect(check_npos(sabc.find("678")));
        testExpect(check_pos(sabc.find_first_of("abcdef?"), '?'));
        testExpect(check_npos(sabc.find_first_of("xyz")));
        testExpect(check_pos(sabc.find_first_not_of("12345"), '?'));
        testExpect(check_npos(sabc.find_first_not_of("12345?hogeabcdef")));
        testExpect(check_pos(sabc.find_last_of("?12345"), '?'));
        testExpect(check_npos(sabc.find_last_of("xyz")));
        testExpect(check_pos(sabc.find_last_not_of("hogeabcdef"), '?'));
        testExpect(check_npos(sabc.find_last_not_of("12345?hogeabcdef")));


        abc.replace(abc.begin() + 3, abc.begin() + 5, "6789");
        testExpect(abc == "1236789?hogeabcdef");

        abc.replace(abc.find("?"), 11, std::string_view(""));
        testExpect(abc == "1236789");

        abc = abc + 'a';
        abc = std::move(abc) + 'b';
        abc = abc + "cd";
        abc = std::move(abc) + "ef";
        abc = abc + std::string_view("gh");
        abc = std::move(abc) + std::string_view("ij");
        abc = 'z' + abc;
        abc = "xy" + abc;
        abc = std::string_view("vw") + abc;
        testExpect(abc == "vwxyz1236789abcdefghij");

        auto xyz = abc.substr(2, 3);
        testExpect(xyz == "xyz");
    }
}

testCase(test_small_vector)
{
    auto& cs_count = cstring::instance_count();
    testExpect(cs_count == 0);

    using vec_t = ist::small_vector<cstring, 4>;
    {
        vec_t vec1{ "a", "b", "c", "d" };
        vec_t vec2{ "0", "1", "2", "3" };
        testExpect(cs_count == 8);

        testExpect(vec2.capacity() == 4);
        vec2.emplace_back("4");
        testExpect(vec2.capacity() == 8);
        testExpect(cs_count == 9);

        vec_t vec3 = vec2;
        testExpect(cs_count == 14);
        vec_t vec4 = std::move(vec2);
        testExpect(vec3 == vec4);

        vec_t vec5 = vec1;
        vec1.swap(vec4);
        testExpect(vec1 == vec3);
        testExpect(vec4 == vec5);
    }
    testExpect(cs_count == 0);
}


testCase(test_shared_vector)
{
    auto& cs_count = cstring::instance_count();
    testExpect(cs_count == 0);

    using vec_t = ist::shared_vector<cstring>;
    {
        vec_t vec1{ "a", "b", "c", "d" };
        vec_t vec2{ "0", "1", "2", "3" };
        testExpect(cs_count == 8);

        testExpect(vec2.capacity() == 4);
        vec2.emplace_back("4");
        testExpect(vec2.capacity() == 8);
        testExpect(cs_count == 9);

        vec_t vec3 = vec2;
        vec_t vec4 = std::move(vec2);
        testExpect(vec3 == vec4);
        testExpect(cs_count == 9);

        vec_t vec5 = vec1;
        vec1.swap(vec4);
        testExpect(vec1 == vec3);
        testExpect(vec4 == vec5);
    }
    testExpect(cs_count == 0);
}


testCase(test_flat_set)
{
    auto& cs_count = cstring::instance_count();
    testExpect(cs_count == 0);
    {
        std::set<cstring, std::less<>> std_set;
        ist::flat_set<cstring> flat_set;
        ist::fixed_set<cstring, 32> fixed_set;
        ist::small_set<cstring, 8> small_set;

        char buf[sizeof(cstring) * 32];
        ist::remote_set<cstring> remote_set(buf, 32);
        ist::shared_set<cstring> shared_set;

        auto op = [&](auto&& func) {
            func(std_set);
            func(flat_set);
            func(fixed_set);
            func(small_set);
            func(remote_set);
            func(shared_set);
        };
        auto cmp = [&](auto&& func) {
            func(std_set, flat_set);
            func(std_set, fixed_set);
            func(std_set, small_set);
            func(std_set, remote_set);
            func(std_set, shared_set);
        };

        auto check = [&]() {
            cmp([&](auto& cont1, auto& cont2) {
                testExpect(cont1.size() == cont2.size());

                auto i1 = cont1.begin();
                auto i2 = cont2.begin();
                while (i1 != cont1.end()) {
                    testExpect(*i1 == *i2);
                    ++i1; ++i2;
                }
                });
        };

        string data[]{ "e", "a", "e", "b", "c", "d", "c", "b", "d", "a", "x", "z" };
        op([&](auto& cont) {
            for (auto& v : data) {
                cont.insert(v);
            }
            cont.insert({ "abc", "def", "ghi", "jkl" });
            cont.emplace("123456");

            cstring k = "123456";
            cont.emplace_hint(cont.cend(), (const cstring&)k);
            cont.emplace_hint(cont.cend(), std::move(k));
            });
        check();

        testExpect(flat_set == fixed_set);
        testExpect(!(flat_set != fixed_set));
        testExpect(!(flat_set < fixed_set));
        testExpect(!(flat_set > fixed_set));
        testExpect(flat_set <= fixed_set);
        testExpect(flat_set >= fixed_set);

        testExpect(flat_set == small_set);
        testExpect(!(flat_set != small_set));
        testExpect(!(flat_set < small_set));
        testExpect(!(flat_set > small_set));
        testExpect(flat_set <= small_set);
        testExpect(flat_set >= small_set);

        op([](auto& cont) {
            constexpr bool is_std = is_std_set_v<ist::remove_cvref_t<decltype(cont)>>;

            testExpect(*cont.find(std::string_view("a")) == "a");
            testExpect(*cont.lower_bound(std::string_view("x")) == "x");
            testExpect(*cont.lower_bound(std::string_view("y")) == "z");
            testExpect(*cont.upper_bound(std::string_view("x")) == "z");
            testExpect(*cont.upper_bound(std::string_view("y")) == "z");
            testExpect(cont.count("a") == 1);
            if constexpr (!is_std) {
                // std::set::contains() requires C++20
                testExpect(cont.contains("a"));
                testExpect(!cont.contains("y"));
            }

            using const_t = std::add_const_t<decltype(cont)>;
            auto& ccont = const_cast<const_t&>(cont);
            testExpect(*ccont.find(std::string_view("a")) == "a");
            testExpect(*ccont.lower_bound(std::string_view("x")) == "x");
            testExpect(*ccont.upper_bound(std::string_view("x")) == "z");
            testExpect(ccont.count("a") == 1);
            if constexpr (!is_std) {
                testExpect(ccont.contains("a"));
                testExpect(!ccont.contains("y"));
            }

            });

        op([](auto& set) {
            set.erase("c");
            set.erase("a");
            set.erase("x");
            });

        check();
    }
    testExpect(cs_count == 0);
}


testCase(test_flat_map)
{
    auto& cs_count = cstring::instance_count();
    testExpect(cs_count == 0);
    {
        std::map<cstring, int, std::less<>> std_map;
        ist::flat_map<cstring, int> flat_map;
        ist::fixed_map<cstring, int, 32> fixed_map;
        ist::small_map<cstring, int, 8> small_map;

        std::byte buf[sizeof(std::pair<cstring, int>) * 32];
        ist::remote_map<cstring, int> remote_map(buf, 32);
        ist::shared_map<cstring, int> shared_map;

        auto op = [&](auto&& func) {
            func(std_map);
            func(flat_map);
            func(fixed_map);
            func(small_map);
            func(remote_map);
            func(shared_map);
        };
        auto cmp = [&](auto&& func) {
            func(std_map, flat_map);
            func(std_map, fixed_map);
            func(std_map, small_map);
            func(std_map, remote_map);
            func(std_map, shared_map);
        };

        auto check = [&]() {
            cmp([&](auto& cont1, auto& cont2) {
                testExpect(cont1.size() == cont2.size());

                auto i1 = cont1.begin();
                auto i2 = cont2.begin();
                while (i1 != cont1.end()) {
                    testExpect(i1->first == i2->first);
                    testExpect(i1->second == i2->second);
                    ++i1; ++i2;
                }
                });
        };


        std::pair<string, int> data[]{
            {"a", 10},
            {"c", 3},
            {"e", 50},
            {"d", 4},
            {"b", 20},
            {"b", 2},
            {"d", 40},
            {"e", 5},
            {"c", 30},
            {"a", 1},
            {"x", 99},
            {"z", 999},
        };
        op([&](auto& cont) {
            for (auto& v : data) {
                cont.insert(v);
            }
            cont.insert({ {"abc", 100}, {"def", 200}, {"ghi", 300}, {"jkl", 400} });
            cont.emplace("123456", 123456);
            cont.emplace_hint(cont.cend(), "123456", 123456);
            cont.try_emplace("abcdefg", 123456);

            cstring k = "abcdefg";
            cont.try_emplace(cont.cend(), (const cstring&)k, 123456);
            cont.try_emplace(cont.cend(), std::move(k), 123456);

            });
        check();

        testExpect(flat_map == fixed_map);
        testExpect(!(flat_map != fixed_map));
        testExpect(!(flat_map < fixed_map));
        testExpect(!(flat_map > fixed_map));
        testExpect(flat_map <= fixed_map);
        testExpect(flat_map >= fixed_map);

        testExpect(flat_map == small_map);
        testExpect(!(flat_map != small_map));
        testExpect(!(flat_map < small_map));
        testExpect(!(flat_map > small_map));
        testExpect(flat_map <= small_map);
        testExpect(flat_map >= small_map);


        op([&](auto& cont) {
            constexpr bool is_std = is_std_map_v<ist::remove_cvref_t<decltype(cont)>>;

            std::string a = "a";
            testExpect(cont[a] == 10);
            testExpect(cont["a"] == 10);
            testExpect(cont.find(std::string_view("a"))->second == 10);
            testExpect(cont.lower_bound(std::string_view("x"))->second == 99);
            testExpect(cont.lower_bound(std::string_view("y"))->second == 999);
            testExpect(cont.upper_bound(std::string_view("x"))->second == 999);
            testExpect(cont.upper_bound(std::string_view("y"))->second == 999);
            testExpect(cont.count("a") == 1);
            if constexpr (!is_std) {
                // std::map::contains() requires C++20
                testExpect(cont.contains("a"));
                testExpect(!cont.contains("y"));
            }

            using const_t = std::add_const_t<decltype(cont)>;
            auto& ccont = const_cast<const_t&>(cont);
            testExpect(ccont.find(std::string_view("a"))->second == 10);
            testExpect(ccont.lower_bound(std::string_view("x"))->second == 99);
            testExpect(ccont.upper_bound(std::string_view("x"))->second == 999);
            testExpect(ccont.count("a") == 1);
            if constexpr (!is_std) {
                testExpect(ccont.contains("a"));
                testExpect(!ccont.contains("y"));
            }

            });

        op([&](auto& cont) {
            cont.erase("c");
            cont.erase("a");
            cont.erase("x");
            });
        check();
    }
    testExpect(cs_count == 0);
}
