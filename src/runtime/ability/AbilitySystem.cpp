#include "runtime/ability/AbilitySystem.hpp"

#include <cmath>
#include <utility>

#include "runtime/collision/EntityHitQuery.hpp"
#include "runtime/combat/CombatSystem.hpp"

namespace iggy3d {
namespace {

constexpr float kAbilityEpsilon = 0.0001F;
constexpr AbilityDefinition kArcaneBoltDefinition{
    AbilityId::ArcaneBolt,
    "arcane_bolt",
    "arcane_bolt_projectile",
    0.75F,
    12.0F,
    ProjectileMotionParams{
        1.50F,
        3.0F,
        45.0F,
        0.07F,
    },
    3,
    1,
    3,
    8,
    20,
};

float vectorLength(Vec3 value) {
  return std::sqrt(lengthSquared(value));
}

AbilityCastResult rejected(AbilityId ability,
                           AbilityCastStatus status,
                           std::string reasonCode,
                           const AbilityDefinition* definition = nullptr) {
  AbilityCastResult result;
  result.ability = ability;
  result.status = status;
  if (definition != nullptr) {
    result.resourceCost = definition->resourceCost;
  }
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

const AbilityProjectileState& projectileSlot(const AbilityRuntimeState& state, AbilityId ability) {
  switch (ability) {
    case AbilityId::ArcaneBolt:
      return state.arcaneBolt;
    case AbilityId::None:
      break;
  }
  return state.arcaneBolt;
}

const AbilityActorState* findActorState(const AbilityState& state, EntityId actor) {
  for (const AbilityActorState& candidate : state.actors) {
    if (candidate.actor == actor) {
      return &candidate;
    }
  }
  return nullptr;
}

AbilityActorState defaultActorState(EntityId actor, const AbilityDefinition& definition) {
  AbilityActorState state;
  state.actor = actor;
  state.arcaneFocus = definition.maxResource;
  return state;
}

std::uint32_t rechargeActorResource(AbilityActorState& actor,
                                    const AbilityDefinition& definition,
                                    CommandTick currentTick) {
  if (definition.resourceRechargeTicks == 0U) {
    return 0;
  }
  if (actor.arcaneFocus >= definition.maxResource) {
    actor.arcaneFocus = definition.maxResource;
    actor.arcaneFocusNextRechargeTick = 0;
    return 0;
  }
  if (actor.arcaneFocusNextRechargeTick == 0U) {
    actor.arcaneFocusNextRechargeTick = currentTick + definition.resourceRechargeTicks;
    return 0;
  }

  std::uint32_t recovered = 0;
  while (actor.arcaneFocus < definition.maxResource &&
         currentTick >= actor.arcaneFocusNextRechargeTick) {
    ++actor.arcaneFocus;
    ++recovered;
    if (actor.arcaneFocus < definition.maxResource) {
      actor.arcaneFocusNextRechargeTick += definition.resourceRechargeTicks;
    } else {
      actor.arcaneFocusNextRechargeTick = 0;
    }
  }
  return recovered;
}

AbilityActorState rechargedActorState(AbilityActorState actor,
                                      const AbilityDefinition& definition,
                                      CommandTick currentTick) {
  static_cast<void>(rechargeActorResource(actor, definition, currentTick));
  return actor;
}

void scheduleResourceRechargeAfterSpend(AbilityActorState& actor,
                                        const AbilityDefinition& definition,
                                        CommandTick currentTick) {
  if (definition.resourceRechargeTicks == 0U ||
      actor.arcaneFocus >= definition.maxResource ||
      actor.arcaneFocusNextRechargeTick != 0U) {
    return;
  }
  actor.arcaneFocusNextRechargeTick = currentTick + definition.resourceRechargeTicks;
}

AbilityActorState actorStateForRead(const AbilityState& state,
                                    EntityId actor,
                                    const AbilityDefinition& definition) {
  const AbilityActorState* existing = findActorState(state, actor);
  return existing == nullptr ? defaultActorState(actor, definition) : *existing;
}

AbilityActorState& actorStateForWrite(AbilityState& state,
                                      EntityId actor,
                                      const AbilityDefinition& definition) {
  for (AbilityActorState& candidate : state.actors) {
    if (candidate.actor == actor) {
      return candidate;
    }
  }
  state.actors.push_back(defaultActorState(actor, definition));
  return state.actors.back();
}

CommandTick cooldownReadyTick(const AbilityActorState& state, AbilityId ability) {
  switch (ability) {
    case AbilityId::ArcaneBolt:
      return state.arcaneBoltReadyTick;
    case AbilityId::None:
      break;
  }
  return 0;
}

void setCooldownReadyTick(AbilityActorState& state, AbilityId ability, CommandTick readyTick) {
  switch (ability) {
    case AbilityId::ArcaneBolt:
      state.arcaneBoltReadyTick = readyTick;
      return;
    case AbilityId::None:
      break;
  }
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
                  CombatAttackRequest{projectile.caster, hit.entity, kArcaneBoltDefinition.damage,
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
    case AbilityCastStatus::OnCooldown:
      return "on_cooldown";
    case AbilityCastStatus::InsufficientResource:
      return "insufficient_resource";
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

const AbilityDefinition* findAbilityDefinition(AbilityId ability) {
  switch (ability) {
    case AbilityId::ArcaneBolt:
      return &kArcaneBoltDefinition;
    case AbilityId::None:
      break;
  }
  return nullptr;
}

AbilityCastResult inspectAbilityCast(const AbilityState& abilityState,
                                     const AbilityRuntimeState& runtimeState,
                                     const AbilityCastRequest& request) {
  const AbilityDefinition* definition = findAbilityDefinition(request.ability);
  if (definition == nullptr) {
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
                    "ability_missing_collision_surfaces", definition);
  }

  const AbilityProjectileState& projectile = projectileSlot(runtimeState, request.ability);
  if (projectile.spawned && projectile.projectile.active) {
    return rejected(request.ability, AbilityCastStatus::ProjectileSlotBusy,
                    "ability_projectile_slot_busy", definition);
  }

  const AbilityActorState actorState =
      rechargedActorState(actorStateForRead(abilityState, request.caster, *definition),
                          *definition, request.currentTick);
  const CommandTick readyTick = cooldownReadyTick(actorState, request.ability);
  if (request.currentTick < readyTick) {
    AbilityCastResult result =
        rejected(request.ability, AbilityCastStatus::OnCooldown,
                 "ability_on_cooldown", definition);
    result.cooldownReadyTick = readyTick;
    result.resourceRemaining = actorState.arcaneFocus;
    return result;
  }
  if (actorState.arcaneFocus < definition->resourceCost) {
    AbilityCastResult result =
        rejected(request.ability, AbilityCastStatus::InsufficientResource,
                 "ability_insufficient_resource", definition);
    result.cooldownReadyTick = readyTick;
    result.resourceRemaining = actorState.arcaneFocus;
    return result;
  }

  AbilityCastResult result;
  result.status = AbilityCastStatus::Accepted;
  result.ability = request.ability;
  result.accepted = true;
  result.resourceRemaining = actorState.arcaneFocus;
  result.resourceCost = definition->resourceCost;
  result.cooldownReadyTick = readyTick;
  result.reasonCode = "ability_cast_available";
  return result;
}

AbilityCastResult castAbility(AbilityState& abilityState,
                              AbilityRuntimeState& runtimeState,
                              const AbilityCastRequest& request) {
  AbilityCastResult available = inspectAbilityCast(abilityState, runtimeState, request);
  if (!available.accepted) {
    return available;
  }

  const AbilityDefinition* definition = findAbilityDefinition(request.ability);
  if (definition == nullptr) {
    return rejected(request.ability, AbilityCastStatus::InvalidAbility,
                    "ability_invalid_ability");
  }

  AbilityActorState& actorState =
      actorStateForWrite(abilityState, request.caster, *definition);
  static_cast<void>(rechargeActorResource(actorState, *definition, request.currentTick));
  actorState.arcaneFocus -= definition->resourceCost;
  scheduleResourceRechargeAfterSpend(actorState, *definition, request.currentTick);
  setCooldownReadyTick(actorState, request.ability,
                       request.currentTick + definition->cooldownTicks);

  const Vec3 direction = normalized(request.direction);
  AbilityProjectileState& projectile = projectileSlot(runtimeState, request.ability);
  projectile = AbilityProjectileState{};
  projectile.ability = request.ability;
  projectile.spawned = true;
  projectile.impact = false;
  projectile.caster = request.caster;
  projectile.sourceCommandId = request.sourceCommandId;
  projectile.projectile.active = true;
  projectile.projectile.positionMeters =
      request.originMeters + direction * definition->muzzleOffsetMeters;
  projectile.projectile.velocityMetersPerSecond =
      direction * definition->projectileSpeedMetersPerSecond;
  projectile.previousPositionMeters = projectile.projectile.positionMeters;
  projectile.impactPointMeters = projectile.projectile.positionMeters;
  projectile.projectileId = std::string(definition->projectileId);
  projectile.hitSurfaceId = "none";
  projectile.hitStableName = "none";
  projectile.reasonCode = "ability_cast_accepted";
  projectile.tickStatus = AbilityTickStatus::Advanced;

  AbilityCastResult result;
  result.status = AbilityCastStatus::Accepted;
  result.ability = request.ability;
  result.accepted = true;
  result.projectileSpawned = true;
  result.resourceRemaining = actorState.arcaneFocus;
  result.resourceCost = definition->resourceCost;
  result.cooldownReadyTick = cooldownReadyTick(actorState, request.ability);
  result.reasonCode = "ability_cast_accepted";
  return result;
}

AbilityRechargeResult tickAbilityState(AbilityState& abilityState, CommandTick currentTick) {
  AbilityRechargeResult result;
  for (AbilityActorState& actor : abilityState.actors) {
    const std::uint32_t recovered =
        rechargeActorResource(actor, kArcaneBoltDefinition, currentTick);
    if (recovered > 0U) {
      ++result.actorsUpdated;
      result.resourceRecovered += recovered;
    }
  }
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
  projectileRequest.params = kArcaneBoltDefinition.projectileMotion;
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
    hitRequest.radiusMeters = kArcaneBoltDefinition.projectileMotion.radiusMeters;
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

bool abilityRuntimeHasActiveProjectile(const AbilityRuntimeState& state) {
  return state.arcaneBolt.spawned && state.arcaneBolt.projectile.active;
}

bool abilityStateHasPendingRecharge(const AbilityState& state) {
  for (const AbilityActorState& actor : state.actors) {
    if (actor.arcaneFocus < kArcaneBoltDefinition.maxResource &&
        actor.arcaneFocusNextRechargeTick != 0U) {
      return true;
    }
  }
  return false;
}

}  // namespace iggy3d
