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
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/window/RendererLifecycle.hpp"
#include "app/iggy3d/creative/render/WireframeDebugLines.hpp"

namespace iggy3d {

void appendProductSaveStateFields(RenderReceipt& receipt, const FrontendState& frontend, const ProductAppWindowState& window, const creative::CreativeActiveIdentity& creativeIdentity) {
  appendReceiptField(receipt, "product_save_status", window.saveSession.productSaveStatus);
  appendReceiptField(receipt, "product_save_reason_code",
                     window.saveSession.productSaveReasonCode);
  appendReceiptField(receipt, "product_save_durable_reason",
                     window.saveSession.productSaveDurableReason);
  appendReceiptField(receipt, "product_save_source", window.saveSession.productSaveSource);
  appendReceiptField(receipt, "product_save_save_id", window.saveSession.productSaveSaveId);
  appendReceiptField(receipt, "product_save_session_saved",
                     window.saveSession.productSaveSessionSaved);
  appendReceiptField(receipt, "active_product_save_id", window.saveSession.activeProductSaveId);
  appendReceiptField(receipt, "active_creative_save_id",
                     creativeIdentity.saveId);
  appendReceiptField(receipt, "active_creative_save_path",
                     creativeIdentity.savePath);
  appendReceiptField(receipt, "active_creative_world_id",
                     creativeIdentity.worldId);
  appendReceiptField(receipt, "active_creative_document_id",
                     creativeIdentity.documentId);
  appendReceiptField(receipt, "active_creative_object_count",
                     creativeIdentity.objectCount);
  appendReceiptField(receipt, "active_creative_next_object_id",
                     creativeIdentity.nextObjectId);
  appendReceiptField(receipt, "active_creative_save_status",
                     creativeIdentity.saveStatus);
  appendReceiptField(receipt, "active_creative_save_reason_code",
                     creativeIdentity.saveReasonCode);
  appendReceiptField(receipt, "active_creative_save_dirty_flags_before",
                     creativeIdentity.saveDirtyFlagsBefore);
  appendReceiptField(receipt, "active_creative_save_dirty_flags_drained",
                     creativeIdentity.saveDirtyFlagsDrained);
  appendReceiptField(receipt, "active_creative_save_dirty_flags_after",
                     creativeIdentity.saveDirtyFlagsAfter);
  appendReceiptField(receipt, "active_creative_save_saved_at_utc",
                     creativeIdentity.saveSavedAtUtc);
  appendReceiptField(receipt, "product_save_load_status",
                     window.saveSession.productSaveLoadResult.status);
  appendReceiptField(receipt, "product_save_load_reason_code",
                     window.saveSession.productSaveLoadResult.reasonCode);
  appendReceiptField(receipt, "product_save_load_save_id",
                     window.saveSession.productSaveLoadResult.record.id);
  appendReceiptField(receipt, "product_save_load_source",
                     window.saveSession.productSaveLoadSource);
  appendReceiptField(receipt, "product_save_load_selected_id",
                     window.saveSession.productSaveLoadSelectedId);
  appendReceiptField(receipt, "product_save_load_selected_enabled",
                     window.saveSession.productSaveLoadSelectedEnabled);
  appendReceiptField(receipt,
                     "product_save_load_authored_room_present",
                     window.saveSession.productSaveLoadResult.authoredRoomPresent);
  appendReceiptField(receipt,
                     "product_save_load_authored_room_id",
                     window.saveSession.productSaveLoadResult.authoredRoomId);
  appendReceiptField(receipt,
                     "product_save_load_authored_floor_count",
                     window.saveSession.productSaveLoadResult.authoredFloorCount);
  appendReceiptField(receipt,
                     "product_save_load_authored_wall_count",
                     window.saveSession.productSaveLoadResult.authoredWallCount);
  appendReceiptField(receipt,
                     "product_save_load_authored_object_count",
                     window.saveSession.productSaveLoadResult.authoredObjectCount);
  appendReceiptField(receipt,
                     "product_save_load_authored_marker_count",
                     window.saveSession.productSaveLoadResult.authoredMarkerCount);
  appendReceiptField(receipt, "saved_marker_bind_status",
                     window.saveSession.savedMarkerBind.status);
  appendReceiptField(receipt, "saved_marker_bind_reason_code",
                     window.saveSession.savedMarkerBind.reasonCode);
  appendReceiptField(receipt, "saved_marker_bind_requested",
                     window.saveSession.savedMarkerBind.requested);
  appendReceiptField(receipt, "saved_marker_bind_session_replaced",
                     window.saveSession.savedMarkerBind.sessionReplaced);
  appendReceiptField(receipt, "saved_marker_bind_room_id",
                     window.saveSession.savedMarkerBind.roomId);
  appendReceiptField(receipt, "saved_marker_bind_marker_count",
                     window.saveSession.savedMarkerBind.markerCount);
  appendReceiptField(receipt, "saved_marker_bind_seed_entity_count",
                     window.saveSession.savedMarkerBind.seedEntityCount);
  appendReceiptField(receipt, "saved_marker_bind_added_entity_count",
                     window.saveSession.savedMarkerBind.addedEntityCount);
  appendReceiptField(receipt, "saved_marker_bind_existing_entity_count",
                     window.saveSession.savedMarkerBind.existingEntityCount);
  appendReceiptField(receipt, "saved_marker_bind_added_objective_count",
                     window.saveSession.savedMarkerBind.addedObjectiveCount);
  appendReceiptField(receipt, "saved_marker_bind_existing_objective_count",
                     window.saveSession.savedMarkerBind.existingObjectiveCount);
  appendReceiptField(receipt, "saved_marker_bind_added_combatant_count",
                     window.saveSession.savedMarkerBind.addedCombatantCount);
  appendReceiptField(receipt, "saved_marker_bind_existing_combatant_count",
                     window.saveSession.savedMarkerBind.existingCombatantCount);
  appendReceiptField(receipt, "saved_marker_bind_pickup_count",
                     window.saveSession.savedMarkerBind.pickupCount);
  appendReceiptField(receipt, "saved_marker_bind_door_count",
                     window.saveSession.savedMarkerBind.doorCount);
  appendReceiptField(receipt, "saved_marker_bind_marker_entity_count",
                     window.saveSession.savedMarkerBind.markerEntityCount);
  appendReceiptField(receipt, "saved_marker_bind_npc_count",
                     window.saveSession.savedMarkerBind.npcCount);
  appendReceiptField(receipt, "saved_marker_bind_previous_hash",
                     window.saveSession.savedMarkerBind.previousHash);
  appendReceiptField(receipt, "saved_marker_bind_bound_hash",
                     window.saveSession.savedMarkerBind.boundHash);
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
                     window.roomEditorOverlay.visible);
  appendReceiptField(receipt, "room_editor_overlay_status",
                     window.roomEditorOverlay.status);
  appendReceiptField(receipt, "room_editor_overlay_reason_code",
                     window.roomEditorOverlay.reasonCode);
  appendReceiptField(receipt, "room_editor_overlay_item_count",
                     window.roomEditorOverlay.itemCount);
  appendReceiptField(receipt, "room_editor_overlay_world_x",
                     floatReceiptValue(window.roomEditorOverlay.worldX));
  appendReceiptField(receipt, "room_editor_overlay_world_y",
                     floatReceiptValue(window.roomEditorOverlay.worldY));
  appendReceiptField(receipt, "room_editor_overlay_world_z",
                     floatReceiptValue(window.roomEditorOverlay.worldZ));
  appendReceiptField(receipt, "room_editor_preview_pending",
                     window.roomEditorPreview.active);
  appendReceiptField(receipt, "room_editor_preview_visible",
                     window.roomEditorPreview.visible);
  appendReceiptField(receipt, "room_editor_preview_status",
                     window.roomEditorPreview.status);
  appendReceiptField(receipt, "room_editor_preview_reason_code",
                     window.roomEditorPreview.reasonCode);
  appendReceiptField(receipt, "room_editor_preview_candidate_id",
                     window.roomEditorPreview.candidateId);
  appendReceiptField(receipt, "room_editor_preview_tool",
                     window.roomEditorPreview.tool);
  appendReceiptField(receipt, "room_editor_preview_grid_x",
                     std::to_string(window.roomEditorPreview.gridX));
  appendReceiptField(receipt, "room_editor_preview_grid_z",
                     std::to_string(window.roomEditorPreview.gridZ));
  appendReceiptField(receipt, "room_editor_preview_before_draw_count",
                     std::to_string(window.roomEditorPreview.beforeDrawCount));
  appendReceiptField(receipt, "room_editor_preview_after_draw_count",
                     std::to_string(window.roomEditorPreview.afterDrawCount));
  appendReceiptField(
      receipt,
      "room_editor_preview_avoided_draw_count_delta",
      std::to_string(window.roomEditorPreview.avoidedDrawCountDelta));
  appendReceiptField(
      receipt,
      "room_editor_preview_before_triangle_count",
      std::to_string(window.roomEditorPreview.beforeTriangleCount));
  appendReceiptField(receipt,
                     "room_editor_preview_after_triangle_count",
                     std::to_string(window.roomEditorPreview.afterTriangleCount));
  appendReceiptField(
      receipt,
      "room_editor_preview_avoided_triangle_count_delta",
      std::to_string(window.roomEditorPreview.avoidedTriangleCountDelta));
  appendReceiptField(receipt, "room_editor_preview_optimized_draw_delta",
                     std::to_string(window.roomEditorPreview.optimizedDrawDelta));
  appendReceiptField(receipt, "room_editor_preview_optimized_triangle_delta",
                     std::to_string(
                         window.roomEditorPreview.optimizedTriangleDelta));
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
  appendReceiptField(receipt, "selected_save_id", window.saveSession.selectedProductSave.id);
  appendReceiptField(receipt, "selected_save_enabled",
                     window.saveSession.selectedProductSave.enabled);
  appendReceiptField(receipt, "selected_save_status",
                     window.saveSession.selectedProductSave.status);
  appendReceiptField(receipt, "save_slot_browser_mode", window.saveSession.saveSlotBrowserMode);
  appendReceiptField(receipt, "save_slot_ring_count", window.saveSession.saveSlotRingCount);
  appendReceiptField(receipt,
                     "save_slot_ring_selected_index",
                     window.saveSession.saveSlotRingSelectedIndex);
  appendReceiptField(receipt,
                     "save_slot_ring_selected_id",
                     window.saveSession.saveSlotRingSelectedId);
  appendReceiptField(receipt,
                     "save_slot_ring_selected_status",
                     window.saveSession.saveSlotRingSelectedStatus);
  appendReceiptField(receipt, "save_slot_action_command",
                     window.saveSession.saveSlotActionCommand);
  appendReceiptField(receipt, "save_slot_action_enabled",
                     window.saveSession.saveSlotActionEnabled);
  appendReceiptField(receipt,
                     "save_slot_action_confirmation_required",
                     window.saveSession.saveSlotActionConfirmationRequired);
  appendReceiptField(receipt, "save_slot_action_status",
                     window.saveSession.saveSlotActionStatus);
  appendReceiptField(receipt, "save_flow_operation", window.saveSession.saveFlow.operation);
  appendReceiptField(receipt,
                     "save_flow_source_surface",
                     window.saveSession.saveFlow.sourceSurface);
  appendReceiptField(receipt, "save_flow_status", window.saveSession.saveFlow.status);
  appendReceiptField(receipt,
                     "save_flow_reason_code",
                     window.saveSession.saveFlow.reasonCode);
  appendReceiptField(receipt,
                     "save_flow_affected_slot_id",
                     window.saveSession.saveFlow.affectedSlotId);
  appendReceiptField(receipt,
                     "save_flow_active_count_before",
                     window.saveSession.saveFlow.activeCountBefore);
  appendReceiptField(receipt,
                     "save_flow_active_count_after",
                     window.saveSession.saveFlow.activeCountAfter);
  appendReceiptField(receipt,
                     "save_flow_deleted_count_after",
                     window.saveSession.saveFlow.deletedCountAfter);
  appendReceiptField(receipt,
                     "save_flow_selected_slot_after",
                     window.saveSession.saveFlow.selectedSlotAfter);
  appendReceiptField(receipt, "save_delete_confirmation_open",
                     window.saveSession.saveDelete.confirmationOpen);
  appendReceiptField(receipt, "save_delete_candidate_id",
                     window.saveSession.saveDelete.candidateId);
  appendReceiptField(receipt, "save_delete_candidate_enabled",
                     window.saveSession.saveDelete.candidateEnabled);
  appendReceiptField(receipt, "save_delete_status", window.saveSession.saveDelete.status);
  appendReceiptField(receipt, "save_delete_reason_code",
                     window.saveSession.saveDelete.reasonCode);
  appendReceiptField(receipt, "save_delete_type", window.saveSession.saveDelete.type);
  appendReceiptField(receipt, "save_delete_recoverable",
                     window.saveSession.saveDelete.recoverable);
  appendReceiptField(receipt, "save_delete_executed", window.saveSession.saveDelete.executed);
  appendReceiptField(receipt, "deleted_save_browser_open",
                     window.saveSession.deletedSaveBrowserOpen);
  appendReceiptField(receipt, "deleted_save_count", window.saveSession.deletedSaveCount);
  appendReceiptField(receipt, "deleted_compatible_save_count",
                     window.saveSession.deletedCompatibleSaveCount);
  appendReceiptField(receipt, "deleted_selected_save_id",
                     window.saveSession.deletedSelectedSaveId);
  appendReceiptField(receipt, "deleted_selected_save_enabled",
                     window.saveSession.deletedSelectedSaveEnabled);
  appendReceiptField(receipt, "deleted_selected_save_status",
                     window.saveSession.deletedSelectedSaveStatus);
  appendReceiptField(receipt, "save_recover_status", window.saveSession.saveRecover.status);
  appendReceiptField(receipt, "save_recover_reason_code",
                     window.saveSession.saveRecover.reasonCode);
  appendReceiptField(receipt, "save_recover_executed",
                     window.saveSession.saveRecover.executed);
  appendReceiptField(receipt, "save_recover_save_id", window.saveSession.saveRecover.saveId);
  appendReceiptField(receipt, "save_recover_snapshot_recovered",
                     window.saveSession.saveRecover.snapshotRecovered);
  appendReceiptField(receipt, "save_recover_snapshot_missing",
                     window.saveSession.saveRecover.snapshotMissing);
  appendReceiptField(receipt, "product_save_load_previous_hash",
                     window.saveSession.productSaveLoadResult.previousHash);
  appendReceiptField(receipt, "product_save_load_loaded_hash",
                     window.saveSession.productSaveLoadResult.loadedHash);
  appendReceiptField(receipt, "product_save_load_session_loaded",
                     window.saveSession.productSaveLoadResult.sessionLoaded);
}

}  // namespace iggy3d
