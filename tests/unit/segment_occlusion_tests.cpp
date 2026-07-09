#include "runtime/ai/SegmentOcclusion.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <span>
#include <string_view>
#include <vector>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::PhysicsAabbCollider colliderAt(iggy3d::PhysicsBodyId id,
                                       iggy3d::Vec3 center,
                                       iggy3d::Vec3 halfExtents,
                                       bool sensor = false) {
  iggy3d::PhysicsAabbCollider collider;
  collider.bodyId = id;
  collider.worldCenterMeters = center;
  collider.halfExtentsMeters = halfExtents;
  collider.bounds = iggy3d::aabbFromCenterExtents(center, halfExtents);
  collider.sensor = sensor;
  return collider;
}

std::span<const iggy3d::PhysicsAabbCollider> asSpan(
    const std::vector<iggy3d::PhysicsAabbCollider>& colliders) {
  return {colliders.data(), colliders.size()};
}

bool emptySuccessfulColliderSpanIsClear() {
  const std::vector<iggy3d::PhysicsAabbCollider> colliders;
  const iggy3d::SegmentOcclusionVerdict verdict = iggy3d::segmentOcclusion(
      asSpan(colliders), {0.0F, 1.6F, 0.0F}, {4.0F, 1.6F, 0.0F}, 0.0F);
  return expect(verdict == iggy3d::SegmentOcclusionVerdict::Clear,
                "empty successful collider span is clear");
}

bool wallSpanningEyeSegmentBlocks() {
  const std::vector<iggy3d::PhysicsAabbCollider> colliders{
      colliderAt({1U}, {2.0F, 1.0F, 0.0F}, {0.1F, 1.0F, 1.0F}),
  };
  const iggy3d::SegmentOcclusionVerdict verdict = iggy3d::segmentOcclusion(
      asSpan(colliders), {0.0F, 1.6F, 0.0F}, {4.0F, 1.6F, 0.0F}, 0.0F);
  return expect(verdict == iggy3d::SegmentOcclusionVerdict::Blocked,
                "wall spanning eye segment blocks");
}

bool originInsideWallColliderBlocks() {
  const std::vector<iggy3d::PhysicsAabbCollider> colliders{
      colliderAt({2U}, {2.0F, 1.0F, 0.0F}, {0.5F, 1.0F, 1.0F}),
  };
  const iggy3d::SegmentOcclusionVerdict verdict = iggy3d::segmentOcclusion(
      asSpan(colliders), {2.0F, 1.6F, 0.0F}, {4.0F, 1.6F, 0.0F}, 0.0F);
  return expect(verdict == iggy3d::SegmentOcclusionVerdict::Blocked,
                "origin inside wall collider blocks");
}

bool shortWallBelowEyeHeightDoesNotBlockStandingTarget() {
  const std::vector<iggy3d::PhysicsAabbCollider> colliders{
      colliderAt({3U}, {2.0F, 0.5F, 0.0F}, {0.1F, 0.5F, 1.0F}),
  };
  const iggy3d::SegmentOcclusionVerdict verdict = iggy3d::segmentOcclusion(
      asSpan(colliders), {0.0F, 1.6F, 0.0F}, {4.0F, 1.6F, 0.0F}, 0.0F);
  return expect(verdict == iggy3d::SegmentOcclusionVerdict::Clear,
                "short wall below eye-height segment is clear");
}

bool marginTurnsNearMissIntoBlocked() {
  const std::vector<iggy3d::PhysicsAabbCollider> colliders{
      colliderAt({4U}, {2.0F, 0.6F, 0.0F}, {0.5F, 0.5F, 0.5F}),
  };
  const iggy3d::SegmentOcclusionVerdict zeroMargin = iggy3d::segmentOcclusion(
      asSpan(colliders), {0.0F, 0.0F, 0.0F}, {4.0F, 0.0F, 0.0F}, 0.0F);
  const iggy3d::SegmentOcclusionVerdict padded = iggy3d::segmentOcclusion(
      asSpan(colliders), {0.0F, 0.0F, 0.0F}, {4.0F, 0.0F, 0.0F}, 0.11F);
  return expect(zeroMargin == iggy3d::SegmentOcclusionVerdict::Clear,
                "zero margin near miss is clear") &&
         expect(padded == iggy3d::SegmentOcclusionVerdict::Blocked,
                "margin near miss blocks");
}

