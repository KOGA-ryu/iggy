#include "runtime/ability/AbilitySystem.hpp"

#include <cmath>
#include <utility>

#include "runtime/collision/EntityHitQuery.hpp"
#include "runtime/combat/CombatSystem.hpp"

namespace iggy3d {
namespace {

constexpr float kAbilityEpsilon = 0.0001F;
constexpr float kArcaneBoltMuzzleOffsetMeters = 0.75F;
constexpr float kArcaneBoltSpeedMetersPerSecond = 12.0F;
constexpr std::int32_t kArcaneBoltDamage = 3;
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
  result.impactKind = projectile.impactKind;
  result.projectileVisible = abilityProjectileVisible(projectile);
  result.projectileActive = projectile.spawned && projectile.projectile.active;
  result.projectileImpact = projectile.impact;
  result.hitEntity = isValid(projectile.hitEntity);
  result.hitEntityId = projectile.hitEntity;
  result.hitStableName = projectile.hitStableName;
  result.damageApplied = projectile.damageApplied > 0;
  result.damageAmount = projectile.damageApplied;
  result.targetDefeated = projectile.targetDefeated;
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

bool canQueryEntityHit(const AbilityTickRequest& request, const ProjectileStepResult& stepped) {
  return request.world != nullptr &&
         (stepped.status == ProjectileStepStatus::Advanced ||
          stepped.status == ProjectileStepStatus::Impact ||
          stepped.status == ProjectileStepStatus::Expired) &&
         distanceSquared(stepped.previousState.positionMeters, stepped.state.positionMeters) >
             kAbilityEpsilon * kAbilityEpsilon;
}

void applyEntityImpact(AbilityProjectileState& projectile,
                       const AbilityTickRequest& request,
                       const ProjectileStepResult& stepped,
                       const EntityHitQueryResult& hit) {
  projectile.impact = true;
  projectile.impactKind = AbilityImpactKind::Entity;
  projectile.hitEntity = hit.entity;
  projectile.hitStableName = hit.stableName;
  projectile.hitSurfaceId = "entity:" + hit.stableName;
  projectile.impactPointMeters = hit.pointMeters;
  projectile.impactNormal = hit.normal;
  projectile.reasonCode = "ability_entity_impact";
  projectile.tickStatus = AbilityTickStatus::Impact;
  projectile.projectile = stepped.previousState;
  projectile.projectile.positionMeters = hit.pointMeters;
  projectile.projectile.ageSeconds += stepped.stepSeconds * hit.timeOfImpact;
  projectile.projectile.distanceTraveledMeters += hit.distanceMeters;
  projectile.projectile.active = false;

  if (request.combat == nullptr) {
    return;
  }
  const CombatAttackResult attack =
      applyAttack(*request.combat,
                  CombatAttackRequest{projectile.caster, hit.entity, kArcaneBoltDamage,
                                      projectile.sourceCommandId});
  if (attack.status != CombatStatus::Succeeded) {
    return;
  }
  projectile.damageApplied = attack.damageApplied;
  projectile.targetDefeated = attack.targetDefeated;
}

AbilityTickResult makeTickResult(const AbilityProjectileState& projectile) {
  AbilityTickResult result;
  result.status = projectile.tickStatus;
  result.impactKind = projectile.impactKind;
  result.projectileVisible = abilityProjectileVisible(projectile);
  result.projectileActive = projectile.projectile.active;
  result.projectileImpact = projectile.impact;
  result.hitEntity = isValid(projectile.hitEntity);
  result.hitEntityId = projectile.hitEntity;
  result.hitStableName = projectile.hitStableName;
  result.damageApplied = projectile.damageApplied > 0;
  result.damageAmount = projectile.damageApplied;
  result.targetDefeated = projectile.targetDefeated;
  result.reasonCode = projectile.reasonCode;
  return result;
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

std::string_view abilityImpactKindName(AbilityImpactKind kind) {
  switch (kind) {
    case AbilityImpactKind::None:
      return "none";
    case AbilityImpactKind::Entity:
      return "entity";
    case AbilityImpactKind::Surface:
      return "surface";
    case AbilityImpactKind::Expired:
      return "expired";
  }
  return "none";
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
  projectile.hitStableName = "none";
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

  if (canQueryEntityHit(request, stepped)) {
    EntityHitQueryRequest hitRequest;
    hitRequest.world = request.world;
    hitRequest.startMeters = stepped.previousState.positionMeters;
    hitRequest.endMeters = stepped.state.positionMeters;
    hitRequest.ignoredEntity = projectile.caster;
    hitRequest.radiusMeters = kArcaneBoltMotion.radiusMeters;
    const EntityHitQueryResult hit = queryFirstEntityHit(hitRequest);
    if (hit.status == EntityHitStatus::Hit) {
      applyEntityImpact(projectile, request, stepped, hit);
      return makeTickResult(projectile);
    }
  }

  if (stepped.impact) {
    projectile.impact = true;
    projectile.impactKind = AbilityImpactKind::Surface;
    projectile.impactPointMeters = stepped.impactPointMeters;
    projectile.impactNormal = stepped.impactNormal;
    projectile.hitSurfaceId =
        stepped.hitSurfaceId.empty() ? std::string("none") : stepped.hitSurfaceId;
  } else if (stepped.status == ProjectileStepStatus::Expired) {
    projectile.impactKind = AbilityImpactKind::Expired;
  }

  return makeTickResult(projectile);
}

void resetAbilityRuntime(AbilityRuntimeState& state) {
  state = AbilityRuntimeState{};
}

bool abilityProjectileVisible(const AbilityProjectileState& projectile) {
  return projectile.spawned;
}

}  // namespace iggy3d
