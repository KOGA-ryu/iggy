#include "content/PackageLoader.hpp"
#include "content/assets/RoomAsset.hpp"
#include "runtime/ability/AbilitySystem.hpp"
#include "runtime/combat/CombatState.hpp"
#include "runtime/world/WorldState.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

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

iggy3d::AbilityCastRequest arcaneBoltRequest(const iggy3d::SpatialSurfaceSet& surfaces) {
  iggy3d::AbilityCastRequest request;
  request.ability = iggy3d::AbilityId::ArcaneBolt;
  request.caster = {1};
  request.originMeters = {0.0F, 1.6F, 0.0F};
  request.direction = {0.0F, 0.0F, -2.0F};
  request.collisionSurfaces = &surfaces;
  request.sourceCommandId = 42;
  return request;
}

iggy3d::EntityState combatEntity(iggy3d::EntityId id,
                                 std::string_view stableName,
                                 iggy3d::Vec3 position,
                                 iggy3d::EntityKind kind = iggy3d::EntityKind::Npc,
                                 bool attackable = true) {
  iggy3d::EntityState entity;
  entity.id = id;
  entity.stableName = std::string(stableName);
  entity.kind = kind;
  entity.transform.position = position;
  entity.localBounds = {{-0.25F, 0.0F, -0.25F}, {0.25F, 1.8F, 0.25F}};
  entity.targeting.targetable = attackable;
  if (attackable) {
    entity.targeting.actions.push_back(iggy3d::TargetAction::Attack);
  }
  return entity;
}

iggy3d::WorldState worldWithCasterAndTarget(iggy3d::EntityId target,
                                            std::string_view targetName,
                                            iggy3d::Vec3 targetPosition) {
  iggy3d::WorldState world;
  static_cast<void>(world.seedEntity(
      combatEntity({1}, "caster", {0.0F, 0.0F, 0.0F}, iggy3d::EntityKind::Player, false)));
  static_cast<void>(world.seedEntity(combatEntity(target, targetName, targetPosition)));
  return world;
}

iggy3d::CombatState combatWithCasterAndTarget(iggy3d::EntityId target,
                                              std::int32_t targetHitPoints) {
  iggy3d::CombatState combat;
  combat.combatants.push_back({{1}, 1, 10, 10, false});
  combat.combatants.push_back({target, 2, targetHitPoints, targetHitPoints, false});
  return combat;
}

iggy3d::SpatialSurfaceSet projectileWallSurfaceSet() {
  iggy3d::RoomSpatialSurface wall;
  wall.id = "test_projectile_wall";
  wall.shape = iggy3d::RoomSpatialSurfaceShape::Box;
  wall.role = iggy3d::RoomSpatialSurfaceRole::ProjectileBlocker;
  wall.pointsMeters = {{-1.0F, 0.0F, -2.60F}, {1.0F, 2.0F, -2.50F}};
  wall.normal = {0.0F, 0.0F, 1.0F};
  wall.blocksProjectile = true;
  iggy3d::RoomAsset room;
  room.spatialSurfaces.push_back(std::move(wall));
  return iggy3d::buildSpatialSurfaceSet(room);
}

bool castAcceptedSpawnsRuntimeProjectile() {
  const iggy3d::SpatialSurfaceSet surfaces;
  iggy3d::AbilityRuntimeState state;
  const iggy3d::AbilityCastResult cast =
      iggy3d::castAbility(state, arcaneBoltRequest(surfaces));

  return expect(cast.status == iggy3d::AbilityCastStatus::Accepted, "cast accepted") &&
         expect(cast.accepted, "accepted flag") &&
         expect(cast.projectileSpawned, "projectile spawned flag") &&
         expect(cast.reasonCode == "ability_cast_accepted", "accepted reason") &&
         expect(state.arcaneBolt.spawned, "runtime projectile spawned") &&
         expect(state.arcaneBolt.projectile.active, "runtime projectile active") &&
         expect(state.arcaneBolt.ability == iggy3d::AbilityId::ArcaneBolt,
                "arcane bolt ability id") &&
         expect(state.arcaneBolt.caster == iggy3d::EntityId{1}, "caster owned") &&
         expect(state.arcaneBolt.sourceCommandId == 42, "source command id owned") &&
         expect(state.arcaneBolt.projectileId == "arcane_bolt_projectile",
                "projectile id") &&
         expect(approx(state.arcaneBolt.projectile.positionMeters.z, -0.75F),
                "muzzle offset applied") &&
         expect(approx(state.arcaneBolt.projectile.velocityMetersPerSecond.z, -12.0F),
                "velocity normalized") &&
         expect(iggy3d::abilityIdName(state.arcaneBolt.ability) == "arcane_bolt",
                "ability id name") &&
         expect(iggy3d::abilityProjectileVisible(state.arcaneBolt), "visible projectile");
}

