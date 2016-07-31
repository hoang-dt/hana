#pragma once

#include "assert.h"
#include <cstdint>

namespace hana {

/** Set the i(th) least significant bit of val to 1. Index starts at 0. */
template <typename T>
void set_bit(T& val, int i)
{
    HANA_ASSERT(i < sizeof(T) * 8);
    val |= static_cast<T>((1ull << i));
}

/** Set the i(th) least significant bit of val to 0. Index starts at 0. */
template <typename T>
void unset_bit(T& val, int i)
{
    HANA_ASSERT(i < sizeof(T) * 8);
    val &= static_cast<T>(~(1ull << i));
}

/** Check if the i(th) least significant bit of val is 1. Index starts at 0. */
template <typename T>
bool check_bit(T val, int i)
{
    HANA_ASSERT(i < sizeof(T) * 8);
    return (1 & (val >> i));
}

/** Flip the i(th) least significant bit of val. Index starts at 0. */
template <typename T>
void flip_bit(T& val, int i)
{
    HANA_ASSERT(i < sizeof(T) * 8);
    val ^= static_cast<T>(1ull << i);
}

/** Count the number of trailing zero bits. */
// TODO: un-inline this
inline int num_trailing_zeros(uint64_t v)
{
    int c = 0;
    if (v & 0x1) { // special case for odd v (assumed to happen half of the time)
        return 0;
    }
    else {
        c = 1;
        if ((v & 0xffffffff) == 0) {
            v >>= 32;
            c += 32;
        }
        if ((v & 0xffff) == 0) {
            v >>= 16;
            c += 16;
        }
        if ((v & 0xff) == 0) {
            v >>= 8;
            c += 8;
        }
        if ((v & 0xf) == 0) {
            v >>= 4;
            c += 4;
        }
        if ((v & 0x3) == 0) {
            v >>= 2;
            c += 2;
        }
        if (v == 0) {
            c += 1;
        }
        else {
            c -= v & 0x1;
        }
    }
    return c;
}

inline int num_leading_zeros(uint64_t v)
{
    if (v == 0) {
        return sizeof(v) * 8;
    }

    int64_t vv = static_cast<int64_t>(v);
    int result = 0;
    while (vv >= 0) {
        vv = vv << 1;
        ++result;
    }
    return result;
}

}
