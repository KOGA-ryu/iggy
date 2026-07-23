#include "app/iggy3d/creative/document/DocumentInternal.hpp"

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/Hierarchy.hpp"
#include "content/assets/StaticMeshAsset.hpp"

#include <cmath>

namespace iggy3d::creative::document_internal {

bool isPathDescriptor(const CreativeObjectDescriptor& descriptor) noexcept {
  return objectStoresPathPoints(descriptor.kind);
}

bool isEndpointLineDescriptor(
    const CreativeObjectDescriptor& descriptor) noexcept {
  return descriptor.shapeKind == CreativeObjectShapeKind::Line &&
         descriptor.projectionProfile ==
             CreativeSpatialProjectionProfile::LinkProjection &&
         !descriptor.hasBounds;
}

bool pathPointsAreFinite(std::span<const CreativePathPoint> pathPoints) noexcept {
  for (const CreativePathPoint& point : pathPoints) {
    if (!isValidCreativePathPoint(point)) {
      return false;
    }
  }

  return true;
}

bool pathPointsAreValid(std::span<const CreativePathPoint> pathPoints) noexcept {
  return pathPoints.size() >= 2U && pathPointsAreFinite(pathPoints);
}

bool lineEndpointsAreValid(
    std::span<const CreativePathPoint> pathPoints) noexcept {
  return pathPoints.size() == 2U && pathPointsAreFinite(pathPoints);
}

std::string_view validateCreatePathPayload(
    const CreativeObjectDescriptor& descriptor,
    const CreativeDocumentCreateRequest& request) noexcept {
  const bool hasPathPayload =
      request.hasPathOverride || !request.pathPoints.empty();
  if (isPathDescriptor(descriptor)) {
    if (!request.hasPathOverride) {
      return "path_override_required";
    }
    if (descriptor.kind == CreativeObjectKind::MovingPlatform &&
        request.pathPoints.size() >
            kCreativeMovingPlatformPathPointCapacity) {
      return "moving_platform_path_too_long";
    }
    return (descriptor.kind == CreativeObjectKind::MovingPlatform
                ? isValidCreativeMovingPlatformPath(request.pathPoints)
                : pathPointsAreValid(request.pathPoints))
               ? std::string_view{}
               : std::string_view{"invalid_path_points"};
  }

  if (isEndpointLineDescriptor(descriptor)) {
    if (!request.hasPathOverride) {
      return "line_endpoint_override_required";
    }
    if (!lineEndpointsAreValid(request.pathPoints)) {
      return "invalid_line_endpoints";
    }
    return {};
  }

  if (hasPathPayload) {
    return "path_unsupported";
  }
  return {};
}

std::string_view validateRestoredPathPayload(
    const CreativeObjectDescriptor& descriptor,
    const CreativeObject& object) noexcept {
  if (isPathDescriptor(descriptor)) {
    if (descriptor.kind == CreativeObjectKind::MovingPlatform &&
        object.pathPoints.size() >
            kCreativeMovingPlatformPathPointCapacity) {
      return "moving_platform_path_too_long";
    }
    return (descriptor.kind == CreativeObjectKind::MovingPlatform
                ? isValidCreativeMovingPlatformPath(object.pathPoints)
                : pathPointsAreValid(object.pathPoints))
               ? std::string_view{}
               : std::string_view{"invalid_path_points"};
  }

  if (isEndpointLineDescriptor(descriptor)) {
    return lineEndpointsAreValid(object.pathPoints) ? std::string_view{}
                                                    : "invalid_line_endpoints";
  }

  return object.pathPoints.empty() ? std::string_view{} : "path_unsupported";
}

std::string_view validateRestoredParentPayload(
    const CreativeObjectDescriptor& descriptor,
    const CreativeObject& object,
    std::span<const CreativeObject> restoredObjects,
    const std::unordered_map<CreativeObjectId, std::size_t>& restoredIndex)
    noexcept {
  if (!object.parentId.has_value()) {
    return object.attachmentSocket.empty()
               ? std::string_view{}
               : std::string_view{"attachment_socket_without_parent"};
  }

  if (!object.attachmentSocket.empty() &&
      !validCreativeAttachmentSocketName(object.attachmentSocket)) {
    return "attachment_socket_invalid";
  }

  if (!descriptor.canHaveParent) {
    return "parent_unsupported";
  }

  const CreativeObjectId parentId = *object.parentId;
  if (parentId == kInvalidObjectId || parentId == object.id) {
    return "invalid_parent";
  }

  const auto parentIt = restoredIndex.find(parentId);
  if (parentIt == restoredIndex.end()) {
    return "missing_parent";
  }

  const CreativeObject& parentObject = restoredObjects[parentIt->second];
  const CreativeObjectDescriptor& parentDescriptor =
      describeObject(parentObject.kind);
  return parentDescriptor.canOwnChildren
             ? std::string_view{}
             : std::string_view{"parent_owner_unsupported"};
}

bool parentGraphContainsCycle(
    std::span<const CreativeObject> objects,
    const std::unordered_map<CreativeObjectId, std::size_t>& objectIndex)
    noexcept {
  for (const CreativeObject& object : objects) {
    std::optional<CreativeObjectId> parentId = object.parentId;
    std::size_t hopCount = 0;
    while (parentId.has_value()) {
      if (*parentId == object.id || hopCount >= objects.size()) {
        return true;
      }

      const auto parentIt = objectIndex.find(*parentId);
      if (parentIt == objectIndex.end()) {
        break;
      }
      parentId = objects[parentIt->second].parentId;
      ++hopCount;
    }
  }

  return false;
}

bool parentGraphExceedsDepthCapacity(
    std::span<const CreativeObject> objects,
    const std::unordered_map<CreativeObjectId, std::size_t>& objectIndex)
    noexcept {
  for (const CreativeObject& object : objects) {
    std::optional<CreativeObjectId> parentId = object.parentId;
    std::size_t depth = 0U;
    while (parentId.has_value()) {
      ++depth;
      if (depth > kCreativeHierarchyDepthCapacity) {
        return true;
      }
      const auto parentIt = objectIndex.find(*parentId);
      if (parentIt == objectIndex.end()) {
        break;
      }
      parentId = objects[parentIt->second].parentId;
    }
  }
  return false;
}

bool isValidUnits(CreativeUnits units) noexcept {
  return units == CreativeUnits::Meters;
}

bool isValidGridSettings(CreativeGridSettings settings) noexcept {
  return isFiniteCreativeVec3(settings.origin) &&
         std::isfinite(settings.cellSizeMeters) &&
         settings.cellSizeMeters > 0.0 && settings.size.width >= 0 &&
         settings.size.height >= 0 && settings.size.depth >= 0;
}

bool isValidWorldBounds(CreativeBounds bounds) noexcept {
  return isFiniteCreativeVec3(bounds.min) &&
         isFiniteCreativeVec3(bounds.max);
}

bool isValidRestoreObject(const CreativeObject& object) noexcept {
  const CreativeObjectDescriptor& descriptor = describeObject(object.kind);
  return object.id != kInvalidObjectId &&
         object.kind != CreativeObjectKind::Unknown &&
         descriptor.kind == object.kind &&
         descriptor.kind != CreativeObjectKind::Unknown &&
         isFiniteCreativeVec3(object.transform.position) &&
         isFiniteCreativeVec3(object.transform.rotationEulerRadians) &&
         isFiniteCreativeVec3(object.transform.scale) &&
         isValidWorldBounds(object.bounds) &&
         (object.assetId.empty()
              ? object.assetContentHash == 0U &&
                    object.assetMaterialVariant.empty()
              : validStaticMeshAssetId(object.assetId) &&
                    validCreativeAssetMaterialVariantName(
                        object.assetMaterialVariant)) &&
         (object.kind != CreativeObjectKind::MovingPlatform ||
          isValidCreativeMovingPlatformSettings(object.movingPlatform)) &&
         (object.kind != CreativeObjectKind::Door ||
          isValidCreativeDoorSettings(object.door)) &&
         (object.kind != CreativeObjectKind::Window ||
          isValidCreativeWindowSettings(object.window)) &&
         (object.kind != CreativeObjectKind::SpawnPoint ||
          isValidCreativePlayerSpawnSettings(object.playerSpawn)) &&
         (object.kind == CreativeObjectKind::SpawnPoint ||
          object.playerSpawn == CreativePlayerSpawnSettings{}) &&
         ((object.kind != CreativeObjectKind::NpcSpawn &&
           object.kind != CreativeObjectKind::EnemySpawn) ||
          isValidCreativeNpcSpawnSettings(object.npcSpawn)) &&
         (object.kind == CreativeObjectKind::NpcSpawn ||
          object.kind == CreativeObjectKind::EnemySpawn ||
          object.npcSpawn == CreativeNpcSpawnSettings{}) &&
         (object.kind != CreativeObjectKind::LootPoint ||
          isValidCreativeLootPointSettings(object.lootPoint)) &&
         (object.kind == CreativeObjectKind::LootPoint ||
          object.lootPoint == CreativeLootPointSettings{}) &&
         (object.kind != CreativeObjectKind::ExitPoint ||
          isValidCreativeExitPointSettings(object.exitPoint)) &&
         (object.kind == CreativeObjectKind::ExitPoint ||
          object.exitPoint == CreativeExitPointSettings{});
}

}  // namespace iggy3d::creative::document_internal

