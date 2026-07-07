#include "app/iggy3d/debug/MovementDebugHud.hpp"

#include <iostream>
#include <string>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/iggy3d/Options.hpp"
#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/gameplay/MovementProof.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"
#include "app/iggy3d/world/DefaultWorldTemplate.hpp"
#include "render/RenderDiagnostics.hpp"

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << "\n";
    return false;
  }
  return true;
}

bool hasLine(const iggy3d::MovementDebugHud& hud,
             const char* label,
             const char* value) {
  for (const iggy3d::MovementDebugHudLine& line : hud.lines) {
    if (line.visible && line.label == label && line.value == value) {
      return true;
    }
  }
  return false;
}

iggy3d::ProductAppWindowState movedWindow() {
  iggy3d::ProductAppWindowState window;
  window.gameplay.gameplayActive = true;
  window.gameplayMovement.attempted = true;
  window.gameplayMovement.status = "moved";
  window.gameplayMovement.debugAvailable = true;
  window.gameplayMovement.reasonCode = "movement_ok";
  window.gameplayMovement.blockedReason = "movement_ok";
  window.gameplayMovement.hitSurfaceId = "none";
  window.gameplayMovement.groundSnapApplied = true;
  window.gameplayMovement.policyBand = "flat";
  window.gameplayMovement.speedMultiplier = 1.0F;
  window.gameplayMovement.state =
      iggy3d::ProductGameplayMovementState::MovingGrounded;
  window.gameplayMovement.grounded = true;
  window.gameplayMovement.horizontalSpeedMetersPerSecond = 3.3F;
  window.gameplayJump.velocityMetersPerSecond = 0.0F;
  window.gameplayWallRun.candidateStatus = "wall_run_grounded";
  window.gameplayWallRun.candidateReasonCode = "wall_run_grounded";
  window.gameplayWallRun.approachSpeedMetersPerSecond = 3.3F;
  window.gameplayMovement.finalX = -1.0F;
  window.gameplayMovement.finalY = 0.0F;
  window.gameplayMovement.finalZ = -1.0F;
  return window;
}

std::size_t receiptFieldCount(const iggy3d::RenderReceipt& receipt,
                              const std::string& key) {
  std::size_t count = 0U;
  for (const iggy3d::RenderReceiptField& field : receipt.fields) {
    if (field.key == key) {
      ++count;
    }
  }
  return count;
}

bool receiptHasSingleField(const iggy3d::RenderReceipt& receipt,
                           const std::string& key,
                           const char* value) {
  return receiptFieldCount(receipt, key) == 1U &&
         iggy3d::hasReceiptField(receipt, key, value);
}

bool movementTuningReceiptRowsUseDescriptorTable() {
  iggy3d::ProductAppOptions options;
  iggy3d::ProductWorldTemplate world;
  iggy3d::FrontendState frontend;
  iggy3d::FrontendSettings settings;
  iggy3d::ProductAppWindowState window;
  iggy3d::ProductSaveBridgeResult saves;
  window.gameplayMovement.tuning.walkSpeedMetersPerSecond = 4.25F;
  window.gameplayMovement.tuning.invertLookEnabled = 1.0F;
  window.gameplayMovement.tuning.wallRunDurationSeconds = 1.25F;
  window.gameplayMovement.tuningSelectedField =
      iggy3d::ProductGameplayMovementTuningField::WallRunDuration;

  const iggy3d::RenderReceipt receipt =
      iggy3d::buildProductAppReceipt(options, world, frontend, settings, window, saves);
  bool ok =
      expect(receiptHasSingleField(receipt,
                                   "gameplay_movement_tuning_selected_field",
                                   "wall_run_duration_s"),
             "receipt selected tuning field") &&
      expect(receiptHasSingleField(receipt,
                                   "gameplay_movement_tuning_walk_speed_mps",
                                   "4.250"),
             "receipt walk speed tuning field") &&
      expect(receiptHasSingleField(receipt,
                                   "gameplay_movement_tuning_invert_look",
                                   "true"),
             "receipt invert tuning field") &&
      expect(receiptHasSingleField(receipt,
                                   "gameplay_movement_tuning_wall_run_duration_s",
                                   "1.250"),
             "receipt wall-run duration tuning field");

  for (const iggy3d::ProductGameplayMovementTuningFieldDescriptor& descriptor :
       iggy3d::kProductGameplayMovementTuningFields) {
    const std::string key =
        "gameplay_movement_tuning_" + std::string{descriptor.name};
    ok = expect(receiptFieldCount(receipt, key) == 1U,
                "receipt tuning descriptor key emitted once") && ok;
  }
  return ok;
}

