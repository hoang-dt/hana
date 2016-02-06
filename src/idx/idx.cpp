#include "../core/macros.h"
#include "../core/array.h"
#include "../core/bitops.h"
#include "../core/constants.h"
#include "../core/string.h"
#include "../core/allocator.h"
#include "../core/utils.h"
#include "utils.h"
#include "idx.h"
#include "error.h"
#include <algorithm>
#include <array>
#include <thread>
#include <mutex>
#include <atomic>
#include <iostream>

namespace hana { namespace idx {

/** Get the name of the binary file that contains a given hz address.
For example, if the hz address is 0100'0101'0010'1100 and the file name template
is ./%02x/%01x/%01x.bin, then the binary file name is ./45/2/c.bin, assuming
there is no time step template (4 = 0100, 5 = 0101, 2 = 0010, and c = 1100).
The time step template, if present, will be added to the beginning of the path. */
void get_file_name(const IdxFile& idx_file, int time, uint64_t hz_address,
                   OUT core::StringRef file_name)
{
    // TODO: we can optimize a little more by "pre-building" the static parts
    // of the file name and just fill in the templated parts
    const char hex_digits[] = "0123456789abcdef";

    int pos = 0; // keep track of where we are in the output string

    // if the path specified in the idx file is relative (to the idx file itself)
    // , add the absolute path to the .idx file itself
    if (idx_file.filename_template.head.is_relative()) {
        pos += snprintf(file_name.ptr + pos, file_name.size - pos, "%s/",
                        idx_file.absolute_path.path_string().cptr);
    }

    // add the head of the path
    const FileNameTemplate& name_template = idx_file.filename_template;
    pos += snprintf(file_name.ptr + pos, file_name.size - pos, "%s/",
                    name_template.head.path_string().cptr);
    // add the time template
    pos += snprintf(file_name.ptr + pos, file_name.size - pos,
                    idx_file.time.template_, time);

    // calculate the end index to the string (stored in pos)
    uint64_t z = hz_address;
    int last_num_hex_bits = 0;
    for (int i = 0; name_template.num_hex_bits[i] != 0; ++i) {
        last_num_hex_bits = name_template.num_hex_bits[i];
        pos += last_num_hex_bits + 1; // +1 for the '/'
        z >>= (4 * last_num_hex_bits);
    }
    while (z > 0) { // repeat the last template if necessary
        pos += last_num_hex_bits + 1;
        z >>= 4 * last_num_hex_bits;
    }
    --pos; // remove the last '/'

    // add the file extension at the end (e.g. ".bin")
    snprintf(file_name.ptr + pos, file_name.size - pos, "%s", name_template.ext);
    --pos;

    // build the template parts backwards (%03x/%04x...)
    for (int i = 0; name_template.num_hex_bits[i] != 0; ++i) {
        last_num_hex_bits = name_template.num_hex_bits[i];
        for (size_t j = 0; j < last_num_hex_bits; ++j) {
            file_name.ptr[pos--] = hex_digits[hz_address & 0xfu]; // take 4 last bits
            hz_address >>= 4;
        }
        file_name.ptr[pos--] = '/';
    }

    // repeat the last template if necessary
    while (hz_address > 0) {
        for (int j = 0; j < last_num_hex_bits; ++j) {
            file_name.ptr[pos--] = hex_digits[hz_address & 0xfu];
            hz_address >>= 4;
        }
        file_name.ptr[pos--] = '/';
    }
}

/** Interleave the bits of the three inputs, according to the rule given in the
input bit string (e.g. V012012012). */
uint64_t interleave_bits(core::StringRef bit_string, core::Vector3i coord)
{
    uint64_t interleaved_val = 0;
    const uint64_t one = 1;
    HANA_ASSERT(bit_string.size <= BIT_SIZE(uint64_t) + 1 && bit_string.size > 0);
    // TODO: try re-writing this loop in a more efficient way by removing
    // data dependency between loop iterations
    for (int i = int(bit_string.size) - 1; i >= 0; --i) {
        char v = bit_string[i];
        int j = static_cast<int>(bit_string.size) - i - 1;
        if (v == '0') {
            interleaved_val |= (coord.x & one) << j;
            coord.x >>= 1;
        }
        else if (v == '1') {
            interleaved_val |= (coord.y & one) << j;
            coord.y >>= 1;
        }
        else if (v == '2') {
            interleaved_val |= (coord.z & one) << j;
            coord.z >>= 1;
        }
        else {
            HANA_ASSERT(false);
        }
    }
    return interleaved_val;
}

/** The reverse of interleave bits. */
core::Vector3i deinterleave_bits(core::StringRef bit_string, uint64_t val)
{
    core::Vector3i coord(0, 0, 0);
    const uint64_t one = 1;
    for (int i = 0; i < bit_string.size; ++i) {
        char v = bit_string[i];
        int j = static_cast<int>(bit_string.size) - i - 1;
        if (v == '0') {
            coord.x |= (val & (one << j)) >> j;
            coord.x <<= 1;
        }
        else if (v == '1') {
            coord.y |= (val & (one << j)) >> j;
            coord.y <<= 1;
        }
        else if (v == '2') {
            coord.z |= (val & (one << j)) >> j;
            coord.z <<= 1;
        }
        else {
            HANA_ASSERT(false);
        }
    }
    coord.x >>= 1;
    coord.y >>= 1;
    coord.z >>= 1;
    return coord;
}

/** Convert a z address to an hz address.
                z      hz
level = 0  000000  000000
level = 1  100000  000001
level = 2  a10000  00001a
level = 3  ab1000  0001ab
level = 4  abc100  001abc
level = 5  abcd10  01abcd
level = 6  abcde1  1abcde
This routines require the bit string's length to be less than 64. */
uint64_t z_to_hz(core::StringRef bit_string, uint64_t z)
{
    HANA_ASSERT(bit_string.size > 0 && bit_string.size < 64);
    z |= uint64_t(1) << (bit_string.size);
    z /= z & -int64_t(z);
    z >>= 1;
    return z;
}

/** Convert an hz address to a z address. The reverse of z_to_hz. */
uint64_t hz_to_z(core::StringRef bit_string, uint64_t hz, int hz_level)
{
    HANA_ASSERT(bit_string.size > 0 && bit_string.size < 64 && bit_string.size >= hz_level);
    int z_level = static_cast<int>(bit_string.size) - hz_level;
    HANA_ASSERT((hz << z_level) >> z_level == hz);
    uint64_t z = hz << (z_level + 1);
    z |= uint64_t(1) << z_level;
    return z;
}

uint64_t xyz_to_hz(core::StringRef bit_string, core::Vector3i coord)
{
    uint64_t z = interleave_bits(bit_string, coord);
    uint64_t hz = z_to_hz(bit_string, z);
    return hz;
}

core::Vector3i get_inter_block_strides(core::StringRef bit_string, int hz_level,
                                       int bits_per_block);

core::Vector3i get_intra_block_strides(core::StringRef bit_string, int hz_level);

core::Vector3i get_first_coord(core::StringRef bit_string, int hz_level);
core::Vector3i get_last_coord(core::StringRef bit_string, int hz_level);

bool intersect_grid(const Volume& vol, const core::Vector3i& from,
                    const core::Vector3i& to, const core::Vector3i& stride,
                    OUT core::Vector3i& output_from, OUT core::Vector3i& output_to);

/** Given a 3D extend (a volume), get the (sorted) list of idx block addresses
that intersect this volume, at a given hz level. The addresses are in hz space.
The input volume (vol) should be inclusive at both ends. */
void get_block_addresses(const IdxFile& idx_file, const Volume& vol, int hz_level,
                         OUT core::Array<IdxBlock>* idx_blocks)
{
    HANA_ASSERT(hz_level <= idx_file.bit_string.size);
    HANA_ASSERT(vol.is_valid());

    using namespace core;
    StringRef bit_string = idx_file.bit_string;
    int bpb = idx_file.bits_per_block;

    bool first_block = hz_level < idx_file.get_min_hz_level();
    Vector3i start(0, 0, 0);
    if (first_block) {
        hz_level = idx_file.get_min_hz_level() - 1;
    }
    else {
        start = get_first_coord(bit_string, hz_level);
    }

    // get the strides for the first sample of each block in x, y, and z
    Vector3i stride = get_inter_block_strides(bit_string, hz_level, bpb);

    // get the range of blocks in x, y, z
    Vector3i from = start + ((vol.from - start) / stride) * stride;
    Vector3i to = start + ((vol.to - start) / stride) * stride;
    if (vol.from.x < start.x) { from.x = start.x; }
    if (vol.from.y < start.y) { from.y = start.y; }
    if (vol.from.z < start.z) { from.z = start.z; }
    if (vol.to.x < start.x) { to.x = start.x - stride.x; }
    if (vol.to.y < start.y) { to.y = start.y - stride.y; }
    if (vol.to.z < start.z) { to.z = start.z - stride.z; }
    if (!(from <= to)) {
        return;  // no blocks intersect with the input volume in this hz level
    }

    // loop through the blocks in xyz space, convert their coordinates to hz
    idx_blocks->clear();
    for (int z = from.z; z <= to.z; z += stride.z) {
        for (int y = from.y; y <= to.y; y += stride.y) {
            for (int x = from.x; x <= to.x; x += stride.x) {
                uint64_t hz = xyz_to_hz(bit_string,Vector3i(x, y, z));
                HANA_ASSERT(((hz >> bpb) << bpb) == hz);
                IdxBlock block;
                block.hz_address = hz;
                block.from = core::Vector3i(x, y, z);
                if (first_block) { // for the first block, we combine all the
                    // hz levels in the block (from 0 to min hz level - 1), and
                    // the resulting grid has the same strides as those of the
                    // next hz level.
                    block.stride = get_intra_block_strides(bit_string, hz_level + 1);
                }
                else {
                    block.stride = get_intra_block_strides(bit_string, hz_level);
                }
                block.to = block.from + stride - block.stride;
                core::Vector3i last_coord = get_last_coord(bit_string, hz_level);
                if (last_coord <= block.to) {
                    block.to = last_coord;
                }
                block.hz_level = hz_level;
                idx_blocks->push_back(block);
            }
        }
    }
    std::sort(idx_blocks->begin(), idx_blocks->end(),
        [](const IdxBlock& a, const IdxBlock& b) { return a.hz_address < b.hz_address; });
}

// We need exclusive access to the memory allocator for the blocks.
std::mutex mutex;

Error read_idx_block(
    const IdxFile& idx_file, int field, int time, IN_OUT uint64_t* last_first_block_index,
    IN_OUT FILE** file, IN_OUT core::Array<IdxBlockHeader>* block_headers,
    IN_OUT IdxBlock* block, core::Allocator& alloc)
{
    using namespace core;

    int bpf = idx_file.blocks_per_file;
    // block index is the rank of this block (0th block, 1st block, 2nd,...)
    uint64_t block_index = block->hz_address >> idx_file.bits_per_block;
    uint64_t first_block_index = block_index - (block_index % bpf);
    uint64_t block_in_file = block_index - first_block_index;
    HANA_ASSERT(block_in_file < bpf);

    char bin_path[512]; // path to the binary file that stores the block
    get_file_name(idx_file, time, first_block_index, STR_REF(bin_path));
    if (*last_first_block_index != first_block_index) { // open a new file
        *last_first_block_index = first_block_index;
        if (*file) {
            fclose(*file);
        }
        // open the binary file and read all necessary block headers
        *file = fopen(bin_path, "rb");
        if (!*file) {
            return Error::FileNotFound;
        }
        fseek(*file, sizeof(IdxFileHeader) + sizeof(IdxBlockHeader) * bpf * field, SEEK_SET);
        fread(&(*block_headers)[0], sizeof(IdxBlockHeader), bpf, *file);
    }

    // decode the header for the current block
    IdxBlockHeader& header = (*block_headers)[block_in_file];
    header.swap_bytes();
    int64_t block_offset = header.offset();
    block->bytes = header.bytes();
    if (block_offset == 0 || block->bytes == 0) {
        return Error::BlockNotFound; // not a critical error
    }
    block->compression = header.compression();
    HANA_ASSERT(block->compression != Compression::Invalid);
    if (block->compression == Compression::Invalid) {
        return Error::InvalidCompression; // critical error
    }
    block->format = header.format();
    block->type = idx_file.fields[field].type;

    // read the block's actual data
    mutex.lock();
    block->data = alloc.allocate(block->bytes);
    mutex.unlock();
    fseek(*file, block_offset, SEEK_SET);
    if (fread(block->data.ptr, block->data.bytes, 1, *file) != 1) {
        return Error::BlockReadFailed; // critical error
    }

    return Error::NoError;
}

struct Tuple {
    uint64_t hz_address;
    int div_pos; /** The position in the bit string corresponding to the axis of division */
    core::Vector3i from;
    core::Vector3i to;
    uint64_t num_elems;
};

/** For example, if the bit string is xyxyxy, the current z address is
100100 (z level 2), and the bits per block is 2, then the div position (which is
the position of the bit that corresponds to the immediate axis of division) is 1,
corresponding to a division along y.
This is the most significant bit among the bits dedicated to the current block.*/
int dividing_pos(const core::StringRef bit_string, int bits_per_block, int hz_level)
{
    int z_level = static_cast<int>(bit_string.size) - hz_level;
    int div_pos = static_cast<int>(bit_string.size) - (z_level + 1 + bits_per_block);
    return core::max(div_pos, 0);
}

/** Copy data from an idx block to a rectilinear grid, assuming the samples in
the block is in hz order, and the samples in the grid is in row-major order.
We use the fast stack algorithm (see Brian Summa's PhD thesis).
TODO: One optimization would be to build a look up table for a small number of
samples and stop the recursion before it gets to a single sample. */
template <typename T>
struct put_block_to_grid_hz {
void operator()(const core::StringRef bit_string, int bits_per_block,
                const IdxBlock& block, const core::Vector3i& output_from,
                const core::Vector3i& output_to,
                const core::Vector3i& output_stride, IN_OUT Grid* grid)
{
    HANA_ASSERT(block.hz_level <= bit_string.size);

    const int stack_size = 65;
    Tuple stack[stack_size];
    int top = 0;
    stack[top] = Tuple{ block.hz_address,
                        dividing_pos(bit_string, bits_per_block, block.hz_level),
                        block.from, block.to, block.num_samples() };
    T* dst = reinterpret_cast<T*>(grid->data.ptr);
    T* src = reinterpret_cast<T*>(block.data.ptr);

    core::Vector3i output_dims = (output_to - output_from ) / output_stride + 1;
    uint64_t dx = output_dims.x;
    uint64_t dxy = output_dims.x * output_dims.y;
    uint64_t dxyz = output_dims.x * output_dims.y * output_dims.z;
    // keep dividing the volume by 2 alternately along x, y, z (following the bit string)
    while (top >= 0) {
        // pop from the top
        Tuple top_tuple = stack[top--];
        HANA_ASSERT(top_tuple.hz_address == xyz_to_hz(bit_string, top_tuple.from));

        // if this is a single element, put it into the grid and continue
        if (top_tuple.num_elems == 1) {
            HANA_ASSERT(top_tuple.from == top_tuple.to);
            core::Vector3i coord = (top_tuple.from - output_from) / output_stride;
            uint64_t xyz = coord.x + coord.y * dx + coord.z * dxy;
            uint64_t ijk = top_tuple.hz_address - block.hz_address;
            if (xyz < dxyz) {
                dst[xyz] = src[ijk];
            }
            continue;
        }

        // divide the tuple into two halves depending on the dividing axis
        char div_axis = bit_string[top_tuple.div_pos];
        Tuple first, second;
        first.div_pos = second.div_pos = top_tuple.div_pos + 1;
        first.num_elems = second.num_elems = top_tuple.num_elems / 2;
        HANA_ASSERT(top_tuple.num_elems % 2 == 0);
        first.hz_address = top_tuple.hz_address;
        second.hz_address = top_tuple.hz_address + first.num_elems;

        first.from = second.from = top_tuple.from;
        first.to = second.to = top_tuple.to;
        if (div_axis == '0') { // split along x
            int num_sample_x = (first.to.x - first.from.x) / block.stride.x + 1;
            HANA_ASSERT(num_sample_x % 2 == 0);
            num_sample_x /= 2;
            first.to.x = first.from.x + (num_sample_x - 1) * block.stride.x;
            second.from.x = second.to.x - (num_sample_x - 1) * block.stride.x;
        }
        else if (div_axis == '1') { // split along y
            int num_sample_y = (first.to.y - first.from.y) / block.stride.y + 1;
            HANA_ASSERT(num_sample_y % 2 == 0);
            num_sample_y /= 2;
            first.to.y = first.from.y + (num_sample_y - 1) * block.stride.y;
            second.from.y = second.to.y - (num_sample_y - 1) * block.stride.y;
        }
        else if (div_axis == '2') { // split along z
            int num_sample_z = (first.to.z - first.from.z) / block.stride.z + 1;
            HANA_ASSERT(num_sample_z % 2 == 0);
            num_sample_z /= 2;
            first.to.z = first.from.z + (num_sample_z - 1) * block.stride.z;
            second.from.z = second.to.z - (num_sample_z - 1) * block.stride.z;
        }

        // push back the two halves to the top of the stack
        // TODO: maybe we can eliminate one test
        if (grid->extends.from <= second.to && second.from <= grid->extends.to) {
            stack[++top] = second;
        }
        if (grid->extends.from <= first.to && first.from <= grid->extends.to) {
            stack[++top] = first;
        }
    }
}
};

/** Copy data from an idx block to a rectilinear grid, assuming the samples in
both are in row-major order. */
template <typename T>
struct put_block_to_grid {
void operator()(const IdxBlock& block, const core::Vector3i& output_from,
                const core::Vector3i& output_to, const core::Vector3i& output_stride,
                IN_OUT Grid* grid)
{
    using namespace core;
    core::Vector3i from, to;
    if (!intersect_grid(grid->extends, block.from, block.to, block.stride, from, to)) {
        return;
    }

    T* dst = reinterpret_cast<T*>(grid->data.ptr);
    T* src = reinterpret_cast<T*>(block.data.ptr);
    // TODO: optimize this loop (parallelize?)
    core::Vector3i input_dims = (block.to - block.from) / block.stride + 1;
    uint64_t sx = input_dims.x;
    uint64_t sxy = input_dims.x * input_dims.y;
    core::Vector3i output_dims = (output_to - output_from) / output_stride + 1;
    uint64_t dx = output_dims.x;
    uint64_t dxy = output_dims.x * output_dims.y;
    // TODO: maybe write a macro to shorten the code?
    core::Vector3i dd = block.stride / output_stride;
    HANA_ASSERT(output_stride <= block.stride);
    for (int z = from.z,
         k = (from.z - block.from.z) / block.stride.z,
         zz = (from.z - output_from.z) / output_stride.z;
         z <= to.z;
         z += block.stride.z, ++k, zz += dd.z) {
        for (int y = from.y,
             j = (from.y - block.from.y) / block.stride.y,
             yy = (from.y - output_from.y) / output_stride.y;
             y <= to.y;
             y += block.stride.y, ++j, yy += dd.y) {
            for (int x = from.x,
                 i = (from.x - block.from.x) / block.stride.x,
                 xx = (from.x - output_from.x) / output_stride.x;
                 x <= to.x;
                 x += block.stride.x, ++i, xx += dd.x) {
                uint64_t ijk = i + j * sx + k * sxy;
                uint64_t xyz = xx + yy * dx + zz * dxy;
                dst[xyz] = src[ijk];
            }
        }
    }
}
};

core::FreelistAllocator<core::Mallocator> freelist;

Error read_idx_grid(const IdxFile& idx_file, int field, int time, int hz_level,
                    IN_OUT Grid* grid)
{
    if (hz_level < idx_file.get_min_hz_level()) {
        return Error::InvalidHzLevel;
    }

    grid->type = idx_file.fields[field].type;
    core::Vector3i from, to, stride;
    idx_file.get_grid(grid->extends, hz_level, from, to, stride);
    return read_idx_grid(idx_file, field, time, hz_level, from, to, stride, grid);
}

Error read_idx_grid(
    const IdxFile& idx_file, int field, int time, int hz_level,
    const core::Vector3i& output_from, const core::Vector3i& output_to,
    const core::Vector3i& output_stride, IN_OUT Grid* grid)
{
    // check the inputs
    if (!verify_idx_file(idx_file)) {
        return Error::InvalidIdxFile;
    }
    if (field < 0 || field > idx_file.num_fields) {
        return Error::FieldNotFound;
    }
    if (time < idx_file.time.begin || time > idx_file.time.end) {
        return Error::TimeStepNotFound;
    }
    if (hz_level < 0 || hz_level > idx_file.get_max_hz_level()) {
        return Error::InvalidHzLevel;
    }
    HANA_ASSERT(grid);
    if (!grid->extends.is_valid()) {
        return Error::InvalidVolume;
    }
    if (!grid->extends.is_inside(idx_file.box)) {
        return Error::VolumeTooBig;
    }
    HANA_ASSERT(grid->data.ptr);

    grid->type = idx_file.fields[field].type;

    // since all the samples in hz levels smaller than the min hz level belong to
    // the same idx block, we use the following trick to avoid reading that same
    // first block again and again.
    if (hz_level < idx_file.get_min_hz_level()) {
        hz_level = idx_file.get_min_hz_level() - 1;
    }

    using namespace core;
    Error error = Error::NoError;

    Mallocator mallocator;
    // TODO: try to get rid of the following allocation
    Array<IdxBlock> idx_blocks(&mallocator);
    Array<IdxBlockHeader> block_headers(&mallocator);
    block_headers.resize(idx_file.blocks_per_file);

    // NOTE: in the case where hz_level < min hz level, we will treat the first
    // block as if it were in level (min hz level - 1), and we will break this
    // block into multiple smaller "virtual" blocks corresponding to the
    // individual levels later
    get_block_addresses(idx_file, grid->extends, hz_level, &idx_blocks);

    // determine the most likely size of each block and use a FreeListAllocator
    // with this size to allocate actual data (not metadata) for the blocks.
    size_t block_size = idx_file.fields[field].type.bytes() * size_t(power2_[idx_file.bits_per_block]);
    if (freelist.size() != block_size) {
        freelist.set_sizes(block_size * 4 / 5, block_size);
    }

    FILE* file = nullptr;
    uint64_t last_first_block_index = (uint64_t)-1;

    std::atomic_int thread_count{0};

    for (size_t i = 0; i < idx_blocks.size(); ++i) {
        IdxBlock& block = idx_blocks[i];
        Error err = read_idx_block(idx_file, field, time,
            &last_first_block_index, &file, &block_headers, &block, freelist);
        if (err == Error::BlockNotFound || err == Error::FileNotFound) {
            error = err;
            continue; // these are not critical errors (a block may not be saved yet)
        }
        if (err == Error::InvalidCompression || err == Error::BlockReadFailed) {
            return err; // critical errors
        }
        if (block.compression != Compression::None) { // TODO?
            return Error::CompressionUnsupported;
        }
        if (block.format == Format::RowMajor) {
            ++thread_count;
            std::thread copy_block_task([&]() {
                forward_functor<put_block_to_grid, int>(block.type.bytes(),
                    block, output_from, output_to, output_stride, grid);
                mutex.lock();
                freelist.deallocate(block.data);
                --thread_count;
                mutex.unlock();
            });
            copy_block_task.detach();
        } else if (block.format == Format::Hz) {
            if (hz_level < idx_file.get_min_hz_level()) {
                ++thread_count;
                std::thread copy_block_task([&]() {
                    // here we break up the first idx block into multiple "virtual"
                    // blocks, each consisting of only samples in one hz level
                    IdxBlock b = block;
                    b.bytes = b.type.bytes();
                    HANA_ASSERT(b.hz_address == 0);
                    b.hz_level = 0;
                    b.data.bytes = b.bytes;
                    b.from = b.to = Vector3i(0, 0, 0);
                    b.stride = get_intra_block_strides(idx_file.bit_string, b.hz_level);
                    uint64_t old_bytes = 0;
                    uint64_t old_hz = 1;
                    for (int j = 0; b.bytes < block.bytes; ++j) {
                        // each iteration corresponds to one hz level, starting from 0
                        // until min_hz_level - 1
                        forward_functor<put_block_to_grid_hz, int>(b.type.bytes(),
                            idx_file.bit_string, idx_file.bits_per_block, b,
                            output_from, output_to, output_stride, grid);
                        ++b.hz_level;
                        b.data.ptr = b.data.ptr + b.bytes;
                        b.bytes += old_bytes;
                        old_bytes = b.bytes;
                        b.data.bytes = b.bytes;
                        b.hz_address += old_hz;
                        old_hz = b.hz_address;
                        b.from = get_first_coord(idx_file.bit_string, b.hz_level);
                        b.stride = get_intra_block_strides(idx_file.bit_string, b.hz_level);
                        b.to = get_last_coord(idx_file.bit_string, b.hz_level);
                    }

                    mutex.lock();
                    freelist.deallocate(block.data);
                    --thread_count;
                    mutex.unlock();
                });
                copy_block_task.detach();
            }
            else { // for hz levels >= min hz level
                ++thread_count;
                std::thread copy_block_task([&](){
                    forward_functor<put_block_to_grid_hz, int>(block.type.bytes(),
                        idx_file.bit_string, idx_file.bits_per_block, block,
                        output_from, output_to, output_stride, grid);
                    mutex.lock();
                    freelist.deallocate(block.data);
                    --thread_count;
                    mutex.unlock();
                });
                copy_block_task.detach();
            }
        } else {
            error = Error::InvalidFormat;
        }
    }

    // wait for all the threads to finish
    while (thread_count > 0) {}

    if (file) {
        fclose(file);
    }

    return error;
}

Error read_idx_grid_inclusive(const IdxFile& idx_file, int field, int time,
                              int hz_level, IN_OUT Grid* grid)
{
    grid->type = idx_file.fields[field].type;
    core::Vector3i from, to, stride;
    if (hz_level < idx_file.get_min_hz_level()) {
        hz_level = idx_file.get_min_hz_level() - 1;
    }
    idx_file.get_grid_inclusive(grid->extends, hz_level, from, to, stride);
    Error err = read_idx_grid(idx_file, field, time, 0, from, to, stride, grid);
    if (err.code != Error::NoError) {
        return err;
    }
    int min_hz = idx_file.get_min_hz_level();
    for (int l = min_hz; l <= hz_level; ++l) {
        err = read_idx_grid(idx_file, field, time, l, from, to, stride, grid);
        if (err.code != Error::NoError) {
            return err;
        }
    }
    return Error::NoError;
}

void deallocate_memory()
{
    freelist.deallocate_all();
}

}}
