#include "runtime/interaction/InteractionSystem.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

iggy3d::Transform3 transformAt(float x, float y, float z) {
  iggy3d::Transform3 transform = iggy3d::identityTransform3();
  transform.position = {x, y, z};
  return transform;
}

iggy3d::EntityState makePlayer() {
  iggy3d::EntityState entity;
  entity.id = {1};
  entity.stableName = "player";
  entity.kind = iggy3d::EntityKind::Player;
  entity.transform = transformAt(2.0F, 0.0F, 0.0F);
  entity.localBounds = iggy3d::makeAabb3({-0.25F, 0.0F, -0.25F}, {0.25F, 1.8F, 0.25F});
  return entity;
}

iggy3d::EntityState makeGoldKey() {
  iggy3d::EntityState entity;
  entity.id = {2};
  entity.stableName = "gold_key";
  entity.kind = iggy3d::EntityKind::Pickup;
  entity.transform = transformAt(3.0F, 0.0F, 0.0F);
  entity.localBounds = iggy3d::makeAabb3({-0.1F, 0.0F, -0.1F}, {0.1F, 0.1F, 0.1F});
  entity.targeting.targetable = true;
  entity.targeting.actions = {iggy3d::TargetAction::Interact, iggy3d::TargetAction::Inspect};
  entity.interaction.kind = iggy3d::InteractionKind::Pickup;
  entity.interaction.primaryEffect = iggy3d::InteractionEffectKind::AddItemToInventory;
  entity.interaction.itemId = "gold_key";
  entity.interaction.itemCount = 1;
  entity.interaction.objectiveId = "collect_gold_key";
  entity.interaction.deactivateTargetOnSuccess = true;
  return entity;
}

iggy3d::EntityState makeDoor() {
  iggy3d::EntityState entity;
  entity.id = {3};
  entity.stableName = "wooden_door";
  entity.kind = iggy3d::EntityKind::Door;
  entity.transform = transformAt(3.0F, 0.0F, 1.0F);
  entity.localBounds = iggy3d::makeAabb3({-0.25F, 0.0F, -0.25F},
                                         {0.25F, 1.8F, 0.25F});
  entity.targeting.targetable = true;
  entity.targeting.actions = {iggy3d::TargetAction::Interact,
                              iggy3d::TargetAction::Inspect};
  entity.interaction.kind = iggy3d::InteractionKind::OpenDoor;
  entity.interaction.primaryEffect = iggy3d::InteractionEffectKind::EmitEventOnly;
  return entity;
}

iggy3d::WorldState makeWorld() {
  iggy3d::WorldState world;
  (void)world.seedEntity(makePlayer());
  (void)world.seedEntity(makeGoldKey());
  (void)world.seedEntity(makeDoor());
  return world;
}

iggy3d::InventoryState makeInventory(bool includePlayer = true) {
  iggy3d::InventoryState inventory;
  if (includePlayer) {
    inventory.players.push_back({0, {}});
  }
  return inventory;
}

iggy3d::ObjectiveState makeObjectives() {
  iggy3d::ObjectiveState objectives;
  iggy3d::ObjectiveRecord objective;
  objective.objectiveId = "collect_gold_key";
  objective.status = iggy3d::ObjectiveStatus::Active;
  objective.condition.kind = iggy3d::ObjectiveConditionKind::PlayerHasItem;
  objective.condition.playerSlot = 0;
  objective.condition.itemId = "gold_key";
  objective.condition.itemCount = 1;
  objectives.objectives.push_back(objective);
  return objectives;
}

iggy3d::CommandRecord acceptedInteract(iggy3d::CommandId id = 8) {
  iggy3d::CommandRecord command;
  command.commandId = id;
  command.sequence = id;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Interact;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.target.hasEntity = true;
  command.payload.target.entity = {2};
  command.admission = iggy3d::CommandAdmissionStatus::Accepted;
  return command;
}

iggy3d::CommandRecord acceptedDoorInteract(iggy3d::CommandId id = 9) {
  iggy3d::CommandRecord command = acceptedInteract(id);
  command.payload.target.entity = {3};
  return command;
}

bool pickupAddsItemDeactivatesTargetAndCompletesObjective() {
  iggy3d::WorldState world = makeWorld();
  iggy3d::InventoryState inventory = makeInventory();
  iggy3d::ObjectiveState objectives = makeObjectives();
  iggy3d::InteractionSystemContext context{&world, &inventory, &objectives};
  const iggy3d::InteractionResult result =
      iggy3d::executeInteraction(context, {acceptedInteract(8), 8, 1});
  return expect(result.status == iggy3d::InteractionStatus::Succeeded, "pickup succeeded") &&
         expect(result.sourceCommandId == 8U && result.retrySourceCommandId == 1U,
                "retry linkage") &&
         expect(result.kind == iggy3d::InteractionKind::Pickup, "pickup kind") &&
         expect(result.primaryEffect == iggy3d::InteractionEffectKind::AddItemToInventory,
                "pickup effect") &&
         expect(result.itemId == "gold_key" && result.itemCount == 1U, "item facts") &&
         expect(result.objectiveId == "collect_gold_key", "objective fact") &&
         expect(result.inventoryMutated && result.targetDeactivated && result.objectiveMutated,
                "mutation flags") &&
         expect(iggy3d::hasItem(inventory, 0, "gold_key", 1), "inventory has key") &&
         expect(!world.findById({2})->active, "target deactivated") &&
         expect(iggy3d::objectiveComplete(objectives, "collect_gold_key"), "objective complete");
}

