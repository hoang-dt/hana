#pragma once

#include <array>
#include <cstdint>
#include <utility>

namespace hana { namespace idx {

/** Often times we want to run a function that must work on different primitive
types (UInt8, UInt32, Float32, etc). Instead of duplicating the same function over
and over, we use this foward_func function to dispatch the function that we want,
base on the size of the type.
NOTE: Since the function that is dispatched is often a template function, it has
to be encapsulated in a template class (i.e. a functor) instead. */
template <template <typename> typename C, typename T, typename ... A>
auto forward_functor(int bytes, A&& ... args)
    -> decltype(C<T>()(std::forward<A>(args) ...))
{
    if (bytes % sizeof(uint64_t) == 0) {
        #define CASE(n) case (n): return C<std::array<uint64_t, (n)>>()(std::forward<A>(args) ...);
        switch (bytes / sizeof(uint64_t)) {
            CASE(1); CASE(2); CASE(3); CASE(4); CASE(5); CASE(6); CASE(7); CASE(8);
            default: HANA_ASSERT(false);
        };
        #undef CASE
    }
    else if (bytes % sizeof(uint32_t) == 0) {
        #define CASE(n) case (n): return C<std::array<uint32_t, (n)>>()(std::forward<A>(args) ...);
        switch (bytes / sizeof(uint32_t)) {
            CASE(1); CASE(3); CASE(5); CASE(7);
            default: HANA_ASSERT(false);
        };
        #undef CASE
    }
    else if (bytes % sizeof(uint16_t) == 0) {
        #define CASE(n) case (n): return C<std::array<uint16_t, (n)>>()(std::forward<A>(args) ...);
        switch (bytes / sizeof(uint16_t)) {
            CASE(1); CASE(3); CASE(5); CASE(7);
            default: HANA_ASSERT(false);
        };
        #undef CASE
    }
    else if (bytes % sizeof(uint8_t) == 0) {
        #define CASE(n) case (n): return C<std::array<uint8_t, (n)>>()(std::forward<A>(args) ...);
        switch (bytes / sizeof(uint8_t)) {
            CASE(1); CASE(3); CASE(5); CASE(7);
            default: HANA_ASSERT(false);
        };
        #undef CASE
    }
}

}}
