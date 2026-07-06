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

float snapScalarToGrid(float value, float step, float origin) noexcept {
  if (!std::isfinite(value) || !std::isfinite(origin) || !std::isfinite(step) ||
      !(step > 0.0F)) {
    return value;
  }
  const float snapped = std::round((value - origin) / step);
  const float scaled = snapped * step;
  const float result = origin + scaled;
  // Finite inputs of large opposite magnitude can overflow the subtraction to +/-inf; never emit
  // a non-finite snap -- fall back to the unsnapped value (consistent with the guards above).
  return std::isfinite(result) ? result : value;
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
