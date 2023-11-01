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
        printf("!overflow handler! %zu -> %zu\n", size, new_size);
        cont.resize(new_size);
        data = cont.data();
        size = new_size;
        return true;
        });

    for (size_t i = 0; i < 32; ++i) {
        uint64_t t = i;
        stream.write((char*)&t, sizeof(t));
    }
    printf("cont.size(): %zu\n", cont.size());


    stream.reset(cont.data(), cont.size());
    stream.set_underflow_handler([&cont](char*& data, size_t& size, char*& pos) -> bool {
        printf("!underflow handler!\n");
        pos = cont.data();
        return true;
        });

    for (size_t i = 0; i < 128; ++i) {
        uint64_t t;
        stream.read((char*) & t, sizeof(t));
        testExpect(t == i % 32);
        printf("%llu ", t);
    }
    printf("\n");

    ist::memory_view_stream stream2;
    std::swap(stream, stream2);
}
