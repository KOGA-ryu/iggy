#include "runtime/ability/AbilitySystem.hpp"

#include <cmath>
#include <utility>

namespace iggy3d {
namespace {

constexpr float kAbilityEpsilon = 0.0001F;
constexpr float kArcaneBoltMuzzleOffsetMeters = 0.75F;
constexpr float kArcaneBoltSpeedMetersPerSecond = 12.0F;
constexpr ProjectileMotionParams kArcaneBoltMotion{
    1.50F,
    3.0F,
    45.0F,
    0.07F,
};

float vectorLength(Vec3 value) {
  return std::sqrt(lengthSquared(value));
}

Vec3 normalized(Vec3 value) {
  return value / vectorLength(value);
}

AbilityCastResult rejected(AbilityId ability,
                           AbilityCastStatus status,
                           std::string reasonCode) {
  AbilityCastResult result;
  result.ability = ability;
  result.status = status;
  result.reasonCode = std::move(reasonCode);
  return result;
}

AbilityTickStatus mapProjectileStatus(ProjectileStepStatus status) {
  switch (status) {
    case ProjectileStepStatus::Advanced:
      return AbilityTickStatus::Advanced;
    case ProjectileStepStatus::Impact:
      return AbilityTickStatus::Impact;
    case ProjectileStepStatus::Expired:
      return AbilityTickStatus::Expired;
    case ProjectileStepStatus::InvalidInput:
      return AbilityTickStatus::InvalidInput;
    case ProjectileStepStatus::MissingCollisionSurfaces:
      return AbilityTickStatus::MissingCollisionSurfaces;
  }
  return AbilityTickStatus::InvalidInput;
}

AbilityTickResult tickNoActive(const AbilityProjectileState& projectile) {
  AbilityTickResult result;
  result.projectileVisible = abilityProjectileVisible(projectile);
  result.projectileActive = projectile.spawned && projectile.projectile.active;
  result.projectileImpact = projectile.impact;
  return result;
}

AbilityProjectileState& projectileSlot(AbilityRuntimeState& state, AbilityId ability) {
  switch (ability) {
    case AbilityId::ArcaneBolt:
      return state.arcaneBolt;
    case AbilityId::None:
      break;
  }
  return state.arcaneBolt;
}

}  // namespace

std::string_view abilityIdName(AbilityId ability) {
  switch (ability) {
    case AbilityId::None:
      return "none";
    case AbilityId::ArcaneBolt:
      return "arcane_bolt";
  }
  return "none";
}

std::string_view abilityCastStatusName(AbilityCastStatus status) {
  switch (status) {
    case AbilityCastStatus::Accepted:
      return "accepted";
    case AbilityCastStatus::InvalidAbility:
      return "invalid_ability";
    case AbilityCastStatus::InvalidCaster:
      return "invalid_caster";
    case AbilityCastStatus::InvalidOrigin:
      return "invalid_origin";
    case AbilityCastStatus::InvalidDirection:
      return "invalid_direction";
    case AbilityCastStatus::MissingCollisionSurfaces:
      return "missing_collision_surfaces";
    case AbilityCastStatus::ProjectileSlotBusy:
      return "projectile_slot_busy";
  }
  return "invalid_ability";
}

std::string_view abilityTickStatusName(AbilityTickStatus status) {
  switch (status) {
    case AbilityTickStatus::NoActiveProjectile:
      return "no_active_projectile";
    case AbilityTickStatus::Advanced:
      return "advanced";
    case AbilityTickStatus::Impact:
      return "impact";
    case AbilityTickStatus::Expired:
      return "expired";
    case AbilityTickStatus::InvalidInput:
      return "invalid_input";
    case AbilityTickStatus::MissingCollisionSurfaces:
      return "missing_collision_surfaces";
  }
  return "invalid_input";
}

AbilityCastResult castAbility(AbilityRuntimeState& state,
                              const AbilityCastRequest& request) {
  if (request.ability != AbilityId::ArcaneBolt) {
    return rejected(request.ability, AbilityCastStatus::InvalidAbility,
                    "ability_invalid_ability");
  }
  if (!isValid(request.caster)) {
    return rejected(request.ability, AbilityCastStatus::InvalidCaster,
                    "ability_invalid_caster");
  }
  if (!isFinite(request.originMeters)) {
    return rejected(request.ability, AbilityCastStatus::InvalidOrigin,
                    "ability_invalid_origin");
  }
  if (!isFinite(request.direction) || vectorLength(request.direction) <= kAbilityEpsilon) {
    return rejected(request.ability, AbilityCastStatus::InvalidDirection,
                    "ability_invalid_direction");
  }
  if (request.collisionSurfaces == nullptr) {
    return rejected(request.ability, AbilityCastStatus::MissingCollisionSurfaces,
                    "ability_missing_collision_surfaces");
  }

  AbilityProjectileState& projectile = projectileSlot(state, request.ability);
  if (projectile.spawned && projectile.projectile.active) {
    return rejected(request.ability, AbilityCastStatus::ProjectileSlotBusy,
                    "ability_projectile_slot_busy");
  }

  const Vec3 direction = normalized(request.direction);
  projectile = AbilityProjectileState{};
  projectile.ability = request.ability;
  projectile.spawned = true;
  projectile.impact = false;
  projectile.caster = request.caster;
  projectile.sourceCommandId = request.sourceCommandId;
  projectile.projectile.active = true;
  projectile.projectile.positionMeters =
      request.originMeters + direction * kArcaneBoltMuzzleOffsetMeters;
  projectile.projectile.velocityMetersPerSecond =
      direction * kArcaneBoltSpeedMetersPerSecond;
  projectile.previousPositionMeters = projectile.projectile.positionMeters;
  projectile.impactPointMeters = projectile.projectile.positionMeters;
  projectile.projectileId = "arcane_bolt_projectile";
  projectile.hitSurfaceId = "none";
  projectile.reasonCode = "ability_cast_accepted";
  projectile.tickStatus = AbilityTickStatus::Advanced;

  AbilityCastResult result;
  result.status = AbilityCastStatus::Accepted;
  result.ability = request.ability;
  result.accepted = true;
  result.projectileSpawned = true;
  result.reasonCode = "ability_cast_accepted";
  return result;
}

AbilityTickResult tickAbilityRuntime(AbilityRuntimeState& state,
                                     const AbilityTickRequest& request) {
  AbilityProjectileState& projectile = state.arcaneBolt;
  if (!projectile.spawned || !projectile.projectile.active) {
    return tickNoActive(projectile);
  }

  ProjectileStepRequest projectileRequest;
  projectileRequest.state = projectile.projectile;
  projectileRequest.params = kArcaneBoltMotion;
  projectileRequest.collisionSurfaces = request.collisionSurfaces;
  projectileRequest.deltaSeconds = request.deltaSeconds;

  const ProjectileStepResult stepped = stepProjectile(projectileRequest);
  projectile.previousPositionMeters = stepped.previousState.positionMeters;
  projectile.projectile = stepped.state;
  projectile.tickStatus = mapProjectileStatus(stepped.status);
  projectile.reasonCode = stepped.reasonCode;
  if (stepped.impact) {
    projectile.impact = true;
    projectile.impactPointMeters = stepped.impactPointMeters;
    projectile.impactNormal = stepped.impactNormal;
    projectile.hitSurfaceId =
        stepped.hitSurfaceId.empty() ? std::string("none") : stepped.hitSurfaceId;
  }

  AbilityTickResult result;
  result.status = projectile.tickStatus;
  result.projectileVisible = abilityProjectileVisible(projectile);
  result.projectileActive = projectile.projectile.active;
  result.projectileImpact = projectile.impact;
  result.reasonCode = projectile.reasonCode;
  return result;
}

void resetAbilityRuntime(AbilityRuntimeState& state) {
  state = AbilityRuntimeState{};
}

bool abilityProjectileVisible(const AbilityProjectileState& projectile) {
  return projectile.spawned;
}

}  // namespace iggy3d
