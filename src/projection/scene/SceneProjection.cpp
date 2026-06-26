#include "projection/scene/SceneProjection.hpp"

#include <set>
#include <string_view>
#include <utility>

#include "content/assets/RoomAsset.hpp"
#include "render/mesh/BeanMesh.hpp"

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
    return std::string(beanModelId(BeanModelKind::Player));
  }
  if (kind == SceneItemKind::Npc) {
    return std::string(beanModelId(BeanModelKind::Npc));
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
  return role == "floor" || role == "wall";
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
    item.role = mesh.role;
    item.materialId = mesh.materialId;
    item.position = mesh.positionMeters;
    item.size = mesh.sizeMeters;
    item.hasWallSegment = mesh.hasWallSegment;
    item.wallStartMeters = mesh.wallStartMeters;
    item.wallEndMeters = mesh.wallEndMeters;
    item.wallBottomY = mesh.wallBottomY;
    item.wallHeightMeters = mesh.wallHeightMeters;
    item.wallThicknessMeters = mesh.wallThicknessMeters;
    projected.floorVisible = projected.floorVisible || mesh.role == "floor";
    projected.wallVisible = projected.wallVisible || mesh.role == "wall";
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
  return result;
}

}  // namespace iggy3d
