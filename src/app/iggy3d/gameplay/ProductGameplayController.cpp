#include "app/iggy3d/gameplay/ProductGameplayController.hpp"

#include <cmath>
#include <string>
#include <string_view>

#include "app/input/ActionState.hpp"
#include "app/iggy3d/gameplay/ProductActiveRoomCollision.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/inventory/InventorySystem.hpp"
#include "runtime/movement/MovementSystem.hpp"
#include "runtime/objective/ObjectiveSystem.hpp"
#include "runtime/session/Session.hpp"
#include "runtime/targeting/ReachQuery.hpp"
#include "runtime/targeting/TargetQuery.hpp"
#include "runtime/world/WorldState.hpp"

namespace iggy3d {
namespace {

struct ProductInteractionOutcomeSnapshot {
  EntityId target;
  PlayerSlotId playerSlot = kInvalidPlayerSlotId;
  std::string itemId;
  std::string objectiveId;
  std::uint32_t itemCountBefore = 0;
  bool objectiveCompleteBefore = false;
  std::size_t eventCountBefore = 0;
};

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

EntityId productPlayerActor(const Session& session) {
  return session.state().players.actorForSlot(0);
}

const EntityState* productPlayerEntity(const Session& session) {
  const EntityId actor = productPlayerActor(session);
  return session.state().world.findById(actor);
}

TargetQueryResult queryProductGameplayTarget(const Session& session, CommandKind kind) {
  const EntityId actor = productPlayerActor(session);
  return queryTarget(TargetQueryRequest{&session.state().world, actor, false, {},
                                        kind, 0.0F, false, true});
}

void clearProductMovementDebug(ProductAppWindowState& window) {
  window.gameplayMovementDebugAvailable = false;
  window.gameplayMovementReasonCode = "not_requested";
  window.gameplayMovementBlockedReason = "none";
  window.gameplayMovementHitSurfaceId = "none";
  window.gameplayMovementGroundSnapApplied = false;
  window.gameplayMovementClamped = false;
  window.gameplayMovementSlid = false;
  window.gameplayMovementCollisionSweepCount = 0;
  window.gameplayMovementPolicyBand = "none";
  window.gameplayMovementSlopeTravelDirection = "stationary";
  window.gameplayMovementSlopeAngleDegrees = 0.0F;
  window.gameplayMovementSpeedMultiplier = 1.0F;
  window.gameplayMovementStartX = 0.0F;
  window.gameplayMovementStartY = 0.0F;
  window.gameplayMovementStartZ = 0.0F;
  window.gameplayMovementFinalX = 0.0F;
  window.gameplayMovementFinalY = 0.0F;
  window.gameplayMovementFinalZ = 0.0F;
  window.gameplayMovementHorizontalDistanceMeters = 0.0F;
  window.gameplayMovementVerticalDeltaMeters = 0.0F;
  window.gameplayMovementGradePercent = 0.0F;
}

bool productMovementDebugChangedPosition(const ProductAppWindowState& window) {
  if (!window.gameplayMovementDebugAvailable) {
    return false;
  }
  const Vec3 start{window.gameplayMovementStartX,
                   window.gameplayMovementStartY,
                   window.gameplayMovementStartZ};
  const Vec3 final{window.gameplayMovementFinalX,
                   window.gameplayMovementFinalY,
                   window.gameplayMovementFinalZ};
  return !nearlyEqual(start, final);
}

void clearProductTargetProof(ProductAppWindowState& window) {
  window.targetDiscovered = false;
  window.gameplayTargetStatus = "not_requested";
  window.gameplayTargetAction = "none";
  window.gameplayTargetEntityId = 0;
  window.gameplayTargetStableName = "none";
  window.gameplayTargetKind = "none";
  window.gameplayTargetDistanceMeters = 0.0F;
  window.gameplayTargetSupportsCommand = false;
}

void clearProductOutcomeProof(ProductAppWindowState& window) {
  window.gameplayOutcomeStatus = "not_requested";
  window.gameplayOutcomeTargetActiveAfter = false;
  window.gameplayOutcomeInventoryChanged = false;
  window.gameplayOutcomeItemId = "none";
  window.gameplayOutcomeItemCount = 0;
  window.gameplayOutcomeObjectiveChanged = false;
  window.gameplayOutcomeEventCount = 0;
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
  window.gameplayOutcomeTargetActiveAfter =
      target != nullptr && target->active;
  window.gameplayOutcomeItemId = before.itemId.empty() ? "none" : before.itemId;
  const std::uint32_t itemCountAfter =
      inventoryItemCount(session.state().inventory,
                         before.playerSlot,
                         before.itemId);
  window.gameplayOutcomeItemCount = itemCountAfter;
  window.gameplayOutcomeInventoryChanged =
      itemCountAfter != before.itemCountBefore;
  const bool objectiveCompleteAfter =
      !before.objectiveId.empty() &&
      objectiveComplete(session.state().objectives, before.objectiveId);
  window.gameplayOutcomeObjectiveChanged =
      objectiveCompleteAfter != before.objectiveCompleteBefore;
  const std::size_t eventCountAfter = session.state().transient.events.size();
  window.gameplayOutcomeEventCount =
      eventCountAfter >= before.eventCountBefore
          ? static_cast<std::uint64_t>(eventCountAfter - before.eventCountBefore)
          : 0U;

  if (!window.gameplayCommandAccepted) {
    window.gameplayOutcomeStatus = "rejected";
    return;
  }
  if (!window.gameplayTickAdvanced) {
    window.gameplayOutcomeStatus = "tick_failed";
    return;
  }
  window.gameplayOutcomeStatus = "succeeded";
}

void recordProductTargetProof(const Session& session,
                              ProductAppWindowState& window,
                              CommandKind kind,
                              const TargetQueryResult& target) {
  window.targetDiscovered = target.status == TargetQueryStatus::Found;
  window.gameplayTargetStatus = targetQueryStatusName(target.status);
  window.gameplayTargetAction = commandKindName(kind);
  window.gameplayTargetEntityId = toUint64(target.target);
  window.gameplayTargetStableName = "none";
  window.gameplayTargetKind = "none";
  window.gameplayTargetDistanceMeters = target.distanceMeters;
  window.gameplayTargetSupportsCommand = target.targetSupportsCommand;

  if (!window.targetDiscovered) {
    window.gameplayTargetEntityId = 0;
    window.gameplayTargetDistanceMeters = 0.0F;
    window.gameplayTargetSupportsCommand = false;
    return;
  }

  const EntityState* entity = session.state().world.findById(target.target);
  if (entity == nullptr) {
    window.gameplayTargetStatus = "found_missing_entity";
    return;
  }
  window.gameplayTargetStableName =
      entity->stableName.empty() ? "none" : entity->stableName;
  window.gameplayTargetKind = entityKindName(entity->kind);
}

void recordProductMovementDebug(const Session& session, ProductAppWindowState& window) {
  const SessionTransientState& transient = session.state().transient;
  if (!transient.lastMovementResultAvailable) {
    clearProductMovementDebug(window);
    return;
  }

  const MovementResult& movement = transient.lastMovementResult;
  window.gameplayMovementDebugAvailable = true;
  window.gameplayMovementReasonCode =
      movement.reasonCode.empty() ? movementBlockedReasonName(movement.blocked)
                                  : movement.reasonCode;
  window.gameplayMovementBlockedReason = movementBlockedReasonName(movement.blocked);
  window.gameplayMovementHitSurfaceId =
      movement.hitSurfaceId.empty() ? "none" : movement.hitSurfaceId;
  window.gameplayMovementGroundSnapApplied = movement.groundSnapApplied;
  window.gameplayMovementClamped = movement.movementClamped;
  window.gameplayMovementSlid = movement.movementSlid;
  window.gameplayMovementCollisionSweepCount = movement.collisionSweepCount;
  window.gameplayMovementPolicyBand =
      movement.movementPolicyBand.empty() ? "none" : movement.movementPolicyBand;
  window.gameplayMovementSlopeTravelDirection = movement.slopeTravelDirection;
  window.gameplayMovementSlopeAngleDegrees = movement.slopeAngleDegrees;
  window.gameplayMovementSpeedMultiplier = movement.speedMultiplier;
  window.gameplayMovementStartX = movement.start.x;
  window.gameplayMovementStartY = movement.start.y;
  window.gameplayMovementStartZ = movement.start.z;
  window.gameplayMovementFinalX = movement.finalPosition.x;
  window.gameplayMovementFinalY = movement.finalPosition.y;
  window.gameplayMovementFinalZ = movement.finalPosition.z;
  window.gameplayMovementHorizontalDistanceMeters = movement.horizontalDistanceMeters;
  window.gameplayMovementVerticalDeltaMeters = movement.verticalDeltaMeters;
  window.gameplayMovementGradePercent = movement.gradePercent;
}

void submitProductGameplayCommand(Session& session,
                                  ProductAppWindowState& window,
                                  CommandRecord command,
                                  const SpatialSurfaceSet* collisionSurfaces) {
  const EntityState* beforePlayer = productPlayerEntity(session);
  const Vec3 before = beforePlayer == nullptr ? Vec3{} : beforePlayer->transform.position;
  window.gameplayInputUsed = true;
  window.gameplayCommandSubmitted = true;
  window.gameplayCommandKind = commandKindName(command.kind);
  if (command.kind == CommandKind::Move) {
    window.gameplayMovementAttempted = true;
    window.gameplayMovementBlocked = false;
    window.gameplayMovementStatus = "submitted";
    clearProductMovementDebug(window);
  }
  window.gameplayCollisionSurfacesUsed = collisionSurfaces != nullptr;
  window.gameplayCollisionSurfaceCount =
      collisionSurfaces == nullptr
          ? 0U
          : static_cast<std::uint64_t>(collisionSurfaces->size());

  const SessionCommandResult submitted = session.submitCommand(command);
  window.gameplayCommandAccepted =
      submitted.command.admission == CommandAdmissionStatus::Accepted;
  window.gameplayLastRejection = commandRejectionReasonName(submitted.command.rejection);
  window.gameplayReachGate = reachGateName(submitted.command.rejection);
  window.gameplayCommandStatus = window.gameplayCommandAccepted ? "accepted" : "rejected";

  if (window.gameplayCommandAccepted) {
    const StatusResult tick = session.tick(collisionSurfaces);
    window.gameplayTickAdvanced = tick.status == ResultStatus::Ok;
    window.gameplayTickReasonCode =
        tick.status == ResultStatus::Ok
            ? "ok"
            : (tick.error.code.empty() ? "tick_failed" : tick.error.code);
    if (command.kind == CommandKind::Move) {
      recordProductMovementDebug(session, window);
    }
  }

  const EntityState* afterPlayer = productPlayerEntity(session);
  bool movedThisCommand = false;
  if (afterPlayer != nullptr && beforePlayer != nullptr) {
    movedThisCommand = !nearlyEqual(before, afterPlayer->transform.position);
    window.playerPositionChanged = window.playerPositionChanged || movedThisCommand;
  }
  if (command.kind == CommandKind::Move && window.gameplayCommandAccepted) {
    const bool runtimeMovementBlocked =
        window.gameplayMovementDebugAvailable &&
        window.gameplayMovementBlockedReason != "movement_ok";
    const bool runtimeMovementChanged = productMovementDebugChangedPosition(window);
    window.playerPositionChanged =
        window.playerPositionChanged || movedThisCommand || runtimeMovementChanged;
    if (!window.gameplayTickAdvanced) {
      window.gameplayMovementStatus = "tick_failed";
    } else if (runtimeMovementBlocked) {
      window.gameplayMovementBlocked = true;
      window.gameplayMovementStatus = "blocked";
    } else if (movedThisCommand || runtimeMovementChanged) {
      window.gameplayMovementStatus = "moved";
    } else {
      window.gameplayMovementBlocked = true;
      window.gameplayMovementStatus = "blocked";
    }
  }
  window.runtimeStateHash = session.stateHash();
}

void submitProductMove(Session& session,
                       ProductAppWindowState& window,
                       float moveX,
                       float moveY,
                       std::string_view source,
                       const SpatialSurfaceSet* collisionSurfaces) {
  clearProductTargetProof(window);
  clearProductOutcomeProof(window);
  const EntityState* actor = productPlayerEntity(session);
  if (actor == nullptr) {
    window.gameplayCommandStatus = "missing_player";
    return;
  }
  if (moveX == 0.0F && moveY == 0.0F) {
    return;
  }
  const float magnitude = std::sqrt(moveX * moveX + moveY * moveY);
  const float scale = magnitude > 1.0F ? 1.0F / magnitude : 1.0F;
  constexpr float kStepMeters = 0.5F;
  Vec3 destination = actor->transform.position;
  destination.x += moveX * scale * kStepMeters;
  destination.z += moveY * scale * kStepMeters;

  CommandRecord command;
  command.playerSlot = 0;
  command.actor = actor->id;
  command.kind = CommandKind::Move;
  command.source = CommandSource::LocalPlayer;
  command.payload.target.hasPoint = true;
  command.payload.target.point = destination;
  window.gameplayInputSource = std::string(source);
  submitProductGameplayCommand(session, window, command, collisionSurfaces);
}

void submitProductTargetCommand(Session& session,
                                ProductAppWindowState& window,
                                CommandKind kind,
                                std::string_view source,
                                const SpatialSurfaceSet* collisionSurfaces) {
  clearProductOutcomeProof(window);
  const EntityId actor = productPlayerActor(session);
  const TargetQueryResult target = queryProductGameplayTarget(session, kind);
  recordProductTargetProof(session, window, kind, target);
  if (!window.targetDiscovered) {
    window.gameplayInputUsed = true;
    window.gameplayInputSource = std::string(source);
    window.gameplayCommandKind = commandKindName(kind);
    window.gameplayCommandStatus = "no_target";
    window.gameplayReachGate = "not_attempted";
    if (kind == CommandKind::Interact) {
      window.gameplayOutcomeStatus = "no_target";
    }
    return;
  }

  const ReachQueryResult reach =
      queryReach(ReachQueryRequest{&session.state().world, actor, target.target, false, {},
                                   session.state().config.interactionRangeMeters, true});
  const CommandRejectionReason reachReason = rejectionReasonForReach(reach);
  window.gameplayReachGate = reachGateName(reachReason);

  CommandRecord command;
  command.playerSlot = 0;
  command.actor = actor;
  command.kind = kind;
  command.source = CommandSource::LocalPlayer;
  command.payload.target.hasEntity = true;
  command.payload.target.entity = target.target;
  if (kind == CommandKind::Attack) {
    command.payload.attackDamage = 3;
  }
  const ProductInteractionOutcomeSnapshot outcomeBefore =
      kind == CommandKind::Interact
          ? makeProductInteractionOutcomeSnapshot(session,
                                                  command.playerSlot,
                                                  target.target)
          : ProductInteractionOutcomeSnapshot{};
  window.gameplayInputSource = std::string(source);
  submitProductGameplayCommand(session, window, command, collisionSurfaces);
  if (kind == CommandKind::Interact) {
    recordProductInteractionOutcomeProof(session, window, outcomeBefore);
  }
  if (kind == CommandKind::Interact && window.gameplayCommandAccepted &&
      window.gameplayTickAdvanced) {
    window.interactionExecuted = true;
    if (window.activeRoom.loaded) {
      window.activeRoomCollision =
          buildProductActiveRoomCollision(window.activeRoom, session.state());
    }
  }
  if (kind == CommandKind::Attack && window.gameplayCommandAccepted &&
      window.gameplayTickAdvanced) {
    window.attackExecuted = true;
  }
}

}  // namespace

void applyProductGameplayActions(Session& session,
                                 const ActionState& actions,
                                 ProductAppWindowState& window,
                                 std::string_view source,
                                 const SpatialSurfaceSet* collisionSurfaces) {
  const float moveX = actionAxisValue(actions, InputAction::PlayerMoveX);
  const float moveY = actionAxisValue(actions, InputAction::PlayerMoveY);
  if (moveX != 0.0F || moveY != 0.0F) {
    submitProductMove(session, window, moveX, moveY, source, collisionSurfaces);
  }
  if (actionWasPressed(actions, InputAction::PlayerInteract)) {
    submitProductTargetCommand(session, window, CommandKind::Interact, source,
                               collisionSurfaces);
  }
  if (actionWasPressed(actions, InputAction::PlayerAttack)) {
    submitProductTargetCommand(session, window, CommandKind::Attack, source,
                               collisionSurfaces);
  }
  if (actionWasPressed(actions, InputAction::PlayerRetryOrReset)) {
    const SessionResetResult reset = session.resetToBaseline();
    clearProductTargetProof(window);
    clearProductOutcomeProof(window);
    window.gameplayInputUsed = true;
    window.gameplayInputSource = std::string(source);
    window.gameplayCommandKind = "reset";
    window.gameplayCommandSubmitted = true;
    window.gameplayCommandAccepted = reset.reset;
    window.gameplayCommandStatus = reset.reset ? "accepted" : "rejected";
    window.runtimeStateHash = session.stateHash();
  }
}

}  // namespace iggy3d
