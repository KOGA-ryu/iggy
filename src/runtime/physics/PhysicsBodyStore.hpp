#pragma once

#include <cstddef>
#include <vector>

#include "runtime/physics/PhysicsTypes.hpp"

namespace iggy3d {

class PhysicsBodyStore;
struct PhysicsStepConfig;
struct PhysicsStepResult;
PhysicsStepResult stepPhysicsBodies(PhysicsBodyStore* store,
                                    const PhysicsStepConfig& config);

struct PhysicsBodyView {
  PhysicsBodyId id;
  PhysicsBodyMotionKind motion = PhysicsBodyMotionKind::Static;
  Vec3 positionMeters;
  Vec3 velocityMetersPerSecond;
  float massKilograms = 0.0F;
  float inverseMass = 0.0F;
};

struct PhysicsBodyStoreResult {
  bool ok = false;
  PhysicsStatus status = PhysicsStatus::BodyNotFound;
  std::string_view reasonCode = "physics_body_not_found";
  PhysicsBodyId id;
  std::size_t index = 0U;
  PhysicsBodyView body;
};

class PhysicsBodyStore {
 public:
  PhysicsBodyStore() = default;

  [[nodiscard]] std::size_t size() const;
  [[nodiscard]] bool empty() const;
  [[nodiscard]] const std::vector<PhysicsBodyId>& ids() const;
  [[nodiscard]] const std::vector<PhysicsBodyMotionKind>& motions() const;
  [[nodiscard]] const std::vector<Vec3>& positions() const;
  [[nodiscard]] const std::vector<Vec3>& velocities() const;
  [[nodiscard]] const std::vector<float>& masses() const;
  [[nodiscard]] const std::vector<float>& inverseMasses() const;

  PhysicsBodyStoreResult add(const PhysicsBodyDescriptor& descriptor);
  PhysicsBodyStoreResult read(PhysicsBodyId id) const;
  PhysicsBodyStoreResult remove(PhysicsBodyId id);
  PhysicsBodyStoreResult reset();

 private:
  friend PhysicsStepResult stepPhysicsBodies(PhysicsBodyStore* store,
                                             const PhysicsStepConfig& config);

  [[nodiscard]] PhysicsBodyView bodyAt(std::size_t index) const;
  [[nodiscard]] std::size_t findIndex(PhysicsBodyId id) const;

  std::vector<PhysicsBodyId> ids_;
  std::vector<PhysicsBodyMotionKind> motions_;
  std::vector<Vec3> positions_;
  std::vector<Vec3> velocities_;
  std::vector<float> masses_;
  std::vector<float> inverseMasses_;
  std::uint32_t nextId_ = 1U;
};

}  // namespace iggy3d
