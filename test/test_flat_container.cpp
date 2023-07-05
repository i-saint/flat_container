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

#ifdef _WIN32
#   include <nmmintrin.h>
#endif

using test::Timer;
using string = ist::string;

testCase(test_flat_set)
{
    std::set<string> sset;
    ist::flat_set<string> fset;
    ist::fixed_set<string, 32> xset;
    ist::sbo_set<string, 8> bset;

    std::byte buf[sizeof(string) * 32];
    ist::mapped_set<string> vset(buf, 32);

    auto check = [&]() {
        testExpect(sset.size() == fset.size());
        testExpect(sset.size() == xset.size());
        testExpect(sset.size() == bset.size());
        testExpect(sset.size() == vset.size());

        auto i1 = sset.begin();
        auto i2 = fset.begin();
        auto i3 = xset.begin();
        auto i4 = bset.begin();
        auto i5 = vset.begin();
        while (i1 != sset.end()) {
            testExpect(*i1 == *i2);
            testExpect(*i1 == *i3);
            testExpect(*i1 == *i4);
            testExpect(*i1 == *i5);
            ++i1; ++i2; ++i3; ++i4; ++i5;
        }
    };
    auto insert = [&](const string& v) {
        sset.insert(v);
        fset.insert(v);
        xset.insert(v);
        bset.insert(v);
        vset.insert(v);
    };
    auto insert_il = [&](std::initializer_list<string>&& v) {
        sset.insert(v);
        fset.insert(v);
        xset.insert(v);
        bset.insert(v);
        vset.insert(v);
    };
    auto erase = [&](const string& v) {
        sset.erase(v);
        fset.erase(v);
        xset.erase(v);
        bset.erase(v);
        vset.erase(v);
    };

    string data[]{ "e", "a", "e", "b", "c", "d", "c", "b", "d", "a", };
    for (auto& v : data) {
        insert(v);
    }
    insert_il({ "abc", "def", "ghi", "jkl" });
    check();

    testExpect(fset == xset);
    testExpect(!(fset != xset));
    testExpect(!(fset < xset));
    testExpect(!(fset > xset));
    testExpect(fset <= xset);
    testExpect(fset >= xset);

    testExpect(fset == bset);
    testExpect(!(fset != bset));
    testExpect(!(fset < bset));
    testExpect(!(fset > bset));
    testExpect(fset <= bset);
    testExpect(fset >= bset);

    erase("c");
    erase("a");
    erase("x");
    check();
}


testCase(test_flat_map)
{
    std::map<string, int> smap;
    ist::flat_map<string, int> fmap;
    ist::fixed_map<string, int, 32> xmap;
    ist::sbo_map<string, int, 8> bmap;

    std::byte buf[sizeof(std::pair<string, int>) * 32];
    ist::mapped_map<string, int> vmap(buf, 32);

    auto check = [&]() {
        testExpect(smap.size() == fmap.size());
        testExpect(smap.size() == xmap.size());
        testExpect(smap.size() == bmap.size());
        testExpect(smap.size() == vmap.size());

        auto i1 = smap.begin();
        auto i2 = fmap.begin();
        auto i3 = xmap.begin();
        auto i4 = bmap.begin();
        auto i5 = vmap.begin();
        while (i1 != smap.end()) {
            testExpect(i1->first == i2->first); testExpect(i1->second == i2->second);
            testExpect(i1->first == i3->first); testExpect(i1->second == i3->second);
            testExpect(i1->first == i4->first); testExpect(i1->second == i4->second);
            testExpect(i1->first == i5->first); testExpect(i1->second == i5->second);
            ++i1; ++i2; ++i3; ++i4; ++i5;
        }
    };
    auto insert = [&](const std::pair<string, int>& v) {
        smap.insert(v);
        fmap.insert(v);
        xmap.insert(v);
        bmap.insert(v);
        vmap.insert(v);
    };
    auto insert_il = [&](std::initializer_list<std::pair<const string, int>>&& v) {
        smap.insert(v);
        fmap.insert(v);
        xmap.insert(v);
        bmap.insert(v);
        vmap.insert(v);
    };
    auto erase = [&](const string& v) {
        smap.erase(v);
        fmap.erase(v);
        xmap.erase(v);
        bmap.erase(v);
        vmap.erase(v);
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
    };

    for (auto& v : data) {
        insert(v);
    }
    insert_il({ {"abc", 100}, {"def", 200}, {"ghi", 300}, {"jkl", 400} });
    check();

    testExpect(fmap == xmap);
    testExpect(!(fmap != xmap));
    testExpect(!(fmap < xmap));
    testExpect(!(fmap > xmap));
    testExpect(fmap <= xmap);
    testExpect(fmap >= xmap);

    testExpect(fmap == bmap);
    testExpect(!(fmap != bmap));
    testExpect(!(fmap < bmap));
    testExpect(!(fmap > bmap));
    testExpect(fmap <= bmap);
    testExpect(fmap >= bmap);

    erase("c");
    erase("a");
    erase("x");
    check();
}


