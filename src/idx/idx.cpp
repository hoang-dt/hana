#include "macros.h"
#include "array.h"
#include "bitops.h"
#include "constants.h"
#include "math.h"
#include "string.h"
#include "allocator.h"
#include "utils.h"
#include "utils.h"
#include "idx.h"
#include "idx_common.h"
#include "error.h"
#include "miniz.h"
#include <algorithm>
#include <array>
#include <iostream>
#include <thread>
#include <mutex>

namespace hana {

// We need exclusive access to the memory allocator for the blocks.
std::mutex mutex;

struct Tuple {
  uint64_t hz_address;
  int div_pos; /** The position in the bit string corresponding to the axis of division */
  Vector3i from;
  Vector3i to;
  uint64_t num_elems;
};

/** For example, if the bit string is xyxyxy, the current z address is
100100 (z level 2), and the bits per block is 2, then the div position (which is
the position of the bit that corresponds to the immediate axis of division) is 1,
corresponding to a division along y.
This is the most significant bit among the bits dedicated to the current block.*/
//TODO: remove this
int dividing_pos(const StringRef bit_string, int bits_per_block, int hz_level)
{
  int z_level = static_cast<int>(bit_string.size) - hz_level;
  int div_pos = static_cast<int>(bit_string.size) - (z_level + 1 + bits_per_block);
  return max(div_pos, 0);
}

/** Copy data from an idx block to a rectilinear grid, assuming the samples in
the block is in hz order, and the samples in the grid is in row-major order.
We use the fast stack algorithm (see Brian Summa's PhD thesis).
TODO: One optimization would be to build a look up table for a small number of
samples and stop the recursion before it gets to a single sample. */
template <typename T>
struct put_block_to_grid_hz {
void operator()(
  const StringRef bit_string, int bits_per_block,
  const IdxBlock& block, const Vector3i& output_from, const Vector3i& output_to, const Vector3i& output_stride,
  IN_OUT Grid* grid)
{
  HANA_ASSERT(block.hz_level <= bit_string.size);

  if (!(grid->extent.from <= block.to) || !(block.from <= grid->extent.to)) {
    return;
  }

  const int stack_size = 65;
  Tuple stack[stack_size];
  int top = 0;
  stack[top] = Tuple{ block.hz_address,
            dividing_pos(bit_string, bits_per_block, block.hz_level),
            block.from, block.to, block.num_samples() };
  T* dst = reinterpret_cast<T*>(grid->data.ptr);
  T* src = reinterpret_cast<T*>(block.data.ptr);

  Vector3i output_dims = (output_to - output_from ) / output_stride + 1;
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
      Vector3i coord = (top_tuple.from - output_from) / output_stride;
      uint64_t xyz = coord.x + coord.y * dx + coord.z * dxy;
      uint64_t ijk = top_tuple.hz_address - block.hz_address;
      dst[xyz] = src[ijk];
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
void operator()(
  const IdxBlock& block, const Vector3i& output_from,
  const Vector3i& output_to, const Vector3i& output_stride,
  IN_OUT Grid* grid)
{
  Vector3i from, to;
  if (!intersect_grid(grid->extent, block.from, block.to, block.stride, &from, &to)) {
    return;
  }

  T* dst = reinterpret_cast<T*>(grid->data.ptr);
  T* src = reinterpret_cast<T*>(block.data.ptr);
  HANA_ASSERT(src && dst);
  // TODO: optimize this loop (parallelize?)
  Vector3i input_dims = (block.to - block.from) / block.stride + 1;
  uint64_t sx = input_dims.x, sxy = input_dims.x * input_dims.y;
  Vector3i output_dims = (output_to - output_from) / output_stride + 1;
  uint64_t dx = output_dims.x, dxy = output_dims.x * output_dims.y;
  Vector3i dd = block.stride / output_stride;
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

// TODO: remove global vars like this one
FreelistAllocator<Mallocator> freelist;

Error read_idx_grid(
  const IdxFile& idx_file, int field, int time, int hz_level, IN_OUT Grid* grid)
{
  grid->type = idx_file.fields[field].type;
  Vector3i from, to, stride;
  idx_file.get_grid(grid->extent, hz_level, from, to, stride);
  return read_idx_grid(idx_file, field, time, hz_level, from, to, stride, grid);
}

Error read_idx_grid(
  const IdxFile& idx_file, int field, int time, int hz_level,
  const Vector3i& output_from, const Vector3i& output_to,
  const Vector3i& output_stride, IN_OUT Grid* grid)
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
    freelist.set_min_max_size(block_size / 2, std::max(sizeof(void*), block_size));

  FILE* file = nullptr;
  Error error;
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
      Error err = read_idx_block(
        idx_file, field, time, true, &last_first_block,&file, &block_headers, &block, freelist);
      if (err == Error::InvalidCompression || err == Error::BlockReadFailed) {
        error = err;
        goto WAIT;
      }
      if (err == Error::BlockNotFound || err == Error::FileNotFound) {
        error = err;
        continue; // these are not critical errors (a block may not be saved yet)
      }
      if (block.compression == Compression::Zip) {
        mutex.lock(); MemBlockChar dst = freelist.allocate(block_size); mutex.unlock();
        uLong dest_len = static_cast<uLong>(dst.bytes);
        Bytef* dest = (Bytef*)dst.ptr;
        Bytef* src = (Byte*)block.data.ptr;
        uncompress(dest, &dest_len, src, static_cast<uLong>(block.data.bytes));
        std::swap(block.data, dst);
        block.bytes = block.data.bytes;
        mutex.lock(); freelist.deallocate(dst); mutex.unlock();
      }
      else if (block.compression != Compression::None) {
        error = Error::CompressionUnsupported;
        goto WAIT;
      }
      if (block.format == Format::RowMajor) {
        threads[thread_count++] = std::thread([&, block]() {
          forward_functor<put_block_to_grid, int>(
            block.type.bytes(), block, output_from, output_to, output_stride, grid);
          mutex.lock(); freelist.deallocate(block.data); mutex.unlock();
        });
      } else if (block.format == Format::Hz) {
        if (hz_level < idx_file.get_min_hz_level()) {
          threads[thread_count++] = std::thread([&, block]() {
            // here we break up the first idx block into multiple "virtual"
            // blocks, each consisting of only samples in one hz level
            IdxBlock b = block;
            b.bytes = b.type.bytes();
            HANA_ASSERT(b.hz_address == 0);
            b.hz_level = 0;
            b.data.bytes = b.bytes;
            b.from = b.to = Vector3i(0, 0, 0);
            b.stride = get_intra_level_strides(idx_file.bit_string, b.hz_level);
            uint64_t old_bytes = 0;
            uint64_t old_hz = 1;
            while (b.bytes < block.bytes && b.hz_level <= hz_level) {
              // each iteration corresponds to one hz level,
              // starting from 0 until min_hz_level - 1
              forward_functor<put_block_to_grid_hz, int>(
                b.type.bytes(), idx_file.bit_string, idx_file.bits_per_block, b,
                output_from, output_to, output_stride, grid);
              ++b.hz_level;
              b.data.ptr = b.data.ptr + b.bytes;
              b.bytes += old_bytes;
              old_bytes = b.bytes;
              b.data.bytes = b.bytes;
              b.hz_address += old_hz;
              old_hz = b.hz_address;
              if (b.hz_level <= hz_level) {
                b.from = get_first_coord(idx_file.bit_string, b.hz_level);
                b.stride = get_intra_level_strides(idx_file.bit_string, b.hz_level);
                b.to = get_last_coord(idx_file.bit_string, b.hz_level);
              }
            }

            mutex.lock(); freelist.deallocate(block.data); mutex.unlock();
          });
        }
        else { // for hz levels >= min hz level
          threads[thread_count++] = std::thread([&](){
            forward_functor<put_block_to_grid_hz, int>(
              block.type.bytes(), idx_file.bit_string, idx_file.bits_per_block, block,
              output_from, output_to, output_stride, grid);
            mutex.lock(); freelist.deallocate(block.data); mutex.unlock();
          });
        }
      } else {
        error = Error::InvalidFormat;
        goto WAIT;
      }
    }

    // wait for all the threads to finish before spawning new ones
WAIT:
    for (int j = 0; j < thread_count; ++j) {
      threads[j].join();
    }
    thread_count = 0;
  }

  if (file) {
    fclose(file);
  }

  return error;
}

Error read_idx_grid_inclusive(
  const IdxFile& idx_file, int field, int time, int hz_level, IN_OUT Grid* grid)
{
  grid->type = idx_file.fields[field].type;
  Vector3i from, to, stride;
  idx_file.get_grid_inclusive(grid->extent, hz_level, from, to, stride);
  Error err = read_idx_grid(idx_file, field, time, idx_file.get_min_hz_level()-1,
                from, to, stride, grid);
  if (err.code != Error::NoError) {
    return err;
  }
  int min_hz = idx_file.get_min_hz_level();
  for (int l = min_hz; l <= hz_level; ++l) {
    err = read_idx_grid(idx_file, field, time, l, from, to, stride, grid);
    if (err.code!=Error::NoError && err.code!=Error::BlockNotFound && err.code!=Error::FileNotFound) {
      return err;
    }
  }
  return Error::NoError;
}

void deallocate_memory()
{
  freelist.deallocate_all();
}

}
