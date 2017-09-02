#pragma once
/**\file
This file defines useful macros.
*/

//TODO: add FORCE_INLINE
#define ARRAY_SIZE(x) sizeof(x) / sizeof(*(x))
#define COPY_ARRAY(dst, src) \
    HANA_ASSERT(ARRAY_SIZE(src) == ARRAY_SIZE(dst)); \
    for (size_t i = 0; i < ARRAY_SIZE(src); ++i) { \
        dst[i] = src[i]; \
    }
#define BIT_SIZE(x) sizeof(x) * 8

#define OUT // useful to annotate output arguments
#define IN_OUT

#define STRING_EQUAL(s, t) strcmp((s), (t)) == 0

/** Create an anonymous variable on the stack. */
#define HANA_CONCATENATE_IMPL(s1, s2) s1##s2
#define HANA_CONCATENATE(s1, s2) HANA_CONCATENATE_IMPL(s1, s2)
#ifdef __COUNTER__
    #define HANA_ANONYMOUS_VARIABLE(str) HANA_CONCATENATE(str, __COUNTER__)
#else
    #define HANA_ANONYMOUS_VARIABLE(str) HANA_CONCATENATE(str, __LINE__)
#endif

/** Enable support for large file with fseek. */
#ifdef  _MSC_VER
    #include <cstdio>
    #define fseek _fseeki64
    #define ftell _ftelli64
#elif defined __linux__ || defined __APPLE__
    #define _FILE_OFFSET_BITS 64
    #include <cstdio>
    #define fseek fseeko
    #define ftell ftello
#endif

/** Macros to swap bytes in a multi-byte value, to convert from big-endian data
to little-endian data and vice versa. These are taken from the Boost library.*/
#ifndef __has_builtin
    #define __has_builtin(x) 0 // Compatibility with non-clang compilers
#endif
#if defined(_MSC_VER)
    #include <cstdlib>
    #define HANA_BYTE_SWAP_2(x) _byteswap_ushort(x)
    #define HANA_BYTE_SWAP_4(x) _byteswap_ulong(x)
    #define HANA_BYTE_SWAP_8(x) _byteswap_uint64(x)
#elif (defined(__clang__) && __has_builtin(__builtin_bswap32) && __has_builtin(__builtin_bswap64)) \
   || (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)))
    #if (defined(__clang__) && __has_builtin(__builtin_bswap16)) \
      || (defined(__GNUC__) &&(__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)))
        #define HANA_BYTE_SWAP_2(x) __builtin_bswap16(x)
    #else
        #define HANA_BYTE_SWAP_2(x) __builtin_bswap32((x) << 16)
    #endif
    #define HANA_BYTE_SWAP_4(x) __builtin_bswap32(x)
    #define HANA_BYTE_SWAP_8(x) __builtin_bswap64(x)
#elif defined(__linux__)
    #include <byteswap.h>
    #define HANA_BYTE_SWAP_2(x) bswap_16(x)
    #define HANA_BYTE_SWAP_4(x) bswap_32(x)
    #define HANA_BYTE_SWAP_8(x) bswap_64(x)
#endif
