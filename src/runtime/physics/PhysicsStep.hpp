#pragma once

#include <cstddef>
#include <string_view>

#include "core/math/Vec3.hpp"
#include "runtime/physics/PhysicsBodyStore.hpp"

namespace iggy3d {

enum class PhysicsStepStatus : std::uint8_t {
  Stepped,
  MissingStore,
  InvalidStepSeconds,
  InvalidGravity,
  InvalidBodyState,
};

struct PhysicsStepConfig {
  float stepSeconds = 1.0F / 60.0F;
  Vec3 gravityMetersPerSecondSquared{0.0F, -9.80665F, 0.0F};
};

struct PhysicsStepResult {
  bool ok = false;
  PhysicsStepStatus status = PhysicsStepStatus::MissingStore;
  std::string_view reasonCode = "physics_step_missing_store";
  float stepSeconds = 0.0F;
  std::size_t bodyCount = 0U;
  std::size_t staticBodyCount = 0U;
  std::size_t dynamicBodyCount = 0U;
  std::size_t kinematicBodyCount = 0U;
  std::size_t integratedBodyCount = 0U;
  std::size_t gravityAppliedBodyCount = 0U;
};

std::string_view physicsStepStatusName(PhysicsStepStatus status);

PhysicsStepResult stepPhysicsBodies(PhysicsBodyStore* store,
                                    const PhysicsStepConfig& config);

}  // namespace iggy3d