bool invalidSegmentIsUnknown() {
  const std::vector<iggy3d::PhysicsAabbCollider> colliders{
      colliderAt({5U}, {2.0F, 1.0F, 0.0F}, {0.5F, 1.0F, 1.0F}),
  };
  const float infinity = std::numeric_limits<float>::infinity();
  const iggy3d::SegmentOcclusionVerdict degenerate =
      iggy3d::segmentOcclusion(asSpan(colliders), {0.0F, 1.6F, 0.0F},
                               {0.0F, 1.6F, 0.0F}, 0.0F);
  const iggy3d::SegmentOcclusionVerdict nonFinite =
      iggy3d::segmentOcclusion(asSpan(colliders), {infinity, 1.6F, 0.0F},
                               {4.0F, 1.6F, 0.0F}, 0.0F);
  return expect(degenerate == iggy3d::SegmentOcclusionVerdict::Unknown,
                "degenerate segment is unknown") &&
         expect(nonFinite == iggy3d::SegmentOcclusionVerdict::Unknown,
                "non-finite segment is unknown");
}

bool invalidColliderInSpanIsUnknown() {
  iggy3d::PhysicsAabbCollider invalid =
      colliderAt({6U}, {2.0F, 1.0F, 0.0F}, {0.5F, 1.0F, 1.0F});
  invalid.halfExtentsMeters.x = 0.0F;
  const std::vector<iggy3d::PhysicsAabbCollider> colliders{invalid};

  const iggy3d::SegmentOcclusionVerdict verdict = iggy3d::segmentOcclusion(
      asSpan(colliders), {0.0F, 1.6F, 0.0F}, {4.0F, 1.6F, 0.0F}, 0.0F);
  return expect(verdict == iggy3d::SegmentOcclusionVerdict::Unknown,
                "invalid collider in span is unknown");
}

bool invalidMarginIsUnknown() {
  const std::vector<iggy3d::PhysicsAabbCollider> colliders{
      colliderAt({7U}, {2.0F, 1.0F, 0.0F}, {0.5F, 1.0F, 1.0F}),
  };
  const float infinity = std::numeric_limits<float>::infinity();
  const float quietNan = std::numeric_limits<float>::quiet_NaN();
  const iggy3d::SegmentOcclusionVerdict negative =
      iggy3d::segmentOcclusion(asSpan(colliders), {0.0F, 1.6F, 0.0F},
                               {4.0F, 1.6F, 0.0F}, -0.01F);
  const iggy3d::SegmentOcclusionVerdict nonFinite =
      iggy3d::segmentOcclusion(asSpan(colliders), {0.0F, 1.6F, 0.0F},
                               {4.0F, 1.6F, 0.0F}, infinity);
  const iggy3d::SegmentOcclusionVerdict nan =
      iggy3d::segmentOcclusion(asSpan(colliders), {0.0F, 1.6F, 0.0F},
                               {4.0F, 1.6F, 0.0F}, quietNan);
  return expect(negative == iggy3d::SegmentOcclusionVerdict::Unknown,
                "negative margin is unknown") &&
         expect(nonFinite == iggy3d::SegmentOcclusionVerdict::Unknown,
                "non-finite margin is unknown") &&
         expect(nan == iggy3d::SegmentOcclusionVerdict::Unknown,
                "nan margin is unknown");
}

}  // namespace

int main() {
  const bool ok =
      emptySuccessfulColliderSpanIsClear() && wallSpanningEyeSegmentBlocks() &&
      originInsideWallColliderBlocks() &&
      shortWallBelowEyeHeightDoesNotBlockStandingTarget() &&
      marginTurnsNearMissIntoBlocked() && invalidSegmentIsUnknown() &&
      invalidColliderInSpanIsUnknown() && invalidMarginIsUnknown();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
