#include "app/iggy3d/creative/document/Document.hpp"

#include "app/iggy3d/creative/document/DocumentInternal.hpp"
#include "content/assets/StaticMeshAsset.hpp"

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

}  // namespace

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

  CreativeObject object;
  object.kind = request.kind;
  object.name = request.name.empty() ? descriptorDefaultName(descriptor)
                                     : request.name;
  object.assetId = request.assetId;
  object.transform = request.hasTransformOverride
                         ? request.transform
                         : descriptor.defaults.transform;
  object.bounds = request.hasBoundsOverride ? request.bounds
                                            : descriptor.defaults.bounds;
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

  if (object.locked) {
    setRemoveStatus(receipt,
                    CreativeDocumentRemoveStatus::LockedObject,
                    "object is locked");
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

}  // namespace iggy3d::creative
