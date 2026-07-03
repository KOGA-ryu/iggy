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

void activateMapMaker(iggy3d::ProductAppWindowState& window) {
  window.interactionMode = iggy3d::ProductInteractionMode::Creative;
  window.viewport.creativeFlyActive = true;
  window.viewport.creativeFlyStatus = "creative_fly_applied";
  window.viewport.creativeFlyReasonCode = window.viewport.creativeFlyStatus;
  window.viewport.creativeFlySpeedMetersPerSecond = 8.0F;
  window.mapMakerStatus = "map_maker_active";
  window.mapMakerReasonCode = window.mapMakerStatus;
}

void showMovementTuning(iggy3d::ProductAppWindowState& window) {
  window.gameplayMovementTuningVisible = true;
  window.gameplayMovementTuningStatus = "movement_tuning_visible";
  window.gameplayMovementTuningReasonCode = window.gameplayMovementTuningStatus;
}

void retainGroundVelocity(iggy3d::ProductAppWindowState& window) {
  window.gameplayMovementGroundVelocityX = 1.25F;
  window.gameplayMovementGroundVelocityZ = -2.5F;
}

void retainJumpTiming(iggy3d::ProductAppWindowState& window) {
  window.gameplayJumpCoyoteSecondsRemaining = 0.08F;
  window.gameplayJumpBufferSecondsRemaining = 0.06F;
  window.gameplayJumpHeld = true;
  window.gameplayJumpCutApplied = true;
}

void showCollisionOverlay(iggy3d::ProductAppWindowState& window) {
  window.devCollisionOverlayVisible = true;
  window.devCollisionOverlayStatus = "dev_collision_overlay_enabled";
  window.devCollisionOverlayReasonCode = window.devCollisionOverlayStatus;
}

