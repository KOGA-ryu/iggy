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
  window.productTransitionLastAction = std::move(action);
  window.productTransitionStatus = std::move(status);
  window.productTransitionReturnedToGameplay = returnedToGameplay;
  window.productTransitionReturnedToTitle = returnedToTitle;
  window.productTransitionSessionPreserved = sessionPreserved;
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
  window.gameplayMovementTuningVisible = false;
  window.gameplayMovementTuningStatus = "movement_tuning_hidden";
  window.gameplayMovementTuningReasonCode = window.gameplayMovementTuningStatus;
}

void clearProductGameplayGroundVelocity(ProductAppWindowState& window) {
  window.gameplayMovementGroundVelocityX = 0.0F;
  window.gameplayMovementGroundVelocityZ = 0.0F;
}

void clearProductMenuOwnedTransientModes(ProductAppWindowState& window) {
  clearProductMapMakerMode(window);
  clearProductGameplayMovementTuning(window);
  clearProductGameplayGroundVelocity(window);
}

void clearProductRoomEditorTransientModes(ProductAppWindowState& window) {
  window.roomEditorCursorReady = false;
  window.roomEditorOverlayVisible = false;
  window.roomEditorOverlayStatus = "room_editor_overlay_not_ready";
  window.roomEditorOverlayReasonCode = window.roomEditorOverlayStatus;
  window.roomEditorOverlayItemCount = 0;
  window.roomEditorPreviewActive = false;
  window.roomEditorPlacementPreview = {};
  window.roomEditorPreviewVisible = false;
  window.roomEditorPreviewStatus = "room_editor_preview_not_requested";
  window.roomEditorPreviewReasonCode = window.roomEditorPreviewStatus;
  window.roomEditorPreviewCandidateId = "none";
  window.roomEditorPreviewTool = "floor";
  window.roomEditorPreviewGridX = 0;
  window.roomEditorPreviewGridZ = 0;
  window.roomEditorPreviewBeforeDrawCount = 0;
  window.roomEditorPreviewAfterDrawCount = 0;
  window.roomEditorPreviewAvoidedDrawCountDelta = 0;
  window.roomEditorPreviewBeforeTriangleCount = 0;
  window.roomEditorPreviewAfterTriangleCount = 0;
  window.roomEditorPreviewAvoidedTriangleCountDelta = 0;
  window.roomEditorPreviewOptimizedDrawDelta = 0;
  window.roomEditorPreviewOptimizedTriangleDelta = 0;
  window.roomEditorHudVisible = false;
  window.roomEditorHudStatus = "room_editor_hud_not_ready";
  window.roomEditorHudReasonCode = window.roomEditorHudStatus;
  window.roomEditorHudPreviewActive = false;
  window.roomEditorHudPreviewStatus = "room_editor_preview_not_requested";
  window.roomEditorHudPreviewCandidateId = "none";
  window.roomEditorHudPreviewOptimizedDrawDelta = 0;
  window.roomEditorHudPreviewOptimizedTriangleDelta = 0;
  window.roomEditorHudLineCount = 0;
  window.viewport.productDrawRoomEditorCursorVisible = false;
  window.viewport.productDrawRoomEditorCursorCount = 0;
  window.viewport.productDrawRoomEditorPreviewVisible = false;
  window.viewport.productDrawRoomEditorPreviewCount = 0;
}

void clearProductGameplayOnlyModes(ProductAppWindowState& window) {
  clearProductMenuOwnedTransientModes(window);
  window.devCollisionOverlayVisible = false;
  window.devCollisionOverlayStatus = "dev_collision_overlay_hidden";
  window.devCollisionOverlayReasonCode = window.devCollisionOverlayStatus;
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
  clearProductMenuOwnedTransientModes(window);
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
