#include "app/iggy3d/receipt/ReceiptFields.hpp"

#include <array>
#include <charconv>
#include <string>
#include <string_view>
#include <utility>

#include "app/iggy3d/ProductAppWindowState.hpp"
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

namespace {

struct FrontendSettingsWindowReceiptContext {
  const ProductAppOptions& options;
  const FrontendState& frontend;
  const FrontendSettings& settings;
  const ProductAppWindowState& window;
  ProductCreativeSurfaceKind creativeSurface;
  bool mapMakerLive;
};

struct FrontendSettingsWindowReceiptFieldRow {
  std::string_view key;
  void (*append)(RenderReceipt& receipt,
                 const FrontendSettingsWindowReceiptContext& context,
                 std::string_view key);
};

const std::array<FrontendSettingsWindowReceiptFieldRow, 33>
    kFrontendSettingsWindowPreludeReceiptFields{{
    {"app",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext&,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           "iggy3d");
     }},
    {"app_surface",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext&,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           "product");
     }},
    {"opening_menu",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.frontend.screen == FrontendScreen::Starter);
     }},
    {"frontend_screen",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           frontendScreenName(context.frontend.screen));
     }},
    {"frontend_child_screen",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           frontendScreenName(context.frontend.childScreen));
     }},
    {"frontend_selected_action",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           frontendActionName(context.frontend.selectedAction));
     }},
    {"frontend_status",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.frontend.status);
     }},
    {"frontend_launch_requested",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.frontend.launchRequested);
     }},
    {"frontend_return_to_title_requested",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.frontend.returnToTitleRequested);
     }},
    {"pause_menu_open",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           frontendPauseMenuOpen(context.frontend));
     }},
    {"dev_tools_open",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           frontendDevToolsOpen(context.frontend));
     }},
    {"starter_world_suppressed",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           !context.window.gameplay.gameplayActive);
     }},
    {"auto_new_world",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.options.autoNewWorld);
     }},
    {"renderer_request",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           productRendererRequestName(context.options.renderer));
     }},
    {"window_mode",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           productWindowModeName(context.options.windowMode));
     }},
    {"input_backend",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           productInputBackendName(context.options.inputBackend));
     }},
    {"settings_input_backend",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           frontendInputBackendName(context.settings.inputBackend));
     }},
    {"settings_selected_tab",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           frontendSettingsTabName(context.window.frontendShell.selectedSettingsTab));
     }},
    {"settings_renderer",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           frontendRendererChoiceName(context.settings.renderer));
     }},
    {"settings_window_mode",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           frontendWindowModeName(context.settings.windowMode));
     }},
    {"settings_camera_mode",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           frontendCameraModeName(context.settings.cameraMode));
     }},
    {"settings_look_sensitivity",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           floatReceiptValue(context.settings.lookSensitivity));
     }},
    {"settings_controller_look_sensitivity",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           floatReceiptValue(context.settings.controllerLookSensitivity));
     }},
    {"settings_invert_look",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.settings.invertLook);
     }},
    {"settings_developer_tools_enabled",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.settings.devToolsEnabled);
     }},
    {"settings_debug_overlay_enabled",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.settings.debugOverlayEnabled);
     }},
    {"dev_collision_overlay_visible",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.debugHud.devCollisionOverlay.visible);
     }},
    {"dev_collision_overlay_status",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.debugHud.devCollisionOverlay.status);
     }},
    {"dev_collision_overlay_reason_code",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.debugHud.devCollisionOverlay.reasonCode);
     }},
    {"gameplay_movement_tuning_status",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.gameplay.gameplayMovement.tuningStatus);
     }},
    {"gameplay_movement_tuning_reason_code",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.gameplay.gameplayMovement.tuningReasonCode);
     }},
    {"gameplay_movement_tuning_visible",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.gameplay.gameplayMovement.tuningVisible);
     }},
    {"gameplay_movement_tuning_selected_field",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           productGameplayMovementTuningFieldName(
                     context.window.gameplay.gameplayMovement.tuningSelectedField));
     }},
}};

