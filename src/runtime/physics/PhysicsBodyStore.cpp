#include "runtime/physics/PhysicsBodyStore.hpp"

#include <limits>

namespace iggy3d {
namespace {

constexpr std::size_t kMissingIndex = std::numeric_limits<std::size_t>::max();

PhysicsBodyStoreResult storeResult(PhysicsStatus status, bool ok) {
  PhysicsBodyStoreResult result;
  result.ok = ok;
  result.status = status;
  result.reasonCode = physicsStatusName(status);
  return result;
}

}  // namespace

std::size_t PhysicsBodyStore::size() const {
  return ids_.size();
}

bool PhysicsBodyStore::empty() const {
  return ids_.empty();
}

const std::vector<PhysicsBodyId>& PhysicsBodyStore::ids() const {
  return ids_;
}

const std::vector<PhysicsBodyMotionKind>& PhysicsBodyStore::motions() const {
  return motions_;
}

const std::vector<Vec3>& PhysicsBodyStore::positions() const {
  return positions_;
}

const std::vector<Vec3>& PhysicsBodyStore::velocities() const {
  return velocities_;
}

const std::vector<float>& PhysicsBodyStore::masses() const {
  return masses_;
}

const std::vector<float>& PhysicsBodyStore::inverseMasses() const {
  return inverseMasses_;
}

PhysicsBodyStoreResult PhysicsBodyStore::add(
    const PhysicsBodyDescriptor& descriptor) {
  const PhysicsValidationResult validation =
      validatePhysicsBodyDescriptor(&descriptor);
  // branch-gate: BG-1085
  if (!validation.ok) {
    PhysicsBodyStoreResult result;
    result.status = validation.status;
    result.reasonCode = validation.reasonCode;
    return result;
  }

  const PhysicsBodyId id{nextId_++};
  ids_.push_back(id);
  motions_.push_back(descriptor.motion);
  positions_.push_back(descriptor.positionMeters);
  velocities_.push_back(descriptor.velocityMetersPerSecond);
  masses_.push_back(descriptor.massKilograms);
  inverseMasses_.push_back(
      computePhysicsInverseMass(descriptor.motion, descriptor.massKilograms));

  PhysicsBodyStoreResult result =
      storeResult(PhysicsStatus::BodyAdded, true);
  result.id = id;
  result.index = ids_.size() - 1U;
  result.body = bodyAt(result.index);
  return result;
}

PhysicsBodyStoreResult PhysicsBodyStore::read(PhysicsBodyId id) const {
  const std::size_t index = findIndex(id);
  // branch-gate: BG-1085
  if (index == kMissingIndex) {
    PhysicsBodyStoreResult result =
        storeResult(PhysicsStatus::BodyNotFound, false);
    result.id = id;
    return result;
  }

  PhysicsBodyStoreResult result = storeResult(PhysicsStatus::Valid, true);
  result.id = id;
  result.index = index;
  result.body = bodyAt(index);
  return result;
}

PhysicsBodyStoreResult PhysicsBodyStore::remove(PhysicsBodyId id) {
  const std::size_t index = findIndex(id);
  // branch-gate: BG-1085
  if (index == kMissingIndex) {
    PhysicsBodyStoreResult result =
        storeResult(PhysicsStatus::BodyNotFound, false);
    result.id = id;
    return result;
  }

  const PhysicsBodyView removed = bodyAt(index);
  ids_.erase(ids_.begin() + static_cast<std::ptrdiff_t>(index));
  motions_.erase(motions_.begin() + static_cast<std::ptrdiff_t>(index));
  positions_.erase(positions_.begin() + static_cast<std::ptrdiff_t>(index));
  velocities_.erase(velocities_.begin() + static_cast<std::ptrdiff_t>(index));
  masses_.erase(masses_.begin() + static_cast<std::ptrdiff_t>(index));
  inverseMasses_.erase(inverseMasses_.begin() +
                       static_cast<std::ptrdiff_t>(index));

  PhysicsBodyStoreResult result =
      storeResult(PhysicsStatus::BodyRemoved, true);
  result.id = id;
  result.index = index;
  result.body = removed;
  return result;
}

PhysicsBodyStoreResult PhysicsBodyStore::reset() {
  ids_.clear();
  motions_.clear();
  positions_.clear();
  velocities_.clear();
  masses_.clear();
  inverseMasses_.clear();
  nextId_ = 1U;
  return storeResult(PhysicsStatus::StoreReset, true);
}

PhysicsBodyView PhysicsBodyStore::bodyAt(std::size_t index) const {
  PhysicsBodyView body;
  body.id = ids_[index];
  body.motion = motions_[index];
  body.positionMeters = positions_[index];
  body.velocityMetersPerSecond = velocities_[index];
  body.massKilograms = masses_[index];
  body.inverseMass = inverseMasses_[index];
  return body;
}

std::size_t PhysicsBodyStore::findIndex(PhysicsBodyId id) const {
  // branch-gate: BG-1085
  if (!isValidPhysicsBodyId(id)) {
    return kMissingIndex;
  }
  for (std::size_t index = 0U; index < ids_.size(); ++index) {
    // branch-gate: BG-1085
    if (ids_[index].value == id.value) {
      return index;
    }
  }
  return kMissingIndex;
}

}  // namespace iggy3d
