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

namespace {

void appendProductCreativeUiCommandMutationFields(
    RenderReceipt& receipt,
    const ProductCreativeUiCommandMutationDiagnostics& fields) {
  appendReceiptField(receipt, "creative_ui_command_mutation_requested",
                     fields.requested);
  appendReceiptField(receipt, "creative_ui_command_mutation_accepted",
                     fields.accepted);
  appendReceiptField(receipt, "creative_ui_command_mutation_changed",
                     fields.changed);
  appendReceiptField(receipt, "creative_ui_command_mutation_status",
                     fields.status);
  appendReceiptField(receipt, "creative_ui_command_document_mutation_status",
                     fields.documentStatus);
  appendReceiptField(receipt, "creative_ui_command_mutation_kind",
                     fields.kind);
  appendReceiptField(receipt, "creative_ui_command_mutation_target",
                     fields.target);
  appendReceiptField(receipt, "creative_ui_command_mutation_object_id",
                     fields.objectId);
  appendReceiptField(receipt, "creative_ui_command_mutation_object_kind",
                     fields.objectKind);
  appendReceiptField(receipt, "creative_ui_command_visible_before",
                     fields.visibleBefore);
  appendReceiptField(receipt, "creative_ui_command_visible_after",
                     fields.visibleAfter);
  appendReceiptField(receipt, "creative_ui_command_locked_before",
                     fields.lockedBefore);
  appendReceiptField(receipt, "creative_ui_command_locked_after",
                     fields.lockedAfter);
  appendReceiptField(receipt, "creative_ui_command_revision_before",
                     fields.revisionBefore);
  appendReceiptField(receipt, "creative_ui_command_revision_after",
                     fields.revisionAfter);
  appendReceiptField(receipt, "creative_ui_command_mutation_message",
                     fields.message);
}

void appendProductCreativeUiCommandCreateFields(
    RenderReceipt& receipt,
    const ProductCreativeUiCommandCreateDiagnostics& fields) {
  appendReceiptField(receipt, "creative_ui_command_create_requested",
                     fields.requested);
  appendReceiptField(receipt, "creative_ui_command_create_accepted",
                     fields.accepted);
  appendReceiptField(receipt, "creative_ui_command_create_changed",
                     fields.changed);
  appendReceiptField(receipt, "creative_ui_command_create_status",
                     fields.status);
  appendReceiptField(receipt, "creative_ui_command_create_object_id",
                     fields.objectId);
  appendReceiptField(receipt, "creative_ui_command_create_object_kind",
                     fields.objectKind);
  appendReceiptField(receipt, "creative_ui_command_create_object_name",
                     fields.objectName);
  appendReceiptField(receipt, "creative_ui_command_create_revision_before",
                     fields.revisionBefore);
  appendReceiptField(receipt, "creative_ui_command_create_revision_after",
                     fields.revisionAfter);
  appendReceiptField(receipt, "creative_ui_command_create_dirty_flags",
                     fields.dirtyFlags);
  appendReceiptField(receipt, "creative_ui_command_create_message",
                     fields.message);
  appendReceiptField(receipt, "creative_ui_command_create_reason_code",
                     fields.reasonCode);
}

void appendProductCreativeUiCommandDeleteFields(
    RenderReceipt& receipt,
    const ProductCreativeUiCommandDeleteDiagnostics& fields) {
  appendReceiptField(receipt, "creative_ui_command_delete_requested",
                     fields.requested);
  appendReceiptField(receipt, "creative_ui_command_delete_accepted",
                     fields.accepted);
  appendReceiptField(receipt, "creative_ui_command_delete_changed",
                     fields.changed);
  appendReceiptField(receipt, "creative_ui_command_delete_removed",
                     fields.removed);
  appendReceiptField(receipt, "creative_ui_command_delete_object_id",
                     fields.objectId);
  appendReceiptField(receipt, "creative_ui_command_delete_object_kind",
                     fields.objectKind);
  appendReceiptField(receipt, "creative_ui_command_delete_object_name",
                     fields.objectName);
  appendReceiptField(receipt, "creative_ui_command_delete_revision_before",
                     fields.revisionBefore);
  appendReceiptField(receipt, "creative_ui_command_delete_revision_after",
                     fields.revisionAfter);
  appendReceiptField(receipt, "creative_ui_command_delete_dirty_flags",
                     fields.dirtyFlags);
  appendReceiptField(receipt, "creative_ui_command_delete_status",
                     fields.status);
  appendReceiptField(receipt, "creative_ui_command_delete_message",
                     fields.message);
  appendReceiptField(receipt, "creative_ui_command_delete_reason_code",
                     fields.reasonCode);
}

void appendProductCreativeUiCommandUndoFields(
    RenderReceipt& receipt,
    const ProductCreativeUiCommandUndoDiagnostics& fields) {
  appendReceiptField(receipt, "creative_ui_command_undo_requested",
                     fields.requested);
  appendReceiptField(receipt, "creative_ui_command_undo_accepted",
                     fields.accepted);
  appendReceiptField(receipt, "creative_ui_command_undo_changed",
                     fields.changed);
  appendReceiptField(receipt, "creative_ui_command_undo_had_snapshot",
                     fields.hadSnapshot);
  appendReceiptField(receipt, "creative_ui_command_undo_document_id",
                     fields.documentId);
  appendReceiptField(receipt, "creative_ui_command_undo_revision_before",
                     fields.revisionBefore);
  appendReceiptField(receipt, "creative_ui_command_undo_revision_after",
                     fields.revisionAfter);
  appendReceiptField(receipt, "creative_ui_command_undo_object_count_before",
                     fields.objectCountBefore);
  appendReceiptField(receipt, "creative_ui_command_undo_object_count_after",
                     fields.objectCountAfter);
  appendReceiptField(receipt, "creative_ui_command_undo_depth_before",
                     fields.depthBefore);
  appendReceiptField(receipt, "creative_ui_command_undo_depth_after",
                     fields.depthAfter);
  appendReceiptField(receipt, "creative_ui_command_undo_status",
                     fields.status);
  appendReceiptField(receipt, "creative_ui_command_undo_message",
                     fields.message);
  appendReceiptField(receipt, "creative_ui_command_undo_reason_code",
                     fields.reasonCode);
}

void appendProductCreativeUiCommandRoomShellFields(
    RenderReceipt& receipt,
    const ProductCreativeUiCommandRoomShellDiagnostics& fields) {
  appendReceiptField(receipt, "creative_ui_command_shell_requested",
                     fields.requested);
  appendReceiptField(receipt, "creative_ui_command_shell_accepted",
                     fields.accepted);
  appendReceiptField(receipt, "creative_ui_command_shell_changed",
                     fields.changed);
  appendReceiptField(receipt, "creative_ui_command_shell_room_object_id",
                     fields.roomObjectId);
  appendReceiptField(receipt,
                     "creative_ui_command_shell_generated_object_count",
                     fields.generatedObjectCount);
  appendReceiptField(receipt,
                     "creative_ui_command_shell_removed_object_count",
                     fields.removedObjectCount);
  appendReceiptField(receipt, "creative_ui_command_shell_floor_count",
                     fields.floorCount);
  appendReceiptField(receipt, "creative_ui_command_shell_wall_count",
                     fields.wallCount);
  appendReceiptField(receipt, "creative_ui_command_shell_revision_before",
                     fields.revisionBefore);
  appendReceiptField(receipt, "creative_ui_command_shell_revision_after",
                     fields.revisionAfter);
  appendReceiptField(receipt, "creative_ui_command_shell_status",
                     fields.status);
  appendReceiptField(receipt, "creative_ui_command_shell_reason_code",
                     fields.reasonCode);
  appendReceiptField(receipt, "creative_ui_command_shell_message",
                     fields.message);
}

struct ProductCreativeBakedRoomRefreshReceiptKeySet {
  std::string_view requested;
  std::string_view accepted;
  std::string_view clearedActiveRoom;
  std::string_view status;
  std::string_view reasonCode;
  std::string_view bakeMeasured;
  std::string_view bakeElapsedMicroseconds;
  std::string_view bakedDocumentRevision;
  std::string_view staticMeshCount;
  std::string_view anchorCount;
  std::string_view spatialSurfaceCount;
  std::string_view collisionReady;
  std::string_view collisionQuerySurfaceCount;
};

void appendProductCreativeBakedRoomRefreshDiagnosticFields(
    RenderReceipt& receipt,
    const ProductCreativeBakedRoomRefreshDiagnostics& fields,
    const ProductCreativeBakedRoomRefreshReceiptKeySet& keys) {
  appendReceiptField(receipt, keys.requested, fields.requested);
  appendReceiptField(receipt, keys.accepted, fields.accepted);
  if (!keys.clearedActiveRoom.empty()) {
    appendReceiptField(receipt,
                       keys.clearedActiveRoom,
                       fields.clearedActiveRoom);
  }
  appendReceiptField(receipt, keys.status, fields.status);
  appendReceiptField(receipt, keys.reasonCode, fields.reasonCode);
  appendReceiptField(receipt, keys.bakeMeasured, fields.bakeMeasured);
  appendReceiptField(receipt,
                     keys.bakeElapsedMicroseconds,
                     fields.bakeElapsedMicroseconds);
  appendReceiptField(receipt,
                     keys.bakedDocumentRevision,
                     fields.bakedDocumentRevision);
  appendReceiptField(receipt, keys.staticMeshCount, fields.staticMeshCount);
  appendReceiptField(receipt, keys.anchorCount, fields.anchorCount);
  appendReceiptField(receipt,
                     keys.spatialSurfaceCount,
                     fields.spatialSurfaceCount);
  appendReceiptField(receipt, keys.collisionReady, fields.collisionReady);
  appendReceiptField(receipt,
                     keys.collisionQuerySurfaceCount,
                     fields.collisionQuerySurfaceCount);
}

void appendProductCreativeUiCommandBakedRoomRefreshFields(
    RenderReceipt& receipt,
    const ProductCreativeBakedRoomRefreshDiagnostics& fields) {
  constexpr ProductCreativeBakedRoomRefreshReceiptKeySet keys{
      "creative_ui_command_baked_room_refresh_requested",
      "creative_ui_command_baked_room_refresh_accepted",
      "",
      "creative_ui_command_baked_room_refresh_status",
      "creative_ui_command_baked_room_refresh_reason_code",
      "creative_ui_command_baked_room_bake_measured",
      "creative_ui_command_baked_room_bake_elapsed_microseconds",
      "creative_ui_command_baked_room_baked_document_revision",
      "creative_ui_command_baked_room_static_mesh_count",
      "creative_ui_command_baked_room_anchor_count",
      "creative_ui_command_baked_room_spatial_surface_count",
      "creative_ui_command_baked_room_collision_ready",
      "creative_ui_command_baked_room_collision_query_surface_count",
  };
  appendProductCreativeBakedRoomRefreshDiagnosticFields(receipt, fields, keys);
}

void appendProductCreativeBakedRoomAutoRefreshFields(
    RenderReceipt& receipt,
    const ProductCreativeBakedRoomRefreshDiagnostics& fields) {
  constexpr ProductCreativeBakedRoomRefreshReceiptKeySet keys{
      "creative_baked_room_auto_refresh_requested",
      "creative_baked_room_auto_refresh_accepted",
      "creative_baked_room_auto_refresh_cleared_active_room",
      "creative_baked_room_auto_refresh_status",
      "creative_baked_room_auto_refresh_reason_code",
      "creative_baked_room_auto_refresh_bake_measured",
      "creative_baked_room_auto_refresh_bake_elapsed_microseconds",
      "creative_baked_room_auto_refresh_baked_document_revision",
      "creative_baked_room_auto_refresh_static_mesh_count",
      "creative_baked_room_auto_refresh_anchor_count",
      "creative_baked_room_auto_refresh_spatial_surface_count",
      "creative_baked_room_auto_refresh_collision_ready",
      "creative_baked_room_auto_refresh_collision_query_surface_count",
  };
  appendProductCreativeBakedRoomRefreshDiagnosticFields(receipt, fields, keys);
}

void appendProductCreativeUiCommandFields(
    RenderReceipt& receipt,
    const ProductCreativeUiCommandDiagnostics& fields) {
  appendReceiptField(receipt, "creative_ui_command_requested",
                     fields.requested);
  appendReceiptField(receipt, "creative_ui_command_facade_available",
                     fields.facadeAvailable);
  appendReceiptField(receipt, "creative_ui_command_input_consumed",
                     fields.inputConsumed);
  appendReceiptField(receipt, "creative_ui_command_input_enabled",
                     fields.inputEnabled);
  appendReceiptField(receipt, "creative_ui_command_accepted",
                     fields.accepted);
  appendReceiptField(receipt, "creative_ui_command_changed", fields.changed);
  appendReceiptField(receipt, "creative_ui_command_kind", fields.kind);
  appendReceiptField(receipt, "creative_ui_command_tool", fields.tool);
  appendReceiptField(receipt, "creative_ui_command_object_kind",
                     fields.objectKind);
  appendReceiptField(receipt, "creative_ui_command_tool_before",
                     fields.toolBefore);
  appendReceiptField(receipt, "creative_ui_command_tool_after",
                     fields.toolAfter);
  appendReceiptField(receipt, "creative_ui_command_semantic_id",
                     fields.semanticId);
  appendReceiptField(receipt, "creative_ui_command_status", fields.status);
  appendReceiptField(receipt, "creative_ui_command_reason_code",
                     fields.reasonCode);
  appendProductCreativeUiCommandMutationFields(receipt, fields.mutation);
  appendProductCreativeUiCommandCreateFields(receipt, fields.create);
  appendProductCreativeUiCommandDeleteFields(receipt, fields.deleteObject);
  appendProductCreativeUiCommandUndoFields(receipt, fields.undo);
  appendProductCreativeUiCommandRoomShellFields(receipt, fields.shell);
  appendProductCreativeUiCommandBakedRoomRefreshFields(
      receipt, fields.bakedRoomRefresh);
}

}  // namespace

