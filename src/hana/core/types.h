#pragma once
#include "assert.h"
#include <cstddef>

namespace hana { namespace core {

/** Encapsulate a pointer and a size so that we don't have to pass them to
functions separately. */
template <typename T>
struct ArrayRef {
    T* ptr = nullptr;
    size_t size = 0;
    ArrayRef() : ptr(nullptr), size(0) {}
    ArrayRef(T* p, size_t s) : ptr(p), size(s) {}
    T& operator[](size_t i) { HANA_ASSERT(i < size); return ptr[i]; }
    const T& operator[](size_t i) const { HANA_ASSERT(i < size); return ptr[i]; }
};

using BufferRef = ArrayRef<char>;

// A memory block with a pointer to the beginning of the block and a size in bytes
template <typename T>
struct MemBlock {
    T* ptr = nullptr;
    size_t bytes = 0;
    MemBlock() = default;
    MemBlock(T* p, size_t b) : ptr(p), bytes(b) {}

    template <typename T2>
    MemBlock(const MemBlock<T2>& b2)
        : ptr(static_cast<T*>(b2.ptr))
        , bytes(b2.bytes) {}

    template <typename T2>
    MemBlock& operator=(const MemBlock<T2>& b2)
    {
        ptr = static_cast<T*>(b2.ptr);
        bytes = b2.bytes;
        return *this;
    }
};

using MemBlockVoid = MemBlock<void>;
using MemBlockChar = MemBlock<char>;

}}
