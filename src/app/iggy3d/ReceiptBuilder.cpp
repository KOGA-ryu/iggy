#include "app/iggy3d/ReceiptBuilder.hpp"

#include <charconv>

#include "app/frontend/FrontendReceipt.hpp"
#include "app/iggy3d/ProductGameplayFeedback.hpp"

namespace iggy3d {
namespace {

std::string floatReceiptValue(float value) {
  char buffer[32]{};
  const auto [ptr, error] =
      std::to_chars(buffer, buffer + sizeof(buffer), value, std::chars_format::fixed, 3);
  if (error != std::errc{}) {
    return "unavailable";
  }
  return std::string(buffer, static_cast<std::size_t>(ptr - buffer));
}

}  // namespace

RenderReceipt buildProductAppReceipt(const ProductAppOptions& options,
                                     const ProductWorldTemplate& world,
                                     const FrontendState& frontend,
                                     const FrontendSettings& settings,
                                     const ProductAppWindowState& window,
                                     const ProductSaveBridgeResult& saves) {
  RenderReceipt receipt;
  const ProductGameplayFeedback feedback = buildProductGameplayFeedback(window);
  appendReceiptField(receipt, "app", "iggy3d");
  appendReceiptField(receipt, "app_surface", "product");
  appendReceiptField(receipt, "opening_menu", frontend.screen == FrontendScreen::Starter);
  appendReceiptField(receipt, "frontend_screen", frontendScreenName(frontend.screen));
  appendReceiptField(receipt, "frontend_child_screen", frontendScreenName(frontend.childScreen));
  appendReceiptField(receipt, "frontend_selected_action",
                     frontendActionName(frontend.selectedAction));
  appendReceiptField(receipt, "frontend_status", frontend.status);
  appendReceiptField(receipt, "frontend_launch_requested", frontend.launchRequested);
  appendReceiptField(receipt, "frontend_return_to_title_requested",
                     frontend.returnToTitleRequested);
  appendReceiptField(receipt, "pause_menu_open", frontend.pauseMenuOpen);
  appendReceiptField(receipt, "dev_tools_open", frontend.devToolsOpen);
  appendReceiptField(receipt, "starter_world_suppressed", !window.gameplayActive);
  appendReceiptField(receipt, "auto_new_world", options.autoNewWorld);
  appendReceiptField(receipt, "renderer_request", productRendererRequestName(options.renderer));
  appendReceiptField(receipt, "window_mode", productWindowModeName(options.windowMode));
  appendReceiptField(receipt, "input_backend", productInputBackendName(options.inputBackend));
  appendReceiptField(receipt, "settings_input_backend",
                     frontendInputBackendName(settings.inputBackend));
  appendReceiptField(receipt, "settings_selected_tab",
                     frontendSettingsTabName(window.selectedSettingsTab));
  appendReceiptField(receipt, "settings_renderer",
                     frontendRendererChoiceName(settings.renderer));
  appendReceiptField(receipt, "settings_window_mode",
                     frontendWindowModeName(settings.windowMode));
  appendReceiptField(receipt, "settings_camera_mode",
                     frontendCameraModeName(settings.cameraMode));
  appendReceiptField(receipt, "settings_look_sensitivity",
                     floatReceiptValue(settings.lookSensitivity));
  appendReceiptField(receipt, "settings_controller_look_sensitivity",
                     floatReceiptValue(settings.controllerLookSensitivity));
  appendReceiptField(receipt, "settings_invert_look", settings.invertLook);
  appendReceiptField(receipt, "settings_developer_tools_enabled",
                     settings.devToolsEnabled);
  appendReceiptField(receipt, "window_requested", window.requested);
  appendReceiptField(receipt, "window_shell", window.sdlAvailable ? "sdl3" : "unavailable");
  appendReceiptField(receipt, "window_created", window.created);
  appendReceiptField(receipt, "window_drawable", window.drawable);
  appendReceiptField(receipt, "window_title",
                     window.gameplayActive ? "iggy3d - Gameplay" : "iggy3d - Opening Menu");
  appendReceiptField(receipt, "opening_menu_visible", window.openingMenuVisible);
  appendReceiptField(receipt, "menu_text_drawn", window.menuTextDrawn);
  appendReceiptField(receipt, "menu_selected_row_drawn", window.selectedRowDrawn);
  appendReceiptField(receipt, "menu_row_count", window.menuRowCount);
  appendReceiptField(receipt, "mouse_menu_select_used", window.mouseMenuSelectUsed);
  appendReceiptField(receipt, "gamepad_available", window.gamepadAvailable);
  appendReceiptField(receipt, "gamepad_name", window.gamepadName);
  appendReceiptField(receipt, "gamepad_mapping", window.gamepadMapping);
  appendReceiptField(receipt, "gamepad_menu_select_used", window.gamepadMenuSelectUsed);
  appendReceiptField(receipt, "dev_tools_category",
                     frontendDevToolsCategoryName(frontend.devToolsCategory));
  appendReceiptField(receipt, "launch_action", window.launchAction);
  appendReceiptField(receipt, "launch_status", window.launchStatus);
  appendReceiptField(receipt, "package_load_status", window.packageLoadStatus);
  appendReceiptField(receipt, "world_setup_title", window.worldSetupTitle);
  appendReceiptField(receipt, "world_setup_status", window.worldSetupStatus);
  appendReceiptField(receipt, "world_creation_status", window.worldCreationStatus);
  appendReceiptField(receipt, "world_creation_reason_code",
                     window.worldCreationReasonCode);
  appendReceiptField(receipt, "world_creation_world_id", window.worldCreationWorldId);
  appendReceiptField(receipt, "world_creation_world_title",
                     window.worldCreationWorldTitle);
  appendReceiptField(receipt, "world_creation_initial_save_requested",
                     window.worldCreationInitialSaveRequested);
  appendReceiptField(receipt, "world_creation_initial_save_written",
                     window.worldCreationInitialSaveWritten);
  appendReceiptField(receipt, "world_creation_initial_save_id",
                     window.worldCreationInitialSaveId);
  appendReceiptField(receipt, "world_creation_initial_save_title",
                     window.worldCreationInitialSaveTitle);
  appendReceiptField(receipt, "world_creation_route_after_create",
                     window.worldCreationRouteAfterCreate);
  appendReceiptField(receipt, "product_save_status", window.productSaveStatus);
  appendReceiptField(receipt, "product_save_reason_code",
                     window.productSaveReasonCode);
  appendReceiptField(receipt, "product_save_durable_reason",
                     window.productSaveDurableReason);
  appendReceiptField(receipt, "product_save_source", window.productSaveSource);
  appendReceiptField(receipt, "product_save_save_id", window.productSaveSaveId);
  appendReceiptField(receipt, "product_save_session_saved",
                     window.productSaveSessionSaved);
  appendReceiptField(receipt, "active_product_save_id", window.activeProductSaveId);
  appendReceiptField(receipt, "product_save_load_status",
                     window.productSaveLoadStatus);
  appendReceiptField(receipt, "product_save_load_reason_code",
                     window.productSaveLoadReasonCode);
  appendReceiptField(receipt, "product_save_load_save_id",
                     window.productSaveLoadSaveId);
  appendReceiptField(receipt, "product_save_load_source",
                     window.productSaveLoadSource);
  appendReceiptField(receipt, "product_save_load_selected_id",
                     window.productSaveLoadSelectedId);
  appendReceiptField(receipt, "product_save_load_selected_enabled",
                     window.productSaveLoadSelectedEnabled);
  appendReceiptField(receipt, "selected_save_id", window.selectedProductSaveId);
  appendReceiptField(receipt, "selected_save_enabled",
                     window.selectedProductSaveEnabled);
  appendReceiptField(receipt, "selected_save_status",
                     window.selectedProductSaveStatus);
  appendReceiptField(receipt, "save_delete_confirmation_open",
                     window.saveDeleteConfirmationOpen);
  appendReceiptField(receipt, "save_delete_candidate_id",
                     window.saveDeleteCandidateId);
  appendReceiptField(receipt, "save_delete_candidate_enabled",
                     window.saveDeleteCandidateEnabled);
  appendReceiptField(receipt, "save_delete_status", window.saveDeleteStatus);
  appendReceiptField(receipt, "save_delete_reason_code",
                     window.saveDeleteReasonCode);
  appendReceiptField(receipt, "save_delete_type", window.saveDeleteType);
  appendReceiptField(receipt, "save_delete_recoverable",
                     window.saveDeleteRecoverable);
  appendReceiptField(receipt, "save_delete_executed", window.saveDeleteExecuted);
  appendReceiptField(receipt, "deleted_save_browser_open",
                     window.deletedSaveBrowserOpen);
  appendReceiptField(receipt, "deleted_save_count", window.deletedSaveCount);
  appendReceiptField(receipt, "deleted_compatible_save_count",
                     window.deletedCompatibleSaveCount);
  appendReceiptField(receipt, "deleted_selected_save_id",
                     window.deletedSelectedSaveId);
  appendReceiptField(receipt, "deleted_selected_save_enabled",
                     window.deletedSelectedSaveEnabled);
  appendReceiptField(receipt, "deleted_selected_save_status",
                     window.deletedSelectedSaveStatus);
  appendReceiptField(receipt, "save_recover_status", window.saveRecoverStatus);
  appendReceiptField(receipt, "save_recover_reason_code",
                     window.saveRecoverReasonCode);
  appendReceiptField(receipt, "save_recover_executed",
                     window.saveRecoverExecuted);
  appendReceiptField(receipt, "save_recover_save_id", window.saveRecoverSaveId);
  appendReceiptField(receipt, "save_recover_snapshot_recovered",
                     window.saveRecoverSnapshotRecovered);
  appendReceiptField(receipt, "save_recover_snapshot_missing",
                     window.saveRecoverSnapshotMissing);
  appendReceiptField(receipt, "product_save_load_previous_hash",
                     window.productSaveLoadPreviousHash);
  appendReceiptField(receipt, "product_save_load_loaded_hash",
                     window.productSaveLoadLoadedHash);
  appendReceiptField(receipt, "product_save_load_session_loaded",
                     window.productSaveLoadSessionLoaded);
  appendReceiptField(receipt, "runtime_session_created", window.runtimeSessionCreated);
  appendReceiptField(receipt, "gameplay_active", window.gameplayActive);
  appendReceiptField(receipt, "runtime_state_hash", window.runtimeStateHash);
  appendReceiptField(receipt, "gameplay_view_visible",
                     window.viewport.gameplayViewVisible);
  appendReceiptField(receipt, "scene_item_count", window.sceneItemCount);
  appendReceiptField(receipt, "debug_item_count", window.debugItemCount);
  appendReceiptField(receipt, "player_visible", window.playerVisible);
  appendReceiptField(receipt, "room_visible", window.roomVisible);
  appendReceiptField(receipt, "objective_visible", window.objectiveVisible);
  appendReceiptField(receipt, "renderer_mutated_runtime", window.rendererMutatedRuntime);
  appendReceiptField(receipt, "scripted_gameplay_smoke", window.scriptedGameplaySmoke);
  appendReceiptField(receipt, "gameplay_input_used", window.gameplayInputUsed);
  appendReceiptField(receipt, "gameplay_input_source", window.gameplayInputSource);
  appendReceiptField(receipt, "gameplay_command_submitted", window.gameplayCommandSubmitted);
  appendReceiptField(receipt, "gameplay_command_kind", window.gameplayCommandKind);
  appendReceiptField(receipt, "gameplay_command_status", window.gameplayCommandStatus);
  appendReceiptField(receipt, "gameplay_command_accepted", window.gameplayCommandAccepted);
  appendReceiptField(receipt, "gameplay_tick_advanced", window.gameplayTickAdvanced);
  appendReceiptField(receipt, "player_position_changed", window.playerPositionChanged);
  appendReceiptField(receipt, "target_discovered", window.targetDiscovered);
  appendReceiptField(receipt, "gameplay_reach_gate", window.gameplayReachGate);
  appendReceiptField(receipt, "gameplay_last_rejection", window.gameplayLastRejection);
  appendReceiptField(receipt, "interaction_executed", window.interactionExecuted);
  appendReceiptField(receipt, "attack_executed", window.attackExecuted);
  appendReceiptField(receipt, "product_transition_last_action",
                     window.productTransitionLastAction);
  appendReceiptField(receipt, "product_transition_status",
                     window.productTransitionStatus);
  appendReceiptField(receipt, "product_transition_returned_to_gameplay",
                     window.productTransitionReturnedToGameplay);
  appendReceiptField(receipt, "product_transition_returned_to_title",
                     window.productTransitionReturnedToTitle);
  appendReceiptField(receipt, "product_transition_session_preserved",
                     window.productTransitionSessionPreserved);
  appendReceiptField(receipt, "camera_controller", window.viewport.cameraController);
  appendReceiptField(receipt, "camera_mode", window.viewport.cameraMode);
  appendReceiptField(receipt, "camera_controller_active",
                     window.viewport.cameraControllerActive);
  appendReceiptField(receipt, "look_input_used", window.viewport.lookInputUsed);
  appendReceiptField(receipt, "camera_input_source", window.viewport.cameraInputSource);
  appendReceiptField(receipt, "camera_yaw_degrees",
                     floatReceiptValue(window.viewport.cameraYawDegrees));
  appendReceiptField(receipt, "camera_pitch_degrees",
                     floatReceiptValue(window.viewport.cameraPitchDegrees));
  appendReceiptField(receipt, "camera_heading_visible",
                     window.viewport.cameraHeadingVisible);
  appendReceiptField(receipt, "product_draw_item_count",
                     window.viewport.productDrawItemCount);
  appendReceiptField(receipt, "product_draw_grid_visible",
                     window.viewport.productDrawGridVisible);
  appendReceiptField(receipt, "product_draw_player_visible",
                     window.viewport.productDrawPlayerVisible);
  appendReceiptField(receipt, "product_draw_room_visible",
                     window.viewport.productDrawRoomVisible);
  appendReceiptField(receipt, "product_draw_objective_visible",
                     window.viewport.productDrawObjectiveVisible);
  appendReceiptField(receipt, "product_draw_target_indicator_visible",
                     window.viewport.productDrawTargetIndicatorVisible);
  appendReceiptField(receipt, "product_draw_debug_marker_count",
                     window.viewport.productDrawDebugMarkerCount);
  appendReceiptField(receipt, "product_view_projection",
                     window.viewport.productViewProjection);
  appendReceiptField(receipt, "product_view_yaw_applied",
                     window.viewport.productViewYawApplied);
  appendReceiptField(receipt, "product_view_pitch_applied",
                     window.viewport.productViewPitchApplied);
  appendReceiptField(receipt, "product_view_player_anchor_found",
                     window.viewport.productViewPlayerAnchorFound);
  appendReceiptField(receipt, "product_render_bridge_ready",
                     window.viewport.productRenderBridgeReady);
  appendReceiptField(receipt, "product_view_frame_ready",
                     window.viewport.productViewFrameReady);
  appendReceiptField(receipt, "product_view_frame_item_count",
                     window.viewport.productViewFrameItemCount);
  appendReceiptField(receipt, "product_view_frame_on_screen_item_count",
                     window.viewport.productViewFrameOnScreenItemCount);
  appendReceiptField(receipt, "product_view_frame_target_item_count",
                     window.viewport.productViewFrameTargetItemCount);
  appendReceiptField(receipt, "product_feedback_bridge_ready",
                     window.viewport.productFeedbackBridgeReady);
  appendReceiptField(receipt, "product_feedback_bridge_line_count",
                     window.viewport.productFeedbackBridgeLineCount);
  appendReceiptField(receipt, "product_feedback_visible", feedback.visible);
  appendReceiptField(receipt, "product_feedback_target_status",
                     feedback.targetStatus);
  appendReceiptField(receipt, "product_feedback_reach_status",
                     feedback.reachStatus);
  appendReceiptField(receipt, "product_feedback_command_kind",
                     feedback.commandKind);
  appendReceiptField(receipt, "product_feedback_command_status",
                     feedback.commandStatus);
  appendReceiptField(receipt, "product_feedback_rejection_reason",
                     feedback.rejectionReason);
  appendReceiptField(receipt, "product_feedback_attack_visible",
                     feedback.combatFeedbackVisible);
  appendReceiptField(receipt, "product_feedback_interaction_visible",
                     feedback.interactionFeedbackVisible);
  appendReceiptField(receipt, "input_owner", menuOwnerName(window.inputOwner));
  appendReceiptField(receipt, "input_action_last", inputActionName(window.lastInputAction));
  appendReceiptField(receipt, "input_action_accepted", window.lastInputAccepted);
  appendReceiptField(receipt, "gameplay_input_suppressed", window.gameplayInputSuppressed);
  appendReceiptField(receipt, "automation_control_requested",
                     window.automationControlRequested);
  appendReceiptField(receipt, "automation_control_loaded",
                     window.automationControlLoaded);
  appendReceiptField(receipt, "automation_control_path", window.automationControlPath);
  appendReceiptField(receipt, "automation_control_status",
                     window.automationControlStatus);
  appendReceiptField(receipt, "automation_control_scope", window.automationControlScope);
  appendReceiptField(receipt, "automation_control_line_count",
                     window.automationControlLineCount);
  appendReceiptField(receipt, "automation_control_applied_count",
                     window.automationControlAppliedCount);
  appendReceiptField(receipt, "automation_control_last_key",
                     window.automationControlLastKey);
  appendReceiptField(receipt, "automation_control_last_action",
                     window.automationControlLastAction);
  appendReceiptField(receipt, "automation_control_last_owner",
                     menuOwnerName(window.automationControlLastOwner));
  appendReceiptField(receipt, "automation_control_last_result",
                     window.automationControlLastResult);
  appendReceiptField(receipt, "event_poll_count", window.eventPollCount);
  appendReceiptField(receipt, "frames", static_cast<std::uint64_t>(options.frames));
  appendReceiptField(receipt, "frames_presented", window.framesPresented);
  appendReceiptField(receipt, "window_status", window.status);
  appendReceiptField(receipt, "save_root", saves.saveRoot.generic_string());
  appendReceiptField(receipt, "save_count",
                     static_cast<std::uint64_t>(saves.slots.slots.size()));
  appendReceiptField(receipt, "compatible_save_count", saves.slots.compatibleCount);
  appendReceiptField(receipt, "selected_package_id", world.packageId);
  appendReceiptField(receipt, "selected_scenario_id", world.scenarioId);
  appendReceiptField(receipt, "world_template_source", world.source);
  appendReceiptField(receipt, "dev_package_override", !options.devPackageOverride.empty());
  appendReceiptField(receipt, "normal_package_cli", false);
  const bool windowFailed = window.requested && !window.created;
  appendReceiptField(receipt, "result", windowFailed ? "skip" : "pass");
  appendReceiptField(receipt, "reason_code",
                     windowFailed ? "sdl3_unavailable"
                                  : (window.gameplayActive ? "product_gameplay_ready"
                                                           : "opening_menu_ready"));
  return receipt;
}

}  // namespace iggy3d
