#include "app/iggy3d/creative/Document.hpp"

#include <utility>

namespace iggy3d::creative {
namespace {

void setCreateStatus(CreativeDocumentCreateReceipt& receipt,
                     CreativeDocumentCreateStatus status,
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

CreativeDocument CreativeDocument::create(std::string name) {
  CreativeDocument document;
  document.valid_ = true;
  document.name_ = std::move(name);
  document.revision_ = 0;
  return document;
}

bool CreativeDocument::isValid() const noexcept {
  return valid_;
}

std::string_view CreativeDocument::name() const noexcept {
  return name_;
}

std::uint64_t CreativeDocument::revision() const noexcept {
  return revision_;
}

bool CreativeDocument::rename(std::string nextName) {
  if (!valid_ || name_ == nextName) {
    return false;
  }

  name_ = std::move(nextName);
  markObjectMutationChanged();
  return true;
}

void CreativeDocument::reset() {
  valid_ = true;
  name_.clear();
  revision_ = 0;

  objects_.clear();
  objectIndex_.clear();
  nextObjectId_ = 1;

  // Future slice reset duties:
  // id_ = {};
  // units_ = CreativeUnits::Meters;
  // gridSettings_ = {};
  // snapSettings_ = {};
  // worldBounds_ = {};
  // layers_.clear();
  // nextLayerId_ = 1;
  // tagRegistry_.clear();
  // notes_.clear();
}

std::uint64_t CreativeDocument::objectCount() const noexcept {
  return objects_.size();
}

bool CreativeDocument::containsObject(CreativeObjectId id) const noexcept {
  return objectIndex_.find(id) != objectIndex_.end();
}

const CreativeObject* CreativeDocument::findObject(
    CreativeObjectId id) const noexcept {
  const auto found = objectIndex_.find(id);
  if (found == objectIndex_.end()) {
    return nullptr;
  }
  return &objects_[found->second];
}

CreativeObject* CreativeDocument::findObject(CreativeObjectId id) noexcept {
  const auto found = objectIndex_.find(id);
  if (found == objectIndex_.end()) {
    return nullptr;
  }
  return &objects_[found->second];
}

std::span<const CreativeObject> CreativeDocument::objects() const noexcept {
  return objects_;
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

  if (request.hasVisibleOverride && !descriptor.canBeHidden) {
    setCreateStatus(receipt,
                    CreativeDocumentCreateStatus::Rejected,
                    "visibility_override_unsupported");
    return receipt;
  }

  if (request.hasLockedOverride && !descriptor.canBeLocked) {
    setCreateStatus(receipt,
                    CreativeDocumentCreateStatus::Rejected,
                    "locked_override_unsupported");
    return receipt;
  }

  if (!request.tags.empty() && !descriptor.canBeTagged) {
    setCreateStatus(receipt,
                    CreativeDocumentCreateStatus::Rejected,
                    "tags_unsupported");
    return receipt;
  }

  CreativeObject object;
  object.kind = request.kind;
  object.name = request.name.empty() ? descriptorDefaultName(descriptor)
                                     : request.name;
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

CreativeObjectId CreativeDocument::createRoom(
    std::string name,
    CreativeTransform transform,
    CreativeBounds bounds,
    CreativeLayerId layerId,
    bool visible,
    bool locked,
    std::vector<std::string> tags,
    std::optional<CreativeObjectId> parentId) {
  if (!valid_) {
    return kInvalidObjectId;
  }

  CreativeObject object = makeRoomObject(kInvalidObjectId,
                                         std::move(name),
                                         transform,
                                         bounds,
                                         layerId,
                                         visible,
                                         locked,
                                         std::move(tags),
                                         parentId);
  return appendObject(std::move(object));
}

bool CreativeDocument::renameObject(CreativeObjectId id, std::string nextName) {
  CreativeObject* object = findObject(id);
  if (object == nullptr || object->name == nextName) {
    return false;
  }

  object->name = std::move(nextName);
  markContentChanged();
  return true;
}

bool CreativeDocument::removeObject(CreativeObjectId id) {
  const auto found = objectIndex_.find(id);
  if (found == objectIndex_.end()) {
    return false;
  }

  const std::size_t index = found->second;
  objectIndex_.erase(found);
  objects_.erase(objects_.begin() + static_cast<std::ptrdiff_t>(index));
  for (std::size_t nextIndex = index; nextIndex < objects_.size();
       ++nextIndex) {
    objectIndex_[objects_[nextIndex].id] = nextIndex;
  }

  markContentChanged();
  return true;
}

void CreativeDocument::markContentChanged() noexcept {
  if (valid_) {
    ++revision_;
  }
}

CreativeObjectId CreativeDocument::appendObject(CreativeObject object) {
  if (!valid_) {
    return kInvalidObjectId;
  }

  const CreativeObjectId id = nextObjectId_++;
  object.id = id;
  objectIndex_[id] = objects_.size();
  objects_.push_back(std::move(object));
  markContentChanged();
  return id;
}

void CreativeDocument::markObjectMutationChanged() noexcept {
  markContentChanged();
}

}  // namespace iggy3d::creative
