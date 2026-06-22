#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "core/ids/EntityId.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/projectile/ProjectileSystem.hpp"

namespace iggy3d {

struct CombatState;
class WorldState;

enum class AbilityId : std::uint8_t {
  None,
  ArcaneBolt,
};

enum class AbilityCastStatus : std::uint8_t {
  Accepted,
  InvalidAbility,
  InvalidCaster,
  InvalidOrigin,
  InvalidDirection,
  MissingCollisionSurfaces,
  ProjectileSlotBusy,
  OnCooldown,
  InsufficientResource,
};

enum class AbilityTickStatus : std::uint8_t {
  NoActiveProjectile,
  Advanced,
  Impact,
  Expired,
  InvalidInput,
  MissingCollisionSurfaces,
};

enum class AbilityImpactKind : std::uint8_t {
  None,
  Entity,
  Surface,
  Expired,
};

struct AbilityProjectileState {
  AbilityId ability = AbilityId::None;
  bool spawned = false;
  bool impact = false;
  EntityId caster;
  EntityId hitEntity;
  CommandId sourceCommandId = kInvalidCommandId;
  ProjectileState projectile{{}, {}, 0.0F, 0.0F, false};
  Vec3 previousPositionMeters;
  Vec3 impactPointMeters;
  Vec3 impactNormal;
  std::string projectileId = "none";
  std::string hitSurfaceId = "none";
  std::string hitStableName = "none";
  std::string reasonCode = "not_started";
  AbilityImpactKind impactKind = AbilityImpactKind::None;
  AbilityTickStatus tickStatus = AbilityTickStatus::NoActiveProjectile;
  std::int32_t damageApplied = 0;
  bool targetDefeated = false;
};

struct AbilityActorState {
  EntityId actor;
  std::uint32_t arcaneFocus = 3;
  CommandTick arcaneBoltReadyTick = 0;
  CommandTick arcaneFocusNextRechargeTick = 0;
};

struct AbilityState {
  std::vector<AbilityActorState> actors;
};

struct AbilityRuntimeState {
  AbilityProjectileState arcaneBolt;
};

struct AbilityDefinition {
  AbilityId ability = AbilityId::None;
  std::string_view abilityName = "none";
  std::string_view projectileId = "none";
  float muzzleOffsetMeters = 0.0F;
  float projectileSpeedMetersPerSecond = 0.0F;
  ProjectileMotionParams projectileMotion{};
  std::int32_t damage = 0;
  std::uint32_t resourceCost = 0;
  std::uint32_t maxResource = 0;
  CommandTick cooldownTicks = 0;
  CommandTick resourceRechargeTicks = 0;
};

struct AbilityRechargeResult {
  std::uint32_t actorsUpdated = 0;
  std::uint32_t resourceRecovered = 0;
};

struct AbilityCastRequest {
  AbilityId ability = AbilityId::ArcaneBolt;
  EntityId caster;
  Vec3 originMeters;
  Vec3 direction;
  const SpatialSurfaceSet* collisionSurfaces = nullptr;
  CommandId sourceCommandId = kInvalidCommandId;
  CommandTick currentTick = 0;
};

struct AbilityCastResult {
  AbilityCastStatus status = AbilityCastStatus::InvalidAbility;
  AbilityId ability = AbilityId::None;
  bool accepted = false;
  bool projectileSpawned = false;
  std::uint32_t resourceRemaining = 0;
  std::uint32_t resourceCost = 0;
  CommandTick cooldownReadyTick = 0;
  std::string reasonCode = "ability_invalid";
};

struct AbilityTickRequest {
  const SpatialSurfaceSet* collisionSurfaces = nullptr;
  const WorldState* world = nullptr;
  CombatState* combat = nullptr;
  float deltaSeconds = 0.0F;
};

struct AbilityTickResult {
  AbilityTickStatus status = AbilityTickStatus::NoActiveProjectile;
  AbilityImpactKind impactKind = AbilityImpactKind::None;
  bool projectileVisible = false;
  bool projectileActive = false;
  bool projectileImpact = false;
  bool hitEntity = false;
  EntityId hitEntityId;
  std::string hitStableName = "none";
  bool damageApplied = false;
  std::int32_t damageAmount = 0;
  bool targetDefeated = false;
  std::string reasonCode = "ability_no_active_projectile";
};

std::string_view abilityIdName(AbilityId ability);
std::string_view abilityCastStatusName(AbilityCastStatus status);
std::string_view abilityTickStatusName(AbilityTickStatus status);
std::string_view abilityImpactKindName(AbilityImpactKind kind);
const AbilityDefinition* findAbilityDefinition(AbilityId ability);

AbilityCastResult inspectAbilityCast(const AbilityState& abilityState,
                                      const AbilityRuntimeState& runtimeState,
                                      const AbilityCastRequest& request);
AbilityCastResult castAbility(AbilityState& abilityState,
                              AbilityRuntimeState& runtimeState,
                              const AbilityCastRequest& request);
AbilityRechargeResult tickAbilityState(AbilityState& abilityState, CommandTick currentTick);
AbilityTickResult tickAbilityRuntime(AbilityRuntimeState& state,
                                     const AbilityTickRequest& request);
void resetAbilityRuntime(AbilityRuntimeState& state);
bool abilityProjectileVisible(const AbilityProjectileState& projectile);
bool abilityRuntimeHasActiveProjectile(const AbilityRuntimeState& state);
bool abilityStateHasPendingRecharge(const AbilityState& state);

}  // namespace iggy3d
