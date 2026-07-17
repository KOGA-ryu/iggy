#include "core/math/OrientedBox.hpp"

#include "core/math/Aabb3.hpp"
#include "core/math/Transform3.hpp"
#include "core/math/Vec3.hpp"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace {
using iggy3d::Aabb3;
using iggy3d::contains;
using iggy3d::intersectsRay;
using iggy3d::makeAabb3;
using iggy3d::makeOrientedBox;
using iggy3d::OrientedBox;
using iggy3d::OrientedBoxRayHit;
using iggy3d::orientedBoxCorners;
using iggy3d::orientedBoxWorldAabb;
using iggy3d::strictlyOverlaps;
using iggy3d::Transform3;
using iggy3d::Vec3;

constexpr float kHalfPi = 1.57079632679489662F;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(float a, float b, float tol = 1e-3F) { return std::fabs(a - b) <= tol; }

Transform3 makeTransform(Vec3 position, Vec3 eulerRadians,
                         Vec3 scale = Vec3{1.0F, 1.0F, 1.0F}) {
  return Transform3{position, eulerRadians, scale};
}

// A box centered at the origin, long along local X (half 2), thin in Y and Z (half 0.5).
Aabb3 longXBox() {
  return makeAabb3(Vec3{-2.0F, -0.5F, -0.5F}, Vec3{2.0F, 0.5F, 0.5F});
}

bool axisAlignedMatchesTranslatedBounds() {
  const OrientedBox box = makeOrientedBox(
      makeTransform(Vec3{5.0F, 0.0F, 0.0F}, Vec3{}),
      makeAabb3(Vec3{-1.0F, -1.0F, -1.0F}, Vec3{1.0F, 1.0F, 1.0F}));
  const Aabb3 worldAabb = orientedBoxWorldAabb(box);
  return expect(near(worldAabb.min.x, 4.0F) && near(worldAabb.max.x, 6.0F),
                "identity-rot world aabb translates in x") &&
         expect(near(worldAabb.min.y, -1.0F) && near(worldAabb.max.z, 1.0F),
                "identity-rot world aabb keeps y/z");
}

bool worldAabbEnclosesEveryCorner() {
  const OrientedBox box = makeOrientedBox(
      makeTransform(Vec3{1.0F, 2.0F, -3.0F}, Vec3{0.3F, kHalfPi, 0.7F},
                    Vec3{1.5F, 1.0F, 2.0F}),
      longXBox());
  const Aabb3 worldAabb = orientedBoxWorldAabb(box);
  const std::array<Vec3, 8> corners = orientedBoxCorners(box);
  bool enclosed = true;
  for (const Vec3& corner : corners) {
    enclosed = enclosed && contains(makeOrientedBox(Transform3{},
                                                    makeAabb3(worldAabb.min,
                                                              worldAabb.max)),
                                    corner);
  }
  return expect(enclosed, "world aabb encloses every oriented corner");
}

// The signature test: a point that is OUTSIDE the long-X box becomes INSIDE once the box is
// rotated 90 deg about Y (its long axis swings onto Z). Proves orientation is honored.
bool rotationChangesContainment() {
  const OrientedBox unrotated =
      makeOrientedBox(makeTransform(Vec3{}, Vec3{}), longXBox());
  const OrientedBox rotated =
      makeOrientedBox(makeTransform(Vec3{}, Vec3{0.0F, kHalfPi, 0.0F}), longXBox());
  const Vec3 probe{0.0F, 0.0F, 1.5F};
  return expect(!contains(unrotated, probe),
                "probe outside the un-rotated long-X box") &&
         expect(contains(rotated, probe),
                "probe inside once the long axis rotates onto z");
}

bool worldAabbReflectsRotation() {
  const OrientedBox rotated =
      makeOrientedBox(makeTransform(Vec3{}, Vec3{0.0F, kHalfPi, 0.0F}), longXBox());
  const Aabb3 worldAabb = orientedBoxWorldAabb(rotated);
  const Vec3 half = iggy3d::extents(worldAabb);
  return expect(near(half.x, 0.5F, 1e-2F), "rotated x half shrinks to 0.5") &&
         expect(near(half.z, 2.0F, 1e-2F), "rotated z half grows to 2.0") &&
         expect(near(half.y, 0.5F, 1e-2F), "rotated y half unchanged");
}

bool rayHitsRotatedBoxAtExpectedDistance() {
  const OrientedBox rotated =
      makeOrientedBox(makeTransform(Vec3{}, Vec3{0.0F, kHalfPi, 0.0F}), longXBox());
  const OrientedBoxRayHit hit =
      intersectsRay(rotated, Vec3{0.0F, 0.0F, 10.0F}, Vec3{0.0F, 0.0F, -1.0F}, 100.0F);
  return expect(hit.hit, "ray hits rotated box") &&
         expect(near(hit.distanceMeters, 8.0F), "hit distance is 8 m (enters at z=2)") &&
         expect(near(hit.pointMeters.z, 2.0F, 1e-2F), "hit point on the +z face") &&
         expect(!hit.startInside, "ray origin is outside");
}

bool rayMissesWhenOffAxis() {
  const OrientedBox rotated =
      makeOrientedBox(makeTransform(Vec3{}, Vec3{0.0F, kHalfPi, 0.0F}), longXBox());
  const OrientedBoxRayHit miss =
      intersectsRay(rotated, Vec3{10.0F, 0.0F, 10.0F}, Vec3{0.0F, 0.0F, -1.0F}, 100.0F);
  return expect(!miss.hit, "off-axis ray misses");
}

