#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include "runtime/physics/PhysicsTypes.hpp"

namespace iggy3d {

enum class PhysicsShapeStatus : std::uint8_t {
  Valid,
  MissingDescriptor,
  InvalidShapeKind,
  InvalidCenterOffset,
  InvalidHalfExtents,
  ShapeAdded,
  ShapeNotFound,
  ShapeRemoved,
  StoreReset,
};

struct PhysicsShapeDescriptor {
  PhysicsShapeKind kind = PhysicsShapeKind::Box;
  Vec3 localCenterOffsetMeters;
  Vec3 halfExtentsMeters{0.5F, 0.5F, 0.5F};
  bool sensor = false;
};

struct PhysicsShapeView {
  PhysicsShapeId id;
  PhysicsShapeKind kind = PhysicsShapeKind::Box;
  Vec3 localCenterOffsetMeters;
  Vec3 halfExtentsMeters{0.5F, 0.5F, 0.5F};
  bool sensor = false;
};

struct PhysicsShapeValidationResult {
  bool ok = false;
  PhysicsShapeStatus status = PhysicsShapeStatus::MissingDescriptor;
  std::string_view reasonCode = "physics_shape_missing_descriptor";
};

struct PhysicsShapeStoreResult {
  bool ok = false;
  PhysicsShapeStatus status = PhysicsShapeStatus::ShapeNotFound;
  std::string_view reasonCode = "physics_shape_not_found";
  PhysicsShapeId id;
  std::size_t index = 0U;
  PhysicsShapeView shape;
};

std::string_view physicsShapeStatusName(PhysicsShapeStatus status);
PhysicsShapeValidationResult validatePhysicsShapeDescriptor(
    const PhysicsShapeDescriptor* descriptor);

class PhysicsShapeStore {
 public:
  PhysicsShapeStore() = default;

  [[nodiscard]] std::size_t size() const;
  [[nodiscard]] bool empty() const;
  [[nodiscard]] const std::vector<PhysicsShapeId>& ids() const;
  [[nodiscard]] const std::vector<PhysicsShapeKind>& kinds() const;
  [[nodiscard]] const std::vector<Vec3>& localCenterOffsets() const;
  [[nodiscard]] const std::vector<Vec3>& halfExtents() const;
  [[nodiscard]] const std::vector<bool>& sensors() const;

  PhysicsShapeStoreResult add(const PhysicsShapeDescriptor& descriptor);
  PhysicsShapeStoreResult read(PhysicsShapeId id) const;
  PhysicsShapeStoreResult remove(PhysicsShapeId id);
  PhysicsShapeStoreResult reset();

 private:
  [[nodiscard]] PhysicsShapeView shapeAt(std::size_t index) const;
  [[nodiscard]] std::size_t findIndex(PhysicsShapeId id) const;

  std::vector<PhysicsShapeId> ids_;
  std::vector<PhysicsShapeKind> kinds_;
  std::vector<Vec3> localCenterOffsets_;
  std::vector<Vec3> halfExtents_;
  std::vector<bool> sensors_;
  std::uint32_t nextId_ = 1U;
};

}  // namespace iggy3d
