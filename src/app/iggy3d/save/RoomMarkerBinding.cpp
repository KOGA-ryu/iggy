#include "app/iggy3d/save/RoomMarkerBinding.hpp"

#include <string_view>
#include <utility>

#include "app/iggy3d/ascii_room/Package.hpp"
#include "app/iggy3d/world/PackageSessionSeed.hpp"
#include "content/FixtureScenarioLoader.hpp"
#include "runtime/combat/CombatState.hpp"
#include "runtime/objective/ObjectiveState.hpp"
#include "runtime/replay/StateHash.hpp"
#include "runtime/world/EntityState.hpp"

namespace iggy3d {
namespace {

ProductSavedRoomMarkerBindingResult resultWithStatus(std::string status,
                                                     std::string roomId) {
  ProductSavedRoomMarkerBindingResult result;
  result.status = std::move(status);
  result.reasonCode = result.status;
  result.roomId = roomId.empty() ? "none" : std::move(roomId);
  return result;
}

ObjectiveStatus objectiveStatusFromSeed(ObjectiveStatusSeed status) {
  switch (status) {
    case ObjectiveStatusSeed::Active:
      return ObjectiveStatus::Active;
    case ObjectiveStatusSeed::Complete:
      return ObjectiveStatus::Complete;
    case ObjectiveStatusSeed::Failed:
      return ObjectiveStatus::Failed;
  }
  return ObjectiveStatus::Inactive;
}

ObjectiveRecord objectiveFromSeed(const ScenarioObjectiveSeed& seed) {
  ObjectiveRecord objective;
  objective.objectiveId = seed.id;
  objective.status = objectiveStatusFromSeed(seed.initialStatus);
  objective.condition.kind = seed.condition == "InventoryContains"
                                 ? ObjectiveConditionKind::PlayerHasItem
                                 : ObjectiveConditionKind::None;
  objective.condition.playerSlot = seed.playerSlot;
  objective.condition.itemId = seed.itemId;
  objective.condition.itemCount = seed.itemCount;
  return objective;
}

EntityState entityFromSeed(const ScenarioEntitySeed& seed) {
  EntityState entity;
  entity.stableName = seed.stableName;
  entity.kind = seed.kind;
  entity.transform = seed.transform;
  entity.localBounds = seed.localBounds;
  entity.active = seed.active;
  entity.persistent = seed.persistent;
  entity.targeting = seed.targeting;
  entity.interaction = seed.interaction;
  return entity;
}

bool objectiveExists(const ObjectiveState& objectives, std::string_view objectiveId) {
  for (const ObjectiveRecord& objective : objectives.objectives) {
    if (objective.objectiveId == objectiveId) {
      return true;
    }
  }
  return false;
}

bool combatantExists(const CombatState& combat, EntityId entity) {
  for (const CombatantState& combatant : combat.combatants) {
    if (combatant.entity == entity) {
      return true;
    }
  }
  return false;
}

bool combatantSeedValid(const CombatantState& combatant) {
  return combatant.maxHitPoints > 0 && combatant.hitPoints > 0 &&
         combatant.hitPoints <= combatant.maxHitPoints;
}

bool addCombatantFromSeed(SessionState& state,
                          const ScenarioEntitySeed& seed,
                          EntityId entity,
                          ProductSavedRoomMarkerBindingResult& result) {
  if (!seed.combatantEnabled) {
    return true;
  }
  if (combatantExists(state.combat, entity)) {
    ++result.existingCombatantCount;
    return true;
  }

  CombatantState combatant = seed.combatant;
  combatant.entity = entity;
  combatant.defeated = combatant.hitPoints == 0;
  if (!combatantSeedValid(combatant) || combatant.defeated) {
    result.status = "saved_marker_bind_invalid_combatant";
    result.reasonCode = result.status;
    return false;
  }
  state.combat.combatants.push_back(combatant);
  ++result.addedCombatantCount;
  return true;
}

bool mergeEntities(SessionState& state,
                   const FixtureScenarioSeed& seed,
                   ProductSavedRoomMarkerBindingResult& result) {
  for (const ScenarioEntitySeed& entitySeed : seed.entities) {
    const EntityState* existing =
        state.world.findByStableName(entitySeed.stableName);
    EntityId entityId;
    if (existing != nullptr) {
      entityId = existing->id;
      ++result.existingEntityCount;
    } else {
      if (entitySeed.kind == EntityKind::Player) {
        result.status = "saved_marker_bind_player_missing";
        result.reasonCode = result.status;
        return false;
      }
      const WorldEntityResult added =
          state.world.addEntity(entityFromSeed(entitySeed));
      if (added.status != WorldStatus::Ok) {
        result.status = "saved_marker_bind_world_insert_failed";
        result.reasonCode = result.status;
        return false;
      }
      entityId = added.id;
      ++result.addedEntityCount;
    }

    if (!addCombatantFromSeed(state, entitySeed, entityId, result)) {
      return false;
    }
  }
  return true;
}

void mergeObjectives(SessionState& state,
                     const FixtureScenarioSeed& seed,
                     ProductSavedRoomMarkerBindingResult& result) {
  for (const ScenarioObjectiveSeed& objectiveSeed : seed.objectives) {
    if (objectiveExists(state.objectives, objectiveSeed.id)) {
      ++result.existingObjectiveCount;
      continue;
    }
    state.objectives.objectives.push_back(objectiveFromSeed(objectiveSeed));
    ++result.addedObjectiveCount;
  }
}

bool resultMutated(const ProductSavedRoomMarkerBindingResult& result) {
  return result.addedEntityCount > 0U || result.addedObjectiveCount > 0U ||
         result.addedCombatantCount > 0U;
}

void refreshBaselineForBoundState(SessionState& state) {
  state.baseline.world = state.world;
  state.baseline.combat = state.combat;
  state.baseline.objectives = state.objectives;
  state.currentStateHash = computeStateHash(state);
  state.baseline.baselineHash = state.currentStateHash;
}

}  // namespace

ProductSavedRoomMarkerBindingResult bindSavedRoomMarkersToSession(
    const ProductActiveRoomState& activeRoom,
    Session& session) {
  ProductSavedRoomMarkerBindingResult result =
      resultWithStatus("saved_marker_bind_not_requested", activeRoom.roomId);
  result.previousHash = session.stateHash();

  if (!activeRoom.loaded || !activeRoom.hasAuthoredRoom) {
    result.ok = true;
    return result;
  }

  result.requested = true;
  result.markerCount = activeRoom.authoredMarkerCount;
  if (activeRoom.authoredMarkerCount == 0U) {
    result.ok = true;
    result.status = "saved_marker_bind_no_markers";
    result.reasonCode = result.status;
    result.boundHash = session.stateHash();
    return result;
  }

  const PackageLoadResult package =
      makeProductAsciiRoomPackage(activeRoom.room,
                                  session.state().identity.packageId,
                                  session.state().identity.scenarioId);
  const ProductPackageSessionSeedResult seed =
      buildProductPackageSessionSeed(package);
  result.seedEntityCount = static_cast<std::uint64_t>(seed.entityCount);
  result.pickupCount = static_cast<std::uint64_t>(seed.pickupCount);
  result.doorCount = static_cast<std::uint64_t>(seed.doorCount);
  result.markerEntityCount = static_cast<std::uint64_t>(seed.markerEntityCount);
  result.npcCount = static_cast<std::uint64_t>(seed.npcCount);
  if (!seed.ok) {
    result.status = seed.reasonCode.empty() ? "saved_marker_bind_seed_failed"
                                            : seed.reasonCode;
    result.reasonCode = result.status;
    return result;
  }

  SessionState candidate = session.state();
  if (!mergeEntities(candidate, seed.seed, result)) {
    return result;
  }
  mergeObjectives(candidate, seed.seed, result);

  if (!resultMutated(result)) {
    result.ok = true;
    result.status = "saved_marker_bind_noop";
    result.reasonCode = result.status;
    result.boundHash = session.stateHash();
    return result;
  }

  refreshBaselineForBoundState(candidate);
  const SessionLoadResult replaced = session.replaceStateFromLoad(std::move(candidate));
  if (replaced.status != SessionLoadStatus::Ok) {
    result.status = "saved_marker_bind_replace_failed";
    result.reasonCode = result.status;
    result.boundHash = session.stateHash();
    return result;
  }

  result.ok = true;
  result.sessionReplaced = true;
  result.status = "saved_marker_bind_applied";
  result.reasonCode = result.status;
  result.previousHash = replaced.previousHash;
  result.boundHash = replaced.loadedHash;
  return result;
}

}  // namespace iggy3d
