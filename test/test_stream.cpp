#include "Test.h"
#include "flat_container/flat_set.h"
#include "flat_container/flat_map.h"
#include "flat_container/raw_vector.h"
#include "flat_container/vector.h"
#include "flat_container/string.h"
#include "flat_container/memory_view_stream.h"
#include <set>
#include <map>
#include <memory>
#include "emut.h"


testCase(test_memory_view_stream)
{
    ist::vector<char> cont;
    ist::memory_view_stream stream(cont.data(), cont.size());

    stream.set_overflow_handler([&cont](char*& data, size_t& size) -> bool {
        size_t new_size = std::max<size_t>(32, size * 2);
        cont.resize(new_size);
        data = cont.data();
        size = new_size;
        printf("! overflow handler !\n");
        return true;
        });

    for (size_t i = 0; i < 32; ++i) {
        uint64_t t = i;
        stream.write((char*)&t, sizeof(t));
    }
    printf("%zu\n", cont.size());


    stream.reset(cont.data(), cont.size());
    stream.set_underflow_handler([&cont](char*& data, size_t& size, char*& pos) -> bool {
        pos = cont.data();
        printf("! underflow handler !\n");
        return true;
        });

    for (size_t i = 0; i < 128; ++i) {
        uint64_t t;
        stream.read((char*) & t, sizeof(t));
        printf("%llu\n", t);
    }
}
