#include "app/iggy3d/creative/play/PlayerSpawn.hpp"

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/adapters/RoomBakeReachability.hpp"
#include "app/iggy3d/creative/document/Hierarchy.hpp"
#include "core/math/Aabb3.hpp"
#include "runtime/collision/CollisionQuery.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace iggy3d::creative {
namespace {

constexpr float kFloorFootprintToleranceMeters = 0.001F;

void setStatus(CreativePlayerSpawnPlan& plan,
               CreativePlayerSpawnStatus status,
               std::string_view reasonCode,
               bool accepted = false) noexcept {
  plan.status = status;
  plan.reasonCode = reasonCode;
  plan.accepted = accepted;
}

void setStatus(CreativePlayerSpawnResolveResult& result,
               CreativePlayerSpawnStatus status,
               std::string_view reasonCode,
               bool accepted = false) noexcept {
  result.status = status;
  result.reasonCode = reasonCode;
  result.accepted = accepted;
}

[[nodiscard]] const RoomAnchorAsset* anchorForObject(
    const CreativeRoomBakeResult& roomBake,
    CreativeObjectId objectId) noexcept {
  const auto source = std::find_if(
      roomBake.anchorSources.begin(), roomBake.anchorSources.end(),
      [objectId](const CreativeRoomBakeAnchorSource& candidate) {
        return candidate.objectId == objectId;
      });
  if (source == roomBake.anchorSources.end()) {
    return nullptr;
  }
  const auto anchor = std::find_if(
      roomBake.room.anchors.begin(), roomBake.room.anchors.end(),
      [&](const RoomAnchorAsset& candidate) {
        return candidate.id == source->anchorId && candidate.kind == "spawn";
      });
  return anchor == roomBake.room.anchors.end() ? nullptr : &*anchor;
}

[[nodiscard]] bool withinDocumentBounds(const CreativeBounds& bounds,
                                        Vec3 foot,
                                        float radius,
                                        float height) noexcept {
  const CreativeBoundsMetrics metrics = measureCreativeBounds(bounds);
  if (!metrics.valid || !isPositiveCreativeVec3(metrics.size)) {
    return true;
  }
  return static_cast<double>(foot.x - radius) >= bounds.min.x &&
         static_cast<double>(foot.x + radius) <= bounds.max.x &&
         static_cast<double>(foot.y) >= bounds.min.y &&
         static_cast<double>(foot.y + height) <= bounds.max.y &&
         static_cast<double>(foot.z - radius) >= bounds.min.z &&
         static_cast<double>(foot.z + radius) <= bounds.max.z;
}

[[nodiscard]] bool resolveSupportedFloor(const SpatialSurfaceSet& surfaces,
                                         Vec3 authored,
                                         float radius,
                                         float& floorHeight) noexcept {
  const float maxHeight =
      authored.y + static_cast<float>(kDefaultPlayerGroundSnapMeters);
  const std::array<Vec3, 5U> samples{{
      authored,
      {authored.x - radius, authored.y, authored.z},
      {authored.x + radius, authored.y, authored.z},
      {authored.x, authored.y, authored.z - radius},
      {authored.x, authored.y, authored.z + radius},
  }};
  bool first = true;
  for (const Vec3 sample : samples) {
    const CollisionQueryResult ground = sampleSurfaceHeightAtOrBelow(
        surfaces, sample, maxHeight, kFloorFootprintToleranceMeters);
    if (ground.status != CollisionQueryStatus::Hit ||
        std::fabs(ground.heightMeters - authored.y) >
            static_cast<float>(kDefaultPlayerGroundSnapMeters)) {
      return false;
    }
    if (first) {
      floorHeight = ground.heightMeters;
      first = false;
    } else if (std::fabs(ground.heightMeters - floorHeight) >
               static_cast<float>(kDefaultPlayerStepHeightMeters)) {
      return false;
    }
  }
  return !first;
}

[[nodiscard]] CollisionQueryResult standingBodyOverlap(
    const SpatialSurfaceSet& surfaces,
    Vec3 foot,
    float radius,
    float height) noexcept {
  const float skin = static_cast<float>(kDefaultPlayerSkinMeters);
  const Aabb3 body = makeAabb3(
      {foot.x - radius, foot.y + skin, foot.z - radius},
      {foot.x + radius, foot.y + height, foot.z + radius});
  return queryAabbOverlap(surfaces, body, CollisionQueryKind::Actor);
}

[[nodiscard]] bool reachabilitySeedKind(std::string_view kind) noexcept {
  return kind == "spawn" || kind == "npc" || kind == "monster";
}

[[nodiscard]] CreativeRoomBakeReachabilityReceipt validateFromSpawn(
    const RoomAsset& sourceRoom,
    const RoomAnchorAsset& spawn,
    float cellSizeMeters) {
  RoomAsset room = sourceRoom;
  std::erase_if(room.anchors, [](const RoomAnchorAsset& anchor) {
    return reachabilitySeedKind(anchor.kind);
  });
  room.anchors.push_back(spawn);
  CreativeRoomBakeRequest request;
  request.validateReachability = true;
  request.reachabilityCellSizeMeters = cellSizeMeters;
  return validateCreativeRoomBakeReachability(room, request);
}

}  // namespace

