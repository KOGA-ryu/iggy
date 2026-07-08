#include "app/iggy3d/receipt/ReceiptFields.hpp"
#include <array>
#include <string_view>

#include "app/iggy3d/ProductAppWindowState.hpp"

namespace iggy3d {

namespace {

struct WorldAuthoringReceiptFieldRow {
  std::string_view key;
  void (*append)(RenderReceipt& receipt,
                 const ProductAppWindowState& window,
                 std::string_view key);
};

const std::array<WorldAuthoringReceiptFieldRow, 91>
    kWorldAuthoringReceiptFields{{
        {"world_setup_title",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.worldSetup.title);
         }},
        {"world_setup_status",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.worldSetup.status);
         }},
        {"world_setup_dungeon_title",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.worldSetup.dungeonTitle);
         }},
        {"world_setup_dungeon_index",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.worldSetup.dungeonIndex);
         }},
        {"world_setup_dungeon_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.worldSetup.dungeonCount);
         }},
        {"world_setup_ascii_room_enabled",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.worldSetup.asciiRoomEnabled);
         }},
        {"world_setup_ascii_room_text_present",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.worldSetup.asciiRoomTextPresent);
         }},
        {"world_setup_ascii_room_id",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.worldSetup.asciiRoomId);
         }},
        {"world_setup_ascii_room_source_name",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.worldSetup.asciiRoomSourceName);
         }},
        {"world_setup_dungeon_draft_edit_mode",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.worldSetup.dungeonDraftEditMode);
         }},
        {"world_setup_dungeon_draft_modified",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.worldSetup.dungeonDraftModified);
         }},
        {"world_setup_dungeon_draft_cursor_row",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.worldSetup.dungeonDraftCursorRow);
         }},
        {"world_setup_dungeon_draft_cursor_column",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.worldSetup.dungeonDraftCursorColumn);
         }},
        {"world_setup_dungeon_draft_status",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.worldSetup.dungeonDraftStatus);
         }},
        {"world_setup_dungeon_draft_reason_code",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.worldSetup.dungeonDraftReasonCode);
         }},
        {"world_setup_dungeon_draft_selected_glyph",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.worldSetup.dungeonDraftSelectedGlyph);
         }},
        {"world_setup_dungeon_draft_last_glyph",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.worldSetup.dungeonDraftLastGlyph);
         }},
        {"world_creation_status",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.worldCreation.status);
         }},
        {"world_creation_reason_code",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.worldCreation.reasonCode);
         }},
        {"world_creation_world_id",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.worldCreation.worldId);
         }},
        {"world_creation_world_title",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.worldCreation.worldTitle);
         }},
        {"world_creation_ascii_room_requested",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.worldCreation.asciiRoomRequested);
         }},
        {"world_creation_ascii_room_id",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.worldCreation.asciiRoomId);
         }},
        {"world_creation_ascii_room_source_name",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.worldCreation.asciiRoomSourceName);
         }},
        {"world_creation_initial_save_requested",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.worldCreation.initialSaveRequested);
         }},
        {"world_creation_initial_save_written",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.worldCreation.initialSaveWritten);
         }},
        {"world_creation_initial_save_id",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.worldCreation.initialSaveId);
         }},
        {"world_creation_initial_save_title",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.worldCreation.initialSaveTitle);
         }},
        {"world_creation_route_after_create",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.worldCreation.routeAfterCreate);
         }},
        {"ascii_room_preview_status",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomPreview.status);
         }},
        {"ascii_room_preview_reason_code",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomPreview.reasonCode);
         }},
        {"ascii_room_preview_failed_stage",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomPreview.failedStage);
         }},
        {"ascii_room_preview_room_id",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomPreview.roomId);
         }},
        {"ascii_room_preview_source_name",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomPreview.sourceName);
         }},
        {"ascii_room_preview_ready",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomPreview.ready);
         }},
        {"ascii_room_preview_width",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomPreview.width);
         }},
        {"ascii_room_preview_height",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomPreview.height);
         }},
        {"ascii_room_preview_floor_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomPreview.floorCount);
         }},
        {"ascii_room_preview_wall_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomPreview.wallCount);
         }},
        {"ascii_room_preview_object_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomPreview.objectCount);
         }},
        {"ascii_room_preview_marker_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomPreview.markerCount);
         }},
        {"ascii_room_preview_elevated_floor_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomPreview.elevatedFloorCount);
         }},
        {"ascii_room_preview_ramp_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomPreview.rampCount);
         }},
        {"ascii_room_preview_blocked_slope_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomPreview.blockedSlopeCount);
         }},
        {"ascii_room_preview_static_mesh_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomPreview.staticMeshCount);
         }},
        {"ascii_room_preview_anchor_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomPreview.anchorCount);
         }},
        {"ascii_room_preview_spatial_surface_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomPreview.spatialSurfaceCount);
         }},
        {"ascii_room_preview_asset_text_written",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomPreview.assetTextWritten);
         }},
        {"ascii_room_preview_asset_text_bytes",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomPreview.assetTextBytes);
         }},
        {"ascii_room_activation_status",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomActivation.status);
         }},
        {"ascii_room_activation_reason_code",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomActivation.reasonCode);
         }},
        {"ascii_room_activation_room_id",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomActivation.roomId);
         }},
        {"ascii_room_activation_package_id",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomActivation.packageId);
         }},
        {"ascii_room_activation_scenario_id",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomActivation.scenarioId);
         }},
        {"ascii_room_activation_session_created",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomActivation.sessionCreated);
         }},
        {"ascii_room_activation_player_spawned",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomActivation.playerSpawned);
         }},
        {"ascii_room_activation_player_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomActivation.playerCount);
         }},
        {"ascii_room_activation_entity_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomActivation.entityCount);
         }},
        {"ascii_room_activation_npc_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomActivation.npcCount);
         }},
        {"ascii_room_activation_pickup_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomActivation.pickupCount);
         }},
        {"ascii_room_activation_door_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomActivation.doorCount);
         }},
        {"ascii_room_activation_marker_entity_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomActivation.markerEntityCount);
         }},
        {"ascii_room_activation_objective_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomActivation.objectiveCount);
         }},
        {"ascii_room_activation_wall_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomActivation.wallCount);
         }},
        {"ascii_room_activation_marker_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomActivation.markerCount);
         }},
        {"ascii_room_activation_runtime_hash",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.asciiRoomActivation.runtimeHash);
         }},
        {"room_editing_ready",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.roomEditing.ready);
         }},
        {"room_editing_status",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.roomEditing.status);
         }},
        {"room_editing_reason_code",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.roomEditing.reasonCode);
         }},
        {"room_editing_floor_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.roomEditing.documentFloorCount);
         }},
        {"room_editing_wall_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.roomEditing.documentWallCount);
         }},
        {"room_editing_object_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.roomEditing.documentObjectCount);
         }},
        {"room_editing_active_room_loaded",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.roomEditing.activeRoom.loaded);
         }},
        {"room_editing_active_room_static_mesh_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.roomEditing.activeRoomStaticMeshCount);
         }},
        {"room_editing_active_room_spatial_surface_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.roomEditing.activeRoomSpatialSurfaceCount);
         }},
        {"room_editing_authored_floor_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.roomEditing.activeRoomAuthoredFloorCount);
         }},
        {"room_editing_authored_wall_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.roomEditing.activeRoomAuthoredWallCount);
         }},
        {"room_editing_authored_object_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.roomEditing.activeRoomAuthoredObjectCount);
         }},
        {"room_editing_collision_ready",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.roomEditing.activeRoomCollision.ready);
         }},
        {"room_editing_collision_surface_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.roomEditing.collisionQuerySurfaceCount);
         }},
        {"room_editing_collision_walkable_surface_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.roomEditing.collisionWalkableSurfaceCount);
         }},
        {"room_editing_collision_actor_blocker_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.roomEditing.collisionActorBlockerSurfaceCount);
         }},
        {"room_editing_collision_projectile_blocker_count",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.roomEditing.collisionProjectileBlockerSurfaceCount);
         }},
        {"room_editing_undo_depth",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.roomEditing.undoDepth);
         }},
        {"room_editing_redo_depth",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.roomEditing.redoDepth);
         }},
        {"room_editing_last_operation",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.roomEditingLastOperation);
         }},
        {"room_editing_last_operation_status",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.roomEditingLastOperationStatus);
         }},
        {"room_editing_last_operation_reason_code",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.roomEditingLastOperationReasonCode);
         }},
        {"room_editing_last_input_source",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.roomEditingLastInputSource);
         }},
        {"room_editing_last_operation_accepted",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.roomEditingLastOperationAccepted);
         }},
        {"room_editing_last_primitive_id",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key, window.creativeAuthoring.roomEditingLastPrimitiveId);
         }},
    }};

}  // namespace

void appendProductWorldAuthoringFields(RenderReceipt& receipt, const ProductAppWindowState& window) {
  for (const WorldAuthoringReceiptFieldRow& row :
       kWorldAuthoringReceiptFields) {
    row.append(receipt, window, row.key);
  }
}

}  // namespace iggy3d
