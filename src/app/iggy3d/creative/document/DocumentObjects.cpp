#include "app/iggy3d/creative/document/Document.hpp"

#include "app/iggy3d/creative/document/DocumentInternal.hpp"
#include "app/iggy3d/creative/document/Hierarchy.hpp"
#include "content/assets/StaticMeshAsset.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>

namespace iggy3d::creative {

using document_internal::documentIdentityDirtyFlags;
using document_internal::validateCreatePathPayload;

namespace {

CreativeObjectDirtyFlags dirtyFlagsForRemoval(
    CreativeObjectKind objectKind) noexcept {
  return dirtyFlagsForCreation(objectKind) | documentIdentityDirtyFlags();
}

void setCreateStatus(CreativeDocumentCreateReceipt& receipt,
                     CreativeDocumentCreateStatus status,
                     std::string_view reason) noexcept {
  receipt.status = status;
  receipt.message = reason;
  receipt.reasonCode = reason;
}

void setRemoveStatus(CreativeDocumentRemoveReceipt& receipt,
                     CreativeDocumentRemoveStatus status,
                     std::string_view reason) noexcept {
  receipt.status = status;
  receipt.message = reason;
  receipt.reasonCode = reason;
}

std::string descriptorDefaultName(
    const CreativeObjectDescriptor& descriptor) {
  if (!descriptor.displayName.empty()) {
    return std::string{descriptor.displayName};
  }
  return std::string{descriptor.name};
}

bool objectHasChildren(std::span<const CreativeObject> objects,
                       CreativeObjectId parentId) noexcept {
  for (const CreativeObject& object : objects) {
    if (object.parentId.has_value() && *object.parentId == parentId) {
      return true;
    }
  }
  return false;
}

bool attachmentSocketOccupied(std::span<const CreativeObject> objects,
                              CreativeObjectId parentId,
                              std::string_view socket) noexcept {
  for (const CreativeObject& object : objects) {
    if (object.parentId == parentId && object.attachmentSocket == socket) {
      return true;
    }
  }
  return false;
}

CreativeBounds resolvedCreateBounds(
    const CreativeObjectDescriptor& descriptor,
    const CreativeDocumentCreateRequest& request) noexcept {
  if (request.hasBoundsOverride || !descriptor.hasBounds ||
      !request.hasTransformOverride) {
    return request.hasBoundsOverride ? request.bounds : descriptor.defaults.bounds;
  }

  const CreativeVec3 delta{
      request.transform.position.x - descriptor.defaults.transform.position.x,
      request.transform.position.y - descriptor.defaults.transform.position.y,
      request.transform.position.z - descriptor.defaults.transform.position.z,
  };
  CreativeBounds bounds = descriptor.defaults.bounds;
  bounds.min.x += delta.x;
  bounds.min.y += delta.y;
  bounds.min.z += delta.z;
  bounds.max.x += delta.x;
  bounds.max.y += delta.y;
  bounds.max.z += delta.z;
  return bounds;
}

}  // namespace

CreativeObject resolveCreativeDocumentCreateObject(
    const CreativeDocumentCreateRequest& request,
    CreativeObjectId objectId) {
  const CreativeObjectDescriptor& descriptor = describeObject(request.kind);
  const CreativeMovingPlatformSettings movingPlatform =
      request.kind == CreativeObjectKind::MovingPlatform &&
              request.hasMovingPlatformSettingsOverride
          ? request.movingPlatform
          : CreativeMovingPlatformSettings{};
  const CreativeDoorSettings door =
      request.kind == CreativeObjectKind::Door &&
              request.hasDoorSettingsOverride
          ? request.door
          : CreativeDoorSettings{};
  const CreativeWindowSettings window =
      request.kind == CreativeObjectKind::Window &&
              request.hasWindowSettingsOverride
          ? request.window
          : CreativeWindowSettings{};
  const CreativePlayerSpawnSettings playerSpawn =
      request.kind == CreativeObjectKind::SpawnPoint &&
              request.hasPlayerSpawnSettingsOverride
          ? request.playerSpawn
          : CreativePlayerSpawnSettings{};

  CreativeObject object;
  object.id = objectId;
  object.kind = request.kind;
  object.name = request.name.empty() ? descriptorDefaultName(descriptor)
                                     : request.name;
  object.assetId = request.assetId;
  object.assetContentHash = request.assetContentHash;
  object.assetMaterialVariant = request.assetMaterialVariant;
  object.transform = request.hasTransformOverride
                         ? request.transform
                         : descriptor.defaults.transform;
  object.bounds = resolvedCreateBounds(descriptor, request);
  object.layerId = request.hasLayerOverride ? request.layerId
                                            : descriptor.defaults.layerId;
  object.visible = request.hasVisibleOverride ? request.visible
                                              : descriptor.defaults.visible;
  object.locked = request.hasLockedOverride ? request.locked
                                            : descriptor.defaults.locked;
  object.tags = request.tags;
  object.parentId = request.parentId;
  object.attachmentSocket = request.attachmentSocket;
  object.pathPoints = request.pathPoints;
  object.movingPlatform = movingPlatform;
  object.door = door;
  object.window = window;
  object.playerSpawn = playerSpawn;
  return object;
}

std::string_view toString(CreativeDocumentCreateStatus status) noexcept {
  switch (status) {
    case CreativeDocumentCreateStatus::Unknown:
      return "Unknown";
    case CreativeDocumentCreateStatus::InvalidDocument:
      return "InvalidDocument";
    case CreativeDocumentCreateStatus::InvalidKind:
      return "InvalidKind";
    case CreativeDocumentCreateStatus::Created:
      return "Created";
    case CreativeDocumentCreateStatus::Rejected:
      return "Rejected";
  }
  return "Unknown";
}

std::string_view toString(CreativeDocumentRemoveStatus status) noexcept {
  switch (status) {
    case CreativeDocumentRemoveStatus::Unknown:
      return "Unknown";
    case CreativeDocumentRemoveStatus::InvalidDocument:
      return "InvalidDocument";
    case CreativeDocumentRemoveStatus::InvalidObjectId:
      return "InvalidObjectId";
    case CreativeDocumentRemoveStatus::MissingObject:
      return "MissingObject";
    case CreativeDocumentRemoveStatus::LockedObject:
      return "LockedObject";
    case CreativeDocumentRemoveStatus::ParentHasChildren:
      return "ParentHasChildren";
    case CreativeDocumentRemoveStatus::Removed:
      return "Removed";
  }
  return "Unknown";
}

CreativeDocumentCreateReceipt CreativeDocument::createObject(
    const CreativeDocumentCreateRequest& request) {
  CreativeDocumentCreateReceipt receipt;
  receipt.requested = true;
  receipt.objectKind = request.kind;
  receipt.revisionBefore = revision_;
  receipt.revisionAfter = revision_;

  if (!valid_) {
    setCreateStatus(receipt,
                    CreativeDocumentCreateStatus::InvalidDocument,
                    "invalid_document");
    return receipt;
  }

  const CreativeObjectDescriptor& descriptor = describeObject(request.kind);
  if (request.kind == CreativeObjectKind::Unknown ||
      descriptor.kind != request.kind ||
      descriptor.kind == CreativeObjectKind::Unknown) {
    receipt.objectKind = CreativeObjectKind::Unknown;
    setCreateStatus(receipt,
                    CreativeDocumentCreateStatus::InvalidKind,
                    "invalid_kind");
    return receipt;
  }

  if (!request.assetId.empty() && !validStaticMeshAssetId(request.assetId)) {
    setCreateStatus(receipt, CreativeDocumentCreateStatus::Rejected,
                    "invalid_asset_id");
    return receipt;
  }
  if ((request.assetId.empty() &&
       (request.assetContentHash != 0U ||
        !request.assetMaterialVariant.empty())) ||
      !validCreativeAssetMaterialVariantName(
          request.assetMaterialVariant)) {
    setCreateStatus(receipt, CreativeDocumentCreateStatus::Rejected,
                    "invalid_asset_identity");
    return receipt;
  }

  if (request.parentId.has_value()) {
    if (!descriptor.canHaveParent) {
      setCreateStatus(receipt,
                      CreativeDocumentCreateStatus::Rejected,
                      "parent_unsupported");
      return receipt;
    }
    if (*request.parentId == kInvalidObjectId ||
        !containsObject(*request.parentId)) {
      setCreateStatus(receipt,
                      CreativeDocumentCreateStatus::Rejected,
                      "missing_parent");
      return receipt;
    }
    const CreativeObject* parentObject = findObject(*request.parentId);
    const CreativeObjectDescriptor& parentDescriptor =
        describeObject(parentObject->kind);
    if (!parentDescriptor.canOwnChildren) {
      setCreateStatus(receipt,
                      CreativeDocumentCreateStatus::Rejected,
                      "parent_owner_unsupported");
      return receipt;
    }
    const CreativeObjectHierarchyState parentState =
        resolveCreativeObjectHierarchyState(*this, *request.parentId);
    if (!parentState.resolved || parentState.effectivelyLocked) {
      setCreateStatus(receipt, CreativeDocumentCreateStatus::Rejected,
                      "parent_locked");
      return receipt;
    }
    if (parentState.depth >= kCreativeHierarchyDepthCapacity) {
      setCreateStatus(receipt, CreativeDocumentCreateStatus::Rejected,
                      "parent_depth_exceeded");
      return receipt;
    }
    if (!request.attachmentSocket.empty() &&
        !validCreativeAttachmentSocketName(request.attachmentSocket)) {
      setCreateStatus(receipt, CreativeDocumentCreateStatus::Rejected,
                      "attachment_socket_invalid");
      return receipt;
    }
    if (!request.attachmentSocket.empty() &&
        attachmentSocketOccupied(objects_, *request.parentId,
                                 request.attachmentSocket)) {
      setCreateStatus(receipt, CreativeDocumentCreateStatus::Rejected,
                      "attachment_socket_occupied");
      return receipt;
    }
  } else if (!request.attachmentSocket.empty()) {
    setCreateStatus(receipt, CreativeDocumentCreateStatus::Rejected,
                    "attachment_socket_without_parent");
    return receipt;
  }

  if (request.hasTransformOverride && !descriptor.hasTransform) {
    setCreateStatus(receipt,
                    CreativeDocumentCreateStatus::Rejected,
                    "transform_override_unsupported");
    return receipt;
  }

  if (request.hasBoundsOverride && !descriptor.hasBounds) {
    setCreateStatus(receipt,
                    CreativeDocumentCreateStatus::Rejected,
                    "bounds_override_unsupported");
    return receipt;
  }

  const std::string_view pathValidation =
      validateCreatePathPayload(descriptor, request);
  if (!pathValidation.empty()) {
    setCreateStatus(receipt,
                    CreativeDocumentCreateStatus::Rejected,
                    pathValidation);
    return receipt;
  }
  if (request.hasMovingPlatformSettingsOverride &&
      request.kind != CreativeObjectKind::MovingPlatform) {
    setCreateStatus(receipt, CreativeDocumentCreateStatus::Rejected,
                    "moving_platform_settings_unsupported");
    return receipt;
  }
  if (request.hasDoorSettingsOverride &&
      request.kind != CreativeObjectKind::Door) {
    setCreateStatus(receipt, CreativeDocumentCreateStatus::Rejected,
                    "door_settings_unsupported");
    return receipt;
  }
  if (request.hasWindowSettingsOverride &&
      request.kind != CreativeObjectKind::Window) {
    setCreateStatus(receipt, CreativeDocumentCreateStatus::Rejected,
                    "window_settings_unsupported");
    return receipt;
  }
  if (request.hasPlayerSpawnSettingsOverride &&
      request.kind != CreativeObjectKind::SpawnPoint) {
    setCreateStatus(receipt, CreativeDocumentCreateStatus::Rejected,
                    "player_spawn_settings_unsupported");
    return receipt;
  }
  const CreativeMovingPlatformSettings movingPlatform =
      request.kind == CreativeObjectKind::MovingPlatform &&
              request.hasMovingPlatformSettingsOverride
          ? request.movingPlatform
          : CreativeMovingPlatformSettings{};
  if (request.kind == CreativeObjectKind::MovingPlatform &&
      !isValidCreativeMovingPlatformSettings(movingPlatform)) {
    setCreateStatus(receipt, CreativeDocumentCreateStatus::Rejected,
                    "moving_platform_settings_invalid");
    return receipt;
  }
  const CreativeDoorSettings door =
      request.kind == CreativeObjectKind::Door &&
              request.hasDoorSettingsOverride
          ? request.door
          : CreativeDoorSettings{};
  if (request.kind == CreativeObjectKind::Door &&
      !isValidCreativeDoorSettings(door)) {
    setCreateStatus(receipt, CreativeDocumentCreateStatus::Rejected,
                    "door_settings_invalid");
    return receipt;
  }
  const CreativeWindowSettings window =
      request.kind == CreativeObjectKind::Window &&
              request.hasWindowSettingsOverride
          ? request.window
          : CreativeWindowSettings{};
  if (request.kind == CreativeObjectKind::Window &&
      !isValidCreativeWindowSettings(window)) {
    setCreateStatus(receipt, CreativeDocumentCreateStatus::Rejected,
                    "window_settings_invalid");
    return receipt;
  }
  const CreativePlayerSpawnSettings playerSpawn =
      request.kind == CreativeObjectKind::SpawnPoint &&
              request.hasPlayerSpawnSettingsOverride
          ? request.playerSpawn
          : CreativePlayerSpawnSettings{};
  if (request.kind == CreativeObjectKind::SpawnPoint &&
      !isValidCreativePlayerSpawnSettings(playerSpawn)) {
    setCreateStatus(receipt, CreativeDocumentCreateStatus::Rejected,
                    "player_spawn_settings_invalid");
    return receipt;
  }

  CreativeObject object = resolveCreativeDocumentCreateObject(request);

  const CreativeObjectId id = appendObject(std::move(object));
  if (id == kInvalidObjectId) {
    setCreateStatus(receipt,
                    CreativeDocumentCreateStatus::Rejected,
                    "create_rejected");
    return receipt;
  }

  receipt.accepted = true;
  receipt.changed = revision_ != receipt.revisionBefore;
  receipt.objectCreated = true;
  receipt.status = CreativeDocumentCreateStatus::Created;
  receipt.objectId = id;
  receipt.objectKind = request.kind;
  receipt.objectName = objects_.back().name;
  receipt.revisionAfter = revision_;
  receipt.creationDirtyFlags = descriptor.creationDirtyFlags;
  receipt.message = "object_created";
  receipt.reasonCode = "object_created";
  return receipt;
}

CreativeDocumentRemoveReceipt CreativeDocument::removeDocumentObject(
    const CreativeDocumentRemoveRequest& request) {
  CreativeDocumentRemoveReceipt receipt;
  receipt.requested = true;
  receipt.objectId = request.objectId;
  receipt.revisionBefore = revision_;
  receipt.revisionAfter = revision_;

  if (!valid_) {
    setRemoveStatus(receipt,
                    CreativeDocumentRemoveStatus::InvalidDocument,
                    "invalid_document");
    return receipt;
  }

  if (request.objectId == kInvalidObjectId) {
    setRemoveStatus(receipt,
                    CreativeDocumentRemoveStatus::InvalidObjectId,
                    "invalid_object_id");
    return receipt;
  }

  const auto found = objectIndex_.find(request.objectId);
  if (found == objectIndex_.end()) {
    setRemoveStatus(receipt,
                    CreativeDocumentRemoveStatus::MissingObject,
                    "missing_object");
    return receipt;
  }

  const std::size_t index = found->second;
  const CreativeObject& object = objects_[index];
  receipt.objectKind = object.kind;
  receipt.objectName = object.name;

  const CreativeObjectHierarchyState hierarchyState =
      resolveCreativeObjectHierarchyState(*this, request.objectId);
  if (!hierarchyState.resolved || hierarchyState.effectivelyLocked) {
    setRemoveStatus(receipt,
                    CreativeDocumentRemoveStatus::LockedObject,
                    hierarchyState.lockedByObjectId == object.id
                        ? "object is locked"
                        : "object is locked by an ancestor");
    return receipt;
  }

  if (objectHasChildren(objects_, request.objectId)) {
    setRemoveStatus(receipt,
                    CreativeDocumentRemoveStatus::ParentHasChildren,
                    "parent_has_children");
    return receipt;
  }

  receipt.removalDirtyFlags = dirtyFlagsForRemoval(object.kind);
  receipt.removedLogicLinkCount = eraseLogicLinksForObject(request.objectId);
  receipt.detachedPatternRecipeCount =
      erasePatternRecipesForObject(request.objectId);
  if (receipt.removedLogicLinkCount > 0U) {
    receipt.removalDirtyFlags |=
        static_cast<CreativeObjectDirtyFlags>(CreativeObjectDirtyFlag::Logic);
  }

  objectIndex_.erase(found);
  objects_.erase(objects_.begin() + static_cast<std::ptrdiff_t>(index));
  for (std::size_t nextIndex = index; nextIndex < objects_.size();
       ++nextIndex) {
    objectIndex_[objects_[nextIndex].id] = nextIndex;
  }

  markContentChanged();
  markDirty(receipt.removalDirtyFlags);
  receipt.accepted = true;
  receipt.changed = revision_ != receipt.revisionBefore;
  receipt.objectRemoved = true;
  receipt.status = CreativeDocumentRemoveStatus::Removed;
  receipt.revisionAfter = revision_;
  receipt.message = "object_removed";
  receipt.reasonCode = "object_removed";
  return receipt;
}

CreativeDocumentRemoveReceipt CreativeDocument::removeDocumentObject(
    CreativeObjectId id) {
  CreativeDocumentRemoveRequest request;
  request.objectId = id;
  return removeDocumentObject(request);
}

CreativeObjectId CreativeDocument::appendObject(CreativeObject object) {
  if (!valid_) {
    return kInvalidObjectId;
  }

  const CreativeObjectDirtyFlags creationDirtyFlags =
      dirtyFlagsForCreation(object.kind);
  const CreativeObjectId id = nextObjectId_++;
  object.id = id;
  objectIndex_[id] = objects_.size();
  objects_.push_back(std::move(object));
  markContentChanged();
  markDirty(creationDirtyFlags);
  return id;
}

std::size_t CreativeDocument::erasePatternRecipesForObject(
    CreativeObjectId objectId) noexcept {
  const std::size_t before = patternRecipeStore_.recipes.size();
  std::erase_if(
      patternRecipeStore_.recipes,
      [objectId](const CreativePatternRecipe& recipe) {
        return std::find(recipe.sourceObjectIds.begin(),
                         recipe.sourceObjectIds.end(), objectId) !=
                   recipe.sourceObjectIds.end() ||
               std::find(recipe.generatedObjectIds.begin(),
                         recipe.generatedObjectIds.end(), objectId) !=
                   recipe.generatedObjectIds.end();
      });
  return before - patternRecipeStore_.recipes.size();
}

}  // namespace iggy3d::creative
