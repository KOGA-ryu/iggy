#include "app/iggy3d/receipt/ReceiptFields.hpp"

namespace iggy3d {

void appendProductWorldAuthoringFields(RenderReceipt& receipt, const ProductAppWindowState& window) {
  appendReceiptField(receipt, "world_setup_title", window.worldSetup.title);
  appendReceiptField(receipt, "world_setup_status", window.worldSetup.status);
  appendReceiptField(receipt,
                     "world_setup_dungeon_title",
                     window.worldSetup.dungeonTitle);
  appendReceiptField(receipt,
                     "world_setup_dungeon_index",
                     window.worldSetup.dungeonIndex);
  appendReceiptField(receipt,
                     "world_setup_dungeon_count",
                     window.worldSetup.dungeonCount);
  appendReceiptField(receipt,
                     "world_setup_ascii_room_enabled",
                     window.worldSetup.asciiRoomEnabled);
  appendReceiptField(receipt,
                     "world_setup_ascii_room_text_present",
                     window.worldSetup.asciiRoomTextPresent);
  appendReceiptField(receipt,
                     "world_setup_ascii_room_id",
                     window.worldSetup.asciiRoomId);
  appendReceiptField(receipt,
                     "world_setup_ascii_room_source_name",
                     window.worldSetup.asciiRoomSourceName);
  appendReceiptField(receipt,
                     "world_setup_dungeon_draft_edit_mode",
                     window.worldSetup.dungeonDraftEditMode);
  appendReceiptField(receipt,
                     "world_setup_dungeon_draft_modified",
                     window.worldSetup.dungeonDraftModified);
  appendReceiptField(receipt,
                     "world_setup_dungeon_draft_cursor_row",
                     window.worldSetup.dungeonDraftCursorRow);
  appendReceiptField(receipt,
                     "world_setup_dungeon_draft_cursor_column",
                     window.worldSetup.dungeonDraftCursorColumn);
  appendReceiptField(receipt,
                     "world_setup_dungeon_draft_status",
                     window.worldSetup.dungeonDraftStatus);
  appendReceiptField(receipt,
                     "world_setup_dungeon_draft_reason_code",
                     window.worldSetup.dungeonDraftReasonCode);
  appendReceiptField(receipt,
                     "world_setup_dungeon_draft_selected_glyph",
                     window.worldSetup.dungeonDraftSelectedGlyph);
  appendReceiptField(receipt,
                     "world_setup_dungeon_draft_last_glyph",
                     window.worldSetup.dungeonDraftLastGlyph);
  appendReceiptField(receipt, "world_creation_status", window.worldCreation.status);
  appendReceiptField(receipt, "world_creation_reason_code",
                     window.worldCreation.reasonCode);
  appendReceiptField(receipt, "world_creation_world_id", window.worldCreation.worldId);
  appendReceiptField(receipt, "world_creation_world_title",
                     window.worldCreation.worldTitle);
  appendReceiptField(receipt,
                     "world_creation_ascii_room_requested",
                     window.worldCreation.asciiRoomRequested);
  appendReceiptField(receipt,
                     "world_creation_ascii_room_id",
                     window.worldCreation.asciiRoomId);
  appendReceiptField(receipt,
                     "world_creation_ascii_room_source_name",
                     window.worldCreation.asciiRoomSourceName);
  appendReceiptField(receipt, "world_creation_initial_save_requested",
                     window.worldCreation.initialSaveRequested);
  appendReceiptField(receipt, "world_creation_initial_save_written",
                     window.worldCreation.initialSaveWritten);
  appendReceiptField(receipt, "world_creation_initial_save_id",
                     window.worldCreation.initialSaveId);
  appendReceiptField(receipt, "world_creation_initial_save_title",
                     window.worldCreation.initialSaveTitle);
  appendReceiptField(receipt, "world_creation_route_after_create",
                     window.worldCreation.routeAfterCreate);
  appendReceiptField(receipt, "ascii_room_preview_status",
                     window.asciiRoomPreview.status);
  appendReceiptField(receipt, "ascii_room_preview_reason_code",
                     window.asciiRoomPreview.reasonCode);
  appendReceiptField(receipt, "ascii_room_preview_failed_stage",
                     window.asciiRoomPreview.failedStage);
  appendReceiptField(receipt, "ascii_room_preview_room_id",
                     window.asciiRoomPreview.roomId);
  appendReceiptField(receipt, "ascii_room_preview_source_name",
                     window.asciiRoomPreview.sourceName);
  appendReceiptField(receipt, "ascii_room_preview_ready",
                     window.asciiRoomPreview.ready);
  appendReceiptField(receipt, "ascii_room_preview_width",
                     window.asciiRoomPreview.width);
  appendReceiptField(receipt, "ascii_room_preview_height",
                     window.asciiRoomPreview.height);
  appendReceiptField(receipt, "ascii_room_preview_floor_count",
                     window.asciiRoomPreview.floorCount);
  appendReceiptField(receipt, "ascii_room_preview_wall_count",
                     window.asciiRoomPreview.wallCount);
  appendReceiptField(receipt, "ascii_room_preview_object_count",
                     window.asciiRoomPreview.objectCount);
  appendReceiptField(receipt, "ascii_room_preview_marker_count",
                     window.asciiRoomPreview.markerCount);
  appendReceiptField(receipt, "ascii_room_preview_elevated_floor_count",
                     window.asciiRoomPreview.elevatedFloorCount);
  appendReceiptField(receipt, "ascii_room_preview_ramp_count",
                     window.asciiRoomPreview.rampCount);
  appendReceiptField(receipt, "ascii_room_preview_blocked_slope_count",
                     window.asciiRoomPreview.blockedSlopeCount);
  appendReceiptField(receipt, "ascii_room_preview_static_mesh_count",
                     window.asciiRoomPreview.staticMeshCount);
  appendReceiptField(receipt, "ascii_room_preview_anchor_count",
                     window.asciiRoomPreview.anchorCount);
  appendReceiptField(receipt, "ascii_room_preview_spatial_surface_count",
                     window.asciiRoomPreview.spatialSurfaceCount);
  appendReceiptField(receipt, "ascii_room_preview_asset_text_written",
                     window.asciiRoomPreview.assetTextWritten);
  appendReceiptField(receipt, "ascii_room_preview_asset_text_bytes",
                     window.asciiRoomPreview.assetTextBytes);
  appendReceiptField(receipt, "ascii_room_activation_status",
                     window.asciiRoomActivation.status);
  appendReceiptField(receipt, "ascii_room_activation_reason_code",
                     window.asciiRoomActivation.reasonCode);
  appendReceiptField(receipt, "ascii_room_activation_room_id",
                     window.asciiRoomActivation.roomId);
  appendReceiptField(receipt, "ascii_room_activation_package_id",
                     window.asciiRoomActivation.packageId);
  appendReceiptField(receipt, "ascii_room_activation_scenario_id",
                     window.asciiRoomActivation.scenarioId);
  appendReceiptField(receipt, "ascii_room_activation_session_created",
                     window.asciiRoomActivation.sessionCreated);
  appendReceiptField(receipt, "ascii_room_activation_player_spawned",
                     window.asciiRoomActivation.playerSpawned);
  appendReceiptField(receipt, "ascii_room_activation_player_count",
                     window.asciiRoomActivation.playerCount);
  appendReceiptField(receipt, "ascii_room_activation_entity_count",
                     window.asciiRoomActivation.entityCount);
  appendReceiptField(receipt, "ascii_room_activation_npc_count",
                     window.asciiRoomActivation.npcCount);
  appendReceiptField(receipt, "ascii_room_activation_pickup_count",
                     window.asciiRoomActivation.pickupCount);
  appendReceiptField(receipt, "ascii_room_activation_door_count",
                     window.asciiRoomActivation.doorCount);
  appendReceiptField(receipt, "ascii_room_activation_marker_entity_count",
                     window.asciiRoomActivation.markerEntityCount);
  appendReceiptField(receipt, "ascii_room_activation_objective_count",
                     window.asciiRoomActivation.objectiveCount);
  appendReceiptField(receipt, "ascii_room_activation_wall_count",
                     window.asciiRoomActivation.wallCount);
  appendReceiptField(receipt, "ascii_room_activation_marker_count",
                     window.asciiRoomActivation.markerCount);
  appendReceiptField(receipt, "ascii_room_activation_runtime_hash",
                     window.asciiRoomActivation.runtimeHash);
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
}

}  // namespace iggy3d
