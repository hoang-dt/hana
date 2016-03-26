#pragma once

#include "assert.h"
#include "utils.h"

namespace hana { namespace core {

template <int base>
int pow(int exp)
{
    HANA_ASSERT(exp >= 0 && exp < 64);

    static int memoir[64] = { 0 };
    static int last_exp = 0;
    memoir[0] = 1;

    int result = memoir[min(last_exp, exp)];
    if (exp > last_exp) {
        for (int i = last_exp + 1; i <= exp; ++i) {
            memoir[i] = (result = result * base);
        }
        last_exp = exp;
    }
    return result;
}

}}
