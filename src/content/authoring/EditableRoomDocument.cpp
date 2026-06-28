#include "content/authoring/EditableRoomDocument.hpp"

#include <algorithm>
#include <cmath>
#include <set>
#include <string_view>
#include <utility>

#include "core/math/Aabb3.hpp"

namespace iggy3d {
namespace {

inline constexpr float kEpsilon = 0.0001F;

bool hasText(std::string_view value) {
  return !value.empty();
}

bool containsString(const std::vector<std::string>& values, std::string_view expected) {
  for (const std::string& value : values) {
    if (value == expected) {
      return true;
    }
  }
  return false;
}

void appendUnique(std::vector<std::string>& values, std::string value) {
  if (!containsString(values, value)) {
    values.push_back(std::move(value));
  }
}

bool hasDuplicateStrings(const std::vector<std::string>& values) {
  std::set<std::string> seen;
  for (const std::string& value : values) {
    if (value.empty() || !seen.insert(value).second) {
      return true;
    }
  }
  return false;
}

bool validSemanticTag(std::string_view tag) {
  return tag == "walkable" || tag == "blocker" ||
         tag == "projectile_blocker" || tag == "opening" ||
         tag == "clamber" || tag == "clamber_candidate" ||
         tag == "vault" || tag == "wire_walk" || tag == "no_player" ||
         tag == "debug_only";
}

bool validateSemantics(const EditableRoomSemantics& semantics) {
  if (!hasText(semantics.materialId) || hasDuplicateStrings(semantics.traversalTags) ||
      hasDuplicateStrings(semantics.gameplayTags)) {
    return false;
  }
  for (const std::string& tag : semantics.traversalTags) {
    if (!validSemanticTag(tag)) {
      return false;
    }
  }
  return true;
}

bool validId(std::string_view id) {
  if (id.empty()) {
    return false;
  }
  for (const char c : id) {
    const bool valid = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                       (c >= '0' && c <= '9') || c == '_' || c == '-';
    if (!valid) {
      return false;
    }
  }
  return true;
}

EditableRoomFloor* findEditableFloorMutable(EditableRoomDocument& document,
                                            const std::string& id) {
  for (EditableRoomFloor& floor : document.floors) {
    if (floor.id == id) {
      return &floor;
    }
  }
  return nullptr;
}

EditableRoomWall* findEditableWallMutable(EditableRoomDocument& document,
                                          const std::string& id) {
  for (EditableRoomWall& wall : document.walls) {
    if (wall.id == id) {
      return &wall;
    }
  }
  return nullptr;
}

EditableRoomObject* findEditableObjectMutable(EditableRoomDocument& document,
                                              const std::string& id) {
  for (EditableRoomObject& object : document.objects) {
    // branch-gate: BG-1125
    if (object.id == id) {
      return &object;
    }
  }
  return nullptr;
}

bool primitiveIdExists(const EditableRoomDocument& document, const std::string& id) {
  return findEditableFloor(document, id) != nullptr ||
         findEditableWall(document, id) != nullptr ||
         findEditableObject(document, id) != nullptr;
}

RoomEditResult makeEditResult(RoomEditStatus status, std::string primitiveId = {}) {
  RoomEditResult result;
  result.status = status;
  result.reasonCode = roomEditStatusName(status);
  result.primitiveId = std::move(primitiveId);
  return result;
}

bool validateFloor(const EditableRoomFloor& floor) {
  return validId(floor.id) && isFinite(floor.centerMeters) && isFinite(floor.sizeMeters) &&
         floor.sizeMeters.x > kEpsilon && floor.sizeMeters.y > kEpsilon &&
         floor.sizeMeters.z > kEpsilon && validateSemantics(floor.semantics);
}

bool isAxisAlignedWall(const EditableRoomWall& wall) {
  const float dx = std::fabs(wall.endMeters.x - wall.startMeters.x);
  const float dz = std::fabs(wall.endMeters.z - wall.startMeters.z);
  return (dx <= kEpsilon && dz > kEpsilon) || (dz <= kEpsilon && dx > kEpsilon);
}

bool validateWall(const EditableRoomWall& wall) {
  return validId(wall.id) && isFinite(wall.startMeters) && isFinite(wall.endMeters) &&
         std::isfinite(wall.bottomY) && std::isfinite(wall.heightMeters) &&
         std::isfinite(wall.thicknessMeters) && wall.heightMeters > kEpsilon &&
         wall.thicknessMeters > kEpsilon && isAxisAlignedWall(wall) &&
         validateSemantics(wall.semantics);
}

bool validateObject(const EditableRoomObject& object) {
  return validId(object.id) && hasText(object.assetId) &&
         isFinite(object.positionMeters) && isFinite(object.sizeMeters) &&
         std::isfinite(object.yawDegrees) && object.sizeMeters.x > kEpsilon &&
         object.sizeMeters.y > kEpsilon && object.sizeMeters.z > kEpsilon;
}

bool nearlyZero(float value) {
  return std::fabs(value) <= kEpsilon;
}

bool validHorizontalDelta(Vec3 delta) {
  return isFinite(delta) && nearlyZero(delta.y);
}

bool pointOnExistingWallAxis(const EditableRoomWall& wall, Vec3 point, bool replacingStart) {
  if (!isFinite(point)) {
    return false;
  }
  const Vec3 fixed = replacingStart ? wall.endMeters : wall.startMeters;
  const float dx = std::fabs(wall.endMeters.x - wall.startMeters.x);
  const float dz = std::fabs(wall.endMeters.z - wall.startMeters.z);
  if (dx >= dz) {
    return nearlyZero(point.z - fixed.z) && nearlyZero(point.y - fixed.y);
  }
  return nearlyZero(point.x - fixed.x) && nearlyZero(point.y - fixed.y);
}

Aabb3 floorBounds(const EditableRoomFloor& floor) {
  return aabbFromCenterExtents(floor.centerMeters, floor.sizeMeters * 0.5F);
}

Aabb3 wallBounds(const EditableRoomWall& wall) {
  const float minX = std::min(wall.startMeters.x, wall.endMeters.x);
  const float maxX = std::max(wall.startMeters.x, wall.endMeters.x);
  const float minZ = std::min(wall.startMeters.z, wall.endMeters.z);
  const float maxZ = std::max(wall.startMeters.z, wall.endMeters.z);
  const bool runsAlongX = std::fabs(wall.endMeters.x - wall.startMeters.x) >
                          std::fabs(wall.endMeters.z - wall.startMeters.z);
  const Vec3 minPoint{runsAlongX ? minX : minX - wall.thicknessMeters * 0.5F,
                      wall.bottomY,
                      runsAlongX ? minZ - wall.thicknessMeters * 0.5F : minZ};
  const Vec3 maxPoint{runsAlongX ? maxX : maxX + wall.thicknessMeters * 0.5F,
                      wall.bottomY + wall.heightMeters,
                      runsAlongX ? maxZ + wall.thicknessMeters * 0.5F : maxZ};
  return makeAabb3(minPoint, maxPoint);
}

Aabb3 objectBounds(const EditableRoomObject& object) {
  return aabbFromCenterExtents(object.positionMeters,
                               object.sizeMeters * 0.5F);
}

std::vector<Vec3> topFacePoints(const Aabb3& bounds) {
  return {
      {bounds.min.x, bounds.max.y, bounds.min.z},
      {bounds.max.x, bounds.max.y, bounds.min.z},
      {bounds.max.x, bounds.max.y, bounds.max.z},
      {bounds.min.x, bounds.max.y, bounds.max.z},
  };
}

std::vector<Vec3> boxExtentPoints(const Aabb3& bounds) {
  return {
      {bounds.min.x, bounds.min.y, bounds.min.z},
      {bounds.max.x, bounds.min.y, bounds.min.z},
      {bounds.max.x, bounds.max.y, bounds.max.z},
      {bounds.min.x, bounds.max.y, bounds.max.z},
  };
}

Vec3 wallForwardNormal(const EditableRoomWall& wall) {
  const float dx = std::fabs(wall.endMeters.x - wall.startMeters.x);
  const float dz = std::fabs(wall.endMeters.z - wall.startMeters.z);
  return dx >= dz ? Vec3{0.0F, 0.0F, 1.0F} : Vec3{1.0F, 0.0F, 0.0F};
}

std::vector<std::string> traversalTagsWithStructuralTag(
    const EditableRoomSemantics& semantics,
    std::string structuralTag) {
  std::vector<std::string> tags;
  appendUnique(tags, std::move(structuralTag));
  for (const std::string& tag : semantics.traversalTags) {
    if (tag != "walkable" && tag != "blocker" && tag != "projectile_blocker") {
      appendUnique(tags, tag);
    }
  }
  return tags;
}

RoomStaticMeshAsset makeFloorMesh(const EditableRoomFloor& floor) {
  RoomStaticMeshAsset mesh;
  mesh.id = floor.id;
  mesh.meshId = "floor_rect";
  mesh.materialId = floor.semantics.materialId;
  mesh.role = "floor";
  mesh.positionMeters = floor.centerMeters;
  mesh.sizeMeters = floor.sizeMeters;
  return mesh;
}

RoomStaticMeshAsset makeWallMesh(const EditableRoomWall& wall) {
  const Aabb3 bounds = wallBounds(wall);
  RoomStaticMeshAsset mesh;
  mesh.id = wall.id;
  mesh.meshId = "wall_segment";
  mesh.materialId = wall.semantics.materialId;
  mesh.role = "wall";
  mesh.positionMeters = center(bounds);
  mesh.sizeMeters = bounds.max - bounds.min;
  mesh.hasWallSegment = true;
  mesh.wallStartMeters = wall.startMeters;
  mesh.wallEndMeters = wall.endMeters;
  mesh.wallBottomY = wall.bottomY;
  mesh.wallHeightMeters = wall.heightMeters;
  mesh.wallThicknessMeters = wall.thicknessMeters;
  return mesh;
}

RoomStaticMeshAsset makeObjectMesh(const EditableRoomObject& object) {
  RoomStaticMeshAsset mesh;
  mesh.id = object.id;
  mesh.meshId = object.assetId;
  mesh.materialId = object.assetId;
  mesh.role = "prop";
  mesh.positionMeters = object.positionMeters;
  mesh.sizeMeters = object.sizeMeters;
  return mesh;
}

RoomSpatialSurface makeWalkableSurface(std::string id,
                                       std::string sourceId,
                                       const Aabb3& bounds,
                                       const EditableRoomSemantics& semantics) {
  RoomSpatialSurface surface;
  surface.id = std::move(id);
  surface.sourceStaticMeshId = std::move(sourceId);
  surface.shape = RoomSpatialSurfaceShape::Plane;
  surface.role = RoomSpatialSurfaceRole::Walkable;
  surface.pointsMeters = topFacePoints(bounds);
  surface.normal = {0.0F, 1.0F, 0.0F};
  surface.traversalTags = traversalTagsWithStructuralTag(semantics, "walkable");
  surface.collisionMask = {"actor"};
  surface.blocksActor = false;
  surface.blocksProjectile = false;
  return surface;
}

RoomSpatialSurface makeActorBlockerSurface(std::string id,
                                           std::string sourceId,
                                           const Aabb3& bounds,
                                           Vec3 normal,
                                           const EditableRoomSemantics& semantics) {
  RoomSpatialSurface surface;
  surface.id = std::move(id);
  surface.sourceStaticMeshId = std::move(sourceId);
  surface.shape = RoomSpatialSurfaceShape::Box;
  surface.role = RoomSpatialSurfaceRole::Blocker;
  surface.pointsMeters = boxExtentPoints(bounds);
  surface.normal = normal;
  surface.traversalTags = traversalTagsWithStructuralTag(semantics, "blocker");
  surface.collisionMask = {"actor"};
  surface.blocksActor = true;
  surface.blocksProjectile = false;
  return surface;
}

RoomSpatialSurface makeProjectileBlockerSurface(std::string id,
                                                std::string sourceId,
                                                const Aabb3& bounds,
                                                Vec3 normal) {
  RoomSpatialSurface surface;
  surface.id = std::move(id);
  surface.sourceStaticMeshId = std::move(sourceId);
  surface.shape = RoomSpatialSurfaceShape::Box;
  surface.role = RoomSpatialSurfaceRole::ProjectileBlocker;
  surface.pointsMeters = boxExtentPoints(bounds);
  surface.normal = normal;
  surface.traversalTags = {"projectile_blocker"};
  surface.collisionMask = {"projectile"};
  surface.blocksActor = false;
  surface.blocksProjectile = true;
  return surface;
}

void appendFloorRuntime(RoomAsset& room, const EditableRoomFloor& floor) {
  const Aabb3 bounds = floorBounds(floor);
  room.staticMeshes.push_back(makeFloorMesh(floor));
  if (floor.semantics.walkable) {
    room.spatialSurfaces.push_back(
        makeWalkableSurface(floor.id + "_walkable", floor.id, bounds, floor.semantics));
  }
}

void appendWallRuntime(RoomAsset& room, const EditableRoomWall& wall) {
  const Aabb3 bounds = wallBounds(wall);
  const Vec3 normal = wallForwardNormal(wall);
  room.staticMeshes.push_back(makeWallMesh(wall));
  if (wall.semantics.blocksActor) {
    room.spatialSurfaces.push_back(makeActorBlockerSurface(
        wall.id + "_actor_blocker", wall.id, bounds, normal, wall.semantics));
  }
  if (wall.semantics.blocksProjectile) {
    room.spatialSurfaces.push_back(makeProjectileBlockerSurface(
        wall.id + "_projectile_blocker", wall.id, bounds, normal));
  }
  if (containsString(wall.semantics.traversalTags, "clamber")) {
    room.spatialSurfaces.push_back(
        makeWalkableSurface(wall.id + "_top_walkable", wall.id, bounds, wall.semantics));
  }
}

void appendObjectRuntime(RoomAsset& room, const EditableRoomObject& object) {
  const Aabb3 bounds = objectBounds(object);
  const Vec3 normal{0.0F, 0.0F, 1.0F};
  room.staticMeshes.push_back(makeObjectMesh(object));
  // branch-gate: BG-1125
  if (object.blocksActor) {
    EditableRoomSemantics semantics;
    semantics.materialId = object.assetId;
    semantics.blocksActor = true;
    room.spatialSurfaces.push_back(makeActorBlockerSurface(
        object.id + "_actor_blocker", object.id, bounds, normal, semantics));
  }
  // branch-gate: BG-1125
  if (object.blocksProjectile) {
    room.spatialSurfaces.push_back(makeProjectileBlockerSurface(
        object.id + "_projectile_blocker", object.id, bounds, normal));
  }
}

}  // namespace

EditableRoomSemantics defaultFloorSemantics(std::string materialId) {
  EditableRoomSemantics semantics;
  semantics.materialId = std::move(materialId);
  semantics.walkable = true;
  return semantics;
}

EditableRoomSemantics defaultWallSemantics(std::string materialId) {
  EditableRoomSemantics semantics;
  semantics.materialId = std::move(materialId);
  semantics.blocksActor = true;
  semantics.blocksProjectile = true;
  return semantics;
}

RoomEditCommand addFloorCommand(EditableRoomFloor floor) {
  RoomEditCommand command;
  command.kind = RoomEditCommandKind::AddFloor;
  command.floor = std::move(floor);
  return command;
}

RoomEditCommand deleteFloorCommand(std::string id) {
  RoomEditCommand command;
  command.kind = RoomEditCommandKind::DeleteFloor;
  command.targetId = std::move(id);
  return command;
}

RoomEditCommand setFloorSemanticsCommand(std::string id, EditableRoomSemantics semantics) {
  RoomEditCommand command;
  command.kind = RoomEditCommandKind::SetFloorSemantics;
  command.targetId = std::move(id);
  command.semantics = std::move(semantics);
  return command;
}

RoomEditCommand moveFloorCommand(std::string id, Vec3 deltaMeters) {
  RoomEditCommand command;
  command.kind = RoomEditCommandKind::MoveFloor;
  command.targetId = std::move(id);
  command.deltaMeters = deltaMeters;
  return command;
}

RoomEditCommand setFloorPositionCommand(std::string id, Vec3 positionMeters) {
  RoomEditCommand command;
  command.kind = RoomEditCommandKind::MoveFloor;
  command.targetId = std::move(id);
  command.positionMeters = positionMeters;
  command.useStart = true;
  return command;
}

RoomEditCommand resizeFloorCommand(std::string id, Vec3 sizeMeters) {
  RoomEditCommand command;
  command.kind = RoomEditCommandKind::ResizeFloor;
  command.targetId = std::move(id);
  command.sizeMeters = sizeMeters;
  return command;
}

RoomEditCommand addWallCommand(EditableRoomWall wall) {
  RoomEditCommand command;
  command.kind = RoomEditCommandKind::AddWall;
  command.wall = std::move(wall);
  return command;
}

RoomEditCommand deleteWallCommand(std::string id) {
  RoomEditCommand command;
  command.kind = RoomEditCommandKind::DeleteWall;
  command.targetId = std::move(id);
  return command;
}

RoomEditCommand setWallSemanticsCommand(std::string id, EditableRoomSemantics semantics) {
  RoomEditCommand command;
  command.kind = RoomEditCommandKind::SetWallSemantics;
  command.targetId = std::move(id);
  command.semantics = std::move(semantics);
  return command;
}

RoomEditCommand moveWallCommand(std::string id, Vec3 deltaMeters) {
  RoomEditCommand command;
  command.kind = RoomEditCommandKind::MoveWall;
  command.targetId = std::move(id);
  command.deltaMeters = deltaMeters;
  return command;
}

RoomEditCommand stretchWallStartCommand(std::string id, Vec3 startMeters) {
  RoomEditCommand command;
  command.kind = RoomEditCommandKind::StretchWall;
  command.targetId = std::move(id);
  command.startMeters = startMeters;
  command.useStart = true;
  return command;
}

RoomEditCommand stretchWallEndCommand(std::string id, Vec3 endMeters) {
  RoomEditCommand command;
  command.kind = RoomEditCommandKind::StretchWall;
  command.targetId = std::move(id);
  command.endMeters = endMeters;
  command.useStart = false;
  return command;
}

RoomEditCommand rotateWall90Command(std::string id, bool left) {
  RoomEditCommand command;
  command.kind = RoomEditCommandKind::RotateWall90;
  command.targetId = std::move(id);
  command.rotateLeft = left;
  return command;
}

RoomEditCommand setWallHeightCommand(std::string id, float heightMeters) {
  RoomEditCommand command;
  command.kind = RoomEditCommandKind::SetWallHeight;
  command.targetId = std::move(id);
  command.scalarMeters = heightMeters;
  return command;
}

RoomEditCommand setWallThicknessCommand(std::string id, float thicknessMeters) {
  RoomEditCommand command;
  command.kind = RoomEditCommandKind::SetWallThickness;
  command.targetId = std::move(id);
  command.scalarMeters = thicknessMeters;
  return command;
}

RoomEditCommand addObjectCommand(EditableRoomObject object) {
  RoomEditCommand command;
  command.kind = RoomEditCommandKind::AddObject;
  command.object = std::move(object);
  return command;
}

RoomEditCommand deleteObjectCommand(std::string id) {
  RoomEditCommand command;
  command.kind = RoomEditCommandKind::DeleteObject;
  command.targetId = std::move(id);
  return command;
}

RoomEditCommand moveObjectCommand(std::string id, Vec3 deltaMeters) {
  RoomEditCommand command;
  command.kind = RoomEditCommandKind::MoveObject;
  command.targetId = std::move(id);
  command.deltaMeters = deltaMeters;
  return command;
}

RoomEditCommand setObjectPositionCommand(std::string id, Vec3 positionMeters) {
  RoomEditCommand command;
  command.kind = RoomEditCommandKind::MoveObject;
  command.targetId = std::move(id);
  command.positionMeters = positionMeters;
  command.useStart = true;
  return command;
}

const EditableRoomFloor* findEditableFloor(const EditableRoomDocument& document,
                                           const std::string& id) {
  for (const EditableRoomFloor& floor : document.floors) {
    if (floor.id == id) {
      return &floor;
    }
  }
  return nullptr;
}

const EditableRoomObject* findEditableObject(const EditableRoomDocument& document,
                                             const std::string& id) {
  for (const EditableRoomObject& object : document.objects) {
    // branch-gate: BG-1125
    if (object.id == id) {
      return &object;
    }
  }
  return nullptr;
}

const EditableRoomWall* findEditableWall(const EditableRoomDocument& document,
                                         const std::string& id) {
  for (const EditableRoomWall& wall : document.walls) {
    if (wall.id == id) {
      return &wall;
    }
  }
  return nullptr;
}

std::vector<std::string> runtimeIdsForEditableFloor(const EditableRoomFloor& floor) {
  std::vector<std::string> ids{floor.id};
  if (floor.semantics.walkable) {
    ids.push_back(floor.id + "_walkable");
  }
  return ids;
}

std::vector<std::string> runtimeIdsForEditableWall(const EditableRoomWall& wall) {
  std::vector<std::string> ids{wall.id};
  if (wall.semantics.blocksActor) {
    ids.push_back(wall.id + "_actor_blocker");
  }
  if (wall.semantics.blocksProjectile) {
    ids.push_back(wall.id + "_projectile_blocker");
  }
  if (containsString(wall.semantics.traversalTags, "clamber")) {
    ids.push_back(wall.id + "_top_walkable");
  }
  return ids;
}

std::vector<std::string> runtimeIdsForEditableObject(const EditableRoomObject& object) {
  std::vector<std::string> ids{object.id};
  // branch-gate: BG-1125
  if (object.blocksActor) {
    ids.push_back(object.id + "_actor_blocker");
  }
  // branch-gate: BG-1125
  if (object.blocksProjectile) {
    ids.push_back(object.id + "_projectile_blocker");
  }
  return ids;
}

std::uint64_t nextEditableIndexForPrefix(const EditableRoomDocument& document,
                                         std::string_view prefix,
                                         bool floors) {
  std::uint64_t maxSuffix = 0;
  const auto scanId = [&](std::string_view id) {
    // branch-gate: BG-1125
    if (!id.starts_with(prefix) || id.size() == prefix.size()) {
      return;
    }
    std::uint64_t suffix = 0;
    for (std::size_t index = prefix.size(); index < id.size(); ++index) {
      const char c = id[index];
      // branch-gate: BG-1125
      if (c < '0' || c > '9') {
        return;
      }
      suffix = suffix * 10U + static_cast<std::uint64_t>(c - '0');
    }
    maxSuffix = std::max(maxSuffix, suffix);
  };

  if (floors) {
    for (const EditableRoomFloor& floor : document.floors) {
      scanId(floor.id);
    }
  } else {
    for (const EditableRoomWall& wall : document.walls) {
      scanId(wall.id);
    }
  }
  return maxSuffix + 1U;
}

std::uint64_t nextEditableObjectIndexForPrefix(
    const EditableRoomDocument& document,
    std::string_view prefix) {
  std::uint64_t maxSuffix = 0;
  const auto scanId = [&](std::string_view id) {
    // branch-gate: BG-1125
    if (!id.starts_with(prefix) || id.size() == prefix.size()) {
      return;
    }
    std::uint64_t suffix = 0;
    for (std::size_t index = prefix.size(); index < id.size(); ++index) {
      const char c = id[index];
      // branch-gate: BG-1125
      if (c < '0' || c > '9') {
        return;
      }
      suffix = suffix * 10U + static_cast<std::uint64_t>(c - '0');
    }
    maxSuffix = std::max(maxSuffix, suffix);
  };

  for (const EditableRoomObject& object : document.objects) {
    scanId(object.id);
  }
  return maxSuffix + 1U;
}

std::uint64_t nextEditableFloorIndex(const EditableRoomDocument& document) {
  return nextEditableIndexForPrefix(document, "edit_floor_", true);
}

std::uint64_t nextEditableWallIndex(const EditableRoomDocument& document) {
  return nextEditableIndexForPrefix(document, "edit_wall_", false);
}

std::uint64_t nextEditableObjectIndex(const EditableRoomDocument& document) {
  return nextEditableObjectIndexForPrefix(document, "edit_object_");
}

RoomEditResult applyRoomEditCommand(EditableRoomDocument& document,
                                    const RoomEditCommand& command) {
  switch (command.kind) {
    case RoomEditCommandKind::AddFloor:
      if (!validateFloor(command.floor)) {
        return makeEditResult(RoomEditStatus::InvalidPrimitive, command.floor.id);
      }
      if (primitiveIdExists(document, command.floor.id)) {
        return makeEditResult(RoomEditStatus::DuplicateId, command.floor.id);
      }
      document.floors.push_back(command.floor);
      return makeEditResult(RoomEditStatus::Applied, command.floor.id);

    case RoomEditCommandKind::DeleteFloor: {
      EditableRoomFloor* floor = findEditableFloorMutable(document, command.targetId);
      if (floor == nullptr) {
        return makeEditResult(RoomEditStatus::MissingPrimitive, command.targetId);
      }
      if (floor->locked) {
        return makeEditResult(RoomEditStatus::LockedPrimitive, command.targetId);
      }
      RoomEditResult result = makeEditResult(RoomEditStatus::Applied, command.targetId);
      result.affectedRuntimeIds = runtimeIdsForEditableFloor(*floor);
      std::erase_if(document.floors,
                    [&](const EditableRoomFloor& item) { return item.id == command.targetId; });
      return result;
    }

    case RoomEditCommandKind::SetFloorSemantics: {
      EditableRoomFloor* floor = findEditableFloorMutable(document, command.targetId);
      if (floor == nullptr) {
        return makeEditResult(RoomEditStatus::MissingPrimitive, command.targetId);
      }
      if (floor->locked) {
        return makeEditResult(RoomEditStatus::LockedPrimitive, command.targetId);
      }
      if (!validateSemantics(command.semantics)) {
        return makeEditResult(RoomEditStatus::InvalidSemantics, command.targetId);
      }
      floor->semantics = command.semantics;
      RoomEditResult result = makeEditResult(RoomEditStatus::Applied, command.targetId);
      result.affectedRuntimeIds = runtimeIdsForEditableFloor(*floor);
      return result;
    }

    case RoomEditCommandKind::MoveFloor: {
      EditableRoomFloor* floor = findEditableFloorMutable(document, command.targetId);
      if (floor == nullptr) {
        return makeEditResult(RoomEditStatus::MissingPrimitive, command.targetId);
      }
      if (floor->locked) {
        return makeEditResult(RoomEditStatus::LockedPrimitive, command.targetId);
      }
      EditableRoomFloor next = *floor;
      if (command.useStart) {
        if (!isFinite(command.positionMeters)) {
          return makeEditResult(RoomEditStatus::InvalidGeometry, command.targetId);
        }
        next.centerMeters.x = command.positionMeters.x;
        next.centerMeters.z = command.positionMeters.z;
      } else {
        if (!validHorizontalDelta(command.deltaMeters)) {
          return makeEditResult(RoomEditStatus::InvalidGeometry, command.targetId);
        }
        next.centerMeters.x += command.deltaMeters.x;
        next.centerMeters.z += command.deltaMeters.z;
      }
      if (!validateFloor(next)) {
        return makeEditResult(RoomEditStatus::InvalidGeometry, command.targetId);
      }
      *floor = next;
      RoomEditResult result = makeEditResult(RoomEditStatus::Applied, command.targetId);
      result.affectedRuntimeIds = runtimeIdsForEditableFloor(*floor);
      return result;
    }

    case RoomEditCommandKind::ResizeFloor: {
      EditableRoomFloor* floor = findEditableFloorMutable(document, command.targetId);
      if (floor == nullptr) {
        return makeEditResult(RoomEditStatus::MissingPrimitive, command.targetId);
      }
      if (floor->locked) {
        return makeEditResult(RoomEditStatus::LockedPrimitive, command.targetId);
      }
      EditableRoomFloor next = *floor;
      if (!std::isfinite(command.sizeMeters.x) || !std::isfinite(command.sizeMeters.z)) {
        return makeEditResult(RoomEditStatus::InvalidGeometry, command.targetId);
      }
      next.sizeMeters.x = command.sizeMeters.x;
      next.sizeMeters.z = command.sizeMeters.z;
      if (!validateFloor(next)) {
        return makeEditResult(RoomEditStatus::InvalidGeometry, command.targetId);
      }
      *floor = next;
      RoomEditResult result = makeEditResult(RoomEditStatus::Applied, command.targetId);
      result.affectedRuntimeIds = runtimeIdsForEditableFloor(*floor);
      return result;
    }

    case RoomEditCommandKind::AddWall:
      if (!validateWall(command.wall)) {
        return makeEditResult(RoomEditStatus::InvalidPrimitive, command.wall.id);
      }
      if (primitiveIdExists(document, command.wall.id)) {
        return makeEditResult(RoomEditStatus::DuplicateId, command.wall.id);
      }
      document.walls.push_back(command.wall);
      return makeEditResult(RoomEditStatus::Applied, command.wall.id);

    case RoomEditCommandKind::DeleteWall: {
      EditableRoomWall* wall = findEditableWallMutable(document, command.targetId);
      if (wall == nullptr) {
        return makeEditResult(RoomEditStatus::MissingPrimitive, command.targetId);
      }
      if (wall->locked) {
        return makeEditResult(RoomEditStatus::LockedPrimitive, command.targetId);
      }
      RoomEditResult result = makeEditResult(RoomEditStatus::Applied, command.targetId);
      result.affectedRuntimeIds = runtimeIdsForEditableWall(*wall);
      std::erase_if(document.walls,
                    [&](const EditableRoomWall& item) { return item.id == command.targetId; });
      return result;
    }

    case RoomEditCommandKind::SetWallSemantics: {
      EditableRoomWall* wall = findEditableWallMutable(document, command.targetId);
      if (wall == nullptr) {
        return makeEditResult(RoomEditStatus::MissingPrimitive, command.targetId);
      }
      if (wall->locked) {
        return makeEditResult(RoomEditStatus::LockedPrimitive, command.targetId);
      }
      if (!validateSemantics(command.semantics)) {
        return makeEditResult(RoomEditStatus::InvalidSemantics, command.targetId);
      }
      wall->semantics = command.semantics;
      RoomEditResult result = makeEditResult(RoomEditStatus::Applied, command.targetId);
      result.affectedRuntimeIds = runtimeIdsForEditableWall(*wall);
      return result;
    }

    case RoomEditCommandKind::MoveWall: {
      EditableRoomWall* wall = findEditableWallMutable(document, command.targetId);
      if (wall == nullptr) {
        return makeEditResult(RoomEditStatus::MissingPrimitive, command.targetId);
      }
      if (wall->locked) {
        return makeEditResult(RoomEditStatus::LockedPrimitive, command.targetId);
      }
      if (!validHorizontalDelta(command.deltaMeters)) {
        return makeEditResult(RoomEditStatus::InvalidGeometry, command.targetId);
      }
      EditableRoomWall next = *wall;
      next.startMeters.x += command.deltaMeters.x;
      next.startMeters.z += command.deltaMeters.z;
      next.endMeters.x += command.deltaMeters.x;
      next.endMeters.z += command.deltaMeters.z;
      if (!validateWall(next)) {
        return makeEditResult(RoomEditStatus::InvalidGeometry, command.targetId);
      }
      *wall = next;
      RoomEditResult result = makeEditResult(RoomEditStatus::Applied, command.targetId);
      result.affectedRuntimeIds = runtimeIdsForEditableWall(*wall);
      return result;
    }

    case RoomEditCommandKind::StretchWall: {
      EditableRoomWall* wall = findEditableWallMutable(document, command.targetId);
      if (wall == nullptr) {
        return makeEditResult(RoomEditStatus::MissingPrimitive, command.targetId);
      }
      if (wall->locked) {
        return makeEditResult(RoomEditStatus::LockedPrimitive, command.targetId);
      }
      const Vec3 newPoint = command.useStart ? command.startMeters : command.endMeters;
      if (!pointOnExistingWallAxis(*wall, newPoint, command.useStart)) {
        return makeEditResult(RoomEditStatus::InvalidGeometry, command.targetId);
      }
      EditableRoomWall next = *wall;
      if (command.useStart) {
        next.startMeters = newPoint;
      } else {
        next.endMeters = newPoint;
      }
      if (!validateWall(next)) {
        return makeEditResult(RoomEditStatus::InvalidGeometry, command.targetId);
      }
      *wall = next;
      RoomEditResult result = makeEditResult(RoomEditStatus::Applied, command.targetId);
      result.affectedRuntimeIds = runtimeIdsForEditableWall(*wall);
      return result;
    }

    case RoomEditCommandKind::RotateWall90: {
      EditableRoomWall* wall = findEditableWallMutable(document, command.targetId);
      if (wall == nullptr) {
        return makeEditResult(RoomEditStatus::MissingPrimitive, command.targetId);
      }
      if (wall->locked) {
        return makeEditResult(RoomEditStatus::LockedPrimitive, command.targetId);
      }
      const Vec3 centerPoint = (wall->startMeters + wall->endMeters) * 0.5F;
      const Vec3 half = (wall->endMeters - wall->startMeters) * 0.5F;
      const Vec3 rotatedHalf = command.rotateLeft ? Vec3{-half.z, half.y, half.x}
                                                  : Vec3{half.z, half.y, -half.x};
      EditableRoomWall next = *wall;
      next.startMeters = centerPoint - rotatedHalf;
      next.endMeters = centerPoint + rotatedHalf;
      if (!validateWall(next)) {
        return makeEditResult(RoomEditStatus::InvalidGeometry, command.targetId);
      }
      *wall = next;
      RoomEditResult result = makeEditResult(RoomEditStatus::Applied, command.targetId);
      result.affectedRuntimeIds = runtimeIdsForEditableWall(*wall);
      return result;
    }

    case RoomEditCommandKind::SetWallHeight:
    case RoomEditCommandKind::SetWallThickness: {
      EditableRoomWall* wall = findEditableWallMutable(document, command.targetId);
      if (wall == nullptr) {
        return makeEditResult(RoomEditStatus::MissingPrimitive, command.targetId);
      }
      if (wall->locked) {
        return makeEditResult(RoomEditStatus::LockedPrimitive, command.targetId);
      }
      if (!std::isfinite(command.scalarMeters) || command.scalarMeters <= kEpsilon) {
        return makeEditResult(RoomEditStatus::InvalidGeometry, command.targetId);
      }
      EditableRoomWall next = *wall;
      if (command.kind == RoomEditCommandKind::SetWallHeight) {
        next.heightMeters = command.scalarMeters;
      } else {
        next.thicknessMeters = command.scalarMeters;
      }
      if (!validateWall(next)) {
        return makeEditResult(RoomEditStatus::InvalidGeometry, command.targetId);
      }
      *wall = next;
      RoomEditResult result = makeEditResult(RoomEditStatus::Applied, command.targetId);
      result.affectedRuntimeIds = runtimeIdsForEditableWall(*wall);
      return result;
    }

    case RoomEditCommandKind::AddObject:
      // branch-gate: BG-1125
      if (!validateObject(command.object)) {
        return makeEditResult(RoomEditStatus::InvalidPrimitive, command.object.id);
      }
      // branch-gate: BG-1125
      if (primitiveIdExists(document, command.object.id)) {
        return makeEditResult(RoomEditStatus::DuplicateId, command.object.id);
      }
      document.objects.push_back(command.object);
      return makeEditResult(RoomEditStatus::Applied, command.object.id);

    case RoomEditCommandKind::DeleteObject: {
      EditableRoomObject* object =
          findEditableObjectMutable(document, command.targetId);
      // branch-gate: BG-1125
      if (object == nullptr) {
        return makeEditResult(RoomEditStatus::MissingPrimitive, command.targetId);
      }
      // branch-gate: BG-1125
      if (object->locked) {
        return makeEditResult(RoomEditStatus::LockedPrimitive, command.targetId);
      }
      RoomEditResult result =
          makeEditResult(RoomEditStatus::Applied, command.targetId);
      result.affectedRuntimeIds = runtimeIdsForEditableObject(*object);
      std::erase_if(document.objects,
                    [&](const EditableRoomObject& item) {
                      return item.id == command.targetId;
                    });
      return result;
    }

    case RoomEditCommandKind::MoveObject: {
      EditableRoomObject* object =
          findEditableObjectMutable(document, command.targetId);
      // branch-gate: BG-1125
      if (object == nullptr) {
        return makeEditResult(RoomEditStatus::MissingPrimitive, command.targetId);
      }
      // branch-gate: BG-1125
      if (object->locked) {
        return makeEditResult(RoomEditStatus::LockedPrimitive, command.targetId);
      }
      EditableRoomObject next = *object;
      // branch-gate: BG-1125
      if (command.useStart) {
        // branch-gate: BG-1125
        if (!isFinite(command.positionMeters)) {
          return makeEditResult(RoomEditStatus::InvalidGeometry, command.targetId);
        }
        next.positionMeters = command.positionMeters;
      } else {
        // branch-gate: BG-1125
        if (!isFinite(command.deltaMeters)) {
          return makeEditResult(RoomEditStatus::InvalidGeometry, command.targetId);
        }
        next.positionMeters = next.positionMeters + command.deltaMeters;
      }
      // branch-gate: BG-1125
      if (!validateObject(next)) {
        return makeEditResult(RoomEditStatus::InvalidGeometry, command.targetId);
      }
      *object = next;
      RoomEditResult result =
          makeEditResult(RoomEditStatus::Applied, command.targetId);
      result.affectedRuntimeIds = runtimeIdsForEditableObject(*object);
      return result;
    }
  }
  return makeEditResult(RoomEditStatus::InvalidCommand);
}

RoomBakeResult bakeEditableRoomDocument(const EditableRoomDocument& document) {
  RoomBakeResult result;
  if (document.id.empty() || document.sourceFile.empty() || document.sourceSubset.empty()) {
    result.reasonCode = "room_bake_invalid_document";
    return result;
  }

  result.room.id = document.id;
  result.room.version = document.version;
  result.room.units = "m";
  result.room.source = document.source;
  result.room.sourceFile = document.sourceFile;
  result.room.sourceSubset = document.sourceSubset;
  for (const EditableRoomFloor& floor : document.floors) {
    if (!validateFloor(floor)) {
      result.reasonCode = "room_bake_invalid_floor";
      return result;
    }
    appendFloorRuntime(result.room, floor);
  }
  for (const EditableRoomWall& wall : document.walls) {
    if (!validateWall(wall)) {
      result.reasonCode = "room_bake_invalid_wall";
      return result;
    }
    appendWallRuntime(result.room, wall);
  }
  for (const EditableRoomObject& object : document.objects) {
    // branch-gate: BG-1125
    if (!validateObject(object)) {
      result.reasonCode = "room_bake_invalid_object";
      return result;
    }
    appendObjectRuntime(result.room, object);
  }

  result.ok = true;
  result.reasonCode = "room_bake_ok";
  return result;
}

const char* roomEditStatusName(RoomEditStatus status) {
  switch (status) {
    case RoomEditStatus::Applied:
      return "room_edit_applied";
    case RoomEditStatus::UndoApplied:
      return "room_edit_undo_applied";
    case RoomEditStatus::RedoApplied:
      return "room_edit_redo_applied";
    case RoomEditStatus::InvalidCommand:
      return "room_edit_invalid_command";
    case RoomEditStatus::DuplicateId:
      return "room_edit_duplicate_id";
    case RoomEditStatus::MissingPrimitive:
      return "room_edit_missing_primitive";
    case RoomEditStatus::LockedPrimitive:
      return "room_edit_locked_primitive";
    case RoomEditStatus::InvalidPrimitive:
      return "room_edit_invalid_primitive";
    case RoomEditStatus::InvalidGeometry:
      return "room_edit_invalid_geometry";
    case RoomEditStatus::InvalidSemantics:
      return "room_edit_invalid_semantics";
    case RoomEditStatus::NothingToUndo:
      return "room_edit_nothing_to_undo";
    case RoomEditStatus::NothingToRedo:
      return "room_edit_nothing_to_redo";
  }
  return "room_edit_invalid_command";
}

const char* roomEditCommandKindName(RoomEditCommandKind kind) {
  switch (kind) {
    case RoomEditCommandKind::AddFloor:
      return "add_floor";
    case RoomEditCommandKind::DeleteFloor:
      return "delete_floor";
    case RoomEditCommandKind::SetFloorSemantics:
      return "set_floor_semantics";
    case RoomEditCommandKind::MoveFloor:
      return "move_floor";
    case RoomEditCommandKind::ResizeFloor:
      return "resize_floor";
    case RoomEditCommandKind::AddWall:
      return "add_wall";
    case RoomEditCommandKind::DeleteWall:
      return "delete_wall";
    case RoomEditCommandKind::SetWallSemantics:
      return "set_wall_semantics";
    case RoomEditCommandKind::MoveWall:
      return "move_wall";
    case RoomEditCommandKind::StretchWall:
      return "stretch_wall";
    case RoomEditCommandKind::RotateWall90:
      return "rotate_wall_90";
    case RoomEditCommandKind::SetWallHeight:
      return "set_wall_height";
    case RoomEditCommandKind::SetWallThickness:
      return "set_wall_thickness";
    case RoomEditCommandKind::AddObject:
      return "add_object";
    case RoomEditCommandKind::DeleteObject:
      return "delete_object";
    case RoomEditCommandKind::MoveObject:
      return "move_object";
  }
  return "add_floor";
}

EditableRoomSession::EditableRoomSession(EditableRoomDocument document)
    : document_(std::move(document)) {}

const EditableRoomDocument& EditableRoomSession::document() const {
  return document_;
}

RoomEditResult EditableRoomSession::submit(const RoomEditCommand& command) {
  EditableRoomDocument next = document_;
  RoomEditResult result = applyRoomEditCommand(next, command);
  if (result.status != RoomEditStatus::Applied) {
    return result;
  }

  undoStack_.push_back({command, document_, next, result});
  redoStack_.clear();
  document_ = std::move(next);
  return result;
}

RoomEditResult EditableRoomSession::undo() {
  if (undoStack_.empty()) {
    return makeEditResult(RoomEditStatus::NothingToUndo);
  }
  HistoryEntry entry = std::move(undoStack_.back());
  undoStack_.pop_back();
  document_ = entry.before;
  redoStack_.push_back(entry);
  RoomEditResult result = entry.result;
  result.status = RoomEditStatus::UndoApplied;
  result.reasonCode = roomEditStatusName(result.status);
  return result;
}

RoomEditResult EditableRoomSession::redo() {
  if (redoStack_.empty()) {
    return makeEditResult(RoomEditStatus::NothingToRedo);
  }
  HistoryEntry entry = std::move(redoStack_.back());
  redoStack_.pop_back();
  document_ = entry.after;
  undoStack_.push_back(entry);
  RoomEditResult result = undoStack_.back().result;
  result.status = RoomEditStatus::RedoApplied;
  result.reasonCode = roomEditStatusName(result.status);
  return result;
}

std::size_t EditableRoomSession::undoDepth() const {
  return undoStack_.size();
}

std::size_t EditableRoomSession::redoDepth() const {
  return redoStack_.size();
}

}  // namespace iggy3d
