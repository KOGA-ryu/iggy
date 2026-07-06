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
  window.productTransition.lastAction = std::move(action);
  window.productTransition.status = std::move(status);
  window.productTransition.returnedToGameplay = returnedToGameplay;
  window.productTransition.returnedToTitle = returnedToTitle;
  window.productTransition.sessionPreserved = sessionPreserved;
}

void clearProductPauseOwnedTransientModes(ProductAppWindowState& window) {
  const bool preserveCreativeWorldMode =
      window.interactionMode == ProductInteractionMode::Creative &&
      productCreativeWorldActiveForWindow(window);
  clearProductMenuOwnedTransientModes(window);
  if (preserveCreativeWorldMode) {
    window.interactionMode = ProductInteractionMode::Creative;
  }
}

}  // namespace

void clearProductMapMakerMode(ProductAppWindowState& window) {
  window.interactionMode = ProductInteractionMode::Player;
  window.viewport.creativeFlyActive = false;
  window.viewport.creativeFlyStatus = "creative_fly_not_requested";
  window.viewport.creativeFlyReasonCode = window.viewport.creativeFlyStatus;
  window.viewport.creativeFlySpeedMetersPerSecond = 0.0F;
  window.mapMakerStatus = "map_maker_inactive";
  window.mapMakerReasonCode = window.mapMakerStatus;
}

void clearProductGameplayMovementTuning(ProductAppWindowState& window) {
  window.gameplayMovement.tuningVisible = false;
  window.gameplayMovement.tuningStatus = "movement_tuning_hidden";
  window.gameplayMovement.tuningReasonCode = window.gameplayMovement.tuningStatus;
}

void clearProductGameplayGroundVelocity(ProductAppWindowState& window) {
  window.gameplayMovement.groundVelocityX = 0.0F;
  window.gameplayMovement.groundVelocityZ = 0.0F;
  window.gameplayMovement.horizontalSpeedMetersPerSecond = 0.0F;
  window.gameplayWallRun.candidateAvailable = false;
  window.gameplayWallRun.candidateStatus = "wall_run_grounded";
  window.gameplayWallRun.candidateReasonCode = window.gameplayWallRun.candidateStatus;
  window.gameplayWallRun.side = "none";
  window.gameplayWallRun.surfaceId = "none";
  window.gameplayWallRun.normalX = 0.0F;
  window.gameplayWallRun.normalY = 0.0F;
  window.gameplayWallRun.normalZ = 0.0F;
  window.gameplayWallRun.approachSpeedMetersPerSecond = 0.0F;
  window.gameplayWallRun.active = false;
  window.gameplayWallRun.status = "wall_run_inactive";
  window.gameplayWallRun.reasonCode = window.gameplayWallRun.status;
  window.gameplayWallRun.remainingSeconds = 0.0F;
  window.gameplayWallRun.durationSeconds = 0.0F;
  window.gameplayWallRun.gravityMultiplier = 1.0F;
  window.gameplayWallRun.speedMultiplier = 1.0F;
  // branch-gate: BG-1161
  if (!window.gameplayJump.active) {
    window.gameplayMovement.grounded = true;
    window.gameplayMovement.state = ProductGameplayMovementState::IdleGrounded;
  }
}

void clearProductGameplayJumpTiming(ProductAppWindowState& window) {
  window.gameplayJump.coyoteSecondsRemaining = 0.0F;
  window.gameplayJump.bufferSecondsRemaining = 0.0F;
  window.gameplayJump.held = false;
  window.gameplayJump.cutApplied = false;
}

void clearProductMenuOwnedTransientModes(ProductAppWindowState& window) {
  clearProductMapMakerMode(window);
  clearProductGameplayMovementTuning(window);
  clearProductGameplayGroundVelocity(window);
  clearProductGameplayJumpTiming(window);
}

