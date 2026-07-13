#include "projection/scene/SceneProjection.hpp"

#include <cmath>
#include <set>
#include <string>
#include <string_view>
#include <utility>

#include "content/assets/RoomAsset.hpp"
#include "projection/scene/SceneModel.hpp"

namespace iggy3d {
namespace {

Aabb3 worldBoundsFor(const EntityState& entity) {
  return makeAabb3(entity.localBounds.min + entity.transform.position,
                   entity.localBounds.max + entity.transform.position);
}

bool hasInteraction(const EntityState& entity) {
  return entity.interaction.kind != InteractionKind::None ||
         isTargetActionSupported(entity.targeting, TargetAction::Interact);
}

PlayerSlotId owningSlotFor(const SessionState& state, EntityId entityId) {
  for (const PlayerSlot& slot : state.players.slots()) {
    if (slot.actor == entityId) {
      return slot.id;
    }
  }
  return kInvalidPlayerSlotId;
}

SceneItemKind kindFor(const EntityState& entity) {
  if (entity.kind == EntityKind::Player) {
    return SceneItemKind::Player;
  }
  if (entity.kind == EntityKind::Npc) {
    return SceneItemKind::Npc;
  }
  if (entity.kind == EntityKind::Pickup) {
    return SceneItemKind::Pickup;
  }
  if (entity.kind == EntityKind::Marker) {
    return SceneItemKind::TacticalMarker;
  }
  if (hasInteraction(entity)) {
    return SceneItemKind::Interactable;
  }
  return SceneItemKind::DebugOnly;
}

bool shouldIncludeItem(SceneItemKind kind, const SceneProjectionConfig& config) {
  if (kind == SceneItemKind::TacticalMarker) {
    return config.includeTacticalMarkers;
  }
  if (kind == SceneItemKind::ObjectiveMarker) {
    return config.includeObjectiveMarkers;
  }
  if (kind == SceneItemKind::DebugOnly) {
    return config.includeDebugOnly;
  }
  return true;
}

std::string modelRefFor(SceneItemKind kind) {
  if (kind == SceneItemKind::Player) {
    return std::string(sceneModelId(SceneModelKind::PlayerBean));
  }
  if (kind == SceneItemKind::Npc) {
    return std::string(sceneModelId(SceneModelKind::NpcBean));
  }
  return {};
}

void countItem(const SceneItem& item, SceneProjectionResult& result) {
  switch (item.kind) {
    case SceneItemKind::Player:
      ++result.playerCount;
      break;
    case SceneItemKind::Npc:
      break;
    case SceneItemKind::Pickup:
      ++result.pickupCount;
      break;
    case SceneItemKind::ObjectiveMarker:
    case SceneItemKind::TacticalMarker:
      ++result.markerCount;
      break;
    case SceneItemKind::DebugOnly:
      ++result.debugOnlyCount;
      break;
    case SceneItemKind::Interactable:
      break;
  }
  if (item.interactable) {
    ++result.interactableCount;
  }
}

SceneItem projectEntity(const SessionState& state, const EntityState& entity) {
  SceneItem item;
  item.entityId = entity.id;
  item.stableName = entity.stableName;
  item.kind = kindFor(entity);
  item.entityKind = entity.kind;
  item.transform = entity.transform;
  item.worldBounds = worldBoundsFor(entity);
  item.active = entity.active;
  item.visible = entity.active;
  item.targetable = entity.targeting.targetable;
  item.interactable = hasInteraction(entity);
  item.tactical = item.kind == SceneItemKind::TacticalMarker;
  item.assetRef = entity.stableName;
  item.modelRef = modelRefFor(item.kind);
  item.itemId = entity.interaction.itemId;
  item.objectiveId = entity.interaction.objectiveId;
  item.interactionKind = entity.interaction.kind;
  item.owningPlayerSlot = owningSlotFor(state, entity.id);
  return item;
}

bool isProjectedRoomMeshRole(std::string_view role) {
  return role == "floor" || role == "wall" || role == "prop" ||
         role == "ledge" || role == "terrain";
}

void attachRoomProjection(const RoomAsset* room, SceneProjectionResult& result) {
  if (room == nullptr) {
    return;
  }

  SceneRoomProjection projected;
  projected.assetId = room->id;
  projected.version = room->version;
  projected.sourceToml = room->sourceFile.empty() ? room->source : room->sourceFile;
  projected.sourceSubset = room->sourceSubset;
  projected.staticMeshCount = room->staticMeshes.size();
  projected.anchorCount = room->anchors.size();

  std::set<std::string> materialIds;
  projected.meshes.reserve(room->staticMeshes.size());
  for (const RoomStaticMeshAsset& mesh : room->staticMeshes) {
    if (!isProjectedRoomMeshRole(mesh.role)) {
      continue;
    }

    SceneRoomMeshItem item;
    item.id = mesh.id;
    item.meshId = mesh.meshId;
    item.role = mesh.role;
    item.materialId = mesh.materialId;
    item.position = mesh.positionMeters;
    item.size = mesh.sizeMeters;
    item.rotationEulerRadians = mesh.rotationEulerRadians;
    item.hasWallSegment = mesh.hasWallSegment;
    item.wallStartMeters = mesh.wallStartMeters;
    item.wallEndMeters = mesh.wallEndMeters;
    item.wallBottomY = mesh.wallBottomY;
    item.wallHeightMeters = mesh.wallHeightMeters;
    item.wallThicknessMeters = mesh.wallThicknessMeters;
    projected.floorVisible = projected.floorVisible || mesh.role == "floor" ||
                             mesh.role == "terrain";
    projected.wallVisible = projected.wallVisible || mesh.role == "wall";
    projected.propVisible = projected.propVisible || mesh.role == "prop" ||
                            mesh.role == "ledge";
    if (!mesh.materialId.empty()) {
      materialIds.insert(mesh.materialId);
    }
    projected.meshes.push_back(std::move(item));
  }

  if (projected.meshes.empty()) {
    return;
  }

  for (const RoomAnchorAsset& anchor : room->anchors) {
    projected.keyAnchorVisible =
        projected.keyAnchorVisible || anchor.runtimeStableName == "gold_key";
    projected.dummyAnchorVisible =
        projected.dummyAnchorVisible || anchor.runtimeStableName == "training_dummy";
  }

  projected.materialCount = materialIds.size();
  projected.loaded = true;
  result.room = std::move(projected);
}

std::string npcGazeRole(const AiActorState& actor) {
  if (actor.lastPerceived) {
    return "npc_gaze_perceived";
  }
  if (actor.lastTargetInRadius && actor.lastTargetInVisionCone &&
      actor.lastInVerticalCone && actor.lastLos == AiPerceptionLos::Blocked) {
    return "npc_gaze_blocked";
  }
  return "npc_gaze_scan";
}

void appendNpcGazeBlade(SceneProjectionResult& result,
                        const std::string& stableName,
                        std::string_view suffix,
                        std::string role,
                        Vec3 apex,
                        Vec3 end,
                        float halfWidthMeters) {
  SceneRoomMeshItem item;
  item.id = "npc_gaze_" + stableName + "_" + std::string(suffix);
  item.role = std::move(role);
  item.position = apex;
  item.size = {halfWidthMeters, 0.0F, 0.0F};
  item.hasWallSegment = true;
  item.wallStartMeters = apex;
  item.wallEndMeters = end;
  item.wallThicknessMeters = halfWidthMeters;
  result.room.meshes.push_back(std::move(item));
}

// Append per-NPC "gaze blade" debug meshes: center plus upper/lower vertical
// cone extents from the mirrored guard eye and profile half-angle. Piggybacks
// the room-mesh render path, which rasterizes world geometry.
void attachNpcVisionDebug(const SessionState& state,
                          const SceneProjectionConfig& config,
                          SceneProjectionResult& result) {
  if (!config.includeNpcVisionDebug || !result.room.loaded) {
    return;
  }
  constexpr float kDegreesToRadians = 0.0174532925199F;
  constexpr float kGazeBladeHalfWidthMeters = 0.12F;
  for (const AiActorState& actor : state.ai.actors) {
    const EntityState* entity = state.world.findById(actor.actor);
    if (entity == nullptr || !entity->active ||
        entity->kind != EntityKind::Npc) {
      continue;
    }
    const float facingLengthSq = actor.facingDirection.x * actor.facingDirection.x +
                                 actor.facingDirection.z * actor.facingDirection.z;
    if (!(facingLengthSq > 1.0e-8F) || !(actor.lastSightRangeMeters > 0.0F) ||
        !std::isfinite(actor.lastGuardEyeHeightMeters) ||
        !std::isfinite(actor.lastVerticalHalfAngleDegrees)) {
      continue;
    }
    const float invLength = 1.0F / std::sqrt(facingLengthSq);
    const Vec3 apex{entity->transform.position.x,
                    entity->transform.position.y + actor.lastGuardEyeHeightMeters,
                    entity->transform.position.z};
    const Vec3 centerEnd{
        apex.x + actor.facingDirection.x * invLength * actor.lastSightRangeMeters,
        apex.y,
        apex.z + actor.facingDirection.z * invLength * actor.lastSightRangeMeters};
    const float verticalHalfAngleRadians =
        actor.lastVerticalHalfAngleDegrees * kDegreesToRadians;
    const float verticalOffset =
        std::tan(verticalHalfAngleRadians) * actor.lastSightRangeMeters;
    if (!std::isfinite(verticalOffset)) {
      continue;
    }
    const Vec3 upperEnd{centerEnd.x, centerEnd.y + verticalOffset, centerEnd.z};
    const Vec3 lowerEnd{centerEnd.x, centerEnd.y - verticalOffset, centerEnd.z};
    const std::string role = npcGazeRole(actor);

    appendNpcGazeBlade(result, entity->stableName, "center", role, apex, centerEnd,
                       kGazeBladeHalfWidthMeters);
    appendNpcGazeBlade(result, entity->stableName, "upper", role, apex, upperEnd,
                       kGazeBladeHalfWidthMeters);
    appendNpcGazeBlade(result, entity->stableName, "lower", role, apex, lowerEnd,
                       kGazeBladeHalfWidthMeters);
  }
}

}  // namespace

SceneProjectionResult buildSceneProjection(const SessionState& state,
                                           const SceneProjectionConfig& config) {
  return buildSceneProjection(state, nullptr, config);
}

SceneProjectionResult buildSceneProjection(const SessionState& state,
                                           const RoomAsset* room,
                                           const SceneProjectionConfig& config) {
  SceneProjectionResult result;
  result.sourceStateHash = state.currentStateHash;
  result.sourceTick = state.clock.tickIndex;
  result.cameraMode = state.camera.activeMode;
  result.previousRealtimeCamera = state.camera.previousRealtimeMode;
  result.cameraTargetEntity = state.camera.target.entity;
  result.cameraTargetPoint = state.camera.target.point;
  result.cameraTargetHasPoint = state.camera.target.hasPoint;

  for (const EntityState& entity : state.world.entities()) {
    if (!config.includeInactive && !entity.active) {
      continue;
    }

    SceneItem item = projectEntity(state, entity);
    if (!shouldIncludeItem(item.kind, config)) {
      continue;
    }

    countItem(item, result);
    result.items.push_back(std::move(item));
  }

  attachRoomProjection(room, result);
  attachNpcVisionDebug(state, config, result);
  return result;
}

}  // namespace iggy3d
