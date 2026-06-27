#include "runtime/physics/PhysicsShapeStore.hpp"

#include <array>
#include <limits>

namespace iggy3d {
namespace {

constexpr std::size_t kMissingIndex = std::numeric_limits<std::size_t>::max();

template <std::size_t Count, typename Enum>
std::string_view enumName(Enum value,
                          const std::array<std::string_view, Count>& names,
                          std::string_view fallback) {
  const auto index = static_cast<std::size_t>(value);
  // branch-gate: BG-1090
  if (index >= names.size()) {
    return fallback;
  }
  return names[index];
}

bool validShapeKind(PhysicsShapeKind kind) {
  const auto index = static_cast<std::size_t>(kind);
  return index < 5U;
}

bool positiveFiniteExtents(Vec3 halfExtentsMeters) {
  return isFinite(halfExtentsMeters) && halfExtentsMeters.x > 0.0F &&
         halfExtentsMeters.y > 0.0F && halfExtentsMeters.z > 0.0F;
}

PhysicsShapeValidationResult validationResult(PhysicsShapeStatus status,
                                              bool ok) {
  PhysicsShapeValidationResult result;
  result.ok = ok;
  result.status = status;
  result.reasonCode = physicsShapeStatusName(status);
  return result;
}

PhysicsShapeStoreResult storeResult(PhysicsShapeStatus status, bool ok) {
  PhysicsShapeStoreResult result;
  result.ok = ok;
  result.status = status;
  result.reasonCode = physicsShapeStatusName(status);
  return result;
}

}  // namespace

std::string_view physicsShapeStatusName(PhysicsShapeStatus status) {
  static constexpr std::array<std::string_view, 9> kNames{
      "physics_shape_valid",
      "physics_shape_missing_descriptor",
      "physics_shape_invalid_kind",
      "physics_shape_invalid_center_offset",
      "physics_shape_invalid_half_extents",
      "physics_shape_added",
      "physics_shape_not_found",
      "physics_shape_removed",
      "physics_shape_store_reset",
  };
  return enumName(status, kNames, "physics_shape_invalid_kind");
}

PhysicsShapeValidationResult validatePhysicsShapeDescriptor(
    const PhysicsShapeDescriptor* descriptor) {
  // branch-gate: BG-1090
  if (descriptor == nullptr) {
    return validationResult(PhysicsShapeStatus::MissingDescriptor, false);
  }
  // branch-gate: BG-1090
  if (!validShapeKind(descriptor->kind)) {
    return validationResult(PhysicsShapeStatus::InvalidShapeKind, false);
  }
  // branch-gate: BG-1090
  if (!isFinite(descriptor->localCenterOffsetMeters)) {
    return validationResult(PhysicsShapeStatus::InvalidCenterOffset, false);
  }
  // branch-gate: BG-1090
  if (!positiveFiniteExtents(descriptor->halfExtentsMeters)) {
    return validationResult(PhysicsShapeStatus::InvalidHalfExtents, false);
  }
  return validationResult(PhysicsShapeStatus::Valid, true);
}

std::size_t PhysicsShapeStore::size() const {
  return ids_.size();
}

bool PhysicsShapeStore::empty() const {
  return ids_.empty();
}

const std::vector<PhysicsShapeId>& PhysicsShapeStore::ids() const {
  return ids_;
}

const std::vector<PhysicsShapeKind>& PhysicsShapeStore::kinds() const {
  return kinds_;
}

const std::vector<Vec3>& PhysicsShapeStore::localCenterOffsets() const {
  return localCenterOffsets_;
}

const std::vector<Vec3>& PhysicsShapeStore::halfExtents() const {
  return halfExtents_;
}

const std::vector<bool>& PhysicsShapeStore::sensors() const {
  return sensors_;
}

PhysicsShapeStoreResult PhysicsShapeStore::add(
    const PhysicsShapeDescriptor& descriptor) {
  const PhysicsShapeValidationResult validation =
      validatePhysicsShapeDescriptor(&descriptor);
  // branch-gate: BG-1090
  if (!validation.ok) {
    PhysicsShapeStoreResult result;
    result.status = validation.status;
    result.reasonCode = validation.reasonCode;
    return result;
  }

  const PhysicsShapeId id{nextId_++};
  ids_.push_back(id);
  kinds_.push_back(descriptor.kind);
  localCenterOffsets_.push_back(descriptor.localCenterOffsetMeters);
  halfExtents_.push_back(descriptor.halfExtentsMeters);
  sensors_.push_back(descriptor.sensor);

  PhysicsShapeStoreResult result =
      storeResult(PhysicsShapeStatus::ShapeAdded, true);
  result.id = id;
  result.index = ids_.size() - 1U;
  result.shape = shapeAt(result.index);
  return result;
}

PhysicsShapeStoreResult PhysicsShapeStore::read(PhysicsShapeId id) const {
  const std::size_t index = findIndex(id);
  // branch-gate: BG-1090
  if (index == kMissingIndex) {
    PhysicsShapeStoreResult result =
        storeResult(PhysicsShapeStatus::ShapeNotFound, false);
    result.id = id;
    return result;
  }

  PhysicsShapeStoreResult result =
      storeResult(PhysicsShapeStatus::Valid, true);
  result.id = id;
  result.index = index;
  result.shape = shapeAt(index);
  return result;
}

PhysicsShapeStoreResult PhysicsShapeStore::remove(PhysicsShapeId id) {
  const std::size_t index = findIndex(id);
  // branch-gate: BG-1090
  if (index == kMissingIndex) {
    PhysicsShapeStoreResult result =
        storeResult(PhysicsShapeStatus::ShapeNotFound, false);
    result.id = id;
    return result;
  }

  const PhysicsShapeView removed = shapeAt(index);
  ids_.erase(ids_.begin() + static_cast<std::ptrdiff_t>(index));
  kinds_.erase(kinds_.begin() + static_cast<std::ptrdiff_t>(index));
  localCenterOffsets_.erase(localCenterOffsets_.begin() +
                            static_cast<std::ptrdiff_t>(index));
  halfExtents_.erase(halfExtents_.begin() +
                     static_cast<std::ptrdiff_t>(index));
  sensors_.erase(sensors_.begin() + static_cast<std::ptrdiff_t>(index));

  PhysicsShapeStoreResult result =
      storeResult(PhysicsShapeStatus::ShapeRemoved, true);
  result.id = id;
  result.index = index;
  result.shape = removed;
  return result;
}

PhysicsShapeStoreResult PhysicsShapeStore::reset() {
  ids_.clear();
  kinds_.clear();
  localCenterOffsets_.clear();
  halfExtents_.clear();
  sensors_.clear();
  nextId_ = 1U;
  return storeResult(PhysicsShapeStatus::StoreReset, true);
}

PhysicsShapeView PhysicsShapeStore::shapeAt(std::size_t index) const {
  PhysicsShapeView shape;
  shape.id = ids_[index];
  shape.kind = kinds_[index];
  shape.localCenterOffsetMeters = localCenterOffsets_[index];
  shape.halfExtentsMeters = halfExtents_[index];
  shape.sensor = sensors_[index];
  return shape;
}

std::size_t PhysicsShapeStore::findIndex(PhysicsShapeId id) const {
  // branch-gate: BG-1090
  if (!isValidPhysicsShapeId(id)) {
    return kMissingIndex;
  }
  for (std::size_t index = 0U; index < ids_.size(); ++index) {
    // branch-gate: BG-1090
    if (ids_[index].value == id.value) {
      return index;
    }
  }
  return kMissingIndex;
}

}  // namespace iggy3d
