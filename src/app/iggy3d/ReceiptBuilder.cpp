#include "app/iggy3d/ReceiptBuilder.hpp"

#include <charconv>
#include <string>
#include <utility>

#include "app/iggy3d/gameplay/GameplayFeedback.hpp"
#include "app/iggy3d/gameplay/MovementProof.hpp"
#include "app/iggy3d/debug/MovementDebugHud.hpp"
#include "app/iggy3d/debug/NpcBehaviorDebugHud.hpp"
#include "app/iggy3d/debug/PhysicsDebugHud.hpp"
#include "app/iggy3d/menu/CreativeUiProjection.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"
#include "app/iggy3d/window/CreativeUiInputFrame.hpp"
#include "app/iggy3d/window/RendererLifecycle.hpp"

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

void setPhysicsMovementPlannerProof(ProductAppWindowState& window,
                                    std::string status,
                                    bool requested,
                                    bool used) {
  window.physicsMovementPlannerRequested = requested;
  window.physicsMovementPlannerUsed = used;
  window.physicsMovementPlannerStatus = std::move(status);
  window.physicsMovementPlannerReasonCode = window.physicsMovementPlannerStatus;
}

std::string_view productUiThemeReceiptName(ProductUiThemeId theme) noexcept {
  switch (theme) {
    case ProductUiThemeId::System:
      return "system";
    case ProductUiThemeId::Journal:
      return "journal";
  }
  return "unknown";
}

std::string_view productUiHitSurfaceReceiptName(
    ProductUiHitSurface surface) noexcept {
  switch (surface) {
    case ProductUiHitSurface::None:
      return "none";
    case ProductUiHitSurface::StarterMenu:
      return "starter_menu";
    case ProductUiHitSurface::PauseMenu:
      return "pause_menu";
    case ProductUiHitSurface::CreativeOverlay:
      return "creative_overlay";
    case ProductUiHitSurface::Notebook:
      return "notebook";
  }
  return "unknown";
}

std::string_view uiHitKindReceiptName(UiHitKind kind) noexcept {
  switch (kind) {
    case UiHitKind::None:
      return "none";
    case UiHitKind::Button:
      return "button";
    case UiHitKind::Row:
      return "row";
    case UiHitKind::Slider:
      return "slider";
    case UiHitKind::Toggle:
      return "toggle";
    case UiHitKind::Viewport:
      return "viewport";
  }
  return "unknown";
}

}  // namespace

void recordProductPhysicsMovementPlannerTickProof(
    ProductAppWindowState& window,
    bool requested,
    bool collisionSurfacesAvailable,
    bool movementPhysicsStatsAvailable) {
  // branch-gate: BG-1114
  if (!requested) {
    setPhysicsMovementPlannerProof(
        window, "physics_movement_planner_disabled", false, false);
    return;
  }
  // branch-gate: BG-1114
  if (!collisionSurfacesAvailable) {
    setPhysicsMovementPlannerProof(
        window, "physics_movement_planner_no_collision_surfaces", true, false);
    return;
  }
  // branch-gate: BG-1114
  if (movementPhysicsStatsAvailable) {
    setPhysicsMovementPlannerProof(
        window, "physics_movement_planner_used", true, true);
    return;
  }
  setPhysicsMovementPlannerProof(
      window, "physics_movement_planner_not_used", true, false);
}

void recordProductCreativeUiProjection(
    ProductAppWindowState& window,
    const ProductCreativeUiProjectionReceipt& receipt) {
  window.creativeUiProjectionRequested = receipt.requested;
  window.creativeUiProjectionReady = receipt.ready;
  window.creativeUiProjectionPartial = receipt.partial;
  window.creativeUiProjectionStatus = std::string(receipt.status);
  window.creativeUiProjectionReasonCode = std::string(receipt.reasonCode);
  window.creativeUiProjectionUsedModel = receipt.usedModel;
  window.creativeUiProjectionUsedFacade = receipt.usedFacade;
  window.creativeUiProjectionVirtualWidth = receipt.virtualWidth;
  window.creativeUiProjectionVirtualHeight = receipt.virtualHeight;
  window.creativeUiProjectionTheme =
      std::string(productUiThemeReceiptName(receipt.theme));
  window.creativeUiProjectionPanelCount = receipt.panelCount;
  window.creativeUiProjectionModelRowCount = receipt.modelRowCount;
  window.creativeUiProjectionPrimitiveCount = receipt.primitiveCount;
  window.creativeUiProjectionTextCount = receipt.textCount;
  window.creativeUiProjectionRectCount = receipt.rectCount;
  window.creativeUiProjectionRowCount = receipt.rowCount;
  window.creativeUiProjectionDisabledRowCount = receipt.disabledRowCount;
  window.creativeUiProjectionHitRegionCount = receipt.hitRegionCount;
}

void recordProductCreativeUiInputFrame(
    ProductAppWindowState& window,
    const ProductCreativeUiInputFrameReceipt& receipt) {
  window.creativeUiInputRequested = receipt.requested;
  window.creativeUiInputClickPresent = receipt.clickPresent;
  window.creativeUiInputDrawListAvailable = receipt.drawListAvailable;
  window.creativeUiInputRouted = receipt.routed;
  window.creativeUiInputHit = receipt.hit;
  window.creativeUiInputConsumed = receipt.consumed;
  window.creativeUiInputEnabled = receipt.enabled;
  window.creativeUiInputSurface =
      std::string(productUiHitSurfaceReceiptName(receipt.surface));
  window.creativeUiInputKind = std::string(uiHitKindReceiptName(receipt.kind));
  window.creativeUiInputAction = std::string(frontendActionName(receipt.action));
  window.creativeUiInputLayerIndex =
      static_cast<std::uint64_t>(receipt.layerIndex);
  window.creativeUiInputRegionIndex =
      static_cast<std::uint64_t>(receipt.regionIndex);
  window.creativeUiInputSemanticId =
      receipt.semanticId.empty() ? "none" : receipt.semanticId;
  window.creativeUiInputStatus = receipt.status;
  window.creativeUiInputReasonCode = receipt.reasonCode;
}

