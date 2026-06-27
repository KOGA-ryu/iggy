#include "runtime/physics/PhysicsStep.hpp"

#include <array>
#include <cmath>

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

PhysicsStepResult stepPhysicsBodies(PhysicsBodyStore* store,
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
  if (!validStoreShape(store->ids_.size(),
                       store->motions_.size(),
                       store->positions_.size(),
                       store->velocities_.size(),
                       store->masses_.size(),
                       store->inverseMasses_.size())) {
    return stepResult(PhysicsStepStatus::InvalidBodyState, false);
  }

  PhysicsStepResult result =
      stepResult(PhysicsStepStatus::Stepped, true);
  result.stepSeconds = config.stepSeconds;
  result.bodyCount = store->ids_.size();

  for (std::size_t index = 0U; index < store->ids_.size(); ++index) {
    // branch-gate: BG-1086
    if (!validStoredBody(store->motions_[index],
                         store->positions_[index],
                         store->velocities_[index],
                         store->masses_[index],
                         store->inverseMasses_[index])) {
      return stepResult(PhysicsStepStatus::InvalidBodyState, false);
    }
    countMotion(result, store->motions_[index]);
  }

  for (std::size_t index = 0U; index < store->ids_.size(); ++index) {
    const PhysicsBodyMotionKind motion = store->motions_[index];
    // branch-gate: BG-1086
    switch (motion) {
      case PhysicsBodyMotionKind::Static:
        break;
      case PhysicsBodyMotionKind::Dynamic:
        store->velocities_[index] =
            store->velocities_[index] +
            config.gravityMetersPerSecondSquared * config.stepSeconds;
        store->positions_[index] =
            store->positions_[index] +
            store->velocities_[index] * config.stepSeconds;
        ++result.integratedBodyCount;
        ++result.gravityAppliedBodyCount;
        break;
      case PhysicsBodyMotionKind::Kinematic:
        store->positions_[index] =
            store->positions_[index] +
            store->velocities_[index] * config.stepSeconds;
        ++result.integratedBodyCount;
        break;
    }
  }

  return result;
}

}  // namespace iggy3d
