#include "app/iggy3d/creative/play/NpcSpawn.hpp"

#include "app/iggy3d/creative/document/Hierarchy.hpp"
#include "runtime/ai/NpcBehaviorProfile.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_set>
#include <utility>

namespace iggy3d::creative {
namespace {

constexpr double kMinimumPatrolLengthMeters = 1.0e-5;
constexpr double kDefaultPatrolDwellSeconds = 0.0;
constexpr double kDefaultPatrolSpeedMultiplier = 1.0;

void reject(CreativeNpcSpawnPlanResult& result,
            CreativeNpcSpawnPlanStatus status,
            std::string_view reasonCode,
            CreativeObjectId failedObjectId = kInvalidObjectId) noexcept {
  result.status = status;
  result.reasonCode = reasonCode;
  result.failedObjectId = failedObjectId;
  result.accepted = false;
  result.actors.clear();
  result.patrolRoutes.clear();
}

[[nodiscard]] bool actorKind(CreativeObjectKind kind) noexcept {
  return kind == CreativeObjectKind::NpcSpawn ||
         kind == CreativeObjectKind::EnemySpawn;
}

[[nodiscard]] std::string_view expectedAnchorKind(
    CreativeObjectKind kind) noexcept {
  switch (kind) {
    case CreativeObjectKind::NpcSpawn:
      return "npc";
    case CreativeObjectKind::EnemySpawn:
      return "monster";
    default:
      return {};
  }
}

[[nodiscard]] const RoomAnchorAsset* anchorForObject(
    const CreativeRoomBakeResult& roomBake,
    CreativeObjectId objectId,
    std::string_view kind) noexcept {
  const RoomAnchorAsset* matched = nullptr;
  for (const CreativeRoomBakeAnchorSource& source : roomBake.anchorSources) {
    if (source.objectId != objectId) {
      continue;
    }
    const auto anchor = std::find_if(
        roomBake.room.anchors.begin(), roomBake.room.anchors.end(),
        [&source, kind](const RoomAnchorAsset& candidate) {
          return candidate.id == source.anchorId && candidate.kind == kind;
        });
    if (anchor == roomBake.room.anchors.end() || matched != nullptr) {
      return nullptr;
    }
    matched = &*anchor;
  }
  return matched;
}

[[nodiscard]] bool anchorIsValid(const RoomAnchorAsset& anchor) noexcept {
  return !anchor.id.empty() && !anchor.kind.empty() &&
         !anchor.runtimeStableName.empty() && isFinite(anchor.positionMeters);
}

[[nodiscard]] bool pathUsesSupportedTiming(
    std::span<const CreativePathPoint> points) noexcept {
  return std::all_of(
      points.begin(), points.end(), [](const CreativePathPoint& point) {
        return point.dwellSeconds == kDefaultPatrolDwellSeconds &&
               point.outgoingSpeedMultiplier ==
                   kDefaultPatrolSpeedMultiplier;
      });
}

[[nodiscard]] bool pathIsValid(
    std::span<const CreativePathPoint> points) noexcept {
  if (points.size() < 2U ||
      points.size() > kCreativeNpcPatrolWaypointCapacity) {
    return false;
  }
  double lengthMeters = 0.0;
  for (std::size_t index = 0U; index < points.size(); ++index) {
    if (!isValidCreativePathPoint(points[index])) {
      return false;
    }
    const Vec3 runtimePoint{
        static_cast<float>(points[index].position.x),
        static_cast<float>(points[index].position.y),
        static_cast<float>(points[index].position.z),
    };
    if (!isFinite(runtimePoint)) {
      return false;
    }
    if (index == 0U) {
      continue;
    }
    const CreativeVec3& from = points[index - 1U].position;
    const CreativeVec3& to = points[index].position;
    lengthMeters +=
        std::hypot(to.x - from.x, to.y - from.y, to.z - from.z);
  }
  return std::isfinite(lengthMeters) &&
         lengthMeters > kMinimumPatrolLengthMeters;
}

[[nodiscard]] bool roomActorAnchorsHavePlans(
    const CreativeRoomBakeResult& roomBake,
    std::span<const CreativeNpcSpawnPlan> plans) noexcept {
  for (const RoomAnchorAsset& anchor : roomBake.room.anchors) {
    if (anchor.kind != "npc" && anchor.kind != "monster") {
      continue;
    }
    const std::size_t count = static_cast<std::size_t>(std::count_if(
        plans.begin(), plans.end(),
        [&anchor](const CreativeNpcSpawnPlan& plan) {
          return plan.anchor.id == anchor.id &&
                 plan.anchor.runtimeStableName == anchor.runtimeStableName;
        }));
    if (count != 1U) {
      return false;
    }
  }
  return true;
}

}  // namespace

std::string_view toString(CreativeNpcSpawnPlanStatus status) noexcept {
  switch (status) {
    case CreativeNpcSpawnPlanStatus::NotRequested:
      return "not_requested";
    case CreativeNpcSpawnPlanStatus::MissingDocument:
      return "missing_document";
    case CreativeNpcSpawnPlanStatus::InvalidDocument:
      return "invalid_document";
    case CreativeNpcSpawnPlanStatus::MissingRoomBake:
      return "missing_room_bake";
    case CreativeNpcSpawnPlanStatus::InvalidRoomBake:
      return "invalid_room_bake";
    case CreativeNpcSpawnPlanStatus::MissingSpawnAnchor:
      return "missing_spawn_anchor";
    case CreativeNpcSpawnPlanStatus::InvalidSpawnAnchor:
      return "invalid_spawn_anchor";
    case CreativeNpcSpawnPlanStatus::InvalidSettings:
      return "invalid_settings";
    case CreativeNpcSpawnPlanStatus::UnsupportedBehaviorProfile:
      return "unsupported_behavior_profile";
    case CreativeNpcSpawnPlanStatus::InvalidFacing:
      return "invalid_facing";
    case CreativeNpcSpawnPlanStatus::InvalidPatrolOwner:
      return "invalid_patrol_owner";
    case CreativeNpcSpawnPlanStatus::InvalidPatrolRoute:
      return "invalid_patrol_route";
    case CreativeNpcSpawnPlanStatus::UnsupportedPatrolTiming:
      return "unsupported_patrol_timing";
    case CreativeNpcSpawnPlanStatus::DuplicateRuntimeIdentity:
      return "duplicate_runtime_identity";
    case CreativeNpcSpawnPlanStatus::Ready:
      return "ready";
  }
  return "not_requested";
}

bool isValidCreativeNpcSpawnPlan(
    const CreativeNpcSpawnPlan& plan) noexcept {
  if (plan.objectId == kInvalidObjectId || !actorKind(plan.objectKind) ||
      plan.anchor.kind != expectedAnchorKind(plan.objectKind) ||
      !anchorIsValid(plan.anchor) || !std::isfinite(plan.yawRadians) ||
      !isFinite(plan.facingDirection) ||
      !isValidCreativeNpcSpawnSettings(plan.settings) ||
      !isSupportedCreativeNpcBehaviorProfileId(
          plan.settings.behaviorProfileId)) {
    return false;
  }
  const Vec3 expectedFacing{std::sin(plan.yawRadians), 0.0F,
                            -std::cos(plan.yawRadians)};
  constexpr float kFacingEpsilon = 1.0e-5F;
  if (std::fabs(plan.facingDirection.x - expectedFacing.x) >
          kFacingEpsilon ||
      std::fabs(plan.facingDirection.y) > kFacingEpsilon ||
      std::fabs(plan.facingDirection.z - expectedFacing.z) >
          kFacingEpsilon) {
    return false;
  }
  if (!plan.hasPatrol()) {
    return true;
  }
  return plan.patrolRouteObjectId != kInvalidObjectId;
}

bool isValidCreativeNpcPatrolRoutePlan(
    const CreativeNpcPatrolRoutePlan& plan) noexcept {
  return plan.objectId != kInvalidObjectId && pathIsValid(plan.path) &&
         pathUsesSupportedTiming(plan.path);
}

bool isSupportedCreativeNpcBehaviorProfileId(std::string_view profileId) {
  if (profileId.empty()) {
    return true;
  }
  static const NpcBehaviorProfileCatalog catalog =
      makeBuiltInNpcBehaviorProfileCatalog();
  return resolveNpcBehaviorProfile({&catalog, profileId}).ok;
}

CreativeNpcSpawnPlanResult planCreativeNpcSpawns(
    const CreativeNpcSpawnPlanRequest& request) {
  CreativeNpcSpawnPlanResult result;
  result.requested = true;
  if (request.document == nullptr) {
    reject(result, CreativeNpcSpawnPlanStatus::MissingDocument,
           "creative_npc_spawn_document_missing");
    return result;
  }
  if (!request.document->isValid()) {
    reject(result, CreativeNpcSpawnPlanStatus::InvalidDocument,
           "creative_npc_spawn_document_invalid");
    return result;
  }
  if (request.roomBake == nullptr || !request.roomBake->receipt.accepted) {
    reject(result, CreativeNpcSpawnPlanStatus::MissingRoomBake,
           "creative_npc_spawn_room_bake_missing");
    return result;
  }
  if (request.roomBake->room.id.empty()) {
    reject(result, CreativeNpcSpawnPlanStatus::InvalidRoomBake,
           "creative_npc_spawn_room_bake_invalid");
    return result;
  }

  std::vector<const CreativeObject*> sourceActors;
  std::unordered_set<CreativeObjectId> visibleRouteIds;
  for (const CreativeObject& object : request.document->objects()) {
    const bool included =
        request.includeHidden ||
        creativeObjectEffectivelyVisible(*request.document, object.id);
    if (!included) {
      continue;
    }
    if (actorKind(object.kind)) {
      sourceActors.push_back(&object);
    } else if (object.kind == CreativeObjectKind::PatrolRoute) {
      visibleRouteIds.insert(object.id);
    }
  }
  std::sort(sourceActors.begin(), sourceActors.end(),
            [](const CreativeObject* left, const CreativeObject* right) {
              return left->id < right->id;
            });
  result.sourceActorCount = sourceActors.size();
  result.actors.reserve(sourceActors.size());

  std::unordered_set<std::string> runtimeStableNames;
  std::unordered_set<CreativeObjectId> assignedRouteIds;
  for (const CreativeObject* object : sourceActors) {
    CreativeNpcSpawnPlan plan;
    plan.objectId = object->id;
    plan.objectKind = object->kind;
    plan.settings = object->npcSpawn;
    if (!isValidCreativeNpcSpawnSettings(plan.settings)) {
      reject(result, CreativeNpcSpawnPlanStatus::InvalidSettings,
             "creative_npc_spawn_settings_invalid", object->id);
      return result;
    }
    if (!isSupportedCreativeNpcBehaviorProfileId(
            plan.settings.behaviorProfileId)) {
      reject(result, CreativeNpcSpawnPlanStatus::UnsupportedBehaviorProfile,
             "creative_npc_spawn_profile_unsupported", object->id);
      return result;
    }
    if (plan.settings.spawnPolicy == CreativeNpcSpawnPolicy::Disabled) {
      ++result.disabledActorCount;
    }
    plan.yawRadians =
        static_cast<float>(object->transform.rotationEulerRadians.y);
    if (!std::isfinite(plan.yawRadians)) {
      reject(result, CreativeNpcSpawnPlanStatus::InvalidFacing,
             "creative_npc_spawn_facing_invalid", object->id);
      return result;
    }
    plan.facingDirection =
        {std::sin(plan.yawRadians), 0.0F, -std::cos(plan.yawRadians)};

    const RoomAnchorAsset* anchor =
        anchorForObject(*request.roomBake, object->id,
                        expectedAnchorKind(object->kind));
    if (anchor == nullptr) {
      reject(result, CreativeNpcSpawnPlanStatus::MissingSpawnAnchor,
             "creative_npc_spawn_anchor_missing", object->id);
      return result;
    }
    if (!anchorIsValid(*anchor)) {
      reject(result, CreativeNpcSpawnPlanStatus::InvalidSpawnAnchor,
             "creative_npc_spawn_anchor_invalid", object->id);
      return result;
    }
    plan.anchor = *anchor;
    if (!runtimeStableNames.insert(plan.anchor.runtimeStableName).second) {
      reject(result, CreativeNpcSpawnPlanStatus::DuplicateRuntimeIdentity,
             "creative_npc_spawn_runtime_identity_duplicate", object->id);
      return result;
    }

    if (object->parentId.has_value()) {
      const CreativeObject* route =
          request.document->findObject(*object->parentId);
      if (route == nullptr || route->kind != CreativeObjectKind::PatrolRoute ||
          !object->attachmentSocket.empty()) {
        reject(result, CreativeNpcSpawnPlanStatus::InvalidPatrolOwner,
               "creative_npc_spawn_patrol_owner_invalid", object->id);
        return result;
      }
      if (!pathIsValid(route->pathPoints)) {
        reject(result, CreativeNpcSpawnPlanStatus::InvalidPatrolRoute,
               "creative_npc_spawn_patrol_route_invalid", route->id);
        return result;
      }
      if (!pathUsesSupportedTiming(route->pathPoints)) {
        reject(result, CreativeNpcSpawnPlanStatus::UnsupportedPatrolTiming,
               "creative_npc_spawn_patrol_timing_unsupported", route->id);
        return result;
      }
      plan.patrolRouteObjectId = route->id;
      ++result.patrollingActorCount;
      if (assignedRouteIds.insert(route->id).second) {
        result.patrolWaypointCount += route->pathPoints.size();
        result.patrolRoutes.push_back({route->id, route->pathPoints});
      }
    } else {
      ++result.stationaryActorCount;
    }
    result.actors.push_back(std::move(plan));
  }

  if (!roomActorAnchorsHavePlans(*request.roomBake, result.actors)) {
    reject(result, CreativeNpcSpawnPlanStatus::InvalidRoomBake,
           "creative_npc_spawn_room_actor_unowned");
    return result;
  }
  result.assignedPatrolRouteCount = assignedRouteIds.size();
  std::sort(result.patrolRoutes.begin(), result.patrolRoutes.end(),
            [](const CreativeNpcPatrolRoutePlan& left,
               const CreativeNpcPatrolRoutePlan& right) {
              return left.objectId < right.objectId;
            });
  for (CreativeObjectId routeId : visibleRouteIds) {
    result.unassignedPatrolRouteCount +=
        assignedRouteIds.contains(routeId) ? 0U : 1U;
  }
  result.status = CreativeNpcSpawnPlanStatus::Ready;
  result.reasonCode = "creative_npc_spawn_plan_ready";
  result.accepted = true;
  return result;
}

}  // namespace iggy3d::creative
