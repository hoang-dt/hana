#pragma once

#include "allocator.h"
#include "assert.h"
#include <streambuf>

namespace hana { namespace core {

/** An output stream buffer that lets you use custom allocators. */
template <typename CharType>
class OStreamBuf : public std::basic_streambuf<CharType, std::char_traits<CharType>>
{
private:
    Allocator* alloc_ = nullptr;
    MemBlockVoid mem_block_;

public:
    OStreamBuf(Allocator* alloc, std::streamsize length)
        : alloc_(alloc)
    {
        HANA_ASSERT(alloc_);
        mem_block_ = alloc_->allocate(sizeof(CharType) * length);
        memset(mem_block_.ptr, 0, mem_block_.bytes);
        CharType* buf = static_cast<CharType*>(mem_block_.ptr);
        std::streambuf::setp(buf, buf + length);
    }

    const char* buffer() const
    {
        return static_cast<char*>(mem_block_.ptr);
    }

    ~OStreamBuf()
    {
        if (alloc_) {
            alloc_->deallocate(mem_block_);
        }
    }
};

}}
