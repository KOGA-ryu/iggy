#include "runtime/physics/PhysicsColliderBake.hpp"

#include <array>
#include <limits>
#include <utility>

namespace iggy3d {
namespace {

constexpr std::size_t kMissingIndex = std::numeric_limits<std::size_t>::max();

template <std::size_t Count, typename Enum>
std::string_view enumName(Enum value,
                          const std::array<std::string_view, Count>& names,
                          std::string_view fallback) {
  const auto index = static_cast<std::size_t>(value);
  // branch-gate: BG-1094
  if (index >= names.size()) {
    return fallback;
  }
  return names[index];
}

PhysicsBodyShapeBindingValidationResult bindingValidationResult(
    PhysicsBodyShapeBindingStatus status,
    bool ok) {
  PhysicsBodyShapeBindingValidationResult result;
  result.ok = ok;
  result.status = status;
  result.reasonCode = physicsBodyShapeBindingStatusName(status);
  return result;
}

PhysicsBodyShapeBindingStoreResult bindingStoreResult(
    PhysicsBodyShapeBindingStatus status,
    bool ok) {
  PhysicsBodyShapeBindingStoreResult result;
  result.ok = ok;
  result.status = status;
  result.reasonCode = physicsBodyShapeBindingStatusName(status);
  return result;
}

PhysicsAabbColliderBakeResult bakeResult(PhysicsAabbColliderBakeStatus status,
                                         bool ok) {
  PhysicsAabbColliderBakeResult result;
  result.ok = ok;
  result.status = status;
  result.reasonCode = physicsAabbColliderBakeStatusName(status);
  return result;
}

bool bindingStoreVectorShapeMatches(
    const PhysicsBodyShapeBindingStore& store) {
  const std::size_t size = store.ids().size();
  return store.bodyIds().size() == size && store.shapeIds().size() == size &&
         store.materialIds().size() == size && store.enabled().size() == size;
}

bool bindingStoreRowValid(const PhysicsBodyShapeBindingStore& store,
                          std::size_t index) {
  return isValidPhysicsBodyShapeBindingId(store.ids()[index]) &&
         isValidPhysicsBodyId(store.bodyIds()[index]) &&
         isValidPhysicsShapeId(store.shapeIds()[index]) &&
         isValidPhysicsMaterialId(store.materialIds()[index]);
}

PhysicsAabbColliderBakeResult invalidBindingStoreResult(
    const PhysicsBodyShapeBindingStore& store,
    std::size_t index) {
  PhysicsAabbColliderBakeResult result =
      bakeResult(PhysicsAabbColliderBakeStatus::InvalidBindingStoreState,
                 false);
  result.bindingCount = store.size();
  result.invalidBindingIndex = index;
  // branch-gate: BG-1094
  if (index < store.bodyIds().size()) {
    result.invalidBodyId = store.bodyIds()[index];
  }
  // branch-gate: BG-1094
  if (index < store.shapeIds().size()) {
    result.invalidShapeId = store.shapeIds()[index];
  }
  // branch-gate: BG-1094
  if (index < store.materialIds().size()) {
    result.invalidMaterialId = store.materialIds()[index];
  }
  return result;
}

PhysicsAabbColliderBakeResult validateBindingStore(
    const PhysicsBodyShapeBindingStore& store) {
  // branch-gate: BG-1094
  if (!bindingStoreVectorShapeMatches(store)) {
    return invalidBindingStoreResult(store, 0U);
  }
  for (std::size_t index = 0U; index < store.size(); ++index) {
    // branch-gate: BG-1094
    if (!bindingStoreRowValid(store, index)) {
      return invalidBindingStoreResult(store, index);
    }
  }
  return bakeResult(PhysicsAabbColliderBakeStatus::Baked, true);
}

PhysicsAabbColliderBakeResult bodyNotFoundResult(
    std::size_t bindingIndex,
    PhysicsBodyId bodyId) {
  PhysicsAabbColliderBakeResult result =
      bakeResult(PhysicsAabbColliderBakeStatus::BodyNotFound, false);
  result.invalidBindingIndex = bindingIndex;
  result.invalidBodyId = bodyId;
  return result;
}

PhysicsAabbColliderBakeResult shapeNotFoundResult(
    std::size_t bindingIndex,
    PhysicsShapeId shapeId) {
  PhysicsAabbColliderBakeResult result =
      bakeResult(PhysicsAabbColliderBakeStatus::ShapeNotFound, false);
  result.invalidBindingIndex = bindingIndex;
  result.invalidShapeId = shapeId;
  return result;
}

PhysicsAabbColliderBakeResult materialNotFoundResult(
    std::size_t bindingIndex,
    PhysicsMaterialId materialId) {
  PhysicsAabbColliderBakeResult result =
      bakeResult(PhysicsAabbColliderBakeStatus::MaterialNotFound, false);
  result.invalidBindingIndex = bindingIndex;
  result.invalidMaterialId = materialId;
  return result;
}

PhysicsAabbColliderBakeResult colliderBuildFailureResult(
    std::size_t bindingIndex,
    PhysicsBodyId bodyId,
    PhysicsShapeId shapeId,
    PhysicsAabbColliderStatus colliderStatus) {
  // branch-gate: BG-1094
  const PhysicsAabbColliderBakeStatus status =
      colliderStatus == PhysicsAabbColliderStatus::InvalidShapeKind
          ? PhysicsAabbColliderBakeStatus::InvalidShapeKind
          : PhysicsAabbColliderBakeStatus::ColliderBuildFailed;
  PhysicsAabbColliderBakeResult result = bakeResult(status, false);
  result.invalidBindingIndex = bindingIndex;
  result.invalidBodyId = bodyId;
  result.invalidShapeId = shapeId;
  return result;
}

struct PhysicsAabbColliderBakePacket {
  std::vector<PhysicsAabbCollider> colliders;
  std::vector<PhysicsBodyId> bodyIds;
  std::vector<PhysicsShapeId> shapeIds;
  std::vector<PhysicsMaterialId> materialIds;
  std::vector<std::size_t> sourceBindingIndices;
};

void appendBakedCollider(PhysicsAabbColliderBakePacket* packet,
                         const PhysicsAabbCollider& collider,
                         PhysicsBodyId bodyId,
                         PhysicsShapeId shapeId,
                         PhysicsMaterialId materialId,
                         std::size_t sourceBindingIndex) {
  packet->colliders.push_back(collider);
  packet->bodyIds.push_back(bodyId);
  packet->shapeIds.push_back(shapeId);
  packet->materialIds.push_back(materialId);
  packet->sourceBindingIndices.push_back(sourceBindingIndex);
}

}  // namespace

std::string_view physicsBodyShapeBindingStatusName(
    PhysicsBodyShapeBindingStatus status) {
  static constexpr std::array<std::string_view, 10> kNames{
      "physics_body_shape_binding_valid",
      "physics_body_shape_binding_missing_descriptor",
      "physics_body_shape_binding_invalid_binding_id",
      "physics_body_shape_binding_invalid_body_id",
      "physics_body_shape_binding_invalid_shape_id",
      "physics_body_shape_binding_invalid_material_id",
      "physics_body_shape_binding_added",
      "physics_body_shape_binding_not_found",
      "physics_body_shape_binding_removed",
      "physics_body_shape_binding_store_reset",
  };
  return enumName(status, kNames,
                  "physics_body_shape_binding_invalid_binding_id");
}

std::string_view physicsAabbColliderBakeStatusName(
    PhysicsAabbColliderBakeStatus status) {
  static constexpr std::array<std::string_view, 11> kNames{
      "physics_aabb_bake_baked",
      "physics_aabb_bake_missing_body_store",
      "physics_aabb_bake_missing_shape_store",
      "physics_aabb_bake_missing_material_table",
      "physics_aabb_bake_missing_binding_store",
      "physics_aabb_bake_invalid_binding_store_state",
      "physics_aabb_bake_body_not_found",
      "physics_aabb_bake_shape_not_found",
      "physics_aabb_bake_material_not_found",
      "physics_aabb_bake_invalid_shape_kind",
      "physics_aabb_bake_collider_build_failed",
  };
  return enumName(status, kNames, "physics_aabb_bake_collider_build_failed");
}

bool isValidPhysicsBodyShapeBindingId(PhysicsBodyShapeBindingId id) {
  return id.value != 0U;
}

PhysicsBodyShapeBindingValidationResult
validatePhysicsBodyShapeBindingDescriptor(
    const PhysicsBodyShapeBindingDescriptor* descriptor) {
  // branch-gate: BG-1094
  if (descriptor == nullptr) {
    return bindingValidationResult(
        PhysicsBodyShapeBindingStatus::MissingDescriptor, false);
  }
  // branch-gate: BG-1094
  if (!isValidPhysicsBodyId(descriptor->bodyId)) {
    return bindingValidationResult(
        PhysicsBodyShapeBindingStatus::InvalidBodyId, false);
  }
  // branch-gate: BG-1094
  if (!isValidPhysicsShapeId(descriptor->shapeId)) {
    return bindingValidationResult(
        PhysicsBodyShapeBindingStatus::InvalidShapeId, false);
  }
  // branch-gate: BG-1094
  if (!isValidPhysicsMaterialId(descriptor->materialId)) {
    return bindingValidationResult(
        PhysicsBodyShapeBindingStatus::InvalidMaterialId, false);
  }
  return bindingValidationResult(PhysicsBodyShapeBindingStatus::Valid, true);
}

std::size_t PhysicsBodyShapeBindingStore::size() const {
  return ids_.size();
}

bool PhysicsBodyShapeBindingStore::empty() const {
  return ids_.empty();
}

const std::vector<PhysicsBodyShapeBindingId>&
PhysicsBodyShapeBindingStore::ids() const {
  return ids_;
}

const std::vector<PhysicsBodyId>& PhysicsBodyShapeBindingStore::bodyIds()
    const {
  return bodyIds_;
}

const std::vector<PhysicsShapeId>& PhysicsBodyShapeBindingStore::shapeIds()
    const {
  return shapeIds_;
}

const std::vector<PhysicsMaterialId>&
PhysicsBodyShapeBindingStore::materialIds() const {
  return materialIds_;
}

const std::vector<bool>& PhysicsBodyShapeBindingStore::enabled() const {
  return enabled_;
}

PhysicsBodyShapeBindingStoreResult PhysicsBodyShapeBindingStore::add(
    const PhysicsBodyShapeBindingDescriptor& descriptor) {
  const PhysicsBodyShapeBindingValidationResult validation =
      validatePhysicsBodyShapeBindingDescriptor(&descriptor);
  // branch-gate: BG-1094
  if (!validation.ok) {
    PhysicsBodyShapeBindingStoreResult result;
    result.status = validation.status;
    result.reasonCode = validation.reasonCode;
    return result;
  }

  const PhysicsBodyShapeBindingId id{nextId_++};
  ids_.push_back(id);
  bodyIds_.push_back(descriptor.bodyId);
  shapeIds_.push_back(descriptor.shapeId);
  materialIds_.push_back(descriptor.materialId);
  enabled_.push_back(descriptor.enabled);

  PhysicsBodyShapeBindingStoreResult result =
      bindingStoreResult(PhysicsBodyShapeBindingStatus::BindingAdded, true);
  result.id = id;
  result.index = ids_.size() - 1U;
  result.binding = bindingAt(result.index);
  return result;
}

PhysicsBodyShapeBindingStoreResult PhysicsBodyShapeBindingStore::read(
    PhysicsBodyShapeBindingId id) const {
  const std::size_t index = findIndex(id);
  // branch-gate: BG-1094
  if (index == kMissingIndex) {
    PhysicsBodyShapeBindingStoreResult result =
        bindingStoreResult(PhysicsBodyShapeBindingStatus::BindingNotFound,
                           false);
    result.id = id;
    return result;
  }

  PhysicsBodyShapeBindingStoreResult result =
      bindingStoreResult(PhysicsBodyShapeBindingStatus::Valid, true);
  result.id = id;
  result.index = index;
  result.binding = bindingAt(index);
  return result;
}

PhysicsBodyShapeBindingStoreResult PhysicsBodyShapeBindingStore::remove(
    PhysicsBodyShapeBindingId id) {
  const std::size_t index = findIndex(id);
  // branch-gate: BG-1094
  if (index == kMissingIndex) {
    PhysicsBodyShapeBindingStoreResult result =
        bindingStoreResult(PhysicsBodyShapeBindingStatus::BindingNotFound,
                           false);
    result.id = id;
    return result;
  }

  const PhysicsBodyShapeBindingView removed = bindingAt(index);
  ids_.erase(ids_.begin() + static_cast<std::ptrdiff_t>(index));
  bodyIds_.erase(bodyIds_.begin() + static_cast<std::ptrdiff_t>(index));
  shapeIds_.erase(shapeIds_.begin() + static_cast<std::ptrdiff_t>(index));
  materialIds_.erase(materialIds_.begin() +
                     static_cast<std::ptrdiff_t>(index));
  enabled_.erase(enabled_.begin() + static_cast<std::ptrdiff_t>(index));

  PhysicsBodyShapeBindingStoreResult result =
      bindingStoreResult(PhysicsBodyShapeBindingStatus::BindingRemoved, true);
  result.id = id;
  result.index = index;
  result.binding = removed;
  return result;
}

PhysicsBodyShapeBindingStoreResult PhysicsBodyShapeBindingStore::reset() {
  ids_.clear();
  bodyIds_.clear();
  shapeIds_.clear();
  materialIds_.clear();
  enabled_.clear();
  nextId_ = 1U;
  return bindingStoreResult(PhysicsBodyShapeBindingStatus::StoreReset, true);
}

PhysicsBodyShapeBindingView PhysicsBodyShapeBindingStore::bindingAt(
    std::size_t index) const {
  PhysicsBodyShapeBindingView binding;
  binding.id = ids_[index];
  binding.bodyId = bodyIds_[index];
  binding.shapeId = shapeIds_[index];
  binding.materialId = materialIds_[index];
  binding.enabled = enabled_[index];
  return binding;
}

std::size_t PhysicsBodyShapeBindingStore::findIndex(
    PhysicsBodyShapeBindingId id) const {
  // branch-gate: BG-1094
  if (!isValidPhysicsBodyShapeBindingId(id)) {
    return kMissingIndex;
  }
  for (std::size_t index = 0U; index < ids_.size(); ++index) {
    // branch-gate: BG-1094
    if (ids_[index].value == id.value) {
      return index;
    }
  }
  return kMissingIndex;
}

PhysicsAabbColliderBakeResult bakePhysicsAabbColliders(
    const PhysicsAabbColliderBakeRequest& request) {
  // branch-gate: BG-1094
  if (request.bodies == nullptr) {
    return bakeResult(PhysicsAabbColliderBakeStatus::MissingBodyStore, false);
  }
  // branch-gate: BG-1094
  if (request.shapes == nullptr) {
    return bakeResult(PhysicsAabbColliderBakeStatus::MissingShapeStore, false);
  }
  // branch-gate: BG-1094
  if (request.materials == nullptr) {
    return bakeResult(PhysicsAabbColliderBakeStatus::MissingMaterialTable,
                      false);
  }
  // branch-gate: BG-1094
  if (request.bindings == nullptr) {
    return bakeResult(PhysicsAabbColliderBakeStatus::MissingBindingStore,
                      false);
  }

  PhysicsAabbColliderBakeResult bindingValidation =
      validateBindingStore(*request.bindings);
  // branch-gate: BG-1094
  if (!bindingValidation.ok) {
    return bindingValidation;
  }

  PhysicsAabbColliderBakePacket packet;
  PhysicsAabbColliderBakeResult counts =
      bakeResult(PhysicsAabbColliderBakeStatus::Baked, true);
  counts.bindingCount = request.bindings->size();

  for (std::size_t index = 0U; index < request.bindings->size(); ++index) {
    // branch-gate: BG-1094
    if (!request.bindings->enabled()[index]) {
      ++counts.disabledBindingCount;
      continue;
    }
    ++counts.enabledBindingCount;
    const PhysicsBodyId bodyId = request.bindings->bodyIds()[index];
    const PhysicsShapeId shapeId = request.bindings->shapeIds()[index];
    const PhysicsMaterialId materialId = request.bindings->materialIds()[index];

    const PhysicsBodyStoreResult body = request.bodies->read(bodyId);
    // branch-gate: BG-1094
    if (!body.ok) {
      return bodyNotFoundResult(index, bodyId);
    }
    const PhysicsShapeStoreResult shape = request.shapes->read(shapeId);
    // branch-gate: BG-1094
    if (!shape.ok) {
      return shapeNotFoundResult(index, shapeId);
    }
    const PhysicsMaterialTableResult material =
        readPhysicsMaterial(request.materials, materialId);
    // branch-gate: BG-1094
    if (!material.ok) {
      return materialNotFoundResult(index, materialId);
    }
    const PhysicsAabbColliderResult collider =
        buildPhysicsAabbColliderFromShape(request.shapes, shapeId, bodyId,
                                          body.body.positionMeters);
    // branch-gate: BG-1094
    if (!collider.ok) {
      return colliderBuildFailureResult(index, bodyId, shapeId,
                                        collider.status);
    }

    appendBakedCollider(&packet, collider.collider, bodyId, shapeId,
                        materialId, index);
  }

  counts.colliderCount = packet.colliders.size();
  counts.colliders = std::move(packet.colliders);
  counts.bodyIds = std::move(packet.bodyIds);
  counts.shapeIds = std::move(packet.shapeIds);
  counts.materialIds = std::move(packet.materialIds);
  counts.sourceBindingIndices = std::move(packet.sourceBindingIndices);
  return counts;
}

}  // namespace iggy3d
