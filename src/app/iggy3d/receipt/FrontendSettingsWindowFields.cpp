#include "app/iggy3d/receipt/ReceiptFields.hpp"

#include <charconv>
#include <string>
#include <string_view>
#include <utility>

#include "app/iggy3d/gameplay/GameplayFeedback.hpp"
#include "app/iggy3d/gameplay/MovementProof.hpp"
#include "app/iggy3d/debug/MovementDebugHud.hpp"
#include "app/iggy3d/debug/NpcBehaviorDebugHud.hpp"
#include "app/iggy3d/debug/PhysicsDebugHud.hpp"
#include "app/iggy3d/creative/ui/UiProjection.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"
#include "app/iggy3d/creative/bridge/UiCommandFrame.hpp"
#include "app/iggy3d/creative/bridge/UiInputFrame.hpp"
#include "app/iggy3d/creative/bridge/ViewportPickFrame.hpp"
#include "app/iggy3d/creative/bridge/WireframeFrame.hpp"
#include "app/iggy3d/window/RendererLifecycle.hpp"
#include "app/iggy3d/creative/render/WireframeDebugLines.hpp"

namespace iggy3d {

void appendProductFrontendSettingsWindowFields(RenderReceipt& receipt, const ProductAppOptions& options, const FrontendState& frontend, const FrontendSettings& settings, const ProductAppWindowState& window, ProductCreativeSurfaceKind creativeSurface, bool mapMakerLive) {
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
  appendReceiptField(receipt, "pause_menu_open",
                     frontendPauseMenuOpen(frontend));
  appendReceiptField(receipt, "dev_tools_open",
                     frontendDevToolsOpen(frontend));
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
  appendReceiptField(receipt, "settings_debug_overlay_enabled",
                     settings.debugOverlayEnabled);
  appendReceiptField(receipt,
                     "dev_collision_overlay_visible",
                     window.devCollisionOverlay.visible);
  appendReceiptField(receipt,
                     "dev_collision_overlay_status",
                     window.devCollisionOverlay.status);
  appendReceiptField(receipt,
                     "dev_collision_overlay_reason_code",
                     window.devCollisionOverlay.reasonCode);
  appendReceiptField(receipt,
                     "gameplay_movement_tuning_status",
                     window.gameplayMovement.tuningStatus);
  appendReceiptField(receipt,
                     "gameplay_movement_tuning_reason_code",
                     window.gameplayMovement.tuningReasonCode);
  appendReceiptField(receipt,
                     "gameplay_movement_tuning_visible",
                     window.gameplayMovement.tuningVisible);
  appendReceiptField(
      receipt,
      "gameplay_movement_tuning_selected_field",
      productGameplayMovementTuningFieldName(
          window.gameplayMovement.tuningSelectedField));
  for (const ProductGameplayMovementTuningFieldDescriptor& descriptor :
       kProductGameplayMovementTuningFields) {
    const std::string receiptKey =
        "gameplay_movement_tuning_" + std::string{descriptor.name};
    // branch-gate: BG-1208
    if (descriptor.kind == ProductGameplayMovementTuningFieldKind::Toggle) {
      appendReceiptField(
          receipt,
          receiptKey,
          productGameplayMovementTuningFieldValue(
              window.gameplayMovement.tuning, descriptor.field) >= 0.5F);
    } else {
      appendReceiptField(
          receipt,
          receiptKey,
          floatReceiptValue(productGameplayMovementTuningFieldValue(
              window.gameplayMovement.tuning, descriptor.field)));
    }
  }
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
  appendReceiptField(receipt, "interaction_mode",
                     productInteractionModeName(window.interactionMode));
  appendReceiptField(receipt, "creative_surface_kind",
                     productCreativeSurfaceKindName(creativeSurface));
  appendReceiptField(receipt, "map_maker_active", mapMakerLive);
  appendReceiptField(receipt, "map_maker_status", window.mapMakerStatus);
  appendReceiptField(receipt, "map_maker_reason_code",
                     window.mapMakerReasonCode);
  appendReceiptField(receipt, "map_maker_grid_visible",
                     window.mapMakerGridVisible);
  appendReceiptField(receipt, "map_maker_grid_status",
                     window.mapMakerGridStatus);
  appendReceiptField(receipt, "map_maker_grid_reason_code",
                     window.mapMakerGridReasonCode);
  appendReceiptField(receipt, "map_maker_grid_pitch_meters",
                     floatReceiptValue(window.mapMakerGridPitchMeters));
  appendReceiptField(receipt, "map_maker_grid_major_step_meters",
                     floatReceiptValue(window.mapMakerGridMajorStepMeters));
  appendReceiptField(receipt, "map_maker_grid_plane_y",
                     floatReceiptValue(window.mapMakerGridPlaneY));
  appendReceiptField(receipt, "map_maker_grid_layer_count",
                     window.mapMakerGridLayerCount);
  appendReceiptField(receipt, "map_maker_grid_dot_count",
                     window.mapMakerGridDotCount);
  appendReceiptField(receipt, "map_maker_grid_major_dot_count",
                     window.mapMakerGridMajorDotCount);
  appendReceiptField(receipt, "interaction_mode_hud_visible",
                     window.interactionModeHud.visible);
  appendReceiptField(receipt, "interaction_mode_hud_status",
                     window.interactionModeHud.status);
  appendReceiptField(receipt, "interaction_mode_hud_reason_code",
                     window.interactionModeHud.reasonCode);
  appendReceiptField(receipt, "interaction_mode_hud_mode",
                     window.interactionModeHud.mode);
  appendReceiptField(receipt, "interaction_mode_hud_label",
                     window.interactionModeHud.label);
  appendReceiptField(receipt, "top_down_map_visible", window.topDownMap.visible);
  appendReceiptField(receipt, "top_down_map_purpose", window.topDownMap.purpose);
  appendReceiptField(receipt, "top_down_map_size", window.topDownMap.size);
  appendReceiptField(receipt, "top_down_map_status", window.topDownMap.status);
  appendReceiptField(receipt, "top_down_map_reason_code",
                     window.topDownMap.reasonCode);
  appendReceiptField(receipt, "top_down_map_item_count",
                     window.topDownMap.itemCount);
  appendReceiptField(receipt, "mouse_capture_requested",
                     window.mouseCapture.requested);
  appendReceiptField(receipt, "mouse_capture_active", window.mouseCapture.active);
  appendReceiptField(receipt, "mouse_capture_status", window.mouseCapture.status);
  appendReceiptField(receipt, "mouse_capture_reason_code",
                     window.mouseCapture.reasonCode);
  appendReceiptField(receipt, "mouse_capture_mode", window.mouseCapture.mode);
  appendReceiptField(receipt, "mouse_capture_input_owner",
                     window.mouseCapture.inputOwner);
  appendReceiptField(receipt,
                     "controller_mode_toggle_requested",
                     window.controllerModeToggle.requested);
  appendReceiptField(receipt,
                     "controller_mode_toggle_accepted",
                     window.controllerModeToggle.accepted);
  appendReceiptField(receipt,
                     "controller_mode_toggle_status",
                     window.controllerModeToggle.status);
  appendReceiptField(receipt,
                     "controller_mode_toggle_reason_code",
                     window.controllerModeToggle.reasonCode);
  appendReceiptField(receipt,
                     "controller_mode_toggle_surface",
                     window.controllerModeToggle.surface);
  appendReceiptField(receipt, "controller_action_mapped",
                     window.controllerAction.mapped);
  appendReceiptField(receipt, "controller_action_status",
                     window.controllerAction.status);
  appendReceiptField(receipt, "controller_action_reason_code",
                     window.controllerAction.reasonCode);
  appendReceiptField(receipt, "controller_action_control",
                     window.controllerAction.control);
  appendReceiptField(receipt, "controller_action_mode",
                     window.controllerAction.mode);
  appendReceiptField(receipt, "controller_action_surface",
                     window.controllerAction.surface);
  appendReceiptField(receipt, "controller_action_input_action",
                     window.controllerAction.inputAction);
}

}  // namespace iggy3d
