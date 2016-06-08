#pragma once

#include "assert.h"
#include "utils.h"

namespace hana { namespace core {

/** Generate a power table for a particular base and type. */
template <typename T, int N>
const T* power(T base)
{
    static T memoir[N] = { 0 };
    memoir[0] = 1;
    for (int i = 1; i <= N; ++i) {
        memoir[i] = memoir[i - 1] * base;
    }
    return memoir;
}

static auto pow2 = power<int, 31>(2);
static auto pow10 = power<int, 9>(10);

/** Find the first power of, say, 2 that is greater than or equal to the given number. */
inline int pow_greater_equal(int base, int num)
{
    HANA_ASSERT(base > 1);
    HANA_ASSERT(num > 0);

    int result = 1;
    while (result < num)
        result *= base;
    return result;
}

/** Find the integer log of a number in a given base.
If the number is not a power of the base, round the log down. */
template <typename T>
inline int log_int(T base, T num)
{
    //static_assert(std::is_integral<T>::value, "Integer required.");

    HANA_ASSERT(base > 1);
    HANA_ASSERT(num > 0);

    T s = 1;
    int log = 0;
    while (s <= num) {
        s *= base;
        ++log;
    }
    return max(log - 1, 0);
}

}}