bool movementProofPacketCopiesWindowProof() {
  iggy3d::ProductAppWindowState window = movedWindow();
  window.gameplayMovement.attempted = true;
  window.gameplayMovement.blocked = true;
  window.gameplayMovement.status = "blocked";
  window.gameplayMovement.reasonCode = "blocked_by_collision";
  window.gameplayMovement.blockedReason = "blocked_by_collision";
  window.gameplayMovement.hitSurfaceId = "wall_r0_c1_actor_blocker";
  window.gameplayMovement.clamped = true;
  window.gameplayMovement.slid = true;
  window.gameplayMovement.collisionSweepCount = 2U;
  window.gameplayMovement.slopeAngleDegrees = 12.5F;
  window.gameplayMovement.horizontalDistanceMeters = 0.75F;
  window.gameplayMovement.verticalDeltaMeters = -0.25F;
  window.gameplayMovement.groundVelocityX = 1.25F;
  window.gameplayMovement.groundVelocityZ = -0.5F;
  window.gameplayMovement.state =
      iggy3d::ProductGameplayMovementState::WallRunning;
  window.gameplayMovement.grounded = false;
  window.gameplayMovement.horizontalSpeedMetersPerSecond = 5.5F;
  window.gameplayJump.velocityMetersPerSecond = -1.75F;
  window.gameplayMovement.gradePercent = -3.0F;
  window.gameplayMovement.profile = "manual_first_person_sprint";
  window.gameplayMovement.maxSpeedMetersPerSecond = 6.2F;
  window.gameplayWallRun.candidateAvailable = true;
  window.gameplayWallRun.candidateStatus = "wall_run_candidate";
  window.gameplayWallRun.candidateReasonCode = "wall_run_candidate";
  window.gameplayWallRun.active = true;
  window.gameplayWallRun.status = "wall_run_active";
  window.gameplayWallRun.reasonCode = "wall_run_started";
  window.gameplayWallRun.side = "left";
  window.gameplayWallRun.surfaceId = "wall_r0_c1_actor_blocker";
  window.gameplayWallRun.normalX = 1.0F;
  window.gameplayWallRun.normalY = 0.0F;
  window.gameplayWallRun.normalZ = 0.0F;
  window.gameplayWallRun.remainingSeconds = 0.42F;
  window.gameplayWallRun.durationSeconds = 0.75F;
  window.gameplayWallRun.gravityMultiplier = 0.25F;
  window.gameplayWallRun.speedMultiplier = 1.1F;

  const iggy3d::ProductMovementProofPacket proof =
      iggy3d::buildProductMovementProofPacket(window);
  return expect(proof.attempted, "proof movement attempted") &&
         expect(proof.blocked, "proof movement blocked") &&
         expect(proof.status == "blocked", "proof movement status") &&
         expect(proof.reasonCode == "blocked_by_collision",
                "proof movement reason") &&
         expect(proof.hitSurfaceId == "wall_r0_c1_actor_blocker",
                "proof hit surface") &&
         expect(proof.movementClamped, "proof clamped") &&
         expect(proof.movementSlid, "proof slid") &&
         expect(proof.collisionSweepCount == 2U, "proof sweep count") &&
         expect(proof.state == iggy3d::ProductGameplayMovementState::WallRunning,
                "proof state enum") &&
         expect(proof.stateName == "wall_running", "proof state name") &&
         expect(proof.stateHudLabel == "wall running", "proof state label") &&
         expect(!proof.grounded, "proof grounded") &&
         expect(proof.horizontalSpeedMetersPerSecond == 5.5F,
                "proof horizontal speed") &&
         expect(proof.verticalVelocityMetersPerSecond == -1.75F,
                "proof vertical velocity") &&
         expect(proof.wallRunCandidateAvailable,
                "proof wall-run candidate") &&
         expect(proof.wallRunActive, "proof wall-run active") &&
         expect(proof.wallRunReasonCode == "wall_run_started",
                "proof wall-run reason") &&
         expect(proof.wallRunReasonHudLabel == "wall run started",
                "proof wall-run label") &&
         expect(proof.wallRunSurfaceId == "wall_r0_c1_actor_blocker",
                "proof wall-run surface") &&
         expect(proof.wallRunRemainingSeconds == 0.42F,
                "proof wall-run remaining") &&
         expect(proof.wallRunGravityMultiplier == 0.25F,
                "proof wall-run gravity") &&
         expect(proof.profile == "manual_first_person_sprint",
                "proof movement profile");
}