bool activeProjectileSlotRejectsSecondCast() {
  const iggy3d::SpatialSurfaceSet surfaces;
  iggy3d::AbilityRuntimeState state;
  const iggy3d::AbilityCastResult first =
      iggy3d::castAbility(state, arcaneBoltRequest(surfaces));
  const iggy3d::AbilityCastResult second =
      iggy3d::castAbility(state, arcaneBoltRequest(surfaces));

  return expect(first.accepted, "first cast accepted") &&
         expect(second.status == iggy3d::AbilityCastStatus::ProjectileSlotBusy,
                "second cast slot busy") &&
         expect(!second.accepted, "second cast rejected") &&
         expect(second.reasonCode == "ability_projectile_slot_busy", "slot busy reason");
}

bool tickAdvancesProjectileBallistically() {
  const iggy3d::SpatialSurfaceSet surfaces;
  iggy3d::AbilityRuntimeState state;
  const iggy3d::AbilityCastResult cast =
      iggy3d::castAbility(state, arcaneBoltRequest(surfaces));
  iggy3d::AbilityTickRequest tick;
  tick.collisionSurfaces = &surfaces;
  tick.deltaSeconds = 1.0F / 60.0F;

  const iggy3d::AbilityTickResult result = iggy3d::tickAbilityRuntime(state, tick);
  return expect(cast.accepted, "cast accepted before tick") &&
         expect(result.status == iggy3d::AbilityTickStatus::Advanced, "tick advanced") &&
         expect(result.projectileVisible, "projectile visible after tick") &&
         expect(result.projectileActive, "projectile active after tick") &&
         expect(!result.projectileImpact, "no impact after empty tick") &&
         expect(result.reasonCode == "projectile_advanced", "projectile advanced reason") &&
         expect(state.arcaneBolt.projectile.positionMeters.z < -0.75F, "projectile moved") &&
         expect(state.arcaneBolt.previousPositionMeters.z < 0.0F,
                "previous position retained") &&
         expect(iggy3d::abilityTickStatusName(result.status) == "advanced",
                "tick status name");
}

bool arcaneBoltEntityImpactAppliesCombatDamage() {
  const iggy3d::SpatialSurfaceSet surfaces;
  iggy3d::AbilityRuntimeState state;
  const iggy3d::EntityId target{2};
  const iggy3d::WorldState world =
      worldWithCasterAndTarget(target, "target_dummy", {0.0F, 0.0F, -4.0F});
  iggy3d::CombatState combat = combatWithCasterAndTarget(target, 6);
  const iggy3d::AbilityCastResult cast =
      iggy3d::castAbility(state, arcaneBoltRequest(surfaces));
  iggy3d::AbilityTickRequest tick;
  tick.collisionSurfaces = &surfaces;
  tick.world = &world;
  tick.combat = &combat;
  tick.deltaSeconds = 0.5F;

  const iggy3d::AbilityTickResult result = iggy3d::tickAbilityRuntime(state, tick);
  return expect(cast.accepted, "entity cast accepted") &&
         expect(result.status == iggy3d::AbilityTickStatus::Impact, "entity impact") &&
         expect(result.impactKind == iggy3d::AbilityImpactKind::Entity,
                "entity impact kind") &&
         expect(result.hitEntity, "hit entity flag") &&
         expect(result.hitEntityId == target, "hit entity id") &&
         expect(result.hitStableName == "target_dummy", "hit stable name") &&
         expect(result.damageApplied, "damage applied flag") &&
         expect(result.damageAmount == 3, "damage amount") &&
         expect(!result.targetDefeated, "target not defeated") &&
         expect(combat.combatants[1].hitPoints == 3, "combat hp reduced") &&
         expect(state.arcaneBolt.hitSurfaceId == "entity:target_dummy",
                "entity hit surface label") &&
         expect(state.arcaneBolt.reasonCode == "ability_entity_impact",
                "entity impact reason") &&
         expect(iggy3d::abilityImpactKindName(result.impactKind) == "entity",
                "impact kind name");
}

