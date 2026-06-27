#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include "runtime/physics/PhysicsAabbCollider.hpp"
#include "runtime/physics/PhysicsBodyStore.hpp"
#include "runtime/physics/PhysicsMaterialTraits.hpp"
#include "runtime/physics/PhysicsShapeStore.hpp"

namespace iggy3d {

struct PhysicsBodyShapeBindingId {
  std::uint32_t value = 0U;
};

enum class PhysicsBodyShapeBindingStatus : std::uint8_t {
  Valid,
  MissingDescriptor,
  InvalidBindingId,
  InvalidBodyId,
  InvalidShapeId,
  InvalidMaterialId,
  BindingAdded,
  BindingNotFound,
  BindingRemoved,
  StoreReset,
};

enum class PhysicsAabbColliderBakeStatus : std::uint8_t {
  Baked,
  MissingBodyStore,
  MissingShapeStore,
  MissingMaterialTable,
  MissingBindingStore,
  InvalidBindingStoreState,
  BodyNotFound,
  ShapeNotFound,
  MaterialNotFound,
  InvalidShapeKind,
  ColliderBuildFailed,
};

struct PhysicsBodyShapeBindingDescriptor {
  PhysicsBodyId bodyId;
  PhysicsShapeId shapeId;
  PhysicsMaterialId materialId;
  bool enabled = true;
};

struct PhysicsBodyShapeBindingView {
  PhysicsBodyShapeBindingId id;
  PhysicsBodyId bodyId;
  PhysicsShapeId shapeId;
  PhysicsMaterialId materialId;
  bool enabled = true;
};

struct PhysicsBodyShapeBindingValidationResult {
  bool ok = false;
  PhysicsBodyShapeBindingStatus status =
      PhysicsBodyShapeBindingStatus::MissingDescriptor;
  std::string_view reasonCode = "physics_body_shape_binding_missing_descriptor";
};

struct PhysicsBodyShapeBindingStoreResult {
  bool ok = false;
  PhysicsBodyShapeBindingStatus status =
      PhysicsBodyShapeBindingStatus::BindingNotFound;
  std::string_view reasonCode = "physics_body_shape_binding_not_found";
  PhysicsBodyShapeBindingId id;
  std::size_t index = 0U;
  PhysicsBodyShapeBindingView binding;
};

struct PhysicsAabbColliderBakeRequest {
  const PhysicsBodyStore* bodies = nullptr;
  const PhysicsShapeStore* shapes = nullptr;
  const PhysicsMaterialTable* materials = nullptr;
  const class PhysicsBodyShapeBindingStore* bindings = nullptr;
};

struct PhysicsAabbColliderBakeResult {
  bool ok = false;
  PhysicsAabbColliderBakeStatus status =
      PhysicsAabbColliderBakeStatus::MissingBodyStore;
  std::string_view reasonCode = "physics_aabb_bake_missing_body_store";
  std::size_t bindingCount = 0U;
  std::size_t enabledBindingCount = 0U;
  std::size_t disabledBindingCount = 0U;
  std::size_t colliderCount = 0U;
  std::size_t invalidBindingIndex = 0U;
  PhysicsBodyId invalidBodyId;
  PhysicsShapeId invalidShapeId;
  PhysicsMaterialId invalidMaterialId;
  std::vector<PhysicsAabbCollider> colliders;
  std::vector<PhysicsBodyId> bodyIds;
  std::vector<PhysicsShapeId> shapeIds;
  std::vector<PhysicsMaterialId> materialIds;
  std::vector<std::size_t> sourceBindingIndices;
};

std::string_view physicsBodyShapeBindingStatusName(
    PhysicsBodyShapeBindingStatus status);
std::string_view physicsAabbColliderBakeStatusName(
    PhysicsAabbColliderBakeStatus status);
bool isValidPhysicsBodyShapeBindingId(PhysicsBodyShapeBindingId id);
PhysicsBodyShapeBindingValidationResult validatePhysicsBodyShapeBindingDescriptor(
    const PhysicsBodyShapeBindingDescriptor* descriptor);

class PhysicsBodyShapeBindingStore {
 public:
  PhysicsBodyShapeBindingStore() = default;

  [[nodiscard]] std::size_t size() const;
  [[nodiscard]] bool empty() const;
  [[nodiscard]] const std::vector<PhysicsBodyShapeBindingId>& ids() const;
  [[nodiscard]] const std::vector<PhysicsBodyId>& bodyIds() const;
  [[nodiscard]] const std::vector<PhysicsShapeId>& shapeIds() const;
  [[nodiscard]] const std::vector<PhysicsMaterialId>& materialIds() const;
  [[nodiscard]] const std::vector<bool>& enabled() const;

  PhysicsBodyShapeBindingStoreResult add(
      const PhysicsBodyShapeBindingDescriptor& descriptor);
  PhysicsBodyShapeBindingStoreResult read(
      PhysicsBodyShapeBindingId id) const;
  PhysicsBodyShapeBindingStoreResult remove(PhysicsBodyShapeBindingId id);
  PhysicsBodyShapeBindingStoreResult reset();

 private:
  [[nodiscard]] PhysicsBodyShapeBindingView bindingAt(
      std::size_t index) const;
  [[nodiscard]] std::size_t findIndex(PhysicsBodyShapeBindingId id) const;

  std::vector<PhysicsBodyShapeBindingId> ids_;
  std::vector<PhysicsBodyId> bodyIds_;
  std::vector<PhysicsShapeId> shapeIds_;
  std::vector<PhysicsMaterialId> materialIds_;
  std::vector<bool> enabled_;
  std::uint32_t nextId_ = 1U;
};

PhysicsAabbColliderBakeResult bakePhysicsAabbColliders(
    const PhysicsAabbColliderBakeRequest& request);

}  // namespace iggy3d
