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
using iggy3d::snapToCellCenter;
using iggy3d::snapVec3ToCellCenter;
using iggy3d::snapVec3ToGrid;
using iggy3d::Vec3;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(float a, float b, float tol = 1e-4F) { return std::fabs(a - b) <= tol; }

bool near(double a, double b, double tol = 1e-9) {
  return std::fabs(a - b) <= tol;
}

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

bool doubleScalarSnapsAndHonorsOrigin() {
  return expect(near(snapScalarToGrid(2.2, 2.0, 1.0), 3.0),
                "double scalar honors origin x") &&
         expect(near(snapScalarToGrid(5.0, 3.0, 1.0), 4.0),
                "double scalar honors origin y") &&
         expect(near(snapScalarToGrid(-1.26, 0.5, 0.0), -1.5),
                "double scalar snaps negative");
}

bool doubleScalarPassesThroughOnInvalidInputs() {
  const double inf = std::numeric_limits<double>::infinity();
  const double big = 1.0e308;
  const double overflowOut = snapScalarToGrid(big, 1.0, -big);
  return expect(std::isinf(snapScalarToGrid(inf, 1.0, 0.0)),
                "double non-finite value passes through") &&
         expect(near(snapScalarToGrid(5.0, 1.0, inf), 5.0),
                "double non-finite origin passes through") &&
         expect(near(snapScalarToGrid(5.0, inf, 0.0), 5.0),
                "double non-finite step passes through") &&
         expect(near(snapScalarToGrid(5.0, 0.0, 0.0), 5.0),
                "double zero step passes through") &&
         expect(near(snapScalarToGrid(5.0, -2.0, 0.0), 5.0),
                "double negative step passes through") &&
         expect(std::isfinite(overflowOut), "double overflow result stays finite") &&
         expect(near(overflowOut, big, 1.0e292),
                "double overflow falls back to input");
}

bool doubleScalarDoesNotRouteThroughFloat() {
  const double value = 16777216.75;
  const double out = snapScalarToGrid(value, 0.5, 0.0);
  const float floatOut =
      snapScalarToGrid(static_cast<float>(value), 0.5F, 0.0F);
  return expect(out == 16777217.0,
                "double scalar preserves large half-step precision") &&
         expect(static_cast<double>(floatOut) != out,
                "double scalar is not routed through float");
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

bool cellCenterUsesContainingCellNotNearest() {
  // The exact reported case: a ground hit ON the origin line must land in cell [0,1), center 0.5 --
  // NOT the negative cell (-0.5) a nearest-grid-point (round) snap would pick.
  return expect(near(snapToCellCenter(0.0F, 1.0F, 0.0F), 0.5F),
                "value 0 -> containing cell [0,1) center 0.5") &&
         expect(near(snapScalarToGrid(0.0F, 1.0F, 0.5F), -0.5F),
                "the round route WOULD give -0.5 -- the bug we avoid");
}

bool cellCenterMatchesOldPlaceFormula() {
  // Old Place: floor(v/cell)*cell + cell*0.5 (gridOrigin 0). Pin equivalence across signs/boundaries.
  const float cell = 1.0F;
  const float samples[] = {-2.4F, -1.0F, -0.3F, 0.0F, 0.5F, 0.999F, 1.0F, 1.5F, 3.2F};
  bool ok = true;
  for (const float v : samples) {
    const float expected = std::floor(v / cell) * cell + cell * 0.5F;
    ok = ok && expect(near(snapToCellCenter(v, cell, 0.0F), expected),
                      "cell center matches the old Place formula");
  }
  return ok;
}

bool cellCenterBoundaryOpensUpperCell() {
  // A point exactly on a cell line belongs to the cell it opens (floor: [1,2) includes 1.0).
  return expect(near(snapToCellCenter(1.0F, 1.0F, 0.0F), 1.5F), "1.0 -> cell [1,2) center 1.5") &&
         expect(near(snapToCellCenter(-1.0F, 1.0F, 0.0F), -0.5F), "-1.0 -> cell [-1,0) center -0.5");
}

bool cellCenterHonorsOriginAndSize() {
  // 2 m cells offset by origin 1.0: cells [1,3),[3,5),... centers 2,4,...
  return expect(near(snapToCellCenter(1.2F, 2.0F, 1.0F), 2.0F), "1.2 in [1,3) -> center 2") &&
         expect(near(snapToCellCenter(3.0F, 2.0F, 1.0F), 4.0F), "3.0 in [3,5) -> center 4");
}

bool cellCenterVec3HoldsGroundAxis() {
  // Place: snap X,Z to containing cell centers, hold Y at the ground (mask 0x5).
  const Vec3 out = snapVec3ToCellCenter(Vec3{0.0F, 7.3F, 0.0F}, Vec3{1.0F, 1.0F, 1.0F},
                                        Vec3{0.0F, 0.0F, 0.0F}, 0x5u);
  return expect(near(out.x, 0.5F) && near(out.z, 0.5F), "X,Z -> containing cell centers") &&
         expect(near(out.y, 7.3F), "held Y passes through");
}

bool cellCenterPassThroughBadCell() {
  return expect(near(snapToCellCenter(3.0F, 0.0F, 0.0F), 3.0F), "zero cell size passes through") &&
         expect(near(snapToCellCenter(3.0F, -1.0F, 0.0F), 3.0F), "negative cell size passes through");
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
                  scalarOverflowPassesThrough() &&
                  doubleScalarSnapsAndHonorsOrigin() &&
                  doubleScalarPassesThroughOnInvalidInputs() &&
                  doubleScalarDoesNotRouteThroughFloat() && vec3SnapsEveryAxis() &&
                  vec3AxisMaskHoldsAxis() && vec3PerAxisStepAndOrigin() &&
                  deltaSnapViaGrabOrigin() &&
                  cellCenterUsesContainingCellNotNearest() &&
                  cellCenterMatchesOldPlaceFormula() &&
                  cellCenterBoundaryOpensUpperCell() &&
                  cellCenterHonorsOriginAndSize() &&
                  cellCenterVec3HoldsGroundAxis() && cellCenterPassThroughBadCell() &&
                  alignBaseDefaultsToY() &&
                  alignBaseOnZAndX() && alignPreservesSize() &&
                  alignPassThroughOnInvalid();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
