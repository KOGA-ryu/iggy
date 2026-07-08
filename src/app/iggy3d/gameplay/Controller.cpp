#include "app/iggy3d/gameplay/Controller.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

#include "app/input/ActionState.hpp"
#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/gameplay/ControllerGroundQueries.hpp"
#include "app/iggy3d/gameplay/ControllerJumpActions.hpp"
#include "app/iggy3d/gameplay/ControllerJumpDashState.hpp"
#include "app/iggy3d/gameplay/ControllerKinematics.hpp"
#include "app/iggy3d/gameplay/ControllerMovementProof.hpp"
#include "app/iggy3d/gameplay/ControllerPlayerAccess.hpp"
#include "app/iggy3d/gameplay/ControllerResetFall.hpp"
#include "app/iggy3d/gameplay/ControllerTargetOutcomeProof.hpp"
#include "app/iggy3d/gameplay/ControllerWallRunEvaluation.hpp"
#include "app/iggy3d/gameplay/ControllerWallQueries.hpp"
#include "app/iggy3d/gameplay/MovementTuning.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/movement/MovementSystem.hpp"
#include "runtime/session/Session.hpp"
#include "runtime/targeting/ReachQuery.hpp"
#include "runtime/targeting/TargetQuery.hpp"
#include "runtime/world/WorldState.hpp"

