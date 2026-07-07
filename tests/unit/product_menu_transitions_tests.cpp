#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/menu/Transitions.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"

#include <iostream>

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << "\n";
    return false;
  }
  return true;
}

// window no longer mirrors input-owner/gameplay-suppression; live source is the
// resolved active surface (pure function of frontend+window).
iggy3d::ProductActiveSurfaceFrame liveSurface(
    const iggy3d::FrontendState& frontend,
    iggy3d::ProductAppWindowState& window) {
  return iggy3d::syncProductWindowInputOwnerFromActiveSurface(frontend, window);
}

void activateMapMaker(iggy3d::ProductAppWindowState& window) {
  window.inputDevice.interactionMode = iggy3d::ProductInteractionMode::Creative;
  window.viewport.creativeFlyActive = true;
  window.viewport.creativeFlyStatus = "creative_fly_applied";
  window.viewport.creativeFlyReasonCode = window.viewport.creativeFlyStatus;
  window.viewport.creativeFlySpeedMetersPerSecond = 8.0F;
  window.viewport.mapMakerStatus = "map_maker_active";
  window.viewport.mapMakerReasonCode = window.viewport.mapMakerStatus;
}

void showMovementTuning(iggy3d::ProductAppWindowState& window) {
  window.gameplay.gameplayMovement.tuningVisible = true;
  window.gameplay.gameplayMovement.tuningStatus = "movement_tuning_visible";
  window.gameplay.gameplayMovement.tuningReasonCode = window.gameplay.gameplayMovement.tuningStatus;
}

void retainGroundVelocity(iggy3d::ProductAppWindowState& window) {
  window.gameplay.gameplayMovement.groundVelocityX = 1.25F;
  window.gameplay.gameplayMovement.groundVelocityZ = -2.5F;
}

void retainJumpTiming(iggy3d::ProductAppWindowState& window) {
  window.gameplay.gameplayJump.coyoteSecondsRemaining = 0.08F;
  window.gameplay.gameplayJump.bufferSecondsRemaining = 0.06F;
  window.gameplay.gameplayJump.held = true;
  window.gameplay.gameplayJump.cutApplied = true;
}

void showCollisionOverlay(iggy3d::ProductAppWindowState& window) {
  window.debugHud.devCollisionOverlay.visible = true;
  window.debugHud.devCollisionOverlay.status = "dev_collision_overlay_enabled";
  window.debugHud.devCollisionOverlay.reasonCode =
      window.debugHud.devCollisionOverlay.status;
}

void showRoomEditorTransients(iggy3d::ProductAppWindowState& window) {
  window.roomEditing.ready = true;
  window.roomEditorCursorReady = true;
  window.roomEditorOverlay.visible = true;
  window.roomEditorOverlay.status = "room_editor_overlay_visible";
  window.roomEditorOverlay.reasonCode = window.roomEditorOverlay.status;
  window.roomEditorOverlay.itemCount = 3;
  window.roomEditorPreview.active = true;
  window.roomEditorPlacementPreview.ok = true;
  window.roomEditorPreview.visible = true;
  window.roomEditorPreview.status = "room_editor_preview_ready";
  window.roomEditorPreview.reasonCode = window.roomEditorPreview.status;
  window.roomEditorPreview.candidateId = "preview_floor";
  window.roomEditorPreview.tool = "wall";
  window.roomEditorPreview.gridX = 4;
  window.roomEditorPreview.gridZ = 5;
  window.roomEditorPreview.beforeDrawCount = 7;
  window.roomEditorPreview.afterDrawCount = 8;
  window.roomEditorPreview.optimizedDrawDelta = 1;
  window.roomEditorHud.visible = true;
  window.roomEditorHud.status = "room_editor_hud_visible";
  window.roomEditorHud.reasonCode = window.roomEditorHud.status;
  window.roomEditorHud.previewActive = true;
  window.roomEditorHud.previewStatus = "room_editor_preview_ready";
  window.roomEditorHud.previewCandidateId = "preview_floor";
  window.roomEditorHud.previewOptimizedDrawDelta = 1;
  window.roomEditorHud.lineCount = 4;
  window.viewport.productDrawRoomEditorCursorVisible = true;
  window.viewport.productDrawRoomEditorCursorCount = 1;
  window.viewport.productDrawRoomEditorPreviewVisible = true;
  window.viewport.productDrawRoomEditorPreviewCount = 1;
}

