#include "runtime/physics/PhysicsTypes.hpp"

#include <array>
#include <cmath>

namespace iggy3d {
namespace {

template <std::size_t Count, typename Enum>
std::string_view enumName(Enum value,
                          const std::array<std::string_view, Count>& names,
                          std::string_view fallback) {
  const auto index = static_cast<std::size_t>(value);
  // branch-gate: BG-1084
  if (index >= names.size()) {
    return fallback;
  }
  return names[index];
}

bool validMotionKind(PhysicsBodyMotionKind motion) {
  const auto index = static_cast<std::size_t>(motion);
  return index < 3U;
}

bool finiteNonNegative(float value) {
  return std::isfinite(value) && value >= 0.0F;
}

bool finitePositive(float value) {
  return std::isfinite(value) && value > 0.0F;
}

PhysicsValidationResult validationResult(PhysicsStatus status, bool ok) {
  PhysicsValidationResult result;
  result.ok = ok;
  result.status = status;
  result.reasonCode = physicsStatusName(status);
  return result;
}

}  // namespace

std::string_view physicsBodyMotionKindName(PhysicsBodyMotionKind kind) {
  static constexpr std::array<std::string_view, 3> kNames{
      "static",
      "dynamic",
      "kinematic",
  };
  return enumName(kind, kNames, "unknown");
}

std::string_view physicsShapeKindName(PhysicsShapeKind kind) {
  static constexpr std::array<std::string_view, 5> kNames{
      "box",
      "capsule",
      "floor_span",
      "wall_slab",
      "trigger_aabb",
  };
  return enumName(kind, kNames, "unknown");
}

std::string_view physicsStatusName(PhysicsStatus status) {
  static constexpr std::array<std::string_view, 10> kNames{
      "physics_body_valid",
      "physics_body_missing_descriptor",
      "physics_body_invalid_motion_kind",
      "physics_body_invalid_position",
      "physics_body_invalid_velocity",
      "physics_body_invalid_mass",
      "physics_body_added",
      "physics_body_not_found",
      "physics_body_removed",
      "physics_body_store_reset",
  };
  return enumName(status, kNames, "physics_body_invalid_motion_kind");
}

bool isValidPhysicsBodyId(PhysicsBodyId id) {
  return id.value != 0U;
}

bool isValidPhysicsShapeId(PhysicsShapeId id) {
  return id.value != 0U;
}

float computePhysicsInverseMass(PhysicsBodyMotionKind motion,
                                float massKilograms) {
  // branch-gate: BG-1084
  if (motion != PhysicsBodyMotionKind::Dynamic) {
    return 0.0F;
  }
  // branch-gate: BG-1084
  if (!finitePositive(massKilograms)) {
    return 0.0F;
  }
  return 1.0F / massKilograms;
}

PhysicsValidationResult validatePhysicsBodyDescriptor(
    const PhysicsBodyDescriptor* descriptor) {
  // branch-gate: BG-1084
  if (descriptor == nullptr) {
    return validationResult(PhysicsStatus::MissingDescriptor, false);
  }
  // branch-gate: BG-1084
  if (!validMotionKind(descriptor->motion)) {
    return validationResult(PhysicsStatus::InvalidMotionKind, false);
  }
  // branch-gate: BG-1084
  if (!isFinite(descriptor->positionMeters)) {
    return validationResult(PhysicsStatus::InvalidPosition, false);
  }
  // branch-gate: BG-1084
  if (!isFinite(descriptor->velocityMetersPerSecond)) {
    return validationResult(PhysicsStatus::InvalidVelocity, false);
  }
  const bool dynamicBody = descriptor->motion == PhysicsBodyMotionKind::Dynamic;
  // branch-gate: BG-1084
  if (dynamicBody && !finitePositive(descriptor->massKilograms)) {
    return validationResult(PhysicsStatus::InvalidMass, false);
  }
  // branch-gate: BG-1084
  if (!dynamicBody && !finiteNonNegative(descriptor->massKilograms)) {
    return validationResult(PhysicsStatus::InvalidMass, false);
  }
  return validationResult(PhysicsStatus::Valid, true);
}

}  // namespace iggy3d
