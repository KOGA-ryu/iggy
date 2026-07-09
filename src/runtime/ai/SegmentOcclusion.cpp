#include "runtime/ai/SegmentOcclusion.hpp"

#include <cmath>

#include "runtime/physics/PhysicsCollisionQueries.hpp"

namespace iggy3d {
namespace {

constexpr float kSegmentEpsilonSquared = 0.000001F;

bool validMargin(float marginMeters) {
  return std::isfinite(marginMeters) && marginMeters >= 0.0F;
}

bool validSegment(Vec3 fromEye, Vec3 toEye) {
  if (!isFinite(fromEye) || !isFinite(toEye)) {
    return false;
  }
  const float segmentLengthSquared = lengthSquared(toEye - fromEye);
  return std::isfinite(segmentLengthSquared) &&
         segmentLengthSquared > kSegmentEpsilonSquared;
}

bool validColliders(std::span<const PhysicsAabbCollider> colliders) {
  for (const PhysicsAabbCollider& collider : colliders) {
    if (!isValidPhysicsAabbCollider(collider)) {
      return false;
    }
  }
  return true;
}

}  // namespace

SegmentOcclusionVerdict segmentOcclusion(
    std::span<const PhysicsAabbCollider> colliders,
    Vec3 fromEye,
    Vec3 toEye,
    float marginMeters) {
  if (!validSegment(fromEye, toEye) || !validMargin(marginMeters) ||
      !validColliders(colliders)) {
    return SegmentOcclusionVerdict::Unknown;
  }
  if (colliders.empty()) {
    return SegmentOcclusionVerdict::Clear;
  }

  bool startInside = false;
  const bool hit = segmentHitsAnyPhysicsAabb(
      colliders, fromEye, toEye, marginMeters, &startInside);
  return hit ? SegmentOcclusionVerdict::Blocked
             : SegmentOcclusionVerdict::Clear;
}

}  // namespace iggy3d
