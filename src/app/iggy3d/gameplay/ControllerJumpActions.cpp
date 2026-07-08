#include "app/iggy3d/gameplay/ControllerJumpActions.hpp"

#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollision.hpp"
#include "app/iggy3d/gameplay/ControllerGroundQueries.hpp"
#include "app/iggy3d/gameplay/ControllerJumpDashState.hpp"
#include "app/iggy3d/gameplay/ControllerKinematics.hpp"
#include "app/iggy3d/gameplay/ControllerPlayerAccess.hpp"
#include "app/iggy3d/gameplay/ControllerResetFall.hpp"
#include "app/iggy3d/gameplay/ControllerTargetOutcomeProof.hpp"
#include "app/iggy3d/gameplay/ControllerTraversalProof.hpp"
#include "app/iggy3d/gameplay/ControllerWallRunEvaluation.hpp"
#include "app/iggy3d/gameplay/ControllerWallQueries.hpp"
#include "app/iggy3d/gameplay/MovementTuning.hpp"
#include "app/iggy3d/gameplay/ProductRoomStore.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/movement/MovementTraversal.hpp"
#include "runtime/replay/StateHash.hpp"
#include "runtime/session/Session.hpp"
#include "runtime/world/WorldState.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <string>

namespace iggy3d {
namespace {

void beginProductJumpArc(Session& session,
                         ProductAppWindowState& window,
                         const EntityState& actor,
                         float groundY,
                         std::string_view reason) {
  const ProductGameplayMovementTuning& tuning = window.gameplay.gameplayMovement.tuning;
  window.gameplay.gameplayJump.accepted = true;
  window.gameplay.gameplayJump.active = true;
  window.gameplay.gameplayJump.velocityMetersPerSecond =
      tuning.jumpImpulseMetersPerSecond;
  window.gameplay.gameplayJump.coyoteSecondsRemaining = 0.0F;
  window.gameplay.gameplayJump.bufferSecondsRemaining = 0.0F;
  window.gameplay.gameplayJump.held = true;
  window.gameplay.gameplayJump.cutApplied = false;
  recordProductJumpPosition(window, groundY, actor.transform.position.y,
                            actor.transform.position.y);
  window.gameplay.gameplayJump.status = "accepted";
  window.gameplay.gameplayJump.reasonCode = std::string{reason};
  advanceProductJump(session,
                     window,
                     productActiveRoomCollisionSurfaces(activeRoomCollision(window)));
}

bool tryProductCoyoteJump(Session& session, ProductAppWindowState& window) {
  // branch-gate: BG-1153
  if (!window.gameplay.gameplayJump.active ||
      window.gameplay.gameplayJump.coyoteSecondsRemaining <= 0.0F) {
    return false;
  }
  const EntityState* actor = productPlayerEntity(session);
  // branch-gate: BG-1153
  if (actor == nullptr) {
    rejectProductJump(window, "missing_player", "gameplay_jump_missing_player");
    return true;
  }
  beginProductJumpArc(session,
                      window,
                      *actor,
                      window.gameplay.gameplayJump.groundY,
                      "gameplay_jump_coyote");
  return true;
}

bool tryProductWallJump(Session& session, ProductAppWindowState& window) {
  const ProductGameplayMovementTuning& tuning = window.gameplay.gameplayMovement.tuning;
  const SpatialSurfaceSet* surfaces =
      productActiveRoomCollisionSurfaces(activeRoomCollision(window));
  const EntityId actor = productPlayerActor(session);
  const EntityState* entity = session.state().world.findById(actor);
  // branch-gate: BG-1157
  if (surfaces == nullptr || entity == nullptr) {
    return false;
  }

  const Vec3 start = entity->transform.position;
  // branch-gate: BG-1157
  if (!window.gameplay.gameplayJump.active &&
      start.y <= tuning.wallJumpMinAirborneHeightMeters) {
    return false;
  }

  Vec3 awayNormal;
  const CollisionSurfaceView* surface =
      findWallJumpSurface(*surfaces, start, awayNormal, tuning);
  // branch-gate: BG-1157
  if (surface == nullptr) {
    return false;
  }

  Vec3 finalPosition = start + awayNormal * tuning.wallJumpPushMeters;
  finalPosition.y = start.y + tuning.wallJumpRiseMeters;
  // branch-gate: BG-1157
  if (!setProductPlayerPosition(session, actor, finalPosition)) {
    return false;
  }

  recordProductWallJumpTraversalProof(window, *surface, start, finalPosition);
  window.gameplay.gameplayJump.accepted = true;
  window.gameplay.gameplayJump.active = true;
  window.gameplay.gameplayJump.velocityMetersPerSecond =
      tuning.jumpImpulseMetersPerSecond;
  recordProductJumpPosition(window, 0.0F, start.y, finalPosition.y);
  window.gameplay.gameplayJump.status = "wall_jump";
  window.gameplay.gameplayJump.reasonCode = "gameplay_jump_wall_jump";
  window.gameplay.playerPositionChanged = true;
  return true;
}

bool tryProductTraversalJump(Session& session, ProductAppWindowState& window) {
  clearProductTraversalProof(window);
  const SpatialSurfaceSet* surfaces =
      productActiveRoomCollisionSurfaces(activeRoomCollision(window));
  // branch-gate: BG-1156
  if (!activeRoom(window).loaded || surfaces == nullptr) {
    return false;
  }

  TraversalIntentRequest request;
  request.actor = productPlayerActor(session);
  request.jumpPressed = true;
  request.forward = productManualFirstPersonDirection(
      0.0F, 1.0F, window.viewport.cameraYawDegrees);
  request.room = &activeRoom(window).room;
  request.collisionSurfaces = surfaces;

  SessionState& state = session.mutableStateForOwnedSystems();
  const TraversalIntentResult result = executeTraversalIntent(state.world, request);
  // If the player is already on top of a clamberable wall, the clamber slot is
  // height-rejected. Do not consume jump in that state; let normal jump run.
  // branch-gate: BG-1160
  if (result.status == TraversalIntentStatus::TraversalRejected &&
      result.selectedMechanic == TraversalMechanic::Clamber &&
      result.traversal.status == TraversalStatus::HeightRejected) {
    return false;
  }
  recordProductTraversalProof(window, result);
  // branch-gate: BG-1156
  if (result.traversalAttempted) {
    state.currentStateHash = computeStateHash(state);
  }
  // branch-gate: BG-1156
  if (result.accepted) {
    window.gameplay.playerPositionChanged = true;
  }
  return result.consumedInput;
}

}  // namespace

void advanceProductJump(Session& session,
                        ProductAppWindowState& window,
                        const SpatialSurfaceSet* collisionSurfaces) {
  const ProductGameplayMovementTuning& tuning = window.gameplay.gameplayMovement.tuning;
  const float dt = std::max(0.0F, tuning.inputStepSeconds);
  window.gameplay.gameplayJump.bufferSecondsRemaining =
      std::max(0.0F, window.gameplay.gameplayJump.bufferSecondsRemaining - dt);
  window.gameplay.gameplayJump.coyoteSecondsRemaining =
      std::max(0.0F, window.gameplay.gameplayJump.coyoteSecondsRemaining - dt);
  // branch-gate: BG-1181
  if (applyProductGameplayResetIfNeeded(session, window, collisionSurfaces)) {
    return;
  }
  // branch-gate: BG-1173
  if (!window.gameplay.gameplayJump.active &&
      !beginProductFallIfUnsupported(session, window, collisionSurfaces)) {
    return;
  }

  const EntityId actor = productPlayerActor(session);
  const EntityState* entity = session.state().world.findById(actor);
  // branch-gate: BG-1153
  if (entity == nullptr) {
    window.gameplay.gameplayJump.active = false;
    window.gameplay.gameplayJump.velocityMetersPerSecond = 0.0F;
    window.gameplay.gameplayJump.status = "missing_player";
    window.gameplay.gameplayJump.reasonCode = "gameplay_jump_missing_player";
    return;
  }

  const float previousY = entity->transform.position.y;
  float gravityMultiplier = 1.0F;
  // branch-gate: BG-1153
  if (window.gameplay.gameplayJump.velocityMetersPerSecond <= 0.0F) {
    // branch-gate: BG-1157
    gravityMultiplier =
        window.gameplay.gameplayWallRun.active
            ? std::clamp(tuning.wallRunGravityMultiplier, 0.0F, 1.0F)
            : tuning.fallGravityMultiplier;
  }
  const float gravity =
      tuning.gravityMetersPerSecondSquared *
      std::clamp(gravityMultiplier, 0.0F, 4.0F);
  const float nextVelocity =
      window.gameplay.gameplayJump.velocityMetersPerSecond - gravity * dt;
  float nextY = previousY +
                window.gameplay.gameplayJump.velocityMetersPerSecond * dt -
                0.5F * gravity * dt * dt;
  bool landed = false;
  float landingY = window.gameplay.gameplayJump.groundY;
  const bool collisionLanding =
      nextVelocity <= 0.0F &&
      findHighestWalkableGroundAtOrBelow(
          collisionSurfaces,
          entity->transform.position,
          previousY,
          landingY) &&
      nextY <= landingY + kProductGameplayGroundContactToleranceMeters;
  if ((collisionSurfaces == nullptr && nextY <= window.gameplay.gameplayJump.groundY &&
       nextVelocity <= 0.0F) ||
      collisionLanding) {
    nextY = landingY;
    landed = true;
  }

  Vec3 position = entity->transform.position;
  position.y = nextY;
  // branch-gate: BG-1153
  if (!setProductPlayerPosition(session, actor, position)) {
    window.gameplay.gameplayJump.active = false;
    window.gameplay.gameplayJump.velocityMetersPerSecond = 0.0F;
    window.gameplay.gameplayJump.status = "mutation_failed";
    window.gameplay.gameplayJump.reasonCode = "gameplay_jump_mutation_failed";
    return;
  }

  window.gameplay.playerPositionChanged =
      window.gameplay.playerPositionChanged || std::fabs(previousY - nextY) > 0.0001F;
  window.gameplay.gameplayJump.active = !landed;
  // branch-gate: BG-1153
  window.gameplay.gameplayJump.velocityMetersPerSecond = landed ? 0.0F : nextVelocity;
  // branch-gate: BG-1153
  if (landed) {
    clearProductWallRunActiveProof(window, "wall_run_landed");
    window.gameplay.gameplayJump.coyoteSecondsRemaining = 0.0F;
    window.gameplay.gameplayJump.held = false;
    window.gameplay.gameplayJump.cutApplied = false;
  }
  const std::array<float, 2U> groundProofYs{window.gameplay.gameplayJump.groundY, landingY};
  recordProductJumpPosition(
      window,
      groundProofYs[static_cast<std::size_t>(landed || collisionLanding)],
      window.gameplay.gameplayJump.startY,
      nextY);
  // branch-gate: BG-1153
  window.gameplay.gameplayJump.status = landed ? "landed" : "airborne";
  // branch-gate: BG-1153
  window.gameplay.gameplayJump.reasonCode =
      landed ? "gameplay_jump_landed" : "gameplay_jump_airborne";
  // branch-gate: BG-1153
  if (landed && productJumpBufferLive(window)) {
    const EntityState* landedEntity = productPlayerEntity(session);
    // branch-gate: BG-1153
    if (landedEntity != nullptr) {
      beginProductJumpArc(session,
                          window,
                          *landedEntity,
                          landingY,
                          "gameplay_jump_buffered");
      return;
    }
  }
  applyProductGameplayResetIfNeeded(session, window, collisionSurfaces);
}

void submitProductJump(Session& session,
                       ProductAppWindowState& window,
                       std::string_view source) {
  clearProductTargetProof(window);
  clearProductOutcomeProof(window);
  window.gameplay.gameplayInputUsed = true;
  window.gameplay.gameplayInputSource = std::string{source};
  window.gameplay.gameplayJump.requested = true;

  // branch-gate: BG-1156
  if (tryProductTraversalJump(session, window)) {
    window.gameplay.gameplayJump.accepted = false;
    // branch-gate: BG-1156
    window.gameplay.gameplayJump.status = window.gameplay.gameplayTraversal.accepted ? "traversal"
                                                                 : "traversal_rejected";
    window.gameplay.gameplayJump.reasonCode = window.gameplay.gameplayTraversal.reasonCode;
    return;
  }

  // branch-gate: BG-1157
  if (tryProductWallJump(session, window)) {
    return;
  }

  // branch-gate: BG-1153
  if (window.gameplay.gameplayJump.active) {
    // branch-gate: BG-1153
    if (tryProductCoyoteJump(session, window)) {
      return;
    }
    bufferProductJump(window);
    rejectProductJump(window, "already_airborne",
                      "gameplay_jump_already_airborne");
    return;
  }

  const EntityState* actor = productPlayerEntity(session);
  // branch-gate: BG-1153
  if (actor == nullptr) {
    rejectProductJump(window, "missing_player", "gameplay_jump_missing_player");
    return;
  }

  beginProductJumpArc(session,
                      window,
                      *actor,
                      actor->transform.position.y,
                      "gameplay_jump_accepted");
}

}  // namespace iggy3d
