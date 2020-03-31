// TODO: implement a sliding-window approach, since a file mapping view
// can be limited in size
// TODO: add MAP_NORESERVE on Linux

#pragma once

#include "types.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

// TODO: create a mapping that is not backed by a file

enum class mmap_err_code : int { 
  NoError, FileCreateFailed, FileCloseFailed, MappingFailed, MapViewFailed, AllocateFailed, FlushFailed, SyncFailed, UnmapFailed };

namespace hana {

enum class map_mode { Read, Write };

#if defined(_WIN32)
using file_handle = HANDLE;
#elif defined(__linux__) || defined(__APPLE__)
using file_handle = int;
#endif

struct mmap_file {
  file_handle File;
  file_handle FileMapping;
  map_mode Mode;
  MemBlock<char> Buf;
};

mmap_err_code OpenFile(mmap_file* MMap, const char* Name, map_mode Mode);
mmap_err_code MapFile(mmap_file* MMap, int64_t Bytes = 0);
mmap_err_code FlushFile(mmap_file* MMap, char* Start = nullptr, int64_t Bytes = 0);
mmap_err_code SyncFile(mmap_file* MMap);
mmap_err_code UnmapFile(mmap_file* MMap);
mmap_err_code CloseFile(mmap_file* MMap);
template <typename t> void Write(mmap_file* MMap, const t* Data);
template <typename t> void Write(mmap_file* MMap, const t* Data, int64_t Size);
template <typename t> void Write(mmap_file* MMap, t Val);

} // namespace hana

#include "string.h"

namespace hana {

template <typename t> void
Write(mmap_file* MMap, const t* Data, int64_t Size) {
  memcpy(MMap->Buf.ptr + MMap->Buf.bytes, Data, Size);
  MMap->Buf.bytes += Size;
}

template <typename t> void
Write(mmap_file* MMap, const t* Data) {
  Write(MMap, Data, sizeof(t));
}

template <typename t> void
Write(mmap_file* MMap, t Val) {
  Write(MMap, &Val, sizeof(t));
}

} // namespace hana

