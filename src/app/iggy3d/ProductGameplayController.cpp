#include "app/iggy3d/ProductGameplayController.hpp"

#include <cmath>
#include <string>
#include <string_view>

#include "app/input/ActionState.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/movement/MovementSystem.hpp"
#include "runtime/session/Session.hpp"
#include "runtime/targeting/ReachQuery.hpp"
#include "runtime/targeting/TargetQuery.hpp"
#include "runtime/world/WorldState.hpp"

namespace iggy3d {
namespace {

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
    if (!window.gameplayTickAdvanced) {
      window.gameplayMovementStatus = "tick_failed";
    } else if (runtimeMovementBlocked) {
      window.gameplayMovementBlocked = true;
      window.gameplayMovementStatus = "blocked";
    } else if (movedThisCommand) {
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
  constexpr float kStepMeters = 1.0F;
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
  const EntityId actor = productPlayerActor(session);
  const TargetQueryResult target = queryProductGameplayTarget(session, kind);
  window.targetDiscovered = target.status == TargetQueryStatus::Found;
  if (!window.targetDiscovered) {
    window.gameplayInputUsed = true;
    window.gameplayInputSource = std::string(source);
    window.gameplayCommandKind = commandKindName(kind);
    window.gameplayCommandStatus = "no_target";
    window.gameplayReachGate = "not_attempted";
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
  window.gameplayInputSource = std::string(source);
  submitProductGameplayCommand(session, window, command, collisionSurfaces);
  if (kind == CommandKind::Interact && window.gameplayCommandAccepted) {
    window.interactionExecuted = true;
  }
  if (kind == CommandKind::Attack && window.gameplayCommandAccepted) {
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
