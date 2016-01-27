#pragma once

#define BITS_PER_BYTES 8

#include <cstddef>
#include <cstdint>

namespace hana { namespace core {

const size_t size_max_ = (size_t)-1;

const int power10_[] = { 1,
                         10,
                         100,
                         1000,
                         10000,
                         100000,
                         1000000,
                         10000000,
                         100000000 };

const int power2_[] = { 1,
                        2,
                        4,
                        8,
                        16,
                        32,
                        64,
                        128,
                        256,
                        512,
                        1024,
                        2048,
                        4096,
                        8192,
                        16384,
                        32768,
                        65536,
                        131072,
                        262144,
                        524288,
                        1048576,
                        2097152,
                        4194304,
                        8388608,
                        16777216,
                        33554432,
                        67108864,
                        134217728,
                        268435456,
                        536870912,
                        1073741824 };

}}