bool movementProofFeedsReceiptAndHudConsistently() {
  iggy3d::ProductAppOptions options;
  iggy3d::ProductWorldTemplate world;
  iggy3d::FrontendState frontend;
  iggy3d::FrontendSettings settings;
  settings.devToolsEnabled = true;
  settings.debugOverlayEnabled = true;
  iggy3d::ProductAppWindowState window = movedWindow();
  iggy3d::ProductSaveBridgeResult saves;
  window.gameplayMovement.state =
      iggy3d::ProductGameplayMovementState::WallRunning;
  window.gameplayWallRun.active = true;
  window.gameplayWallRun.status = "wall_run_active";
  window.gameplayWallRun.reasonCode = "wall_run_started";
  window.gameplayWallRun.side = "left";
  window.gameplayWallRun.surfaceId = "wall_r0_c1_actor_blocker";
  window.gameplayWallRun.remainingSeconds = 0.500F;

  const iggy3d::ProductMovementProofPacket proof =
      iggy3d::buildProductMovementProofPacket(window);
  const iggy3d::MovementDebugHud hud =
      iggy3d::buildMovementDebugHud(proof, true, true, true);
  const iggy3d::RenderReceipt receipt =
      iggy3d::buildProductAppReceipt(options, world, frontend, settings, window, saves);

  return expect(hud.movementState == proof.stateName,
                "hud state from proof") &&
         expect(hud.wallRunStatus == proof.wallRunStatus,
                "hud wall-run status from proof") &&
         expect(hasLine(hud, "STATE", "wall running h3.300 v0.000"),
                "hud state line from proof label") &&
         expect(hasLine(hud,
                        "WALLRUN",
                        "active left wall r0 c1 actor blocker wall run started t0.500 h3.300"),
                "hud wall-run line from proof label") &&
         expect(iggy3d::hasReceiptField(receipt,
                                        "movement_state",
                                        proof.stateName),
                "receipt movement state from proof") &&
         expect(iggy3d::hasReceiptField(receipt,
                                        "wall_run_reason_code",
                                        proof.wallRunReasonCode),
                "receipt wall-run reason from proof");
}

}  // namespace

