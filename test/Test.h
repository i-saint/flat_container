#pragma once
#include <cstdint>
#include <cstring>
#include <cstdarg>
#include <string>
#include <functional>
#include <chrono>
#include <stdexcept>

#ifdef _WIN32
    #define testExport extern "C" __declspec(dllexport)
#else
    #define testExport extern "C"
#endif

#define testPrint(...) ::test::PrintImpl(__VA_ARGS__)
#define testPuts(...) ::test::PutsImpl(__VA_ARGS__)

#define testRegisterInitializer(Name, Init, Fini)\
    static struct _TestInitializer_##Name {\
        _TestInitializer_##Name() { ::test::RegisterInitializer(Init, Fini); }\
    } g_TestInitializer_##Name;

#define testRegisterTestCase(Name)\
    static struct _TestCase_##Name {\
        _TestCase_##Name() { ::test::RegisterTestCaseImpl(#Name, Name); }\
    } g_TestCase_##Name;

#define testCase(Name) void Name(); testRegisterTestCase(Name); void Name()
#define testExpect(Body) if(!(Body)) { throw std::runtime_error(::test::Format("%s(%d): failed - " #Body "\n", __FILE__, __LINE__)); }


namespace test {

using nanosec = uint64_t;
nanosec Now();
inline double NS2US(nanosec ns) { return double(ns) / 1000.0; }
inline double NS2MS(nanosec ns) { return double(ns) / 1000000.0; }
inline double NS2S(nanosec ns)  { return double(ns) / 1000000000.0; }

void RegisterInitializer(const std::function<void()>& init, const std::function<void()>& fini);
void RegisterTestCaseImpl(const char* name, const std::function<void()>& body);
std::string Format(const char* format, ...);
void PrintImpl(const char* format, ...);
void PutsImpl(const char* format);
template<class T> bool GetArg(const char* name, T& dst);


class Timer
{
public:
    Timer(nanosec start = Now()) : start_(start) {}
    void reset(nanosec start = Now()) { start_ = start; }
    nanosec elapsed_ns() { return Now() - start_; }
    double elapsed_ms() { return NS2MS(elapsed_ns()); }

private:
    nanosec start_;
};

template<class Body>
inline void TestScope(const char* name, const Body& body, int num_try = 1)
{
    Timer timer;
    for (int i = 0; i < num_try; ++i)
        body();

    float elapsed = timer.elapsed_ms();
    testPrint("    %s: %.2lfms", name, elapsed / num_try);
    if (num_try > 1) {
        testPrint(" (%.2lfms in total)", elapsed);
    }
    testPrint("\n");
}

} // namespace test

