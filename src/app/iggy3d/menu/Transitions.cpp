#include "app/iggy3d/menu/Transitions.hpp"

#include <string>
#include <utility>

#include "app/iggy3d/menu/FrontendRouter.hpp"

namespace iggy3d {
namespace {

void setTransition(ProductAppWindowState& window,
                   std::string action,
                   std::string status,
                   bool returnedToGameplay,
                   bool returnedToTitle,
                   bool sessionPreserved) {
  window.gameplay.productTransition.lastAction = std::move(action);
  window.gameplay.productTransition.status = std::move(status);
  window.gameplay.productTransition.returnedToGameplay = returnedToGameplay;
  window.gameplay.productTransition.returnedToTitle = returnedToTitle;
  window.gameplay.productTransition.sessionPreserved = sessionPreserved;
}

void clearProductPauseOwnedTransientModes(ProductAppWindowState& window) {
  const bool preserveCreativeWorldMode =
      window.inputDevice.interactionMode == ProductInteractionMode::Creative &&
      productCreativeWorldActiveForWindow(window);
  clearProductMenuOwnedTransientModes(window);
  if (preserveCreativeWorldMode) {
    window.inputDevice.interactionMode = ProductInteractionMode::Creative;
  }
}

}  // namespace

void clearProductMapMakerMode(ProductAppWindowState& window) {
  window.inputDevice.interactionMode = ProductInteractionMode::Player;
  window.viewport.creativeFlyActive = false;
  window.viewport.creativeFlyStatus = "creative_fly_not_requested";
  window.viewport.creativeFlyReasonCode = window.viewport.creativeFlyStatus;
  window.viewport.creativeFlySpeedMetersPerSecond = 0.0F;
  window.viewport.mapMakerStatus = "map_maker_inactive";
  window.viewport.mapMakerReasonCode = window.viewport.mapMakerStatus;
}

void clearProductGameplayMovementTuning(ProductAppWindowState& window) {
  window.gameplay.gameplayMovement.tuningVisible = false;
  window.gameplay.gameplayMovement.tuningStatus = "movement_tuning_hidden";
  window.gameplay.gameplayMovement.tuningReasonCode = window.gameplay.gameplayMovement.tuningStatus;
}

void clearProductGameplayGroundVelocity(ProductAppWindowState& window) {
  window.gameplay.gameplayMovement.groundVelocityX = 0.0F;
  window.gameplay.gameplayMovement.groundVelocityZ = 0.0F;
  window.gameplay.gameplayMovement.horizontalSpeedMetersPerSecond = 0.0F;
  window.gameplay.gameplayWallRun.candidateAvailable = false;
  window.gameplay.gameplayWallRun.candidateStatus = "wall_run_grounded";
  window.gameplay.gameplayWallRun.candidateReasonCode = window.gameplay.gameplayWallRun.candidateStatus;
  window.gameplay.gameplayWallRun.side = "none";
  window.gameplay.gameplayWallRun.surfaceId = "none";
  window.gameplay.gameplayWallRun.normalX = 0.0F;
  window.gameplay.gameplayWallRun.normalY = 0.0F;
  window.gameplay.gameplayWallRun.normalZ = 0.0F;
  window.gameplay.gameplayWallRun.approachSpeedMetersPerSecond = 0.0F;
  window.gameplay.gameplayWallRun.active = false;
  window.gameplay.gameplayWallRun.status = "wall_run_inactive";
  window.gameplay.gameplayWallRun.reasonCode = window.gameplay.gameplayWallRun.status;
  window.gameplay.gameplayWallRun.remainingSeconds = 0.0F;
  window.gameplay.gameplayWallRun.durationSeconds = 0.0F;
  window.gameplay.gameplayWallRun.gravityMultiplier = 1.0F;
  window.gameplay.gameplayWallRun.speedMultiplier = 1.0F;
  // branch-gate: BG-1161
  if (!window.gameplay.gameplayJump.active) {
    window.gameplay.gameplayMovement.grounded = true;
    window.gameplay.gameplayMovement.state = ProductGameplayMovementState::IdleGrounded;
  }
}

void clearProductGameplayJumpTiming(ProductAppWindowState& window) {
  window.gameplay.gameplayJump.coyoteSecondsRemaining = 0.0F;
  window.gameplay.gameplayJump.bufferSecondsRemaining = 0.0F;
  window.gameplay.gameplayJump.held = false;
  window.gameplay.gameplayJump.cutApplied = false;
}

void clearProductMenuOwnedTransientModes(ProductAppWindowState& window) {
  clearProductMapMakerMode(window);
  clearProductGameplayMovementTuning(window);
  clearProductGameplayGroundVelocity(window);
  clearProductGameplayJumpTiming(window);
}

void clearProductRoomEditorTransientModes(ProductAppWindowState& window) {
  window.creativeAuthoring.roomEditorCursorReady = false;
  window.creativeAuthoring.roomEditorOverlay.visible = false;
  window.creativeAuthoring.roomEditorOverlay.status = "room_editor_overlay_not_ready";
  window.creativeAuthoring.roomEditorOverlay.reasonCode = window.creativeAuthoring.roomEditorOverlay.status;
  window.creativeAuthoring.roomEditorOverlay.itemCount = 0;
  window.creativeAuthoring.roomEditorPreview.active = false;
  window.creativeAuthoring.roomEditorPlacementPreview = {};
  window.creativeAuthoring.roomEditorPreview.visible = false;
  window.creativeAuthoring.roomEditorPreview.status = "room_editor_preview_not_requested";
  window.creativeAuthoring.roomEditorPreview.reasonCode = window.creativeAuthoring.roomEditorPreview.status;
  window.creativeAuthoring.roomEditorPreview.candidateId = "none";
  window.creativeAuthoring.roomEditorPreview.tool = "floor";
  window.creativeAuthoring.roomEditorPreview.gridX = 0;
  window.creativeAuthoring.roomEditorPreview.gridZ = 0;
  window.creativeAuthoring.roomEditorPreview.beforeDrawCount = 0;
  window.creativeAuthoring.roomEditorPreview.afterDrawCount = 0;
  window.creativeAuthoring.roomEditorPreview.avoidedDrawCountDelta = 0;
  window.creativeAuthoring.roomEditorPreview.beforeTriangleCount = 0;
  window.creativeAuthoring.roomEditorPreview.afterTriangleCount = 0;
  window.creativeAuthoring.roomEditorPreview.avoidedTriangleCountDelta = 0;
  window.creativeAuthoring.roomEditorPreview.optimizedDrawDelta = 0;
  window.creativeAuthoring.roomEditorPreview.optimizedTriangleDelta = 0;
  window.creativeAuthoring.roomEditorHud.visible = false;
  window.creativeAuthoring.roomEditorHud.status = "room_editor_hud_not_ready";
  window.creativeAuthoring.roomEditorHud.reasonCode = window.creativeAuthoring.roomEditorHud.status;
  window.creativeAuthoring.roomEditorHud.previewActive = false;
  window.creativeAuthoring.roomEditorHud.previewStatus = "room_editor_preview_not_requested";
  window.creativeAuthoring.roomEditorHud.previewCandidateId = "none";
  window.creativeAuthoring.roomEditorHud.previewOptimizedDrawDelta = 0;
  window.creativeAuthoring.roomEditorHud.previewOptimizedTriangleDelta = 0;
  window.creativeAuthoring.roomEditorHud.lineCount = 0;
  window.viewport.productDrawRoomEditorCursorVisible = false;
  window.viewport.productDrawRoomEditorCursorCount = 0;
  window.viewport.productDrawRoomEditorPreviewVisible = false;
  window.viewport.productDrawRoomEditorPreviewCount = 0;
}

void clearProductGameplayOnlyModes(ProductAppWindowState& window) {
  clearProductMenuOwnedTransientModes(window);
  window.debugHud.devCollisionOverlay.visible = false;
  window.debugHud.devCollisionOverlay.status = "dev_collision_overlay_hidden";
  window.debugHud.devCollisionOverlay.reasonCode =
      window.debugHud.devCollisionOverlay.status;
  clearProductRoomEditorTransientModes(window);
}

void clearProductGameplayOnlyModes(ProductAppWindowState& window,
                                   FrontendSettings& settings) {
  clearProductGameplayOnlyModes(window);
  settings.debugOverlayEnabled = false;
}

void applyReturnProductToTitleTransition(FrontendState& frontend,
                                         ProductAppWindowState& window) {
  frontend.screen = FrontendScreen::Starter;
  frontend.childScreen = FrontendScreen::Gameplay;
  frontend.selectedAction = FrontendAction::NewWorld;
  frontend.returnToTitleRequested = true;
  frontend.inputOwned = true;
  frontend.status = "returned_to_title";
  setTransition(window, "return_to_title", "returned_to_title", false, true, false);
}

void initializeProductStarterTransition(FrontendState& frontend,
                                        ProductAppWindowState& window,
                                        bool hasCompatibleSave) {
  clearProductGameplayOnlyModes(window);
  completeFrontendBoot(frontend, true, true);
  frontend.screen = FrontendScreen::Starter;
  frontend.childScreen = FrontendScreen::Gameplay;
  frontend.selectedAction =
      hasCompatibleSave ? FrontendAction::Continue : FrontendAction::NewWorld;
  frontend.status = "opening_menu_ready";
  frontend.inputOwned = true;
  setTransition(window, "startup", "starter_ready", false, false, false);
}

void enterProductGameplayTransition(FrontendState& frontend,
                                    ProductAppWindowState& window,
                                    FrontendAction launchAction) {
  enterFrontendGameplay(frontend, launchAction);
  frontend.status = "gameplay_active";
  setTransition(window, "launch_gameplay", "gameplay_active", true, false, true);
}

void openProductPauseTransition(FrontendState& frontend,
                                ProductAppWindowState& window,
                                FrontendAction selectedAction) {
  clearProductPauseOwnedTransientModes(window);
  openFrontendPause(frontend, selectedAction);
  setTransition(window, "open_pause", "pause_ready", false, false,
                window.gameplay.gameplayActive);
}

void openProductPauseSettingsTransition(FrontendState& frontend,
                                        ProductAppWindowState& window,
                                        FrontendSettingsTab& settingsTab) {
  clearProductMenuOwnedTransientModes(window);
  frontend.screen = FrontendScreen::Settings;
  frontend.childScreen = FrontendScreen::Pause;
  settingsTab = FrontendSettingsTab::Input;
  frontend.inputOwned = true;
  frontend.status = "pause_settings_selected";
  setTransition(window, "open_settings", "settings_from_pause", false, false,
                window.gameplay.gameplayActive);
}

void openProductPauseDevToolsTransition(FrontendState& frontend,
                                        ProductAppWindowState& window,
                                        FrontendDevToolsCategory category) {
  clearProductMenuOwnedTransientModes(window);
  openFrontendDevOverlay(frontend, category);
  frontend.status = "pause_dev_tools_selected";
  setTransition(window, "open_dev_tools", "dev_overlay_from_pause", false, false,
                window.gameplay.gameplayActive);
}

void closeProductOverlayToGameplayTransition(FrontendState& frontend,
                                             ProductAppWindowState& window) {
  closeFrontendOverlayToGameplay(frontend);
  setTransition(window, "resume_gameplay", "gameplay_resumed", true, false,
                window.gameplay.gameplayActive);
}

void returnProductToTitleTransition(FrontendState& frontend,
                                    ProductAppWindowState& window) {
  window.gameplay.gameplayActive = false;
  window.gameplay.runtimeSessionCreated = false;
  clearProductGameplayOnlyModes(window);
  applyReturnProductToTitleTransition(frontend, window);
}

void returnProductToTitleTransition(FrontendState& frontend,
                                    ProductAppWindowState& window,
                                    FrontendSettings& settings) {
  window.gameplay.gameplayActive = false;
  window.gameplay.runtimeSessionCreated = false;
  clearProductGameplayOnlyModes(window, settings);
  applyReturnProductToTitleTransition(frontend, window);
}

}  // namespace iggy3d
