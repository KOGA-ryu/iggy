#include "app/iggy3d/creative/adapters/RoomBake.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

namespace iggy3d::creative {
namespace {

struct BakeBounds {
  Vec3 min;
  Vec3 max;
  Vec3 center;
  Vec3 size;
};

enum class BakedRoomRole {
  Unsupported,
  Floor,
  Wall,
  Prop,
};

[[nodiscard]] bool finite(double value) noexcept {
  return std::isfinite(value);
}

[[nodiscard]] bool finite(CreativeVec3 value) noexcept {
  return finite(value.x) && finite(value.y) && finite(value.z);
}

[[nodiscard]] bool fitsFloat(double value) noexcept {
  return std::fabs(value) <=
         static_cast<double>(std::numeric_limits<float>::max());
}

[[nodiscard]] float toFloat(double value) noexcept {
  return static_cast<float>(value);
}

[[nodiscard]] Vec3 toVec3(CreativeVec3 value) noexcept {
  return {toFloat(value.x), toFloat(value.y), toFloat(value.z)};
}

[[nodiscard]] bool validAnchorPosition(CreativeVec3 position) noexcept {
  return finite(position) && fitsFloat(position.x) && fitsFloat(position.y) &&
         fitsFloat(position.z);
}

[[nodiscard]] bool validBakeBounds(CreativeBounds bounds,
                                   BakeBounds& baked) noexcept {
  if (!finite(bounds.min) || !finite(bounds.max) ||
      !(bounds.max.x > bounds.min.x) || !(bounds.max.y > bounds.min.y) ||
      !(bounds.max.z > bounds.min.z)) {
    return false;
  }

  const double sizeX = bounds.max.x - bounds.min.x;
  const double sizeY = bounds.max.y - bounds.min.y;
  const double sizeZ = bounds.max.z - bounds.min.z;
  const double centerX = bounds.min.x + sizeX * 0.5;
  const double centerY = bounds.min.y + sizeY * 0.5;
  const double centerZ = bounds.min.z + sizeZ * 0.5;
  const double maxFloat = static_cast<double>(std::numeric_limits<float>::max());
  const double values[] = {bounds.min.x, bounds.min.y, bounds.min.z,
                           bounds.max.x, bounds.max.y, bounds.max.z,
                           sizeX,        sizeY,        sizeZ,
                           centerX,      centerY,      centerZ};
  for (const double value : values) {
    if (std::fabs(value) > maxFloat) {
      return false;
    }
  }

  baked.min = {toFloat(bounds.min.x), toFloat(bounds.min.y),
               toFloat(bounds.min.z)};
  baked.max = {toFloat(bounds.max.x), toFloat(bounds.max.y),
               toFloat(bounds.max.z)};
  baked.center = {toFloat(centerX), toFloat(centerY), toFloat(centerZ)};
  baked.size = {toFloat(sizeX), toFloat(sizeY), toFloat(sizeZ)};
  return true;
}

[[nodiscard]] bool occupancySupportsRuntimeRoomGeometry(
    CreativeSpatialOccupancyKind occupancy) noexcept {
  return occupancy == CreativeSpatialOccupancyKind::Structural ||
         occupancy == CreativeSpatialOccupancyKind::Collision ||
         occupancy == CreativeSpatialOccupancyKind::Navigation;
}

[[nodiscard]] bool descriptorSupportsRuntimeRoomGeometry(
    const CreativeObjectDescriptor& descriptor) noexcept {
  return descriptor.isRuntimeMeaningful ||
         occupancySupportsRuntimeRoomGeometry(descriptor.occupancyKind);
}

[[nodiscard]] bool occupancySupportsRuntimeRoomAnchor(
    CreativeSpatialOccupancyKind occupancy) noexcept {
  return occupancy == CreativeSpatialOccupancyKind::Navigation ||
         occupancy == CreativeSpatialOccupancyKind::Gameplay ||
         occupancy == CreativeSpatialOccupancyKind::Light ||
         occupancy == CreativeSpatialOccupancyKind::Audio ||
         occupancy == CreativeSpatialOccupancyKind::Camera;
}

[[nodiscard]] bool descriptorSupportsRuntimeRoomAnchor(
    const CreativeObjectDescriptor& descriptor) noexcept {
  return descriptor.shapeKind == CreativeObjectShapeKind::Point &&
         descriptor.hasTransform &&
         occupancySupportsRuntimeRoomAnchor(descriptor.occupancyKind);
}

[[nodiscard]] bool projectionSupportsBoundsBackedLineGeometry(
    CreativeSpatialProjectionProfile projection) noexcept {
  return projection == CreativeSpatialProjectionProfile::BoxProjection ||
         projection == CreativeSpatialProjectionProfile::LineProjection;
}

[[nodiscard]] bool descriptorSupportsBoundsBackedLineGeometry(
    const CreativeObjectDescriptor& descriptor) noexcept {
  return descriptor.shapeKind == CreativeObjectShapeKind::Line &&
         descriptor.hasBounds &&
         projectionSupportsBoundsBackedLineGeometry(descriptor.projectionProfile);
}

[[nodiscard]] bool descriptorSupportsStaticMeshBake(
    const CreativeObjectDescriptor& descriptor) noexcept {
  switch (descriptor.shapeKind) {
    case CreativeObjectShapeKind::Surface:
    case CreativeObjectShapeKind::MeshProxy:
    case CreativeObjectShapeKind::BoxVolume:
      return true;
    case CreativeObjectShapeKind::Line:
      return descriptorSupportsBoundsBackedLineGeometry(descriptor);
    case CreativeObjectShapeKind::Unknown:
    case CreativeObjectShapeKind::Point:
    case CreativeObjectShapeKind::Path:
      return false;
  }
  return false;
}

[[nodiscard]] bool horizontalSurface(BakeBounds bounds) noexcept {
  return bounds.size.y <= bounds.size.x && bounds.size.y <= bounds.size.z;
}

[[nodiscard]] bool standingSurface(BakeBounds bounds) noexcept {
  return bounds.size.y > bounds.size.x || bounds.size.y > bounds.size.z;
}

[[nodiscard]] BakedRoomRole roleForObject(
    const CreativeObjectDescriptor& descriptor,
    BakeBounds bounds) noexcept {
  if (!descriptorSupportsRuntimeRoomGeometry(descriptor)) {
    return BakedRoomRole::Unsupported;
  }

  switch (descriptor.shapeKind) {
    case CreativeObjectShapeKind::Surface:
      if (descriptor.occupancyKind == CreativeSpatialOccupancyKind::Structural &&
          horizontalSurface(bounds)) {
        return BakedRoomRole::Floor;
      }
      if (descriptor.occupancyKind == CreativeSpatialOccupancyKind::Structural &&
          standingSurface(bounds)) {
        return BakedRoomRole::Wall;
      }
      return BakedRoomRole::Prop;
    case CreativeObjectShapeKind::MeshProxy:
    case CreativeObjectShapeKind::BoxVolume:
      return BakedRoomRole::Prop;
    case CreativeObjectShapeKind::Line:
      if (descriptorSupportsBoundsBackedLineGeometry(descriptor)) {
        return BakedRoomRole::Prop;
      }
      return BakedRoomRole::Unsupported;
    case CreativeObjectShapeKind::Unknown:
    case CreativeObjectShapeKind::Point:
    case CreativeObjectShapeKind::Path:
      return BakedRoomRole::Unsupported;
  }

  return BakedRoomRole::Unsupported;
}

[[nodiscard]] std::string_view roleName(BakedRoomRole role) noexcept {
  switch (role) {
    case BakedRoomRole::Floor:
      return "floor";
    case BakedRoomRole::Wall:
      return "wall";
    case BakedRoomRole::Prop:
      return "prop";
    case BakedRoomRole::Unsupported:
      return "";
  }
  return "";
}

[[nodiscard]] std::string_view meshIdForRole(BakedRoomRole role) noexcept {
  switch (role) {
    case BakedRoomRole::Floor:
      return "creative_floor_rect";
    case BakedRoomRole::Wall:
      return "creative_wall_segment";
    case BakedRoomRole::Prop:
      return "creative_box_proxy";
    case BakedRoomRole::Unsupported:
      return "creative_unsupported";
  }
  return "creative_unsupported";
}

[[nodiscard]] std::string_view materialIdForRole(BakedRoomRole role) noexcept {
  switch (role) {
    case BakedRoomRole::Floor:
      return "creative_floor";
    case BakedRoomRole::Wall:
      return "creative_wall";
    case BakedRoomRole::Prop:
      return "creative_prop";
    case BakedRoomRole::Unsupported:
      return "creative_unsupported";
  }
  return "creative_unsupported";
}

[[nodiscard]] std::string stableObjectId(
    const CreativeObject& object,
    std::string_view suffix = {}) {
  std::string id = "creative_object_" + std::to_string(object.id);
  if (!suffix.empty()) {
    id += "_";
    id += suffix;
  }
  return id;
}

[[nodiscard]] std::string_view anchorKindForDescriptor(
    const CreativeObjectDescriptor& descriptor) noexcept {
  switch (descriptor.occupancyKind) {
    case CreativeSpatialOccupancyKind::Navigation:
      return "navigation";
    case CreativeSpatialOccupancyKind::Gameplay:
      return "gameplay";
    case CreativeSpatialOccupancyKind::Light:
      return "light";
    case CreativeSpatialOccupancyKind::Audio:
      return "audio";
    case CreativeSpatialOccupancyKind::Camera:
      return "camera";
    case CreativeSpatialOccupancyKind::Unknown:
    case CreativeSpatialOccupancyKind::Structural:
    case CreativeSpatialOccupancyKind::Collision:
    case CreativeSpatialOccupancyKind::Trigger:
    case CreativeSpatialOccupancyKind::Testing:
    case CreativeSpatialOccupancyKind::Authoring:
      return "";
  }
  return "";
}

[[nodiscard]] RoomAnchorAsset anchorForObject(
    const CreativeObject& object,
    const CreativeObjectDescriptor& descriptor) {
  RoomAnchorAsset anchor;
  anchor.id = stableObjectId(object, "anchor");
  anchor.kind = std::string(anchorKindForDescriptor(descriptor));
  anchor.runtimeStableName = stableObjectId(object);
  anchor.positionMeters = toVec3(object.transform.position);
  return anchor;
}

void setWallSegmentFields(RoomStaticMeshAsset& mesh, BakeBounds bounds) {
  const bool runsAlongX = bounds.size.x >= bounds.size.z;
  mesh.hasWallSegment = true;
  mesh.wallBottomY = bounds.min.y;
  mesh.wallHeightMeters = bounds.size.y;
  if (runsAlongX) {
    mesh.wallStartMeters = {bounds.min.x, bounds.min.y, bounds.center.z};
    mesh.wallEndMeters = {bounds.max.x, bounds.min.y, bounds.center.z};
    mesh.wallThicknessMeters = bounds.size.z;
  } else {
    mesh.wallStartMeters = {bounds.center.x, bounds.min.y, bounds.min.z};
    mesh.wallEndMeters = {bounds.center.x, bounds.min.y, bounds.max.z};
    mesh.wallThicknessMeters = bounds.size.x;
  }
}

[[nodiscard]] RoomStaticMeshAsset staticMeshForObject(
    const CreativeObject& object,
    BakeBounds bounds,
    BakedRoomRole role) {
  RoomStaticMeshAsset mesh;
  mesh.id = stableObjectId(object);
  mesh.meshId = std::string(meshIdForRole(role));
  mesh.materialId = std::string(materialIdForRole(role));
  mesh.role = std::string(roleName(role));
  mesh.positionMeters = bounds.center;
  mesh.sizeMeters = bounds.size;
  if (role == BakedRoomRole::Wall) {
    setWallSegmentFields(mesh, bounds);
  }
  return mesh;
}

[[nodiscard]] std::vector<Vec3> topFacePoints(BakeBounds bounds) {
  return {{bounds.min.x, bounds.max.y, bounds.min.z},
          {bounds.max.x, bounds.max.y, bounds.min.z},
          {bounds.max.x, bounds.max.y, bounds.max.z},
          {bounds.min.x, bounds.max.y, bounds.max.z}};
}

[[nodiscard]] std::vector<Vec3> boxExtentPoints(BakeBounds bounds) {
  return {{bounds.min.x, bounds.min.y, bounds.min.z},
          {bounds.max.x, bounds.min.y, bounds.min.z},
          {bounds.max.x, bounds.max.y, bounds.max.z},
          {bounds.min.x, bounds.max.y, bounds.max.z}};
}

[[nodiscard]] RoomSpatialSurface walkableSurfaceForObject(
    const CreativeObject& object,
    BakeBounds bounds) {
  RoomSpatialSurface surface;
  surface.id = stableObjectId(object, "walkable");
  surface.sourceStaticMeshId = stableObjectId(object);
  surface.shape = RoomSpatialSurfaceShape::Plane;
  surface.role = RoomSpatialSurfaceRole::Walkable;
  surface.pointsMeters = topFacePoints(bounds);
  surface.normal = {0.0F, 1.0F, 0.0F};
  surface.traversalTags = {"walkable"};
  surface.collisionMask = {"actor"};
  surface.blocksActor = false;
  surface.blocksProjectile = false;
  return surface;
}

[[nodiscard]] RoomSpatialSurface actorBlockerSurfaceForObject(
    const CreativeObject& object,
    BakeBounds bounds) {
  RoomSpatialSurface surface;
  surface.id = stableObjectId(object, "actor_blocker");
  surface.sourceStaticMeshId = stableObjectId(object);
  surface.shape = RoomSpatialSurfaceShape::Box;
  surface.role = RoomSpatialSurfaceRole::Blocker;
  surface.pointsMeters = boxExtentPoints(bounds);
  surface.normal = {0.0F, 0.0F, 1.0F};
  surface.traversalTags = {"blocker"};
  surface.collisionMask = {"actor"};
  surface.blocksActor = true;
  surface.blocksProjectile = false;
  return surface;
}

[[nodiscard]] RoomSpatialSurface projectileBlockerSurfaceForObject(
    const CreativeObject& object,
    BakeBounds bounds) {
  RoomSpatialSurface surface;
  surface.id = stableObjectId(object, "projectile_blocker");
  surface.sourceStaticMeshId = stableObjectId(object);
  surface.shape = RoomSpatialSurfaceShape::Box;
  surface.role = RoomSpatialSurfaceRole::ProjectileBlocker;
  surface.pointsMeters = boxExtentPoints(bounds);
  surface.normal = {0.0F, 0.0F, 1.0F};
  surface.traversalTags = {"projectile_blocker"};
  surface.collisionMask = {"projectile"};
  surface.blocksActor = false;
  surface.blocksProjectile = true;
  return surface;
}

void appendSpatialSurfaces(RoomAsset& room,
                           const CreativeObject& object,
                           const CreativeObjectDescriptor& descriptor,
                           BakeBounds bounds,
                           BakedRoomRole role) {
  if (role == BakedRoomRole::Floor) {
    room.spatialSurfaces.push_back(walkableSurfaceForObject(object, bounds));
    return;
  }

  if (descriptor.occupancyKind == CreativeSpatialOccupancyKind::Structural ||
      descriptor.occupancyKind == CreativeSpatialOccupancyKind::Collision) {
    room.spatialSurfaces.push_back(actorBlockerSurfaceForObject(object, bounds));
    room.spatialSurfaces.push_back(
        projectileBlockerSurfaceForObject(object, bounds));
  }
}

void setStatus(CreativeRoomBakeReceipt& receipt,
               CreativeRoomBakeStatus status,
               std::string reasonCode,
               bool accepted = false) {
  receipt.status = status;
  receipt.reasonCode = std::move(reasonCode);
  receipt.message = receipt.reasonCode;
  receipt.accepted = accepted;
}

}  // namespace

std::string_view toString(CreativeRoomBakeStatus status) noexcept {
  switch (status) {
    case CreativeRoomBakeStatus::Unknown:
      return "unknown";
    case CreativeRoomBakeStatus::MissingDocument:
      return "missing_document";
    case CreativeRoomBakeStatus::InvalidDocument:
      return "invalid_document";
    case CreativeRoomBakeStatus::NoRenderableObjects:
      return "no_renderable_objects";
    case CreativeRoomBakeStatus::Baked:
      return "baked";
  }
  return "unknown";
}

CreativeRoomBakeResult buildRoomAssetFromCreativeDocument(
    const CreativeRoomBakeRequest& request) {
  CreativeRoomBakeResult result;
  result.receipt.requested = true;
  result.room.id = request.roomId.empty() ? "creative_room" : request.roomId;
  result.room.version = 1;
  result.room.units = "m";
  result.room.source = "iggy3d.creative_document";
  result.room.sourceFile = request.sourceName;
  result.room.sourceSubset = request.sourceSubset;

  if (request.document == nullptr) {
    setStatus(result.receipt,
              CreativeRoomBakeStatus::MissingDocument,
              "creative_room_bake_document_missing");
    return result;
  }

  const CreativeDocument& document = *request.document;
  if (!document.isValid()) {
    setStatus(result.receipt,
              CreativeRoomBakeStatus::InvalidDocument,
              "creative_room_bake_document_invalid");
    return result;
  }

  result.receipt.objectCount = document.objectCount();
  for (const CreativeObject& object : document.objects()) {
    if (!object.visible && !request.includeHidden) {
      ++result.receipt.skippedHiddenCount;
      continue;
    }

    const CreativeObjectDescriptor& descriptor = describeObject(object.kind);
    if (descriptor.isEditorOnly) {
      ++result.receipt.skippedEditorOnlyCount;
      continue;
    }

    if (object.kind == CreativeObjectKind::Room) {
      ++result.receipt.skippedRoomMetadataCount;
      continue;
    }

    ++result.receipt.consideredObjectCount;
    if (descriptor.shapeKind == CreativeObjectShapeKind::Point) {
      if (!descriptorSupportsRuntimeRoomAnchor(descriptor) ||
          !validAnchorPosition(object.transform.position)) {
        ++result.receipt.skippedUnsupportedAnchorCount;
        continue;
      }

      result.room.anchors.push_back(anchorForObject(object, descriptor));
      continue;
    }

    if (!descriptorSupportsStaticMeshBake(descriptor)) {
      ++result.receipt.skippedUnsupportedShapeCount;
      continue;
    }

    if (!descriptor.hasBounds) {
      ++result.receipt.skippedNoBoundsCount;
      continue;
    }

    BakeBounds bounds;
    if (!validBakeBounds(object.bounds, bounds)) {
      ++result.receipt.skippedNoBoundsCount;
      continue;
    }

    const BakedRoomRole role = roleForObject(descriptor, bounds);
    if (role == BakedRoomRole::Unsupported) {
      ++result.receipt.skippedUnsupportedShapeCount;
      continue;
    }

    result.room.staticMeshes.push_back(staticMeshForObject(object, bounds, role));
    appendSpatialSurfaces(result.room, object, descriptor, bounds, role);
  }

  result.receipt.bakedStaticMeshCount = result.room.staticMeshes.size();
  result.receipt.bakedAnchorCount = result.room.anchors.size();
  result.receipt.bakedSpatialSurfaceCount = result.room.spatialSurfaces.size();

  if (result.room.staticMeshes.empty() && result.room.anchors.empty()) {
    setStatus(result.receipt,
              CreativeRoomBakeStatus::NoRenderableObjects,
              "creative_room_bake_no_renderable_objects");
    return result;
  }

  setStatus(result.receipt,
            CreativeRoomBakeStatus::Baked,
            "creative_room_baked",
            true);
  return result;
}

}  // namespace iggy3d::creative
