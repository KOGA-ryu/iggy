#include "app/iggy3d/ReceiptBuilder.hpp"

#include <charconv>
#include <string>

#include "app/frontend/FrontendReceipt.hpp"
#include "app/iggy3d/ProductGameplayFeedback.hpp"
#include "app/iggy3d/ProductMovementDebugHud.hpp"
#include "app/iggy3d/ProductNpcBehaviorDebugHud.hpp"

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
  const ProductMovementDebugHud movementHud =
      buildProductMovementDebugHud(window, settings.devToolsEnabled,
                                   settings.debugOverlayEnabled);
  const ProductNpcBehaviorDebugHud npcBehaviorHud{
      window.npcBehaviorDebugHudVisible,
      settings.devToolsEnabled,
      settings.debugOverlayEnabled,
      window.npcBehaviorDebugHudDebugAvailable,
      static_cast<std::size_t>(window.npcBehaviorDebugHudLineCount),
      window.npcBehaviorDebugHudStatus,
      window.npcBehaviorDebugHudReasonCode,
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
  appendReceiptField(receipt,
                     "product_save_load_authored_room_present",
                     window.productSaveLoadAuthoredRoomPresent);
  appendReceiptField(receipt,
                     "product_save_load_authored_room_id",
                     window.productSaveLoadAuthoredRoomId);
  appendReceiptField(receipt,
                     "product_save_load_authored_floor_count",
                     window.productSaveLoadAuthoredFloorCount);
  appendReceiptField(receipt,
                     "product_save_load_authored_wall_count",
                     window.productSaveLoadAuthoredWallCount);
  appendReceiptField(receipt,
                     "product_save_load_authored_marker_count",
                     window.productSaveLoadAuthoredMarkerCount);
  appendReceiptField(receipt, "saved_marker_bind_status",
                     window.savedMarkerBindStatus);
  appendReceiptField(receipt, "saved_marker_bind_reason_code",
                     window.savedMarkerBindReasonCode);
  appendReceiptField(receipt, "saved_marker_bind_requested",
                     window.savedMarkerBindRequested);
  appendReceiptField(receipt, "saved_marker_bind_session_replaced",
                     window.savedMarkerBindSessionReplaced);
  appendReceiptField(receipt, "saved_marker_bind_room_id",
                     window.savedMarkerBindRoomId);
  appendReceiptField(receipt, "saved_marker_bind_marker_count",
                     window.savedMarkerBindMarkerCount);
  appendReceiptField(receipt, "saved_marker_bind_seed_entity_count",
                     window.savedMarkerBindSeedEntityCount);
  appendReceiptField(receipt, "saved_marker_bind_added_entity_count",
                     window.savedMarkerBindAddedEntityCount);
  appendReceiptField(receipt, "saved_marker_bind_existing_entity_count",
                     window.savedMarkerBindExistingEntityCount);
  appendReceiptField(receipt, "saved_marker_bind_added_objective_count",
                     window.savedMarkerBindAddedObjectiveCount);
  appendReceiptField(receipt, "saved_marker_bind_existing_objective_count",
                     window.savedMarkerBindExistingObjectiveCount);
  appendReceiptField(receipt, "saved_marker_bind_added_combatant_count",
                     window.savedMarkerBindAddedCombatantCount);
  appendReceiptField(receipt, "saved_marker_bind_existing_combatant_count",
                     window.savedMarkerBindExistingCombatantCount);
  appendReceiptField(receipt, "saved_marker_bind_pickup_count",
                     window.savedMarkerBindPickupCount);
  appendReceiptField(receipt, "saved_marker_bind_door_count",
                     window.savedMarkerBindDoorCount);
  appendReceiptField(receipt, "saved_marker_bind_marker_entity_count",
                     window.savedMarkerBindMarkerEntityCount);
  appendReceiptField(receipt, "saved_marker_bind_npc_count",
                     window.savedMarkerBindNpcCount);
  appendReceiptField(receipt, "saved_marker_bind_previous_hash",
                     window.savedMarkerBindPreviousHash);
  appendReceiptField(receipt, "saved_marker_bind_bound_hash",
                     window.savedMarkerBindBoundHash);
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
  appendReceiptField(receipt, "gameplay_tick_reason_code",
                     window.gameplayTickReasonCode);
  appendReceiptField(receipt, "player_position_changed", window.playerPositionChanged);
  appendReceiptField(receipt, "gameplay_movement_attempted",
                     window.gameplayMovementAttempted);
  appendReceiptField(receipt, "gameplay_movement_blocked",
                     window.gameplayMovementBlocked);
  appendReceiptField(receipt, "gameplay_movement_status",
                     window.gameplayMovementStatus);
  appendReceiptField(receipt, "gameplay_movement_debug_available",
                     window.gameplayMovementDebugAvailable);
  appendReceiptField(receipt, "gameplay_movement_reason_code",
                     window.gameplayMovementReasonCode);
  appendReceiptField(receipt, "gameplay_movement_blocked_reason",
                     window.gameplayMovementBlockedReason);
  appendReceiptField(receipt, "gameplay_movement_hit_surface_id",
                     window.gameplayMovementHitSurfaceId);
  appendReceiptField(receipt, "gameplay_movement_ground_snap_applied",
                     window.gameplayMovementGroundSnapApplied);
  appendReceiptField(receipt, "gameplay_movement_clamped",
                     window.gameplayMovementClamped);
  appendReceiptField(receipt, "gameplay_movement_slid",
                     window.gameplayMovementSlid);
  appendReceiptField(receipt, "gameplay_movement_collision_sweep_count",
                     window.gameplayMovementCollisionSweepCount);
  appendReceiptField(receipt, "gameplay_movement_policy_band",
                     window.gameplayMovementPolicyBand);
  appendReceiptField(receipt, "gameplay_movement_slope_travel_direction",
                     window.gameplayMovementSlopeTravelDirection);
  appendReceiptField(receipt, "gameplay_movement_slope_angle_degrees",
                     floatReceiptValue(window.gameplayMovementSlopeAngleDegrees));
  appendReceiptField(receipt, "gameplay_movement_speed_multiplier",
                     floatReceiptValue(window.gameplayMovementSpeedMultiplier));
  appendReceiptField(receipt, "gameplay_movement_start_x",
                     floatReceiptValue(window.gameplayMovementStartX));
  appendReceiptField(receipt, "gameplay_movement_start_y",
                     floatReceiptValue(window.gameplayMovementStartY));
  appendReceiptField(receipt, "gameplay_movement_start_z",
                     floatReceiptValue(window.gameplayMovementStartZ));
  appendReceiptField(receipt, "gameplay_movement_final_x",
                     floatReceiptValue(window.gameplayMovementFinalX));
  appendReceiptField(receipt, "gameplay_movement_final_y",
                     floatReceiptValue(window.gameplayMovementFinalY));
  appendReceiptField(receipt, "gameplay_movement_final_z",
                     floatReceiptValue(window.gameplayMovementFinalZ));
  appendReceiptField(receipt, "gameplay_movement_horizontal_distance_meters",
                     floatReceiptValue(window.gameplayMovementHorizontalDistanceMeters));
  appendReceiptField(receipt, "gameplay_movement_vertical_delta_meters",
                     floatReceiptValue(window.gameplayMovementVerticalDeltaMeters));
  appendReceiptField(receipt, "gameplay_movement_grade_percent",
                     floatReceiptValue(window.gameplayMovementGradePercent));
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
  appendReceiptField(receipt, "gameplay_collision_surfaces_used",
                     window.gameplayCollisionSurfacesUsed);
  appendReceiptField(receipt, "gameplay_collision_surface_count",
                     window.gameplayCollisionSurfaceCount);
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
  appendReceiptField(receipt, "product_vulkan_renderer_requested",
                     window.productVulkanRendererRequested);
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
