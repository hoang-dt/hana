namespace hana {
// TODO: move this function
template <typename t>
Error copy_grid(
  const Vector3i& srcFrom, const Vector3i& srcTo, const Vector3i& srcStride, const Grid& src,
  const Vector3i& dstFrom, const Vector3i& dstTo, const Vector3i& dstStride, IN_OUT Grid* dst)
{
  if (srcStride.x % dstStride.x != 0 || srcStride.y % dstStride.y != 0 || srcStride.z % dstStride.z != 0) {
    printf("Error: dst strides do not divide src strides\n");
    return Error::InvalidGrid;
  }
  // TODO: check the number of samples of the two grid too
  // TODO: check if the source "from" >= the dst "from"
  if (!(src.extent.from == dst->extent.from && src.extent.to == dst->extent.to)) {
    return Error::InvalidVolume;
  }
  Vector3i dims = (srcTo - srcFrom) / srcStride + 1;
  Vector3i todims = (dstTo - dstFrom) / dstStride + 1;
  Vector3i from = (srcFrom - dstFrom) / dstStride;
  Vector3i stride = srcStride / dstStride;
  Vector3i p = from;
  Vector3i q(0, 0, 0);
  const t* srcPtr = (const t*)src.data.ptr;
  t* dstPtr = (t*)dst->data.ptr;
  for (p.z = from.z, q.z = 0; q.z < dims.z; ++q.z, p.z += stride.z) {
    for (p.y = from.y, q.y = 0; q.y < dims.y; ++q.y, p.y += stride.y) {
      for (p.x = from.x, q.x = 0; q.x < dims.x; ++q.x, p.x += stride.x) {
        uint64_t i = uint64_t(q.z) * (dims.x * dims.y) + uint64_t(q.y) * (dims.x) + uint64_t(q.x);
        uint64_t j = uint64_t(p.z) * (todims.x * todims.y) + uint64_t(p.y) * (todims.x) + uint64_t(p.x);
        dstPtr[j] = srcPtr[i];
      }
    }
  }
  return Error::NoError;
}
}