#include "app/iggy3d/receipt/ReceiptFields.hpp"

#include "app/iggy3d/ProductAppWindowState.hpp"

namespace iggy3d {

void appendProductWorldAuthoringFields(RenderReceipt& receipt, const ProductAppWindowState& window) {
  appendReceiptField(receipt, "world_setup_title", window.creativeAuthoring.worldSetup.title);
  appendReceiptField(receipt, "world_setup_status", window.creativeAuthoring.worldSetup.status);
  appendReceiptField(receipt,
                     "world_setup_dungeon_title",
                     window.creativeAuthoring.worldSetup.dungeonTitle);
  appendReceiptField(receipt,
                     "world_setup_dungeon_index",
                     window.creativeAuthoring.worldSetup.dungeonIndex);
  appendReceiptField(receipt,
                     "world_setup_dungeon_count",
                     window.creativeAuthoring.worldSetup.dungeonCount);
  appendReceiptField(receipt,
                     "world_setup_ascii_room_enabled",
                     window.creativeAuthoring.worldSetup.asciiRoomEnabled);
  appendReceiptField(receipt,
                     "world_setup_ascii_room_text_present",
                     window.creativeAuthoring.worldSetup.asciiRoomTextPresent);
  appendReceiptField(receipt,
                     "world_setup_ascii_room_id",
                     window.creativeAuthoring.worldSetup.asciiRoomId);
  appendReceiptField(receipt,
                     "world_setup_ascii_room_source_name",
                     window.creativeAuthoring.worldSetup.asciiRoomSourceName);
  appendReceiptField(receipt,
                     "world_setup_dungeon_draft_edit_mode",
                     window.creativeAuthoring.worldSetup.dungeonDraftEditMode);
  appendReceiptField(receipt,
                     "world_setup_dungeon_draft_modified",
                     window.creativeAuthoring.worldSetup.dungeonDraftModified);
  appendReceiptField(receipt,
                     "world_setup_dungeon_draft_cursor_row",
                     window.creativeAuthoring.worldSetup.dungeonDraftCursorRow);
  appendReceiptField(receipt,
                     "world_setup_dungeon_draft_cursor_column",
                     window.creativeAuthoring.worldSetup.dungeonDraftCursorColumn);
  appendReceiptField(receipt,
                     "world_setup_dungeon_draft_status",
                     window.creativeAuthoring.worldSetup.dungeonDraftStatus);
  appendReceiptField(receipt,
                     "world_setup_dungeon_draft_reason_code",
                     window.creativeAuthoring.worldSetup.dungeonDraftReasonCode);
  appendReceiptField(receipt,
                     "world_setup_dungeon_draft_selected_glyph",
                     window.creativeAuthoring.worldSetup.dungeonDraftSelectedGlyph);
  appendReceiptField(receipt,
                     "world_setup_dungeon_draft_last_glyph",
                     window.creativeAuthoring.worldSetup.dungeonDraftLastGlyph);
  appendReceiptField(receipt, "world_creation_status", window.creativeAuthoring.worldCreation.status);
  appendReceiptField(receipt, "world_creation_reason_code",
                     window.creativeAuthoring.worldCreation.reasonCode);
  appendReceiptField(receipt, "world_creation_world_id", window.creativeAuthoring.worldCreation.worldId);
  appendReceiptField(receipt, "world_creation_world_title",
                     window.creativeAuthoring.worldCreation.worldTitle);
  appendReceiptField(receipt,
                     "world_creation_ascii_room_requested",
                     window.creativeAuthoring.worldCreation.asciiRoomRequested);
  appendReceiptField(receipt,
                     "world_creation_ascii_room_id",
                     window.creativeAuthoring.worldCreation.asciiRoomId);
  appendReceiptField(receipt,
                     "world_creation_ascii_room_source_name",
                     window.creativeAuthoring.worldCreation.asciiRoomSourceName);
  appendReceiptField(receipt, "world_creation_initial_save_requested",
                     window.creativeAuthoring.worldCreation.initialSaveRequested);
  appendReceiptField(receipt, "world_creation_initial_save_written",
                     window.creativeAuthoring.worldCreation.initialSaveWritten);
  appendReceiptField(receipt, "world_creation_initial_save_id",
                     window.creativeAuthoring.worldCreation.initialSaveId);
  appendReceiptField(receipt, "world_creation_initial_save_title",
                     window.creativeAuthoring.worldCreation.initialSaveTitle);
  appendReceiptField(receipt, "world_creation_route_after_create",
                     window.creativeAuthoring.worldCreation.routeAfterCreate);
  appendReceiptField(receipt, "ascii_room_preview_status",
                     window.creativeAuthoring.asciiRoomPreview.status);
  appendReceiptField(receipt, "ascii_room_preview_reason_code",
                     window.creativeAuthoring.asciiRoomPreview.reasonCode);
  appendReceiptField(receipt, "ascii_room_preview_failed_stage",
                     window.creativeAuthoring.asciiRoomPreview.failedStage);
  appendReceiptField(receipt, "ascii_room_preview_room_id",
                     window.creativeAuthoring.asciiRoomPreview.roomId);
  appendReceiptField(receipt, "ascii_room_preview_source_name",
                     window.creativeAuthoring.asciiRoomPreview.sourceName);
  appendReceiptField(receipt, "ascii_room_preview_ready",
                     window.creativeAuthoring.asciiRoomPreview.ready);
  appendReceiptField(receipt, "ascii_room_preview_width",
                     window.creativeAuthoring.asciiRoomPreview.width);
  appendReceiptField(receipt, "ascii_room_preview_height",
                     window.creativeAuthoring.asciiRoomPreview.height);
  appendReceiptField(receipt, "ascii_room_preview_floor_count",
                     window.creativeAuthoring.asciiRoomPreview.floorCount);
  appendReceiptField(receipt, "ascii_room_preview_wall_count",
                     window.creativeAuthoring.asciiRoomPreview.wallCount);
  appendReceiptField(receipt, "ascii_room_preview_object_count",
                     window.creativeAuthoring.asciiRoomPreview.objectCount);
  appendReceiptField(receipt, "ascii_room_preview_marker_count",
                     window.creativeAuthoring.asciiRoomPreview.markerCount);
  appendReceiptField(receipt, "ascii_room_preview_elevated_floor_count",
                     window.creativeAuthoring.asciiRoomPreview.elevatedFloorCount);
  appendReceiptField(receipt, "ascii_room_preview_ramp_count",
                     window.creativeAuthoring.asciiRoomPreview.rampCount);
  appendReceiptField(receipt, "ascii_room_preview_blocked_slope_count",
                     window.creativeAuthoring.asciiRoomPreview.blockedSlopeCount);
  appendReceiptField(receipt, "ascii_room_preview_static_mesh_count",
                     window.creativeAuthoring.asciiRoomPreview.staticMeshCount);
  appendReceiptField(receipt, "ascii_room_preview_anchor_count",
                     window.creativeAuthoring.asciiRoomPreview.anchorCount);
  appendReceiptField(receipt, "ascii_room_preview_spatial_surface_count",
                     window.creativeAuthoring.asciiRoomPreview.spatialSurfaceCount);
  appendReceiptField(receipt, "ascii_room_preview_asset_text_written",
                     window.creativeAuthoring.asciiRoomPreview.assetTextWritten);
  appendReceiptField(receipt, "ascii_room_preview_asset_text_bytes",
                     window.creativeAuthoring.asciiRoomPreview.assetTextBytes);
  appendReceiptField(receipt, "ascii_room_activation_status",
                     window.creativeAuthoring.asciiRoomActivation.status);
  appendReceiptField(receipt, "ascii_room_activation_reason_code",
                     window.creativeAuthoring.asciiRoomActivation.reasonCode);
  appendReceiptField(receipt, "ascii_room_activation_room_id",
                     window.creativeAuthoring.asciiRoomActivation.roomId);
  appendReceiptField(receipt, "ascii_room_activation_package_id",
                     window.creativeAuthoring.asciiRoomActivation.packageId);
  appendReceiptField(receipt, "ascii_room_activation_scenario_id",
                     window.creativeAuthoring.asciiRoomActivation.scenarioId);
  appendReceiptField(receipt, "ascii_room_activation_session_created",
                     window.creativeAuthoring.asciiRoomActivation.sessionCreated);
  appendReceiptField(receipt, "ascii_room_activation_player_spawned",
                     window.creativeAuthoring.asciiRoomActivation.playerSpawned);
  appendReceiptField(receipt, "ascii_room_activation_player_count",
                     window.creativeAuthoring.asciiRoomActivation.playerCount);
  appendReceiptField(receipt, "ascii_room_activation_entity_count",
                     window.creativeAuthoring.asciiRoomActivation.entityCount);
  appendReceiptField(receipt, "ascii_room_activation_npc_count",
                     window.creativeAuthoring.asciiRoomActivation.npcCount);
  appendReceiptField(receipt, "ascii_room_activation_pickup_count",
                     window.creativeAuthoring.asciiRoomActivation.pickupCount);
  appendReceiptField(receipt, "ascii_room_activation_door_count",
                     window.creativeAuthoring.asciiRoomActivation.doorCount);
  appendReceiptField(receipt, "ascii_room_activation_marker_entity_count",
                     window.creativeAuthoring.asciiRoomActivation.markerEntityCount);
  appendReceiptField(receipt, "ascii_room_activation_objective_count",
                     window.creativeAuthoring.asciiRoomActivation.objectiveCount);
  appendReceiptField(receipt, "ascii_room_activation_wall_count",
                     window.creativeAuthoring.asciiRoomActivation.wallCount);
  appendReceiptField(receipt, "ascii_room_activation_marker_count",
                     window.creativeAuthoring.asciiRoomActivation.markerCount);
  appendReceiptField(receipt, "ascii_room_activation_runtime_hash",
                     window.creativeAuthoring.asciiRoomActivation.runtimeHash);
  appendReceiptField(receipt, "room_editing_ready", window.creativeAuthoring.roomEditing.ready);
  appendReceiptField(receipt, "room_editing_status", window.creativeAuthoring.roomEditing.status);
  appendReceiptField(receipt, "room_editing_reason_code",
                     window.creativeAuthoring.roomEditing.reasonCode);
  appendReceiptField(receipt, "room_editing_floor_count",
                     window.creativeAuthoring.roomEditing.documentFloorCount);
  appendReceiptField(receipt, "room_editing_wall_count",
                     window.creativeAuthoring.roomEditing.documentWallCount);
  appendReceiptField(receipt, "room_editing_object_count",
                     window.creativeAuthoring.roomEditing.documentObjectCount);
  appendReceiptField(receipt, "room_editing_active_room_loaded",
                     window.creativeAuthoring.roomEditing.activeRoom.loaded);
  appendReceiptField(receipt, "room_editing_active_room_static_mesh_count",
                     window.creativeAuthoring.roomEditing.activeRoomStaticMeshCount);
  appendReceiptField(receipt, "room_editing_active_room_spatial_surface_count",
                     window.creativeAuthoring.roomEditing.activeRoomSpatialSurfaceCount);
  appendReceiptField(receipt, "room_editing_authored_floor_count",
                     window.creativeAuthoring.roomEditing.activeRoomAuthoredFloorCount);
  appendReceiptField(receipt, "room_editing_authored_wall_count",
                     window.creativeAuthoring.roomEditing.activeRoomAuthoredWallCount);
  appendReceiptField(receipt, "room_editing_authored_object_count",
                     window.creativeAuthoring.roomEditing.activeRoomAuthoredObjectCount);
  appendReceiptField(receipt, "room_editing_collision_ready",
                     window.creativeAuthoring.roomEditing.activeRoomCollision.ready);
  appendReceiptField(receipt, "room_editing_collision_surface_count",
                     window.creativeAuthoring.roomEditing.collisionQuerySurfaceCount);
  appendReceiptField(receipt, "room_editing_collision_walkable_surface_count",
                     window.creativeAuthoring.roomEditing.collisionWalkableSurfaceCount);
  appendReceiptField(receipt, "room_editing_collision_actor_blocker_count",
                     window.creativeAuthoring.roomEditing.collisionActorBlockerSurfaceCount);
  appendReceiptField(receipt, "room_editing_collision_projectile_blocker_count",
                     window.creativeAuthoring.roomEditing.collisionProjectileBlockerSurfaceCount);
  appendReceiptField(receipt, "room_editing_undo_depth",
                     window.creativeAuthoring.roomEditing.undoDepth);
  appendReceiptField(receipt, "room_editing_redo_depth",
                     window.creativeAuthoring.roomEditing.redoDepth);
  appendReceiptField(receipt, "room_editing_last_operation",
                     window.creativeAuthoring.roomEditingLastOperation);
  appendReceiptField(receipt, "room_editing_last_operation_status",
                     window.creativeAuthoring.roomEditingLastOperationStatus);
  appendReceiptField(receipt, "room_editing_last_operation_reason_code",
                     window.creativeAuthoring.roomEditingLastOperationReasonCode);
  appendReceiptField(receipt, "room_editing_last_input_source",
                     window.creativeAuthoring.roomEditingLastInputSource);
  appendReceiptField(receipt, "room_editing_last_operation_accepted",
                     window.creativeAuthoring.roomEditingLastOperationAccepted);
  appendReceiptField(receipt, "room_editing_last_primitive_id",
                     window.creativeAuthoring.roomEditingLastPrimitiveId);
}

}  // namespace iggy3d