void showRoomEditorTransients(iggy3d::ProductAppWindowState& window) {
  window.roomEditing.ready = true;
  window.roomEditorCursorReady = true;
  window.roomEditorOverlayVisible = true;
  window.roomEditorOverlayStatus = "room_editor_overlay_visible";
  window.roomEditorOverlayReasonCode = window.roomEditorOverlayStatus;
  window.roomEditorOverlayItemCount = 3;
  window.roomEditorPreviewActive = true;
  window.roomEditorPlacementPreview.ok = true;
  window.roomEditorPreviewVisible = true;
  window.roomEditorPreviewStatus = "room_editor_preview_ready";
  window.roomEditorPreviewReasonCode = window.roomEditorPreviewStatus;
  window.roomEditorPreviewCandidateId = "preview_floor";
  window.roomEditorPreviewTool = "wall";
  window.roomEditorPreviewGridX = 4;
  window.roomEditorPreviewGridZ = 5;
  window.roomEditorPreviewBeforeDrawCount = 7;
  window.roomEditorPreviewAfterDrawCount = 8;
  window.roomEditorPreviewOptimizedDrawDelta = 1;
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
  ok &= expect(window.inputOwner == iggy3d::MenuOwner::Starter,
               "starter owns input");
  ok &= expect(window.gameplayInputSuppressed, "starter suppresses gameplay input");
  ok &= expect(window.productTransitionStatus == "starter_ready",
               "starter transition status");
  iggy3d::FrontendSettings settings;
  iggy3d::ProductAppWindowState staleStarterWindow = window;
  staleStarterWindow.gameplayActive = true;
  staleStarterWindow.runtimeSessionCreated = true;
  staleStarterWindow.inputOwner = iggy3d::MenuOwner::Gameplay;
  staleStarterWindow.gameplayInputSuppressed = false;
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

  window.gameplayActive = true;
  window.runtimeSessionCreated = true;
  enterProductGameplayTransition(frontend, window,
                                 iggy3d::FrontendAction::CreateAndEnter);
  ok &= expect(frontend.screen == iggy3d::FrontendScreen::Gameplay,
               "gameplay screen after launch");
  ok &= expect(!frontend.inputOwned, "gameplay owns no frontend input");
  ok &= expect(window.inputOwner == iggy3d::MenuOwner::Gameplay,
               "gameplay input owner");
  ok &= expect(!window.gameplayInputSuppressed, "gameplay input accepted");
  ok &= expect(window.productTransitionReturnedToGameplay,
               "gameplay transition returned to gameplay");
  ok &= expect(window.productTransitionSessionPreserved,
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
  window.interactionMode = iggy3d::ProductInteractionMode::Creative;
  const iggy3d::RenderReceipt creativeGameplayReceipt =
      receiptFor(frontend, settings, window);
  ok &= expectReceiptField(creativeGameplayReceipt, "map_maker_active", "false",
                           "creative gameplay alone is not map maker live");
  window.interactionMode = iggy3d::ProductInteractionMode::Player;

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
  ok &= expect(window.inputOwner == iggy3d::MenuOwner::Pause,
               "pause owns input");
  ok &= expect(window.gameplayInputSuppressed, "pause suppresses gameplay input");
  ok &= expect(window.gameplayMovementGroundVelocityX == 0.0F &&
                   window.gameplayMovementGroundVelocityZ == 0.0F,
               "pause clears retained ground velocity");
  ok &= expect(window.gameplayJumpCoyoteSecondsRemaining == 0.0F &&
                   window.gameplayJumpBufferSecondsRemaining == 0.0F &&
                   !window.gameplayJumpHeld &&
                   !window.gameplayJumpCutApplied,
               "pause clears jump timing state");
  ok &= expect(window.productTransitionSessionPreserved,
               "pause keeps session active");
  ok &= expect(!iggy3d::productMapMakerLiveForWindow(frontend, window),
               "pause clears map maker live state");
  ok &= expect(window.interactionMode == iggy3d::ProductInteractionMode::Player,
               "pause returns interaction mode to player");
  ok &= expect(!window.viewport.creativeFlyActive,
               "pause clears creative fly active");
  ok &= expect(!window.gameplayMovementTuningVisible,
               "pause clears movement tuning");
  ok &= expect(settings.debugOverlayEnabled,
               "pause preserves debug overlay setting");
  ok &= expect(window.devCollisionOverlayVisible,
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
  window.interactionMode = iggy3d::ProductInteractionMode::Creative;
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
  ok &= expect(window.inputOwner == iggy3d::MenuOwner::Settings,
               "settings owns input");
  ok &= expect(!iggy3d::productMapMakerLiveForWindow(frontend, window),
               "settings keeps map maker inactive");
  ok &= expect(window.interactionMode == iggy3d::ProductInteractionMode::Player,
               "settings keeps player interaction mode");
  ok &= expect(!window.gameplayMovementTuningVisible,
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
  window.interactionMode = iggy3d::ProductInteractionMode::Creative;
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
  ok &= expect(!window.gameplayMovementTuningVisible,
               "settings back keeps movement tuning cleared");

  showMovementTuning(window);
  openProductPauseDevToolsTransition(frontend, window,
                                     iggy3d::FrontendDevToolsCategory::Session);
  ok &= expect(frontend.screen == iggy3d::FrontendScreen::DevOverlay,
               "dev overlay opened");
  ok &= expect(iggy3d::frontendDevToolsOpen(frontend), "dev tools open");
  ok &= expect(window.inputOwner == iggy3d::MenuOwner::DevTools,
               "dev tools own input");
  ok &= expect(window.gameplayInputSuppressed, "dev tools suppress gameplay input");
  ok &= expect(!iggy3d::productMapMakerLiveForWindow(frontend, window),
               "dev tools keeps map maker inactive");
  ok &= expect(!window.gameplayMovementTuningVisible,
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
  window.interactionMode = iggy3d::ProductInteractionMode::Creative;
  const iggy3d::RenderReceipt staleDevToolsMapMakerReceipt =
      receiptFor(frontend, settings, window);
  ok &= expectReceiptField(staleDevToolsMapMakerReceipt, "map_maker_active", "false",
                           "stale dev tools receipt derives map maker inactive");
  iggy3d::clearProductMapMakerMode(window);

  closeProductOverlayToGameplayTransition(frontend, window);
  ok &= expect(frontend.screen == iggy3d::FrontendScreen::Gameplay,
               "resume returns to gameplay");
  ok &= expect(!frontend.inputOwned, "resume releases frontend input");
  ok &= expect(window.inputOwner == iggy3d::MenuOwner::Gameplay,
               "resume restores gameplay owner");
  ok &= expect(window.productTransitionReturnedToGameplay,
               "resume transition status");
  ok &= expect(!iggy3d::productMapMakerLiveForWindow(frontend, window),
               "resume does not restore map maker");
  ok &= expect(window.interactionMode == iggy3d::ProductInteractionMode::Player,
               "resume stays player mode");
  ok &= expect(!window.gameplayMovementTuningVisible,
               "resume does not restore movement tuning");
  window.roomEditing.ready = true;
  window.interactionMode = iggy3d::ProductInteractionMode::Creative;
  window.inputOwner = iggy3d::MenuOwner::Gameplay;
  window.gameplayInputSuppressed = false;
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
  ok &= expect(!window.gameplayActive, "return to title clears gameplay active");
  ok &= expect(!window.runtimeSessionCreated, "return to title clears session flag");
  ok &= expect(window.gameplayMovementGroundVelocityX == 0.0F &&
                   window.gameplayMovementGroundVelocityZ == 0.0F,
               "return to title clears retained ground velocity");
  ok &= expect(window.gameplayJumpCoyoteSecondsRemaining == 0.0F &&
                   window.gameplayJumpBufferSecondsRemaining == 0.0F &&
                   !window.gameplayJumpHeld &&
                   !window.gameplayJumpCutApplied,
               "return to title clears jump timing state");
  ok &= expect(window.inputOwner == iggy3d::MenuOwner::Starter,
               "return to title restores starter owner");
  ok &= expect(window.interactionMode == iggy3d::ProductInteractionMode::Player,
               "return to title resets interaction mode");
  ok &= expect(!iggy3d::productMapMakerLiveForWindow(frontend, window),
               "return to title clears map maker live state");
  ok &= expect(!window.viewport.creativeFlyActive,
               "return to title clears creative fly active");
  ok &= expect(!window.gameplayMovementTuningVisible,
               "return to title clears movement tuning");
  ok &= expect(!settings.debugOverlayEnabled,
               "return to title clears debug overlay setting");
  ok &= expect(!window.devCollisionOverlayVisible,
               "return to title clears collision overlay");
  ok &= expect(window.devCollisionOverlayStatus == "dev_collision_overlay_hidden",
               "return to title collision overlay status hidden");
  ok &= expect(window.roomEditing.ready,
               "return to title preserves room editing document");
  ok &= expect(!window.roomEditorCursorReady,
               "return to title clears room editor cursor transient");
  ok &= expect(!window.roomEditorOverlayVisible,
               "return to title clears room editor overlay");
  ok &= expect(!window.roomEditorPreviewActive,
               "return to title clears room editor preview active");
  ok &= expect(!window.roomEditorPreviewVisible,
               "return to title clears room editor preview visible");
  ok &= expect(window.roomEditorPreviewStatus ==
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
  ok &= expect(window.productTransitionReturnedToTitle,
               "return to title transition status");
  ok &= expect(!window.productTransitionSessionPreserved,
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