namespace iggy3d::creative {

using document_internal::parentGraphContainsCycle;
using document_internal::validateRestoredParentPayload;

std::string_view validateCreativeObjectParentGraph(
    std::span<const CreativeObject> objects) {
  std::unordered_map<CreativeObjectId, std::size_t> objectIndex;
  objectIndex.reserve(objects.size());
  for (std::size_t index = 0; index < objects.size(); ++index) {
    objectIndex.emplace(objects[index].id, index);
  }

  for (const CreativeObject& object : objects) {
    const CreativeObjectDescriptor& descriptor = describeObject(object.kind);
    const std::string_view parentValidation =
        validateRestoredParentPayload(descriptor, object, objects, objectIndex);
    if (!parentValidation.empty()) {
      return parentValidation;
    }
  }

  for (std::size_t index = 0; index < objects.size(); ++index) {
    const CreativeObject& object = objects[index];
    if (object.attachmentSocket.empty()) {
      continue;
    }
    for (std::size_t previous = 0; previous < index; ++previous) {
      const CreativeObject& candidate = objects[previous];
      if (candidate.parentId == object.parentId &&
          candidate.attachmentSocket == object.attachmentSocket) {
        return "attachment_socket_occupied";
      }
    }
  }

  if (parentGraphContainsCycle(objects, objectIndex)) {
    return "parent_cycle";
  }
  if (document_internal::parentGraphExceedsDepthCapacity(objects,
                                                          objectIndex)) {
    return "parent_depth_exceeded";
  }

  return {};
}

}  // namespace iggy3d::creative
