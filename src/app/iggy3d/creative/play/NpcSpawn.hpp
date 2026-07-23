#pragma once

#include "app/iggy3d/creative/adapters/RoomBake.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

inline constexpr std::size_t kCreativeNpcPatrolWaypointCapacity = 256U;

enum class CreativeNpcSpawnPlanStatus : std::uint8_t {
  NotRequested,
  MissingDocument,
  InvalidDocument,
  MissingRoomBake,
  InvalidRoomBake,
  MissingSpawnAnchor,
  InvalidSpawnAnchor,
  InvalidFacing,
  InvalidPatrolOwner,
  InvalidPatrolRoute,
  UnsupportedPatrolTiming,
  DuplicateRuntimeIdentity,
  Ready,
};

// Canonical activation data for one authored NPC or enemy. A parentless actor
// is stationary. A patrolling actor explicitly names its owning PatrolRoute;
// document or bake ordering never assigns route ownership.
struct CreativeNpcSpawnPlan {
  CreativeObjectId objectId = kInvalidObjectId;
  CreativeObjectKind objectKind = CreativeObjectKind::Unknown;
  RoomAnchorAsset anchor;
  float yawRadians = 0.0F;
  Vec3 facingDirection;
  CreativeObjectId patrolRouteObjectId = kInvalidObjectId;

  [[nodiscard]] bool hasPatrol() const noexcept {
    return patrolRouteObjectId != kInvalidObjectId;
  }
};

// Routes are normalized separately from actors so a shared route is stored and
// validated once rather than copying up to 256 points into every actor plan.
struct CreativeNpcPatrolRoutePlan {
  CreativeObjectId objectId = kInvalidObjectId;
  std::vector<CreativePathPoint> path;
};

struct CreativeNpcSpawnPlanRequest {
  const CreativeDocument* document = nullptr;
  const CreativeRoomBakeResult* roomBake = nullptr;
  bool includeHidden = false;
};

struct CreativeNpcSpawnPlanResult {
  bool requested = false;
  bool accepted = false;
  CreativeNpcSpawnPlanStatus status =
      CreativeNpcSpawnPlanStatus::NotRequested;
  std::string_view reasonCode = "creative_npc_spawn_plan_not_requested";
  CreativeObjectId failedObjectId = kInvalidObjectId;
  std::size_t sourceActorCount = 0U;
  std::size_t stationaryActorCount = 0U;
  std::size_t patrollingActorCount = 0U;
  std::size_t assignedPatrolRouteCount = 0U;
  std::size_t unassignedPatrolRouteCount = 0U;
  std::size_t patrolWaypointCount = 0U;
  std::vector<CreativeNpcSpawnPlan> actors;
  std::vector<CreativeNpcPatrolRoutePlan> patrolRoutes;
};

[[nodiscard]] std::string_view toString(
    CreativeNpcSpawnPlanStatus status) noexcept;
[[nodiscard]] bool isValidCreativeNpcSpawnPlan(
    const CreativeNpcSpawnPlan& plan) noexcept;
[[nodiscard]] bool isValidCreativeNpcPatrolRoutePlan(
    const CreativeNpcPatrolRoutePlan& plan) noexcept;
[[nodiscard]] CreativeNpcSpawnPlanResult planCreativeNpcSpawns(
    const CreativeNpcSpawnPlanRequest& request);

}  // namespace iggy3d::creative