bool rayStartingInsideReportsZeroDistance() {
  const OrientedBox box =
      makeOrientedBox(makeTransform(Vec3{}, Vec3{}), longXBox());
  const OrientedBoxRayHit hit =
      intersectsRay(box, Vec3{0.0F, 0.0F, 0.0F}, Vec3{1.0F, 0.0F, 0.0F}, 100.0F);
  return expect(hit.hit, "ray from inside hits") &&
         expect(hit.startInside, "start-inside reported") &&
         expect(near(hit.distanceMeters, 0.0F), "inside start distance is zero");
}

bool invalidInputsRejected() {
  const OrientedBox inverted = makeOrientedBox(
      makeTransform(Vec3{}, Vec3{}),
      makeAabb3(Vec3{1.0F, 1.0F, 1.0F}, Vec3{-1.0F, -1.0F, -1.0F}));
  const float inf = std::numeric_limits<float>::infinity();
  const OrientedBox nonFinite =
      makeOrientedBox(makeTransform(Vec3{}, Vec3{0.0F, inf, 0.0F}), longXBox());

  const Aabb3 invertedAabb = orientedBoxWorldAabb(inverted);
  return expect(!contains(inverted, Vec3{}), "inverted box contains nothing") &&
         expect(!intersectsRay(nonFinite, Vec3{0, 0, 10}, Vec3{0, 0, -1}, 100.0F).hit,
                "non-finite transform yields no ray hit") &&
         expect(near(invertedAabb.min.x, 0.0F) && near(invertedAabb.max.x, 0.0F),
                "invalid box world aabb is a zero box");
}

bool degenerateDirectionRejected() {
  const OrientedBox box =
      makeOrientedBox(makeTransform(Vec3{}, Vec3{}), longXBox());
  return expect(!intersectsRay(box, Vec3{0, 0, 10}, Vec3{0, 0, 0}, 100.0F).hit,
                "zero-length direction yields no hit") &&
         expect(!intersectsRay(box, Vec3{0, 0, 10}, Vec3{0, 0, -1}, -1.0F).hit,
                "negative max distance yields no hit");
}

bool strictOverlapDistinguishesContactFromPenetration() {
  const Aabb3 unit =
      makeAabb3(Vec3{-0.5F, -0.5F, -0.5F}, Vec3{0.5F, 0.5F, 0.5F});
  const OrientedBox origin =
      makeOrientedBox(makeTransform(Vec3{}, Vec3{}), unit);
  const OrientedBox touching = makeOrientedBox(
      makeTransform(Vec3{1.0F, 0.0F, 0.0F}, Vec3{}), unit);
  const OrientedBox penetrating = makeOrientedBox(
      makeTransform(Vec3{0.99F, 0.0F, 0.0F}, Vec3{}), unit);
  const OrientedBox epsilonOnly = makeOrientedBox(
      makeTransform(Vec3{0.999995F, 0.0F, 0.0F}, Vec3{}), unit);
  return expect(!strictlyOverlaps(origin, touching),
                "face contact is not positive-volume overlap") &&
         expect(strictlyOverlaps(origin, penetrating),
                "positive-volume penetration overlaps") &&
         expect(!strictlyOverlaps(origin, epsilonOnly),
                "sub-epsilon penetration is treated as contact");
}

bool rotatedNarrowPhaseRejectsBroadphaseFalsePositive() {
  constexpr float kQuarterTurn = 0.78539816339744831F;
  const Vec3 perpendicular{0.84852815F, 0.0F, 0.84852815F};
  const OrientedBox first = makeOrientedBox(
      makeTransform(Vec3{}, Vec3{0.0F, kQuarterTurn, 0.0F}), longXBox());
  const OrientedBox separated = makeOrientedBox(
      makeTransform(perpendicular, Vec3{0.0F, kQuarterTurn, 0.0F}),
      longXBox());
  const OrientedBox penetrating = makeOrientedBox(
      makeTransform(perpendicular * 0.75F,
                    Vec3{0.0F, kQuarterTurn, 0.0F}),
      longXBox());
  return expect(intersects(orientedBoxWorldAabb(first),
                           orientedBoxWorldAabb(separated)),
                "rotated broadphase bounds overlap") &&
         expect(!strictlyOverlaps(first, separated),
                "exact rotated boxes remain separated") &&
         expect(strictlyOverlaps(first, penetrating),
                "rotated penetration is detected");
}

bool strictOverlapRejectsInvalidInputs() {
  const OrientedBox valid =
      makeOrientedBox(makeTransform(Vec3{}, Vec3{}), longXBox());
  const OrientedBox flat = makeOrientedBox(
      makeTransform(Vec3{}, Vec3{}),
      makeAabb3(Vec3{-1.0F, 0.0F, -1.0F}, Vec3{1.0F, 0.0F, 1.0F}));
  return expect(!strictlyOverlaps(valid, flat),
                "zero-volume box cannot strictly overlap") &&
         expect(!strictlyOverlaps(valid, valid, -1.0F),
                "negative overlap epsilon is rejected");
}

}  // namespace

int main() {
  const bool ok =
      axisAlignedMatchesTranslatedBounds() && worldAabbEnclosesEveryCorner() &&
      rotationChangesContainment() && worldAabbReflectsRotation() &&
      rayHitsRotatedBoxAtExpectedDistance() && rayMissesWhenOffAxis() &&
      rayStartingInsideReportsZeroDistance() && invalidInputsRejected() &&
      degenerateDirectionRejected() &&
      strictOverlapDistinguishesContactFromPenetration() &&
      rotatedNarrowPhaseRejectsBroadphaseFalsePositive() &&
      strictOverlapRejectsInvalidInputs();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