void appendProductCreativeUiFields(RenderReceipt& receipt, const ProductAppWindowState& window) {
  const auto& authoring = window.creativeAuthoring;
  appendReceiptField(receipt, "creative_ui_projection_requested",
                     authoring.creativeUiProjection.requested);
  appendReceiptField(receipt, "creative_ui_projection_ready",
                     authoring.creativeUiProjection.ready);
  appendReceiptField(receipt, "creative_ui_projection_partial",
                     authoring.creativeUiProjection.partial);
  appendReceiptField(receipt, "creative_ui_projection_status",
                     authoring.creativeUiProjection.status);
  appendReceiptField(receipt, "creative_ui_projection_reason_code",
                     authoring.creativeUiProjection.reasonCode);
  appendReceiptField(receipt, "creative_ui_projection_used_model",
                     authoring.creativeUiProjection.usedModel);
  appendReceiptField(receipt, "creative_ui_projection_used_facade",
                     authoring.creativeUiProjection.usedFacade);
  appendReceiptField(receipt, "creative_ui_projection_virtual_width",
                     static_cast<std::uint64_t>(
                         authoring.creativeUiProjection.virtualWidth));
  appendReceiptField(receipt, "creative_ui_projection_virtual_height",
                     static_cast<std::uint64_t>(
                         authoring.creativeUiProjection.virtualHeight));
  appendReceiptField(receipt, "creative_ui_projection_theme",
                     authoring.creativeUiProjection.theme);
  appendReceiptField(receipt, "creative_ui_projection_panel_count",
                     authoring.creativeUiProjection.panelCount);
  appendReceiptField(receipt, "creative_ui_projection_model_row_count",
                     authoring.creativeUiProjection.modelRowCount);
  appendReceiptField(receipt, "creative_ui_projection_primitive_count",
                     authoring.creativeUiProjection.primitiveCount);
  appendReceiptField(receipt, "creative_ui_projection_text_count",
                     authoring.creativeUiProjection.textCount);
  appendReceiptField(receipt, "creative_ui_projection_rect_count",
                     authoring.creativeUiProjection.rectCount);
  appendReceiptField(receipt, "creative_ui_projection_row_count",
                     authoring.creativeUiProjection.rowCount);
  appendReceiptField(receipt, "creative_ui_projection_disabled_row_count",
                     authoring.creativeUiProjection.disabledRowCount);
  appendReceiptField(receipt, "creative_ui_projection_hit_region_count",
                     authoring.creativeUiProjection.hitRegionCount);
  appendReceiptField(receipt, "creative_ui_input_requested",
                     authoring.creativeUiInput.requested);
  appendReceiptField(receipt, "creative_ui_input_click_present",
                     authoring.creativeUiInput.clickPresent);
  appendReceiptField(receipt, "creative_ui_input_draw_list_available",
                     authoring.creativeUiInput.drawListAvailable);
  appendReceiptField(receipt, "creative_ui_input_routed",
                     authoring.creativeUiInput.routed);
  appendReceiptField(receipt, "creative_ui_input_hit",
                     authoring.creativeUiInput.hit);
  appendReceiptField(receipt, "creative_ui_input_consumed",
                     authoring.creativeUiInput.consumed);
  appendReceiptField(receipt, "creative_ui_input_enabled",
                     authoring.creativeUiInput.enabled);
  appendReceiptField(receipt, "creative_ui_input_surface",
                     authoring.creativeUiInput.surface);
  appendReceiptField(receipt, "creative_ui_input_kind",
                     authoring.creativeUiInput.kind);
  appendReceiptField(receipt, "creative_ui_input_action",
                     authoring.creativeUiInput.action);
  appendReceiptField(receipt, "creative_ui_input_layer_index",
                     authoring.creativeUiInput.layerIndex);
  appendReceiptField(receipt, "creative_ui_input_region_index",
                     authoring.creativeUiInput.regionIndex);
  appendReceiptField(receipt, "creative_ui_input_semantic_id",
                     authoring.creativeUiInput.semanticId);
  appendReceiptField(receipt, "creative_ui_input_status",
                     authoring.creativeUiInput.status);
  appendReceiptField(receipt, "creative_ui_input_reason_code",
                     authoring.creativeUiInput.reasonCode);
  appendReceiptField(receipt, "creative_ui_last_click_seen",
                     authoring.creativeUiLast.clickSeen);
  appendReceiptField(receipt, "creative_ui_last_click_x",
                     authoring.creativeUiLast.clickX);
  appendReceiptField(receipt, "creative_ui_last_click_y",
                     authoring.creativeUiLast.clickY);
  appendReceiptField(receipt, "creative_ui_last_input_hit",
                     authoring.creativeUiLast.inputHit);
  appendReceiptField(receipt, "creative_ui_last_input_consumed",
                     authoring.creativeUiLast.inputConsumed);
  appendReceiptField(receipt, "creative_ui_last_input_status",
                     authoring.creativeUiLast.inputStatus);
  appendReceiptField(receipt, "creative_ui_last_input_semantic_id",
                     authoring.creativeUiLast.inputSemanticId);
  appendReceiptField(receipt, "creative_ui_last_command_kind",
                     authoring.creativeUiLast.commandKind);
  appendReceiptField(receipt, "creative_ui_last_command_status",
                     authoring.creativeUiLast.commandStatus);
  appendReceiptField(receipt,
                     "creative_ui_last_command_create_requested",
                     authoring.creativeUiLast.commandCreateRequested);
  appendReceiptField(receipt,
                     "creative_ui_last_command_create_accepted",
                     authoring.creativeUiLast.commandCreateAccepted);
  appendReceiptField(receipt,
                     "creative_ui_last_command_create_changed",
                     authoring.creativeUiLast.commandCreateChanged);
  appendReceiptField(receipt,
                     "creative_ui_last_command_create_object_id",
                     authoring.creativeUiLast.commandCreateObjectId);
  appendReceiptField(receipt,
                     "creative_ui_input_downstream_click_requested",
                     authoring.creativeUiInput.downstreamClickRequested);
  appendReceiptField(receipt,
                     "creative_ui_input_downstream_click_present",
                     authoring.creativeUiInput.downstreamClickPresent);
  appendReceiptField(receipt,
                     "creative_ui_input_downstream_click_higher_priority",
                     authoring.creativeUiInput.downstreamClickHigherPriority);
  appendReceiptField(receipt,
                     "creative_ui_input_downstream_click_suppressed",
                     authoring.creativeUiInput.downstreamClickSuppressed);
  appendReceiptField(receipt,
                     "creative_ui_input_downstream_click_status",
                     authoring.creativeUiInput.downstreamClickStatus);
  appendReceiptField(receipt,
                     "creative_ui_input_downstream_click_reason_code",
                     authoring.creativeUiInput.downstreamClickReasonCode);
  appendProductCreativeUiCommandFields(receipt, authoring.creativeUiCommand);
  appendProductCreativeBakedRoomAutoRefreshFields(
      receipt, authoring.creativeBakedRoomAutoRefresh);
  appendReceiptField(receipt, "creative_document_revision_observed",
                     authoring.creativeDocumentRevision.observed);
  appendReceiptField(receipt, "creative_document_changed_this_frame",
                     authoring.creativeDocumentChangedThisFrame);
  appendReceiptField(receipt, "creative_document_revision_document_id",
                     authoring.creativeDocumentRevision.documentId);
  appendReceiptField(receipt, "creative_document_revision_before_frame",
                     authoring.creativeDocumentRevision.beforeFrame);
  appendReceiptField(receipt, "creative_document_revision_after_frame",
                     authoring.creativeDocumentRevision.afterFrame);
  appendReceiptField(receipt, "creative_undo_available",
                     authoring.creativeUndo.available);
  appendReceiptField(receipt, "creative_undo_depth",
                     authoring.creativeUndo.depth);
  appendReceiptField(receipt, "creative_baked_room_stale",
                     authoring.creativeBakedRoomStale);
  appendReceiptField(receipt, "creative_baked_room_stale_document_id",
                     authoring.creativeBakedRoomStaleDocumentId);
  appendReceiptField(receipt, "creative_baked_room_stale_revision",
                     authoring.creativeBakedRoomStaleRevision);
  appendReceiptField(receipt, "creative_baked_room_stale_status",
                     authoring.creativeBakedRoomStaleStatus);
  appendReceiptField(receipt, "creative_baked_room_stale_reason_code",
                     authoring.creativeBakedRoomStaleReasonCode);
}

}  // namespace iggy3d
