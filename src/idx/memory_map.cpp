#include "memory_map.h"

#if defined(__APPLE__)
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

inline int // TODO: move to file_utils.cpp
fallocate(int fd, int mode, off_t offset, off_t len) {
  (void)mode; // TODO: what to do with mode?
  //int fd = PR_FileDesc2NativeHandle(aFD);
  fstore_t store = { F_ALLOCATECONTIG, F_PEOFPOSMODE, offset, len, 0 };
  // Try to get a continous chunk of disk space
  int ret = fcntl(fd, F_PREALLOCATE, &store);
  if (-1 == ret) {
    // OK, perhaps we are too fragmented, allocate non-continuous
    store.fst_flags = F_ALLOCATEALL;
    ret = fcntl(fd, F_PREALLOCATE, &store);
    if (-1 == ret)
      return -1;
  }
  if (0 == ftruncate(fd, len))
    return 0;
  return -1;
}
#endif

namespace hana {

mmap_err_code
OpenFile(mmap_file* MMap, const char* Name, map_mode Mode) {
#if defined(_WIN32)
  MMap->File =
    CreateFileA(Name,
                Mode == map_mode::Read ? GENERIC_READ
                          : GENERIC_READ | GENERIC_WRITE,
                0,
                NULL,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL);
  if (MMap->File == INVALID_HANDLE_VALUE)
    return mmap_err_code::FileCreateFailed;
#elif defined(__linux__) || defined(__APPLE__)
  MMap->File = open(Name, Mode == map_mode::Read ? O_RDONLY
                                                 : O_RDWR | O_CREAT | O_TRUNC, 0600);
  if (MMap->File == -1)
    return mmap_err_code::FileCreateFailed;
#endif
  MMap->Mode = Mode;
  return mmap_err_code::NoError;
}

/* Size is only used when Mode is Write or ReadWrite */
mmap_err_code
MapFile(mmap_file* MMap, int64_t Bytes) {
#if defined(_WIN32)
  LARGE_INTEGER FileSize{{0, 0}};
  if (!GetFileSizeEx(MMap->File, &FileSize) || Bytes != 0)
    FileSize.QuadPart = Bytes;
  MMap->FileMapping =
    CreateFileMapping(MMap->File,
                      NULL,
                      MMap->Mode == map_mode::Read ? PAGE_READONLY
                                                   : PAGE_READWRITE,
                      FileSize.HighPart,
                      FileSize.LowPart,
                      0);
  if (MMap->FileMapping == NULL)
    return mmap_err_code::MappingFailed;

  LPVOID MapAddress =
    MapViewOfFile(MMap->FileMapping,
                  MMap->Mode == map_mode::Read ? FILE_MAP_READ
                    : MMap->Mode == map_mode::Write ? FILE_MAP_WRITE
                      : FILE_MAP_ALL_ACCESS,
                  0,
                  0,
                  0);
  if (MapAddress == NULL)
    return mmap_err_code::MapViewFailed;
  MMap->Buf.ptr = (char*)MapAddress;
  MMap->Buf.bytes = FileSize.QuadPart;
#elif defined(__linux__) || defined(__APPLE__)
  size_t FileSize;
  struct stat Stat;
  if (Bytes != 0)
    FileSize = Bytes;
  else if (fstat(MMap->File, &Stat) == 0)
    FileSize = Stat.st_size;
  if (MMap->Mode == map_mode::Write)
    // TODO: only works on Linux, not Mac OS X
    if (fallocate(MMap->File, 0, 0, FileSize) == -1)
      return mmap_err_code::AllocateFailed;
  void* MapAddress =
    mmap(0,
         FileSize,
         MMap->Mode == map_mode::Read ? PROT_READ : PROT_READ | PROT_WRITE,
         MAP_SHARED,
         MMap->File,
         0);
  if (MapAddress == MAP_FAILED)
    return mmap_err_code::MapViewFailed;
  MMap->Buf.ptr = (char*)MapAddress;
  MMap->Buf.bytes = FileSize;
#endif
  return mmap_err_code::NoError;
}

/* (Non-blocking) flush dirty pages */
mmap_err_code
FlushFile(mmap_file* MMap, char* Start, int64_t Bytes) {
#if defined(_WIN32)
  bool Result = Start ? FlushViewOfFile(Start, (size_t)Bytes)
                      : FlushViewOfFile(MMap->Buf.ptr, (size_t)Bytes);
  if (!Result)
    return mmap_err_code::FlushFailed;
#elif defined(__linux__) || defined(__APPLE__)
  int Result;
  if (Start) {
    Result = Bytes ? msync(Start, Bytes, MS_ASYNC)
                   : msync(Start, MMap->Buf.bytes - (Start - MMap->Buf.ptr), MS_ASYNC);
  } else {
    Result = Bytes ? msync(MMap->Buf.ptr, Bytes, MS_ASYNC)
                   : msync(MMap->Buf.ptr, MMap->Buf.bytes, MS_ASYNC);
  }
  if (Result == -1)
    return mmap_err_code::FlushFailed;
#endif
  return mmap_err_code::NoError;
}

/* (Blocking) flush file metadata and ensure file is physically written.
 * Meant to be called at the very end */
mmap_err_code
SyncFile(mmap_file* MMap) {
#if defined(_WIN32)
  if (!FlushFileBuffers(MMap->File))
    return mmap_err_code::SyncFailed;
#elif defined(__linux__) || defined(__APPLE__)
  if (msync(MMap->Buf.ptr, MMap->Buf.bytes, MS_SYNC) == -1)
    return mmap_err_code::SyncFailed;
#endif
  return mmap_err_code::NoError;
}

/* Unmap the file and close all handles */
mmap_err_code
UnmapFile(mmap_file* MMap) {
#if defined(_WIN32)
  if (!UnmapViewOfFile(MMap->Buf.ptr)) {
    CloseHandle(MMap->FileMapping);
    return mmap_err_code::UnmapFailed;
  }
  if (!CloseHandle(MMap->FileMapping))
    return mmap_err_code::UnmapFailed;
#elif defined(__linux__) || defined(__APPLE__)
  if (munmap(MMap->Buf.ptr, MMap->Buf.bytes) == -1)
    return mmap_err_code::UnmapFailed;
#endif
  MMap->Buf.ptr = nullptr; MMap->Buf.bytes = 0;
  return mmap_err_code::NoError;
}

/* Close the file */
mmap_err_code
CloseFile(mmap_file* MMap) {
#if defined(_WIN32)
  if (!CloseHandle(MMap->File))
    return mmap_err_code::FileCloseFailed;
#elif defined(__linux__) || defined(__APPLE__)
  if (close(MMap->File) == -1)
    return mmap_err_code::FileCloseFailed;
#endif
  MMap->Buf.ptr = nullptr; MMap->Buf.bytes = 0;
  return mmap_err_code::NoError;
}

} // namespace mg

