#include "allocator.h"
#include "array.h"
#include "macros.h"
#include "math.h"
#include "error.h"
#include "idx.h"
#include "idx_common.h"
#include <cstdint>
#include <thread>

namespace hana {

extern FreelistAllocator<Mallocator> freelist;

Error write_idx_block(
  const IdxFile& idx_file, int field, int time, const IdxBlock& block, bool read,
  IN_OUT uint64_t* last_first_block_index, IN_OUT FILE** file)
{
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
    if (*file) {
      fclose(*file);
    }
    *file = fopen(bin_path, "rb+");
    if (!*file) {
      return Error::FileNotFound;
    }
  }

  // write the actual data
  IdxBlockHeader header;
  if (read) {
    //read_idx_block(
    //  idx_file, field, time,
    //);
    Error err = read_one_block_header(file, bin_path, field, bpf, block_in_file, &header);
    if (err.code != Error::NoError && err.code != Error::HeaderNotFound) {
      return err;
    }
    if (header.offset() == 0) { // the block has not been written yet, write to the end of the file
      fseek(*file, 0, SEEK_END);
    }
    else { // the block exists on file, but probably not complete, we will rewrite it
      fseek(*file, header.offset(), SEEK_SET);
    }
  }
  else { // do not read before write, write to the end of the file
    fseek(*file, 0, SEEK_END);
  }
  if (fwrite(block.data.ptr, block.data.bytes, 1, *file) != 1) {
    return Error::BlockWriteFailed;
  }

  return Error::NoError;
}

/* Write an IDX grid at one particular HZ level.
TODO: this function overlaps quite a bit with read_idx_grid. */
Error write_idx_grid(
  const IdxFile& idx_file, int field, int time, int hz_level, const Grid& grid)
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
  if (!grid.extent.is_valid())
    return Error::InvalidVolume;
  if (!grid.extent.is_inside(idx_file.box))
    return Error::VolumeTooBig;
  HANA_ASSERT(grid.data.ptr);

  /* figure out which blocks touch this grid */
  Mallocator mallocator;
  Array<IdxBlock> idx_blocks(&mallocator);
  Array<IdxBlockHeader> block_headers(&mallocator);
  block_headers.resize(idx_file.blocks_per_file);
  get_block_addresses(idx_file, grid.extent, hz_level, &idx_blocks);

  size_t samples_per_block = (size_t)pow2[idx_file.bits_per_block];
  size_t block_size = idx_file.fields[field].type.bytes() * samples_per_block;
  if (freelist.max_size() != block_size) {
    freelist.set_min_max_size(block_size / 2, std::max(sizeof(void*), block_size));
  }

  FILE* file = nullptr;
  Error error = Error::NoError;
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
      Error err = read_idx_block(
        idx_file, field, time, true, &last_first_block, &file, &block_headers, &block, freelist);
      if (err == Error::HeaderNotFound) { // populate the header
        // tODO
      }
      else if (err == Error::InvalidCompression || err == Error::BlockReadFailed) {
        return err; // critical errors
      }
      else if (err == Error::BlockNotFound || err == Error::FileNotFound) {
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
}

/** Copy data from a rectilinear grid to an idx block, assuming the samples in
both are in row-major order. Here we don't need to specify the input grid's
from/to/stride because most of the time (a subset of) the original grid is given. */
template <typename T>
struct put_grid_to_block {
void operator()(const Grid& grid, IN_OUT IdxBlock* block)
{
  Vector3i from, to;
  if (!intersect_grid(grid.extent, block->from, block->to, block->stride, &from, &to)) {
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

Error write_idx_grid(
  const IdxFile& idx_file, int field, int time, const Grid& grid, bool read)
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

}