const std::array<FrontendSettingsWindowReceiptFieldRow, 57>
    kFrontendSettingsWindowPostTuningReceiptFields{{
    {"window_requested",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.requested);
     }},
    {"window_shell",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.sdlAvailable ? "sdl3" : "unavailable");
     }},
    {"window_created",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.created);
     }},
    {"window_drawable",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.drawable);
     }},
    {"window_title",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.gameplay.gameplayActive ? "iggy3d - Gameplay" : "iggy3d - Opening Menu");
     }},
    {"opening_menu_visible",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.frontendShell.openingMenuVisible);
     }},
    {"menu_text_drawn",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.frontendShell.menuTextDrawn);
     }},
    {"menu_selected_row_drawn",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.frontendShell.selectedRowDrawn);
     }},
    {"menu_row_count",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.frontendShell.menuRowCount);
     }},
    {"mouse_menu_select_used",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.frontendShell.mouseMenuSelectUsed);
     }},
    {"gamepad_available",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.inputDevice.gamepadAvailable);
     }},
    {"gamepad_name",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.inputDevice.gamepadName);
     }},
    {"gamepad_mapping",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.inputDevice.gamepadMapping);
     }},
    {"gamepad_menu_select_used",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.frontendShell.gamepadMenuSelectUsed);
     }},
    {"interaction_mode",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           productInteractionModeName(
                                    context.window.inputDevice.interactionMode));
     }},
    {"creative_surface_kind",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           productCreativeSurfaceKindName(context.creativeSurface));
     }},
    {"map_maker_active",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.mapMakerLive);
     }},
    {"map_maker_status",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.viewport.mapMakerStatus);
     }},
    {"map_maker_reason_code",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.viewport.mapMakerReasonCode);
     }},
    {"map_maker_grid_visible",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.viewport.mapMakerGridVisible);
     }},
    {"map_maker_grid_status",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.viewport.mapMakerGridStatus);
     }},
    {"map_maker_grid_reason_code",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.viewport.mapMakerGridReasonCode);
     }},
    {"map_maker_grid_pitch_meters",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           floatReceiptValue(context.window.viewport.mapMakerGridPitchMeters));
     }},
    {"map_maker_grid_major_step_meters",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           floatReceiptValue(context.window.viewport.mapMakerGridMajorStepMeters));
     }},
    {"map_maker_grid_plane_y",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           floatReceiptValue(context.window.viewport.mapMakerGridPlaneY));
     }},
    {"map_maker_grid_layer_count",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.viewport.mapMakerGridLayerCount);
     }},
    {"map_maker_grid_dot_count",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.viewport.mapMakerGridDotCount);
     }},
    {"map_maker_grid_major_dot_count",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.viewport.mapMakerGridMajorDotCount);
     }},
    {"interaction_mode_hud_visible",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.inputDevice.interactionModeHud.visible);
     }},
    {"interaction_mode_hud_status",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.inputDevice.interactionModeHud.status);
     }},
    {"interaction_mode_hud_reason_code",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.inputDevice.interactionModeHud.reasonCode);
     }},
    {"interaction_mode_hud_mode",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.inputDevice.interactionModeHud.mode);
     }},
    {"interaction_mode_hud_label",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.inputDevice.interactionModeHud.label);
     }},
    {"top_down_map_visible",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.debugHud.topDownMap.visible);
     }},
    {"top_down_map_purpose",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.debugHud.topDownMap.purpose);
     }},
    {"top_down_map_size",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.debugHud.topDownMap.size);
     }},
    {"top_down_map_status",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.debugHud.topDownMap.status);
     }},
    {"top_down_map_reason_code",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.debugHud.topDownMap.reasonCode);
     }},
    {"top_down_map_item_count",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.debugHud.topDownMap.itemCount);
     }},
    {"mouse_capture_requested",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.inputDevice.mouseCapture.requested);
     }},
    {"mouse_capture_active",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.inputDevice.mouseCapture.active);
     }},
    {"mouse_capture_status",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.inputDevice.mouseCapture.status);
     }},
    {"mouse_capture_reason_code",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.inputDevice.mouseCapture.reasonCode);
     }},
    {"mouse_capture_mode",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.inputDevice.mouseCapture.mode);
     }},
    {"mouse_capture_input_owner",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.inputDevice.mouseCapture.inputOwner);
     }},
    {"controller_mode_toggle_requested",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.inputDevice.controllerModeToggle.requested);
     }},
    {"controller_mode_toggle_accepted",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.inputDevice.controllerModeToggle.accepted);
     }},
    {"controller_mode_toggle_status",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.inputDevice.controllerModeToggle.status);
     }},
    {"controller_mode_toggle_reason_code",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.inputDevice.controllerModeToggle.reasonCode);
     }},
    {"controller_mode_toggle_surface",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.inputDevice.controllerModeToggle.surface);
     }},
    {"controller_action_mapped",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.inputDevice.controllerAction.mapped);
     }},
    {"controller_action_status",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.inputDevice.controllerAction.status);
     }},
    {"controller_action_reason_code",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.inputDevice.controllerAction.reasonCode);
     }},
    {"controller_action_control",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.inputDevice.controllerAction.control);
     }},
    {"controller_action_mode",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.inputDevice.controllerAction.mode);
     }},
    {"controller_action_surface",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.inputDevice.controllerAction.surface);
     }},
    {"controller_action_input_action",
     [](RenderReceipt& receipt,
        const FrontendSettingsWindowReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.inputDevice.controllerAction.inputAction);
     }},
}};

}  // namespace

void appendProductFrontendSettingsWindowFields(
    RenderReceipt& receipt,
    const ProductAppOptions& options,
    const FrontendState& frontend,
    const FrontendSettings& settings,
    const ProductAppWindowState& window,
    ProductCreativeSurfaceKind creativeSurface,
    bool mapMakerLive) {
  const FrontendSettingsWindowReceiptContext context{
      options,
      frontend,
      settings,
      window,
      creativeSurface,
      mapMakerLive,
  };

  for (const FrontendSettingsWindowReceiptFieldRow& row :
       kFrontendSettingsWindowPreludeReceiptFields) {
    row.append(receipt, context, row.key);
  }

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
              window.gameplay.gameplayMovement.tuning, descriptor.field) >= 0.5F);
    } else {
      appendReceiptField(
          receipt,
          receiptKey,
          floatReceiptValue(productGameplayMovementTuningFieldValue(
              window.gameplay.gameplayMovement.tuning, descriptor.field)));
    }
  }

  for (const FrontendSettingsWindowReceiptFieldRow& row :
       kFrontendSettingsWindowPostTuningReceiptFields) {
    row.append(receipt, context, row.key);
  }
}

}  // namespace iggy3d
