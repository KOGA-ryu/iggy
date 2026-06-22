#include "content/PackageLoader.hpp"
#include "runtime/projectile/ProjectileSystem.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string_view>

namespace {

constexpr float kFeetToMeters = 0.3048F;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool approx(float lhs, float rhs, float epsilon = 0.0001F) {
  return std::fabs(lhs - rhs) <= epsilon;
}

iggy3d::SpatialSurfaceSet loadFirstRoomSurfaceSet() {
  const std::filesystem::path packagePath =
      std::filesystem::current_path() / "fixtures/demos/first_room/package.iggy3d.toml";
  const iggy3d::PackageLoadResult package =
      iggy3d::loadPackage({packagePath.generic_string()});
  if (package.status != iggy3d::PackageLoadStatus::Ok || package.rooms.empty()) {
    return {};
  }
  return iggy3d::buildSpatialSurfaceSet(package.rooms.front());
}

iggy3d::ProjectileStepRequest requestWithSurfaces(const iggy3d::SpatialSurfaceSet& surfaces) {
  iggy3d::ProjectileStepRequest request;
  request.collisionSurfaces = &surfaces;
  request.deltaSeconds = 0.5F;
  request.params.gravityMetersPerSecondSquared = 10.0F;
  request.state.positionMeters = {0.0F, 1.0F, 0.0F};
  request.state.velocityMetersPerSecond = {0.0F, 0.0F, 10.0F};
  return request;
}

bool emptySurfaceSetStillAdvancesBallistically() {
  const iggy3d::SpatialSurfaceSet surfaces;
  const iggy3d::ProjectileStepResult result =
      iggy3d::stepProjectile(requestWithSurfaces(surfaces));
  return expect(result.status == iggy3d::ProjectileStepStatus::Advanced,
                "projectile advanced") &&
         expect(!result.impact, "no impact") &&
         expect(result.state.active, "still active") &&
         expect(approx(result.state.positionMeters.x, 0.0F), "x advanced") &&
         expect(approx(result.state.positionMeters.y, -0.25F), "gravity applied") &&
         expect(approx(result.state.positionMeters.z, 5.0F), "z advanced") &&
         expect(approx(result.state.velocityMetersPerSecond.y, -5.0F),
                "vertical velocity updated") &&
         expect(result.checkedSurfaceCount == 0U, "empty checked count") &&
         expect(result.reasonCode == "projectile_advanced", "advanced reason");
}

bool projectileBlockerProducesImpact() {
  const iggy3d::SpatialSurfaceSet surfaces = loadFirstRoomSurfaceSet();
  iggy3d::ProjectileStepRequest request;
  request.collisionSurfaces = &surfaces;
  request.deltaSeconds = 1.0F;
  request.params.gravityMetersPerSecondSquared = 0.0F;
  request.state.positionMeters =
      {4.0F * kFeetToMeters, 1.0F * kFeetToMeters, 14.0F * kFeetToMeters};
  request.state.velocityMetersPerSecond = {0.0F, 0.0F, 4.0F * kFeetToMeters};

  const iggy3d::ProjectileStepResult result = iggy3d::stepProjectile(request);
  return expect(result.status == iggy3d::ProjectileStepStatus::Impact,
                "projectile impact") &&
         expect(result.impact, "impact flag") &&
         expect(!result.state.active, "inactive after impact") &&
         expect(result.hitSurfaceId == "spawn_crate_projectile_blocker",
                "projectile blocker id") &&
         expect(result.blockingSurfaceCount >= 1U, "projectile blockers checked") &&
         expect(result.stepDistanceMeters > 0.0F, "impact travel distance") &&
         expect(result.timeOfImpact > 0.0F && result.timeOfImpact <= 1.0F,
                "impact time range") &&
         expect(result.reasonCode == "projectile_impact", "impact reason");
}

bool actorOnlyBlockerDoesNotStopProjectile() {
  const iggy3d::SpatialSurfaceSet surfaces = loadFirstRoomSurfaceSet();
  iggy3d::ProjectileStepRequest request;
  request.collisionSurfaces = &surfaces;
  request.deltaSeconds = 1.0F;
  request.params.gravityMetersPerSecondSquared = 0.0F;
  request.state.positionMeters = {10.0F * kFeetToMeters, 1.0F, 2.0F};
  request.state.velocityMetersPerSecond = {0.0F, 0.0F, -3.0F};

  const iggy3d::ProjectileStepResult result = iggy3d::stepProjectile(request);
  return expect(result.status == iggy3d::ProjectileStepStatus::Advanced,
                "actor blocker ignored") &&
         expect(!result.impact, "no actor-only impact") &&
         expect(result.hitSurfaceId.empty(), "no hit surface") &&
         expect(result.state.active, "projectile remains active");
}

bool projectileExpiresByLifetimeAndDistance() {
  const iggy3d::SpatialSurfaceSet surfaces;
  iggy3d::ProjectileStepRequest lifetime = requestWithSurfaces(surfaces);
  lifetime.params.gravityMetersPerSecondSquared = 0.0F;
  lifetime.params.maxLifetimeSeconds = 0.25F;
  lifetime.deltaSeconds = 0.50F;
  const iggy3d::ProjectileStepResult expiredByLife = iggy3d::stepProjectile(lifetime);

  iggy3d::ProjectileStepRequest distance = requestWithSurfaces(surfaces);
  distance.params.gravityMetersPerSecondSquared = 0.0F;
  distance.params.maxDistanceMeters = 2.0F;
  distance.deltaSeconds = 0.50F;
  const iggy3d::ProjectileStepResult expiredByDistance = iggy3d::stepProjectile(distance);

  return expect(expiredByLife.status == iggy3d::ProjectileStepStatus::Expired,
                "lifetime expired") &&
         expect(!expiredByLife.state.active, "lifetime inactive") &&
         expect(approx(expiredByLife.state.ageSeconds, 0.25F), "lifetime clamped") &&
         expect(approx(expiredByLife.state.positionMeters.z, 2.5F), "lifetime travel") &&
         expect(expiredByDistance.status == iggy3d::ProjectileStepStatus::Expired,
                "distance expired") &&
         expect(!expiredByDistance.state.active, "distance inactive") &&
         expect(approx(expiredByDistance.state.distanceTraveledMeters, 2.0F),
                "distance clamped") &&
         expect(approx(expiredByDistance.state.positionMeters.z, 2.0F),
                "distance travel");
}

bool invalidInputsAreDiagnosed() {
  const iggy3d::SpatialSurfaceSet surfaces;
  iggy3d::ProjectileStepRequest missing = requestWithSurfaces(surfaces);
  missing.collisionSurfaces = nullptr;
  const iggy3d::ProjectileStepResult missingResult = iggy3d::stepProjectile(missing);

  iggy3d::ProjectileStepRequest invalid = requestWithSurfaces(surfaces);
  invalid.state.positionMeters.x = std::numeric_limits<float>::infinity();
  const iggy3d::ProjectileStepResult invalidResult = iggy3d::stepProjectile(invalid);

  return expect(missingResult.status == iggy3d::ProjectileStepStatus::MissingCollisionSurfaces,
                "missing collision surfaces") &&
         expect(missingResult.reasonCode == "projectile_missing_collision_surfaces",
                "missing reason") &&
         expect(invalidResult.status == iggy3d::ProjectileStepStatus::InvalidInput,
                "invalid input") &&
         expect(invalidResult.reasonCode == "projectile_invalid_input", "invalid reason");
}

}  // namespace

int main() {
  const bool ok = emptySurfaceSetStillAdvancesBallistically() &&
                  projectileBlockerProducesImpact() &&
                  actorOnlyBlockerDoesNotStopProjectile() &&
                  projectileExpiresByLifetimeAndDistance() &&
                  invalidInputsAreDiagnosed();
  return ok ? 0 : 1;
}
