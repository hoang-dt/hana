#include "types.h"

namespace hana {

bool Volume::is_valid() const {
  return from <= to;
}

bool Volume::is_inside(const Volume& other) const {
  return other.from <= from && from <= other.to &&
         other.from <= to   && to   <= other.to;
}

uint64_t Volume::get_num_samples() const
{
  Vector3u64 diff = to - from + 1;
  return diff.x * diff.y * diff.z;
}

}
