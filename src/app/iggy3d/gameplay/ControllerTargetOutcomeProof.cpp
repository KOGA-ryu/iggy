#include "app/iggy3d/gameplay/ControllerTargetOutcomeProof.hpp"

#include "app/iggy3d/ProductAppWindowState.hpp"
#include "runtime/inventory/InventorySystem.hpp"
#include "runtime/objective/ObjectiveSystem.hpp"
#include "runtime/session/Session.hpp"
#include "runtime/world/WorldState.hpp"

namespace iggy3d {
namespace {

std::string targetQueryStatusName(TargetQueryStatus status) {
  switch (status) {
    case TargetQueryStatus::Found:
      return "found";
    case TargetQueryStatus::NotFound:
      return "not_found";
    case TargetQueryStatus::InvalidWorld:
      return "invalid_world";
    case TargetQueryStatus::InvalidActor:
      return "invalid_actor";
    case TargetQueryStatus::InvalidOrigin:
      return "invalid_origin";
  }
  return "unknown";
}

std::string entityKindName(EntityKind kind) {
  switch (kind) {
    case EntityKind::Player:
      return "player";
    case EntityKind::Pickup:
      return "pickup";
    case EntityKind::Door:
      return "door";
    case EntityKind::Marker:
      return "marker";
    case EntityKind::Npc:
      return "npc";
    case EntityKind::Unknown:
      break;
  }
  return "unknown";
}

std::uint32_t inventoryItemCount(const InventoryState& inventory,
                                 PlayerSlotId playerSlot,
                                 const std::string& itemId) {
  if (itemId.empty()) {
    return 0;
  }
  const PlayerInventory* playerInventory = findInventory(inventory, playerSlot);
  if (playerInventory == nullptr) {
    return 0;
  }
  for (const InventoryStack& stack : playerInventory->stacks) {
    if (stack.itemId == itemId) {
      return stack.count;
    }
  }
  return 0;
}

}  // namespace

std::string commandKindName(CommandKind kind) {
  switch (kind) {
    case CommandKind::Move:
      return "move";
    case CommandKind::Interact:
      return "interact";
    case CommandKind::Attack:
      return "attack";
    case CommandKind::Retry:
      return "retry";
    case CommandKind::Reset:
      return "reset";
    case CommandKind::None:
      return "none";
    default:
      break;
  }
  return "other";
}

std::string commandRejectionReasonName(CommandRejectionReason reason) {
  switch (reason) {
    case CommandRejectionReason::None:
      return "none";
    case CommandRejectionReason::InvalidTarget:
      return "invalid_target";
    case CommandRejectionReason::OutOfRange:
      return "out_of_range";
    case CommandRejectionReason::TargetNotReachable:
      return "target_not_reachable";
    case CommandRejectionReason::InvalidDamage:
      return "invalid_damage";
    case CommandRejectionReason::TargetDefeated:
      return "target_defeated";
    case CommandRejectionReason::FriendlyFireBlocked:
      return "friendly_fire_blocked";
    case CommandRejectionReason::InvalidActor:
      return "invalid_actor";
    case CommandRejectionReason::RequiredItemMissing:
      return "required_item_missing";
    default:
      break;
  }
  return "rejected";
}

std::string reachGateName(CommandRejectionReason reason) {
  if (reason == CommandRejectionReason::None) {
    return "pass";
  }
  if (reason == CommandRejectionReason::OutOfRange ||
      reason == CommandRejectionReason::TargetNotReachable) {
    return "fail";
  }
  return "not_attempted";
}

void clearProductTargetProof(ProductAppWindowState& window) {
  window.gameplay.targetDiscovered = false;
  window.gameplay.gameplayTarget.status = "not_requested";
  window.gameplay.gameplayTarget.action = "none";
  window.gameplay.gameplayTarget.entityId = 0;
  window.gameplay.gameplayTarget.stableName = "none";
  window.gameplay.gameplayTarget.kind = "none";
  window.gameplay.gameplayTarget.distanceMeters = 0.0F;
  window.gameplay.gameplayTarget.supportsCommand = false;
}

void clearProductOutcomeProof(ProductAppWindowState& window) {
  window.gameplay.gameplayOutcome.status = "not_requested";
  window.gameplay.gameplayOutcome.targetActiveAfter = false;
  window.gameplay.gameplayOutcome.inventoryChanged = false;
  window.gameplay.gameplayOutcome.itemId = "none";
  window.gameplay.gameplayOutcome.itemCount = 0;
  window.gameplay.gameplayOutcome.objectiveChanged = false;
  window.gameplay.gameplayOutcome.eventCount = 0;
}

ProductInteractionOutcomeSnapshot makeProductInteractionOutcomeSnapshot(
    const Session& session,
    PlayerSlotId playerSlot,
    EntityId target) {
  ProductInteractionOutcomeSnapshot snapshot;
  snapshot.target = target;
  snapshot.playerSlot = playerSlot;
  snapshot.eventCountBefore = session.state().transient.events.size();
  const EntityState* entity = session.state().world.findById(target);
  if (entity == nullptr) {
    return snapshot;
  }
  snapshot.itemId = entity->interaction.itemId;
  snapshot.objectiveId = entity->interaction.objectiveId;
  snapshot.itemCountBefore =
      inventoryItemCount(session.state().inventory, playerSlot, snapshot.itemId);
  snapshot.objectiveCompleteBefore =
      !snapshot.objectiveId.empty() &&
      objectiveComplete(session.state().objectives, snapshot.objectiveId);
  return snapshot;
}

void recordProductInteractionOutcomeProof(
    const Session& session,
    ProductAppWindowState& window,
    const ProductInteractionOutcomeSnapshot& before) {
  const EntityState* target = session.state().world.findById(before.target);
  window.gameplay.gameplayOutcome.targetActiveAfter =
      target != nullptr && target->active;
  window.gameplay.gameplayOutcome.itemId = before.itemId.empty() ? "none" : before.itemId;
  const std::uint32_t itemCountAfter =
      inventoryItemCount(session.state().inventory,
                         before.playerSlot,
                         before.itemId);
  window.gameplay.gameplayOutcome.itemCount = itemCountAfter;
  window.gameplay.gameplayOutcome.inventoryChanged =
      itemCountAfter != before.itemCountBefore;
  const bool objectiveCompleteAfter =
      !before.objectiveId.empty() &&
      objectiveComplete(session.state().objectives, before.objectiveId);
  window.gameplay.gameplayOutcome.objectiveChanged =
      objectiveCompleteAfter != before.objectiveCompleteBefore;
  const std::size_t eventCountAfter = session.state().transient.events.size();
  window.gameplay.gameplayOutcome.eventCount =
      eventCountAfter >= before.eventCountBefore
          ? static_cast<std::uint64_t>(eventCountAfter - before.eventCountBefore)
          : 0U;

  if (!window.gameplay.gameplayCommand.accepted) {
    window.gameplay.gameplayOutcome.status = "rejected";
    return;
  }
  if (!window.gameplay.gameplayTickAdvanced) {
    window.gameplay.gameplayOutcome.status = "tick_failed";
    return;
  }
  window.gameplay.gameplayOutcome.status = "succeeded";
}

void recordProductTargetProof(const Session& session,
                              ProductAppWindowState& window,
                              CommandKind kind,
                              const TargetQueryResult& target) {
  window.gameplay.targetDiscovered = target.status == TargetQueryStatus::Found;
  window.gameplay.gameplayTarget.status = targetQueryStatusName(target.status);
  window.gameplay.gameplayTarget.action = commandKindName(kind);
  window.gameplay.gameplayTarget.entityId = toUint64(target.target);
  window.gameplay.gameplayTarget.stableName = "none";
  window.gameplay.gameplayTarget.kind = "none";
  window.gameplay.gameplayTarget.distanceMeters = target.distanceMeters;
  window.gameplay.gameplayTarget.supportsCommand = target.targetSupportsCommand;

  if (!window.gameplay.targetDiscovered) {
    window.gameplay.gameplayTarget.entityId = 0;
    window.gameplay.gameplayTarget.distanceMeters = 0.0F;
    window.gameplay.gameplayTarget.supportsCommand = false;
    return;
  }

  const EntityState* entity = session.state().world.findById(target.target);
  if (entity == nullptr) {
    window.gameplay.gameplayTarget.status = "found_missing_entity";
    return;
  }
  window.gameplay.gameplayTarget.stableName =
      entity->stableName.empty() ? "none" : entity->stableName;
  window.gameplay.gameplayTarget.kind = entityKindName(entity->kind);
}

}  // namespace iggy3d
