#pragma once

#include <cstdint>
#include <string>
#include <string_view>

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

struct AbilityRuntimeState {
  AbilityProjectileState arcaneBolt;
};

struct AbilityCastRequest {
  AbilityId ability = AbilityId::ArcaneBolt;
  EntityId caster;
  Vec3 originMeters;
  Vec3 direction;
  const SpatialSurfaceSet* collisionSurfaces = nullptr;
  CommandId sourceCommandId = kInvalidCommandId;
};

struct AbilityCastResult {
  AbilityCastStatus status = AbilityCastStatus::InvalidAbility;
  AbilityId ability = AbilityId::None;
  bool accepted = false;
  bool projectileSpawned = false;
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

AbilityCastResult castAbility(AbilityRuntimeState& state,
                              const AbilityCastRequest& request);
AbilityTickResult tickAbilityRuntime(AbilityRuntimeState& state,
                                     const AbilityTickRequest& request);
void resetAbilityRuntime(AbilityRuntimeState& state);
bool abilityProjectileVisible(const AbilityProjectileState& projectile);

}  // namespace iggy3d
