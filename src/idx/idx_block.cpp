#include "../core/macros.h"
#include "idx_block.h"

namespace hana { namespace idx {


int64_t IdxBlockHeader::offset() const
{
    return int64_t(buf[2]) | int64_t(buf[3]) << 32;
}

uint32_t IdxBlockHeader::bytes() const
{
    return buf[4];
}

Compression IdxBlockHeader::compression() const
{
    switch (buf[5] & 0xf) {
        case 0: return Compression::None;
        case 3: return Compression::Zip;
        case 4: return Compression::Jpg;
        case 5: return Compression::Exr;
        case 6: return Compression::Png;
    default: HANA_ASSERT(false);
    };
    return Compression::Invalid;
}

Format IdxBlockHeader::format() const
{
    if ((buf[5] & 0x10) == 0x10) {
        return Format::RowMajor;
    }
    return Format::Hz;
}

void IdxBlockHeader::swap_bytes()
{
    buf[2] = _byteswap_ulong(buf[2]);
    buf[3] = _byteswap_ulong(buf[3]);
    buf[4] = _byteswap_ulong(buf[4]);
    buf[5] = _byteswap_ulong(buf[5]);
}

uint64_t IdxBlock::num_samples() const
{
    core::Vector3u64 num_samples = (to - from) / stride + 1;
    return num_samples.x * num_samples.y * num_samples.z;
}

}}