int main() {
  bool ok = true;

  ok &= movementTuningReceiptRowsUseDescriptorTable();
  ok &= movementProofPacketCopiesWindowProof();
  ok &= movementProofFeedsReceiptAndHudConsistently();

  iggy3d::ProductAppWindowState inactive = movedWindow();
  inactive.gameplay.gameplayActive = false;
  const iggy3d::MovementDebugHud inactiveHud =
      iggy3d::buildMovementDebugHud(inactive, true, true);
  ok &= expect(!inactiveHud.visible, "inactive gameplay hides movement hud");
  ok &= expect(inactiveHud.status == "not_requested", "inactive hud status");
  ok &= expect(inactiveHud.reasonCode == "not_requested", "inactive hud reason");
  ok &= expect(inactiveHud.lines.empty(), "inactive hud has no visible lines");

  const iggy3d::MovementDebugHud devDisabled =
      iggy3d::buildMovementDebugHud(movedWindow(), false, true);
  ok &= expect(!devDisabled.visible, "developer tools disabled hides hud");
  ok &= expect(devDisabled.debugAvailable, "disabled hud still reports debug availability");
  ok &= expect(devDisabled.status == "not_requested", "developer tools disabled status");
  ok &= expect(devDisabled.reasonCode == "not_requested", "developer tools disabled reason");

  const iggy3d::MovementDebugHud overlayDisabled =
      iggy3d::buildMovementDebugHud(movedWindow(), true, false);
  ok &= expect(!overlayDisabled.visible, "debug overlay disabled hides hud");
  ok &= expect(overlayDisabled.debugAvailable, "overlay disabled still reports debug availability");
  ok &= expect(overlayDisabled.status == "not_requested", "overlay disabled status");
  ok &= expect(overlayDisabled.reasonCode == "not_requested", "overlay disabled reason");

  const iggy3d::MovementDebugHud moved =
      iggy3d::buildMovementDebugHud(movedWindow(), true, true);
  ok &= expect(moved.visible, "moved hud visible");
  ok &= expect(moved.status == "moved", "moved status");
  ok &= expect(moved.reasonCode == "movement_ok", "moved reason");
  ok &= expect(!moved.blocked, "moved not blocked");
  ok &= expect(moved.movementState == "moving_grounded",
               "moved movement state");
  ok &= expect(!moved.wallRunCandidateAvailable,
               "moved wall-run candidate unavailable");
  ok &= expect(moved.lines.size() == 9U, "moved line count");
  ok &= expect(hasLine(moved, "STATUS", "moved"), "moved status line");
  ok &= expect(hasLine(moved, "STATE", "moving grounded h3.300 v0.000"),
               "moved state display");
  ok &= expect(hasLine(moved,
                       "WALLRUN",
                       "cand none none wall run inactive t0.000 h3.300"),
               "moved wall-run display");
  ok &= expect(hasLine(moved, "REASON", "movement ok"), "moved reason display");
  ok &= expect(hasLine(moved, "HIT", "none"), "moved hit display");
  ok &= expect(hasLine(moved, "SPEED", "1.000"), "moved speed display");
  ok &= expect(hasLine(moved, "SNAP", "true"), "moved snap display");

  iggy3d::ProductAppWindowState blocked = movedWindow();
  blocked.gameplayMovement.blocked = true;
  blocked.gameplayMovement.state =
      iggy3d::ProductGameplayMovementState::BlockedOrSliding;
  blocked.gameplayMovement.status = "blocked";
  blocked.gameplayMovement.reasonCode = "blocked_by_collision";
  blocked.gameplayMovement.blockedReason = "blocked_by_collision";
  blocked.gameplayMovement.hitSurfaceId = "wall_r0_c1_actor_blocker";
  blocked.gameplayMovement.groundSnapApplied = false;
  blocked.gameplayMovement.clamped = true;
  blocked.gameplayMovement.policyBand = "none";
  blocked.gameplayWallRun.candidateAvailable = true;
  blocked.gameplayWallRun.candidateStatus = "wall_run_candidate";
  blocked.gameplayWallRun.candidateReasonCode = "wall_run_candidate";
  blocked.gameplayWallRun.active = true;
  blocked.gameplayWallRun.status = "wall_run_active";
  blocked.gameplayWallRun.reasonCode = "wall_run_active";
  blocked.gameplayWallRun.side = "left";
  blocked.gameplayWallRun.surfaceId = "wall_r0_c1_actor_blocker";
  blocked.gameplayWallRun.remainingSeconds = 0.500F;
  const iggy3d::MovementDebugHud blockedHud =
      iggy3d::buildMovementDebugHud(blocked, true, true);
  ok &= expect(blockedHud.visible, "blocked hud visible");
  ok &= expect(blockedHud.blocked, "blocked hud blocked");
  ok &= expect(blockedHud.movementState == "blocked_or_sliding",
               "blocked hud state");
  ok &= expect(blockedHud.wallRunCandidateAvailable,
               "blocked hud wall-run candidate");
  ok &= expect(blockedHud.wallRunActive, "blocked hud wall-run active");
  ok &= expect(blockedHud.hitSurfaceId == "wall_r0_c1_actor_blocker", "blocked hit id");
  ok &= expect(hasLine(blockedHud, "STATE", "blocked or sliding h3.300 v0.000"),
               "blocked state display");
  ok &= expect(hasLine(blockedHud,
                       "WALLRUN",
                       "active left wall r0 c1 actor blocker wall run active t0.500 h3.300"),
               "blocked wall-run display");
  ok &= expect(iggy3d::findProductWallRunStatusDescriptor(
                   blocked.gameplayWallRun.reasonCode) != nullptr,
               "blocked wall-run status descriptor");
  ok &= expect(hasLine(blockedHud, "STATUS", "blocked"), "blocked status line");
  ok &= expect(hasLine(blockedHud, "REASON", "blocked by collision"),
               "blocked reason display");
  ok &= expect(hasLine(blockedHud, "HIT", "wall r0 c1 actor blocker"),
               "blocked hit display");

  if (!ok) {
    return 1;
  }
  std::cout << "product_movement_debug_hud_tests=pass\n";
  return 0;
}