namespace iggy3d {
namespace {

void submitProductGameplayCommand(Session& session,
                                  ProductAppWindowState& window,
                                  CommandRecord command,
                                  const SpatialSurfaceSet* collisionSurfaces);
TargetQueryResult queryProductGameplayTarget(const Session& session, CommandKind kind) {
  const EntityId actor = productPlayerActor(session);
  return queryTarget(TargetQueryRequest{&session.state().world, actor, false, {},
                                        kind, 0.0F, false, true});
}

void submitProductDash(Session& session,
                       ProductAppWindowState& window,
                       float moveX,
                       float moveY,
                       std::string_view source,
                       const SpatialSurfaceSet* collisionSurfaces) {
  const ProductGameplayMovementTuning& tuning = window.gameplay.gameplayMovement.tuning;
  clearProductTargetProof(window);
  clearProductOutcomeProof(window);
  window.gameplay.gameplayInputUsed = true;
  window.gameplay.gameplayInputSource = std::string{source};
  window.gameplay.gameplayDash.requested = true;

  // branch-gate: BG-1155
  if (window.gameplay.gameplayDash.cooldownRemainingSeconds > 0.0F) {
    rejectProductDash(window, "cooldown", "gameplay_dash_cooldown");
    return;
  }

  const EntityState* actor = productPlayerEntity(session);
  // branch-gate: BG-1155
  if (actor == nullptr) {
    rejectProductDash(window, "missing_player", "gameplay_dash_missing_player");
    return;
  }

  const Vec3 direction = productManualFirstPersonDirection(
      moveX, moveY, window.viewport.cameraYawDegrees);
  const float dashDistance = tuning.dashSpeedMetersPerSecond *
                             tuning.dashDurationSeconds;
  Vec3 destination = actor->transform.position + direction * dashDistance;
  destination.y = actor->transform.position.y;

  window.gameplay.gameplayDash.accepted = true;
  window.gameplay.gameplayDash.status = "accepted";
  window.gameplay.gameplayDash.reasonCode = "gameplay_dash_accepted";
  window.gameplay.gameplayDash.speedMetersPerSecond = tuning.dashSpeedMetersPerSecond;
  window.gameplay.gameplayDash.distanceMeters = dashDistance;
  window.gameplay.gameplayDash.cooldownRemainingSeconds =
      tuning.dashCooldownSeconds;
  window.gameplay.gameplayDash.directionX = direction.x;
  window.gameplay.gameplayDash.directionZ = direction.z;
  window.gameplay.gameplayMovement.profile = std::string{tuning.dashProfile};
  window.gameplay.gameplayMovement.maxSpeedMetersPerSecond =
      tuning.dashSpeedMetersPerSecond;

  CommandRecord command;
  command.playerSlot = 0;
  command.actor = actor->id;
  command.kind = CommandKind::Move;
  command.source = CommandSource::LocalPlayer;
  command.payload.target.hasPoint = true;
  command.payload.target.point = destination;
  submitProductGameplayCommand(session, window, command, collisionSurfaces);
}

bool horizontalVelocityActive(const ProductAppWindowState& window) {
  const float speedSquared =
      window.gameplay.gameplayMovement.groundVelocityX *
          window.gameplay.gameplayMovement.groundVelocityX +
      window.gameplay.gameplayMovement.groundVelocityZ *
          window.gameplay.gameplayMovement.groundVelocityZ;
  return speedSquared > 0.000001F;
}

Vec3 updateProductGroundMovementVelocity(ProductAppWindowState& window,
                                         float moveX,
                                         float moveY,
                                         bool sprinting) {
  const ProductGameplayMovementTuning& tuning = window.gameplay.gameplayMovement.tuning;
  const float dt = std::max(0.0F, tuning.inputStepSeconds);
  Vec3 current{window.gameplay.gameplayMovement.groundVelocityX,
               0.0F,
               window.gameplay.gameplayMovement.groundVelocityZ};
  const Vec3 target = productManualFirstPersonDesiredVelocity(
      moveX, moveY, window.viewport.cameraYawDegrees, sprinting, tuning);
  const bool hasIntent = target.x != 0.0F || target.z != 0.0F;
  const float rate =
      hasIntent ? tuning.groundAccelerationMetersPerSecondSquared
                : tuning.groundDecelerationMetersPerSecondSquared;  // branch-gate: BG-1161
  const float maxSpeed =
      productManualFirstPersonMaxSpeedMetersPerSecond(tuning, sprinting);
  Vec3 next = moveProductHorizontalVelocityToward(
      current, target, std::max(0.0F, rate) * dt);
  next = clampProductHorizontalVelocity(next, maxSpeed);
  window.gameplay.gameplayMovement.groundVelocityX = next.x;
  window.gameplay.gameplayMovement.groundVelocityZ = next.z;
  return next;
}

void submitProductAirborneMove(Session& session,
                               ProductAppWindowState& window,
                               const EntityState& actor,
                               float moveX,
                               float moveY,
                               bool sprinting,
                               std::string_view source) {
  window.gameplay.gameplayInputUsed = true;
  window.gameplay.gameplayInputSource = std::string{source};
  window.gameplay.gameplayCommand.submitted = false;
  window.gameplay.gameplayCommand.kind = "move";
  window.gameplay.gameplayCommand.accepted = false;
  window.gameplay.gameplayCommand.status = "airborne";
  window.gameplay.gameplayReachGate = "not_attempted";
  window.gameplay.gameplayLastRejection = "none";
  window.gameplay.gameplayTickAdvanced = false;
  window.gameplay.gameplayMovement.attempted = true;
  window.gameplay.gameplayMovement.blocked = false;
  recordProductMovementProfile(window, sprinting);

  const Vec3 start = actor.transform.position;
  Vec3 finalPosition =
      start +
      productManualFirstPersonMoveDelta(
          moveX,
          moveY,
          window.viewport.cameraYawDegrees,
          sprinting,
          window.gameplay.gameplayMovement.tuning,
          window.gameplay.gameplayMovement.tuning.airControlMultiplier);
  // branch-gate: BG-1157
  if (window.gameplay.gameplayWallRun.active) {
    Vec3 wallRunDirection;
    // branch-gate: BG-1157
    if (wallRunTangentDirection(window, moveX, moveY, wallRunDirection)) {
      const float speed =
          productManualFirstPersonMaxSpeedMetersPerSecond(
              window.gameplay.gameplayMovement.tuning, sprinting) *
          std::clamp(window.gameplay.gameplayMovement.tuning.wallRunSpeedMultiplier,
                     0.25F,
                     2.0F);
      finalPosition = start + wallRunDirection *
                                  (speed *
                                   window.gameplay.gameplayMovement.tuning.inputStepSeconds);
    } else {
      clearProductWallRunActiveProof(window, "wall_run_input_away");
    }
  }
  // branch-gate: BG-1161
  if (!setProductPlayerPosition(session, actor.id, finalPosition)) {
    window.gameplay.gameplayMovement.blocked = true;
    window.gameplay.gameplayMovement.status = "mutation_failed";
    window.gameplay.gameplayCommand.status = "mutation_failed";
    window.gameplay.gameplayMovement.reasonCode = "airborne_manual_move_mutation_failed";
    return;
  }

  window.gameplay.gameplayMovement.status = "moved";
  window.gameplay.playerPositionChanged = true;
  recordProductAirborneMovementDebug(window, start, finalPosition);
}

bool commandMovementHasPhysicsFrameStats(const Session& session,
                                         CommandKind kind) {
  // branch-gate: BG-1115
  if (kind != CommandKind::Move) {
    return false;
  }
  const SessionTransientState& transient = session.state().transient;
  return transient.lastMovementResultAvailable &&
         transient.lastMovementResult.physicsFrameStatsAvailable;
}

StatusResult tickProductGameplayCommand(Session& session,
                                        ProductAppWindowState& window,
                                        const SpatialSurfaceSet* collisionSurfaces) {
  // branch-gate: BG-1115
  if (!window.gameplay.physicsMovementPlanner.enabled) {
    recordProductPhysicsMovementPlannerTickProof(window, false,
                                                 collisionSurfaces != nullptr,
                                                 false);
    return session.tick(collisionSurfaces);
  }
  return session.tickWithOptions(SessionTickOptions{collisionSurfaces, true});
}

bool applyProductLedgeFallMoveFallback(Session& session,
                                       ProductAppWindowState& window,
                                       const CommandRecord& command,
                                       Vec3 before,
                                       const SpatialSurfaceSet* collisionSurfaces) {
  const bool ledgeBlockedReason =
      window.gameplay.gameplayMovement.blockedReason == "no_walkable_ground" ||
      window.gameplay.gameplayMovement.blockedReason == "slope_rejected";
  // branch-gate: BG-1188
  if (window.gameplay.gameplayJump.active ||
      command.kind != CommandKind::Move ||
      !command.payload.target.hasPoint ||
      !ledgeBlockedReason ||
      !playerHasNearbyGround(collisionSurfaces, before)) {
    return false;
  }

  Vec3 finalPosition = command.payload.target.point;
  finalPosition.y = before.y;
  float destinationGroundY = before.y;
  const bool hasDestinationGround =
      findHighestWalkableGroundAtOrBelow(
          collisionSurfaces, finalPosition, before.y, destinationGroundY);
  // branch-gate: BG-1188
  if (hasDestinationGround &&
      destinationGroundY >= before.y - kProductGameplayGroundContactToleranceMeters) {
    return false;
  }
  // branch-gate: BG-1188
  if (!setProductPlayerPosition(session, command.actor, finalPosition)) {
    return false;
  }

  window.gameplay.playerPositionChanged = true;
  window.gameplay.gameplayMovement.blocked = false;
  window.gameplay.gameplayMovement.status = "moved";
  recordProductLedgeFallMovementDebug(window, before, finalPosition);
  beginProductFallIfUnsupported(session, window, collisionSurfaces);
  return true;
}

void submitProductGameplayCommand(Session& session,
                                  ProductAppWindowState& window,
                                  CommandRecord command,
                                  const SpatialSurfaceSet* collisionSurfaces) {
  const EntityState* beforePlayer = productPlayerEntity(session);
  const Vec3 before = beforePlayer == nullptr ? Vec3{} : beforePlayer->transform.position;
  window.gameplay.gameplayInputUsed = true;
  window.gameplay.gameplayCommand.submitted = true;
  window.gameplay.gameplayCommand.kind = commandKindName(command.kind);
  if (command.kind == CommandKind::Move) {
    window.gameplay.gameplayMovement.attempted = true;
    window.gameplay.gameplayMovement.blocked = false;
    window.gameplay.gameplayMovement.status = "submitted";
    clearProductMovementDebug(window);
  }
  window.gameplay.gameplayCollision.surfacesUsed = collisionSurfaces != nullptr;
  window.gameplay.gameplayCollision.surfaceCount =
      collisionSurfaces == nullptr
          ? 0U
          : static_cast<std::uint64_t>(collisionSurfaces->size());
  // branch-gate: BG-1115
  if (!window.gameplay.physicsMovementPlanner.enabled) {
    recordProductPhysicsMovementPlannerTickProof(window, false,
                                                 collisionSurfaces != nullptr,
                                                 false);
  }

  const SessionCommandResult submitted = session.submitCommand(command);
  window.gameplay.gameplayCommand.accepted =
      submitted.command.admission == CommandAdmissionStatus::Accepted;
  window.gameplay.gameplayLastRejection = commandRejectionReasonName(submitted.command.rejection);
  window.gameplay.gameplayReachGate = reachGateName(submitted.command.rejection);
  window.gameplay.gameplayCommand.status = window.gameplay.gameplayCommand.accepted ? "accepted" : "rejected";

  if (window.gameplay.gameplayCommand.accepted) {
    const StatusResult tick =
        tickProductGameplayCommand(session, window, collisionSurfaces);
    window.gameplay.gameplayTickAdvanced = tick.status == ResultStatus::Ok;
    window.gameplay.gameplayTickReasonCode =
        tick.status == ResultStatus::Ok
            ? "ok"
            : (tick.error.code.empty() ? "tick_failed" : tick.error.code);
    if (command.kind == CommandKind::Move) {
      recordProductMovementDebug(session, window);
    }
    recordProductPhysicsMovementPlannerTickProof(
        window,
        window.gameplay.physicsMovementPlanner.enabled,
        collisionSurfaces != nullptr,
        commandMovementHasPhysicsFrameStats(session, command.kind));
  }

  const EntityState* afterPlayer = productPlayerEntity(session);
  bool movedThisCommand = false;
  if (afterPlayer != nullptr && beforePlayer != nullptr) {
    movedThisCommand = !nearlyEqual(before, afterPlayer->transform.position);
    window.gameplay.playerPositionChanged = window.gameplay.playerPositionChanged || movedThisCommand;
  }
  if (command.kind == CommandKind::Move && window.gameplay.gameplayCommand.accepted) {
    const bool runtimeMovementBlocked =
        window.gameplay.gameplayMovement.debugAvailable &&
        window.gameplay.gameplayMovement.blockedReason != "movement_ok";
    const bool runtimeMovementChanged = productMovementDebugChangedPosition(window);
    window.gameplay.playerPositionChanged =
        window.gameplay.playerPositionChanged || movedThisCommand || runtimeMovementChanged;
    if (!window.gameplay.gameplayTickAdvanced) {
      window.gameplay.gameplayMovement.status = "tick_failed";
    } else if (runtimeMovementBlocked) {
      window.gameplay.gameplayMovement.blocked = true;
      window.gameplay.gameplayMovement.status = "blocked";
    } else if (movedThisCommand || runtimeMovementChanged) {
      window.gameplay.gameplayMovement.status = "moved";
    } else {
      window.gameplay.gameplayMovement.blocked = true;
      window.gameplay.gameplayMovement.status = "blocked";
    }
    const bool ledgeFallFallbackApplied =
        applyProductLedgeFallMoveFallback(
            session, window, command, before, collisionSurfaces);
    // branch-gate: BG-1187
    if (!ledgeFallFallbackApplied && !window.gameplay.gameplayJump.active) {
      beginProductFallIfUnsupported(session, window, collisionSurfaces);
    }
  }
}

void submitProductMove(Session& session,
                       ProductAppWindowState& window,
                       float moveX,
                       float moveY,
                       bool sprinting,
                       std::string_view source,
                       const SpatialSurfaceSet* collisionSurfaces) {
  clearProductTargetProof(window);
  clearProductOutcomeProof(window);
  const EntityState* actor = productPlayerEntity(session);
  if (actor == nullptr) {
    window.gameplay.gameplayCommand.status = "missing_player";
    window.gameplay.gameplayMovement.groundVelocityX = 0.0F;
    window.gameplay.gameplayMovement.groundVelocityZ = 0.0F;
    return;
  }
  recordProductMovementProfile(window, sprinting);
  // Jump/fall owns vertical motion. While airborne, apply manual X/Z intent
  // directly so holding movement with jump does not get snapped back to ground.
  // branch-gate: BG-1161
  if (window.gameplay.gameplayJump.active) {
    window.gameplay.gameplayMovement.groundVelocityX = 0.0F;
    window.gameplay.gameplayMovement.groundVelocityZ = 0.0F;
    submitProductAirborneMove(session, window, *actor, moveX, moveY, sprinting, source);
    return;
  }
  const Vec3 retainedVelocity =
      updateProductGroundMovementVelocity(window, moveX, moveY, sprinting);
  const Vec3 retainedDelta =
      retainedVelocity * window.gameplay.gameplayMovement.tuning.inputStepSeconds;
  // branch-gate: BG-1161
  if (retainedDelta.x == 0.0F && retainedDelta.z == 0.0F) {
    return;
  }
  Vec3 destination = actor->transform.position;
  destination = destination + retainedDelta;

  CommandRecord command;
  command.playerSlot = 0;
  command.actor = actor->id;
  command.kind = CommandKind::Move;
  command.source = CommandSource::LocalPlayer;
  command.payload.target.hasPoint = true;
  command.payload.target.point = destination;
  window.gameplay.gameplayInputSource = std::string(source);
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
  if (!window.gameplay.targetDiscovered) {
    window.gameplay.gameplayInputUsed = true;
    window.gameplay.gameplayInputSource = std::string(source);
    window.gameplay.gameplayCommand.kind = commandKindName(kind);
    window.gameplay.gameplayCommand.status = "no_target";
    window.gameplay.gameplayReachGate = "not_attempted";
    if (kind == CommandKind::Interact) {
      window.gameplay.gameplayOutcome.status = "no_target";
    }
    return;
  }

  const ReachQueryResult reach =
      queryReach(ReachQueryRequest{&session.state().world, actor, target.target, false, {},
                                   session.state().config.interactionRangeMeters, true});
  const CommandRejectionReason reachReason = rejectionReasonForReach(reach);
  window.gameplay.gameplayReachGate = reachGateName(reachReason);

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
  window.gameplay.gameplayInputSource = std::string(source);
  submitProductGameplayCommand(session, window, command, collisionSurfaces);
  if (kind == CommandKind::Interact) {
    recordProductInteractionOutcomeProof(session, window, outcomeBefore);
  }
  if (kind == CommandKind::Interact && window.gameplay.gameplayCommand.accepted &&
      window.gameplay.gameplayTickAdvanced) {
    window.gameplay.interactionExecuted = true;
  }
  if (kind == CommandKind::Attack && window.gameplay.gameplayCommand.accepted &&
      window.gameplay.gameplayTickAdvanced) {
    window.gameplay.attackExecuted = true;
  }
}

struct ProductGameplayInputIntent {
  float moveX = 0.0F;
  float moveY = 0.0F;
  bool sprinting = false;
  bool jumpPressed = false;
  bool jumpReleased = false;
  bool dashPressed = false;
  bool interactPressed = false;
  bool attackPressed = false;
  bool resetPressed = false;
};

ProductGameplayInputIntent sampleProductGameplayInputIntent(
    const ActionState& actions) {
  ProductGameplayInputIntent intent;
  intent.moveX = actionAxisValue(actions, InputAction::PlayerMoveX);
  intent.moveY = actionAxisValue(actions, InputAction::PlayerMoveY);
  intent.sprinting = actionIsDown(actions, InputAction::PlayerSprint);
  intent.jumpPressed = actionWasPressed(actions, InputAction::PlayerJump);
  intent.jumpReleased = actionWasReleased(actions, InputAction::PlayerJump);
  intent.dashPressed = actionWasPressed(actions, InputAction::PlayerDash);
  intent.interactPressed = actionWasPressed(actions, InputAction::PlayerInteract);
  intent.attackPressed = actionWasPressed(actions, InputAction::PlayerAttack);
  intent.resetPressed =
      actionWasPressed(actions, InputAction::PlayerRetryOrReset);
  return intent;
}

bool productGameplayIntentHasMovement(
    const ProductGameplayInputIntent& intent,
    const ProductAppWindowState& window) {
  return intent.moveX != 0.0F || intent.moveY != 0.0F ||
         horizontalVelocityActive(window);
}

void updateProductJumpTimingPhase(Session& session,
                                  ProductAppWindowState& window,
                                  const ProductGameplayInputIntent& intent,
                                  std::string_view source,
                                  const SpatialSurfaceSet* collisionSurfaces) {
  advanceProductDashCooldown(window);
  // branch-gate: BG-1153
  if (intent.jumpPressed) {
    // branch-gate: BG-1157
    if (window.gameplay.gameplayWallRun.active) {
      clearProductWallRunActiveProof(window, "wall_run_exit_jump");
    }
    submitProductJump(session, window, source);
    return;
  }

  // branch-gate: BG-1153
  if (intent.jumpReleased) {
    applyProductJumpReleaseCut(window);
  }
  advanceProductJump(session, window, collisionSurfaces);
}

ProductWallRunEvaluationResult resolveProductWallRunCandidatePhase(
    const Session& session,
    const ProductAppWindowState& window,
    const ProductGameplayInputIntent& intent,
    const SpatialSurfaceSet* collisionSurfaces) {
  std::optional<Vec3> playerPosition;
  const EntityState* actor = productPlayerEntity(session);
  // branch-gate: BG-1153
  if (actor != nullptr) {
    playerPosition = actor->transform.position;
  }
  return evaluateProductWallRun(ProductWallRunEvaluationRequest{
      window,
      collisionSurfaces,
      playerPosition,
      intent.moveX,
      intent.moveY,
      intent.jumpPressed});
}

void applyProductActiveMovementStatePhase(
    ProductAppWindowState& window,
    const ProductWallRunEvaluationResult& wallRun) {
  publishProductWallRunEvaluation(window, wallRun);
}

void publishProductMovementProofPhase(
    const Session& session,
    ProductAppWindowState& window,
    const ProductGameplayInputIntent& intent,
    const SpatialSurfaceSet* collisionSurfaces) {
  updateProductMovementStateProof(window);
  const ProductWallRunEvaluationResult wallRun =
      resolveProductWallRunCandidatePhase(session, window, intent, collisionSurfaces);
  applyProductActiveMovementStatePhase(window, wallRun);
  updateProductMovementStateProof(window);
}

bool applyProductDashPhase(Session& session,
                           ProductAppWindowState& window,
                           const ProductGameplayInputIntent& intent,
                           std::string_view source,
                           const SpatialSurfaceSet* collisionSurfaces) {
  // branch-gate: BG-1155
  if (!intent.dashPressed) {
    return false;
  }
  submitProductDash(session,
                    window,
                    intent.moveX,
                    intent.moveY,
                    source,
                    collisionSurfaces);
  publishProductMovementProofPhase(session, window, intent, collisionSurfaces);
  return true;
}

void updateProductRetainedHorizontalVelocityPhase(
    Session& session,
    ProductAppWindowState& window,
    const ProductGameplayInputIntent& intent,
    std::string_view source,
    const SpatialSurfaceSet* collisionSurfaces) {
  // branch-gate: BG-1161
  if (!productGameplayIntentHasMovement(intent, window)) {
    return;
  }
  submitProductMove(session,
                    window,
                    intent.moveX,
                    intent.moveY,
                    intent.sprinting,
                    source,
                    collisionSurfaces);
}

void applyProductTargetActionPhase(Session& session,
                                   ProductAppWindowState& window,
                                   const ProductGameplayInputIntent& intent,
                                   std::string_view source,
                                   const SpatialSurfaceSet* collisionSurfaces) {
  // branch-gate: BG-1155
  if (intent.interactPressed) {
    submitProductTargetCommand(session,
                               window,
                               CommandKind::Interact,
                               source,
                               collisionSurfaces);
  }
  // branch-gate: BG-1155
  if (intent.attackPressed) {
    submitProductTargetCommand(session,
                               window,
                               CommandKind::Attack,
                               source,
                               collisionSurfaces);
  }
}

void applyProductResetActionPhase(Session& session,
                                  ProductAppWindowState& window,
                                  const ProductGameplayInputIntent& intent,
                                  std::string_view source) {
  // branch-gate: BG-1155
  if (!intent.resetPressed) {
    return;
  }
  const SessionResetResult reset = session.resetToBaseline();
  clearProductTargetProof(window);
  clearProductOutcomeProof(window);
  window.gameplay.gameplayInputUsed = true;
  window.gameplay.gameplayInputSource = std::string(source);
  window.gameplay.gameplayCommand.kind = "reset";
  window.gameplay.gameplayCommand.submitted = true;
  window.gameplay.gameplayCommand.accepted = reset.reset;
  window.gameplay.gameplayCommand.status = reset.reset ? "accepted" : "rejected";  // branch-gate: BG-1155
}

}  // namespace

void applyProductGameplayActions(Session& session,
                                 const ActionState& actions,
                                 ProductAppWindowState& window,
                                 std::string_view source,
                                 const SpatialSurfaceSet* collisionSurfaces) {
  const ProductGameplayInputIntent intent =
      sampleProductGameplayInputIntent(actions);
  updateProductJumpTimingPhase(session, window, intent, source, collisionSurfaces);
  if (applyProductDashPhase(
          session, window, intent, source, collisionSurfaces)) {
    return;
  }
  updateProductRetainedHorizontalVelocityPhase(
      session, window, intent, source, collisionSurfaces);
  applyProductTargetActionPhase(
      session, window, intent, source, collisionSurfaces);
  applyProductResetActionPhase(session, window, intent, source);
  publishProductMovementProofPhase(session, window, intent, collisionSurfaces);
}

}  // namespace iggy3d
