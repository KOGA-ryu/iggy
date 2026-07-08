#include "core/math/Snap.hpp"

#include <cmath>

// Snapping is determinism-critical: identical inputs must yield identical BITS on macOS/arm64
// (Clang) and the Linux/x86-64 box (GCC), so saves and hashes match. Bar FP contraction for this
// TU: otherwise arm64 fuses `round(...)*step + origin` into a single-rounding FMA and diverges
// ~1 ULP from the box's two-rounding path. The named temporaries below keep the two roundings
// explicit; the pragma stops the fuse on toolchains that would otherwise re-merge them. (Aabb3's
// center/extents carry the same latent fused pattern -- worth the same discipline in a follow-up.)
#pragma STDC FP_CONTRACT OFF

namespace iggy3d {

namespace {

template <typename Scalar>
[[nodiscard]] Scalar snapScalarToGridChecked(Scalar value,
                                             Scalar step,
                                             Scalar origin) noexcept {
  if (!std::isfinite(value) || !std::isfinite(origin) ||
      !std::isfinite(step) || !(step > Scalar{0})) {
    return value;
  }
  const Scalar snapped = std::round((value - origin) / step);
  const Scalar scaled = snapped * step;
  const Scalar result = origin + scaled;
  // Finite inputs of large opposite magnitude can overflow the subtraction to +/-inf; never emit
  // a non-finite snap -- fall back to the unsnapped value (consistent with the guards above).
  return std::isfinite(result) ? result : value;
}

}  // namespace

float snapScalarToGrid(float value, float step, float origin) noexcept {
  return snapScalarToGridChecked(value, step, origin);
}

double snapScalarToGrid(double value, double step, double origin) noexcept {
  return snapScalarToGridChecked(value, step, origin);
}

Vec3 snapVec3ToGrid(Vec3 value, Vec3 step, Vec3 origin,
                    unsigned axisMask) noexcept {
  Vec3 result = value;
  if (axisMask & 0x1u) {
    result.x = snapScalarToGrid(value.x, step.x, origin.x);
  }
  if (axisMask & 0x2u) {
    result.y = snapScalarToGrid(value.y, step.y, origin.y);
  }
  if (axisMask & 0x4u) {
    result.z = snapScalarToGrid(value.z, step.z, origin.z);
  }
  return result;
}

float snapToCellCenter(float value, float cellSize, float gridOrigin) noexcept {
  if (!std::isfinite(value) || !std::isfinite(cellSize) ||
      !std::isfinite(gridOrigin) || !(cellSize > 0.0F)) {
    return value;
  }
  const float cellIndex = std::floor((value - gridOrigin) / cellSize);
  const float center = gridOrigin + (cellIndex + 0.5F) * cellSize;
  return std::isfinite(center) ? center : value;
}

Vec3 snapVec3ToCellCenter(Vec3 value, Vec3 cellSize, Vec3 gridOrigin,
                          unsigned axisMask) noexcept {
  Vec3 result = value;
  if (axisMask & 0x1u) {
    result.x = snapToCellCenter(value.x, cellSize.x, gridOrigin.x);
  }
  if (axisMask & 0x2u) {
    result.y = snapToCellCenter(value.y, cellSize.y, gridOrigin.y);
  }
  if (axisMask & 0x4u) {
    result.z = snapToCellCenter(value.z, cellSize.z, gridOrigin.z);
  }
  return result;
}

Aabb3 alignAabbBaseToHeight(const Aabb3& box, float targetBase,
                            unsigned axis) noexcept {
  if (!isValid(box) || !std::isfinite(targetBase) || axis > 2u) {
    return box;
  }

  float currentBase = box.min.y;
  if (axis == 0u) {
    currentBase = box.min.x;
  } else if (axis == 2u) {
    currentBase = box.min.z;
  }

  const float delta = targetBase - currentBase;
  Aabb3 shifted = box;
  if (axis == 0u) {
    shifted.min.x += delta;
    shifted.max.x += delta;
  } else if (axis == 2u) {
    shifted.min.z += delta;
    shifted.max.z += delta;
  } else {
    shifted.min.y += delta;
    shifted.max.y += delta;
  }
  return shifted;
}

}  // namespace iggy3d
