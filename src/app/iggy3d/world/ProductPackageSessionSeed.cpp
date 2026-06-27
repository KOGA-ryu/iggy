#include "app/iggy3d/world/ProductPackageSessionSeed.hpp"

#include "app/iggy3d/ProductNpcProfileAssignment.hpp"
#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/math/Aabb3.hpp"
#include "core/math/Transform3.hpp"
#include "runtime/combat/CombatState.hpp"
#include "runtime/interaction/InteractionDefinition.hpp"
#include "runtime/player/PlayerSlot.hpp"
#include "runtime/world/EntityState.hpp"

namespace iggy3d {
namespace {

ProductPackageSessionSeedResult fail(std::string reason, const PackageLoadResult& package) {
  ProductPackageSessionSeedResult result;
  result.status = std::move(reason);
  result.reasonCode = result.status;
  result.roomCount = package.rooms.size();
  result.playerCount = package.scenario.players.size();
  result.entityCount = package.scenario.entities.size();
  result.objectiveCount = package.scenario.objectives.size();
  return result;
}

Transform3 transformAt(Vec3 position) {
  Transform3 transform = identityTransform3();
  transform.position = position;
  return transform;
}

ScenarioEntitySeed baseEntity(std::string stableName,
                              EntityKind kind,
                              Vec3 position,
                              Aabb3 bounds) {
  ScenarioEntitySeed seed;
  seed.stableName = std::move(stableName);
  seed.kind = kind;
  seed.transform = transformAt(position);
  seed.localBounds = bounds;
  seed.active = true;
  seed.persistent = true;
  return seed;
}

ScenarioEntitySeed playerFromAnchor(const RoomAnchorAsset& anchor) {
  ScenarioEntitySeed seed = baseEntity("player",
                                       EntityKind::Player,
                                       anchor.positionMeters,
                                       makeAabb3({-0.25F, 0.0F, -0.25F},
                                                 {0.25F, 1.8F, 0.25F}));
  seed.targeting.targetable = true;
  seed.targeting.actions = {TargetAction::Attack, TargetAction::Inspect};
  seed.combatantEnabled = true;
  seed.combatant.factionId = 1;
  seed.combatant.hitPoints = 10;
  seed.combatant.maxHitPoints = 10;
  return seed;
}

ScenarioEntitySeed npcFromAnchor(const RoomAnchorAsset& anchor) {
  ScenarioEntitySeed seed = baseEntity(anchor.id,
                                       EntityKind::Npc,
                                       anchor.positionMeters,
                                       makeAabb3({-0.25F, 0.0F, -0.25F},
                                                 {0.25F, 1.2F, 0.25F}));
  seed.targeting.targetable = true;
  seed.targeting.actions = {TargetAction::Attack, TargetAction::Inspect};
  seed.combatantEnabled = true;
  seed.combatant.factionId = 2;
  seed.combatant.hitPoints = 3;
  seed.combatant.maxHitPoints = 3;
  return seed;
}

std::string objectiveIdFor(std::string_view prefix, std::string_view stableName) {
  return std::string(prefix) + "_" + std::string(stableName);
}

ScenarioEntitySeed pickupFromAnchor(const RoomAnchorAsset& anchor) {
  ScenarioEntitySeed seed = baseEntity(anchor.id,
                                       EntityKind::Pickup,
                                       anchor.positionMeters,
                                       makeAabb3({-0.10F, 0.0F, -0.10F},
                                                 {0.10F, 0.10F, 0.10F}));
  seed.targeting.targetable = true;
  seed.targeting.actions = {TargetAction::Interact, TargetAction::Inspect};
  seed.interaction.kind = InteractionKind::Pickup;
  seed.interaction.primaryEffect = InteractionEffectKind::AddItemToInventory;
  seed.interaction.itemId = anchor.id;
  seed.interaction.itemCount = 1;
  seed.interaction.deactivateTargetOnSuccess = true;
  return seed;
}

ScenarioEntitySeed doorFromAnchor(const RoomAnchorAsset& anchor,
                                  std::string_view requiredItemId) {
  ScenarioEntitySeed seed = baseEntity(anchor.id,
                                       EntityKind::Door,
                                       anchor.positionMeters,
                                       makeAabb3({-0.25F, 0.0F, -0.25F},
                                                 {0.25F, 1.8F, 0.25F}));
  seed.targeting.targetable = true;
  seed.targeting.actions = {TargetAction::Interact, TargetAction::Inspect};
  seed.interaction.kind = InteractionKind::OpenDoor;
  seed.interaction.primaryEffect = InteractionEffectKind::EmitEventOnly;
  if (anchor.kind == "secret_door" && !requiredItemId.empty()) {
    seed.interaction.requiredItemId = std::string(requiredItemId);
    seed.interaction.requiredItemCount = 1;
  }
  seed.interaction.deactivateTargetOnSuccess = true;
  return seed;
}

ScenarioEntitySeed exitFromAnchor(const RoomAnchorAsset& anchor,
                                  std::string_view requiredItemId) {
  ScenarioEntitySeed seed = baseEntity(anchor.id,
                                       EntityKind::Marker,
                                       anchor.positionMeters,
                                       makeAabb3({-0.10F, 0.0F, -0.10F},
                                                 {0.10F, 0.10F, 0.10F}));
  seed.targeting.targetable = true;
  seed.targeting.actions = {TargetAction::Interact, TargetAction::Move,
                            TargetAction::Inspect};
  seed.interaction.kind = InteractionKind::ObjectiveTrigger;
  seed.interaction.primaryEffect = InteractionEffectKind::CompleteObjective;
  seed.interaction.objectiveId = objectiveIdFor("exit", anchor.id);
  if (!requiredItemId.empty()) {
    seed.interaction.requiredItemId = std::string(requiredItemId);
    seed.interaction.requiredItemCount = 1;
  }
  return seed;
}

ScenarioEntitySeed markerFromAnchor(const RoomAnchorAsset& anchor,
                                    std::initializer_list<TargetAction> actions) {
  ScenarioEntitySeed seed = baseEntity(anchor.id,
                                       EntityKind::Marker,
                                       anchor.positionMeters,
                                       makeAabb3({-0.10F, 0.0F, -0.10F},
                                                 {0.10F, 0.10F, 0.10F}));
  seed.targeting.targetable = true;
  seed.targeting.actions = actions;
  return seed;
}

bool isSpawn(const RoomAnchorAsset& anchor) {
  return anchor.kind == "spawn";
}

ScenarioEntitySeed entityFromAnchor(const RoomAnchorAsset& anchor,
                                    std::string_view firstKeyItemId,
                                    std::string_view firstTreasureItemId) {
  if (anchor.kind == "npc") {
    return npcFromAnchor(anchor);
  }
  if (anchor.kind == "pickup" || anchor.kind == "key" || anchor.kind == "treasure") {
    return pickupFromAnchor(anchor);
  }
  if (anchor.kind == "door" || anchor.kind == "secret_door") {
    return doorFromAnchor(anchor, firstKeyItemId);
  }
  if (anchor.kind == "exit") {
    return exitFromAnchor(anchor, firstTreasureItemId);
  }
  return markerFromAnchor(anchor, {TargetAction::Inspect});
}

ScenarioObjectiveSeed inventoryObjective(std::string id, const ScenarioEntitySeed& pickup) {
  ScenarioObjectiveSeed objective;
  objective.id = std::move(id);
  objective.initialStatus = ObjectiveStatusSeed::Active;
  objective.condition = "InventoryContains";
  objective.playerSlot = 0;
  objective.itemId = pickup.interaction.itemId;
  objective.itemCount = 1;
  objective.completeStatus = ObjectiveStatusSeed::Complete;
  return objective;
}

ScenarioObjectiveSeed objectiveForPickup(const ScenarioEntitySeed& pickup) {
  return inventoryObjective(objectiveIdFor("collect", pickup.stableName), pickup);
}

ScenarioObjectiveSeed objectiveForExit(const ScenarioEntitySeed& exit) {
  ScenarioObjectiveSeed objective;
  objective.id = exit.interaction.objectiveId;
  objective.initialStatus = ObjectiveStatusSeed::Active;
  objective.condition = "None";
  objective.playerSlot = 0;
  objective.completeStatus = ObjectiveStatusSeed::Complete;
  return objective;
}

void fillCounts(ProductPackageSessionSeedResult& result) {
  result.playerCount = result.seed.players.size();
  result.entityCount = result.seed.entities.size();
  result.objectiveCount = result.seed.objectives.size();
  result.npcCount = 0;
  result.pickupCount = 0;
  result.doorCount = 0;
  result.markerEntityCount = 0;
  for (const ScenarioEntitySeed& entity : result.seed.entities) {
    switch (entity.kind) {
      case EntityKind::Npc:
        ++result.npcCount;
        break;
      case EntityKind::Pickup:
        ++result.pickupCount;
        break;
      case EntityKind::Door:
        ++result.doorCount;
        break;
      case EntityKind::Marker:
        ++result.markerEntityCount;
        break;
      default:
        break;
    }
  }
}

ProductPackageSessionSeedResult assignmentFailure(const PackageLoadResult& package) {
  return fail("product_package_seed_npc_profile_assignment_invalid", package);
}

ProductNpcProfileAssignmentTable assignmentTableFromAiActors(
    const std::vector<ScenarioAiActorSeed>& aiActors) {
  ProductNpcProfileAssignmentTable table;
  table.assignments.reserve(aiActors.size());
  for (const ScenarioAiActorSeed& aiActor : aiActors) {
    table.assignments.push_back({aiActor.actorStableName, aiActor.behaviorProfileId});
  }
  return table;
}

bool validateScenarioAiActors(const std::vector<ScenarioAiActorSeed>& aiActors) {
  const ProductNpcProfileAssignmentTable table = assignmentTableFromAiActors(aiActors);
  return validateProductNpcProfileAssignments(table).ok;
}

bool assignNpcBehaviorProfiles(FixtureScenarioSeed& seed,
                               const ProductNpcProfileAssignmentTable* assignments) {
  seed.aiActors.clear();
  if (assignments != nullptr) {
    const ProductNpcProfileAssignmentValidationResult validation =
        validateProductNpcProfileAssignments(*assignments);
    if (!validation.ok) {
      return false;
    }
  }

  for (const ScenarioEntitySeed& entity : seed.entities) {
    if (entity.kind != EntityKind::Npc) {
      continue;
    }
    const ProductNpcProfileResolveResult resolved =
        resolveProductNpcProfileAssignment({assignments, entity.stableName});
    if (!resolved.ok) {
      return false;
    }
    ScenarioAiActorSeed aiActor;
    aiActor.actorStableName = entity.stableName;
    aiActor.behaviorProfileId = resolved.behaviorProfileId;
    seed.aiActors.push_back(std::move(aiActor));
  }
  return true;
}

}  // namespace

ProductPackageSessionSeedResult buildProductPackageSessionSeed(
    const PackageLoadResult& package,
    const ProductNpcProfileAssignmentTable* npcProfileAssignments) {
  if (package.status != PackageLoadStatus::Ok) {
    return fail("product_package_seed_package_not_loaded", package);
  }

  ProductPackageSessionSeedResult result;
  result.roomCount = package.rooms.size();
  result.seed = package.scenario;

  if (!package.scenario.entities.empty()) {
    if (npcProfileAssignments != nullptr) {
      if (!assignNpcBehaviorProfiles(result.seed, npcProfileAssignments)) {
        return assignmentFailure(package);
      }
    } else if (!result.seed.aiActors.empty() &&
               !validateScenarioAiActors(result.seed.aiActors)) {
      return assignmentFailure(package);
    }
    result.ok = true;
    result.status = "product_package_seed_ready";
    result.reasonCode = result.status;
    result.synthesizedFromRoomAnchors = false;
    fillCounts(result);
    return result;
  }

  if (package.rooms.empty()) {
    return fail("product_package_seed_missing_room", package);
  }

  const RoomAsset& room = package.rooms.front();
  result.sourceRoomId = room.id;
  result.anchorCount = room.anchors.size();

  const RoomAnchorAsset* spawn = nullptr;
  for (const RoomAnchorAsset& anchor : room.anchors) {
    if (isSpawn(anchor)) {
      spawn = &anchor;
      break;
    }
  }
  if (spawn == nullptr) {
    return fail("product_package_seed_missing_spawn_anchor", package);
  }

  const ProductNpcProfileAssignmentTable authoredAssignments =
      assignmentTableFromAiActors(package.scenario.aiActors);
  const ProductNpcProfileAssignmentTable* resolvedAssignments = npcProfileAssignments;
  if (resolvedAssignments == nullptr && !authoredAssignments.assignments.empty()) {
    resolvedAssignments = &authoredAssignments;
  }

  result.seed.players.clear();
  result.seed.entities.clear();
  result.seed.objectives.clear();
  result.seed.aiActors.clear();
  result.seed.players.push_back({0, PlayerSlotKind::Local, "player"});
  result.seed.entities.push_back(playerFromAnchor(*spawn));

  std::string firstKeyItemId;
  std::string firstTreasureItemId;
  for (const RoomAnchorAsset& anchor : room.anchors) {
    if (firstKeyItemId.empty() && anchor.kind == "key") {
      firstKeyItemId = anchor.id;
    }
    if (firstTreasureItemId.empty() && anchor.kind == "treasure") {
      firstTreasureItemId = anchor.id;
    }
  }

  for (const RoomAnchorAsset& anchor : room.anchors) {
    if (isSpawn(anchor)) {
      continue;
    }
    result.seed.entities.push_back(
        entityFromAnchor(anchor, firstKeyItemId, firstTreasureItemId));
  }

  for (ScenarioEntitySeed& entity : result.seed.entities) {
    if (entity.kind == EntityKind::Pickup) {
      entity.interaction.objectiveId = objectiveIdFor("collect", entity.stableName);
      result.seed.objectives.push_back(objectiveForPickup(entity));
    } else if (entity.kind == EntityKind::Marker &&
               entity.interaction.kind == InteractionKind::ObjectiveTrigger) {
      result.seed.objectives.push_back(objectiveForExit(entity));
    }
  }

  if (!assignNpcBehaviorProfiles(result.seed, resolvedAssignments)) {
    return assignmentFailure(package);
  }

  result.ok = true;
  result.status = "product_package_seed_ready";
  result.reasonCode = result.status;
  result.synthesizedFromRoomAnchors = true;
  fillCounts(result);
  return result;
}

ProductPackageSessionSeedResult buildProductPackageSessionSeed(
    const PackageLoadResult& package) {
  return buildProductPackageSessionSeed(package, nullptr);
}

}  // namespace iggy3d
