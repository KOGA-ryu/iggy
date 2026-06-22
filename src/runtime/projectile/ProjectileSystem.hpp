#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "core/math/Vec3.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"

namespace iggy3d {

enum class ProjectileStepStatus : std::uint8_t {
  Advanced,
  Impact,
  Expired,
  InvalidInput,
  MissingCollisionSurfaces,
};

struct ProjectileMotionParams {
  float gravityMetersPerSecondSquared = 9.8F;
  float maxLifetimeSeconds = 5.0F;
  float maxDistanceMeters = 60.0F;
  float radiusMeters = 0.05F;
};

struct ProjectileState {
  Vec3 positionMeters;
  Vec3 velocityMetersPerSecond;
  float ageSeconds = 0.0F;
  float distanceTraveledMeters = 0.0F;
  bool active = true;
};

struct ProjectileStepRequest {
  ProjectileState state;
  ProjectileMotionParams params;
  const SpatialSurfaceSet* collisionSurfaces = nullptr;
  float deltaSeconds = 0.0F;
};

struct ProjectileStepResult {
  ProjectileStepStatus status = ProjectileStepStatus::InvalidInput;
  ProjectileState previousState;
  ProjectileState state;
  bool impact = false;
  std::string hitSurfaceId;
  Vec3 impactPointMeters;
  Vec3 impactNormal;
  float timeOfImpact = 0.0F;
  float stepSeconds = 0.0F;
  float stepDistanceMeters = 0.0F;
  std::size_t checkedSurfaceCount = 0;
  std::size_t blockingSurfaceCount = 0;
  std::string reasonCode = "projectile_invalid_input";
};

std::string_view projectileStepStatusName(ProjectileStepStatus status);
ProjectileStepResult stepProjectile(const ProjectileStepRequest& request);

}  // namespace iggy3d
