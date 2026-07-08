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
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/window/RendererLifecycle.hpp"
#include "app/iggy3d/creative/render/WireframeDebugLines.hpp"

namespace iggy3d {

namespace {

struct SaveStateReceiptContext {
  const FrontendState& frontend;
  const ProductAppWindowState& window;
  const creative::CreativeActiveIdentity& creativeIdentity;
};

struct SaveStateReceiptFieldRow {
  std::string_view key;
  void (*append)(RenderReceipt& receipt,
                 const SaveStateReceiptContext& context,
                 std::string_view key);
};

const std::array<SaveStateReceiptFieldRow, 146> kSaveStateReceiptFields{{
    {"product_save_status",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.productSaveStatus);
     }},
    {"product_save_reason_code",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.productSaveReasonCode);
     }},
    {"product_save_durable_reason",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.productSaveDurableReason);
     }},
    {"product_save_source",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.productSaveSource);
     }},
    {"product_save_save_id",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.productSaveSaveId);
     }},
    {"product_save_session_saved",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.productSaveSessionSaved);
     }},
    {"active_product_save_id",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.activeProductSaveId);
     }},
    {"active_creative_save_id",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.creativeIdentity.saveId);
     }},
    {"active_creative_save_path",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.creativeIdentity.savePath);
     }},
    {"active_creative_world_id",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.creativeIdentity.worldId);
     }},
    {"active_creative_document_id",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.creativeIdentity.documentId);
     }},
    {"active_creative_object_count",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.creativeIdentity.objectCount);
     }},
    {"active_creative_next_object_id",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.creativeIdentity.nextObjectId);
     }},
    {"active_creative_save_status",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.creativeIdentity.saveStatus);
     }},
    {"active_creative_save_reason_code",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.creativeIdentity.saveReasonCode);
     }},
    {"active_creative_save_dirty_flags_before",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.creativeIdentity.saveDirtyFlagsBefore);
     }},
    {"active_creative_save_dirty_flags_drained",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.creativeIdentity.saveDirtyFlagsDrained);
     }},
    {"active_creative_save_dirty_flags_after",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.creativeIdentity.saveDirtyFlagsAfter);
     }},
    {"active_creative_save_saved_at_utc",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.creativeIdentity.saveSavedAtUtc);
     }},
    {"product_save_load_status",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.productSaveLoadResult.status);
     }},
    {"product_save_load_reason_code",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.productSaveLoadResult.reasonCode);
     }},
    {"product_save_load_save_id",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.productSaveLoadResult.record.id);
     }},
    {"product_save_load_source",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.productSaveLoadSource);
     }},
    {"product_save_load_selected_id",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.productSaveLoadSelectedId);
     }},
    {"product_save_load_selected_enabled",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.productSaveLoadSelectedEnabled);
     }},
    {"product_save_load_authored_room_present",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.productSaveLoadResult.authoredRoomPresent);
     }},
    {"product_save_load_authored_room_id",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.productSaveLoadResult.authoredRoomId);
     }},
    {"product_save_load_authored_floor_count",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.productSaveLoadResult.authoredFloorCount);
     }},
    {"product_save_load_authored_wall_count",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.productSaveLoadResult.authoredWallCount);
     }},
    {"product_save_load_authored_object_count",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.productSaveLoadResult.authoredObjectCount);
     }},
    {"product_save_load_authored_marker_count",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.productSaveLoadResult.authoredMarkerCount);
     }},
    {"saved_marker_bind_status",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.savedMarkerBind.status);
     }},
    {"saved_marker_bind_reason_code",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.savedMarkerBind.reasonCode);
     }},
    {"saved_marker_bind_requested",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.savedMarkerBind.requested);
     }},
    {"saved_marker_bind_session_replaced",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.savedMarkerBind.sessionReplaced);
     }},
    {"saved_marker_bind_room_id",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.savedMarkerBind.roomId);
     }},
    {"saved_marker_bind_marker_count",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.savedMarkerBind.markerCount);
     }},
    {"saved_marker_bind_seed_entity_count",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.savedMarkerBind.seedEntityCount);
     }},
    {"saved_marker_bind_added_entity_count",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.savedMarkerBind.addedEntityCount);
     }},
    {"saved_marker_bind_existing_entity_count",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.savedMarkerBind.existingEntityCount);
     }},
    {"saved_marker_bind_added_objective_count",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.savedMarkerBind.addedObjectiveCount);
     }},
    {"saved_marker_bind_existing_objective_count",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.savedMarkerBind.existingObjectiveCount);
     }},
    {"saved_marker_bind_added_combatant_count",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.savedMarkerBind.addedCombatantCount);
     }},
    {"saved_marker_bind_existing_combatant_count",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.savedMarkerBind.existingCombatantCount);
     }},
    {"saved_marker_bind_pickup_count",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.savedMarkerBind.pickupCount);
     }},
    {"saved_marker_bind_door_count",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.savedMarkerBind.doorCount);
     }},
    {"saved_marker_bind_marker_entity_count",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.savedMarkerBind.markerEntityCount);
     }},
    {"saved_marker_bind_npc_count",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.savedMarkerBind.npcCount);
     }},
    {"saved_marker_bind_previous_hash",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.savedMarkerBind.previousHash);
     }},
    {"saved_marker_bind_bound_hash",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.savedMarkerBind.boundHash);
     }},
    {"room_editor_cursor_ready",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.creativeAuthoring.roomEditorCursorReady);
     }},
    {"room_editor_grid_x",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           std::to_string(context.window.creativeAuthoring.roomEditorCursor.gridX));
     }},
    {"room_editor_grid_z",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           std::to_string(context.window.creativeAuthoring.roomEditorCursor.gridZ));
     }},
    {"room_editor_story_index",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           std::to_string(context.window.creativeAuthoring.roomEditorCursor.storyIndex));
     }},
    {"room_editor_cell_size_meters",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           floatReceiptValue(context.window.creativeAuthoring.roomEditorCursor.cellSizeMeters));
     }},
    {"room_editor_tool",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           productRoomEditorToolName(context.window.creativeAuthoring.roomEditorCursor.selectedTool));
     }},
    {"room_editor_wall_direction",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           productRoomEditorDirectionName(context.window.creativeAuthoring.roomEditorCursor.wallDirection));
     }},
    {"room_editor_status",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.creativeAuthoring.roomEditorStatus);
     }},
    {"room_editor_reason_code",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.creativeAuthoring.roomEditorReasonCode);
     }},
    {"room_editor_last_operation",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.creativeAuthoring.roomEditorLastOperation);
     }},
    {"room_editor_last_operation_accepted",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.creativeAuthoring.roomEditorLastOperationAccepted);
     }},
    {"room_editor_last_primitive_id",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.creativeAuthoring.roomEditorLastPrimitiveId);
     }},
    {"room_editor_overlay_visible",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.creativeAuthoring.roomEditorOverlay.visible);
     }},
    {"room_editor_overlay_status",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.creativeAuthoring.roomEditorOverlay.status);
     }},
    {"room_editor_overlay_reason_code",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.creativeAuthoring.roomEditorOverlay.reasonCode);
     }},
    {"room_editor_overlay_item_count",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.creativeAuthoring.roomEditorOverlay.itemCount);
     }},
    {"room_editor_overlay_world_x",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           floatReceiptValue(context.window.creativeAuthoring.roomEditorOverlay.worldX));
     }},
    {"room_editor_overlay_world_y",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           floatReceiptValue(context.window.creativeAuthoring.roomEditorOverlay.worldY));
     }},
    {"room_editor_overlay_world_z",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           floatReceiptValue(context.window.creativeAuthoring.roomEditorOverlay.worldZ));
     }},
    {"room_editor_preview_pending",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.creativeAuthoring.roomEditorPreview.active);
     }},
    {"room_editor_preview_visible",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.creativeAuthoring.roomEditorPreview.visible);
     }},
    {"room_editor_preview_status",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.creativeAuthoring.roomEditorPreview.status);
     }},
    {"room_editor_preview_reason_code",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.creativeAuthoring.roomEditorPreview.reasonCode);
     }},
    {"room_editor_preview_candidate_id",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.creativeAuthoring.roomEditorPreview.candidateId);
     }},
    {"room_editor_preview_tool",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.creativeAuthoring.roomEditorPreview.tool);
     }},
    {"room_editor_preview_grid_x",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           std::to_string(context.window.creativeAuthoring.roomEditorPreview.gridX));
     }},
    {"room_editor_preview_grid_z",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           std::to_string(context.window.creativeAuthoring.roomEditorPreview.gridZ));
     }},
    {"room_editor_preview_before_draw_count",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           std::to_string(context.window.creativeAuthoring.roomEditorPreview.beforeDrawCount));
     }},
    {"room_editor_preview_after_draw_count",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           std::to_string(context.window.creativeAuthoring.roomEditorPreview.afterDrawCount));
     }},
    {"room_editor_preview_avoided_draw_count_delta",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           std::to_string(context.window.creativeAuthoring.roomEditorPreview.avoidedDrawCountDelta));
     }},
    {"room_editor_preview_before_triangle_count",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           std::to_string(context.window.creativeAuthoring.roomEditorPreview.beforeTriangleCount));
     }},
    {"room_editor_preview_after_triangle_count",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           std::to_string(context.window.creativeAuthoring.roomEditorPreview.afterTriangleCount));
     }},
    {"room_editor_preview_avoided_triangle_count_delta",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           std::to_string(context.window.creativeAuthoring.roomEditorPreview.avoidedTriangleCountDelta));
     }},
    {"room_editor_preview_optimized_draw_delta",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           std::to_string(context.window.creativeAuthoring.roomEditorPreview.optimizedDrawDelta));
     }},
    {"room_editor_preview_optimized_triangle_delta",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           std::to_string(
                                    context.window.creativeAuthoring.roomEditorPreview.optimizedTriangleDelta));
     }},
    {"room_editor_hud_visible",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.creativeAuthoring.roomEditorHud.visible);
     }},
    {"room_editor_hud_status",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.creativeAuthoring.roomEditorHud.status);
     }},
    {"room_editor_hud_reason_code",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.creativeAuthoring.roomEditorHud.reasonCode);
     }},
    {"room_editor_hud_tool",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.creativeAuthoring.roomEditorHud.toolName);
     }},
    {"room_editor_hud_wall_direction",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.creativeAuthoring.roomEditorHud.wallDirectionName);
     }},
    {"room_editor_hud_grid_x",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           std::to_string(context.window.creativeAuthoring.roomEditorHud.gridX));
     }},
    {"room_editor_hud_grid_z",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           std::to_string(context.window.creativeAuthoring.roomEditorHud.gridZ));
     }},
    {"room_editor_hud_last_operation",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.creativeAuthoring.roomEditorHud.lastOperation);
     }},
    {"room_editor_hud_last_operation_accepted",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.creativeAuthoring.roomEditorHud.lastOperationAccepted);
     }},
    {"room_editor_hud_last_primitive_id",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.creativeAuthoring.roomEditorHud.lastPrimitiveId);
     }},
    {"room_editor_hud_preview_active",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.creativeAuthoring.roomEditorHud.previewActive);
     }},
    {"room_editor_hud_preview_status",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.creativeAuthoring.roomEditorHud.previewStatus);
     }},
    {"room_editor_hud_preview_candidate_id",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.creativeAuthoring.roomEditorHud.previewCandidateId);
     }},
    {"room_editor_hud_preview_optimized_draw_delta",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           std::to_string(
                                    context.window.creativeAuthoring.roomEditorHud.previewOptimizedDrawDelta));
     }},
    {"room_editor_hud_preview_optimized_triangle_delta",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           std::to_string(
                                    context.window.creativeAuthoring.roomEditorHud.previewOptimizedTriangleDelta));
     }},
    {"room_editor_hud_line_count",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           std::to_string(context.window.creativeAuthoring.roomEditorHud.lineCount));
     }},
    {"save_browser_mode",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           frontendSaveBrowserModeName(context.frontend.saveBrowserMode));
     }},
    {"selected_save_id",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.selectedProductSave.id);
     }},
    {"selected_save_enabled",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.selectedProductSave.enabled);
     }},
    {"selected_save_status",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.selectedProductSave.status);
     }},
    {"save_slot_browser_mode",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.saveSlotBrowserMode);
     }},
    {"save_slot_ring_count",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.saveSlotRingCount);
     }},
    {"save_slot_ring_selected_index",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.saveSlotRingSelectedIndex);
     }},
    {"save_slot_ring_selected_id",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.saveSlotRingSelectedId);
     }},
    {"save_slot_ring_selected_status",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.saveSlotRingSelectedStatus);
     }},
    {"save_slot_action_command",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.saveSlotActionCommand);
     }},
    {"save_slot_action_enabled",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.saveSlotActionEnabled);
     }},
    {"save_slot_action_confirmation_required",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.saveSlotActionConfirmationRequired);
     }},
    {"save_slot_action_status",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.saveSlotActionStatus);
     }},
    {"save_flow_operation",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.saveFlow.operation);
     }},
    {"save_flow_source_surface",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.saveFlow.sourceSurface);
     }},
    {"save_flow_status",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.saveFlow.status);
     }},
    {"save_flow_reason_code",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.saveFlow.reasonCode);
     }},
    {"save_flow_affected_slot_id",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.saveFlow.affectedSlotId);
     }},
    {"save_flow_active_count_before",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.saveFlow.activeCountBefore);
     }},
    {"save_flow_active_count_after",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.saveFlow.activeCountAfter);
     }},
    {"save_flow_deleted_count_after",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.saveFlow.deletedCountAfter);
     }},
    {"save_flow_selected_slot_after",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.saveFlow.selectedSlotAfter);
     }},
    {"save_delete_confirmation_open",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.saveDelete.confirmationOpen);
     }},
    {"save_delete_candidate_id",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.saveDelete.candidateId);
     }},
    {"save_delete_candidate_enabled",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.saveDelete.candidateEnabled);
     }},
    {"save_delete_status",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.saveDelete.status);
     }},
    {"save_delete_reason_code",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.saveDelete.reasonCode);
     }},
    {"save_delete_type",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.saveDelete.type);
     }},
    {"save_delete_recoverable",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.saveDelete.recoverable);
     }},
    {"save_delete_executed",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.saveDelete.executed);
     }},
    {"deleted_save_browser_open",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.deletedSaveBrowserOpen);
     }},
    {"deleted_save_count",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.deletedSaveCount);
     }},
    {"deleted_compatible_save_count",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.deletedCompatibleSaveCount);
     }},
    {"deleted_selected_save_id",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.deletedSelectedSaveId);
     }},
    {"deleted_selected_save_enabled",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.deletedSelectedSaveEnabled);
     }},
    {"deleted_selected_save_status",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.deletedSelectedSaveStatus);
     }},
    {"save_recover_status",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.saveRecover.status);
     }},
    {"save_recover_reason_code",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.saveRecover.reasonCode);
     }},
    {"save_recover_executed",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.saveRecover.executed);
     }},
    {"save_recover_save_id",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.saveRecover.saveId);
     }},
    {"save_recover_snapshot_recovered",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.saveRecover.snapshotRecovered);
     }},
    {"save_recover_snapshot_missing",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.saveRecover.snapshotMissing);
     }},
    {"product_save_load_previous_hash",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.productSaveLoadResult.previousHash);
     }},
    {"product_save_load_loaded_hash",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.productSaveLoadResult.loadedHash);
     }},
    {"product_save_load_session_loaded",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.window.saveSession.productSaveLoadResult.sessionLoaded);
     }},
}};

}  // namespace

void appendProductSaveStateFields(
    RenderReceipt& receipt,
    const FrontendState& frontend,
    const ProductAppWindowState& window,
    const creative::CreativeActiveIdentity& creativeIdentity) {
  const SaveStateReceiptContext context{
      frontend,
      window,
      creativeIdentity,
  };

  for (const SaveStateReceiptFieldRow& row : kSaveStateReceiptFields) {
    row.append(receipt, context, row.key);
  }
}


}  // namespace iggy3d