bool nullContextsReturnStructuredStatus() {
  iggy3d::WorldState world = makeWorld();
  iggy3d::InventoryState inventory = makeInventory();
  iggy3d::ObjectiveState objectives = makeObjectives();
  iggy3d::InteractionSystemContext missingWorld{nullptr, &inventory, &objectives};
  iggy3d::InteractionSystemContext missingInventory{&world, nullptr, &objectives};
  iggy3d::InteractionSystemContext missingObjectives{&world, &inventory, nullptr};
  return expect(iggy3d::executeInteraction(missingWorld, {acceptedInteract(), 8, 1}).status ==
                    iggy3d::InteractionStatus::InvalidWorld,
                "missing world") &&
         expect(iggy3d::executeInteraction(missingInventory, {acceptedInteract(), 8, 1}).status ==
                    iggy3d::InteractionStatus::InvalidInventory,
                "missing inventory") &&
         expect(iggy3d::executeInteraction(missingObjectives, {acceptedInteract(), 8, 1}).status ==
                    iggy3d::InteractionStatus::InvalidObjectiveState,
                "missing objectives");
}

bool openDoorEmitsOnlyAndDoesNotMutateState() {
  iggy3d::WorldState world = makeWorld();
  iggy3d::InventoryState inventory = makeInventory();
  iggy3d::ObjectiveState objectives = makeObjectives();
  iggy3d::InteractionSystemContext context{&world, &inventory, &objectives};
  const iggy3d::InteractionResult result =
      iggy3d::executeInteraction(context, {acceptedDoorInteract(), 9, 1});
  return expect(result.status == iggy3d::InteractionStatus::Succeeded,
                "door succeeded") &&
         expect(result.kind == iggy3d::InteractionKind::OpenDoor,
                "door interaction kind") &&
         expect(result.primaryEffect == iggy3d::InteractionEffectKind::EmitEventOnly,
                "door emit-only effect") &&
         expect(result.itemId.empty() && result.itemCount == 0U, "door no item") &&
         expect(!result.inventoryMutated, "door no inventory mutation") &&
         expect(!result.targetDeactivated, "door target remains active") &&
         expect(!result.objectiveMutated, "door no objective mutation") &&
         expect(world.findById({3})->active, "door still active") &&
         expect(inventory.players[0].stacks.empty(), "door inventory unchanged") &&
         expect(!iggy3d::objectiveComplete(objectives, "collect_gold_key"),
                "door objective unchanged");
}

bool rawRetryIsInvalidCommand() {
  iggy3d::WorldState world = makeWorld();
  iggy3d::InventoryState inventory = makeInventory();
  iggy3d::ObjectiveState objectives = makeObjectives();
  iggy3d::CommandRecord retry = acceptedInteract();
  retry.kind = iggy3d::CommandKind::Retry;
  retry.payload.retrySourceCommandId = 1;
  iggy3d::InteractionSystemContext context{&world, &inventory, &objectives};
  return expect(iggy3d::executeInteraction(context, {retry, 8, 1}).status ==
                    iggy3d::InteractionStatus::InvalidCommand,
                "raw retry invalid") &&
         expect(world.findById({2})->active, "raw retry no world mutation") &&
         expect(inventory.players[0].stacks.empty(), "raw retry no inventory mutation");
}

bool inventoryFailureDoesNotDeactivateTarget() {
  iggy3d::WorldState world = makeWorld();
  iggy3d::InventoryState inventory = makeInventory(false);
  iggy3d::ObjectiveState objectives = makeObjectives();
  iggy3d::InteractionSystemContext context{&world, &inventory, &objectives};
  const iggy3d::InteractionResult result =
      iggy3d::executeInteraction(context, {acceptedInteract(), 8, 1});
  return expect(result.status == iggy3d::InteractionStatus::InventoryFailed,
                "inventory failure") &&
         expect(world.findById({2})->active, "inventory failure target active") &&
         expect(!iggy3d::objectiveComplete(objectives, "collect_gold_key"),
                "inventory failure objective unchanged");
}

}  // namespace

int main() {
  const bool ok = pickupAddsItemDeactivatesTargetAndCompletesObjective() &&
                  nullContextsReturnStructuredStatus() &&
                  openDoorEmitsOnlyAndDoesNotMutateState() &&
                  rawRetryIsInvalidCommand() &&
                  inventoryFailureDoesNotDeactivateTarget();
  return ok ? 0 : 1;
}
