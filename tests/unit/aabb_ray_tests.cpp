#include "core/math/Aabb3.hpp"
#include "core/math/Ray3.hpp"
#include "core/math/Vec3.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace {

using iggy3d::Aabb3;
using iggy3d::AabbRayHit;
using iggy3d::intersectsRay;
using iggy3d::makeAabb3;
using iggy3d::Ray3;
using iggy3d::Vec3;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(float lhs, float rhs, float epsilon = 1.0e-4F) {
  return std::fabs(lhs - rhs) <= epsilon;
}

Aabb3 unitBox() {
  return makeAabb3(Vec3{-1.0F, -1.0F, -1.0F},
                   Vec3{1.0F, 1.0F, 1.0F});
}

bool frontFaceHitReportsMeterDistanceFromNonUnitRay() {
  const AabbRayHit hit = intersectsRay(
      unitBox(), Ray3{Vec3{0.0F, 0.0F, 5.0F}, Vec3{0.0F, 0.0F, -2.0F}},
      100.0F);
  return expect(hit.hit, "front face hit") &&
         expect(near(hit.distanceMeters, 4.0F), "entry distance in meters") &&
         expect(near(hit.exitDistanceMeters, 6.0F), "exit distance in meters") &&
         expect(near(hit.pointMeters.z, 1.0F), "hit point on front z face") &&
         expect(!hit.startInside, "front hit starts outside");
}

bool startInsideReportsZeroEntryAndForwardExit() {
  const AabbRayHit hit = intersectsRay(
      unitBox(), Ray3{Vec3{0.0F, 0.0F, 0.0F}, Vec3{1.0F, 0.0F, 0.0F}},
      100.0F);
  return expect(hit.hit, "inside ray hits") &&
         expect(near(hit.distanceMeters, 0.0F), "inside entry is zero") &&
         expect(near(hit.exitDistanceMeters, 1.0F), "inside exit is forward face") &&
         expect(hit.startInside, "inside ray marks startInside");
}

bool parallelRayInsideSlabsCanHit() {
  const AabbRayHit hit = intersectsRay(
      unitBox(), Ray3{Vec3{0.25F, 0.5F, 5.0F}, Vec3{0.0F, 0.0F, -1.0F}},
      100.0F);
  return expect(hit.hit, "parallel-in-x/y ray can hit") &&
         expect(near(hit.distanceMeters, 4.0F), "parallel slab entry distance");
}

bool parallelRayOutsideSlabMisses() {
  const AabbRayHit hit = intersectsRay(
      unitBox(), Ray3{Vec3{2.0F, 0.0F, 5.0F}, Vec3{0.0F, 0.0F, -1.0F}},
      100.0F);
  return expect(!hit.hit, "parallel outside slab misses");
}

bool boxBehindRayMisses() {
  const AabbRayHit hit = intersectsRay(
      unitBox(), Ray3{Vec3{0.0F, 0.0F, 5.0F}, Vec3{0.0F, 0.0F, 1.0F}},
      100.0F);
  return expect(!hit.hit, "box behind ray misses");
}

bool maxDistanceClipsHit() {
  const AabbRayHit miss = intersectsRay(
      unitBox(), Ray3{Vec3{0.0F, 0.0F, 5.0F}, Vec3{0.0F, 0.0F, -1.0F}},
      3.9F);
  const AabbRayHit hit = intersectsRay(
      unitBox(), Ray3{Vec3{0.0F, 0.0F, 5.0F}, Vec3{0.0F, 0.0F, -1.0F}},
      4.0F);
  return expect(!miss.hit, "max distance before face clips miss") &&
         expect(hit.hit, "max distance at face hits");
}

bool invalidInputMisses() {
  const float inf = std::numeric_limits<float>::infinity();
  return expect(!intersectsRay(unitBox(), Ray3{Vec3{}, Vec3{}}, 100.0F).hit,
                "zero direction misses") &&
         expect(!intersectsRay(unitBox(),
                               Ray3{Vec3{inf, 0.0F, 0.0F},
                                    Vec3{0.0F, 0.0F, -1.0F}},
                               100.0F)
                     .hit,
                "non-finite origin misses") &&
         expect(!intersectsRay(makeAabb3(Vec3{1.0F, 0.0F, 0.0F},
                                         Vec3{-1.0F, 0.0F, 0.0F}),
                               Ray3{Vec3{}, Vec3{1.0F, 0.0F, 0.0F}},
                               100.0F)
                     .hit,
                "invalid bounds miss") &&
         expect(!intersectsRay(unitBox(),
                               Ray3{Vec3{}, Vec3{1.0F, 0.0F, 0.0F}},
                               -1.0F)
                     .hit,
                "negative max distance misses");
}

}  // namespace

int main() {
  const bool ok = frontFaceHitReportsMeterDistanceFromNonUnitRay() &&
                  startInsideReportsZeroEntryAndForwardExit() &&
                  parallelRayInsideSlabsCanHit() &&
                  parallelRayOutsideSlabMisses() && boxBehindRayMisses() &&
                  maxDistanceClipsHit() && invalidInputMisses();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
