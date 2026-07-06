#include "StandalonePlacement.hpp"

#include "core/math/Snap.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

using iggy3d::Vec3;
using iggy3d_creative_app::snapGroundToCellCenter;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(float lhs, float rhs, float tolerance = 1.0e-5F) {
  return std::fabs(lhs - rhs) <= tolerance;
}

bool sameVec3(Vec3 lhs, Vec3 rhs) {
  return near(lhs.x, rhs.x) && near(lhs.y, rhs.y) && near(lhs.z, rhs.z);
}

Vec3 expectedCellCenter(double worldX, double worldZ, double cellSize) {
  const float cell = static_cast<float>(cellSize);
  Vec3 expected = iggy3d::snapVec3ToCellCenter(
      {static_cast<float>(worldX), 0.0F, static_cast<float>(worldZ)},
      {cell, cell, cell},
      {0.0F, 0.0F, 0.0F},
      0x5u);
  expected.y = 0.0F;
  return expected;
}

bool groundSnapUsesCoreCellCenterMath() {
  const Vec3 snapped = snapGroundToCellCenter(1.2, -1.2, 1.0);
  return expect(sameVec3(snapped, expectedCellCenter(1.2, -1.2, 1.0)),
                "1m cell snap matches core") &&
         expect(near(snapped.x, 1.5F), "1m cell x center") &&
         expect(near(snapped.y, 0.0F), "1m cell y remains ground") &&
         expect(near(snapped.z, -1.5F), "1m cell z center");
}

bool groundSnapUsesContainingCellAtOriginBoundary() {
  const Vec3 snapped = snapGroundToCellCenter(0.0, 0.0, 1.0);
  return expect(sameVec3(snapped, expectedCellCenter(0.0, 0.0, 1.0)),
                "origin boundary snap matches containing-cell core") &&
         expect(near(snapped.x, 0.5F), "origin boundary x uses upper cell") &&
         expect(near(snapped.y, 0.0F), "origin boundary y remains ground") &&
         expect(near(snapped.z, 0.5F), "origin boundary z uses upper cell");
}

bool groundSnapSupportsLargerCellsAndLeavesYAtZero() {
  const Vec3 snapped = snapGroundToCellCenter(2.1, -2.1, 2.0);
  return expect(sameVec3(snapped, expectedCellCenter(2.1, -2.1, 2.0)),
                "2m cell snap matches core") &&
         expect(near(snapped.x, 3.0F), "2m cell x center") &&
         expect(near(snapped.y, 0.0F), "2m cell y remains ground") &&
         expect(near(snapped.z, -3.0F), "2m cell z center");
}

}  // namespace

int main() {
  const bool ok = groundSnapUsesCoreCellCenterMath() &&
                  groundSnapUsesContainingCellAtOriginBoundary() &&
                  groundSnapSupportsLargerCellsAndLeavesYAtZero();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
