#include "core/hash/StableHash.hpp"
#include "core/math/Aabb3.hpp"
#include "core/math/Mat4.hpp"
#include "core/math/Plane.hpp"
#include "core/math/Ray3.hpp"
#include "core/math/Transform3.hpp"
#include "core/math/Vec3.hpp"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool vec3Tests() {
  using namespace iggy3d;
  static_assert(sizeof(Vec3) == sizeof(float) * 3U);
  Vec3 value;
  return expect(nearlyEqual(value, Vec3{0.0F, 0.0F, 0.0F}), "Vec3 default") &&
         expect(nearlyEqual(vec3UnitX() + vec3UnitY(), Vec3{1.0F, 1.0F, 0.0F}), "Vec3 add") &&
         expect(nearlyEqual(Vec3{3.0F, 2.0F, 1.0F} - Vec3{1.0F, 1.0F, 1.0F},
                            Vec3{2.0F, 1.0F, 0.0F}),
                "Vec3 subtract") &&
         expect(nearlyEqual(Vec3{1.0F, 2.0F, 3.0F} * 2.0F, Vec3{2.0F, 4.0F, 6.0F}),
                "Vec3 scale") &&
         expect(dot(Vec3{1.0F, 2.0F, 3.0F}, Vec3{4.0F, 5.0F, 6.0F}) == 32.0F, "Vec3 dot") &&
         expect(distanceSquared(Vec3{0.0F, 0.0F, 0.0F}, Vec3{2.0F, 0.0F, 0.0F}) == 4.0F,
                "Vec3 distance") &&
         expect(!isFinite(Vec3{std::numeric_limits<float>::infinity(), 0.0F, 0.0F}),
                "Vec3 finite rejection");
}

bool transformTests() {
  using namespace iggy3d;
  const Transform3 identity = identityTransform3();
  return expect(nearlyEqual(identity.position, vec3Zero()), "Transform position") &&
         expect(nearlyEqual(identity.rotationEulerRadians, vec3Zero()), "Transform rotation") &&
         expect(nearlyEqual(identity.scale, Vec3{1.0F, 1.0F, 1.0F}), "Transform scale") &&
         expect(hasPositiveFiniteScale(identity), "Transform positive scale") &&
         expect(nearlyEqual(transformPoint(Transform3{Vec3{1.0F, 2.0F, 3.0F}, {}, Vec3{2.0F, 3.0F, 4.0F}},
                                           Vec3{1.0F, 1.0F, 1.0F}),
                            Vec3{3.0F, 5.0F, 7.0F}),
                "Transform point");
}

bool aabbTests() {
  using namespace iggy3d;
  const Aabb3 box = makeAabb3(Vec3{0.0F, 0.0F, 0.0F}, Vec3{2.0F, 2.0F, 2.0F});
  return expect(isValid(box), "Aabb valid") &&
         expect(!isValid(makeAabb3(Vec3{2.0F, 0.0F, 0.0F}, Vec3{1.0F, 0.0F, 0.0F})),
                "Aabb invalid") &&
         expect(nearlyEqual(center(box), Vec3{1.0F, 1.0F, 1.0F}), "Aabb center") &&
         expect(contains(box, Vec3{2.0F, 2.0F, 2.0F}), "Aabb inclusive contains") &&
         expect(intersects(box, makeAabb3(Vec3{2.0F, 2.0F, 2.0F}, Vec3{3.0F, 3.0F, 3.0F})),
                "Aabb touching intersects") &&
         expect(!intersects(box, makeAabb3(Vec3{3.0F, 3.0F, 3.0F}, Vec3{2.0F, 2.0F, 2.0F})),
                "Aabb invalid no intersect") &&
         expect(nearlyEqual(closestPoint(box, Vec3{4.0F, -1.0F, 1.0F}), Vec3{2.0F, 0.0F, 1.0F}),
                "Aabb closest");
}

bool rayPlaneTests() {
  using namespace iggy3d;
  const Ray3 ray = makeRay3(Vec3{0.0F, 0.0F, 0.0F}, Vec3{1.0F, 0.0F, 0.0F});
  const Plane plane = planeFromPointNormal(Vec3{2.0F, 0.0F, 0.0F}, Vec3{1.0F, 0.0F, 0.0F});
  const RayPlaneHit hit = intersectRayPlane(ray, plane);
  const RayPlaneHit miss = intersectRayPlane(makeRay3(vec3Zero(), vec3UnitY()), plane);
  return expect(isValid(ray), "Ray valid") &&
         expect(!isValid(makeRay3(vec3Zero(), vec3Zero())), "Ray zero invalid") &&
         expect(nearlyEqual(pointAt(ray, 2.0F), Vec3{2.0F, 0.0F, 0.0F}), "Ray pointAt") &&
         expect(isValid(plane), "Plane valid") &&
         expect(!isValid(makePlane(vec3Zero(), 0.0F)), "Plane invalid") &&
         expect(signedDistance(plane, Vec3{3.0F, 0.0F, 0.0F}) == 1.0F, "Plane distance") &&
         expect(hit.hit && hit.t == 2.0F && nearlyEqual(hit.point, Vec3{2.0F, 0.0F, 0.0F}),
                "Plane ray hit") &&
         expect(!miss.hit, "Plane ray miss");
}

bool mat4Tests() {
  using namespace iggy3d;
  const Mat4 identity = identityMat4();
  const Mat4 translation = translationMat4(Vec3{1.0F, 2.0F, 3.0F});
  const Mat4 scale = scaleMat4(Vec3{2.0F, 2.0F, 2.0F});
  const Mat4 composed = translation * scale;
  return expect(at(identity, 0, 0) == 1.0F && at(identity, 3, 3) == 1.0F, "Mat4 identity") &&
         expect(nearlyEqual(transformPoint(translation, vec3Zero()), Vec3{1.0F, 2.0F, 3.0F}),
                "Mat4 translation") &&
         expect(nearlyEqual(transformPoint(composed, Vec3{1.0F, 1.0F, 1.0F}), Vec3{3.0F, 4.0F, 5.0F}),
                "Mat4 order") &&
         expect(isFinite(rotationEulerRadiansMat4(Vec3{0.1F, 0.2F, 0.3F})), "Mat4 rotation finite");
}

bool hashTests() {
  using namespace iggy3d;
  StableHasher first;
  StableHasher second;
  first.addString("abc");
  second.addString("abc");
  StableHasher orderedA;
  orderedA.addString("a");
  orderedA.addString("b");
  StableHasher orderedB;
  orderedB.addString("b");
  orderedB.addString("a");
  StableHasher rounded;
  rounded.addFloatQuantized(1.0004F);
  StableHasher roundedDifferent;
  roundedDifferent.addFloatQuantized(1.0006F);
  return expect(kStableHashOffsetBasis == 14695981039346656037ULL, "Hash offset") &&
         expect(kStableHashPrime == 1099511628211ULL, "Hash prime") &&
         expect(first.value() == second.value(), "Hash repeat") &&
         expect(orderedA.value() != orderedB.value(), "Hash order") &&
         expect(rounded.value() != roundedDifferent.value(), "Hash quantized rounding");
}

}  // namespace

int main() {
  const bool ok = vec3Tests() && transformTests() && aabbTests() && rayPlaneTests() && mat4Tests() &&
                  hashTests();
  return ok ? 0 : 1;
}