bool arcaneBoltReportsDefeatedTarget() {
  const iggy3d::SpatialSurfaceSet surfaces;
  iggy3d::AbilityRuntimeState state;
  const iggy3d::EntityId target{2};
  const iggy3d::WorldState world =
      worldWithCasterAndTarget(target, "fragile_dummy", {0.0F, 0.0F, -4.0F});
  iggy3d::CombatState combat = combatWithCasterAndTarget(target, 3);
  static_cast<void>(iggy3d::castAbility(state, arcaneBoltRequest(surfaces)));
  iggy3d::AbilityTickRequest tick;
  tick.collisionSurfaces = &surfaces;
  tick.world = &world;
  tick.combat = &combat;
  tick.deltaSeconds = 0.5F;

  const iggy3d::AbilityTickResult result = iggy3d::tickAbilityRuntime(state, tick);
  return expect(result.damageApplied, "defeat damage applied") &&
         expect(result.damageAmount == 3, "defeat damage amount") &&
         expect(result.targetDefeated, "defeat reported") &&
         expect(combat.combatants[1].hitPoints == 0, "combat hp zero") &&
         expect(combat.combatants[1].defeated, "combat defeated flag");
}

bool projectileBlockerProducesAbilityImpactReceipt() {
  const iggy3d::SpatialSurfaceSet surfaces = loadFirstRoomSurfaceSet();
  iggy3d::AbilityRuntimeState state;
  iggy3d::AbilityCastRequest request = arcaneBoltRequest(surfaces);
  request.originMeters =
      {4.0F * kFeetToMeters, 1.0F * kFeetToMeters, 14.0F * kFeetToMeters};
  request.direction = {0.0F, 0.0F, 1.0F};
  const iggy3d::AbilityCastResult cast = iggy3d::castAbility(state, request);

  iggy3d::AbilityTickRequest tick;
  tick.collisionSurfaces = &surfaces;
  tick.deltaSeconds = 1.0F;
  const iggy3d::AbilityTickResult result = iggy3d::tickAbilityRuntime(state, tick);

  return expect(cast.accepted, "impact cast accepted") &&
         expect(result.status == iggy3d::AbilityTickStatus::Impact, "ability impact") &&
         expect(result.projectileVisible, "impact projectile visible") &&
         expect(!result.projectileActive, "impact projectile inactive") &&
         expect(result.projectileImpact, "impact flag") &&
         expect(state.arcaneBolt.impact, "state impact") &&
         expect(state.arcaneBolt.hitSurfaceId == "spawn_crate_projectile_blocker",
                "hit surface id") &&
         expect(result.impactKind == iggy3d::AbilityImpactKind::Surface,
                "surface impact kind") &&
         expect(state.arcaneBolt.reasonCode == "projectile_impact", "impact reason");
}