RenderReceipt buildProductAppReceipt(const ProductAppOptions& options,
                                     const ProductWorldTemplate& world,
                                     const FrontendState& frontend,
                                     const FrontendSettings& settings,
                                     const ProductAppWindowState& window,
                                     const ProductSaveBridgeResult& saves) {
  RenderReceipt receipt;
  const ProductActiveSurfaceFrame activeSurface = resolveProductActiveSurface(
      productActiveSurfaceContextForWindow(frontend, window));
  const bool mapMakerLive = productMapMakerLiveForWindow(frontend, window);
  const GameplayFeedback feedback = buildGameplayFeedback(window);
  const ProductMovementProofPacket movementProof =
      buildProductMovementProofPacket(window);
  const ProductVulkanGameplayReadiness vulkanGameplayReadiness =
      evaluateProductVulkanGameplayReadiness(window);
  const MovementDebugHud movementHud =
      buildMovementDebugHud(movementProof,
                            window.gameplayActive,
                            settings.devToolsEnabled,
                            settings.debugOverlayEnabled);
  const NpcBehaviorDebugHud npcBehaviorHud{
      window.npcBehaviorDebugHudVisible,
      settings.devToolsEnabled,
      settings.debugOverlayEnabled,
      window.npcBehaviorDebugHudDebugAvailable,
      static_cast<std::size_t>(window.npcBehaviorDebugHudLineCount),
      window.npcBehaviorDebugHudStatus,
      window.npcBehaviorDebugHudReasonCode,
      {}};
  const PhysicsDebugHud physicsHud{
      window.physicsDebugHud.visible,
      settings.devToolsEnabled,
      settings.debugOverlayEnabled,
      window.physicsDebugHud.debugAvailable,
      window.physicsDebugHud.lineCount,
      window.physicsDebugHud.status,
      window.physicsDebugHud.reasonCode,
      window.physicsDebugHud.hasWarnings,
      {}};
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
                     window.devCollisionOverlayVisible);
  appendReceiptField(receipt,
                     "dev_collision_overlay_status",
                     window.devCollisionOverlayStatus);
  appendReceiptField(receipt,
                     "dev_collision_overlay_reason_code",
                     window.devCollisionOverlayReasonCode);
  appendReceiptField(receipt,
                     "gameplay_movement_tuning_status",
                     window.gameplayMovementTuningStatus);
  appendReceiptField(receipt,
                     "gameplay_movement_tuning_reason_code",
                     window.gameplayMovementTuningReasonCode);
  appendReceiptField(receipt,
                     "gameplay_movement_tuning_visible",
                     window.gameplayMovementTuningVisible);
  appendReceiptField(
      receipt,
      "gameplay_movement_tuning_selected_field",
      productGameplayMovementTuningFieldName(
          window.gameplayMovementTuningSelectedField));
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
              window.gameplayMovementTuning, descriptor.field) >= 0.5F);
    } else {
      appendReceiptField(
          receipt,
          receiptKey,
          floatReceiptValue(productGameplayMovementTuningFieldValue(
              window.gameplayMovementTuning, descriptor.field)));
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
  appendReceiptField(receipt, "top_down_map_visible", window.topDownMapVisible);
  appendReceiptField(receipt, "top_down_map_purpose", window.topDownMapPurpose);
  appendReceiptField(receipt, "top_down_map_size", window.topDownMapSize);
  appendReceiptField(receipt, "top_down_map_status", window.topDownMapStatus);
  appendReceiptField(receipt, "top_down_map_reason_code",
                     window.topDownMapReasonCode);
  appendReceiptField(receipt, "top_down_map_item_count",
                     window.topDownMapItemCount);
  appendReceiptField(receipt, "mouse_capture_requested",
                     window.mouseCaptureRequested);
  appendReceiptField(receipt, "mouse_capture_active", window.mouseCaptureActive);
  appendReceiptField(receipt, "mouse_capture_status", window.mouseCaptureStatus);
  appendReceiptField(receipt, "mouse_capture_reason_code",
                     window.mouseCaptureReasonCode);
  appendReceiptField(receipt, "mouse_capture_mode", window.mouseCaptureMode);
  appendReceiptField(receipt, "mouse_capture_input_owner",
                     window.mouseCaptureInputOwner);
  appendReceiptField(receipt,
                     "controller_mode_toggle_requested",
                     window.controllerModeToggleRequested);
  appendReceiptField(receipt,
                     "controller_mode_toggle_accepted",
                     window.controllerModeToggleAccepted);
  appendReceiptField(receipt,
                     "controller_mode_toggle_status",
                     window.controllerModeToggleStatus);
  appendReceiptField(receipt,
                     "controller_mode_toggle_reason_code",
                     window.controllerModeToggleReasonCode);
  appendReceiptField(receipt,
                     "controller_mode_toggle_surface",
                     window.controllerModeToggleSurface);
  appendReceiptField(receipt, "controller_action_mapped",
                     window.controllerActionMapped);
  appendReceiptField(receipt, "controller_action_status",
                     window.controllerActionStatus);
  appendReceiptField(receipt, "controller_action_reason_code",
                     window.controllerActionReasonCode);
  appendReceiptField(receipt, "controller_action_control",
                     window.controllerActionControl);
  appendReceiptField(receipt, "controller_action_mode",
                     window.controllerActionMode);
  appendReceiptField(receipt, "controller_action_surface",
                     window.controllerActionSurface);
  appendReceiptField(receipt, "controller_action_input_action",
                     window.controllerActionInputAction);
  appendReceiptField(receipt, "dev_tools_category",
                     frontendDevToolsCategoryName(frontend.devToolsCategory));
  appendReceiptField(receipt, "launch_action", window.launchAction);
  appendReceiptField(receipt, "launch_status", window.launchStatus);
  appendReceiptField(receipt, "package_load_status", window.packageLoadStatus);
  appendReceiptField(receipt, "world_setup_title", window.worldSetupTitle);
  appendReceiptField(receipt, "world_setup_status", window.worldSetupStatus);
  appendReceiptField(receipt,
                     "world_setup_dungeon_title",
                     window.worldSetupDungeonTitle);
  appendReceiptField(receipt,
                     "world_setup_dungeon_index",
                     window.worldSetupDungeonIndex);
  appendReceiptField(receipt,
                     "world_setup_dungeon_count",
                     window.worldSetupDungeonCount);
  appendReceiptField(receipt,
                     "world_setup_ascii_room_enabled",
                     window.worldSetupAsciiRoomEnabled);
  appendReceiptField(receipt,
                     "world_setup_ascii_room_text_present",
                     window.worldSetupAsciiRoomTextPresent);
  appendReceiptField(receipt,
                     "world_setup_ascii_room_id",
                     window.worldSetupAsciiRoomId);
  appendReceiptField(receipt,
                     "world_setup_ascii_room_source_name",
                     window.worldSetupAsciiRoomSourceName);
  appendReceiptField(receipt,
                     "world_setup_dungeon_draft_edit_mode",
                     window.worldSetupDungeonDraftEditMode);
  appendReceiptField(receipt,
                     "world_setup_dungeon_draft_modified",
                     window.worldSetupDungeonDraftModified);
  appendReceiptField(receipt,
                     "world_setup_dungeon_draft_cursor_row",
                     window.worldSetupDungeonDraftCursorRow);
  appendReceiptField(receipt,
                     "world_setup_dungeon_draft_cursor_column",
                     window.worldSetupDungeonDraftCursorColumn);
  appendReceiptField(receipt,
                     "world_setup_dungeon_draft_status",
                     window.worldSetupDungeonDraftStatus);
  appendReceiptField(receipt,
                     "world_setup_dungeon_draft_reason_code",
                     window.worldSetupDungeonDraftReasonCode);
  appendReceiptField(receipt,
                     "world_setup_dungeon_draft_selected_glyph",
                     window.worldSetupDungeonDraftSelectedGlyph);
  appendReceiptField(receipt,
                     "world_setup_dungeon_draft_last_glyph",
                     window.worldSetupDungeonDraftLastGlyph);
  appendReceiptField(receipt, "world_creation_status", window.worldCreationStatus);
  appendReceiptField(receipt, "world_creation_reason_code",
                     window.worldCreationReasonCode);
  appendReceiptField(receipt, "world_creation_world_id", window.worldCreationWorldId);
  appendReceiptField(receipt, "world_creation_world_title",
                     window.worldCreationWorldTitle);
  appendReceiptField(receipt,
                     "world_creation_ascii_room_requested",
                     window.worldCreationAsciiRoomRequested);
  appendReceiptField(receipt,
                     "world_creation_ascii_room_id",
                     window.worldCreationAsciiRoomId);
  appendReceiptField(receipt,
                     "world_creation_ascii_room_source_name",
                     window.worldCreationAsciiRoomSourceName);
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
  appendReceiptField(receipt, "ascii_room_preview_status",
                     window.asciiRoomPreviewStatus);
  appendReceiptField(receipt, "ascii_room_preview_reason_code",
                     window.asciiRoomPreviewReasonCode);
  appendReceiptField(receipt, "ascii_room_preview_failed_stage",
                     window.asciiRoomPreviewFailedStage);
  appendReceiptField(receipt, "ascii_room_preview_room_id",
                     window.asciiRoomPreviewRoomId);
  appendReceiptField(receipt, "ascii_room_preview_source_name",
                     window.asciiRoomPreviewSourceName);
  appendReceiptField(receipt, "ascii_room_preview_ready",
                     window.asciiRoomPreviewReady);
  appendReceiptField(receipt, "ascii_room_preview_width",
                     window.asciiRoomPreviewWidth);
  appendReceiptField(receipt, "ascii_room_preview_height",
                     window.asciiRoomPreviewHeight);
  appendReceiptField(receipt, "ascii_room_preview_floor_count",
                     window.asciiRoomPreviewFloorCount);
  appendReceiptField(receipt, "ascii_room_preview_wall_count",
                     window.asciiRoomPreviewWallCount);
  appendReceiptField(receipt, "ascii_room_preview_object_count",
                     window.asciiRoomPreviewObjectCount);
  appendReceiptField(receipt, "ascii_room_preview_marker_count",
                     window.asciiRoomPreviewMarkerCount);
  appendReceiptField(receipt, "ascii_room_preview_elevated_floor_count",
                     window.asciiRoomPreviewElevatedFloorCount);
  appendReceiptField(receipt, "ascii_room_preview_ramp_count",
                     window.asciiRoomPreviewRampCount);
  appendReceiptField(receipt, "ascii_room_preview_blocked_slope_count",
                     window.asciiRoomPreviewBlockedSlopeCount);
  appendReceiptField(receipt, "ascii_room_preview_static_mesh_count",
                     window.asciiRoomPreviewStaticMeshCount);
  appendReceiptField(receipt, "ascii_room_preview_anchor_count",
                     window.asciiRoomPreviewAnchorCount);
  appendReceiptField(receipt, "ascii_room_preview_spatial_surface_count",
                     window.asciiRoomPreviewSpatialSurfaceCount);
  appendReceiptField(receipt, "ascii_room_preview_asset_text_written",
                     window.asciiRoomPreviewAssetTextWritten);
  appendReceiptField(receipt, "ascii_room_preview_asset_text_bytes",
                     window.asciiRoomPreviewAssetTextBytes);
  appendReceiptField(receipt, "ascii_room_activation_status",
                     window.asciiRoomActivationStatus);
  appendReceiptField(receipt, "ascii_room_activation_reason_code",
                     window.asciiRoomActivationReasonCode);
  appendReceiptField(receipt, "ascii_room_activation_room_id",
                     window.asciiRoomActivationRoomId);
  appendReceiptField(receipt, "ascii_room_activation_package_id",
                     window.asciiRoomActivationPackageId);
  appendReceiptField(receipt, "ascii_room_activation_scenario_id",
                     window.asciiRoomActivationScenarioId);
  appendReceiptField(receipt, "ascii_room_activation_session_created",
                     window.asciiRoomActivationSessionCreated);
  appendReceiptField(receipt, "ascii_room_activation_player_spawned",
                     window.asciiRoomActivationPlayerSpawned);
  appendReceiptField(receipt, "ascii_room_activation_player_count",
                     window.asciiRoomActivationPlayerCount);
  appendReceiptField(receipt, "ascii_room_activation_entity_count",
                     window.asciiRoomActivationEntityCount);
  appendReceiptField(receipt, "ascii_room_activation_npc_count",
                     window.asciiRoomActivationNpcCount);
  appendReceiptField(receipt, "ascii_room_activation_pickup_count",
                     window.asciiRoomActivationPickupCount);
  appendReceiptField(receipt, "ascii_room_activation_door_count",
                     window.asciiRoomActivationDoorCount);
  appendReceiptField(receipt, "ascii_room_activation_marker_entity_count",
                     window.asciiRoomActivationMarkerEntityCount);
  appendReceiptField(receipt, "ascii_room_activation_objective_count",
                     window.asciiRoomActivationObjectiveCount);
  appendReceiptField(receipt, "ascii_room_activation_wall_count",
                     window.asciiRoomActivationWallCount);
  appendReceiptField(receipt, "ascii_room_activation_marker_count",
                     window.asciiRoomActivationMarkerCount);
  appendReceiptField(receipt, "ascii_room_activation_runtime_hash",
                     window.asciiRoomActivationRuntimeHash);
  appendReceiptField(receipt, "room_editing_ready", window.roomEditing.ready);
  appendReceiptField(receipt, "room_editing_status", window.roomEditing.status);
  appendReceiptField(receipt, "room_editing_reason_code",
                     window.roomEditing.reasonCode);
  appendReceiptField(receipt, "room_editing_floor_count",
                     window.roomEditing.documentFloorCount);
  appendReceiptField(receipt, "room_editing_wall_count",
                     window.roomEditing.documentWallCount);
  appendReceiptField(receipt, "room_editing_object_count",
                     window.roomEditing.documentObjectCount);
  appendReceiptField(receipt, "room_editing_active_room_loaded",
                     window.roomEditing.activeRoom.loaded);
  appendReceiptField(receipt, "room_editing_active_room_static_mesh_count",
                     window.roomEditing.activeRoomStaticMeshCount);
  appendReceiptField(receipt, "room_editing_active_room_spatial_surface_count",
                     window.roomEditing.activeRoomSpatialSurfaceCount);
  appendReceiptField(receipt, "room_editing_authored_floor_count",
                     window.roomEditing.activeRoomAuthoredFloorCount);
  appendReceiptField(receipt, "room_editing_authored_wall_count",
                     window.roomEditing.activeRoomAuthoredWallCount);
  appendReceiptField(receipt, "room_editing_authored_object_count",
                     window.roomEditing.activeRoomAuthoredObjectCount);
  appendReceiptField(receipt, "room_editing_collision_ready",
                     window.roomEditing.activeRoomCollision.ready);
  appendReceiptField(receipt, "room_editing_collision_surface_count",
                     window.roomEditing.collisionQuerySurfaceCount);
  appendReceiptField(receipt, "room_editing_collision_walkable_surface_count",
                     window.roomEditing.collisionWalkableSurfaceCount);
  appendReceiptField(receipt, "room_editing_collision_actor_blocker_count",
                     window.roomEditing.collisionActorBlockerSurfaceCount);
  appendReceiptField(receipt, "room_editing_collision_projectile_blocker_count",
                     window.roomEditing.collisionProjectileBlockerSurfaceCount);
  appendReceiptField(receipt, "room_editing_undo_depth",
                     window.roomEditing.undoDepth);
  appendReceiptField(receipt, "room_editing_redo_depth",
                     window.roomEditing.redoDepth);
  appendReceiptField(receipt, "room_editing_last_operation",
                     window.roomEditingLastOperation);
  appendReceiptField(receipt, "room_editing_last_operation_status",
                     window.roomEditingLastOperationStatus);
  appendReceiptField(receipt, "room_editing_last_operation_reason_code",
                     window.roomEditingLastOperationReasonCode);
  appendReceiptField(receipt, "room_editing_last_input_source",
                     window.roomEditingLastInputSource);
  appendReceiptField(receipt, "room_editing_last_operation_accepted",
                     window.roomEditingLastOperationAccepted);
  appendReceiptField(receipt, "room_editing_last_primitive_id",
                     window.roomEditingLastPrimitiveId);
  appendReceiptField(receipt, "active_room_loaded", window.activeRoom.loaded);
  appendReceiptField(receipt, "active_room_status", window.activeRoom.status);
  appendReceiptField(receipt, "active_room_reason_code",
                     window.activeRoom.reasonCode);
  appendReceiptField(receipt, "active_room_source", window.activeRoom.source);
  appendReceiptField(receipt, "active_room_id", window.activeRoom.roomId);
  appendReceiptField(receipt, "active_room_source_name",
                     window.activeRoom.sourceName);
  appendReceiptField(receipt, "active_room_source_subset",
                     window.activeRoom.sourceSubset);
  appendReceiptField(receipt, "active_room_has_authored_room",
                     window.activeRoom.hasAuthoredRoom);
  appendReceiptField(receipt, "active_room_authored_floor_count",
                     window.activeRoom.authoredFloorCount);
  appendReceiptField(receipt, "active_room_authored_wall_count",
                     window.activeRoom.authoredWallCount);
  appendReceiptField(receipt, "active_room_authored_object_count",
                     window.activeRoom.authoredObjectCount);
  appendReceiptField(receipt, "active_room_authored_marker_count",
                     window.activeRoom.authoredMarkerCount);
  appendReceiptField(receipt, "active_room_static_mesh_count",
                     window.activeRoom.staticMeshCount);
  appendReceiptField(receipt, "active_room_anchor_count",
                     window.activeRoom.anchorCount);
  appendReceiptField(receipt, "active_room_opening_count",
                     window.activeRoom.openingCount);
  appendReceiptField(receipt, "active_room_spatial_surface_count",
                     window.activeRoom.spatialSurfaceCount);
  appendReceiptField(receipt, "active_room_walkable_surface_count",
                     window.activeRoom.walkableSurfaceCount);
  appendReceiptField(receipt, "active_room_actor_blocker_count",
                     window.activeRoom.actorBlockerSurfaceCount);
  appendReceiptField(receipt, "active_room_projectile_blocker_count",
                     window.activeRoom.projectileBlockerSurfaceCount);
  appendReceiptField(receipt, "active_room_collision_ready",
                     window.activeRoomCollision.ready);
  appendReceiptField(receipt, "active_room_collision_status",
                     window.activeRoomCollision.status);
  appendReceiptField(receipt, "active_room_collision_reason_code",
                     window.activeRoomCollision.reasonCode);
  appendReceiptField(receipt, "active_room_collision_room_id",
                     window.activeRoomCollision.roomId);
  appendReceiptField(receipt, "active_room_collision_spatial_surface_count",
                     window.activeRoomCollision.spatialSurfaceCount);
  appendReceiptField(receipt, "active_room_collision_query_surface_count",
                     window.activeRoomCollision.querySurfaceCount);
  appendReceiptField(receipt, "active_room_collision_walkable_surface_count",
                     window.activeRoomCollision.walkableSurfaceCount);
  appendReceiptField(receipt, "active_room_collision_actor_blocker_count",
                     window.activeRoomCollision.actorBlockerSurfaceCount);
  appendReceiptField(receipt, "active_room_collision_projectile_blocker_count",
                     window.activeRoomCollision.projectileBlockerSurfaceCount);
  appendReceiptField(receipt, "active_room_collision_runtime_owned_surface_count",
                     window.activeRoomCollision.runtimeOwnedSurfaceCount);
  appendReceiptField(receipt, "active_room_collision_runtime_filtered_surface_count",
                     window.activeRoomCollision.runtimeFilteredSurfaceCount);
  appendReceiptField(receipt, "active_room_collision_door_blocker_count",
                     window.activeRoomCollision.doorBlockerSurfaceCount);
  appendReceiptField(receipt, "active_room_collision_active_door_blocker_count",
                     window.activeRoomCollision.activeDoorBlockerSurfaceCount);
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
                     window.productSaveLoadResult.status);
  appendReceiptField(receipt, "product_save_load_reason_code",
                     window.productSaveLoadResult.reasonCode);
  appendReceiptField(receipt, "product_save_load_save_id",
                     window.productSaveLoadResult.record.id);
  appendReceiptField(receipt, "product_save_load_source",
                     window.productSaveLoadSource);
  appendReceiptField(receipt, "product_save_load_selected_id",
                     window.productSaveLoadSelectedId);
  appendReceiptField(receipt, "product_save_load_selected_enabled",
                     window.productSaveLoadSelectedEnabled);
  appendReceiptField(receipt,
                     "product_save_load_authored_room_present",
                     window.productSaveLoadResult.authoredRoomPresent);
  appendReceiptField(receipt,
                     "product_save_load_authored_room_id",
                     window.productSaveLoadResult.authoredRoomId);
  appendReceiptField(receipt,
                     "product_save_load_authored_floor_count",
                     window.productSaveLoadResult.authoredFloorCount);
  appendReceiptField(receipt,
                     "product_save_load_authored_wall_count",
                     window.productSaveLoadResult.authoredWallCount);
  appendReceiptField(receipt,
                     "product_save_load_authored_object_count",
                     window.productSaveLoadResult.authoredObjectCount);
  appendReceiptField(receipt,
                     "product_save_load_authored_marker_count",
                     window.productSaveLoadResult.authoredMarkerCount);
  appendReceiptField(receipt, "saved_marker_bind_status",
                     window.savedMarkerBind.status);
  appendReceiptField(receipt, "saved_marker_bind_reason_code",
                     window.savedMarkerBind.reasonCode);
  appendReceiptField(receipt, "saved_marker_bind_requested",
                     window.savedMarkerBind.requested);
  appendReceiptField(receipt, "saved_marker_bind_session_replaced",
                     window.savedMarkerBind.sessionReplaced);
  appendReceiptField(receipt, "saved_marker_bind_room_id",
                     window.savedMarkerBind.roomId);
  appendReceiptField(receipt, "saved_marker_bind_marker_count",
                     window.savedMarkerBind.markerCount);
  appendReceiptField(receipt, "saved_marker_bind_seed_entity_count",
                     window.savedMarkerBind.seedEntityCount);
  appendReceiptField(receipt, "saved_marker_bind_added_entity_count",
                     window.savedMarkerBind.addedEntityCount);
  appendReceiptField(receipt, "saved_marker_bind_existing_entity_count",
                     window.savedMarkerBind.existingEntityCount);
  appendReceiptField(receipt, "saved_marker_bind_added_objective_count",
                     window.savedMarkerBind.addedObjectiveCount);
  appendReceiptField(receipt, "saved_marker_bind_existing_objective_count",
                     window.savedMarkerBind.existingObjectiveCount);
  appendReceiptField(receipt, "saved_marker_bind_added_combatant_count",
                     window.savedMarkerBind.addedCombatantCount);
  appendReceiptField(receipt, "saved_marker_bind_existing_combatant_count",
                     window.savedMarkerBind.existingCombatantCount);
  appendReceiptField(receipt, "saved_marker_bind_pickup_count",
                     window.savedMarkerBind.pickupCount);
  appendReceiptField(receipt, "saved_marker_bind_door_count",
                     window.savedMarkerBind.doorCount);
  appendReceiptField(receipt, "saved_marker_bind_marker_entity_count",
                     window.savedMarkerBind.markerEntityCount);
  appendReceiptField(receipt, "saved_marker_bind_npc_count",
                     window.savedMarkerBind.npcCount);
  appendReceiptField(receipt, "saved_marker_bind_previous_hash",
                     window.savedMarkerBind.previousHash);
  appendReceiptField(receipt, "saved_marker_bind_bound_hash",
                     window.savedMarkerBind.boundHash);
  appendReceiptField(receipt, "room_editor_cursor_ready",
                     window.roomEditorCursorReady);
  appendReceiptField(receipt, "room_editor_grid_x",
                     std::to_string(window.roomEditorCursor.gridX));
  appendReceiptField(receipt, "room_editor_grid_z",
                     std::to_string(window.roomEditorCursor.gridZ));
  appendReceiptField(receipt, "room_editor_story_index",
                     std::to_string(window.roomEditorCursor.storyIndex));
  appendReceiptField(receipt, "room_editor_cell_size_meters",
                     floatReceiptValue(window.roomEditorCursor.cellSizeMeters));
  appendReceiptField(receipt, "room_editor_tool",
                     productRoomEditorToolName(window.roomEditorCursor.selectedTool));
  appendReceiptField(receipt, "room_editor_wall_direction",
                     productRoomEditorDirectionName(window.roomEditorCursor.wallDirection));
  appendReceiptField(receipt, "room_editor_status", window.roomEditorStatus);
  appendReceiptField(receipt, "room_editor_reason_code",
                     window.roomEditorReasonCode);
  appendReceiptField(receipt, "room_editor_last_operation",
                     window.roomEditorLastOperation);
  appendReceiptField(receipt, "room_editor_last_operation_accepted",
                     window.roomEditorLastOperationAccepted);
  appendReceiptField(receipt, "room_editor_last_primitive_id",
                     window.roomEditorLastPrimitiveId);
  appendReceiptField(receipt, "room_editor_overlay_visible",
                     window.roomEditorOverlayVisible);
  appendReceiptField(receipt, "room_editor_overlay_status",
                     window.roomEditorOverlayStatus);
  appendReceiptField(receipt, "room_editor_overlay_reason_code",
                     window.roomEditorOverlayReasonCode);
  appendReceiptField(receipt, "room_editor_overlay_item_count",
                     window.roomEditorOverlayItemCount);
  appendReceiptField(receipt, "room_editor_overlay_world_x",
                     floatReceiptValue(window.roomEditorOverlayWorldX));
  appendReceiptField(receipt, "room_editor_overlay_world_y",
                     floatReceiptValue(window.roomEditorOverlayWorldY));
  appendReceiptField(receipt, "room_editor_overlay_world_z",
                     floatReceiptValue(window.roomEditorOverlayWorldZ));
  appendReceiptField(receipt, "room_editor_preview_pending",
                     window.roomEditorPreviewActive);
  appendReceiptField(receipt, "room_editor_preview_visible",
                     window.roomEditorPreviewVisible);
  appendReceiptField(receipt, "room_editor_preview_status",
                     window.roomEditorPreviewStatus);
  appendReceiptField(receipt, "room_editor_preview_reason_code",
                     window.roomEditorPreviewReasonCode);
  appendReceiptField(receipt, "room_editor_preview_candidate_id",
                     window.roomEditorPreviewCandidateId);
  appendReceiptField(receipt, "room_editor_preview_tool",
                     window.roomEditorPreviewTool);
  appendReceiptField(receipt, "room_editor_preview_grid_x",
                     std::to_string(window.roomEditorPreviewGridX));
  appendReceiptField(receipt, "room_editor_preview_grid_z",
                     std::to_string(window.roomEditorPreviewGridZ));
  appendReceiptField(receipt, "room_editor_preview_before_draw_count",
                     std::to_string(window.roomEditorPreviewBeforeDrawCount));
  appendReceiptField(receipt, "room_editor_preview_after_draw_count",
                     std::to_string(window.roomEditorPreviewAfterDrawCount));
  appendReceiptField(
      receipt,
      "room_editor_preview_avoided_draw_count_delta",
      std::to_string(window.roomEditorPreviewAvoidedDrawCountDelta));
  appendReceiptField(
      receipt,
      "room_editor_preview_before_triangle_count",
      std::to_string(window.roomEditorPreviewBeforeTriangleCount));
  appendReceiptField(receipt,
                     "room_editor_preview_after_triangle_count",
                     std::to_string(window.roomEditorPreviewAfterTriangleCount));
  appendReceiptField(
      receipt,
      "room_editor_preview_avoided_triangle_count_delta",
      std::to_string(window.roomEditorPreviewAvoidedTriangleCountDelta));
  appendReceiptField(receipt, "room_editor_preview_optimized_draw_delta",
                     std::to_string(window.roomEditorPreviewOptimizedDrawDelta));
  appendReceiptField(receipt, "room_editor_preview_optimized_triangle_delta",
                     std::to_string(
                         window.roomEditorPreviewOptimizedTriangleDelta));
  appendReceiptField(receipt, "room_editor_hud_visible",
                     window.roomEditorHud.visible);
  appendReceiptField(receipt, "room_editor_hud_status",
                     window.roomEditorHud.status);
  appendReceiptField(receipt, "room_editor_hud_reason_code",
                     window.roomEditorHud.reasonCode);
  appendReceiptField(receipt, "room_editor_hud_tool",
                     window.roomEditorHud.toolName);
  appendReceiptField(receipt, "room_editor_hud_wall_direction",
                     window.roomEditorHud.wallDirectionName);
  appendReceiptField(receipt, "room_editor_hud_grid_x",
                     std::to_string(window.roomEditorHud.gridX));
  appendReceiptField(receipt, "room_editor_hud_grid_z",
                     std::to_string(window.roomEditorHud.gridZ));
  appendReceiptField(receipt, "room_editor_hud_last_operation",
                     window.roomEditorHud.lastOperation);
  appendReceiptField(receipt, "room_editor_hud_last_operation_accepted",
                     window.roomEditorHud.lastOperationAccepted);
  appendReceiptField(receipt, "room_editor_hud_last_primitive_id",
                     window.roomEditorHud.lastPrimitiveId);
  appendReceiptField(receipt, "room_editor_hud_preview_active",
                     window.roomEditorHud.previewActive);
  appendReceiptField(receipt, "room_editor_hud_preview_status",
                     window.roomEditorHud.previewStatus);
  appendReceiptField(receipt, "room_editor_hud_preview_candidate_id",
                     window.roomEditorHud.previewCandidateId);
  appendReceiptField(receipt, "room_editor_hud_preview_optimized_draw_delta",
                     std::to_string(
                         window.roomEditorHud.previewOptimizedDrawDelta));
  appendReceiptField(receipt, "room_editor_hud_preview_optimized_triangle_delta",
                     std::to_string(
                         window.roomEditorHud.previewOptimizedTriangleDelta));
  appendReceiptField(receipt, "room_editor_hud_line_count",
                     std::to_string(window.roomEditorHud.lineCount));
  appendReceiptField(receipt,
                     "save_browser_mode",
                     frontendSaveBrowserModeName(frontend.saveBrowserMode));
  appendReceiptField(receipt, "selected_save_id", window.selectedProductSaveId);
  appendReceiptField(receipt, "selected_save_enabled",
                     window.selectedProductSaveEnabled);
  appendReceiptField(receipt, "selected_save_status",
                     window.selectedProductSaveStatus);
  appendReceiptField(receipt, "save_slot_browser_mode", window.saveSlotBrowserMode);
  appendReceiptField(receipt, "save_slot_ring_count", window.saveSlotRingCount);
  appendReceiptField(receipt,
                     "save_slot_ring_selected_index",
                     window.saveSlotRingSelectedIndex);
  appendReceiptField(receipt,
                     "save_slot_ring_selected_id",
                     window.saveSlotRingSelectedId);
  appendReceiptField(receipt,
                     "save_slot_ring_selected_status",
                     window.saveSlotRingSelectedStatus);
  appendReceiptField(receipt, "save_slot_action_command",
                     window.saveSlotActionCommand);
  appendReceiptField(receipt, "save_slot_action_enabled",
                     window.saveSlotActionEnabled);
  appendReceiptField(receipt,
                     "save_slot_action_confirmation_required",
                     window.saveSlotActionConfirmationRequired);
  appendReceiptField(receipt, "save_slot_action_status",
                     window.saveSlotActionStatus);
  appendReceiptField(receipt, "save_flow_operation", window.saveFlowOperation);
  appendReceiptField(receipt,
                     "save_flow_source_surface",
                     window.saveFlowSourceSurface);
  appendReceiptField(receipt, "save_flow_status", window.saveFlowStatus);
  appendReceiptField(receipt,
                     "save_flow_reason_code",
                     window.saveFlowReasonCode);
  appendReceiptField(receipt,
                     "save_flow_affected_slot_id",
                     window.saveFlowAffectedSlotId);
  appendReceiptField(receipt,
                     "save_flow_active_count_before",
                     window.saveFlowActiveCountBefore);
  appendReceiptField(receipt,
                     "save_flow_active_count_after",
                     window.saveFlowActiveCountAfter);
  appendReceiptField(receipt,
                     "save_flow_deleted_count_after",
                     window.saveFlowDeletedCountAfter);
  appendReceiptField(receipt,
                     "save_flow_selected_slot_after",
                     window.saveFlowSelectedSlotAfter);
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
                     window.productSaveLoadResult.previousHash);
  appendReceiptField(receipt, "product_save_load_loaded_hash",
                     window.productSaveLoadResult.loadedHash);
  appendReceiptField(receipt, "product_save_load_session_loaded",
                     window.productSaveLoadResult.sessionLoaded);
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
  appendReceiptField(receipt, "gameplay_tick_reason_code",
                     window.gameplayTickReasonCode);
  appendReceiptField(receipt, "player_position_changed", window.playerPositionChanged);
  appendReceiptField(receipt, "gameplay_movement_attempted",
                     movementProof.attempted);
  appendReceiptField(receipt, "gameplay_movement_blocked",
                     movementProof.blocked);
  appendReceiptField(receipt, "gameplay_movement_status",
                     movementProof.status);
  appendReceiptField(receipt, "gameplay_movement_debug_available",
                     movementProof.debugAvailable);
  appendReceiptField(receipt, "gameplay_movement_reason_code",
                     movementProof.reasonCode);
  appendReceiptField(receipt, "gameplay_movement_blocked_reason",
                     movementProof.blockedReason);
  appendReceiptField(receipt, "gameplay_movement_hit_surface_id",
                     movementProof.hitSurfaceId);
  appendReceiptField(receipt, "gameplay_movement_ground_snap_applied",
                     movementProof.groundSnapApplied);
  appendReceiptField(receipt, "gameplay_movement_clamped",
                     movementProof.movementClamped);
  appendReceiptField(receipt, "gameplay_movement_slid",
                     movementProof.movementSlid);
  appendReceiptField(receipt, "gameplay_movement_collision_sweep_count",
                     movementProof.collisionSweepCount);
  appendReceiptField(receipt, "gameplay_movement_policy_band",
                     movementProof.policyBand);
  appendReceiptField(receipt, "gameplay_movement_slope_travel_direction",
                     movementProof.slopeTravelDirection);
  appendReceiptField(receipt, "gameplay_movement_slope_angle_degrees",
                     floatReceiptValue(movementProof.slopeAngleDegrees));
  appendReceiptField(receipt, "gameplay_movement_speed_multiplier",
                     floatReceiptValue(movementProof.speedMultiplier));
  appendReceiptField(receipt, "gameplay_movement_start_x",
                     floatReceiptValue(movementProof.startX));
  appendReceiptField(receipt, "gameplay_movement_start_y",
                     floatReceiptValue(movementProof.startY));
  appendReceiptField(receipt, "gameplay_movement_start_z",
                     floatReceiptValue(movementProof.startZ));
  appendReceiptField(receipt, "gameplay_movement_final_x",
                     floatReceiptValue(movementProof.finalX));
  appendReceiptField(receipt, "gameplay_movement_final_y",
                     floatReceiptValue(movementProof.finalY));
  appendReceiptField(receipt, "gameplay_movement_final_z",
                     floatReceiptValue(movementProof.finalZ));
  appendReceiptField(receipt, "gameplay_movement_horizontal_distance_meters",
                     floatReceiptValue(movementProof.horizontalDistanceMeters));
  appendReceiptField(receipt, "gameplay_movement_vertical_delta_meters",
                     floatReceiptValue(movementProof.verticalDeltaMeters));
  appendReceiptField(receipt, "gameplay_movement_ground_velocity_x",
                     floatReceiptValue(movementProof.groundVelocityX));
  appendReceiptField(receipt, "gameplay_movement_ground_velocity_z",
                     floatReceiptValue(movementProof.groundVelocityZ));
  appendReceiptField(receipt, "gameplay_movement_state",
                     movementProof.stateName);
  appendReceiptField(receipt, "movement_state",
                     movementProof.stateName);
  appendReceiptField(receipt, "movement_grounded",
                     movementProof.grounded);
  appendReceiptField(receipt, "movement_vertical_velocity_mps",
                     floatReceiptValue(movementProof.verticalVelocityMetersPerSecond));
  appendReceiptField(receipt, "movement_horizontal_speed_mps",
                     floatReceiptValue(movementProof.horizontalSpeedMetersPerSecond));
  appendReceiptField(receipt, "movement_hit_surface_id",
                     movementProof.hitSurfaceId);
  appendReceiptField(receipt, "wall_run_candidate_available",
                     movementProof.wallRunCandidateAvailable);
  appendReceiptField(receipt, "wall_run_candidate_status",
                     movementProof.wallRunCandidateStatus);
  appendReceiptField(receipt, "wall_run_candidate_reason_code",
                     movementProof.wallRunCandidateReasonCode);
  appendReceiptField(receipt, "wall_run_side", movementProof.wallRunSide);
  appendReceiptField(receipt, "wall_run_surface_id",
                     movementProof.wallRunSurfaceId);
  appendReceiptField(receipt, "wall_run_normal_x",
                     floatReceiptValue(movementProof.wallRunNormalX));
  appendReceiptField(receipt, "wall_run_normal_y",
                     floatReceiptValue(movementProof.wallRunNormalY));
  appendReceiptField(receipt, "wall_run_normal_z",
                     floatReceiptValue(movementProof.wallRunNormalZ));
  appendReceiptField(receipt, "wall_run_approach_speed_mps",
                     floatReceiptValue(
                         movementProof.wallRunApproachSpeedMetersPerSecond));
  appendReceiptField(receipt, "wall_run_active",
                     movementProof.wallRunActive);
  appendReceiptField(receipt, "wall_run_status",
                     movementProof.wallRunStatus);
  appendReceiptField(receipt, "wall_run_reason_code",
                     movementProof.wallRunReasonCode);
  appendReceiptField(receipt, "wall_run_remaining_s",
                     floatReceiptValue(movementProof.wallRunRemainingSeconds));
  appendReceiptField(receipt, "wall_run_duration_s",
                     floatReceiptValue(movementProof.wallRunDurationSeconds));
  appendReceiptField(receipt, "wall_run_gravity_multiplier",
                     floatReceiptValue(movementProof.wallRunGravityMultiplier));
  appendReceiptField(receipt, "wall_run_speed_multiplier",
                     floatReceiptValue(movementProof.wallRunSpeedMultiplier));
  appendReceiptField(receipt, "gameplay_movement_grade_percent",
                     floatReceiptValue(movementProof.gradePercent));
  appendReceiptField(receipt, "gameplay_movement_profile",
                     movementProof.profile);
  appendReceiptField(receipt, "gameplay_movement_max_speed_mps",
                     floatReceiptValue(movementProof.maxSpeedMetersPerSecond));
  appendReceiptField(receipt, "gameplay_jump_requested",
                     window.gameplayJumpRequested);
  appendReceiptField(receipt, "gameplay_jump_accepted",
                     window.gameplayJumpAccepted);
  appendReceiptField(receipt, "gameplay_jump_active", window.gameplayJumpActive);
  appendReceiptField(receipt, "gameplay_jump_status", window.gameplayJumpStatus);
  appendReceiptField(receipt, "gameplay_jump_reason_code",
                     window.gameplayJumpReasonCode);
  appendReceiptField(receipt, "gameplay_jump_velocity_mps",
                     floatReceiptValue(window.gameplayJumpVelocityMetersPerSecond));
  appendReceiptField(receipt, "gameplay_jump_coyote_seconds_remaining",
                     floatReceiptValue(window.gameplayJumpCoyoteSecondsRemaining));
  appendReceiptField(receipt, "gameplay_jump_buffer_seconds_remaining",
                     floatReceiptValue(window.gameplayJumpBufferSecondsRemaining));
  appendReceiptField(receipt, "gameplay_jump_held", window.gameplayJumpHeld);
  appendReceiptField(receipt, "gameplay_jump_cut_applied",
                     window.gameplayJumpCutApplied);
  appendReceiptField(receipt, "gameplay_jump_ground_y",
                     floatReceiptValue(window.gameplayJumpGroundY));
  appendReceiptField(receipt, "gameplay_jump_start_y",
                     floatReceiptValue(window.gameplayJumpStartY));
  appendReceiptField(receipt, "gameplay_jump_final_y",
                     floatReceiptValue(window.gameplayJumpFinalY));
  appendReceiptField(receipt, "gameplay_jump_height_meters",
                     floatReceiptValue(window.gameplayJumpHeightMeters));
  appendReceiptField(receipt, "gameplay_reset_triggered",
                     window.gameplayResetTriggered);
  appendReceiptField(receipt, "gameplay_reset_status",
                     window.gameplayResetStatus);
  appendReceiptField(receipt, "gameplay_reset_reason_code",
                     window.gameplayResetReasonCode);
  appendReceiptField(receipt, "gameplay_reset_spawn_anchor_id",
                     window.gameplayResetSpawnAnchorId);
  appendReceiptField(receipt, "gameplay_reset_source_anchor_id",
                     window.gameplayResetSourceAnchorId);
  appendReceiptField(receipt, "gameplay_reset_start_y",
                     floatReceiptValue(window.gameplayResetStartY));
  appendReceiptField(receipt, "gameplay_reset_final_y",
                     floatReceiptValue(window.gameplayResetFinalY));
  appendReceiptField(receipt, "gameplay_traversal_requested",
                     window.gameplayTraversalRequested);
  appendReceiptField(receipt, "gameplay_traversal_consumed",
                     window.gameplayTraversalConsumed);
  appendReceiptField(receipt, "gameplay_traversal_accepted",
                     window.gameplayTraversalAccepted);
  appendReceiptField(receipt, "gameplay_traversal_fallback_jump_allowed",
                     window.gameplayTraversalFallbackJumpAllowed);
  appendReceiptField(receipt, "gameplay_traversal_status",
                     window.gameplayTraversalStatus);
  appendReceiptField(receipt, "gameplay_traversal_reason_code",
                     window.gameplayTraversalReasonCode);
  appendReceiptField(receipt, "gameplay_traversal_mechanic",
                     window.gameplayTraversalMechanic);
  appendReceiptField(receipt, "gameplay_traversal_slot_id",
                     window.gameplayTraversalSlotId);
  appendReceiptField(receipt, "gameplay_traversal_target_id",
                     window.gameplayTraversalTargetId);
  appendReceiptField(receipt, "gameplay_traversal_landing_surface_id",
                     window.gameplayTraversalLandingSurfaceId);
  appendReceiptField(receipt, "gameplay_traversal_start_x",
                     floatReceiptValue(window.gameplayTraversalStartX));
  appendReceiptField(receipt, "gameplay_traversal_start_y",
                     floatReceiptValue(window.gameplayTraversalStartY));
  appendReceiptField(receipt, "gameplay_traversal_start_z",
                     floatReceiptValue(window.gameplayTraversalStartZ));
  appendReceiptField(receipt, "gameplay_traversal_final_x",
                     floatReceiptValue(window.gameplayTraversalFinalX));
  appendReceiptField(receipt, "gameplay_traversal_final_y",
                     floatReceiptValue(window.gameplayTraversalFinalY));
  appendReceiptField(receipt, "gameplay_traversal_final_z",
                     floatReceiptValue(window.gameplayTraversalFinalZ));
  appendReceiptField(receipt, "gameplay_dash_requested",
                     window.gameplayDashRequested);
  appendReceiptField(receipt, "gameplay_dash_accepted",
                     window.gameplayDashAccepted);
  appendReceiptField(receipt, "gameplay_dash_status", window.gameplayDashStatus);
  appendReceiptField(receipt, "gameplay_dash_reason_code",
                     window.gameplayDashReasonCode);
  appendReceiptField(receipt, "gameplay_dash_speed_mps",
                     floatReceiptValue(window.gameplayDashSpeedMetersPerSecond));
  appendReceiptField(receipt, "gameplay_dash_distance_meters",
                     floatReceiptValue(window.gameplayDashDistanceMeters));
  appendReceiptField(receipt, "gameplay_dash_cooldown_remaining_seconds",
                     floatReceiptValue(window.gameplayDashCooldownRemainingSeconds));
  appendReceiptField(receipt, "gameplay_dash_direction_x",
                     floatReceiptValue(window.gameplayDashDirectionX));
  appendReceiptField(receipt, "gameplay_dash_direction_z",
                     floatReceiptValue(window.gameplayDashDirectionZ));
  appendReceiptField(receipt, "movement_debug_hud_visible", movementHud.visible);
  appendReceiptField(receipt, "movement_debug_hud_line_count",
                     static_cast<std::uint64_t>(movementHud.lines.size()));
  appendReceiptField(receipt, "movement_debug_hud_dev_tools_enabled",
                     movementHud.developerToolsEnabled);
  appendReceiptField(receipt, "movement_debug_hud_debug_overlay_enabled",
                     movementHud.debugOverlayEnabled);
  appendReceiptField(receipt, "movement_debug_hud_debug_available",
                     movementHud.debugAvailable);
  appendReceiptField(receipt, "movement_debug_hud_status", movementHud.status);
  appendReceiptField(receipt, "movement_debug_hud_blocked", movementHud.blocked);
  appendReceiptField(receipt, "movement_debug_hud_reason_code",
                     movementHud.reasonCode);
  appendReceiptField(receipt, "movement_debug_hud_hit_surface_id",
                     movementHud.hitSurfaceId);
  appendReceiptField(receipt, "movement_debug_hud_policy_band",
                     movementHud.policyBand);
  appendReceiptField(receipt, "movement_debug_hud_speed_multiplier",
                     floatReceiptValue(movementHud.speedMultiplier));
  appendReceiptField(receipt,
                     "npc_behavior_debug_hud_visible",
                     npcBehaviorHud.visible);
  appendReceiptField(receipt,
                     "npc_behavior_debug_hud_line_count",
                     static_cast<std::uint64_t>(npcBehaviorHud.lineCount));
  appendReceiptField(receipt,
                     "npc_behavior_debug_hud_dev_tools_enabled",
                     npcBehaviorHud.developerToolsEnabled);
  appendReceiptField(receipt,
                     "npc_behavior_debug_hud_debug_overlay_enabled",
                     npcBehaviorHud.debugOverlayEnabled);
  appendReceiptField(receipt,
                     "npc_behavior_debug_hud_debug_available",
                     npcBehaviorHud.debugAvailable);
  appendReceiptField(receipt,
                     "npc_behavior_debug_hud_status",
                     npcBehaviorHud.status);
  appendReceiptField(receipt,
                     "npc_behavior_debug_hud_reason_code",
                     npcBehaviorHud.reasonCode);
  appendReceiptField(receipt,
                     "npc_behavior_debug_hud_has_unresolved_profile",
                     window.npcBehaviorDebugHudHasUnresolvedProfile);
  appendReceiptField(receipt, "physics_debug_hud_visible",
                     physicsHud.visible);
  appendReceiptField(receipt, "physics_debug_hud_line_count",
                     static_cast<std::uint64_t>(physicsHud.lineCount));
  appendReceiptField(receipt, "physics_debug_hud_dev_tools_enabled",
                     physicsHud.developerToolsEnabled);
  appendReceiptField(receipt, "physics_debug_hud_debug_overlay_enabled",
                     physicsHud.debugOverlayEnabled);
  appendReceiptField(receipt, "physics_debug_hud_debug_available",
                     physicsHud.debugAvailable);
  appendReceiptField(receipt, "physics_debug_hud_status",
                     physicsHud.status);
  appendReceiptField(receipt, "physics_debug_hud_reason_code",
                     physicsHud.reasonCode);
  appendReceiptField(receipt, "physics_debug_hud_has_warnings",
                     physicsHud.hasWarnings);
  appendReceiptField(receipt, "position_hud_visible",
                     window.positionHud.visible);
  appendReceiptField(receipt, "position_hud_line_count",
                     static_cast<std::uint64_t>(window.positionHud.lineCount));
  appendReceiptField(receipt, "position_hud_debug_available",
                     window.positionHud.debugAvailable);
  appendReceiptField(receipt, "position_hud_status",
                     window.positionHud.status);
  appendReceiptField(receipt, "position_hud_reason_code",
                     window.positionHud.reasonCode);
  appendReceiptField(receipt, "position_hud_player_position_available",
                     window.positionHud.playerPositionAvailable);
  appendReceiptField(receipt, "position_hud_world_x",
                     floatReceiptValue(window.positionHud.worldX));
  appendReceiptField(receipt, "position_hud_world_y",
                     floatReceiptValue(window.positionHud.worldY));
  appendReceiptField(receipt, "position_hud_world_z",
                     floatReceiptValue(window.positionHud.worldZ));
  appendReceiptField(receipt, "position_hud_grid_x",
                     std::to_string(window.positionHud.gridX));
  appendReceiptField(receipt, "position_hud_grid_y",
                     std::to_string(window.positionHud.gridY));
  appendReceiptField(receipt, "position_hud_grid_z",
                     std::to_string(window.positionHud.gridZ));
  appendReceiptField(receipt, "position_hud_layer_index",
                     std::to_string(window.positionHud.layerIndex));
  appendReceiptField(receipt, "position_hud_facing",
                     window.positionHud.facing);
  appendReceiptField(receipt, "position_hud_yaw_degrees",
                     floatReceiptValue(window.positionHud.yawDegrees));
  appendReceiptField(receipt, "position_hud_pitch_degrees",
                     floatReceiptValue(window.positionHud.pitchDegrees));
  appendReceiptField(receipt, "gameplay_collision_surfaces_used",
                     window.gameplayCollisionSurfacesUsed);
  appendReceiptField(receipt, "gameplay_collision_surface_count",
                     window.gameplayCollisionSurfaceCount);
  appendReceiptField(receipt, "physics_movement_planner_enabled",
                     window.physicsMovementPlannerEnabled);
  appendReceiptField(receipt, "physics_movement_planner_requested",
                     window.physicsMovementPlannerRequested);
  appendReceiptField(receipt, "physics_movement_planner_used",
                     window.physicsMovementPlannerUsed);
  appendReceiptField(receipt, "physics_movement_planner_status",
                     window.physicsMovementPlannerStatus);
  appendReceiptField(receipt, "physics_movement_planner_reason_code",
                     window.physicsMovementPlannerReasonCode);
  appendReceiptField(receipt, "target_discovered", window.targetDiscovered);
  appendReceiptField(receipt, "gameplay_target_status",
                     window.gameplayTargetStatus);
  appendReceiptField(receipt, "gameplay_target_action",
                     window.gameplayTargetAction);
  appendReceiptField(receipt, "gameplay_target_entity_id",
                     window.gameplayTargetEntityId);
  appendReceiptField(receipt, "gameplay_target_stable_name",
                     window.gameplayTargetStableName);
  appendReceiptField(receipt, "gameplay_target_kind", window.gameplayTargetKind);
  appendReceiptField(receipt, "gameplay_target_distance_meters",
                     floatReceiptValue(window.gameplayTargetDistanceMeters));
  appendReceiptField(receipt, "gameplay_target_supports_command",
                     window.gameplayTargetSupportsCommand);
  appendReceiptField(receipt, "gameplay_outcome_status",
                     window.gameplayOutcomeStatus);
  appendReceiptField(receipt, "gameplay_outcome_target_active_after",
                     window.gameplayOutcomeTargetActiveAfter);
  appendReceiptField(receipt, "gameplay_outcome_inventory_changed",
                     window.gameplayOutcomeInventoryChanged);
  appendReceiptField(receipt, "gameplay_outcome_item_id",
                     window.gameplayOutcomeItemId);
  appendReceiptField(receipt, "gameplay_outcome_item_count",
                     window.gameplayOutcomeItemCount);
  appendReceiptField(receipt, "gameplay_outcome_objective_changed",
                     window.gameplayOutcomeObjectiveChanged);
  appendReceiptField(receipt, "gameplay_outcome_event_count",
                     window.gameplayOutcomeEventCount);
  appendReceiptField(receipt, "session_outcome", window.sessionOutcome);
  appendReceiptField(receipt, "gameplay_tape_requested",
                     window.gameplayTapeRequested);
  appendReceiptField(receipt, "gameplay_tape_loaded", window.gameplayTapeLoaded);
  appendReceiptField(receipt, "gameplay_tape_path", window.gameplayTapePath);
  appendReceiptField(receipt, "gameplay_tape_status", window.gameplayTapeStatus);
  appendReceiptField(receipt, "gameplay_tape_reason_code",
                     window.gameplayTapeReasonCode);
  appendReceiptField(receipt, "gameplay_tape_line_count",
                     window.gameplayTapeLineCount);
  appendReceiptField(receipt, "gameplay_tape_step_count",
                     window.gameplayTapeStepCount);
  appendReceiptField(receipt, "gameplay_tape_executed_step_count",
                     window.gameplayTapeExecutedStepCount);
  appendReceiptField(receipt, "gameplay_tape_expected_rejected_step_count",
                     window.gameplayTapeExpectedRejectedStepCount);
  appendReceiptField(receipt, "gameplay_tape_expected_blocked_step_count",
                     window.gameplayTapeExpectedBlockedStepCount);
  appendReceiptField(receipt, "gameplay_tape_failed_step",
                     window.gameplayTapeFailedStep);
  appendReceiptField(receipt, "gameplay_tape_failed_source_line",
                     window.gameplayTapeFailedSourceLine);
  appendReceiptField(receipt, "gameplay_tape_failed_action",
                     window.gameplayTapeFailedAction);
  appendReceiptField(receipt, "gameplay_tape_failed_target",
                     window.gameplayTapeFailedTarget);
  appendReceiptField(receipt, "gameplay_tape_failed_rejection",
                     window.gameplayTapeFailedRejection);
  appendReceiptField(receipt, "gameplay_tape_failed_movement_block",
                     window.gameplayTapeFailedMovementBlock);
  appendReceiptField(receipt, "gameplay_tape_last_action",
                     window.gameplayTapeLastAction);
  appendReceiptField(receipt, "gameplay_tape_last_target",
                     window.gameplayTapeLastTarget);
  appendReceiptField(receipt, "gameplay_tape_last_movement_block",
                     window.gameplayTapeLastMovementBlock);
  appendReceiptField(receipt, "gameplay_tape_key_collected",
                     window.gameplayTapeKeyCollected);
  appendReceiptField(receipt, "gameplay_tape_secret_door_opened",
                     window.gameplayTapeSecretDoorOpened);
  appendReceiptField(receipt, "gameplay_tape_treasure_collected",
                     window.gameplayTapeTreasureCollected);
  appendReceiptField(receipt, "gameplay_tape_npc_targetable",
                     window.gameplayTapeNpcTargetable);
  appendReceiptField(receipt, "gameplay_tape_npc_defeated",
                     window.gameplayTapeNpcDefeated);
  appendReceiptField(receipt, "gameplay_tape_exit_objective_complete",
                     window.gameplayTapeExitObjectiveComplete);
  appendReceiptField(receipt, "gameplay_tape_loop_complete",
                     window.gameplayTapeLoopComplete);
  appendReceiptField(receipt, "gameplay_tape_ai_command_logged",
                     window.gameplayTapeAiCommandLogged);
  appendReceiptField(receipt, "gameplay_tape_ai_attack_logged",
                     window.gameplayTapeAiAttackLogged);
  appendReceiptField(receipt, "gameplay_tape_ai_wait_logged",
                     window.gameplayTapeAiWaitLogged);
  appendReceiptField(receipt, "gameplay_tape_ai_player_damaged",
                     window.gameplayTapeAiPlayerDamaged);
  appendReceiptField(receipt, "gameplay_tape_ai_player_hp_before",
                     static_cast<std::uint64_t>(window.gameplayTapeAiPlayerHpBefore));
  appendReceiptField(receipt, "gameplay_tape_ai_player_hp_after",
                     static_cast<std::uint64_t>(window.gameplayTapeAiPlayerHpAfter));
  appendReceiptField(receipt, "gameplay_tape_ai_actor_id",
                     window.gameplayTapeAiActorId);
  appendReceiptField(receipt, "gameplay_tape_ai_target_id",
                     window.gameplayTapeAiTargetId);
  appendReceiptField(receipt, "gameplay_tape_ai_behavior",
                     window.gameplayTapeAiBehavior);
  appendReceiptField(receipt, "gameplay_tape_ai_intent",
                     window.gameplayTapeAiIntent);
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
  appendReceiptField(receipt, "creative_fly_active",
                     window.viewport.creativeFlyActive);
  appendReceiptField(receipt, "creative_fly_status",
                     window.viewport.creativeFlyStatus);
  appendReceiptField(receipt, "creative_fly_reason_code",
                     window.viewport.creativeFlyReasonCode);
  appendReceiptField(receipt, "creative_fly_speed_mps",
                     floatReceiptValue(
                         window.viewport.creativeFlySpeedMetersPerSecond));
  appendReceiptField(receipt, "creative_fly_anchor_valid",
                     window.viewport.creativeFlyAnchorValid);
  appendReceiptField(receipt, "creative_fly_world_x",
                     floatReceiptValue(window.viewport.creativeFlyPositionMeters.x));
  appendReceiptField(receipt, "creative_fly_world_y",
                     floatReceiptValue(window.viewport.creativeFlyPositionMeters.y));
  appendReceiptField(receipt, "creative_fly_world_z",
                     floatReceiptValue(window.viewport.creativeFlyPositionMeters.z));
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
  appendReceiptField(receipt, "product_draw_door_visible",
                     window.viewport.productDrawDoorVisible);
  appendReceiptField(receipt, "product_draw_open_door_visible",
                     window.viewport.productDrawOpenDoorVisible);
  appendReceiptField(receipt, "product_draw_closed_door_visible",
                     window.viewport.productDrawClosedDoorVisible);
  appendReceiptField(receipt, "product_draw_debug_marker_count",
                     window.viewport.productDrawDebugMarkerCount);
  appendReceiptField(receipt, "product_draw_door_count",
                     window.viewport.productDrawDoorCount);
  appendReceiptField(receipt, "product_draw_open_door_count",
                     window.viewport.productDrawOpenDoorCount);
  appendReceiptField(receipt, "product_draw_closed_door_count",
                     window.viewport.productDrawClosedDoorCount);
  appendReceiptField(receipt, "product_draw_room_geometry_count",
                     window.viewport.productDrawRoomGeometryCount);
  appendReceiptField(receipt, "product_draw_floor_tile_count",
                     window.viewport.productDrawFloorTileCount);
  appendReceiptField(receipt, "product_draw_elevated_floor_tile_count",
                     window.viewport.productDrawElevatedFloorTileCount);
  appendReceiptField(receipt, "product_draw_ramp_tile_count",
                     window.viewport.productDrawRampTileCount);
  appendReceiptField(receipt, "product_draw_blocked_slope_tile_count",
                     window.viewport.productDrawBlockedSlopeTileCount);
  appendReceiptField(receipt, "product_draw_wall_tile_count",
                     window.viewport.productDrawWallTileCount);
  appendReceiptField(receipt, "product_draw_prop_visible",
                     window.viewport.productDrawPropVisible);
  appendReceiptField(receipt, "product_draw_prop_tile_count",
                     window.viewport.productDrawPropTileCount);
  appendReceiptField(receipt, "product_draw_room_editor_cursor_visible",
                     window.viewport.productDrawRoomEditorCursorVisible);
  appendReceiptField(receipt, "product_draw_room_editor_cursor_count",
                     window.viewport.productDrawRoomEditorCursorCount);
  appendReceiptField(receipt, "product_draw_room_editor_preview_visible",
                     window.viewport.productDrawRoomEditorPreviewVisible);
  appendReceiptField(receipt, "product_draw_room_editor_preview_count",
                     window.viewport.productDrawRoomEditorPreviewCount);
  appendReceiptField(receipt, "product_draw_physics_debug_visible",
                     window.viewport.productDrawPhysicsDebugVisible);
  appendReceiptField(receipt, "product_draw_physics_debug_item_count",
                     window.viewport.productDrawPhysicsDebugItemCount);
  appendReceiptField(receipt, "product_draw_physics_aabb_debug_count",
                     window.viewport.productDrawPhysicsAabbDebugCount);
  appendReceiptField(receipt,
                     "product_draw_physics_contact_normal_debug_count",
                     window.viewport.productDrawPhysicsContactNormalDebugCount);
  appendReceiptField(receipt, "product_draw_map_maker_grid_visible",
                     window.viewport.productDrawMapMakerGridVisible);
  appendReceiptField(receipt, "product_draw_map_maker_grid_dot_count",
                     window.viewport.productDrawMapMakerGridDotCount);
  appendReceiptField(receipt, "product_draw_map_maker_major_grid_dot_count",
                     window.viewport.productDrawMapMakerMajorGridDotCount);
  appendReceiptField(receipt, "product_draw_map_maker_cube_preview_visible",
                     window.viewport.productDrawMapMakerCubePreviewVisible);
  appendReceiptField(receipt, "product_draw_map_maker_cube_preview_count",
                     window.viewport.productDrawMapMakerCubePreviewCount);
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
  appendReceiptField(receipt, "product_render_bridge_room_editor_cursor_visible",
                     window.viewport.productRenderBridgeRoomEditorCursorVisible);
  appendReceiptField(receipt, "product_render_bridge_room_editor_cursor_count",
                     window.viewport.productRenderBridgeRoomEditorCursorCount);
  appendReceiptField(receipt, "product_render_bridge_room_editor_preview_visible",
                     window.viewport.productRenderBridgeRoomEditorPreviewVisible);
  appendReceiptField(receipt, "product_render_bridge_room_editor_preview_count",
                     window.viewport.productRenderBridgeRoomEditorPreviewCount);
  appendReceiptField(receipt, "product_render_bridge_prop_visible",
                     window.viewport.productRenderBridgePropVisible);
  appendReceiptField(receipt, "product_render_bridge_prop_tile_count",
                     window.viewport.productRenderBridgePropTileCount);
  appendReceiptField(receipt, "product_render_bridge_physics_debug_visible",
                     window.viewport.productRenderBridgePhysicsDebugVisible);
  appendReceiptField(receipt, "product_render_bridge_physics_debug_item_count",
                     window.viewport.productRenderBridgePhysicsDebugItemCount);
  appendReceiptField(receipt,
                     "product_render_bridge_physics_aabb_debug_count",
                     window.viewport.productRenderBridgePhysicsAabbDebugCount);
  appendReceiptField(
      receipt,
      "product_render_bridge_physics_contact_normal_debug_count",
      window.viewport.productRenderBridgePhysicsContactNormalDebugCount);
  appendReceiptField(receipt, "product_render_bridge_map_maker_grid_visible",
                     window.viewport.productRenderBridgeMapMakerGridVisible);
  appendReceiptField(receipt, "product_render_bridge_map_maker_grid_dot_count",
                     window.viewport.productRenderBridgeMapMakerGridDotCount);
  appendReceiptField(receipt,
                     "product_render_bridge_map_maker_major_grid_dot_count",
                     window.viewport.productRenderBridgeMapMakerMajorGridDotCount);
  appendReceiptField(
      receipt,
      "product_render_bridge_map_maker_cube_preview_visible",
      window.viewport.productRenderBridgeMapMakerCubePreviewVisible);
  appendReceiptField(
      receipt,
      "product_render_bridge_map_maker_cube_preview_count",
      window.viewport.productRenderBridgeMapMakerCubePreviewCount);
  appendReceiptField(receipt, "product_feedback_bridge_ready",
                     window.viewport.productFeedbackBridgeReady);
  appendReceiptField(receipt, "product_feedback_bridge_line_count",
                     window.viewport.productFeedbackBridgeLineCount);
  appendReceiptField(receipt, "product_vulkan_room_mesh_cpu_ready",
                     window.viewport.productVulkanRoomMeshCpuReady);
  appendReceiptField(receipt, "product_vulkan_room_mesh_backend_presented",
                     window.viewport.productVulkanRoomMeshBackendPresented);
  appendReceiptField(receipt, "product_vulkan_room_mesh_source",
                     window.viewport.productVulkanRoomMeshSource);
  appendReceiptField(receipt, "product_vulkan_room_asset_id",
                     window.viewport.productVulkanRoomAssetId);
  appendReceiptField(receipt, "product_vulkan_room_floor_visible",
                     window.viewport.productVulkanRoomFloorVisible);
  appendReceiptField(receipt, "product_vulkan_room_wall_visible",
                     window.viewport.productVulkanRoomWallVisible);
  appendReceiptField(receipt, "product_vulkan_room_grid_visible",
                     window.viewport.productVulkanRoomGridVisible);
  appendReceiptField(receipt, "product_vulkan_room_source_mesh_count",
                     window.viewport.productVulkanRoomSourceMeshCount);
  appendReceiptField(receipt, "product_vulkan_room_vertex_count",
                     window.viewport.productVulkanRoomVertexCount);
  appendReceiptField(receipt, "product_vulkan_room_index_count",
                     window.viewport.productVulkanRoomIndexCount);
  appendReceiptField(receipt, "product_vulkan_room_draw_count",
                     window.viewport.productVulkanRoomDrawCount);
  appendReceiptField(receipt, "product_vulkan_room_floor_draw_count",
                     window.viewport.productVulkanRoomFloorDrawCount);
  appendReceiptField(receipt, "product_vulkan_room_wall_draw_count",
                     window.viewport.productVulkanRoomWallDrawCount);
  appendReceiptField(receipt, "product_vulkan_room_grid_line_draw_count",
                     window.viewport.productVulkanRoomGridLineDrawCount);
  appendReceiptField(receipt, "product_vulkan_room_grid_truncated",
                     window.viewport.productVulkanRoomGridTruncated);
  appendReceiptField(receipt, "product_vulkan_room_geometry_signature",
                     window.viewport.productVulkanRoomGeometrySignature);
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
  appendReceiptField(receipt, "active_surface",
                     productFrontendSurfaceName(activeSurface.activeSurface));
  appendReceiptField(receipt, "active_parent_surface",
                     productFrontendSurfaceName(activeSurface.parentSurface));
  appendReceiptField(receipt, "input_surface",
                     productInputSurfaceName(activeSurface.inputSurface));
  appendReceiptField(receipt, "input_owner",
                     menuOwnerName(activeSurface.inputOwner));
  appendReceiptField(receipt, "active_surface_status", activeSurface.status);
  appendReceiptField(receipt,
                     "active_surface_mouse_capture_policy",
                     productActiveMouseCapturePolicyName(
                         activeSurface.mouseCapturePolicy));
  appendReceiptField(receipt, "input_action_last", inputActionName(window.lastInputAction));
  appendReceiptField(receipt, "input_action_accepted", window.lastInputAccepted);
  appendReceiptField(receipt, "gameplay_input_suppressed",
                     activeSurface.gameplayInputSuppressed);
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
  appendReceiptField(receipt, "product_vulkan_renderer_requested",
                     window.productVulkanRendererRequested);
  appendReceiptField(receipt, "product_vulkan_backend_built",
                     vulkanGameplayReadiness.backendBuilt);
  appendReceiptField(receipt, "product_vulkan_renderer_created",
                     window.productVulkanRendererCreated);
  appendReceiptField(receipt, "product_vulkan_renderer_ready",
                     window.productVulkanRendererReady);
  appendReceiptField(receipt, "product_vulkan_surface_created",
                     window.productVulkanSurfaceCreated);
  appendReceiptField(receipt, "product_vulkan_swapchain_ready",
                     window.productVulkanSwapchainReady);
  appendReceiptField(receipt, "product_vulkan_frame_submitted",
                     window.productVulkanFrameSubmitted);
  appendReceiptField(receipt, "product_vulkan_frame_submitted_count",
                     window.productVulkanFrameSubmittedCount);
  appendReceiptField(receipt, "product_vulkan_status", window.productVulkanStatus);
  appendReceiptField(receipt, "product_vulkan_reason_code",
                     window.productVulkanReasonCode);
  appendReceiptField(receipt, "product_vulkan_rendering_path",
                     window.productVulkanRenderingPath);
  appendReceiptField(receipt, "product_vulkan_record_mode",
                     window.productVulkanRecordMode);
  appendReceiptField(receipt, "product_vulkan_menu_requested",
                     window.productVulkanMenuRequested);
  appendReceiptField(receipt, "product_vulkan_menu_visible",
                     window.productVulkanMenuVisible);
  appendReceiptField(receipt, "product_vulkan_menu_status",
                     window.productVulkanMenuStatus);
  appendReceiptField(receipt, "product_vulkan_menu_reason_code",
                     window.productVulkanMenuReasonCode);
  appendReceiptField(receipt, "product_vulkan_menu_surface",
                     window.productVulkanMenuSurface);
  appendReceiptField(receipt, "product_vulkan_menu_ui_ready",
                     window.productVulkanMenuUiReady);
  appendReceiptField(receipt, "product_vulkan_menu_ui_partial",
                     window.productVulkanMenuUiPartial);
  appendReceiptField(receipt, "product_vulkan_menu_ui_status",
                     window.productVulkanMenuUiStatus);
  appendReceiptField(receipt, "product_vulkan_menu_ui_reason_code",
                     window.productVulkanMenuUiReasonCode);
  appendReceiptField(receipt, "product_vulkan_menu_ui_primitive_count",
                     window.productVulkanMenuUiPrimitiveCount);
  appendReceiptField(receipt, "product_vulkan_menu_ui_text_count",
                     window.productVulkanMenuUiTextCount);
  appendReceiptField(receipt, "product_vulkan_menu_ui_rect_count",
                     window.productVulkanMenuUiRectCount);
  appendReceiptField(receipt, "product_vulkan_menu_ui_row_count",
                     window.productVulkanMenuUiRowCount);
  appendReceiptField(receipt, "product_vulkan_menu_ui_selected_action",
                     window.productVulkanMenuUiSelectedAction);
  appendReceiptField(receipt, "creative_ui_projection_requested",
                     window.creativeUiProjectionRequested);
  appendReceiptField(receipt, "creative_ui_projection_ready",
                     window.creativeUiProjectionReady);
  appendReceiptField(receipt, "creative_ui_projection_partial",
                     window.creativeUiProjectionPartial);
  appendReceiptField(receipt, "creative_ui_projection_status",
                     window.creativeUiProjectionStatus);
  appendReceiptField(receipt, "creative_ui_projection_reason_code",
                     window.creativeUiProjectionReasonCode);
  appendReceiptField(receipt, "creative_ui_projection_used_model",
                     window.creativeUiProjectionUsedModel);
  appendReceiptField(receipt, "creative_ui_projection_used_facade",
                     window.creativeUiProjectionUsedFacade);
  appendReceiptField(receipt, "creative_ui_projection_virtual_width",
                     static_cast<std::uint64_t>(
                         window.creativeUiProjectionVirtualWidth));
  appendReceiptField(receipt, "creative_ui_projection_virtual_height",
                     static_cast<std::uint64_t>(
                         window.creativeUiProjectionVirtualHeight));
  appendReceiptField(receipt, "creative_ui_projection_theme",
                     window.creativeUiProjectionTheme);
  appendReceiptField(receipt, "creative_ui_projection_panel_count",
                     window.creativeUiProjectionPanelCount);
  appendReceiptField(receipt, "creative_ui_projection_model_row_count",
                     window.creativeUiProjectionModelRowCount);
  appendReceiptField(receipt, "creative_ui_projection_primitive_count",
                     window.creativeUiProjectionPrimitiveCount);
  appendReceiptField(receipt, "creative_ui_projection_text_count",
                     window.creativeUiProjectionTextCount);
  appendReceiptField(receipt, "creative_ui_projection_rect_count",
                     window.creativeUiProjectionRectCount);
  appendReceiptField(receipt, "creative_ui_projection_row_count",
                     window.creativeUiProjectionRowCount);
  appendReceiptField(receipt, "creative_ui_projection_disabled_row_count",
                     window.creativeUiProjectionDisabledRowCount);
  appendReceiptField(receipt, "creative_ui_projection_hit_region_count",
                     window.creativeUiProjectionHitRegionCount);
  appendReceiptField(receipt, "creative_ui_input_requested",
                     window.creativeUiInputRequested);
  appendReceiptField(receipt, "creative_ui_input_click_present",
                     window.creativeUiInputClickPresent);
  appendReceiptField(receipt, "creative_ui_input_draw_list_available",
                     window.creativeUiInputDrawListAvailable);
  appendReceiptField(receipt, "creative_ui_input_routed",
                     window.creativeUiInputRouted);
  appendReceiptField(receipt, "creative_ui_input_hit",
                     window.creativeUiInputHit);
  appendReceiptField(receipt, "creative_ui_input_consumed",
                     window.creativeUiInputConsumed);
  appendReceiptField(receipt, "creative_ui_input_enabled",
                     window.creativeUiInputEnabled);
  appendReceiptField(receipt, "creative_ui_input_surface",
                     window.creativeUiInputSurface);
  appendReceiptField(receipt, "creative_ui_input_kind",
                     window.creativeUiInputKind);
  appendReceiptField(receipt, "creative_ui_input_action",
                     window.creativeUiInputAction);
  appendReceiptField(receipt, "creative_ui_input_layer_index",
                     window.creativeUiInputLayerIndex);
  appendReceiptField(receipt, "creative_ui_input_region_index",
                     window.creativeUiInputRegionIndex);
  appendReceiptField(receipt, "creative_ui_input_semantic_id",
                     window.creativeUiInputSemanticId);
  appendReceiptField(receipt, "creative_ui_input_status",
                     window.creativeUiInputStatus);
  appendReceiptField(receipt, "creative_ui_input_reason_code",
                     window.creativeUiInputReasonCode);
  appendReceiptField(receipt, "product_vulkan_gameplay_ready",
                     vulkanGameplayReadiness.ready);
  appendReceiptField(receipt, "product_vulkan_gameplay_status",
                     vulkanGameplayReadiness.status);
  appendReceiptField(receipt, "product_vulkan_gameplay_reason_code",
                     vulkanGameplayReadiness.reasonCode);
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