std::string_view toString(CreativePlayerSpawnStatus status) noexcept {
  switch (status) {
    case CreativePlayerSpawnStatus::NotRequested:
      return "not_requested";
    case CreativePlayerSpawnStatus::MissingDocument:
      return "missing_document";
    case CreativePlayerSpawnStatus::InvalidDocument:
      return "invalid_document";
    case CreativePlayerSpawnStatus::MissingRoomBake:
      return "missing_room_bake";
    case CreativePlayerSpawnStatus::InvalidObject:
      return "invalid_object";
    case CreativePlayerSpawnStatus::InvalidSettings:
      return "invalid_settings";
    case CreativePlayerSpawnStatus::UnsupportedProfile:
      return "unsupported_profile";
    case CreativePlayerSpawnStatus::OutsideWorldBounds:
      return "outside_world_bounds";
    case CreativePlayerSpawnStatus::UnsupportedFloor:
      return "unsupported_floor";
    case CreativePlayerSpawnStatus::Obstructed:
      return "obstructed";
    case CreativePlayerSpawnStatus::Unreachable:
      return "unreachable";
    case CreativePlayerSpawnStatus::GroupUnavailable:
      return "group_unavailable";
    case CreativePlayerSpawnStatus::Ready:
      return "ready";
  }
  return "not_requested";
}

bool isSupportedCreativePlayerProfileId(
    std::string_view profileId) noexcept {
  return profileId == kCreativeDefaultPlayerProfileId;
}

CreativePlayerSpawnPlan planCreativePlayerSpawn(
    const CreativePlayerSpawnPlanRequest& request) {
  CreativePlayerSpawnPlan plan;
  plan.requested = true;
  if (request.document == nullptr) {
    setStatus(plan, CreativePlayerSpawnStatus::MissingDocument,
              "creative_player_spawn_document_missing");
    return plan;
  }
  if (!request.document->isValid()) {
    setStatus(plan, CreativePlayerSpawnStatus::InvalidDocument,
              "creative_player_spawn_document_invalid");
    return plan;
  }
  if (request.roomBake == nullptr || !request.roomBake->receipt.accepted) {
    setStatus(plan, CreativePlayerSpawnStatus::MissingRoomBake,
              "creative_player_spawn_room_bake_missing");
    return plan;
  }
  if (request.spawnObject == nullptr ||
      request.spawnObject->kind != CreativeObjectKind::SpawnPoint ||
      request.spawnObject->id == kInvalidObjectId) {
    setStatus(plan, CreativePlayerSpawnStatus::InvalidObject,
              "creative_player_spawn_object_invalid");
    return plan;
  }

  const CreativeObject& object = *request.spawnObject;
  plan.objectId = object.id;
  plan.settings = object.playerSpawn;
  plan.yawRadians = static_cast<float>(object.transform.rotationEulerRadians.y);
  if (!isValidCreativePlayerSpawnSettings(plan.settings) ||
      !std::isfinite(plan.yawRadians) ||
      !std::isfinite(request.reachabilityCellSizeMeters) ||
      request.reachabilityCellSizeMeters <= 0.0F) {
    setStatus(plan, CreativePlayerSpawnStatus::InvalidSettings,
              "creative_player_spawn_settings_invalid");
    return plan;
  }
  if (!isSupportedCreativePlayerProfileId(plan.settings.playerProfileId)) {
    setStatus(plan, CreativePlayerSpawnStatus::UnsupportedProfile,
              "creative_player_spawn_profile_unsupported");
    return plan;
  }

  const RoomAnchorAsset* sourceAnchor =
      anchorForObject(*request.roomBake, object.id);
  if (sourceAnchor == nullptr || !isFinite(sourceAnchor->positionMeters)) {
    setStatus(plan, CreativePlayerSpawnStatus::InvalidObject,
              "creative_player_spawn_anchor_missing");
    return plan;
  }
  plan.anchor = *sourceAnchor;
  plan.authoredPositionMeters = sourceAnchor->positionMeters;
  plan.clearanceRadiusMeters = static_cast<float>(std::max(
      plan.settings.validationRadiusMeters, kDefaultPlayerBodyRadiusMeters));

  const SpatialSurfaceSet surfaces =
      buildSpatialSurfaceSet(request.roomBake->room);
  float floorHeight = 0.0F;
  if (!resolveSupportedFloor(surfaces, plan.authoredPositionMeters,
                             plan.clearanceRadiusMeters, floorHeight)) {
    setStatus(plan, CreativePlayerSpawnStatus::UnsupportedFloor,
              "creative_player_spawn_floor_unsupported");
    return plan;
  }
  plan.groundedPositionMeters = plan.authoredPositionMeters;
  plan.groundedPositionMeters.y = floorHeight;
  if (!withinDocumentBounds(request.document->worldBounds(),
                            plan.groundedPositionMeters,
                            plan.clearanceRadiusMeters,
                            plan.bodyHeightMeters)) {
    setStatus(plan, CreativePlayerSpawnStatus::OutsideWorldBounds,
              "creative_player_spawn_outside_world_bounds");
    return plan;
  }
  const CollisionQueryResult bodyOverlap = standingBodyOverlap(
      surfaces, plan.groundedPositionMeters, plan.clearanceRadiusMeters,
      plan.bodyHeightMeters);
  if (bodyOverlap.status != CollisionQueryStatus::NoHit) {
    plan.obstructionSurfaceId = bodyOverlap.surfaceId;
    setStatus(plan, CreativePlayerSpawnStatus::Obstructed,
              "creative_player_spawn_obstructed");
    return plan;
  }

  plan.anchor.positionMeters = plan.groundedPositionMeters;
  plan.cameraPositionMeters =
      plan.groundedPositionMeters + Vec3{0.0F,
                                        kCreativeDefaultPlayerEyeHeightMeters,
                                        0.0F};
  plan.facingDirection =
      {std::sin(plan.yawRadians), 0.0F, -std::cos(plan.yawRadians)};
  plan.reachability = validateFromSpawn(request.roomBake->room, plan.anchor,
                                        request.reachabilityCellSizeMeters);
  if (plan.reachability.status !=
      CreativeRoomBakeReachabilityStatus::Reachable) {
    setStatus(plan, CreativePlayerSpawnStatus::Unreachable,
              "creative_player_spawn_unreachable");
    return plan;
  }

  setStatus(plan, CreativePlayerSpawnStatus::Ready,
            "creative_player_spawn_ready", true);
  return plan;
}

