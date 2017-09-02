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

  Error read_all_block_headers(
    IN_OUT FILE** file, const char* bin_path, int field, int blocks_per_file,
    IN_OUT Array<IdxBlockHeader>* headers);

  Error read_idx_block(
    const IdxFile& idx_file, int field, int time, bool read_headers,
    IN_OUT uint64_t* last_first_block, IN_OUT FILE** file,
    IN_OUT Array<IdxBlockHeader>* block_headers,
    IN_OUT IdxBlock* block, Allocator& alloc);

  Error read_one_block_header(
    IN_OUT FILE** file, const char* bin_path, int field, int blocks_per_file, uint64_t block_in_file,
    OUT IdxBlockHeader* header);
}
