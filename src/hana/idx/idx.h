#pragma once

#include "idx_block.h"
#include "idx_file.h"
#include "error.h"
#include "types.h"
#include "../core/error.h"
#include "../core/types.h"
#include <cstdint>
#include <cstdio>

namespace hana { namespace idx {

/** Read a rectilinear grid of a field at a time step and hz level.
The argument grid's should be populated with the needed extends of the grid.
Also, its data member should point to a pre-allocated buffer large enough to
hold the data. The input hz level must be larger than the minimum hz level
as returned by idx_file.get_min_hz_level().
NOTE: This function cannot be used to read the entire volume of the data,
since at most, at the highest hz level possible, it will give you half the
entire volume. */
Error read_idx_grid(
    const IdxFile& idx_file, int field, int time, int hz_level,
    IN_OUT Grid* grid);

/** This function is useful in progressive mode, where it is supposed to be
called multiple times with different hz_levels, but the data will be saved to
the same final grid. Most of the time you don't want to call this function
directly.
NOTE: If the given hz_level is smaller than the minimum hz_level (see the class
IdxFile in idx_file.h), the returned grid will have samples from all hz levels
from 0 up to the minimum hz level. This is because they all belong to the same
(first) idx block. */
Error read_idx_grid(
    const IdxFile& idx_file, int field, int time, int hz_level,
    const core::Vector3i& output_from, const core::Vector3i& output_to,
    const core::Vector3i& output_stride, IN_OUT Grid* grid);

/** Read data at hz levels 0, 1, 2, ..., hz_level and combine all samples into
one grid. This function can be used to read the entire volume of the data, by
passing in the maximum hz_level possible. */
Error read_idx_grid_inclusive(const IdxFile& idx_file, int field, int time,
                              int hz_level, IN_OUT Grid* grid);

/** Call this function ONLY when you are done using the library's API. */
void deallocate_memory();

}}