CreativePlayerSpawnResolveResult resolveCreativePlayerSpawn(
    const CreativePlayerSpawnResolveRequest& request) {
  CreativePlayerSpawnResolveResult result;
  result.requested = true;
  if (request.document == nullptr) {
    setStatus(result, CreativePlayerSpawnStatus::MissingDocument,
              "creative_player_spawn_document_missing");
    return result;
  }
  if (!request.document->isValid()) {
    setStatus(result, CreativePlayerSpawnStatus::InvalidDocument,
              "creative_player_spawn_document_invalid");
    return result;
  }
  if (request.roomBake == nullptr || !request.roomBake->receipt.accepted) {
    setStatus(result, CreativePlayerSpawnStatus::MissingRoomBake,
              "creative_player_spawn_room_bake_missing");
    return result;
  }
  if (!isValidCreativeSpawnGroup(request.spawnGroup)) {
    setStatus(result, CreativePlayerSpawnStatus::InvalidSettings,
              "creative_player_spawn_group_invalid");
    return result;
  }

  std::vector<const CreativeObject*> candidates;
  for (const CreativeObject& object : request.document->objects()) {
    if (object.kind != CreativeObjectKind::SpawnPoint ||
        (!request.includeHidden &&
         !creativeObjectEffectivelyVisible(*request.document, object.id)) ||
        object.playerSpawn.spawnGroup != request.spawnGroup) {
      continue;
    }
    candidates.push_back(&object);
  }
  std::sort(candidates.begin(), candidates.end(),
            [](const CreativeObject* left, const CreativeObject* right) {
              if (left->playerSpawn.fallbackPriority !=
                  right->playerSpawn.fallbackPriority) {
                return left->playerSpawn.fallbackPriority <
                       right->playerSpawn.fallbackPriority;
              }
              return left->id < right->id;
            });
  result.groupCandidateCount = candidates.size();
  if (candidates.empty()) {
    setStatus(result, CreativePlayerSpawnStatus::GroupUnavailable,
              "creative_player_spawn_group_unavailable");
    return result;
  }

  CreativePlayerSpawnPlan firstRejected;
  for (const CreativeObject* candidate : candidates) {
    CreativePlayerSpawnPlan plan = planCreativePlayerSpawn(
        {request.document, request.roomBake, candidate,
         request.reachabilityCellSizeMeters});
    if (plan.accepted) {
      result.selected = std::move(plan);
      setStatus(result, CreativePlayerSpawnStatus::Ready,
                "creative_player_spawn_resolved", true);
      return result;
    }
    if (!firstRejected.requested) {
      firstRejected = plan;
    }
    ++result.rejectedCandidateCount;
  }
  result.selected = std::move(firstRejected);
  setStatus(result, result.selected.status, result.selected.reasonCode);
  return result;
}

}  // namespace iggy3d::creative
