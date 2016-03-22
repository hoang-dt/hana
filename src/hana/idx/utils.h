#pragma once

#include "types.h"
#include <cstdint>
#include <cstring>
#include <iosfwd>
#include <utility>

namespace hana { namespace idx {

// T must be POD type
// This type "wraps" for example an array of 64 doubles.
// When copying a large number of samples, we use memcpy.
// When copying a small number of them, we use regular operator= (see the
// specialization below).
template <typename T, int N>
struct TypeWrapper {
    T a[N];
    TypeWrapper& operator=(const TypeWrapper& other) {
        memcpy(a, other.a, sizeof(T) * N);
        return *this;
    }
};

// Specialize TypeWrapper for small types (to avoid calling memcpy in operator=).
#define SMALL_TYPE_WRAPPER(N) \
    template <typename T> \
    struct TypeWrapper<T, (N)> { \
        T a[N]; \
    }
SMALL_TYPE_WRAPPER(1);
SMALL_TYPE_WRAPPER(2);
SMALL_TYPE_WRAPPER(3);
SMALL_TYPE_WRAPPER(4);
SMALL_TYPE_WRAPPER(5);
SMALL_TYPE_WRAPPER(6);
SMALL_TYPE_WRAPPER(7);
SMALL_TYPE_WRAPPER(8);
#undef SMALL_TYPE_WRAPPER

template <typename T, int N>
std::ostream& operator<<(std::ostream& os, const TypeWrapper<T, N>& tw)
{
    for (const auto& v : tw.a) {
        os << v << " ";
    }
    return os;
}

/** Often times we want to run a function that must work on different primitive
types (UInt8, UInt32, Float32, etc). Instead of duplicating the same function over
and over, we use this foward_func function to dispatch the function that we want,
base on the size of the type.
NOTE: Since the function that is dispatched is often a template function, it has
to be encapsulated in a template class (i.e. a functor) instead. */
template <template <typename> class C, typename T, typename ... A>
auto forward_functor(int bytes, A&& ... args)
    -> decltype(C<T>()(std::forward<A>(args) ...))
{
    if (bytes % sizeof(uint64_t) == 0) {
        #define CASE(n) case (n): return C<TypeWrapper<uint64_t, (n)>>()(std::forward<A>(args) ...);
        switch (bytes / sizeof(uint64_t)) {
            CASE(1); CASE(2); CASE(3); CASE(4); CASE(8); CASE(16); CASE(32); CASE(64);
            default: HANA_ASSERT(false);
        };
        #undef CASE
    }
    else if (bytes % sizeof(uint32_t) == 0) {
        #define CASE(n) case (n): return C<TypeWrapper<uint32_t, (n)>>()(std::forward<A>(args) ...);
        switch (bytes / sizeof(uint32_t)) {
            CASE(1); CASE(3); CASE(5); CASE(7);
            default: HANA_ASSERT(false);
        };
        #undef CASE
    }
    else if (bytes % sizeof(uint16_t) == 0) {
        #define CASE(n) case (n): return C<TypeWrapper<uint16_t, (n)>>()(std::forward<A>(args) ...);
        switch (bytes / sizeof(uint16_t)) {
            CASE(1); CASE(3); CASE(5); CASE(7);
            default: HANA_ASSERT(false);
        };
        #undef CASE
    }
    else if (bytes % sizeof(uint8_t) == 0) {
        #define CASE(n) case (n): return C<TypeWrapper<uint8_t, (n)>>()(std::forward<A>(args) ...);
        switch (bytes / sizeof(uint8_t)) {
            CASE(1); CASE(3); CASE(5); CASE(7);
            default: HANA_ASSERT(false);
        };
        #undef CASE
    }
}

}}
