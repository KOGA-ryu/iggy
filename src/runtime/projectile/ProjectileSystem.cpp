#include "runtime/projectile/ProjectileSystem.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

#include "runtime/collision/CollisionQuery.hpp"

namespace iggy3d {
namespace {

constexpr float kProjectileEpsilon = 0.0001F;

float vectorLength(Vec3 value) {
  return std::sqrt(lengthSquared(value));
}

bool finiteNonNegative(float value) {
  return std::isfinite(value) && value >= 0.0F;
}

bool validParams(const ProjectileMotionParams& params) {
  return finiteNonNegative(params.gravityMetersPerSecondSquared) &&
         std::isfinite(params.maxLifetimeSeconds) && params.maxLifetimeSeconds > 0.0F &&
         std::isfinite(params.maxDistanceMeters) && params.maxDistanceMeters > 0.0F &&
         std::isfinite(params.radiusMeters) && params.radiusMeters > 0.0F;
}

bool validState(const ProjectileState& state) {
  return isFinite(state.positionMeters) && isFinite(state.velocityMetersPerSecond) &&
         finiteNonNegative(state.ageSeconds) && finiteNonNegative(state.distanceTraveledMeters);
}

ProjectileStepResult baseResult(const ProjectileStepRequest& request) {
  ProjectileStepResult result;
  result.previousState = request.state;
  result.state = request.state;
  return result;
}

ProjectileStepResult invalidResult(const ProjectileStepRequest& request) {
  ProjectileStepResult result = baseResult(request);
  result.status = ProjectileStepStatus::InvalidInput;
  result.reasonCode = "projectile_invalid_input";
  return result;
}

ProjectileStepResult missingSurfacesResult(const ProjectileStepRequest& request) {
  ProjectileStepResult result = baseResult(request);
  result.status = ProjectileStepStatus::MissingCollisionSurfaces;
  result.reasonCode = "projectile_missing_collision_surfaces";
  return result;
}

ProjectileStepResult expiredResult(const ProjectileStepRequest& request,
                                   std::string reason = "projectile_expired") {
  ProjectileStepResult result = baseResult(request);
  result.status = ProjectileStepStatus::Expired;
  result.state.active = false;
  result.reasonCode = std::move(reason);
  return result;
}

ProjectileState integrate(ProjectileState state,
                          Vec3 accelerationMetersPerSecondSquared,
                          float seconds) {
  state.positionMeters =
      state.positionMeters + state.velocityMetersPerSecond * seconds +
      accelerationMetersPerSecondSquared * (0.5F * seconds * seconds);
  state.velocityMetersPerSecond =
      state.velocityMetersPerSecond + accelerationMetersPerSecondSquared * seconds;
  state.ageSeconds += seconds;
  return state;
}

float clampTravelFraction(float remainingDistanceMeters, float stepDistanceMeters) {
  if (stepDistanceMeters <= kProjectileEpsilon) {
    return 1.0F;
  }
  return std::clamp(remainingDistanceMeters / stepDistanceMeters, 0.0F, 1.0F);
}

}  // namespace

std::string_view projectileStepStatusName(ProjectileStepStatus status) {
  switch (status) {
    case ProjectileStepStatus::Advanced:
      return "advanced";
    case ProjectileStepStatus::Impact:
      return "impact";
    case ProjectileStepStatus::Expired:
      return "expired";
    case ProjectileStepStatus::InvalidInput:
      return "invalid_input";
    case ProjectileStepStatus::MissingCollisionSurfaces:
      return "missing_collision_surfaces";
  }
  return "invalid_input";
}

ProjectileStepResult stepProjectile(const ProjectileStepRequest& request) {
  if (!validState(request.state) || !validParams(request.params) ||
      !std::isfinite(request.deltaSeconds) || request.deltaSeconds < 0.0F) {
    return invalidResult(request);
  }
  if (request.collisionSurfaces == nullptr) {
    return missingSurfacesResult(request);
  }
  if (!request.state.active) {
    return expiredResult(request, "projectile_inactive");
  }
  if (request.state.ageSeconds >= request.params.maxLifetimeSeconds ||
      request.state.distanceTraveledMeters >= request.params.maxDistanceMeters) {
    return expiredResult(request);
  }

  ProjectileStepResult result = baseResult(request);
  const float remainingLifetime =
      request.params.maxLifetimeSeconds - request.state.ageSeconds;
  const float stepSeconds = std::min(request.deltaSeconds, remainingLifetime);
  result.stepSeconds = stepSeconds;
  if (stepSeconds <= 0.0F) {
    result.status = ProjectileStepStatus::Advanced;
    result.reasonCode = "projectile_no_time";
    return result;
  }

  const Vec3 acceleration{0.0F, -request.params.gravityMetersPerSecondSquared, 0.0F};
  const ProjectileState integrated = integrate(request.state, acceleration, stepSeconds);
  const Vec3 start = request.state.positionMeters;
  Vec3 end = integrated.positionMeters;
  float pathDistance = vectorLength(end - start);

  const float remainingDistance =
      request.params.maxDistanceMeters - request.state.distanceTraveledMeters;
  float distanceFraction = 1.0F;
  if (pathDistance > remainingDistance + kProjectileEpsilon) {
    distanceFraction = clampTravelFraction(remainingDistance, pathDistance);
    end = start + (end - start) * distanceFraction;
    pathDistance = remainingDistance;
  }

  if (pathDistance <= kProjectileEpsilon) {
    result.status = distanceFraction < 1.0F ? ProjectileStepStatus::Expired
                                            : ProjectileStepStatus::Advanced;
    result.state.positionMeters = end;
    result.state.ageSeconds += stepSeconds * distanceFraction;
    result.state.distanceTraveledMeters += pathDistance;
    result.state.active = result.status != ProjectileStepStatus::Expired;
    result.reasonCode = result.status == ProjectileStepStatus::Expired ? "projectile_expired"
                                                                       : "projectile_no_motion";
    return result;
  }

  const CollisionQueryResult collision =
      querySegment(*request.collisionSurfaces, start, end, CollisionQueryKind::Projectile);
  result.checkedSurfaceCount = collision.checkedSurfaceCount;
  result.blockingSurfaceCount = collision.blockingSurfaceCount;

  if (collision.status == CollisionQueryStatus::Hit) {
    const float impactSeconds = stepSeconds * std::clamp(collision.timeOfImpact, 0.0F, 1.0F) *
                                distanceFraction;
    result.status = ProjectileStepStatus::Impact;
    result.impact = true;
    result.hitSurfaceId = collision.surfaceId;
    result.impactPointMeters = collision.pointMeters;
    result.impactNormal = collision.normal;
    result.timeOfImpact = collision.timeOfImpact;
    result.stepSeconds = impactSeconds;
    result.stepDistanceMeters = collision.distanceMeters;
    result.state.positionMeters = collision.pointMeters;
    result.state.velocityMetersPerSecond =
        request.state.velocityMetersPerSecond + acceleration * impactSeconds;
    result.state.ageSeconds += impactSeconds;
    result.state.distanceTraveledMeters += collision.distanceMeters;
    result.state.active = false;
    result.reasonCode = "projectile_impact";
    return result;
  }
  if (collision.status == CollisionQueryStatus::InvalidInput) {
    return invalidResult(request);
  }

  result.status = ProjectileStepStatus::Advanced;
  result.state = integrated;
  result.state.positionMeters = end;
  result.state.ageSeconds =
      request.state.ageSeconds + stepSeconds * distanceFraction;
  result.state.velocityMetersPerSecond =
      request.state.velocityMetersPerSecond + acceleration * (stepSeconds * distanceFraction);
  result.state.distanceTraveledMeters += pathDistance;
  result.stepDistanceMeters = pathDistance;
  result.reasonCode = "projectile_advanced";
  if (distanceFraction < 1.0F ||
      result.state.ageSeconds >= request.params.maxLifetimeSeconds - kProjectileEpsilon ||
      result.state.distanceTraveledMeters >= request.params.maxDistanceMeters - kProjectileEpsilon) {
    result.status = ProjectileStepStatus::Expired;
    result.state.active = false;
    result.reasonCode = "projectile_expired";
  }
  return result;
}

}  // namespace iggy3d
