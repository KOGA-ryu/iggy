#include "app/iggy3d/creative/Document.hpp"

#include <utility>

namespace iggy3d::creative {

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

  const CreativeObjectId id = nextObjectId_++;
  CreativeObject object = makeRoomObject(id,
                                         std::move(name),
                                         transform,
                                         bounds,
                                         layerId,
                                         visible,
                                         locked,
                                         std::move(tags),
                                         parentId);
  objectIndex_[id] = objects_.size();
  objects_.push_back(std::move(object));
  markContentChanged();
  return id;
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

void CreativeDocument::markObjectMutationChanged() noexcept {
  markContentChanged();
}

}  // namespace iggy3d::creative
