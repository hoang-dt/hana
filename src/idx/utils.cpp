#include "idx_file.h"
#include "utils.h"
#include "bitops.h"
#include "math.h"

namespace hana {

void guess_bit_string(const Vector3i& dims, OUT StringRef& bit_string)
{
    Vector3i power_2_dims;
    power_2_dims.x = pow_greater_equal(2, dims.x);
    power_2_dims.y = pow_greater_equal(2, dims.y);
    power_2_dims.z = pow_greater_equal(2, dims.z);
    size_t size = 0;
    char buffer[detail::IdxFileBase::num_bits_max];
    while (power_2_dims.x > 1 || power_2_dims.y > 1 || power_2_dims.z > 1) {
        int max = std::max(power_2_dims.z, std::max(power_2_dims.x, power_2_dims.y));
        if (max == power_2_dims.z) {
            power_2_dims.z /= 2;
            buffer[size++] = '2';
        }
        else if (max == power_2_dims.y) {
            power_2_dims.y /= 2;
            buffer[size++] = '1';
        }
        else {
            power_2_dims.x /= 2;
            buffer[size++] = '0';
        }
    }
    bit_string.size = size;
    HANA_ASSERT(size > 0);

    for (size_t i = 0; i < size; ++i) {
        bit_string[i] = buffer[i];
    }
}


Vector3i get_last_coord(StringRef bit_string, int hz_level)
{
    HANA_ASSERT(hz_level >= 0 && hz_level <= bit_string.size);

    if (hz_level == 0) {
        return Vector3i(0, 0, 0);
    }

    int pos = hz_level - 1; // the position of the right-most 1 bit in the bit string
    int size = static_cast<int>(bit_string.size);
    Vector3i count(0, 0, 0);
    for (int i = size - 1; i > pos; --i) {
        if (bit_string[i] == '0') {
            ++count.x;
        }
        else if (bit_string[i] == '1') {
            ++count.y;
        }
        else if (bit_string[i] == '2') {
            ++count.z;
        }
    }
    Vector3i coord(0, 0, 0);
    for (int i = pos; i >= 0; --i) {
        if (bit_string[i] == '0') {
            coord.x += pow2[count.x++];
        }
        else if (bit_string[i] == '1') {
            coord.y += pow2[count.y++];
        }
        else if (bit_string[i] == '2') {
            coord.z += pow2[count.z++];
        }
    }
    return coord;
}

Vector3i get_first_coord(StringRef bit_string, int hz_level)
{
    HANA_ASSERT(hz_level >= 0 && hz_level <= bit_string.size);

    if (hz_level == 0) {
        return Vector3i(0, 0, 0);
    }

    int pos = hz_level - 1; // the position of the right-most 1 bit in the bit string
    // count the number of "bits" that is the same with the one at position pos
    int count = 0;
    char c = bit_string[pos];
    for (size_t i = pos + 1; i < bit_string.size; ++i) {
        if (bit_string[i] == c) {
            ++count;
        }
    }
    // raise the corresponding coordinate to the appropriate power of 2 (the other
    // 2 coordinates are 0)
    Vector3i coord(0, 0, 0);
    if (c == '0') {
        coord.x = pow2[count];
    }
    else if (c == '1') {
        coord.y = pow2[count];
    }
    else if (c == '2') {
        coord.z = pow2[count];
    }
    return coord;
}

Vector3i get_intra_level_strides(StringRef bit_string, int hz_level)
{
    // count the number of x, y, z in the least significant (z_level + 1) bits
    // in the bit_string
    int z_level = static_cast<int>(bit_string.size) - hz_level;
    int len = z_level + 1;
    return get_strides(bit_string, len);
}

Vector3i get_inter_block_strides(StringRef bit_string, int hz_level, int bits_per_block)
{
    HANA_ASSERT(bit_string.size >= hz_level);
    // count the number of x, y, z in the least significant
    // (z_level + bits_per_block + 1) bits in the bit_string
    int len = static_cast<int>(bit_string.size) - hz_level + bits_per_block + 1;
    // len can get bigger than bit_string.size if the input hz_level is smaller
    // than the minimum hz level
    return get_strides(bit_string, len);
}

void get_block_grid(StringRef bit_string, uint64_t block_number, int bits_per_block,
                    OUT Vector3i& from, OUT Vector3i& to, OUT Vector3i& stride)
{
    uint64_t first_sample_hz = block_number << bits_per_block;
    from = hz_to_xyz(bit_string, first_sample_hz);
    uint64_t last_sample_hz = first_sample_hz + (1ull << bits_per_block) - 1;
    to = hz_to_xyz(bit_string, last_sample_hz);
    // NOTE: for the first block (first_sample_hz == 0), we increase the level of the last
    // sample by one to correctly compute the strides (because this block contains samples
    // from multiple levels)
    int last_sample_level = (first_sample_hz == 0) ? (bits_per_block + 1)
                                                   : (log_int(2, first_sample_hz) + 1);
    stride = get_intra_level_strides(bit_string, last_sample_level);
}

Vector3i get_strides(StringRef bit_string, int len)
{
    HANA_ASSERT(len >= 0);
    Vector3i stride(0, 0, 0);
    size_t start = max(static_cast<int>(bit_string.size) - len, 0);
    for (size_t i = start; i < bit_string.size; ++i) {
        if (bit_string[i] == '0') {
            ++stride.x;
        }
        else if (bit_string[i] == '1') {
            ++stride.y;
        }
        else if (bit_string[i] == '2') {
            ++stride.z;
        }
    }
    if (len > static_cast<int>(bit_string.size)) {
        stride = stride + 1;
    }
    stride.x = pow2[stride.x];
    stride.y = pow2[stride.y];
    stride.z = pow2[stride.z];
    return stride;
}

uint64_t interleave_bits(StringRef bit_string, Vector3i coord)
{
    uint64_t interleaved_val = 0;
    const uint64_t one = 1;
    HANA_ASSERT(bit_string.size <= BIT_SIZE(uint64_t) + 1 && bit_string.size > 0);
    // TODO: try re-writing this loop in a more efficient way by removing
    // data dependency between loop iterations
    for (int i = int(bit_string.size) - 1; i >= 0; --i) {
        char v = bit_string[i];
        int j = static_cast<int>(bit_string.size) - i - 1;
        if (v == '0') {
            interleaved_val |= (coord.x & one) << j;
            coord.x >>= 1;
        }
        else if (v == '1') {
            interleaved_val |= (coord.y & one) << j;
            coord.y >>= 1;
        }
        else if (v == '2') {
            interleaved_val |= (coord.z & one) << j;
            coord.z >>= 1;
        }
        else {
            HANA_ASSERT(false);
        }
    }
    return interleaved_val;
}

Vector3i deinterleave_bits(StringRef bit_string, uint64_t val)
{
    Vector3i coord(0, 0, 0);
    const uint64_t one = 1;
    for (size_t i = 0; i < bit_string.size; ++i) {
        char v = bit_string[i];
        size_t j = bit_string.size - i - 1;
        if (v == '0') {
            coord.x |= (val & (one << j)) >> j;
            coord.x <<= 1;
        }
        else if (v == '1') {
            coord.y |= (val & (one << j)) >> j;
            coord.y <<= 1;
        }
        else if (v == '2') {
            coord.z |= (val & (one << j)) >> j;
            coord.z <<= 1;
        }
        else {
            HANA_ASSERT(false);
        }
    }
    coord.x >>= 1;
    coord.y >>= 1;
    coord.z >>= 1;
    return coord;
}

uint64_t z_to_hz(StringRef bit_string, uint64_t z)
{
    HANA_ASSERT(bit_string.size > 0 && bit_string.size < 64);
    z |= uint64_t(1) << (bit_string.size);
    z /= z & -int64_t(z);
    z >>= 1;
    return z;
}

uint64_t hz_to_z(StringRef bit_string, uint64_t hz, int hz_level)
{
    HANA_ASSERT(bit_string.size > 0 && bit_string.size < 64 && bit_string.size >= hz_level);
    int z_level = static_cast<int>(bit_string.size) - hz_level;
    HANA_ASSERT((hz << z_level) >> z_level == hz);
    uint64_t z = hz << (z_level + 1);
    z |= uint64_t(1) << z_level;
    return z;
}

uint64_t hz_to_z(StringRef bit_string, uint64_t hz)
{
    HANA_ASSERT(bit_string.size > 0 && bit_string.size < 64);
    int leading_zeros = num_leading_zeros(hz);
    int z_level = leading_zeros - (64 - static_cast<int>(bit_string.size));
    HANA_ASSERT(z_level >= 0);
    HANA_ASSERT((hz << z_level) >> z_level == hz);
    uint64_t z = hz << (z_level + 1);
    z |= uint64_t(1) << z_level;
    return z;
}

uint64_t xyz_to_hz(StringRef bit_string, Vector3i coord)
{
    uint64_t z = interleave_bits(bit_string, coord);
    uint64_t hz = z_to_hz(bit_string, z);
    return hz;
}

int hz_to_level(uint64_t hz)
{
  int level = 0;
  while (hz > 0) {
    hz /= 2;
    ++level;
  }
  return level;
}

Vector3i hz_to_xyz(StringRef bit_string, uint64_t hz)
{
    HANA_ASSERT(bit_string.size > 0 && bit_string.size < 64);
    int leading_zeros = num_leading_zeros(hz);
    int z_level = leading_zeros - (64 - static_cast<int>(bit_string.size));
    HANA_ASSERT(z_level >= 0);
    HANA_ASSERT((hz << z_level) >> z_level == hz);
    uint64_t z = hz << (z_level + 1);
    z |= uint64_t(1) << z_level;
    Vector3i xyz = deinterleave_bits(bit_string, z);
    return xyz;
}

bool intersect_grid(const Volume& vol, const Vector3i& from,
                    const Vector3i& to, const Vector3i& stride,
                    OUT Vector3i& output_from, OUT Vector3i& output_to)
{
    HANA_ASSERT(vol.is_valid());

    Vector3i min_to = vol.to;
    min_to.x = min(min_to.x, to.x);
    min_to.y = min(min_to.y, to.y);
    min_to.z = min(min_to.z, to.z);

    output_from = from + ((vol.from + stride - 1 - from) / stride) * stride;
    output_to = from + ((min_to - from) / stride) * stride;

    // we need to do the following corrections because the behavior of integer
    // division with negative integers are not well defined...
    if (vol.from.x < from.x) { output_from.x = from.x; }
    if (vol.from.y < from.y) { output_from.y = from.y; }
    if (vol.from.z < from.z) { output_from.z = from.z; }
    if (min_to.x < from.x) { output_to.x = from.x - stride.x; }
    if (min_to.y < from.y) { output_to.y = from.y - stride.y; }
    if (min_to.z < from.z) { output_to.z = from.z - stride.z; }

    return output_from <= output_to;
}

}
