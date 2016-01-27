#pragma once

#include "types.h"

namespace hana { namespace core {

template <typename T>
const T& min(const T& a, const T& b)
{
    return a < b ? a : b;
}

template <typename T>
const T& max(const T& a, const T& b)
{
    return a > b ? a : b;
}

template <typename T>
void swap(T& a, T& b)
{
    T c = a;
    a = b;
    b = c;
}

template <typename T>
void reverse(ArrayRef<T> arr)
{
    if (arr.size <= 1) {
        return;
    }
    size_t i = arr.size - 1;
    while (i >= arr.size / 2) {
        swap(arr[i], arr[arr.size - i - 1]);
        --i;
    }
}

}}
