#pragma once

#include "idx_file.h"
#include "../core/types.h"
#include <cstdint>

namespace hana { namespace idx {

struct IdxFileHeader {
    static const size_t size = 10;
    uint32_t buf[size] = {};
};

struct IdxBlockHeader {
    static const size_t size_ = 10;
    uint32_t buf[size_] = {};

    /** Get the offset of the idx block in the file (offset from the beginning of the file. */
    int64_t offset() const;
    /** Get the size of the idx block (not the size of the header). */
    uint32_t bytes() const;
    /** Get the type of compression used for the idx block. */
    Compression compression() const;
    /** Get whether the samples in the block are in Hz or row-major order. */
    Format format() const;
    /** Swap the byte order in the header (big endian to little endian and vice versa). */
    void swap_bytes();
};

struct IdxBlock {
    /** The smallest xyz coordinates among all samples in the block. */
    core::Vector3i from = core::Vector3i(0, 0, 0);
    /** The largest xyz coordinates among all samples in the block. */
    core::Vector3i to = core::Vector3i(0, 0 ,0);
    /** The strides in xyz coordinates for samples in the block. */
    core::Vector3i stride = core::Vector3i(0, 0, 0);
    /** The "payload" of the block. */
    core::MemBlockChar data;
    /** hz address of the first sample in the block. */
    uint64_t hz_address = 0;
    /** Size of the block in bytes. */
    uint64_t bytes = 0;
    /** Type of each sample in the block (e.g. UInt8/UInt16, etc). */
    IdxType type;
    /** The hz level to which the block belongs. If the block contains multiple
    hz levels (i.e. the first block), this is the largest among them. */
    int hz_level = 0;
    /** The type of compression used to compress the block. */
    Compression compression = Compression::Invalid;
    /** Whether the samples are in Hz order or row-major order. */
    Format format = Format::Hz;

    /** Get the number of samples in the block*/
    uint64_t num_samples() const;
};

}}

