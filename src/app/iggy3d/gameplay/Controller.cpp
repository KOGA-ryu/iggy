#include "app/iggy3d/gameplay/Controller.hpp"

#include <optional>
#include <string>
#include <string_view>

#include "app/input/ActionState.hpp"
#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/gameplay/ControllerDashActions.hpp"
#include "app/iggy3d/gameplay/ControllerJumpActions.hpp"
#include "app/iggy3d/gameplay/ControllerJumpDashState.hpp"
#include "app/iggy3d/gameplay/ControllerMovementProof.hpp"
#include "app/iggy3d/gameplay/ControllerMoveActions.hpp"
#include "app/iggy3d/gameplay/ControllerPlayerAccess.hpp"
#include "app/iggy3d/gameplay/ControllerResetActions.hpp"
#include "app/iggy3d/gameplay/ControllerTargetActions.hpp"
#include "app/iggy3d/gameplay/ControllerWallRunEvaluation.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/session/Session.hpp"

namespace iggy3d {
namespace {

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
         productHorizontalVelocityActive(window);
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
  submitProductReset(session, window, source);
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
