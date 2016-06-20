#include "../core/macros.h"
#include "../core/array.h"
#include "../core/bitops.h"
#include "../core/constants.h"
#include "../core/math.h"
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

namespace hana { namespace idx {

/** Get the name of the binary file that contains a given hz address.
For example, if the hz address is 0100'0101'0010'1100 and the file name template
is ./%02x/%01x/%01x.bin, then the binary file name is ./45/2/c.bin, assuming
there is no time step template (4 = 0100, 5 = 0101, 2 = 0010, and c = 1100).
The time step template, if present, will be added to the beginning of the path. */
namespace {
void get_file_name_from_hz(const IdxFile& idx_file, int time, uint64_t hz_address,
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
        for (int j = 0; j < last_num_hex_bits; ++j) {
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
}}

/** Given a block's hz address, compute the hz address of the first block in the
file, and the block's index in the file. */
namespace {
void get_first_block_in_file(uint64_t block, int bits_per_block, int blocks_per_file,
                             OUT uint64_t* first_block, OUT int* block_in_file)
{
    // block index is the rank of this block (0th block, 1st block, 2nd,...)
    uint64_t block_id = block >> bits_per_block;
    *first_block = block_id - (block_id % blocks_per_file);
    *block_in_file = static_cast<int>(block_id - *first_block);
    HANA_ASSERT(*block_in_file < blocks_per_file);
}}

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

/** Given a binary file, read all the block headers from the file. */
namespace {
Error read_block_headers(IN_OUT FILE** file, const char* bin_path, int field,
                         int blocks_per_file,
                         IN_OUT core::Array<IdxBlockHeader>* headers)
{
    HANA_ASSERT(file != nullptr);
    *file = fopen(bin_path, "rb");
    if (!*file)
        return Error::FileNotFound;
    if (fseek(*file, sizeof(IdxFileHeader) +
        sizeof(IdxBlockHeader) * blocks_per_file * field, SEEK_SET))
        return Error::HeaderNotFound;
    if (fread(&(*headers)[0], sizeof(IdxBlockHeader), blocks_per_file, *file)
        != blocks_per_file)
        return Error::HeaderNotFound;
    return Error::NoError;
}}

// We need exclusive access to the memory allocator for the blocks.
std::mutex mutex;
/** Read an IDX block out of a file. Beside returning the data in the block, this
function returns the HZ index of the last first block in a file read, as well as
the last file read. This is so that the next call to the function does not open
the same file. The file is returned so that after the last call, the caller can
close the last file opened. */
namespace {
Error read_idx_block(const IdxFile& idx_file, int field, int time, bool read_headers,
                     IN_OUT uint64_t* last_first_block, IN_OUT FILE** file,
                     IN_OUT core::Array<IdxBlockHeader>* block_headers,
                     IN_OUT IdxBlock* block, core::Allocator& alloc)
{
    using namespace core;
    HANA_ASSERT(file != nullptr);
    HANA_ASSERT(last_first_block != nullptr);
    HANA_ASSERT(block_headers != nullptr);
    HANA_ASSERT(block != nullptr);

    // block index is the rank of this block (0th block, 1st block, 2nd,...)
    uint64_t first_block = 0;
    int block_in_file = 0;
    get_first_block_in_file(block->hz_address, idx_file.bits_per_block,
                            idx_file.blocks_per_file, &first_block, &block_in_file);
    char bin_path[512]; // path to the binary file that stores the block
    get_file_name_from_hz(idx_file, time, first_block, STR_REF(bin_path));
    if (*last_first_block != first_block) { // open a new file
        *last_first_block = first_block;
        if (*file)
            fclose(*file);
        if (read_headers) {
            Error err = read_block_headers(file, bin_path, field,
                                           idx_file.blocks_per_file, block_headers);
            if (err.code != Error::NoError)
                return err;
        }
    }

    // decode the header for the current block
    IdxBlockHeader& header = (*block_headers)[block_in_file];
    header.swap_bytes();
    int64_t block_offset = header.offset();
    block->bytes = header.bytes();
    if (block_offset == 0 || block->bytes == 0)
        return Error::BlockNotFound; // not a critical error
    block->compression = header.compression();
    HANA_ASSERT(block->compression != Compression::Invalid);
    if (block->compression == Compression::Invalid)
        return Error::InvalidCompression; // critical error
    block->format = header.format();
    block->type = idx_file.fields[field].type;

    // read the block's actual data
    mutex.lock();
    block->data = alloc.allocate(block->bytes);
    mutex.unlock();
    fseek(*file, block_offset, SEEK_SET);
    if (fread(block->data.ptr, block->data.bytes, 1, *file) != 1)
        return Error::BlockReadFailed; // critical error

    return Error::NoError;
}}

Error write_idx_block(const IdxFile& idx_file, int field, int time,
                      const core::Array<IdxBlockHeader>& block_headers,
                      const IdxBlock& block,
                      IN_OUT uint64_t* last_first_block_index, IN_OUT FILE** file)
{
    using namespace core;

    HANA_ASSERT(last_first_block_index != nullptr);
    HANA_ASSERT(file != nullptr);


    int bpf = idx_file.blocks_per_file;
    uint64_t block_index = block.hz_address >> idx_file.bits_per_block;
    uint64_t first_block_index = block_index - (block_index % bpf);
    uint64_t block_in_file = block_index - first_block_index;
    HANA_ASSERT(block_in_file < bpf);

    char bin_path[512]; // path to the binary file that stores the block
    get_file_name_from_hz(idx_file, time, first_block_index, STR_REF(bin_path));
    if (*last_first_block_index != first_block_index) { // open a new file
        *last_first_block_index = first_block_index;
        if (*file)
            fclose(*file);
        *file = fopen(bin_path, "wb");
        if (!*file)
            return Error::FileNotFound;
    }

    // write the actual data
    const IdxBlockHeader& header = block_headers[block_in_file];
    HANA_ASSERT(header.offset() != 0);
    fseek(*file, header.offset(), SEEK_SET);
    if (fwrite(block.data.ptr, block.data.bytes, 1, *file) != 1)
        return Error::BlockWriteFailed;

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
//TODO: remove this
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
    HANA_ASSERT(grid->data.bytes >= dxyz * sizeof(T));
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
        if (grid->extent.from <= second.to && second.from <= grid->extent.to) {
            stack[++top] = second;
        }
        if (grid->extent.from <= first.to && first.from <= grid->extent.to) {
            stack[++top] = first;
        }
    }
}
};

/** Copy data from an idx block to a rectilinear grid, assuming the samples in
both are in row-major order. output_from/to/stride describe the output grid in relation to the
entire domain. */
template <typename T>
struct put_block_to_grid {
void operator()(const IdxBlock& block, const core::Vector3i& output_from,
                const core::Vector3i& output_to, const core::Vector3i& output_stride,
                IN_OUT Grid* grid)
{
    core::Vector3i from, to;
    if (!intersect_grid(grid->extent, block.from, block.to, block.stride, from, to)) {
        return;
    }

    T* dst = reinterpret_cast<T*>(grid->data.ptr);
    T* src = reinterpret_cast<T*>(block.data.ptr);
    HANA_ASSERT(src && dst);
    // TODO: optimize this loop (parallelize?)
    core::Vector3i input_dims = (block.to - block.from) / block.stride + 1;
    uint64_t sx = input_dims.x, sxy = input_dims.x * input_dims.y;
    core::Vector3i output_dims = (output_to - output_from) / output_stride + 1;
    uint64_t dx = output_dims.x, dxy = output_dims.x * output_dims.y;
    core::Vector3i dd = block.stride / output_stride;
    for (int z = from.z, // loop variable
         k = (from.z - block.from.z) / block.stride.z, // index into the block's buffer
         zz = (from.z - output_from.z) / output_stride.z; // index into the grid's buffer
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

/** Copy data from a rectilinear grid to an idx block, assuming the samples in
both are in row-major order. Here we don't need to specify the input grid's
from/to/stride because most of the time (a subset of) the original grid is given. */
template <typename T>
struct put_grid_to_block {
void operator()(const Grid& grid, IN_OUT IdxBlock* block)
{
    using namespace core;
    Vector3i from, to;
    if (!intersect_grid(grid.extent, block->from, block->to, block->stride, from, to)) {
        return;
    }

    T* src = reinterpret_cast<T*>(grid.data.ptr);
    T* dst = reinterpret_cast<T*>(block->data.ptr);
    HANA_ASSERT(src && dst);
    // TODO: optimize this loop (parallelize?)
    Vector3i output_dims = (block->to - block->from) / block->stride + 1;
    uint64_t sx = output_dims.x, sxy = output_dims.x * output_dims.y;
    Vector3i input_dims = grid.extent.to - grid.extent.from + 1;
    uint64_t dx = input_dims.x, dxy = input_dims.x * input_dims.y;
    for (int z = from.z,
        k = (from.z - block->from.z) / block->stride.z; // index into the block's buffer
        z <= to.z; // loop variable and index into the grid's buffer
        z += block->stride.z, ++k) {
        for (int y = from.y,
            j = (from.y - block->from.y) / block->stride.y;
            y <= to.y;
            y += block->stride.y, ++j) {
            for (int x = from.x,
                i = (from.x - block->from.x) / block->stride.x;
                x <= to.x;
                x += block->stride.x, ++i) {
                uint64_t ijk = i + j * sx + k * sxy;
                uint64_t xyz = x + y * dx + z * dxy;
                dst[ijk] = src[xyz];
            }
        }
    }
}
};

// TODO: remove global vars like this one
core::FreelistAllocator<core::Mallocator> freelist;

Error read_idx_grid(const IdxFile& idx_file, int field, int time, int hz_level,
                    IN_OUT Grid* grid)
{
    grid->type = idx_file.fields[field].type;
    core::Vector3i from, to, stride;
    idx_file.get_grid(grid->extent, hz_level, from, to, stride);
    return read_idx_grid(idx_file, field, time, hz_level, from, to, stride, grid);
}

Error read_idx_grid(const IdxFile& idx_file, int field, int time, int hz_level,
                    const core::Vector3i& output_from, const core::Vector3i& output_to,
                    const core::Vector3i& output_stride, IN_OUT Grid* grid)
{
    // check the inputs
    if (!verify_idx_file(idx_file))
        return Error::InvalidIdxFile;
    if (field < 0 || field > idx_file.num_fields)
        return Error::FieldNotFound;
    if (time < idx_file.time.begin || time > idx_file.time.end)
        return Error::TimeStepNotFound;
    if (hz_level < 0 || hz_level > idx_file.get_max_hz_level())
        return Error::InvalidHzLevel;
    HANA_ASSERT(grid);
    if (!grid->extent.is_valid())
        return Error::InvalidVolume;
    if (!grid->extent.is_inside(idx_file.box))
        return Error::VolumeTooBig;
    HANA_ASSERT(grid->data.ptr);

    grid->type = idx_file.fields[field].type;

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
    get_block_addresses(idx_file, grid->extent, hz_level, &idx_blocks);

    // determine the most likely size of each block and use a FreeListAllocator
    // with this size to allocate actual data (not metadata) for the blocks.
    // some blocks can be smaller due to compression, and/or being near the
    // boundary
    size_t samples_per_block = (size_t)pow2[idx_file.bits_per_block];
    size_t block_size = idx_file.fields[field].type.bytes() * samples_per_block;
    if (freelist.max_size() != block_size)
        freelist.set_min_max_size(block_size / 2, block_size);

    FILE* file = nullptr;
    uint64_t last_first_block = (uint64_t)-1;

    // if the system has 8 cores (say with Hyperthreading), we use 16 threads.
    // 1024 is the upper limit for the number of threads.
    size_t num_thread_max = size_t(std::thread::hardware_concurrency());
    num_thread_max = min(num_thread_max * 2, (size_t)1024);
    std::thread threads[1024];

    /* read the blocks */
    for (size_t i = 0; i < idx_blocks.size(); i += num_thread_max) {
        int thread_count = 0;
        for (size_t j = 0; j < num_thread_max && i + j < idx_blocks.size(); ++j) {
            IdxBlock& block = idx_blocks[i + j];
            Error err = read_idx_block(idx_file, field, time, true, &last_first_block,
                                       &file, &block_headers, &block, freelist);
            if (err == Error::InvalidCompression || err == Error::BlockReadFailed)
                return err; // critical errors
            if (err == Error::BlockNotFound || err == Error::FileNotFound) {
                error = err;
                continue; // these are not critical errors (a block may not be saved yet)
            }
            if (block.compression != Compression::None)// TODO?
                return Error::CompressionUnsupported;
            if (block.format == Format::RowMajor) {
                ++thread_count;
                threads[j] = std::thread([&, block]() {
                    forward_functor<put_block_to_grid, int>(block.type.bytes(),
                        block, output_from, output_to, output_stride, grid);
                    mutex.lock();
                    freelist.deallocate(block.data);
                    mutex.unlock();
                });
            } else if (block.format == Format::Hz) {
                if (hz_level < idx_file.get_min_hz_level()) {
                    ++thread_count;
                    threads[j] = std::thread([&, block]() {
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
                        while (b.bytes < block.bytes && b.bytes < grid->data.bytes) {
                            // each iteration corresponds to one hz level,
                            // starting from 0 until min_hz_level - 1
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
                        mutex.unlock();
                    });
                }
                else { // for hz levels >= min hz level
                    ++thread_count;
                    threads[j] = std::thread([&](){
                        forward_functor<put_block_to_grid_hz, int>(block.type.bytes(),
                            idx_file.bit_string, idx_file.bits_per_block, block,
                            output_from, output_to, output_stride, grid);
                        mutex.lock();
                        freelist.deallocate(block.data);
                        mutex.unlock();
                    });
                }
            } else {
                error = Error::InvalidFormat;
            }
        }

        // wait for all the threads to finish before spawning new ones
        for (int j = 0; j < thread_count; ++j)
            threads[j].join();
        thread_count = 0;
    }

    if (file)
        fclose(file);

    return error;
}

Error read_idx_grid_inclusive(const IdxFile& idx_file, int field, int time,
                              int hz_level, IN_OUT Grid* grid)
{
    grid->type = idx_file.fields[field].type;
    core::Vector3i from, to, stride;
    idx_file.get_grid_inclusive(grid->extent, hz_level, from, to, stride);
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

//namespace {
//Error write_idx_headers()
//{
//}}

/* Write an IDX grid at one particular HZ level.
TODO: this function overlaps quite a bit with read_idx_grid. */
namespace {
Error write_idx_grid(const IdxFile& idx_file, int field, int time, int hz_level,
                     const Grid& grid, bool read)
{
    using namespace core;

    /* figure out which blocks touch this grid */
    Mallocator mallocator;
    Array<IdxBlock> idx_blocks(&mallocator);
    Array<IdxBlockHeader> block_headers(&mallocator);
    block_headers.resize(idx_file.blocks_per_file);
    get_block_addresses(idx_file, grid.extent, hz_level, &idx_blocks);

    size_t samples_per_block = (size_t)pow2[idx_file.bits_per_block];
    size_t block_size = idx_file.fields[field].type.bytes() * samples_per_block;
    if (freelist.max_size() != block_size)
        freelist.set_min_max_size(block_size / 2, block_size);

    FILE* file = nullptr;
    uint64_t last_first_block = (uint64_t)-1;

    size_t num_thread_max = size_t(std::thread::hardware_concurrency());
    num_thread_max = min(num_thread_max * 2, (size_t)1024);
    std::thread threads[1024];

    /* (read and) write the blocks */
    // TODO: write block header?
    for (size_t i = 0; i < idx_blocks.size(); i += num_thread_max) {
        int thread_count = 0;
        for (size_t j = 0; j < num_thread_max && i + j < idx_blocks.size(); ++j) {
            IdxBlock& block = idx_blocks[i + j];
            Error err = Error::NoError;
            if (read) {
                // read the headers
                uint64_t first_block = 0;
                int block_in_file = 0;
                get_first_block_in_file(block.hz_address, idx_file.bits_per_block,
                                        idx_file.blocks_per_file, &first_block,
                                        &block_in_file);
                char bin_path[512];
                get_file_name_from_hz(idx_file, time, first_block, STR_REF(bin_path));
                err = read_block_headers(&file, bin_path, field,
                                         idx_file.blocks_per_file, &block_headers);
                if (err.code != Error::NoError) {
                    err = read_idx_block(idx_file, field, time, &last_first_block,
                                         false, &file, &block_headers, &block, freelist);
                }
            }
            if (err == Error::HeaderNotFound) { // write the headers
                // tODO
            }

            if (err == Error::InvalidCompression || err == Error::BlockReadFailed)
                return err; // critical errors
            if (err == Error::BlockNotFound || err == Error::FileNotFound) {
                // TODO: handle compression
                block.data = freelist.allocate(block_size);
            }
            if (block.compression != Compression::None)
                return Error::CompressionUnsupported;
            if (block.format != Format::RowMajor)
                return Error::InvalidFormat;
            // copy the data from the grid over
            ++thread_count;
            /*threads[j] = std::thread([&, block]() {
                forward_functor<put_block_to_grid, int>(block.type.bytes(),
                    block, output_from, output_to, output_stride, grid);
                mutex.lock();
                freelist.deallocate(block.data);
                mutex.unlock();
            });*/
        }
    }

    // write to the blocks
    // write the blocks to disk
    return Error::NoError;
}}

Error write_idx_grid(const IdxFile& idx_file, int field, int time,
                     const Grid& grid, bool read)
{
    HANA_ASSERT(grid.data.ptr != nullptr);
    int min_hz = idx_file.get_min_hz_level();
    int max_hz = idx_file.get_max_hz_level();
    for (int l = min_hz; l <= max_hz; ++l) {
        Error err = write_idx_grid(idx_file, field, time, l, grid, read);
        if (err.code != Error::NoError)
            return err;
    }
    return Error::NoError;
}

void deallocate_memory()
{
    freelist.deallocate_all();
}

}}
