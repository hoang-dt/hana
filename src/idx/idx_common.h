#pragma once

#include "macros.h"
#include <cstdint>

class IdxFile;
class StringRef;

namespace hana {
  void get_file_name_from_hz(
    const IdxFile& idx_file, int time, uint64_t hz_address, OUT StringRef file_name);

  void get_block_addresses(
    const IdxFile& idx_file, const Volume& vol, int hz_level, OUT Array<IdxBlock>* idx_blocks);

  void get_first_block_in_file(
    uint64_t block, int bits_per_block, int blocks_per_file,
    OUT uint64_t* first_block, OUT int* block_in_file);

  Error read_idx_block(
    const IdxFile& idx_file, int field, int time, bool open_new_file, uint64_t block_in_file,
    IN_OUT FILE** file, IN_OUT Array<IdxBlockHeader>* block_headers, IN_OUT IdxBlock* block, Allocator& alloc);
}