void clearProductRoomEditorTransientModes(ProductAppWindowState& window) {
  window.roomEditorCursorReady = false;
  window.roomEditorOverlay.visible = false;
  window.roomEditorOverlay.status = "room_editor_overlay_not_ready";
  window.roomEditorOverlay.reasonCode = window.roomEditorOverlay.status;
  window.roomEditorOverlay.itemCount = 0;
  window.roomEditorPreview.active = false;
  window.roomEditorPlacementPreview = {};
  window.roomEditorPreview.visible = false;
  window.roomEditorPreview.status = "room_editor_preview_not_requested";
  window.roomEditorPreview.reasonCode = window.roomEditorPreview.status;
  window.roomEditorPreview.candidateId = "none";
  window.roomEditorPreview.tool = "floor";
  window.roomEditorPreview.gridX = 0;
  window.roomEditorPreview.gridZ = 0;
  window.roomEditorPreview.beforeDrawCount = 0;
  window.roomEditorPreview.afterDrawCount = 0;
  window.roomEditorPreview.avoidedDrawCountDelta = 0;
  window.roomEditorPreview.beforeTriangleCount = 0;
  window.roomEditorPreview.afterTriangleCount = 0;
  window.roomEditorPreview.avoidedTriangleCountDelta = 0;
  window.roomEditorPreview.optimizedDrawDelta = 0;
  window.roomEditorPreview.optimizedTriangleDelta = 0;
  window.roomEditorHud.visible = false;
  window.roomEditorHud.status = "room_editor_hud_not_ready";
  window.roomEditorHud.reasonCode = window.roomEditorHud.status;
  window.roomEditorHud.previewActive = false;
  window.roomEditorHud.previewStatus = "room_editor_preview_not_requested";
  window.roomEditorHud.previewCandidateId = "none";
  window.roomEditorHud.previewOptimizedDrawDelta = 0;
  window.roomEditorHud.previewOptimizedTriangleDelta = 0;
  window.roomEditorHud.lineCount = 0;
  window.viewport.productDrawRoomEditorCursorVisible = false;
  window.viewport.productDrawRoomEditorCursorCount = 0;
  window.viewport.productDrawRoomEditorPreviewVisible = false;
  window.viewport.productDrawRoomEditorPreviewCount = 0;
}

void clearProductGameplayOnlyModes(ProductAppWindowState& window) {
  clearProductMenuOwnedTransientModes(window);
  window.devCollisionOverlay.visible = false;
  window.devCollisionOverlay.status = "dev_collision_overlay_hidden";
  window.devCollisionOverlay.reasonCode = window.devCollisionOverlay.status;
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
  syncProductWindowInputOwnerFromActiveSurface(frontend, window);
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
  syncProductWindowInputOwnerFromActiveSurface(frontend, window);
  setTransition(window, "startup", "starter_ready", false, false, false);
}

void enterProductGameplayTransition(FrontendState& frontend,
                                    ProductAppWindowState& window,
                                    FrontendAction launchAction) {
  enterFrontendGameplay(frontend, launchAction);
  frontend.status = "gameplay_active";
  syncProductWindowInputOwnerFromActiveSurface(frontend, window);
  setTransition(window, "launch_gameplay", "gameplay_active", true, false, true);
}

void openProductPauseTransition(FrontendState& frontend,
                                ProductAppWindowState& window,
                                FrontendAction selectedAction) {
  clearProductPauseOwnedTransientModes(window);
  openFrontendPause(frontend, selectedAction);
  syncProductWindowInputOwnerFromActiveSurface(frontend, window);
  setTransition(window, "open_pause", "pause_ready", false, false,
                window.gameplayActive);
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
  syncProductWindowInputOwnerFromActiveSurface(frontend, window);
  setTransition(window, "open_settings", "settings_from_pause", false, false,
                window.gameplayActive);
}

void openProductPauseDevToolsTransition(FrontendState& frontend,
                                        ProductAppWindowState& window,
                                        FrontendDevToolsCategory category) {
  clearProductMenuOwnedTransientModes(window);
  openFrontendDevOverlay(frontend, category);
  frontend.status = "pause_dev_tools_selected";
  syncProductWindowInputOwnerFromActiveSurface(frontend, window);
  setTransition(window, "open_dev_tools", "dev_overlay_from_pause", false, false,
                window.gameplayActive);
}

void closeProductOverlayToGameplayTransition(FrontendState& frontend,
                                             ProductAppWindowState& window) {
  closeFrontendOverlayToGameplay(frontend);
  syncProductWindowInputOwnerFromActiveSurface(frontend, window);
  setTransition(window, "resume_gameplay", "gameplay_resumed", true, false,
                window.gameplayActive);
}

void returnProductToTitleTransition(FrontendState& frontend,
                                    ProductAppWindowState& window) {
  window.gameplayActive = false;
  window.runtimeSessionCreated = false;
  clearProductGameplayOnlyModes(window);
  applyReturnProductToTitleTransition(frontend, window);
}

void returnProductToTitleTransition(FrontendState& frontend,
                                    ProductAppWindowState& window,
                                    FrontendSettings& settings) {
  window.gameplayActive = false;
  window.runtimeSessionCreated = false;
  clearProductGameplayOnlyModes(window, settings);
  applyReturnProductToTitleTransition(frontend, window);
}

}  // namespace iggy3d
