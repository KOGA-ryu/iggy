#include "app/iggy3d/creative/document/Document.hpp"

#include <cmath>
#include <utility>

namespace iggy3d::creative {
namespace {

constexpr CreativeObjectDirtyFlags dirtyFlagValue(
    CreativeObjectDirtyFlag flag) noexcept {
  return static_cast<CreativeObjectDirtyFlags>(flag);
}

constexpr CreativeObjectDirtyFlags documentIdentityDirtyFlags() noexcept {
  return dirtyFlagValue(CreativeObjectDirtyFlag::Identity) |
         dirtyFlagValue(CreativeObjectDirtyFlag::Preview) |
         dirtyFlagValue(CreativeObjectDirtyFlag::Serialization);
}

constexpr CreativeObjectDirtyFlags documentSettingsDirtyFlags() noexcept {
  return dirtyFlagValue(CreativeObjectDirtyFlag::Preview) |
         dirtyFlagValue(CreativeObjectDirtyFlag::Serialization);
}

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

void setRestoreStatus(CreativeDocumentRestoreReceipt& receipt,
                      CreativeDocumentRestoreStatus status,
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

bool sameVec3(CreativeVec3 lhs, CreativeVec3 rhs) noexcept {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

bool sameGridSize(CreativeGridSize3 lhs, CreativeGridSize3 rhs) noexcept {
  return lhs.width == rhs.width && lhs.height == rhs.height &&
         lhs.depth == rhs.depth;
}

bool sameGridSettings(CreativeGridSettings lhs,
                      CreativeGridSettings rhs) noexcept {
  return sameVec3(lhs.origin, rhs.origin) &&
         lhs.cellSizeMeters == rhs.cellSizeMeters &&
         sameGridSize(lhs.size, rhs.size);
}

bool sameSnapSettings(CreativeDocumentSnapSettings lhs,
                      CreativeDocumentSnapSettings rhs) noexcept {
  return lhs.mode == rhs.mode && lhs.axes == rhs.axes &&
         lhs.stepX == rhs.stepX && lhs.stepY == rhs.stepY &&
         lhs.stepZ == rhs.stepZ && lhs.originX == rhs.originX &&
         lhs.originY == rhs.originY && lhs.originZ == rhs.originZ;
}

bool sameBounds(CreativeBounds lhs, CreativeBounds rhs) noexcept {
  return sameVec3(lhs.min, rhs.min) && sameVec3(lhs.max, rhs.max);
}

bool isFiniteVec3(CreativeVec3 value) noexcept {
  return std::isfinite(value.x) && std::isfinite(value.y) &&
         std::isfinite(value.z);
}

bool isPathDescriptor(const CreativeObjectDescriptor& descriptor) noexcept {
  return descriptor.shapeKind == CreativeObjectShapeKind::Path;
}

bool pathPointsAreValid(std::span<const CreativePathPoint> pathPoints) noexcept {
  if (pathPoints.size() < 2U) {
    return false;
  }

  for (const CreativePathPoint& point : pathPoints) {
    if (!isFiniteVec3(point.position)) {
      return false;
    }
  }

  return true;
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
    if (!pathPointsAreValid(request.pathPoints)) {
      return "invalid_path_points";
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
    return pathPointsAreValid(object.pathPoints) ? std::string_view{}
                                                : "invalid_path_points";
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
    return {};
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

bool objectHasChildren(std::span<const CreativeObject> objects,
                       CreativeObjectId parentId) noexcept {
  for (const CreativeObject& object : objects) {
    if (object.parentId.has_value() && *object.parentId == parentId) {
      return true;
    }
  }
  return false;
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

bool isValidUnits(CreativeUnits units) noexcept {
  return units == CreativeUnits::Meters;
}

bool isValidGridSettings(CreativeGridSettings settings) noexcept {
  return isFiniteVec3(settings.origin) &&
         std::isfinite(settings.cellSizeMeters) &&
         settings.cellSizeMeters > 0.0 && settings.size.width >= 0 &&
         settings.size.height >= 0 && settings.size.depth >= 0;
}

bool isValidWorldBounds(CreativeBounds bounds) noexcept {
  return isFiniteVec3(bounds.min) && isFiniteVec3(bounds.max);
}

bool isValidRestoreObject(const CreativeObject& object) noexcept {
  const CreativeObjectDescriptor& descriptor = describeObject(object.kind);
  return object.id != kInvalidObjectId &&
         object.kind != CreativeObjectKind::Unknown &&
         descriptor.kind == object.kind &&
         descriptor.kind != CreativeObjectKind::Unknown &&
         isFiniteVec3(object.transform.position) &&
         isFiniteVec3(object.transform.rotation) &&
         isFiniteVec3(object.transform.scale) &&
         isValidWorldBounds(object.bounds);
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

std::string_view toString(CreativeDocumentRestoreStatus status) noexcept {
  switch (status) {
    case CreativeDocumentRestoreStatus::Unknown:
      return "Unknown";
    case CreativeDocumentRestoreStatus::InvalidDocument:
      return "InvalidDocument";
    case CreativeDocumentRestoreStatus::InvalidDocumentId:
      return "InvalidDocumentId";
    case CreativeDocumentRestoreStatus::InvalidSettings:
      return "InvalidSettings";
    case CreativeDocumentRestoreStatus::InvalidObject:
      return "InvalidObject";
    case CreativeDocumentRestoreStatus::DuplicateObjectId:
      return "DuplicateObjectId";
    case CreativeDocumentRestoreStatus::InvalidNextObjectId:
      return "InvalidNextObjectId";
    case CreativeDocumentRestoreStatus::Restored:
      return "Restored";
  }
  return "Unknown";
}

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

  if (parentGraphContainsCycle(objects, objectIndex)) {
    return "parent_cycle";
  }

  return {};
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

CreativeDocumentId CreativeDocument::id() const noexcept {
  return id_;
}

std::string_view CreativeDocument::name() const noexcept {
  return name_;
}

std::uint64_t CreativeDocument::revision() const noexcept {
  return revision_;
}

CreativeObjectDirtyFlags CreativeDocument::dirtyFlags() const noexcept {
  return dirtyFlags_;
}

CreativeObjectDirtyFlags CreativeDocument::drainDirtyFlags() noexcept {
  const CreativeObjectDirtyFlags drained = dirtyFlags_;
  dirtyFlags_ = 0;
  return drained;
}

CreativeUnits CreativeDocument::units() const noexcept {
  return units_;
}

CreativeGridSettings CreativeDocument::gridSettings() const noexcept {
  return gridSettings_;
}

CreativeDocumentSnapSettings CreativeDocument::documentSnapSettings()
    const noexcept {
  return snapSettings_;
}

CreativeBounds CreativeDocument::worldBounds() const noexcept {
  return worldBounds_;
}

CreativeObjectId CreativeDocument::nextObjectId() const noexcept {
  return nextObjectId_;
}

bool CreativeDocument::assignId(CreativeDocumentId id) noexcept {
  if (id == kInvalidDocumentId || id_ == id ||
      id_ != kInvalidDocumentId) {
    return false;
  }

  id_ = id;
  return true;
}

bool CreativeDocument::setUnits(CreativeUnits units) {
  if (!valid_ || !isValidUnits(units) || units_ == units) {
    return false;
  }

  units_ = units;
  markObjectMutationChanged(documentSettingsDirtyFlags());
  return true;
}

bool CreativeDocument::setGridSettings(CreativeGridSettings settings) {
  if (!valid_ || !isValidGridSettings(settings) ||
      sameGridSettings(gridSettings_, settings)) {
    return false;
  }

  gridSettings_ = settings;
  markObjectMutationChanged(documentSettingsDirtyFlags());
  return true;
}

bool CreativeDocument::setDocumentSnapSettings(
    CreativeDocumentSnapSettings settings) {
  if (!valid_ || !isValidCreativeDocumentSnapSettings(settings) ||
      sameSnapSettings(snapSettings_, settings)) {
    return false;
  }

  snapSettings_ = settings;
  markObjectMutationChanged(documentSettingsDirtyFlags());
  return true;
}

bool CreativeDocument::setWorldBounds(CreativeBounds bounds) {
  if (!valid_ || !isValidWorldBounds(bounds) ||
      sameBounds(worldBounds_, bounds)) {
    return false;
  }

  worldBounds_ = bounds;
  markObjectMutationChanged(documentSettingsDirtyFlags());
  return true;
}

bool CreativeDocument::rename(std::string nextName) {
  if (!valid_ || name_ == nextName) {
    return false;
  }

  name_ = std::move(nextName);
  markObjectMutationChanged(documentIdentityDirtyFlags());
  return true;
}

void CreativeDocument::reset() {
  valid_ = true;
  id_ = kInvalidDocumentId;
  name_.clear();
  revision_ = 0;
  dirtyFlags_ = 0;

  objects_.clear();
  objectIndex_.clear();
  nextObjectId_ = 1;
  units_ = CreativeUnits::Meters;
  gridSettings_ = {};
  snapSettings_ = makeDefaultCreativeDocumentSnapSettings();
  worldBounds_ = {};

  // Future slice reset duties:
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
    const CreativeObject* parentObject = findObject(*request.parentId);
    const CreativeObjectDescriptor& parentDescriptor =
        describeObject(parentObject->kind);
    if (!parentDescriptor.canOwnChildren) {
      setCreateStatus(receipt,
                      CreativeDocumentCreateStatus::Rejected,
                      "parent_owner_unsupported");
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

void CreativeDocument::markContentChanged() noexcept {
  if (valid_) {
    ++revision_;
  }
}

void CreativeDocument::markDirty(CreativeObjectDirtyFlags dirtyFlags) noexcept {
  dirtyFlags_ |= dirtyFlags;
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

void CreativeDocument::markObjectMutationChanged(
    CreativeObjectDirtyFlags dirtyFlags) noexcept {
  markContentChanged();
  markDirty(dirtyFlags);
}

CreativeDocumentRestoreReceipt CreativeDocument::restoreForLoad(
    const CreativeDocumentRestoreRequest& request) {
  CreativeDocumentRestoreReceipt receipt;
  receipt.requested = true;
  receipt.documentId = request.documentId;
  receipt.objectCount = request.objects.size();
  receipt.nextObjectId = request.nextObjectId;

  if (!valid_) {
    setRestoreStatus(receipt,
                     CreativeDocumentRestoreStatus::InvalidDocument,
                     "invalid_document");
    return receipt;
  }

  if (request.documentId == kInvalidDocumentId) {
    setRestoreStatus(receipt,
                     CreativeDocumentRestoreStatus::InvalidDocumentId,
                     "invalid_document_id");
    return receipt;
  }

  if (!isValidUnits(request.units)) {
    setRestoreStatus(receipt,
                     CreativeDocumentRestoreStatus::InvalidSettings,
                     "invalid_units");
    return receipt;
  }

  if (!isValidGridSettings(request.gridSettings)) {
    setRestoreStatus(receipt,
                     CreativeDocumentRestoreStatus::InvalidSettings,
                     "invalid_grid_settings");
    return receipt;
  }

  if (!isValidCreativeDocumentSnapSettings(request.snapSettings)) {
    setRestoreStatus(receipt,
                     CreativeDocumentRestoreStatus::InvalidSettings,
                     "invalid_document_snap_settings");
    return receipt;
  }

  if (!isValidWorldBounds(request.worldBounds)) {
    setRestoreStatus(receipt,
                     CreativeDocumentRestoreStatus::InvalidSettings,
                     "invalid_world_bounds");
    return receipt;
  }

  std::unordered_map<CreativeObjectId, std::size_t> restoredIndex;
  restoredIndex.reserve(request.objects.size());
  CreativeObjectId maxObjectId = kInvalidObjectId;
  for (std::size_t index = 0; index < request.objects.size(); ++index) {
    const CreativeObject& object = request.objects[index];
    if (!isValidRestoreObject(object)) {
      setRestoreStatus(receipt,
                       CreativeDocumentRestoreStatus::InvalidObject,
                       "invalid_object");
      return receipt;
    }
    const CreativeObjectDescriptor& descriptor = describeObject(object.kind);
    const std::string_view pathValidation =
        validateRestoredPathPayload(descriptor, object);
    if (!pathValidation.empty()) {
      setRestoreStatus(receipt,
                       CreativeDocumentRestoreStatus::InvalidObject,
                       pathValidation);
      return receipt;
    }
    if (restoredIndex.find(object.id) != restoredIndex.end()) {
      setRestoreStatus(receipt,
                       CreativeDocumentRestoreStatus::DuplicateObjectId,
                       "duplicate_object_id");
      return receipt;
    }
    restoredIndex.emplace(object.id, index);
    if (object.id > maxObjectId) {
      maxObjectId = object.id;
    }
  }

  const std::string_view parentGraphValidation =
      validateCreativeObjectParentGraph(request.objects);
  if (!parentGraphValidation.empty()) {
    setRestoreStatus(receipt,
                     CreativeDocumentRestoreStatus::InvalidObject,
                     parentGraphValidation);
    return receipt;
  }

  if (request.nextObjectId == kInvalidObjectId ||
      request.nextObjectId <= maxObjectId) {
    setRestoreStatus(receipt,
                     CreativeDocumentRestoreStatus::InvalidNextObjectId,
                     "invalid_next_object_id");
    return receipt;
  }

  valid_ = true;
  id_ = request.documentId;
  name_ = request.name;
  units_ = request.units;
  gridSettings_ = request.gridSettings;
  snapSettings_ = request.snapSettings;
  worldBounds_ = request.worldBounds;
  objects_ = request.objects;
  objectIndex_ = std::move(restoredIndex);
  nextObjectId_ = request.nextObjectId;
  revision_ = 0;
  dirtyFlags_ = 0;

  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = CreativeDocumentRestoreStatus::Restored;
  receipt.message = "document_restored";
  receipt.reasonCode = "document_restored";
  return receipt;
}

}  // namespace iggy3d::creative
