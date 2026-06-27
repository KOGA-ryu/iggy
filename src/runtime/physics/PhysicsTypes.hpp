#pragma once

#include <cstdint>
#include <string_view>

#include "core/math/Vec3.hpp"

namespace iggy3d {

struct PhysicsBodyId {
  std::uint32_t value = 0U;
};

struct PhysicsShapeId {
  std::uint32_t value = 0U;
};

enum class PhysicsBodyMotionKind : std::uint8_t {
  Static,
  Dynamic,
  Kinematic,
};

enum class PhysicsShapeKind : std::uint8_t {
  Box,
  Capsule,
  FloorSpan,
  WallSlab,
  TriggerAabb,
};

enum class PhysicsStatus : std::uint8_t {
  Valid,
  MissingDescriptor,
  InvalidMotionKind,
  InvalidPosition,
  InvalidVelocity,
  InvalidMass,
  BodyAdded,
  BodyNotFound,
  BodyRemoved,
  StoreReset,
};

struct PhysicsBodyDescriptor {
  PhysicsBodyMotionKind motion = PhysicsBodyMotionKind::Static;
  Vec3 positionMeters;
  Vec3 velocityMetersPerSecond;
  float massKilograms = 0.0F;
};

struct PhysicsValidationResult {
  bool ok = false;
  PhysicsStatus status = PhysicsStatus::MissingDescriptor;
  std::string_view reasonCode = "physics_body_missing_descriptor";
};

std::string_view physicsBodyMotionKindName(PhysicsBodyMotionKind kind);
std::string_view physicsShapeKindName(PhysicsShapeKind kind);
std::string_view physicsStatusName(PhysicsStatus status);

bool isValidPhysicsBodyId(PhysicsBodyId id);
bool isValidPhysicsShapeId(PhysicsShapeId id);
float computePhysicsInverseMass(PhysicsBodyMotionKind motion,
                                float massKilograms);
PhysicsValidationResult validatePhysicsBodyDescriptor(
    const PhysicsBodyDescriptor* descriptor);

}  // namespace iggy3d
