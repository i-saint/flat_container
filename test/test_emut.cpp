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


#ifdef __EMSCRIPTEN__
using namespace emut;


class TestData
{
public:
    TestData(int v = 0) : data_(v) {}

    void test()
    {
        printf("TestData::test(): %d\n", data_);
    }

    void test2(val arg)
    {
        printf("TestData::test(): %d %lf\n", data_, arg.as<double>());
    }

    val getFunc()
    {
        return make_function([](val arg) {
            printf("lambda(): %lf\n", arg.as<double>());
            });
    }

private:
    int data_ = 0;
};

class TestIterable
{
public:
    val getMembers() const
    {
        return make_iterable(data_);
    }

private:
    TestData data_[8]{ 100, 101, 102, 103, 104, 105, 106, 107 };
};


static void __testfunc(val a) {
    using namespace emscripten;
    TestData* hogep = a.as<TestData*>(allow_raw_pointers());
    hogep->test();
};


EMSCRIPTEN_BINDINGS(test)
{
    using namespace emscripten;
    class_<TestData>("TestData")
        .constructor<int>()
        .function("test", &TestData::test)
        .function("test2", &TestData::test2)
        .function("getFunc", &TestData::getFunc)
        ;
    class_<TestIterable>("TestIterable")
        .constructor()
        .property("members", &TestIterable::getMembers)
        ;
}

testCase(test_ems_iterable)
{
    using namespace emscripten;

    {
        TestData hoge(42);
        val hogev = make_pointer(&hoge);
        TestData* hogep = hogev.as<TestData*>(allow_raw_pointers());
        hogep->test();
    }
    {
        val obj{ TestData(43) };
        obj.call<void>("test");
        TestData* hogep = obj.as<TestData*>(allow_raw_pointers());
        hogep->test();


        obj.set("test2", make_function([](val obj) { __testfunc(obj);  }));
        obj["test2"].call<void>("call", obj);
    }
}

#endif // __EMSCRIPTEN__
