#include "app/iggy3d/gameplay/ControllerCommandExecution.hpp"

#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/gameplay/ControllerGroundQueries.hpp"
#include "app/iggy3d/gameplay/ControllerProof.hpp"
#include "app/iggy3d/gameplay/ControllerPlayerAccess.hpp"
#include "app/iggy3d/gameplay/ControllerResetFall.hpp"
#include "app/iggy3d/gameplay/ControllerTargeting.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/session/Session.hpp"
#include "runtime/world/EntityState.hpp"

#include <cstdint>

namespace iggy3d {
namespace {

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

}  // namespace

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

}  // namespace iggy3d