iggy3d::RenderReceipt receiptFor(const iggy3d::FrontendState& frontend,
                                 const iggy3d::FrontendSettings& settings,
                                 const iggy3d::ProductAppWindowState& window) {
  iggy3d::ProductAppOptions options;
  iggy3d::ProductWorldTemplate world;
  iggy3d::ProductSaveBridgeResult saves;
  return iggy3d::buildProductAppReceipt(options, world, frontend, settings, window,
                                        saves);
}

bool expectReceiptField(const iggy3d::RenderReceipt& receipt,
                        const char* key,
                        const char* value,
                        const char* message) {
  return expect(iggy3d::hasReceiptField(receipt, key, value), message);
}

}  // namespace

int main() {
  bool ok = true;

  iggy3d::FrontendState frontend;
  iggy3d::ProductAppWindowState window;
  initializeProductStarterTransition(frontend, window, false);
  ok &= expect(frontend.screen == iggy3d::FrontendScreen::Starter,
               "starter screen initialized");
  ok &= expect(frontend.selectedAction == iggy3d::FrontendAction::NewWorld,
               "starter selects new world without compatible save");
  ok &= expect(liveSurface(frontend, window).inputOwner == iggy3d::MenuOwner::Starter,
               "starter owns input");
  ok &= expect(liveSurface(frontend, window).gameplayInputSuppressed, "starter suppresses gameplay input");
  ok &= expect(window.gameplay.productTransition.status == "starter_ready",
               "starter transition status");
  iggy3d::FrontendSettings settings;
  iggy3d::ProductAppWindowState staleStarterWindow = window;
  staleStarterWindow.gameplay.gameplayActive = true;
  staleStarterWindow.gameplay.runtimeSessionCreated = true;
  const iggy3d::RenderReceipt staleStarterReceipt =
      receiptFor(frontend, settings, staleStarterWindow);
  ok &= expectReceiptField(staleStarterReceipt, "active_surface", "starter",
                           "stale starter receipt active surface");
  ok &= expectReceiptField(staleStarterReceipt, "input_surface", "starter",
                           "stale starter receipt input surface");
  ok &= expectReceiptField(staleStarterReceipt, "input_owner", "starter",
                           "stale starter receipt owner");
  ok &= expectReceiptField(staleStarterReceipt, "gameplay_input_suppressed",
                           "true",
                           "stale starter receipt suppresses gameplay");
  ok &= expectReceiptField(staleStarterReceipt, "pause_menu_open", "false",
                           "stale starter receipt derives pause closed");
  ok &= expectReceiptField(staleStarterReceipt, "dev_tools_open", "false",
                           "stale starter receipt derives dev tools closed");

  window.gameplay.gameplayActive = true;
  window.gameplay.runtimeSessionCreated = true;
  enterProductGameplayTransition(frontend, window,
                                 iggy3d::FrontendAction::CreateAndEnter);
  ok &= expect(frontend.screen == iggy3d::FrontendScreen::Gameplay,
               "gameplay screen after launch");
  ok &= expect(!frontend.inputOwned, "gameplay owns no frontend input");
  ok &= expect(liveSurface(frontend, window).inputOwner == iggy3d::MenuOwner::Gameplay,
               "gameplay input owner");
  ok &= expect(!liveSurface(frontend, window).gameplayInputSuppressed, "gameplay input accepted");
  ok &= expect(window.gameplay.productTransition.returnedToGameplay,
               "gameplay transition returned to gameplay");
  ok &= expect(window.gameplay.productTransition.sessionPreserved,
               "gameplay launch preserves session");
  const iggy3d::RenderReceipt gameplayReceipt =
      receiptFor(frontend, settings, window);
  ok &= expectReceiptField(gameplayReceipt, "active_surface", "gameplay",
                           "gameplay receipt active surface");
  ok &= expectReceiptField(gameplayReceipt, "input_surface", "gameplay",
                           "gameplay receipt input surface");
  ok &= expectReceiptField(gameplayReceipt, "input_owner", "gameplay",
                           "gameplay receipt owner");
  ok &= expectReceiptField(gameplayReceipt, "gameplay_input_suppressed",
                           "false",
                           "gameplay receipt unsuppressed");
  ok &= expectReceiptField(gameplayReceipt, "pause_menu_open", "false",
                           "gameplay receipt pause closed");
  ok &= expectReceiptField(gameplayReceipt, "dev_tools_open", "false",
                           "gameplay receipt dev tools closed");
  window.inputDevice.interactionMode = iggy3d::ProductInteractionMode::Creative;
  const iggy3d::RenderReceipt creativeGameplayReceipt =
      receiptFor(frontend, settings, window);
  ok &= expectReceiptField(creativeGameplayReceipt, "map_maker_active", "false",
                           "creative gameplay alone is not map maker live");
  window.inputDevice.interactionMode = iggy3d::ProductInteractionMode::Player;

  activateMapMaker(window);
  const iggy3d::RenderReceipt activeMapMakerReceipt =
      receiptFor(frontend, settings, window);
  ok &= expectReceiptField(activeMapMakerReceipt, "map_maker_active", "true",
                           "explicit map maker receipt is live");
  showMovementTuning(window);
  retainGroundVelocity(window);
  retainJumpTiming(window);
  showCollisionOverlay(window);
  settings.debugOverlayEnabled = true;
  openProductPauseTransition(frontend, window, iggy3d::FrontendAction::Resume);
  ok &= expect(frontend.screen == iggy3d::FrontendScreen::Pause,
               "pause screen opened");
  ok &= expect(iggy3d::frontendPauseMenuOpen(frontend), "pause menu open");
  ok &= expect(liveSurface(frontend, window).inputOwner == iggy3d::MenuOwner::Pause,
               "pause owns input");
  ok &= expect(liveSurface(frontend, window).gameplayInputSuppressed, "pause suppresses gameplay input");
  ok &= expect(window.gameplay.gameplayMovement.groundVelocityX == 0.0F &&
                   window.gameplay.gameplayMovement.groundVelocityZ == 0.0F,
               "pause clears retained ground velocity");
  ok &= expect(window.gameplay.gameplayJump.coyoteSecondsRemaining == 0.0F &&
                   window.gameplay.gameplayJump.bufferSecondsRemaining == 0.0F &&
                   !window.gameplay.gameplayJump.held &&
                   !window.gameplay.gameplayJump.cutApplied,
               "pause clears jump timing state");
  ok &= expect(window.gameplay.productTransition.sessionPreserved,
               "pause keeps session active");
  ok &= expect(!iggy3d::productMapMakerLiveForWindow(frontend, window),
               "pause clears map maker live state");
  ok &= expect(window.inputDevice.interactionMode ==
                   iggy3d::ProductInteractionMode::Player,
               "pause returns interaction mode to player");
  ok &= expect(!window.viewport.creativeFlyActive,
               "pause clears creative fly active");
  ok &= expect(!window.gameplay.gameplayMovement.tuningVisible,
               "pause clears movement tuning");
  ok &= expect(settings.debugOverlayEnabled,
               "pause preserves debug overlay setting");
  ok &= expect(window.debugHud.devCollisionOverlay.visible,
               "pause preserves collision overlay state");
  const iggy3d::RenderReceipt pauseReceipt =
      receiptFor(frontend, settings, window);
  ok &= expectReceiptField(pauseReceipt, "active_surface", "pause",
                           "pause receipt active surface");
  ok &= expectReceiptField(pauseReceipt, "input_owner", "pause",
                           "pause receipt owner");
  ok &= expectReceiptField(pauseReceipt, "gameplay_input_suppressed", "true",
                           "pause receipt suppresses gameplay");
  ok &= expectReceiptField(pauseReceipt, "pause_menu_open", "true",
                           "pause receipt derives pause open");
  ok &= expectReceiptField(pauseReceipt, "dev_tools_open", "false",
                           "pause receipt derives dev tools closed");
  window.inputDevice.interactionMode = iggy3d::ProductInteractionMode::Creative;
  const iggy3d::RenderReceipt stalePauseMapMakerReceipt =
      receiptFor(frontend, settings, window);
  ok &= expectReceiptField(stalePauseMapMakerReceipt, "map_maker_active", "false",
                           "stale pause receipt derives map maker inactive");

  showMovementTuning(window);
  iggy3d::FrontendSettingsTab settingsTab = iggy3d::FrontendSettingsTab::None;
  openProductPauseSettingsTransition(frontend, window, settingsTab);
  ok &= expect(frontend.screen == iggy3d::FrontendScreen::Settings,
               "settings opened from pause");
  ok &= expect(frontend.childScreen == iggy3d::FrontendScreen::Pause,
               "settings parent is pause");
  ok &= expect(settingsTab == iggy3d::FrontendSettingsTab::Input,
               "settings opens input tab");
  ok &= expect(liveSurface(frontend, window).inputOwner == iggy3d::MenuOwner::Settings,
               "settings owns input");
  ok &= expect(!iggy3d::productMapMakerLiveForWindow(frontend, window),
               "settings keeps map maker inactive");
  ok &= expect(window.inputDevice.interactionMode ==
                   iggy3d::ProductInteractionMode::Player,
               "settings keeps player interaction mode");
  ok &= expect(!window.gameplay.gameplayMovement.tuningVisible,
               "settings keeps movement tuning cleared");
  const iggy3d::RenderReceipt settingsReceipt =
      receiptFor(frontend, settings, window);
  ok &= expectReceiptField(settingsReceipt, "active_surface", "settings",
                           "settings receipt active surface");
  ok &= expectReceiptField(settingsReceipt, "input_surface", "settings",
                           "settings receipt input surface");
  ok &= expectReceiptField(settingsReceipt, "input_owner", "settings",
                           "settings receipt owner");
  ok &= expectReceiptField(settingsReceipt, "gameplay_input_suppressed", "true",
                           "settings receipt suppresses gameplay");
  ok &= expectReceiptField(settingsReceipt, "pause_menu_open", "false",
                           "settings receipt derives pause closed");
  ok &= expectReceiptField(settingsReceipt, "dev_tools_open", "false",
                           "settings receipt derives dev tools closed");
  window.inputDevice.interactionMode = iggy3d::ProductInteractionMode::Creative;
  const iggy3d::RenderReceipt staleSettingsMapMakerReceipt =
      receiptFor(frontend, settings, window);
  ok &= expectReceiptField(staleSettingsMapMakerReceipt, "map_maker_active", "false",
                           "stale settings receipt derives map maker inactive");

  showMovementTuning(window);
  openProductPauseTransition(frontend, window, iggy3d::FrontendAction::Settings);
  ok &= expect(frontend.screen == iggy3d::FrontendScreen::Pause,
               "settings back returns to pause");
  ok &= expect(frontend.selectedAction == iggy3d::FrontendAction::Settings,
               "pause remembers settings row");
  ok &= expect(!iggy3d::productMapMakerLiveForWindow(frontend, window),
               "settings back keeps map maker inactive");
  ok &= expect(!window.gameplay.gameplayMovement.tuningVisible,
               "settings back keeps movement tuning cleared");

  showMovementTuning(window);
  openProductPauseDevToolsTransition(frontend, window,
                                     iggy3d::FrontendDevToolsCategory::Session);
  ok &= expect(frontend.screen == iggy3d::FrontendScreen::DevOverlay,
               "dev overlay opened");
  ok &= expect(iggy3d::frontendDevToolsOpen(frontend), "dev tools open");
  ok &= expect(liveSurface(frontend, window).inputOwner == iggy3d::MenuOwner::DevTools,
               "dev tools own input");
  ok &= expect(liveSurface(frontend, window).gameplayInputSuppressed, "dev tools suppress gameplay input");
  ok &= expect(!iggy3d::productMapMakerLiveForWindow(frontend, window),
               "dev tools keeps map maker inactive");
  ok &= expect(!window.gameplay.gameplayMovement.tuningVisible,
               "dev tools clears movement tuning");
  const iggy3d::RenderReceipt devToolsReceipt =
      receiptFor(frontend, settings, window);
  ok &= expectReceiptField(devToolsReceipt, "active_surface", "dev_tools",
                           "dev tools receipt active surface");
  ok &= expectReceiptField(devToolsReceipt, "input_surface", "dev_tools",
                           "dev tools receipt input surface");
  ok &= expectReceiptField(devToolsReceipt, "input_owner", "dev_tools",
                           "dev tools receipt owner");
  ok &= expectReceiptField(devToolsReceipt, "gameplay_input_suppressed", "true",
                           "dev tools receipt suppresses gameplay");
  ok &= expectReceiptField(devToolsReceipt, "pause_menu_open", "false",
                           "dev tools receipt derives pause closed");
  ok &= expectReceiptField(devToolsReceipt, "dev_tools_open", "true",
                           "dev tools receipt derives dev tools open");
  window.inputDevice.interactionMode = iggy3d::ProductInteractionMode::Creative;
  const iggy3d::RenderReceipt staleDevToolsMapMakerReceipt =
      receiptFor(frontend, settings, window);
  ok &= expectReceiptField(staleDevToolsMapMakerReceipt, "map_maker_active", "false",
                           "stale dev tools receipt derives map maker inactive");
  iggy3d::clearProductMapMakerMode(window);

  closeProductOverlayToGameplayTransition(frontend, window);
  ok &= expect(frontend.screen == iggy3d::FrontendScreen::Gameplay,
               "resume returns to gameplay");
  ok &= expect(!frontend.inputOwned, "resume releases frontend input");
  ok &= expect(liveSurface(frontend, window).inputOwner == iggy3d::MenuOwner::Gameplay,
               "resume restores gameplay owner");
  ok &= expect(window.gameplay.productTransition.returnedToGameplay,
               "resume transition status");
  ok &= expect(!iggy3d::productMapMakerLiveForWindow(frontend, window),
               "resume does not restore map maker");
  ok &= expect(window.inputDevice.interactionMode ==
                   iggy3d::ProductInteractionMode::Player,
               "resume stays player mode");
  ok &= expect(!window.gameplay.gameplayMovement.tuningVisible,
               "resume does not restore movement tuning");
  window.roomEditing.ready = true;
  window.inputDevice.interactionMode = iggy3d::ProductInteractionMode::Creative;
  const iggy3d::RenderReceipt editorReceipt =
      receiptFor(frontend, settings, window);
  ok &= expectReceiptField(editorReceipt, "active_surface", "editor",
                           "editor receipt active surface");
  ok &= expectReceiptField(editorReceipt, "input_surface", "room_editor",
                           "editor receipt input surface");
  ok &= expectReceiptField(editorReceipt, "input_owner", "editor",
                           "editor receipt owner");
  ok &= expectReceiptField(editorReceipt, "gameplay_input_suppressed", "true",
                           "editor receipt suppresses gameplay");
  ok &= expectReceiptField(editorReceipt, "map_maker_active", "false",
                           "editor creative receipt derives map maker inactive");

  openProductPauseTransition(frontend, window, iggy3d::FrontendAction::ReturnToTitle);
  activateMapMaker(window);
  showMovementTuning(window);
  retainGroundVelocity(window);
  retainJumpTiming(window);
  showCollisionOverlay(window);
  showRoomEditorTransients(window);
  settings.debugOverlayEnabled = true;
  returnProductToTitleTransition(frontend, window, settings);
  ok &= expect(frontend.screen == iggy3d::FrontendScreen::Starter,
               "return to title opens starter");
  ok &= expect(frontend.returnToTitleRequested, "return to title requested");
  ok &= expect(!window.gameplay.gameplayActive, "return to title clears gameplay active");
  ok &= expect(!window.gameplay.runtimeSessionCreated, "return to title clears session flag");
  ok &= expect(window.gameplay.gameplayMovement.groundVelocityX == 0.0F &&
                   window.gameplay.gameplayMovement.groundVelocityZ == 0.0F,
               "return to title clears retained ground velocity");
  ok &= expect(window.gameplay.gameplayJump.coyoteSecondsRemaining == 0.0F &&
                   window.gameplay.gameplayJump.bufferSecondsRemaining == 0.0F &&
                   !window.gameplay.gameplayJump.held &&
                   !window.gameplay.gameplayJump.cutApplied,
               "return to title clears jump timing state");
  ok &= expect(liveSurface(frontend, window).inputOwner == iggy3d::MenuOwner::Starter,
               "return to title restores starter owner");
  ok &= expect(window.inputDevice.interactionMode ==
                   iggy3d::ProductInteractionMode::Player,
               "return to title resets interaction mode");
  ok &= expect(!iggy3d::productMapMakerLiveForWindow(frontend, window),
               "return to title clears map maker live state");
  ok &= expect(!window.viewport.creativeFlyActive,
               "return to title clears creative fly active");
  ok &= expect(!window.gameplay.gameplayMovement.tuningVisible,
               "return to title clears movement tuning");
  ok &= expect(!settings.debugOverlayEnabled,
               "return to title clears debug overlay setting");
  ok &= expect(!window.debugHud.devCollisionOverlay.visible,
               "return to title clears collision overlay");
  ok &= expect(window.debugHud.devCollisionOverlay.status ==
                   "dev_collision_overlay_hidden",
               "return to title collision overlay status hidden");
  ok &= expect(window.roomEditing.ready,
               "return to title preserves room editing document");
  ok &= expect(!window.roomEditorCursorReady,
               "return to title clears room editor cursor transient");
  ok &= expect(!window.roomEditorOverlay.visible,
               "return to title clears room editor overlay");
  ok &= expect(!window.roomEditorPreview.active,
               "return to title clears room editor preview active");
  ok &= expect(!window.roomEditorPreview.visible,
               "return to title clears room editor preview visible");
  ok &= expect(window.roomEditorPreview.status ==
                   "room_editor_preview_not_requested",
               "return to title clears room editor preview status");
  ok &= expect(!window.roomEditorHud.visible,
               "return to title clears room editor hud");
  ok &= expect(window.roomEditorHud.status == "room_editor_hud_not_ready",
               "return to title room editor hud hidden status");
  ok &= expect(!window.viewport.productDrawRoomEditorCursorVisible,
               "return to title clears room editor cursor draw");
  ok &= expect(!window.viewport.productDrawRoomEditorPreviewVisible,
               "return to title clears room editor preview draw");
  ok &= expect(window.gameplay.productTransition.returnedToTitle,
               "return to title transition status");
  ok &= expect(!window.gameplay.productTransition.sessionPreserved,
               "return to title does not preserve session");
  const iggy3d::RenderReceipt receipt =
      receiptFor(frontend, settings, window);
  ok &= expectReceiptField(receipt, "active_surface", "starter",
                           "return title receipt active surface");
  ok &= expectReceiptField(receipt, "input_surface", "starter",
                           "return title receipt input surface");
  ok &= expectReceiptField(receipt, "input_owner", "starter",
                           "return title receipt owner");
  ok &= expectReceiptField(receipt, "gameplay_input_suppressed", "true",
                           "return title receipt suppresses gameplay");
  ok &= expectReceiptField(receipt, "pause_menu_open", "false",
                           "return title receipt derives pause closed");
  ok &= expectReceiptField(receipt, "dev_tools_open", "false",
                           "return title receipt derives dev tools closed");
  ok &= expect(iggy3d::hasReceiptField(receipt,
                                       "settings_debug_overlay_enabled",
                                       "false"),
               "receipt debug overlay disabled");
  ok &= expect(iggy3d::hasReceiptField(receipt,
                                       "dev_collision_overlay_visible",
                                       "false"),
               "receipt collision overlay hidden");
  ok &= expect(iggy3d::hasReceiptField(receipt,
                                       "gameplay_movement_tuning_visible",
                                       "false"),
               "receipt movement tuning hidden");
  ok &= expect(iggy3d::hasReceiptField(receipt, "map_maker_active", "false"),
               "receipt map maker inactive");
  ok &= expect(iggy3d::hasReceiptField(receipt,
                                       "room_editor_hud_visible",
                                       "false"),
               "receipt room editor hud hidden");

  if (!ok) {
    return 1;
  }
  std::cout << "product_menu_transitions_tests=pass\n";
  return 0;
}
