#include "app/iggy3d/ProductPackageSessionSeed.hpp"

#include <initializer_list>
#include <string>
#include <string_view>

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
  seed.targeting.targetable = false;
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

ScenarioEntitySeed doorFromAnchor(const RoomAnchorAsset& anchor) {
  ScenarioEntitySeed seed = baseEntity(anchor.id,
                                       EntityKind::Door,
                                       anchor.positionMeters,
                                       makeAabb3({-0.25F, 0.0F, -0.25F},
                                                 {0.25F, 1.8F, 0.25F}));
  seed.targeting.targetable = true;
  seed.targeting.actions = {TargetAction::Interact, TargetAction::Inspect};
  seed.interaction.kind = InteractionKind::OpenDoor;
  seed.interaction.primaryEffect = InteractionEffectKind::EmitEventOnly;
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

ScenarioEntitySeed entityFromAnchor(const RoomAnchorAsset& anchor) {
  if (anchor.kind == "npc") {
    return npcFromAnchor(anchor);
  }
  if (anchor.kind == "pickup") {
    return pickupFromAnchor(anchor);
  }
  if (anchor.kind == "door" || anchor.kind == "secret_door") {
    return doorFromAnchor(anchor);
  }
  if (anchor.kind == "exit") {
    return markerFromAnchor(anchor, {TargetAction::Move, TargetAction::Inspect});
  }
  return markerFromAnchor(anchor, {TargetAction::Inspect});
}

ScenarioObjectiveSeed objectiveForPickup(const ScenarioEntitySeed& pickup) {
  ScenarioObjectiveSeed objective;
  objective.id = "collect_" + pickup.stableName;
  objective.initialStatus = ObjectiveStatusSeed::Active;
  objective.condition = "InventoryContains";
  objective.playerSlot = 0;
  objective.itemId = pickup.interaction.itemId;
  objective.itemCount = 1;
  objective.completeStatus = ObjectiveStatusSeed::Complete;
  return objective;
}

void fillCounts(ProductPackageSessionSeedResult& result) {
  result.playerCount = result.seed.players.size();
  result.entityCount = result.seed.entities.size();
  result.objectiveCount = result.seed.objectives.size();
}

}  // namespace

ProductPackageSessionSeedResult buildProductPackageSessionSeed(
    const PackageLoadResult& package) {
  if (package.status != PackageLoadStatus::Ok) {
    return fail("product_package_seed_package_not_loaded", package);
  }

  ProductPackageSessionSeedResult result;
  result.roomCount = package.rooms.size();
  result.seed = package.scenario;

  if (!package.scenario.entities.empty()) {
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

  result.seed.players.clear();
  result.seed.entities.clear();
  result.seed.objectives.clear();
  result.seed.players.push_back({0, PlayerSlotKind::Local, "player"});
  result.seed.entities.push_back(playerFromAnchor(*spawn));

  std::string firstPickupStableName;
  for (const RoomAnchorAsset& anchor : room.anchors) {
    if (isSpawn(anchor)) {
      continue;
    }
    result.seed.entities.push_back(entityFromAnchor(anchor));
    if (firstPickupStableName.empty() && result.seed.entities.back().kind == EntityKind::Pickup) {
      firstPickupStableName = result.seed.entities.back().stableName;
    }
  }
  if (!firstPickupStableName.empty()) {
    for (const ScenarioEntitySeed& entity : result.seed.entities) {
      if (entity.stableName == firstPickupStableName) {
        result.seed.objectives.push_back(objectiveForPickup(entity));
        break;
      }
    }
    for (ScenarioEntitySeed& entity : result.seed.entities) {
      if (entity.stableName == firstPickupStableName && !result.seed.objectives.empty()) {
        entity.interaction.objectiveId = result.seed.objectives.front().id;
      }
    }
  }

  result.ok = true;
  result.status = "product_package_seed_ready";
  result.reasonCode = result.status;
  result.synthesizedFromRoomAnchors = true;
  fillCounts(result);
  return result;
}

}  // namespace iggy3d
