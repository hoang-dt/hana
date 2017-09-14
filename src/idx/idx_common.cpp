#include "array.h"
#include "macros.h"
#include "idx.h"
#include "idx_common.h"
#include "idx_file.h"
#include "utils.h"
#include <mutex>
#include <iostream>

namespace hana {

extern std::mutex mutex;

/** Get the name of the binary file that contains a given hz address.
For example, if the hz address is 0100'0101'0010'1100 and the file name template
is ./%02x/%01x/%01x.bin, then the binary file name is ./45/2/c.bin, assuming
there is no time step template (4 = 0100, 5 = 0101, 2 = 0010, and c = 1100).
The time step template, if present, will be added to the beginning of the path. */
void get_file_name_from_hz(
  const IdxFile& idx_file, int time, uint64_t hz_address, OUT StringRef& file_name)
{
  // TODO: we can optimize a little more by "pre-building" the static parts
  // of the file name and just fill in the templated parts
  const char hex_digits[] = "0123456789abcdef";

  int pos = 0; // keep track of where we are in the output string

  // if the path specified in the idx file is relative (to the idx file itself)
  // , add the absolute path to the .idx file itself
  if (idx_file.filename_template.head.is_relative()) {
    pos += snprintf(file_name.ptr + pos, file_name.size - pos, "%s/", idx_file.absolute_path.path_string().cptr);
  }

  // add the head of the path
  const FileNameTemplate& name_template = idx_file.filename_template;
  if (name_template.head.num_components() > 0) {
    pos += snprintf(file_name.ptr + pos, file_name.size - pos, "%s/", name_template.head.path_string().cptr);
  }
  // add the time template
  pos += snprintf(file_name.ptr + pos, file_name.size - pos, idx_file.time.template_, time);

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
  int ext = snprintf(file_name.ptr + pos, file_name.size - pos, "%s", name_template.ext);
  file_name.size = pos + ext;
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
}

/** Given a 3D extend (a volume), get the (sorted) list of idx block addresses
that intersect this volume, at a given hz level. The addresses are in hz space.
The input volume (vol) should be inclusive at both ends. */
void get_block_addresses(
  const IdxFile& idx_file, const Volume& vol, int hz_level, OUT Array<IdxBlock>* idx_blocks)
{
  HANA_ASSERT(hz_level <= idx_file.bit_string.size);
  HANA_ASSERT(vol.is_valid());

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
        block.from = Vector3i(x, y, z);
        if (first_block) { // for the first block, we combine all the hz levels in the block
          // (from 0 to min hz level - 1), and the resulting grid has the same strides as those of
          // the next hz level.
          block.stride = get_intra_level_strides(bit_string, hz_level + 1);
        }
        else {
          block.stride = get_intra_level_strides(bit_string, hz_level);
        }
        block.to = block.from + stride - block.stride;
        Vector3i last_coord = get_last_coord(bit_string, hz_level);
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

/** Given a block's hz address, compute the hz address of the first block in the
file, and the block's index in the file. */
void get_first_block_in_file(
  uint64_t block, int bits_per_block, int blocks_per_file,
  OUT uint64_t* first_block, OUT int* block_in_file)
{
  // block index is the rank of this block (0th block, 1st block, 2nd,...)
  uint64_t block_id = block >> bits_per_block;
  *first_block = block_id - (block_id % blocks_per_file);
  *block_in_file = static_cast<int>(block_id - *first_block);
  HANA_ASSERT(*block_in_file < blocks_per_file);
}

/** Read an IDX block out of a file. Beside returning the data in the block, this
function returns the HZ index of the last first block in a file read, as well as
the last file read. This is so that the next call to the function does not open
the same file. The file is returned so that after the last call, the caller can
close the last file opened. */
// TOOD: remove the read_headers param
Error read_idx_block(
  const IdxFile& idx_file, int field, bool open_new_file, uint64_t block_in_file,
  IN_OUT FILE** file, IN_OUT Array<IdxBlockHeader>* block_headers, IN_OUT IdxBlock* block, Allocator& alloc)
{
  HANA_ASSERT(file != nullptr);
  HANA_ASSERT(block_headers != nullptr);
  HANA_ASSERT(block != nullptr);

  Error error = Error::NoError;
  if (open_new_file) { // open a new file
    // read all headers
    if (fseek(*file, sizeof(IdxFileHeader) + sizeof(IdxBlockHeader) * idx_file.blocks_per_file * field, SEEK_SET)) {
      return Error::HeaderNotFound;
    }
    if (fread(&(*block_headers)[0], sizeof(IdxBlockHeader), idx_file.blocks_per_file, *file) != idx_file.blocks_per_file) {
      return Error::HeaderNotFound;
    }
    for (size_t i = 0; i < block_headers->size(); ++i) {
      (*block_headers)[i].swap_bytes();
    }
  }

  IdxBlockHeader& header = (*block_headers)[block_in_file];
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
  mutex.lock(); block->data = alloc.allocate(block->bytes); mutex.unlock();
  fseek(*file, block_offset, SEEK_SET);
  if (fread(block->data.ptr, block->bytes, 1, *file) != 1) {
    return Error::BlockReadFailed; // critical error
  }

  return Error::NoError;
}

}
