#include "app/iggy3d/gameplay/ControllerMoveActions.hpp"

#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/gameplay/ControllerCommandExecution.hpp"
#include "app/iggy3d/gameplay/ControllerKinematics.hpp"
#include "app/iggy3d/gameplay/ControllerMovementProof.hpp"
#include "app/iggy3d/gameplay/ControllerPlayerAccess.hpp"
#include "app/iggy3d/gameplay/ControllerTargetOutcomeProof.hpp"
#include "app/iggy3d/gameplay/ControllerWallQueries.hpp"
#include "app/iggy3d/gameplay/ControllerWallRunEvaluation.hpp"
#include "app/iggy3d/gameplay/MovementTuning.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/world/EntityState.hpp"

#include <algorithm>
#include <string>

namespace iggy3d {
namespace {

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

}  // namespace

bool productHorizontalVelocityActive(const ProductAppWindowState& window) {
  const float speedSquared =
      window.gameplay.gameplayMovement.groundVelocityX *
          window.gameplay.gameplayMovement.groundVelocityX +
      window.gameplay.gameplayMovement.groundVelocityZ *
          window.gameplay.gameplayMovement.groundVelocityZ;
  return speedSquared > 0.000001F;
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

}  // namespace iggy3d
