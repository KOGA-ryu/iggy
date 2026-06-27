#include "runtime/physics/PhysicsStep.hpp"

#include <array>
#include <cmath>
#include <vector>

namespace iggy3d {
namespace {

template <std::size_t Count, typename Enum>
std::string_view enumName(Enum value,
                          const std::array<std::string_view, Count>& names,
                          std::string_view fallback) {
  const auto index = static_cast<std::size_t>(value);
  // branch-gate: BG-1086
  if (index >= names.size()) {
    return fallback;
  }
  return names[index];
}

PhysicsStepResult stepResult(PhysicsStepStatus status, bool ok) {
  PhysicsStepResult result;
  result.ok = ok;
  result.status = status;
  result.reasonCode = physicsStepStatusName(status);
  return result;
}

bool validStepSeconds(float value) {
  return std::isfinite(value) && value > 0.0F;
}

bool validStoreShape(std::size_t ids,
                     std::size_t motions,
                     std::size_t positions,
                     std::size_t velocities,
                     std::size_t masses,
                     std::size_t inverseMasses) {
  return motions == ids && positions == ids && velocities == ids &&
         masses == ids && inverseMasses == ids;
}

bool validStoredBody(PhysicsBodyMotionKind motion,
                     Vec3 position,
                     Vec3 velocity,
                     float mass,
                     float inverseMass) {
  PhysicsBodyDescriptor descriptor;
  descriptor.motion = motion;
  descriptor.positionMeters = position;
  descriptor.velocityMetersPerSecond = velocity;
  descriptor.massKilograms = mass;
  const PhysicsValidationResult validation =
      validatePhysicsBodyDescriptor(&descriptor);
  return validation.ok && std::isfinite(inverseMass);
}

void countMotion(PhysicsStepResult& result, PhysicsBodyMotionKind motion) {
  // branch-gate: BG-1086
  switch (motion) {
    case PhysicsBodyMotionKind::Static:
      ++result.staticBodyCount;
      return;
    case PhysicsBodyMotionKind::Dynamic:
      ++result.dynamicBodyCount;
      return;
    case PhysicsBodyMotionKind::Kinematic:
      ++result.kinematicBodyCount;
      return;
  }
}

PhysicsStepResult validateStepRequest(PhysicsBodyStore* store,
                                      const PhysicsStepConfig& config) {
  // branch-gate: BG-1086
  if (store == nullptr) {
    return stepResult(PhysicsStepStatus::MissingStore, false);
  }
  // branch-gate: BG-1086
  if (!validStepSeconds(config.stepSeconds)) {
    return stepResult(PhysicsStepStatus::InvalidStepSeconds, false);
  }
  // branch-gate: BG-1086
  if (!isFinite(config.gravityMetersPerSecondSquared)) {
    return stepResult(PhysicsStepStatus::InvalidGravity, false);
  }
  // branch-gate: BG-1086
  if (!validStoreShape(store->ids().size(),
                       store->motions().size(),
                       store->positions().size(),
                       store->velocities().size(),
                       store->masses().size(),
                       store->inverseMasses().size())) {
    return stepResult(PhysicsStepStatus::InvalidBodyState, false);
  }

  PhysicsStepResult result = stepResult(PhysicsStepStatus::Stepped, true);
  result.stepSeconds = config.stepSeconds;
  result.bodyCount = store->ids().size();
  for (std::size_t index = 0U; index < store->ids().size(); ++index) {
    // branch-gate: BG-1086
    if (!validStoredBody(store->motions()[index],
                         store->positions()[index],
                         store->velocities()[index],
                         store->masses()[index],
                         store->inverseMasses()[index])) {
      return stepResult(PhysicsStepStatus::InvalidBodyState, false);
    }
    countMotion(result, store->motions()[index]);
  }
  return result;
}

void integrateVelocities(
    const std::vector<PhysicsBodyMotionKind>& motions,
    std::vector<Vec3>& velocities,
    const PhysicsStepConfig& config,
    PhysicsStepResult& result) {
  for (std::size_t index = 0U; index < motions.size(); ++index) {
    // branch-gate: BG-1086
    if (motions[index] != PhysicsBodyMotionKind::Dynamic) {
      continue;
    }
    velocities[index] =
        velocities[index] +
        config.gravityMetersPerSecondSquared * config.stepSeconds;
    ++result.gravityAppliedBodyCount;
    ++result.velocityIntegratedBodyCount;
  }
}

void integratePositions(
    const std::vector<PhysicsBodyMotionKind>& motions,
    std::vector<Vec3>& positions,
    const std::vector<Vec3>& velocities,
    const PhysicsStepConfig& config,
    PhysicsStepResult& result) {
  for (std::size_t index = 0U; index < motions.size(); ++index) {
    const PhysicsBodyMotionKind motion = motions[index];
    // branch-gate: BG-1086
    switch (motion) {
      case PhysicsBodyMotionKind::Static:
        break;
      case PhysicsBodyMotionKind::Dynamic:
      case PhysicsBodyMotionKind::Kinematic:
        positions[index] =
            positions[index] + velocities[index] * config.stepSeconds;
        ++result.integratedBodyCount;
        ++result.positionIntegratedBodyCount;
        break;
    }
  }
}

}  // namespace

std::string_view physicsStepStatusName(PhysicsStepStatus status) {
  static constexpr std::array<std::string_view, 5> kNames{
      "physics_step_stepped",
      "physics_step_missing_store",
      "physics_step_invalid_step_seconds",
      "physics_step_invalid_gravity",
      "physics_step_invalid_body_state",
  };
  return enumName(status, kNames, "physics_step_invalid_body_state");
}

PhysicsStepResult integratePhysicsBodyVelocities(
    PhysicsBodyStore* store,
    const PhysicsStepConfig& config) {
  PhysicsStepResult result = validateStepRequest(store, config);
  // branch-gate: BG-1086
  if (!result.ok) {
    return result;
  }
  integrateVelocities(store->motions_, store->velocities_, config, result);
  return result;
}

PhysicsStepResult integratePhysicsBodyPositions(
    PhysicsBodyStore* store,
    const PhysicsStepConfig& config) {
  PhysicsStepResult result = validateStepRequest(store, config);
  // branch-gate: BG-1086
  if (!result.ok) {
    return result;
  }
  integratePositions(
      store->motions_, store->positions_, store->velocities_, config, result);
  return result;
}

PhysicsStepResult stepPhysicsBodies(PhysicsBodyStore* store,
                                    const PhysicsStepConfig& config) {
  PhysicsStepResult result = validateStepRequest(store, config);
  // branch-gate: BG-1086
  if (!result.ok) {
    return result;
  }
  integrateVelocities(store->motions_, store->velocities_, config, result);
  integratePositions(
      store->motions_, store->positions_, store->velocities_, config, result);
  return result;
}

}  // namespace iggy3d
