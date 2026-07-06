#include "core/math/Snap.hpp"

#include "core/math/Aabb3.hpp"
#include "core/math/Vec3.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace {
using iggy3d::Aabb3;
using iggy3d::alignAabbBaseToHeight;
using iggy3d::extents;
using iggy3d::makeAabb3;
using iggy3d::snapScalarToGrid;
using iggy3d::snapVec3ToGrid;
using iggy3d::Vec3;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(float a, float b, float tol = 1e-4F) { return std::fabs(a - b) <= tol; }

bool scalarSnapsToNearestMultiple() {
  return expect(near(snapScalarToGrid(2.3F, 1.0F, 0.0F), 2.0F), "2.3 -> 2") &&
         expect(near(snapScalarToGrid(2.7F, 1.0F, 0.0F), 3.0F), "2.7 -> 3") &&
         expect(near(snapScalarToGrid(-1.2F, 1.0F, 0.0F), -1.0F), "-1.2 -> -1") &&
         expect(near(snapScalarToGrid(3.0F, 2.0F, 0.0F), 4.0F), "3 on step 2 -> 4");
}

bool scalarHonorsOrigin() {
  // Snap to the grid offset by 0.5: nearest 0.5 + integer.
  return expect(near(snapScalarToGrid(2.3F, 1.0F, 0.5F), 2.5F),
                "2.3 on origin 0.5 -> 2.5") &&
         expect(near(snapScalarToGrid(2.9F, 1.0F, 0.5F), 2.5F),
                "2.9 on origin 0.5 -> 2.5");
}

bool scalarPassThroughOnBadStep() {
  return expect(near(snapScalarToGrid(5.0F, 0.0F, 0.0F), 5.0F), "step 0 passes through") &&
         expect(near(snapScalarToGrid(5.0F, -2.0F, 0.0F), 5.0F), "negative step passes through") &&
         expect(near(snapScalarToGrid(5.0F, std::numeric_limits<float>::infinity(), 0.0F),
                     5.0F),
                "non-finite step passes through");
}

bool scalarPassThroughOnNonFiniteValue() {
  const float inf = std::numeric_limits<float>::infinity();
  return expect(std::isinf(snapScalarToGrid(inf, 1.0F, 0.0F)),
                "non-finite value passes through unchanged");
}

bool scalarOverflowPassesThrough() {
  // Finite inputs of large opposite magnitude overflow (value-origin) to inf; must not emit inf.
  const float big = 3.0e38F;
  const float out = snapScalarToGrid(big, 1.0F, -big);
  return expect(std::isfinite(out), "overflow result stays finite") &&
         expect(near(out, big, 1.0F), "overflow falls back to the input value");
}

bool vec3SnapsEveryAxis() {
  const Vec3 out = snapVec3ToGrid(Vec3{2.3F, 4.6F, -1.2F}, Vec3{1.0F, 1.0F, 1.0F},
                                  Vec3{0.0F, 0.0F, 0.0F});
  return expect(near(out.x, 2.0F) && near(out.y, 5.0F) && near(out.z, -1.0F),
                "vec3 snaps all axes to nearest integer");
}

bool vec3AxisMaskHoldsAxis() {
  // Mask 0x5 = X and Z only; Y is held (the Move-tool held-axis case).
  const Vec3 out = snapVec3ToGrid(Vec3{2.3F, 4.6F, -1.2F}, Vec3{1.0F, 1.0F, 1.0F},
                                  Vec3{0.0F, 0.0F, 0.0F}, 0x5u);
  return expect(near(out.x, 2.0F), "masked X still snaps") &&
         expect(near(out.y, 4.6F), "held Y passes through untouched") &&
         expect(near(out.z, -1.0F), "masked Z still snaps");
}

bool vec3PerAxisStepAndOrigin() {
  const Vec3 out = snapVec3ToGrid(Vec3{3.0F, 0.3F, 2.2F}, Vec3{2.0F, 0.5F, 1.0F},
                                  Vec3{0.0F, 0.0F, 0.0F});
  return expect(near(out.x, 4.0F), "x step 2 -> 4") &&
         expect(near(out.y, 0.5F), "y step 0.5 -> 0.5") &&
         expect(near(out.z, 2.0F), "z step 1 -> 2");
}

