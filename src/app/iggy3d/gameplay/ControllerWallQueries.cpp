#include "app/iggy3d/gameplay/ControllerWallQueries.hpp"

#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/gameplay/ControllerKinematics.hpp"
#include "app/iggy3d/gameplay/MovementTuning.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>

namespace iggy3d {
namespace {

constexpr float kPi = 3.14159265358979323846F;
constexpr float kMovementStateDistanceEpsilonMeters = 0.0001F;
constexpr float kWallRunSurfaceVerticalSlackMeters = 0.35F;
constexpr float kWallRunAlongWallDotThreshold = 0.35F;

bool actorBlockingSurface(const CollisionSurfaceView& surface) {
  return surface.blocksActor || surface.hasActorMask;
}

bool hasTraversalTag(const CollisionSurfaceView& surface, std::string_view expected) {
  for (const std::string& tag : surface.traversalTags) {
    // branch-gate: BG-1157
    if (tag == expected) {
      return true;
    }
  }
  return false;
}

bool horizontalNormal(Vec3 normal, Vec3& out) {
  normal.y = 0.0F;
  const float lenSq = lengthSquared(normal);
  // branch-gate: BG-1157
  if (!isFinite(normal) || lenSq <= 0.0001F) {
    return false;
  }
  out = normal / std::sqrt(lenSq);
  return true;
}

bool isNearVerticalSurface(Vec3 position,
                           const CollisionSurfaceView& surface,
                           Vec3& awayNormal,
                           float& distanceSq,
                           const ProductGameplayMovementTuning& tuning) {
  // branch-gate: BG-1157
  if (!actorBlockingSurface(surface) || surface.opening ||
      surface.role != CollisionSurfaceRole::Blocker ||
      !hasTraversalTag(surface, "wall_jump") ||
      !isValid(surface.bounds)) {
    return false;
  }

  Vec3 normal;
  // branch-gate: BG-1157
  if (!horizontalNormal(surface.normal, normal)) {
    return false;
  }

  const float verticalSlack = 0.35F;
  // branch-gate: BG-1157
  if (position.y < surface.bounds.min.y - verticalSlack ||
      position.y > surface.bounds.max.y + verticalSlack) {
    return false;
  }

  const float clampedX =
      std::clamp(position.x, surface.bounds.min.x, surface.bounds.max.x);
  const float clampedZ =
      std::clamp(position.z, surface.bounds.min.z, surface.bounds.max.z);
  const Vec3 nearest{clampedX, position.y, clampedZ};
  Vec3 fromSurface = position - nearest;
  fromSurface.y = 0.0F;
  const float fromSurfaceSq = lengthSquared(fromSurface);
  // branch-gate: BG-1157
  if (fromSurfaceSq > 0.0001F) {
    awayNormal = fromSurface / std::sqrt(fromSurfaceSq);
  } else {
    const Vec3 centerToPlayer = position - center(surface.bounds);
    // branch-gate: BG-1157
    awayNormal = dot(centerToPlayer, normal) < 0.0F ? normal * -1.0F : normal;
  }

  distanceSq = fromSurfaceSq;
  return distanceSq <= tuning.wallJumpProbeMeters * tuning.wallJumpProbeMeters;
}

bool isNearWallRunSurface(Vec3 position,
                          const CollisionSurfaceView& surface,
                          Vec3& awayNormal,
                          float& distanceSq,
                          const ProductGameplayMovementTuning& tuning) {
  // branch-gate: BG-1157
  if (!actorBlockingSurface(surface) || surface.opening ||
      surface.role != CollisionSurfaceRole::Blocker ||
      !isValid(surface.bounds) ||
      std::fabs(surface.normal.y) >
          std::clamp(tuning.wallRunMaxWallNormalY, 0.0F, 1.0F)) {
    return false;
  }

  Vec3 normal;
  // branch-gate: BG-1157
  if (!horizontalNormal(surface.normal, normal)) {
    return false;
  }

  // branch-gate: BG-1157
  if (position.y < surface.bounds.min.y - kWallRunSurfaceVerticalSlackMeters ||
      position.y > surface.bounds.max.y + kWallRunSurfaceVerticalSlackMeters) {
    return false;
  }

  const float clampedX =
      std::clamp(position.x, surface.bounds.min.x, surface.bounds.max.x);
  const float clampedZ =
      std::clamp(position.z, surface.bounds.min.z, surface.bounds.max.z);
  const Vec3 nearest{clampedX, position.y, clampedZ};
  Vec3 fromSurface = position - nearest;
  fromSurface.y = 0.0F;
  const float fromSurfaceSq = lengthSquared(fromSurface);
  // branch-gate: BG-1157
  if (fromSurfaceSq > 0.0001F) {
    awayNormal = fromSurface / std::sqrt(fromSurfaceSq);
  } else {
    const Vec3 centerToPlayer = position - center(surface.bounds);
    // branch-gate: BG-1157
    awayNormal = dot(centerToPlayer, normal) < 0.0F ? normal * -1.0F : normal;
  }

  distanceSq = fromSurfaceSq;
  return distanceSq <= tuning.wallJumpProbeMeters * tuning.wallJumpProbeMeters;
}

}  // namespace

const CollisionSurfaceView* findWallJumpSurface(
    const SpatialSurfaceSet& surfaces,
    Vec3 position,
    Vec3& awayNormal,
    const ProductGameplayMovementTuning& tuning) {
  const CollisionSurfaceView* best = nullptr;
  float bestDistanceSq = tuning.wallJumpProbeMeters * tuning.wallJumpProbeMeters;
  for (const CollisionSurfaceView& surface : surfaces.surfaces()) {
    Vec3 candidateNormal;
    float candidateDistanceSq = 0.0F;
    // branch-gate: BG-1157
    if (!isNearVerticalSurface(position, surface, candidateNormal,
                               candidateDistanceSq, tuning)) {
      continue;
    }
    // branch-gate: BG-1157
    if (best != nullptr && candidateDistanceSq >= bestDistanceSq) {
      continue;
    }
    best = &surface;
    bestDistanceSq = candidateDistanceSq;
    awayNormal = candidateNormal;
  }
  return best;
}

const CollisionSurfaceView* findWallRunSurface(
    const SpatialSurfaceSet& surfaces,
    Vec3 position,
    Vec3& awayNormal,
    const ProductGameplayMovementTuning& tuning) {
  const CollisionSurfaceView* best = nullptr;
  float bestDistanceSq = tuning.wallJumpProbeMeters * tuning.wallJumpProbeMeters;
  for (const CollisionSurfaceView& surface : surfaces.surfaces()) {
    Vec3 candidateNormal;
    float candidateDistanceSq = 0.0F;
    // branch-gate: BG-1157
    if (!isNearWallRunSurface(position,
                              surface,
                              candidateNormal,
                              candidateDistanceSq,
                              tuning)) {
      continue;
    }
    // branch-gate: BG-1157
    if (best != nullptr && candidateDistanceSq >= bestDistanceSq) {
      continue;
    }
    best = &surface;
    bestDistanceSq = candidateDistanceSq;
    awayNormal = candidateNormal;
  }
  return best;
}

std::string wallRunSideName(Vec3 awayNormal, float yawDegrees) {
  const float yawRadians = yawDegrees * kPi / 180.0F;
  const Vec3 forward{std::sin(yawRadians), 0.0F, -std::cos(yawRadians)};
  const Vec3 right{std::cos(yawRadians), 0.0F, std::sin(yawRadians)};
  const float rightDot = dot(awayNormal, right);
  const float forwardDot = dot(awayNormal, forward);
  // branch-gate: BG-1157
  if (std::fabs(rightDot) >= 0.35F) {
    // branch-gate: BG-1157
    return rightDot > 0.0F ? "left" : "right";
  }
  // branch-gate: BG-1157
  if (std::fabs(forwardDot) >= 0.35F) {
    // branch-gate: BG-1157
    return forwardDot < 0.0F ? "front" : "back";
  }
  return "unknown";
}

bool productMovementDebugAlongWall(const ProductAppWindowState& window,
                                   Vec3 awayNormal) {
  Vec3 travel{window.gameplay.gameplayMovement.finalX - window.gameplay.gameplayMovement.startX,
              0.0F,
              window.gameplay.gameplayMovement.finalZ - window.gameplay.gameplayMovement.startZ};
  const float travelLenSq = lengthSquared(travel);
  // branch-gate: BG-1161
  if (!isFinite(travel) || travelLenSq <= kMovementStateDistanceEpsilonMeters) {
    return false;
  }
  travel = travel / std::sqrt(travelLenSq);
  const Vec3 tangent{-awayNormal.z, 0.0F, awayNormal.x};
  return std::fabs(dot(travel, tangent)) >= kWallRunAlongWallDotThreshold;
}

bool wallRunProofNormal(const ProductAppWindowState& window, Vec3& normal) {
  normal = {window.gameplay.gameplayWallRun.normalX, 0.0F, window.gameplay.gameplayWallRun.normalZ};
  const float lenSq = lengthSquared(normal);
  // branch-gate: BG-1157
  if (!isFinite(normal) || lenSq <= 0.0001F) {
    return false;
  }
  normal = normal / std::sqrt(lenSq);
  return true;
}

bool wallRunTangentDirectionFromNormal(const ProductAppWindowState& window,
                                       Vec3 normal,
                                       float moveX,
                                       float moveY,
                                       Vec3& direction) {
  normal.y = 0.0F;
  const float lenSq = lengthSquared(normal);
  // branch-gate: BG-1157
  if (!isFinite(normal) || lenSq <= 0.0001F) {
    return false;
  }
  normal = normal / std::sqrt(lenSq);
  const Vec3 desired = productManualFirstPersonDirection(
      moveX, moveY, window.viewport.cameraYawDegrees);
  const Vec3 tangent{-normal.z, 0.0F, normal.x};
  const float tangentDot = dot(desired, tangent);
  // branch-gate: BG-1157
  if (std::fabs(tangentDot) < kWallRunAlongWallDotThreshold) {
    return false;
  }
  // branch-gate: BG-1157
  direction = tangentDot >= 0.0F ? tangent : tangent * -1.0F;
  return true;
}

bool wallRunTangentDirection(const ProductAppWindowState& window,
                             float moveX,
                             float moveY,
                             Vec3& direction) {
  Vec3 normal;
  // branch-gate: BG-1157
  if (!wallRunProofNormal(window, normal)) {
    return false;
  }
  return wallRunTangentDirectionFromNormal(
      window, normal, moveX, moveY, direction);
}

}  // namespace iggy3d
