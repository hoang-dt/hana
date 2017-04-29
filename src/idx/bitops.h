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

// TODO: replace this with the forward scan intrinsic
#if defined(__GNUC__) || defined(__MINGW32__) || defined(__clang__)
inline int num_leading_zeros(uint64_t v)
{
    return v==0 ? CHAR_BIT*sizeof(uint64_t) : __builtin_clzll(v);
}
#elif defined(_MSC_VER)
#include <intrin.h>
#pragma intrinsic(_BitScanReverse64)
inline int num_leading_zeros(unsigned __int64 v)
{
  unsigned long index = 0;
  _BitScanReverse64(&index, v);
  return v==0 ? CHAR_BIT*sizeof(__int64) : CHAR_BIT*sizeof(__int64)-1-index;
}
#endif

}