bool deltaSnapViaGrabOrigin() {
  // Incremental snap = grid snap with origin at the grab point: only whole-step deltas land.
  const Vec3 grab{0.3F, 0.0F, 0.7F};
  const Vec3 dragged{1.1F, 0.0F, 1.9F};  // moved +0.8, +1.2 from grab
  const Vec3 out = snapVec3ToGrid(dragged, Vec3{1.0F, 1.0F, 1.0F}, grab);
  return expect(near(out.x, 1.3F), "x delta snaps to +1 step from grab") &&
         expect(near(out.z, 1.7F), "z delta snaps to +1 step from grab");
}

bool alignBaseDefaultsToY() {
  const Aabb3 box = makeAabb3(Vec3{0.0F, 5.0F, 0.0F}, Vec3{2.0F, 7.0F, 2.0F});
  const Aabb3 out = alignAabbBaseToHeight(box, 0.0F);
  return expect(near(out.min.y, 0.0F), "y base lands on target") &&
         expect(near(out.max.y, 2.0F), "y size preserved (2m)") &&
         expect(near(out.min.x, 0.0F) && near(out.min.z, 0.0F),
                "other axes untouched");
}

bool alignBaseOnZAndX() {
  const Aabb3 box = makeAabb3(Vec3{0.0F, 0.0F, 0.0F}, Vec3{2.0F, 2.0F, 2.0F});
  const Aabb3 z = alignAabbBaseToHeight(box, 10.0F, 2u);
  const Aabb3 x = alignAabbBaseToHeight(box, -3.0F, 0u);
  return expect(near(z.min.z, 10.0F) && near(z.max.z, 12.0F), "z base -> 10..12") &&
         expect(near(x.min.x, -3.0F) && near(x.max.x, -1.0F), "x base -> -3..-1");
}

bool alignPreservesSize() {
  const Aabb3 box = makeAabb3(Vec3{-1.0F, 3.0F, 2.0F}, Vec3{4.0F, 9.0F, 5.5F});
  const Aabb3 out = alignAabbBaseToHeight(box, 100.0F, 1u);
  const Vec3 before = extents(box);
  const Vec3 after = extents(out);
  return expect(near(before.x, after.x) && near(before.y, after.y) &&
                    near(before.z, after.z),
                "base-align preserves box extents on every axis");
}

bool alignPassThroughOnInvalid() {
  const Aabb3 inverted = makeAabb3(Vec3{1.0F, 1.0F, 1.0F}, Vec3{-1.0F, -1.0F, -1.0F});
  const Aabb3 valid = makeAabb3(Vec3{0.0F, 0.0F, 0.0F}, Vec3{1.0F, 1.0F, 1.0F});
  const float inf = std::numeric_limits<float>::infinity();
  const Aabb3 invOut = alignAabbBaseToHeight(inverted, 0.0F);
  const Aabb3 badAxis = alignAabbBaseToHeight(valid, 0.0F, 3u);
  const Aabb3 badTarget = alignAabbBaseToHeight(valid, inf, 1u);
  return expect(near(invOut.min.y, 1.0F), "invalid box unchanged") &&
         expect(near(badAxis.min.y, 0.0F), "axis > 2 unchanged") &&
         expect(near(badTarget.min.y, 0.0F), "non-finite target unchanged");
}

}  // namespace

int main() {
  const bool ok = scalarSnapsToNearestMultiple() && scalarHonorsOrigin() &&
                  scalarPassThroughOnBadStep() &&
                  scalarPassThroughOnNonFiniteValue() &&
                  scalarOverflowPassesThrough() && vec3SnapsEveryAxis() &&
                  vec3AxisMaskHoldsAxis() && vec3PerAxisStepAndOrigin() &&
                  deltaSnapViaGrabOrigin() && alignBaseDefaultsToY() &&
                  alignBaseOnZAndX() && alignPreservesSize() &&
                  alignPassThroughOnInvalid();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
