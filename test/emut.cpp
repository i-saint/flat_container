#ifdef __EMSCRIPTEN__
#include "emut.h"

namespace emut {

EMSCRIPTEN_BINDINGS(emut)
{
    using namespace emscripten;
    class_<Iterator>("Iterator")
        .function("next", &Iterator::next)
        ;
    class_<Iterable>("Iterable")
        .function("@@iterator", &Iterable::iterator)
        ;
}

} // namespace emut
#endif // __EMSCRIPTEN__
