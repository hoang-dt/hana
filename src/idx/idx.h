#pragma once

#include "idx_block.h"
#include "idx_file.h"
#include "error.h"
#include "types.h"
#include "error.h"
#include "types.h"
#include <cstdint>
#include <cstdio>

namespace hana {

/** Read a rectilinear grid of a field at a time step and hz level.
The argument grid's should be populated with the needed extends of the grid.
Also, its data member should point to a pre-allocated buffer large enough to
hold the data. The input hz level must be larger than the minimum hz level
as returned by idx_file.get_min_hz_level().
NOTE: This function cannot be used to read the entire volume of the data,
since at most, at the highest hz level possible, it will give you half the
entire volume. */
Error read_idx_grid(const IdxFile& idx_file, int field, int time, int hz_level,
                    IN_OUT Grid* grid);

/** This function is useful in progressive mode, where it is supposed to be
called multiple times with different hz_levels, but the data will be saved to
the same final grid. Most of the time you don't want to call this function
directly.
NOTE: If the given hz_level is smaller than the minimum hz_level (see the class
IdxFile in idx_file.h), the returned grid will have samples from all hz levels
from 0 up to the minimum hz level. This is because they all belong to the same
(first) idx block. */
Error read_idx_grid(const IdxFile& idx_file, int field, int time, int hz_level,
                    const Vector3i& output_from, const Vector3i& output_to,
                    const Vector3i& output_stride, IN_OUT Grid* grid);

/** Read data at hz levels 0, 1, 2, ..., hz_level and combine all samples into
one grid. This function can be used to read the entire volume of the data, by
passing in the maximum hz_level possible. */
Error read_idx_grid_inclusive(const IdxFile& idx_file, int field, int time,
                              int hz_level, IN_OUT Grid* grid);

/** Write a raw array in memory into an IDX file on disk. The blocks and levels
are determined automatically. The input grid does not have to be of the same
dimensions as the dimensions specified in the IDX file. If read is true, the
block is read first before it is written. */
Error write_idx_grid(const IdxFile& idx_file, int field, int time,
                     const Grid& grid, bool read);

/** Call this function ONLY when you are done using the library's API. */
void deallocate_memory();

}
