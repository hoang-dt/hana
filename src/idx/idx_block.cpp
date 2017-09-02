#include "macros.h"
#include "bitops.h"
#include "idx_block.h"

namespace hana {

int64_t IdxBlockHeader::offset() const
{
  return int64_t(buf[2]) | (int64_t(buf[3]) << 32);
}

void IdxBlockHeader::set_offset(int64_t offset)
{
  buf[2] = static_cast<uint32_t>(offset & 0xFFFFFFFF);
  buf[3] = static_cast<uint32_t>(offset >> 32);
}

uint32_t IdxBlockHeader::bytes() const
{
  return buf[4];
}

void IdxBlockHeader::set_bytes(uint32_t bytes)
{
  buf[4] = bytes;
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

void IdxBlockHeader::set_compression(Compression comp)
{
  // TODO
}

Format IdxBlockHeader::format() const
{
  if ((buf[5] & 0x10) == 0x10) {
    return Format::RowMajor;
  }
  return Format::Hz;
}

void IdxBlockHeader::set_format(Format format)
{
  if (format == Format::RowMajor) {
    set_bit(buf[5], 5);
  }
  else if (format == Format::Hz) {
    unset_bit(buf[5], 5);
  }
}

void IdxBlockHeader::swap_bytes()
{
  buf[2] = HANA_BYTE_SWAP_4(buf[2]);
  buf[3] = HANA_BYTE_SWAP_4(buf[3]);
  buf[4] = HANA_BYTE_SWAP_4(buf[4]);
  buf[5] = HANA_BYTE_SWAP_4(buf[5]);
}

uint64_t IdxBlock::num_samples() const
{
  Vector3u64 num_samples = (to - from) / stride + 1;
  return num_samples.x * num_samples.y * num_samples.z;
}

}
