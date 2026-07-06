#include "core/hash/StableHash.hpp"
#include "core/math/Aabb3.hpp"
#include "core/math/Mat4.hpp"
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

constexpr float kHalfPi = 1.57079632679489662F;

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
  const Transform3 rotated{{1.0F, 2.0F, 3.0F},
                           {0.0F, kHalfPi, 0.0F},
                           {2.0F, 3.0F, 4.0F}};
  const Vec3 local{1.0F, 1.0F, 1.0F};
  const Vec3 scaleTranslate = transformPointScaleTranslate(rotated, local);
  const Vec3 trs = transformPointTrs(rotated, local);
  return expect(nearlyEqual(identity.position, vec3Zero()), "Transform position") &&
         expect(nearlyEqual(identity.rotationEulerRadians, vec3Zero()), "Transform rotation") &&
         expect(nearlyEqual(identity.scale, Vec3{1.0F, 1.0F, 1.0F}), "Transform scale") &&
         expect(hasPositiveFiniteScale(identity), "Transform positive scale") &&
         expect(nearlyEqual(scaleTranslate, Vec3{3.0F, 5.0F, 7.0F}),
                "Transform scale-translate ignores rotation") &&
         expect(nearlyEqual(transformPoint(rotated, local), scaleTranslate),
                "Transform compatibility wrapper matches scale-translate") &&
         expect(nearlyEqual(trs, Vec3{5.0F, 5.0F, 1.0F}),
                "Transform TRS honors rotation");
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

bool mat4Tests() {
  using namespace iggy3d;
  const Mat4 identity = identityMat4();
  return expect(at(identity, 0, 0) == 1.0F && at(identity, 3, 3) == 1.0F, "Mat4 identity") &&
         expect(isFinite(identity), "Mat4 identity finite") &&
         expect(nearlyEqual(transformPoint(identity, Vec3{1.0F, 2.0F, 3.0F}),
                            Vec3{1.0F, 2.0F, 3.0F}),
                "Mat4 identity transform");
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
  const bool ok = vec3Tests() && transformTests() && aabbTests() && mat4Tests() &&
                  hashTests();
  return ok ? 0 : 1;
}
