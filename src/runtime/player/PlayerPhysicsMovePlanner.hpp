#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/physics/PhysicsKinematicMotor.hpp"
#include "runtime/physics/PhysicsSpatialSurfaceColliderBake.hpp"

namespace iggy3d {

enum class PlayerPhysicsMovePlannerStatus : std::uint8_t {
  Planned,
  MissingCollisionSurfaces,
  InvalidStartCenter,
  InvalidBodyHalfExtents,
  InvalidBodyId,
  InvalidDesiredDisplacement,
  InvalidSurfaceBakeConfig,
  InvalidMotorConfig,
  SurfaceBakeFailed,
  MotorPlanFailed,
};

struct PlayerPhysicsMovePlannerConfig {
  PhysicsSpatialSurfaceColliderBakeConfig surfaceBake;
  PhysicsKinematicMotorConfig motor;
};

struct PlayerPhysicsMovePlannerRequest {
  const SpatialSurfaceSet* collisionSurfaces = nullptr;
  Vec3 startCenterMeters;
  Vec3 bodyHalfExtentsMeters{0.35F, 0.90F, 0.35F};
  Vec3 desiredDisplacementMeters;
  bool includeSensors = false;
  PhysicsBodyId physicsBodyId{1U};
  PlayerPhysicsMovePlannerConfig config;
  const PhysicsSpatialSurfaceColliderBakeResult* precomputedSurfaceBake =
      nullptr;
};

struct PlayerPhysicsMovePlannerResult {
  bool ok = false;
  PlayerPhysicsMovePlannerStatus status =
      PlayerPhysicsMovePlannerStatus::MissingCollisionSurfaces;
  std::string_view reasonCode =
      "player_physics_move_planner_missing_collision_surfaces";
  std::string_view upstreamReasonCode;
  Vec3 startCenterMeters;
  Vec3 finalCenterMeters;
  Vec3 appliedDisplacementMeters;
  Vec3 remainingDisplacementMeters;
  bool grounded = false;
  bool snappedToGround = false;
  bool blocked = false;
  std::size_t hitCount = 0U;
  std::size_t iterationCount = 0U;
  std::size_t bakedSurfaceCount = 0U;
  std::size_t bakedColliderCount = 0U;
  std::size_t skippedSurfaceCount = 0U;
  std::size_t invalidSurfaceIndex = 0U;
  PhysicsBodyId firstHitBodyId;
  std::string firstHitSourceSurfaceId;
  std::vector<PhysicsKinematicMotorHit> hits;
  std::vector<std::string> hitSourceSurfaceIds;
  bool debugGeometryAvailable = false;
  std::vector<PhysicsAabbCollider> debugAabbColliders;
  std::vector<std::string> debugAabbSourceSurfaceIds;
};

std::string_view playerPhysicsMovePlannerStatusName(
    PlayerPhysicsMovePlannerStatus status);
bool isValidPlayerPhysicsMovePlannerConfig(
    const PlayerPhysicsMovePlannerConfig& config);
PlayerPhysicsMovePlannerResult planPlayerPhysicsMove(
    const PlayerPhysicsMovePlannerRequest& request);

}  // namespace iggy3d