bool surfaceImpactBlocksEntityBehindWall() {
  const iggy3d::SpatialSurfaceSet surfaces = projectileWallSurfaceSet();
  iggy3d::AbilityRuntimeState state;
  const iggy3d::EntityId target{2};
  const iggy3d::WorldState world =
      worldWithCasterAndTarget(target, "blocked_dummy", {0.0F, 0.0F, -4.0F});
  iggy3d::CombatState combat = combatWithCasterAndTarget(target, 6);
  static_cast<void>(iggy3d::castAbility(state, arcaneBoltRequest(surfaces)));
  iggy3d::AbilityTickRequest tick;
  tick.collisionSurfaces = &surfaces;
  tick.world = &world;
  tick.combat = &combat;
  tick.deltaSeconds = 0.5F;

  const iggy3d::AbilityTickResult result = iggy3d::tickAbilityRuntime(state, tick);
  return expect(result.status == iggy3d::AbilityTickStatus::Impact, "wall impact") &&
         expect(result.impactKind == iggy3d::AbilityImpactKind::Surface,
                "wall impact kind") &&
         expect(!result.hitEntity, "wall blocks entity hit") &&
         expect(!result.damageApplied, "wall blocks damage") &&
         expect(combat.combatants[1].hitPoints == 6, "wall leaves hp") &&
         expect(state.arcaneBolt.hitSurfaceId == "test_projectile_wall",
                "wall surface id");
}

bool resetClearsAbilityRuntime() {
  const iggy3d::SpatialSurfaceSet surfaces;
  iggy3d::AbilityRuntimeState state;
  const iggy3d::AbilityCastResult cast =
      iggy3d::castAbility(state, arcaneBoltRequest(surfaces));
  iggy3d::resetAbilityRuntime(state);

  return expect(cast.accepted, "cast accepted before reset") &&
         expect(!state.arcaneBolt.spawned, "reset clears spawned") &&
         expect(!state.arcaneBolt.projectile.active, "reset clears active") &&
         expect(!iggy3d::abilityProjectileVisible(state.arcaneBolt), "reset clears visible");
}

bool invalidInputsAreDiagnosed() {
  const iggy3d::SpatialSurfaceSet surfaces;
  iggy3d::AbilityRuntimeState state;

  iggy3d::AbilityCastRequest missingCaster = arcaneBoltRequest(surfaces);
  missingCaster.caster = iggy3d::kInvalidEntityId;
  const iggy3d::AbilityCastResult invalidCaster =
      iggy3d::castAbility(state, missingCaster);

  iggy3d::AbilityCastRequest invalidDirection = arcaneBoltRequest(surfaces);
  invalidDirection.direction = {0.0F, 0.0F, 0.0F};
  const iggy3d::AbilityCastResult zeroDirection =
      iggy3d::castAbility(state, invalidDirection);

  iggy3d::AbilityCastRequest invalidOrigin = arcaneBoltRequest(surfaces);
  invalidOrigin.originMeters.x = std::numeric_limits<float>::infinity();
  const iggy3d::AbilityCastResult badOrigin =
      iggy3d::castAbility(state, invalidOrigin);

  iggy3d::AbilityCastRequest missingSurfaces = arcaneBoltRequest(surfaces);
  missingSurfaces.collisionSurfaces = nullptr;
  const iggy3d::AbilityCastResult noSurfaces =
      iggy3d::castAbility(state, missingSurfaces);

  return expect(invalidCaster.status == iggy3d::AbilityCastStatus::InvalidCaster,
                "invalid caster") &&
         expect(zeroDirection.status == iggy3d::AbilityCastStatus::InvalidDirection,
                "zero direction") &&
         expect(badOrigin.status == iggy3d::AbilityCastStatus::InvalidOrigin,
                "invalid origin") &&
         expect(noSurfaces.status == iggy3d::AbilityCastStatus::MissingCollisionSurfaces,
                "missing surfaces") &&
         expect(noSurfaces.reasonCode == "ability_missing_collision_surfaces",
                "missing surfaces reason");
}

}  // namespace

int main() {
  const bool ok = castAcceptedSpawnsRuntimeProjectile() &&
                  activeProjectileSlotRejectsSecondCast() &&
                  tickAdvancesProjectileBallistically() &&
                  arcaneBoltEntityImpactAppliesCombatDamage() &&
                  arcaneBoltReportsDefeatedTarget() &&
                  projectileBlockerProducesAbilityImpactReceipt() &&
                  surfaceImpactBlocksEntityBehindWall() &&
                  resetClearsAbilityRuntime() &&
                  invalidInputsAreDiagnosed();
  return ok ? 0 : 1;
}