testCase(test_fixed_vector)
{
    printf("is_memory_view_v<ist::fixed_vector<int, 8>>: %d\n",
        (int)ist::is_memory_view_v<ist::fixed_vector<int, 8>>);
    printf("is_memory_view_v<ist::mapped_vector<int>>: %d\n",
        (int)ist::is_memory_view_v<ist::mapped_vector<int>>);

    {
        // basic tests
        ist::fixed_vector<string, 128> data, data2, data3;
        ist::sbo_vector<string, 32> sdata;
        ist::vector<string> ddata;

        std::byte buf[sizeof(string) * 128];
        ist::mapped_vector<string> vdata(buf, 128);

        auto make_data = [](auto& dst) {
            string tmp;
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
                case 6: dst.insert(dst.begin(), std::initializer_list<string>{tmp}); break;
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

    {
        // with non-copyable element
        class elem
        {
        public:
            elem(const string& v) : value(v) {}
            elem(const char* str, size_t len) : value(str, len) {}
            elem() = delete;
            elem(const elem&) = delete;
            elem(elem&&) = default;
            elem& operator=(const elem&) = delete;
            elem& operator=(elem&&) = default;
            bool operator==(const elem& v) const { return value == v.value; }

            string value;
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
}

testCase(test_fixed_raw_vector)
{
    // causes static assertion failure
    //ist::fixed_raw_vector<string, 128> hoge;

    {
        // basic tests
        ist::fixed_raw_vector<int, 128> data, data2, data3;
        ist::sbo_raw_vector<int, 32> sdata;
        ist::raw_vector<int> ddata;

        std::byte buf1[sizeof(int) * 128];
        ist::mapped_raw_vector<int> vdata(buf1, 128);

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



#ifdef _WIN32

__declspec(noinline)
bool streq_strcmp(const char* a, const char* b, size_t len)
{
    return std::strcmp(a, b) == 0;
}

__declspec(noinline)
bool streq_strncmp(const char* a, const char* b, size_t len)
{
    return std::strncmp(a, b, len) == 0;
}

__declspec(noinline)
bool streq_memcmp(const char* a, const char* b, size_t len)
{
    return memcmp(a, b, len) == 0;
}

// len must be multiples of 8
__declspec(noinline)
bool streq_uint64(const char* _a, const char* _b, size_t len)
{
    auto a = (const uint64_t*)_a;
    auto b = (const uint64_t*)_b;
    size_t n = len / 8;
    for (size_t i = 0; i < n; ++i) {
        if (*a++ != *b++) {
            return false;
        }
    }
    return true;
}

// len must be multiples of 16
__declspec(noinline)
bool streq_sse42(const char* _a, const char* _b, size_t len)
{
    auto a = (const __m128i*)_a;
    auto b = (const __m128i*)_b;
    size_t n = len / 16;
    for (size_t i = 0; i < n; ++i) {
        int r = _mm_cmpestri(
            _mm_loadu_si128(a++), 16,
            _mm_loadu_si128(b++), 16,
            _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ORDERED);
        //printf("%d\n", r);
        if (r != 0) {
            return false;
        }
    }
    return true;
}
#endif

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

        auto pos = abc.find("345");
        testExpect(abc[pos] == '3');

        abc.replace(abc.begin() + 3, abc.begin() + 5, "6789");
        testExpect(abc == "1236789?hogeabcdef");

        abc.replace(abc.find("?"), 11, std::string_view(""));
        testExpect(abc == "1236789");

        pos = abc.find("345");
        testExpect(pos == ~0);

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


#ifdef _WIN32
    const size_t num = 1000000;
    //const size_t num = 500000;
    const size_t len = 128;

    std::vector<std::string> a;
    std::vector<std::string> b;
    a.resize(num);
    b.resize(num);
    for (size_t i = 0; i < num; ++i) {
        a[i].resize(len, 0x40 + (i % 0x40));
        b[i].resize(len, 0x80 - (i % 0x40));
    }

    printf("loop count: %zu\n", num);
    printf("string length: %zu\n", len);
    {
        Timer timer;
        int r = 0;
        for (uint64_t i = 0; i < num; ++i) {
            if (streq_strncmp(a[i].c_str(), b[i].c_str(), len)) {
                ++r;
            }
        }
        printf("streq_strncmp(): %.2lfms %d\n", timer.elapsed_ms(), r);
    }
    {
        Timer timer;
        int r = 0;
        for (uint64_t i = 0; i < num; ++i) {
            if (streq_strcmp(a[i].c_str(), b[i].c_str(), len)) {
                ++r;
            }
        }
        printf("streq_strcmp(): %.2lfms %d\n", timer.elapsed_ms(), r);
    }
    {
        Timer timer;
        int r = 0;
        for (uint64_t i = 0; i < num; ++i) {
            if (streq_memcmp(a[i].c_str(), b[i].c_str(), len)) {
                ++r;
            }
        }
        printf("streq_memcmp(): %.2lfms %d\n", timer.elapsed_ms(), r);
    }
    {
        Timer timer;
        int r = 0;
        for (uint64_t i = 0; i < num; ++i) {
            if (streq_uint64(a[i].c_str(), b[i].c_str(), len)) {
                ++r;
            }
        }
        printf("streq_uint64(): %.2lfms %d\n", timer.elapsed_ms(), r);
    }
    {
        Timer timer;
        int r = 0;
        for (uint64_t i = 0; i < num; ++i) {
            if (streq_sse42(a[i].c_str(), b[i].c_str(), len)) {
                ++r;
            }
        }
        printf("streq_sse42(): %.2lfms %d\n", timer.elapsed_ms(), r);
    }
#endif
}