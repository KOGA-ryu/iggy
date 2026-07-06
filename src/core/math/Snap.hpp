#pragma once

#include "core/math/Aabb3.hpp"
#include "core/math/Vec3.hpp"

namespace iggy3d {

// Pure grid/pivot snapping over the core float types -- the atomic rounding the whole editor
// snap system stands on. Two `double` copies exist (creative/spatial/Snap::snapScalar 2D,
// creative/document/DocumentSnap::snapCreativeDocumentScalar 3D) but both are trapped in the
// creative lane; core had no snap math at all. This is its pure, deterministic, float home, so a
// creative object inherits snapping the same way it inherits picking (AabbGridIndex) and rotated
// hit-testing (OrientedBox): one core primitive, wrapped.

// Round `value` to the nearest multiple of `step` measured from `origin`:
//   origin + round((value - origin) / step) * step
// Pass-through (returns `value` unchanged) when step is non-positive/non-finite, when any input is
// non-finite, OR when the result would overflow to non-finite -- snapping never invents a NaN/inf
// or divides by zero. Bit-deterministic across toolchains (FP contraction is barred in the .cpp).
[[nodiscard]] float snapScalarToGrid(float value, float step, float origin) noexcept;

// Per-axis grid snap of a point. Each axis is snapped independently via snapScalarToGrid, but only
// for axes whose bit is set in `axisMask` (bit0 = X, bit1 = Y, bit2 = Z; default 0x7 = all three).
// Masked-out axes pass through untouched. Snapping a DELTA is the same call with origin = the grab
// point, so no separate incremental-snap function is needed.
[[nodiscard]] Vec3 snapVec3ToGrid(Vec3 value, Vec3 step, Vec3 origin,
                                  unsigned axisMask = 0x7u) noexcept;

// Pivot / feet-first snap: translate the whole box along one axis (0 = X, 1 = Y, 2 = Z; default Y,
// the core up-axis) so its MINIMUM coordinate on that axis lands exactly on `targetBase`, preserving
// the box's size on every axis. This is the pure core of drop-to-floor / pivot snap: given a surface
// height, drop the object's base onto it. (Finding that surface height is impure occupancy work and
// stays in the creative lane.) Creative document space is Z-up -- those callers pass axis = 2.
// Returns the box unchanged for an invalid box, non-finite target, or axis > 2.
[[nodiscard]] Aabb3 alignAabbBaseToHeight(const Aabb3& box, float targetBase,
                                          unsigned axis = 1u) noexcept;

}  // namespace iggy3d
