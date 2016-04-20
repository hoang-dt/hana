#pragma once

#include "types.h"
#include "../core/macros.h"
#include "../core/string.h"
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

/** Given the dimensions of a volume, guess a suitable bit string for it.
For example, a 8x4 2D slice will be assigned a bit string of 00101. */
void guess_bit_string(const core::Vector3i& dims, OUT core::StringRef& bit_string);

/** Get the (x, y, z) coordinates of the last sample in the given hz level. */
core::Vector3i get_last_coord(core::StringRef bit_string, int hz_level);

/** Get the (x, y, z) coordinates of the first sample in the given hz level. */
core::Vector3i get_first_coord(core::StringRef bit_string, int hz_level);

/** Return the intra-block strides in x, y, and z for samples in a given hz level. */
core::Vector3i get_intra_block_strides(core::StringRef bit_string, int hz_level);

/** Return the strides (in terms of the first sample) of idx blocks, in x, y, and z. */
core::Vector3i get_inter_block_strides(core::StringRef bit_string, int hz_level,
                                       int bits_per_block);

/** Return the strides in x, y, z assuming the last "len" bits in the bit string
are fixed. len can be larger than bit_string.size, in which case the strides will
be larger than the dimensions of the volume itself. */
core::Vector3i get_strides(core::StringRef bit_string, int len);

/** Interleave the bits of the three inputs, according to the rule given in the
input bit string (e.g. V012012012). */
uint64_t interleave_bits(core::StringRef bit_string, core::Vector3i coord);

/** The reverse of interleave bits. */
core::Vector3i deinterleave_bits(core::StringRef bit_string, uint64_t val);

/** Convert a z address to an hz address.
                z      hz
level = 0  000000  000000
level = 1  100000  000001
level = 2  a10000  00001a
level = 3  ab1000  0001ab
level = 4  abc100  001abc
level = 5  abcd10  01abcd
level = 6  abcde1  1abcde
This routine requires the bit string's length to be less than 64. */
uint64_t z_to_hz(core::StringRef bit_string, uint64_t z);

/** Convert an hz address to a z address. The reverse of z_to_hz. */
uint64_t hz_to_z(core::StringRef bit_string, uint64_t hz, int hz_level);

/** Convert an xyz address to an hz address. */
uint64_t xyz_to_hz(core::StringRef bit_string, core::Vector3i coord);

/** Intersect a grid (given by from, to, stride) with a volume. Return only the
part of the grid that are within the volume. Return true if there is at least
one sample of the grid falls inside the volume. */
bool intersect_grid(const Volume& vol, const core::Vector3i& from,
                    const core::Vector3i& to, const core::Vector3i& stride,
                    OUT core::Vector3i& output_from, OUT core::Vector3i& output_to);

}}
