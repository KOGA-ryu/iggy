#include "app/iggy3d/ReceiptBuilder.hpp"

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
  window.physicsMovementPlanner.requested = requested;
  window.physicsMovementPlanner.used = used;
  window.physicsMovementPlanner.status = std::move(status);
  window.physicsMovementPlanner.reasonCode = window.physicsMovementPlanner.status;
}

std::string_view creativeToolReceiptName(creative::Tool tool) noexcept;

ProductCreativeBakedRoomRefreshDiagnostics&
uiCommandBakedRoomRefreshFields(ProductAppWindowState& window) noexcept {
  return window.creativeUiCommand.bakedRoomRefresh;
}

ProductCreativeBakedRoomRefreshDiagnostics&
autoBakedRoomRefreshFields(ProductAppWindowState& window) noexcept {
  return window.creativeBakedRoomAutoRefresh;
}

void resetProductCreativeBakedRoomRefreshDiagnostics(
    ProductCreativeBakedRoomRefreshDiagnostics& fields) {
  fields = ProductCreativeBakedRoomRefreshDiagnostics{};
}

void copyProductCreativeBakedRoomRefreshDiagnostics(
    ProductCreativeBakedRoomRefreshDiagnostics& fields,
    const ProductCreativeBakedActiveRoomRefreshResult& refresh) {
  fields.requested = true;
  fields.accepted = refresh.accepted;
  fields.clearedActiveRoom = refresh.clearedActiveRoom;
  fields.status = refresh.status;
  fields.reasonCode = refresh.reasonCode;
  fields.bakeMeasured = refresh.bakeMeasured;
  fields.bakeElapsedMicroseconds = refresh.bakeElapsedMicroseconds;
  fields.bakedDocumentRevision = refresh.bakedDocumentRevision;
  fields.staticMeshCount = refresh.staticMeshCount;
  fields.anchorCount = refresh.anchorCount;
  fields.spatialSurfaceCount = refresh.spatialSurfaceCount;
  fields.collisionReady = refresh.collisionReady;
  fields.collisionQuerySurfaceCount = refresh.collisionQuerySurfaceCount;
}

void copyProductCreativeUiCommandMutationDiagnostics(
    ProductCreativeUiCommandMutationDiagnostics& fields,
    const ProductCreativeUiCommandFrameReceipt& receipt) {
  fields.requested = receipt.mutationRequested;
  fields.accepted = receipt.mutationAccepted;
  fields.changed = receipt.mutationChanged;
  fields.status = std::string(creative::toString(receipt.mutationStatus));
  fields.documentStatus =
      std::string(creative::toString(receipt.documentMutationStatus));
  fields.kind = std::string(creative::toString(receipt.mutationKind));
  fields.target = receipt.mutationTarget.value;
  fields.objectId = receipt.mutationObjectId;
  fields.objectKind = std::string(creative::toString(receipt.mutationObjectKind));
  fields.visibleBefore = receipt.visibleBefore;
  fields.visibleAfter = receipt.visibleAfter;
  fields.lockedBefore = receipt.lockedBefore;
  fields.lockedAfter = receipt.lockedAfter;
  fields.revisionBefore = receipt.revisionBefore;
  fields.revisionAfter = receipt.revisionAfter;
  fields.message =
      receipt.mutationMessage.empty() ? "none" : receipt.mutationMessage;
}

void copyProductCreativeUiCommandCreateDiagnostics(
    ProductCreativeUiCommandCreateDiagnostics& fields,
    const ProductCreativeUiCommandFrameReceipt& receipt) {
  fields.requested = receipt.createRequested;
  fields.accepted = receipt.createAccepted;
  fields.changed = receipt.createChanged;
  fields.status = std::string(creative::toString(receipt.createStatus));
  fields.objectId = receipt.createObjectId;
  fields.objectKind = std::string(creative::toString(receipt.createObjectKind));
  fields.objectName =
      receipt.createObjectName.empty() ? "none" : receipt.createObjectName;
  fields.revisionBefore = receipt.createRevisionBefore;
  fields.revisionAfter = receipt.createRevisionAfter;
  fields.dirtyFlags = receipt.createDirtyFlags;
  fields.message = receipt.createMessage.empty() ? "none" : receipt.createMessage;
  fields.reasonCode =
      receipt.createReasonCode.empty() ? "none" : receipt.createReasonCode;
}

void copyProductCreativeUiCommandDeleteDiagnostics(
    ProductCreativeUiCommandDeleteDiagnostics& fields,
    const ProductCreativeUiCommandFrameReceipt& receipt) {
  fields.requested = receipt.deleteRequested;
  fields.accepted = receipt.deleteAccepted;
  fields.changed = receipt.deleteChanged;
  fields.removed = receipt.deleteRemoved;
  fields.objectId = receipt.deleteObjectId;
  fields.objectKind = std::string(creative::toString(receipt.deleteObjectKind));
  fields.objectName =
      receipt.deleteObjectName.empty() ? "none" : receipt.deleteObjectName;
  fields.revisionBefore = receipt.deleteRevisionBefore;
  fields.revisionAfter = receipt.deleteRevisionAfter;
  fields.dirtyFlags = receipt.deleteDirtyFlags;
  fields.status = receipt.deleteStatus.empty() ? "Unknown" : receipt.deleteStatus;
  fields.message = receipt.deleteMessage.empty() ? "none" : receipt.deleteMessage;
  fields.reasonCode =
      receipt.deleteReasonCode.empty() ? "none" : receipt.deleteReasonCode;
}

void copyProductCreativeUiCommandUndoDiagnostics(
    ProductCreativeUiCommandUndoDiagnostics& fields,
    const ProductCreativeUiCommandFrameReceipt& receipt) {
  fields.requested = receipt.undoRequested;
  fields.accepted = receipt.undoAccepted;
  fields.changed = receipt.undoChanged;
  fields.hadSnapshot = receipt.undoHadSnapshot;
  fields.documentId = receipt.undoDocumentId;
  fields.revisionBefore = receipt.undoRevisionBefore;
  fields.revisionAfter = receipt.undoRevisionAfter;
  fields.objectCountBefore = receipt.undoObjectCountBefore;
  fields.objectCountAfter = receipt.undoObjectCountAfter;
  fields.depthBefore = receipt.undoDepthBefore;
  fields.depthAfter = receipt.undoDepthAfter;
  fields.status =
      receipt.undoStatus.empty() ? "creative_undo_not_requested"
                                 : receipt.undoStatus;
  fields.message = receipt.undoMessage.empty() ? "none" : receipt.undoMessage;
  fields.reasonCode =
      receipt.undoReasonCode.empty() ? "none" : receipt.undoReasonCode;
}

void copyProductCreativeUiCommandRoomShellDiagnostics(
    ProductCreativeUiCommandRoomShellDiagnostics& fields,
    const ProductCreativeUiCommandFrameReceipt& receipt) {
  fields.requested = receipt.shellRequested;
  fields.accepted = receipt.shellAccepted;
  fields.changed = receipt.shellChanged;
  fields.roomObjectId = receipt.shellRoomObjectId;
  fields.generatedObjectCount = receipt.shellGeneratedObjectCount;
  fields.removedObjectCount = receipt.shellRemovedObjectCount;
  fields.floorCount = receipt.shellFloorCount;
  fields.wallCount = receipt.shellWallCount;
  fields.revisionBefore = receipt.shellRevisionBefore;
  fields.revisionAfter = receipt.shellRevisionAfter;
  fields.status =
      receipt.shellStatus.empty() ? "creative_room_shell_not_requested"
                                  : receipt.shellStatus;
  fields.reasonCode =
      receipt.shellReasonCode.empty() ? "none" : receipt.shellReasonCode;
  fields.message = receipt.shellMessage.empty() ? "none" : receipt.shellMessage;
}

void copyProductCreativeUiCommandDiagnostics(
    ProductCreativeUiCommandDiagnostics& fields,
    const ProductCreativeUiCommandFrameReceipt& receipt) {
  fields.requested = receipt.requested;
  fields.facadeAvailable = receipt.facadeAvailable;
  fields.inputConsumed = receipt.inputConsumed;
  fields.inputEnabled = receipt.inputEnabled;
  fields.accepted = receipt.accepted;
  fields.changed = receipt.changed;
  fields.kind =
      std::string(productCreativeUiCommandKindReceiptName(receipt.commandKind));
  fields.tool =
      receipt.commandKind == ProductCreativeUiCommandKind::SetActiveTool
          ? std::string(creativeToolReceiptName(receipt.commandTool))
          : std::string("none");
  fields.objectKind = std::string(creative::toString(receipt.commandObjectKind));
  fields.toolBefore = std::string(creativeToolReceiptName(receipt.toolBefore));
  fields.toolAfter = std::string(creativeToolReceiptName(receipt.toolAfter));
  fields.semanticId = receipt.semanticId.empty() ? "none" : receipt.semanticId;
  fields.status = receipt.status;
  fields.reasonCode = receipt.reasonCode;
  copyProductCreativeUiCommandMutationDiagnostics(fields.mutation, receipt);
  copyProductCreativeUiCommandCreateDiagnostics(fields.create, receipt);
  copyProductCreativeUiCommandDeleteDiagnostics(fields.deleteObject, receipt);
  copyProductCreativeUiCommandUndoDiagnostics(fields.undo, receipt);
  copyProductCreativeUiCommandRoomShellDiagnostics(fields.shell, receipt);
}

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

void resetProductCreativeUiCommandBakedRoomRefresh(
    ProductAppWindowState& window) {
  resetProductCreativeBakedRoomRefreshDiagnostics(
      uiCommandBakedRoomRefreshFields(window));
}

void resetProductCreativeBakedRoomAutoRefresh(ProductAppWindowState& window) {
  resetProductCreativeBakedRoomRefreshDiagnostics(
      autoBakedRoomRefreshFields(window));
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

std::string_view creativeToolReceiptName(creative::Tool tool) noexcept {
  switch (tool) {
    case creative::Tool::Select:
      return "Select";
    case creative::Tool::Move:
      return "Move";
    case creative::Tool::Measure:
      return "Measure";
    case creative::Tool::Navigate:
      return "Navigate";
  }
  return "Unknown";
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
  window.creativeUiProjection.requested = receipt.requested;
  window.creativeUiProjection.ready = receipt.ready;
  window.creativeUiProjection.partial = receipt.partial;
  window.creativeUiProjection.status = std::string(receipt.status);
  window.creativeUiProjection.reasonCode = std::string(receipt.reasonCode);
  window.creativeUiProjection.usedModel = receipt.usedModel;
  window.creativeUiProjection.usedFacade = receipt.usedFacade;
  window.creativeUiProjection.virtualWidth = receipt.virtualWidth;
  window.creativeUiProjection.virtualHeight = receipt.virtualHeight;
  window.creativeUiProjection.theme =
      std::string(productUiThemeReceiptName(receipt.theme));
  window.creativeUiProjection.panelCount = receipt.panelCount;
  window.creativeUiProjection.modelRowCount = receipt.modelRowCount;
  window.creativeUiProjection.primitiveCount = receipt.primitiveCount;
  window.creativeUiProjection.textCount = receipt.textCount;
  window.creativeUiProjection.rectCount = receipt.rectCount;
  window.creativeUiProjection.rowCount = receipt.rowCount;
  window.creativeUiProjection.disabledRowCount = receipt.disabledRowCount;
  window.creativeUiProjection.hitRegionCount = receipt.hitRegionCount;
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

  if (receipt.clickPresent) {
    window.creativeUiLast.clickSeen = true;
    window.creativeUiLast.clickX = floatReceiptValue(receipt.clickX);
    window.creativeUiLast.clickY = floatReceiptValue(receipt.clickY);
    window.creativeUiLast.inputHit = receipt.hit;
    window.creativeUiLast.inputConsumed = receipt.consumed;
    window.creativeUiLast.inputStatus = receipt.status;
    window.creativeUiLast.inputSemanticId =
        receipt.semanticId.empty() ? "none" : receipt.semanticId;
  }
}

void recordProductCreativeUiDownstreamClick(
    ProductAppWindowState& window,
    const ProductCreativeUiDownstreamClickReceipt& receipt) {
  window.creativeUiInputDownstreamClickRequested = receipt.requested;
  window.creativeUiInputDownstreamClickPresent = receipt.clickPresent;
  window.creativeUiInputDownstreamClickHigherPriority =
      receipt.higherPriorityUiConsumed;
  window.creativeUiInputDownstreamClickSuppressed = receipt.suppressed;
  window.creativeUiInputDownstreamClickStatus = receipt.status;
  window.creativeUiInputDownstreamClickReasonCode = receipt.reasonCode;
}

void recordProductCreativeUiCommandFrame(
    ProductAppWindowState& window,
    const ProductCreativeUiCommandFrameReceipt& receipt) {
  copyProductCreativeUiCommandDiagnostics(window.creativeUiCommand, receipt);
  resetProductCreativeUiCommandBakedRoomRefresh(window);

  const bool commandTouchedCreativeState =
      receipt.inputClickPresent ||
      receipt.commandKind != ProductCreativeUiCommandKind::None ||
      receipt.accepted || receipt.changed || receipt.createRequested ||
      receipt.mutationRequested ||
      (receipt.inputConsumed && !receipt.semanticId.empty());
  if (commandTouchedCreativeState) {
    window.creativeUiLast.commandKind =
        std::string(productCreativeUiCommandKindReceiptName(receipt.commandKind));
    window.creativeUiLast.commandStatus = receipt.status;
    window.creativeUiLast.commandCreateRequested = receipt.createRequested;
    window.creativeUiLast.commandCreateAccepted = receipt.createAccepted;
    window.creativeUiLast.commandCreateChanged = receipt.createChanged;
    window.creativeUiLast.commandCreateObjectId = receipt.createObjectId;
  }
}

void recordProductCreativeUiBakedRoomRefresh(
    ProductAppWindowState& window,
    const ProductCreativeBakedActiveRoomRefreshResult& refresh) {
  copyProductCreativeBakedRoomRefreshDiagnostics(
      uiCommandBakedRoomRefreshFields(window), refresh);
}

void recordProductCreativeBakedRoomAutoRefresh(
    ProductAppWindowState& window,
    const ProductCreativeBakedActiveRoomRefreshResult& refresh) {
  copyProductCreativeBakedRoomRefreshDiagnostics(
      autoBakedRoomRefreshFields(window), refresh);
}

void recordProductCreativeDocumentRevisionFrame(
    ProductAppWindowState& window,
    bool observed,
    std::uint64_t documentIdBefore,
    std::uint64_t revisionBefore,
    std::uint64_t documentIdAfter,
    std::uint64_t revisionAfter) {
  resetProductCreativeBakedRoomAutoRefresh(window);
  window.creativeDocumentRevisionObserved = observed;
  window.creativeDocumentChangedThisFrame = false;
  window.creativeDocumentRevisionDocumentId = observed ? documentIdAfter : 0U;
  window.creativeDocumentRevisionBeforeFrame = observed ? revisionBefore : 0U;
  window.creativeDocumentRevisionAfterFrame = observed ? revisionAfter : 0U;
  if (!observed) {
    return;
  }

  const bool documentReplaced = documentIdBefore != documentIdAfter;
  const bool revisionChanged = revisionBefore != revisionAfter;
  if (!documentReplaced && !revisionChanged) {
    return;
  }

  window.creativeDocumentChangedThisFrame = true;
  window.creativeBakedRoomStale = true;
  window.creativeBakedRoomStaleDocumentId = documentIdAfter;
  window.creativeBakedRoomStaleRevision = revisionAfter;
  window.creativeBakedRoomStaleStatus =
      documentReplaced ? "creative_baked_room_stale_document_replaced"
                       : "creative_baked_room_stale_document_changed";
  window.creativeBakedRoomStaleReasonCode =
      window.creativeBakedRoomStaleStatus;
}

void recordProductCreativeBakedRoomFresh(ProductAppWindowState& window,
                                         std::uint64_t documentId,
                                         std::uint64_t revision) {
  window.creativeBakedRoomStale = false;
  window.creativeBakedRoomStaleDocumentId = documentId;
  window.creativeBakedRoomStaleRevision = revision;
  window.creativeBakedRoomStaleStatus = "creative_baked_room_fresh";
  window.creativeBakedRoomStaleReasonCode = "creative_baked_room_fresh";
}

void recordProductCreativeViewportPickFrame(
    ProductAppWindowState& window,
    const ProductCreativeViewportPickFrameReceipt& receipt) {
  window.creativeViewportPickRequested = receipt.requested;
  window.creativeViewportPickActive = receipt.active;
  window.creativeViewportPickClickPresent = receipt.clickPresent;
  window.creativeViewportPickClickSuppressed =
      receipt.downstreamClickSuppressed;
  window.creativeViewportPickFacadeAvailable = receipt.facadeAvailable;
  window.creativeViewportPickSourceAvailable = receipt.sourceAvailable;
  window.creativeViewportPickProjected = receipt.projected;
  window.creativeViewportPickPicked = receipt.picked;
  window.creativeViewportPickObjectCount = receipt.objectCount;
  window.creativeViewportPickProjectionCellCount =
      receipt.projectionCellCount;
  window.creativeViewportPickStatus = receipt.status;
  window.creativeViewportPickReasonCode = receipt.reasonCode;
  window.creativeViewportPickPickStatus =
      std::string(creative::toString(receipt.pickStatus));
  window.creativeViewportPickMessage =
      receipt.pickMessage.empty() ? "none" : receipt.pickMessage;
  window.creativeViewportPickCoordX = receipt.coord.x;
  window.creativeViewportPickCoordY = receipt.coord.y;
  window.creativeViewportPickCoordZ = receipt.coord.z;
  window.creativeViewportPickGridIndex = receipt.gridIndex;
  window.creativeViewportPickObjectId = receipt.objectId;
  window.creativeViewportPickObjectKind =
      std::string(creative::toString(receipt.objectKind));
  window.creativeViewportPickOccupancyKind =
      std::string(creative::toString(receipt.occupancyKind));
  window.creativeViewportPickTarget = receipt.target.value;
  window.creativeViewportPickCellIndex =
      static_cast<std::uint64_t>(receipt.cellIndex);
}

void recordProductCreativeWireframeFrame(
    ProductAppWindowState& window,
    const ProductCreativeWireframeFrameReceipt& receipt) {
  window.creativeWireframeRequested = receipt.requested;
  window.creativeWireframeActive = receipt.active;
  window.creativeWireframeFacadeAvailable = receipt.facadeAvailable;
  window.creativeWireframeDocumentAvailable = receipt.documentAvailable;
  window.creativeWireframeSourceAvailable = receipt.sourceAvailable;
  window.creativeWireframeObjectCount = receipt.objectCount;
  window.creativeWireframeVisibleObjectCount = receipt.visibleObjectCount;
  window.creativeWireframeItemCount = receipt.itemCount;
  window.creativeWireframeSegmentCount = receipt.segmentCount;
  window.creativeWireframeBoxItemCount = receipt.boxItemCount;
  window.creativeWireframeLineItemCount = receipt.lineItemCount;
  window.creativeWireframePointItemCount = receipt.pointItemCount;
  window.creativeWireframeSkippedDegenerateCount =
      receipt.skippedDegenerateCount;
  window.creativeWireframeStatus = receipt.status;
  window.creativeWireframeReasonCode = receipt.reasonCode;
  window.creativeWireframeWireframeStatus =
      std::string(creative::toString(receipt.wireframeStatus));
  window.creativeWireframeWireframeReasonCode =
      receipt.wireframeReasonCode.empty() ? "none"
                                          : receipt.wireframeReasonCode;
  window.creativeWireframeSegmentStatus =
      std::string(creative::toString(receipt.segmentStatus));
  window.creativeWireframeSegmentReasonCode =
      receipt.segmentReasonCode.empty() ? "none" : receipt.segmentReasonCode;
  window.creativeWireframeDebugLineRequested = receipt.debugLineRequested;
  window.creativeWireframeDebugLineSourceAvailable =
      receipt.debugLineSourceAvailable;
  window.creativeWireframeDebugLineInputSegmentCount =
      receipt.debugLineInputSegmentCount;
  window.creativeWireframeDebugLineCount = receipt.debugLineCount;
  window.creativeWireframeDebugLineSkippedDegenerateCount =
      receipt.debugLineSkippedDegenerateCount;
  window.creativeWireframeDebugLineStatus =
      std::string(toString(receipt.debugLineStatus));
  window.creativeWireframeDebugLineReasonCode =
      receipt.debugLineReasonCode.empty() ? "none" : receipt.debugLineReasonCode;
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
  const ProductCreativeSurfaceKind creativeSurface =
      productCreativeSurfaceKindForWindow(frontend, window);
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
  appendReceiptField(receipt, "dev_tools_category",
                     frontendDevToolsCategoryName(frontend.devToolsCategory));
  appendReceiptField(receipt, "launch_action", window.launchAction);
  appendReceiptField(receipt, "launch_status", window.launchStatus);
  appendReceiptField(receipt, "package_load_status", window.packageLoadStatus);
  appendReceiptField(receipt, "startup_package_path",
                     window.startup.packagePath);
  appendReceiptField(receipt, "startup_package_lookup_measured",
                     window.startup.packageLookupMeasured);
  appendReceiptField(receipt, "startup_package_lookup_us",
                     window.startup.packageLookupMicroseconds);
  appendReceiptField(receipt, "startup_package_lookup_status",
                     window.startup.packageLookupStatus);
  appendReceiptField(receipt, "startup_package_load_measured",
                     window.startup.packageLoadMeasured);
  appendReceiptField(receipt, "startup_package_load_us",
                     window.startup.packageLoadMicroseconds);
  appendReceiptField(receipt, "startup_package_load_status",
                     window.startup.packageLoadStatus);
  appendReceiptField(receipt, "startup_runtime_session_create_measured",
                     window.startup.runtimeSessionCreateMeasured);
  appendReceiptField(receipt, "startup_runtime_session_create_us",
                     window.startup.runtimeSessionCreateMicroseconds);
  appendReceiptField(receipt, "startup_runtime_session_create_status",
                     window.startup.runtimeSessionCreateStatus);
  appendReceiptField(receipt, "startup_save_catalog_scan_measured",
                     saves.scanMeasured);
  appendReceiptField(receipt, "startup_save_catalog_scan_us",
                     saves.scanMicroseconds);
  appendReceiptField(receipt, "startup_save_catalog_scan_entry_count",
                     saves.scanEntryCount);
  appendReceiptField(receipt, "startup_save_catalog_scan_status",
                     saves.scanStatus);
  appendReceiptField(receipt, "startup_creative_world_id_scan_measured",
                     window.startup.creativeWorldIdScanMeasured);
  appendReceiptField(receipt, "startup_creative_world_id_scan_us",
                     window.startup.creativeWorldIdScanMicroseconds);
  appendReceiptField(receipt, "startup_creative_world_id_scan_entry_count",
                     window.startup.creativeWorldIdScanEntryCount);
  appendReceiptField(receipt, "startup_creative_world_id_scan_status",
                     window.startup.creativeWorldIdScanStatus);
  appendReceiptField(receipt, "startup_creative_document_id_scan_measured",
                     window.startup.creativeDocumentIdScanMeasured);
  appendReceiptField(receipt, "startup_creative_document_id_scan_us",
                     window.startup.creativeDocumentIdScanMicroseconds);
  appendReceiptField(receipt, "startup_creative_document_id_scan_entry_count",
                     window.startup.creativeDocumentIdScanEntryCount);
  appendReceiptField(receipt, "startup_creative_document_id_scan_status",
                     window.startup.creativeDocumentIdScanStatus);
  appendReceiptField(receipt, "startup_creative_ui_first_frame_measured",
                     window.startup.creativeUiFirstFrameMeasured);
  appendReceiptField(receipt, "startup_creative_ui_first_frame_us",
                     window.startup.creativeUiFirstFrameMicroseconds);
  appendReceiptField(receipt, "startup_creative_ui_first_frame_status",
                     window.startup.creativeUiFirstFrameStatus);
  appendReceiptField(receipt, "startup_creative_wireframe_first_frame_measured",
                     window.startup.creativeWireframeFirstFrameMeasured);
  appendReceiptField(receipt, "startup_creative_wireframe_first_frame_us",
                     window.startup.creativeWireframeFirstFrameMicroseconds);
  appendReceiptField(receipt, "startup_creative_wireframe_first_frame_status",
                     window.startup.creativeWireframeFirstFrameStatus);
  appendReceiptField(receipt, "startup_vulkan_renderer_init_measured",
                     window.startup.vulkanRendererInitMeasured);
  appendReceiptField(receipt, "startup_vulkan_renderer_init_us",
                     window.startup.vulkanRendererInitMicroseconds);
  appendReceiptField(receipt, "startup_vulkan_renderer_init_status",
                     window.startup.vulkanRendererInitStatus);
  appendReceiptField(receipt, "startup_vulkan_first_submit_measured",
                     window.startup.vulkanFirstSubmitMeasured);
  appendReceiptField(receipt, "startup_vulkan_first_submit_us",
                     window.startup.vulkanFirstSubmitMicroseconds);
  appendReceiptField(receipt, "startup_vulkan_first_submit_status",
                     window.startup.vulkanFirstSubmitStatus);
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
  appendReceiptField(receipt, "active_creative_save_id",
                     window.activeCreative.saveId);
  appendReceiptField(receipt, "active_creative_save_path",
                     window.activeCreative.savePath);
  appendReceiptField(receipt, "active_creative_world_id",
                     window.activeCreative.worldId);
  appendReceiptField(receipt, "active_creative_document_id",
                     window.activeCreative.documentId);
  appendReceiptField(receipt, "active_creative_object_count",
                     window.activeCreative.objectCount);
  appendReceiptField(receipt, "active_creative_next_object_id",
                     window.activeCreative.nextObjectId);
  appendReceiptField(receipt, "active_creative_save_status",
                     window.activeCreative.saveStatus);
  appendReceiptField(receipt, "active_creative_save_reason_code",
                     window.activeCreative.saveReasonCode);
  appendReceiptField(receipt, "active_creative_save_dirty_flags_before",
                     window.activeCreative.saveDirtyFlagsBefore);
  appendReceiptField(receipt, "active_creative_save_dirty_flags_drained",
                     window.activeCreative.saveDirtyFlagsDrained);
  appendReceiptField(receipt, "active_creative_save_dirty_flags_after",
                     window.activeCreative.saveDirtyFlagsAfter);
  appendReceiptField(receipt, "active_creative_save_saved_at_utc",
                     window.activeCreative.saveSavedAtUtc);
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
  appendReceiptField(receipt, "selected_save_id", window.selectedProductSave.id);
  appendReceiptField(receipt, "selected_save_enabled",
                     window.selectedProductSave.enabled);
  appendReceiptField(receipt, "selected_save_status",
                     window.selectedProductSave.status);
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
  appendReceiptField(receipt, "save_flow_operation", window.saveFlow.operation);
  appendReceiptField(receipt,
                     "save_flow_source_surface",
                     window.saveFlow.sourceSurface);
  appendReceiptField(receipt, "save_flow_status", window.saveFlow.status);
  appendReceiptField(receipt,
                     "save_flow_reason_code",
                     window.saveFlow.reasonCode);
  appendReceiptField(receipt,
                     "save_flow_affected_slot_id",
                     window.saveFlow.affectedSlotId);
  appendReceiptField(receipt,
                     "save_flow_active_count_before",
                     window.saveFlow.activeCountBefore);
  appendReceiptField(receipt,
                     "save_flow_active_count_after",
                     window.saveFlow.activeCountAfter);
  appendReceiptField(receipt,
                     "save_flow_deleted_count_after",
                     window.saveFlow.deletedCountAfter);
  appendReceiptField(receipt,
                     "save_flow_selected_slot_after",
                     window.saveFlow.selectedSlotAfter);
  appendReceiptField(receipt, "save_delete_confirmation_open",
                     window.saveDelete.confirmationOpen);
  appendReceiptField(receipt, "save_delete_candidate_id",
                     window.saveDelete.candidateId);
  appendReceiptField(receipt, "save_delete_candidate_enabled",
                     window.saveDelete.candidateEnabled);
  appendReceiptField(receipt, "save_delete_status", window.saveDelete.status);
  appendReceiptField(receipt, "save_delete_reason_code",
                     window.saveDelete.reasonCode);
  appendReceiptField(receipt, "save_delete_type", window.saveDelete.type);
  appendReceiptField(receipt, "save_delete_recoverable",
                     window.saveDelete.recoverable);
  appendReceiptField(receipt, "save_delete_executed", window.saveDelete.executed);
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
  appendReceiptField(receipt, "save_recover_status", window.saveRecover.status);
  appendReceiptField(receipt, "save_recover_reason_code",
                     window.saveRecover.reasonCode);
  appendReceiptField(receipt, "save_recover_executed",
                     window.saveRecover.executed);
  appendReceiptField(receipt, "save_recover_save_id", window.saveRecover.saveId);
  appendReceiptField(receipt, "save_recover_snapshot_recovered",
                     window.saveRecover.snapshotRecovered);
  appendReceiptField(receipt, "save_recover_snapshot_missing",
                     window.saveRecover.snapshotMissing);
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
                     window.gameplayReset.triggered);
  appendReceiptField(receipt, "gameplay_reset_status",
                     window.gameplayReset.status);
  appendReceiptField(receipt, "gameplay_reset_reason_code",
                     window.gameplayReset.reasonCode);
  appendReceiptField(receipt, "gameplay_reset_spawn_anchor_id",
                     window.gameplayReset.spawnAnchorId);
  appendReceiptField(receipt, "gameplay_reset_source_anchor_id",
                     window.gameplayReset.sourceAnchorId);
  appendReceiptField(receipt, "gameplay_reset_start_y",
                     floatReceiptValue(window.gameplayReset.startY));
  appendReceiptField(receipt, "gameplay_reset_final_y",
                     floatReceiptValue(window.gameplayReset.finalY));
  appendReceiptField(receipt, "gameplay_traversal_requested",
                     window.gameplayTraversal.requested);
  appendReceiptField(receipt, "gameplay_traversal_consumed",
                     window.gameplayTraversal.consumed);
  appendReceiptField(receipt, "gameplay_traversal_accepted",
                     window.gameplayTraversal.accepted);
  appendReceiptField(receipt, "gameplay_traversal_fallback_jump_allowed",
                     window.gameplayTraversal.fallbackJumpAllowed);
  appendReceiptField(receipt, "gameplay_traversal_status",
                     window.gameplayTraversal.status);
  appendReceiptField(receipt, "gameplay_traversal_reason_code",
                     window.gameplayTraversal.reasonCode);
  appendReceiptField(receipt, "gameplay_traversal_mechanic",
                     window.gameplayTraversal.mechanic);
  appendReceiptField(receipt, "gameplay_traversal_slot_id",
                     window.gameplayTraversal.slotId);
  appendReceiptField(receipt, "gameplay_traversal_target_id",
                     window.gameplayTraversal.targetId);
  appendReceiptField(receipt, "gameplay_traversal_landing_surface_id",
                     window.gameplayTraversal.landingSurfaceId);
  appendReceiptField(receipt, "gameplay_traversal_start_x",
                     floatReceiptValue(window.gameplayTraversal.startX));
  appendReceiptField(receipt, "gameplay_traversal_start_y",
                     floatReceiptValue(window.gameplayTraversal.startY));
  appendReceiptField(receipt, "gameplay_traversal_start_z",
                     floatReceiptValue(window.gameplayTraversal.startZ));
  appendReceiptField(receipt, "gameplay_traversal_final_x",
                     floatReceiptValue(window.gameplayTraversal.finalX));
  appendReceiptField(receipt, "gameplay_traversal_final_y",
                     floatReceiptValue(window.gameplayTraversal.finalY));
  appendReceiptField(receipt, "gameplay_traversal_final_z",
                     floatReceiptValue(window.gameplayTraversal.finalZ));
  appendReceiptField(receipt, "gameplay_dash_requested",
                     window.gameplayDash.requested);
  appendReceiptField(receipt, "gameplay_dash_accepted",
                     window.gameplayDash.accepted);
  appendReceiptField(receipt, "gameplay_dash_status", window.gameplayDash.status);
  appendReceiptField(receipt, "gameplay_dash_reason_code",
                     window.gameplayDash.reasonCode);
  appendReceiptField(receipt, "gameplay_dash_speed_mps",
                     floatReceiptValue(window.gameplayDash.speedMetersPerSecond));
  appendReceiptField(receipt, "gameplay_dash_distance_meters",
                     floatReceiptValue(window.gameplayDash.distanceMeters));
  appendReceiptField(receipt, "gameplay_dash_cooldown_remaining_seconds",
                     floatReceiptValue(window.gameplayDash.cooldownRemainingSeconds));
  appendReceiptField(receipt, "gameplay_dash_direction_x",
                     floatReceiptValue(window.gameplayDash.directionX));
  appendReceiptField(receipt, "gameplay_dash_direction_z",
                     floatReceiptValue(window.gameplayDash.directionZ));
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
                     window.gameplayCollision.surfacesUsed);
  appendReceiptField(receipt, "gameplay_collision_surface_count",
                     window.gameplayCollision.surfaceCount);
  appendReceiptField(receipt, "physics_movement_planner_enabled",
                     window.physicsMovementPlanner.enabled);
  appendReceiptField(receipt, "physics_movement_planner_requested",
                     window.physicsMovementPlanner.requested);
  appendReceiptField(receipt, "physics_movement_planner_used",
                     window.physicsMovementPlanner.used);
  appendReceiptField(receipt, "physics_movement_planner_status",
                     window.physicsMovementPlanner.status);
  appendReceiptField(receipt, "physics_movement_planner_reason_code",
                     window.physicsMovementPlanner.reasonCode);
  appendReceiptField(receipt, "target_discovered", window.targetDiscovered);
  appendReceiptField(receipt, "gameplay_target_status",
                     window.gameplayTarget.status);
  appendReceiptField(receipt, "gameplay_target_action",
                     window.gameplayTarget.action);
  appendReceiptField(receipt, "gameplay_target_entity_id",
                     window.gameplayTarget.entityId);
  appendReceiptField(receipt, "gameplay_target_stable_name",
                     window.gameplayTarget.stableName);
  appendReceiptField(receipt, "gameplay_target_kind", window.gameplayTarget.kind);
  appendReceiptField(receipt, "gameplay_target_distance_meters",
                     floatReceiptValue(window.gameplayTarget.distanceMeters));
  appendReceiptField(receipt, "gameplay_target_supports_command",
                     window.gameplayTarget.supportsCommand);
  appendReceiptField(receipt, "gameplay_outcome_status",
                     window.gameplayOutcome.status);
  appendReceiptField(receipt, "gameplay_outcome_target_active_after",
                     window.gameplayOutcome.targetActiveAfter);
  appendReceiptField(receipt, "gameplay_outcome_inventory_changed",
                     window.gameplayOutcome.inventoryChanged);
  appendReceiptField(receipt, "gameplay_outcome_item_id",
                     window.gameplayOutcome.itemId);
  appendReceiptField(receipt, "gameplay_outcome_item_count",
                     window.gameplayOutcome.itemCount);
  appendReceiptField(receipt, "gameplay_outcome_objective_changed",
                     window.gameplayOutcome.objectiveChanged);
  appendReceiptField(receipt, "gameplay_outcome_event_count",
                     window.gameplayOutcome.eventCount);
  appendReceiptField(receipt, "session_outcome", window.sessionOutcome);
  appendReceiptField(receipt, "gameplay_tape_requested",
                     window.gameplayTape.requested);
  appendReceiptField(receipt, "gameplay_tape_loaded", window.gameplayTape.loaded);
  appendReceiptField(receipt, "gameplay_tape_path", window.gameplayTape.path);
  appendReceiptField(receipt, "gameplay_tape_status", window.gameplayTape.status);
  appendReceiptField(receipt, "gameplay_tape_reason_code",
                     window.gameplayTape.reasonCode);
  appendReceiptField(receipt, "gameplay_tape_line_count",
                     window.gameplayTape.lineCount);
  appendReceiptField(receipt, "gameplay_tape_step_count",
                     window.gameplayTape.stepCount);
  appendReceiptField(receipt, "gameplay_tape_executed_step_count",
                     window.gameplayTape.executedStepCount);
  appendReceiptField(receipt, "gameplay_tape_expected_rejected_step_count",
                     window.gameplayTape.expectedRejectedStepCount);
  appendReceiptField(receipt, "gameplay_tape_expected_blocked_step_count",
                     window.gameplayTape.expectedBlockedStepCount);
  appendReceiptField(receipt, "gameplay_tape_failed_step",
                     window.gameplayTape.failedStep);
  appendReceiptField(receipt, "gameplay_tape_failed_source_line",
                     window.gameplayTape.failedSourceLine);
  appendReceiptField(receipt, "gameplay_tape_failed_action",
                     window.gameplayTape.failedAction);
  appendReceiptField(receipt, "gameplay_tape_failed_target",
                     window.gameplayTape.failedTarget);
  appendReceiptField(receipt, "gameplay_tape_failed_rejection",
                     window.gameplayTape.failedRejection);
  appendReceiptField(receipt, "gameplay_tape_failed_movement_block",
                     window.gameplayTape.failedMovementBlock);
  appendReceiptField(receipt, "gameplay_tape_last_action",
                     window.gameplayTape.lastAction);
  appendReceiptField(receipt, "gameplay_tape_last_target",
                     window.gameplayTape.lastTarget);
  appendReceiptField(receipt, "gameplay_tape_last_movement_block",
                     window.gameplayTape.lastMovementBlock);
  appendReceiptField(receipt, "gameplay_tape_key_collected",
                     window.gameplayTape.keyCollected);
  appendReceiptField(receipt, "gameplay_tape_secret_door_opened",
                     window.gameplayTape.secretDoorOpened);
  appendReceiptField(receipt, "gameplay_tape_treasure_collected",
                     window.gameplayTape.treasureCollected);
  appendReceiptField(receipt, "gameplay_tape_npc_targetable",
                     window.gameplayTape.npcTargetable);
  appendReceiptField(receipt, "gameplay_tape_npc_defeated",
                     window.gameplayTape.npcDefeated);
  appendReceiptField(receipt, "gameplay_tape_exit_objective_complete",
                     window.gameplayTape.exitObjectiveComplete);
  appendReceiptField(receipt, "gameplay_tape_loop_complete",
                     window.gameplayTape.loopComplete);
  appendReceiptField(receipt, "gameplay_tape_ai_command_logged",
                     window.gameplayTape.aiCommandLogged);
  appendReceiptField(receipt, "gameplay_tape_ai_attack_logged",
                     window.gameplayTape.aiAttackLogged);
  appendReceiptField(receipt, "gameplay_tape_ai_wait_logged",
                     window.gameplayTape.aiWaitLogged);
  appendReceiptField(receipt, "gameplay_tape_ai_player_damaged",
                     window.gameplayTape.aiPlayerDamaged);
  appendReceiptField(receipt, "gameplay_tape_ai_player_hp_before",
                     static_cast<std::uint64_t>(window.gameplayTape.aiPlayerHpBefore));
  appendReceiptField(receipt, "gameplay_tape_ai_player_hp_after",
                     static_cast<std::uint64_t>(window.gameplayTape.aiPlayerHpAfter));
  appendReceiptField(receipt, "gameplay_tape_ai_actor_id",
                     window.gameplayTape.aiActorId);
  appendReceiptField(receipt, "gameplay_tape_ai_target_id",
                     window.gameplayTape.aiTargetId);
  appendReceiptField(receipt, "gameplay_tape_ai_behavior",
                     window.gameplayTape.aiBehavior);
  appendReceiptField(receipt, "gameplay_tape_ai_intent",
                     window.gameplayTape.aiIntent);
  appendReceiptField(receipt, "gameplay_reach_gate", window.gameplayReachGate);
  appendReceiptField(receipt, "gameplay_last_rejection", window.gameplayLastRejection);
  appendReceiptField(receipt, "interaction_executed", window.interactionExecuted);
  appendReceiptField(receipt, "attack_executed", window.attackExecuted);
  appendReceiptField(receipt, "product_transition_last_action",
                     window.productTransition.lastAction);
  appendReceiptField(receipt, "product_transition_status",
                     window.productTransition.status);
  appendReceiptField(receipt, "product_transition_returned_to_gameplay",
                     window.productTransition.returnedToGameplay);
  appendReceiptField(receipt, "product_transition_returned_to_title",
                     window.productTransition.returnedToTitle);
  appendReceiptField(receipt, "product_transition_session_preserved",
                     window.productTransition.sessionPreserved);
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
  appendReceiptField(receipt, "creative_navigate_active",
                     window.creativeNavigateActive);
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
                     window.automationControl.requested);
  appendReceiptField(receipt, "automation_control_loaded",
                     window.automationControl.loaded);
  appendReceiptField(receipt, "automation_control_path", window.automationControl.path);
  appendReceiptField(receipt, "automation_control_status",
                     window.automationControl.status);
  appendReceiptField(receipt, "automation_control_scope", window.automationControl.scope);
  appendReceiptField(receipt, "automation_control_line_count",
                     window.automationControl.lineCount);
  appendReceiptField(receipt, "automation_control_applied_count",
                     window.automationControl.appliedCount);
  appendReceiptField(receipt, "automation_control_last_key",
                     window.automationControl.lastKey);
  appendReceiptField(receipt, "automation_control_last_action",
                     window.automationControl.lastAction);
  appendReceiptField(receipt, "automation_control_last_owner",
                     menuOwnerName(window.automationControl.lastOwner));
  appendReceiptField(receipt, "automation_control_last_result",
                     window.automationControl.lastResult);
  appendReceiptField(receipt, "product_vulkan_renderer_requested",
                     window.productVulkanRenderer.requested);
  appendReceiptField(receipt, "product_vulkan_backend_built",
                     vulkanGameplayReadiness.backendBuilt);
  appendReceiptField(receipt, "product_vulkan_renderer_created",
                     window.productVulkanRenderer.created);
  appendReceiptField(receipt, "product_vulkan_renderer_ready",
                     window.productVulkanRenderer.ready);
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
                     window.productVulkanMenu.requested);
  appendReceiptField(receipt, "product_vulkan_menu_visible",
                     window.productVulkanMenu.visible);
  appendReceiptField(receipt, "product_vulkan_menu_status",
                     window.productVulkanMenu.status);
  appendReceiptField(receipt, "product_vulkan_menu_reason_code",
                     window.productVulkanMenu.reasonCode);
  appendReceiptField(receipt, "product_vulkan_menu_surface",
                     window.productVulkanMenu.surface);
  appendReceiptField(receipt, "product_vulkan_menu_ui_ready",
                     window.productVulkanMenu.uiReady);
  appendReceiptField(receipt, "product_vulkan_menu_ui_partial",
                     window.productVulkanMenu.uiPartial);
  appendReceiptField(receipt, "product_vulkan_menu_ui_status",
                     window.productVulkanMenu.uiStatus);
  appendReceiptField(receipt, "product_vulkan_menu_ui_reason_code",
                     window.productVulkanMenu.uiReasonCode);
  appendReceiptField(receipt, "product_vulkan_menu_ui_primitive_count",
                     window.productVulkanMenu.uiPrimitiveCount);
  appendReceiptField(receipt, "product_vulkan_menu_ui_text_count",
                     window.productVulkanMenu.uiTextCount);
  appendReceiptField(receipt, "product_vulkan_menu_ui_rect_count",
                     window.productVulkanMenu.uiRectCount);
  appendReceiptField(receipt, "product_vulkan_menu_ui_row_count",
                     window.productVulkanMenu.uiRowCount);
  appendReceiptField(receipt, "product_vulkan_menu_ui_selected_action",
                     window.productVulkanMenu.uiSelectedAction);
  appendReceiptField(receipt, "creative_ui_projection_requested",
                     window.creativeUiProjection.requested);
  appendReceiptField(receipt, "creative_ui_projection_ready",
                     window.creativeUiProjection.ready);
  appendReceiptField(receipt, "creative_ui_projection_partial",
                     window.creativeUiProjection.partial);
  appendReceiptField(receipt, "creative_ui_projection_status",
                     window.creativeUiProjection.status);
  appendReceiptField(receipt, "creative_ui_projection_reason_code",
                     window.creativeUiProjection.reasonCode);
  appendReceiptField(receipt, "creative_ui_projection_used_model",
                     window.creativeUiProjection.usedModel);
  appendReceiptField(receipt, "creative_ui_projection_used_facade",
                     window.creativeUiProjection.usedFacade);
  appendReceiptField(receipt, "creative_ui_projection_virtual_width",
                     static_cast<std::uint64_t>(
                         window.creativeUiProjection.virtualWidth));
  appendReceiptField(receipt, "creative_ui_projection_virtual_height",
                     static_cast<std::uint64_t>(
                         window.creativeUiProjection.virtualHeight));
  appendReceiptField(receipt, "creative_ui_projection_theme",
                     window.creativeUiProjection.theme);
  appendReceiptField(receipt, "creative_ui_projection_panel_count",
                     window.creativeUiProjection.panelCount);
  appendReceiptField(receipt, "creative_ui_projection_model_row_count",
                     window.creativeUiProjection.modelRowCount);
  appendReceiptField(receipt, "creative_ui_projection_primitive_count",
                     window.creativeUiProjection.primitiveCount);
  appendReceiptField(receipt, "creative_ui_projection_text_count",
                     window.creativeUiProjection.textCount);
  appendReceiptField(receipt, "creative_ui_projection_rect_count",
                     window.creativeUiProjection.rectCount);
  appendReceiptField(receipt, "creative_ui_projection_row_count",
                     window.creativeUiProjection.rowCount);
  appendReceiptField(receipt, "creative_ui_projection_disabled_row_count",
                     window.creativeUiProjection.disabledRowCount);
  appendReceiptField(receipt, "creative_ui_projection_hit_region_count",
                     window.creativeUiProjection.hitRegionCount);
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
  appendReceiptField(receipt, "creative_ui_last_click_seen",
                     window.creativeUiLast.clickSeen);
  appendReceiptField(receipt, "creative_ui_last_click_x",
                     window.creativeUiLast.clickX);
  appendReceiptField(receipt, "creative_ui_last_click_y",
                     window.creativeUiLast.clickY);
  appendReceiptField(receipt, "creative_ui_last_input_hit",
                     window.creativeUiLast.inputHit);
  appendReceiptField(receipt, "creative_ui_last_input_consumed",
                     window.creativeUiLast.inputConsumed);
  appendReceiptField(receipt, "creative_ui_last_input_status",
                     window.creativeUiLast.inputStatus);
  appendReceiptField(receipt, "creative_ui_last_input_semantic_id",
                     window.creativeUiLast.inputSemanticId);
  appendReceiptField(receipt, "creative_ui_last_command_kind",
                     window.creativeUiLast.commandKind);
  appendReceiptField(receipt, "creative_ui_last_command_status",
                     window.creativeUiLast.commandStatus);
  appendReceiptField(receipt,
                     "creative_ui_last_command_create_requested",
                     window.creativeUiLast.commandCreateRequested);
  appendReceiptField(receipt,
                     "creative_ui_last_command_create_accepted",
                     window.creativeUiLast.commandCreateAccepted);
  appendReceiptField(receipt,
                     "creative_ui_last_command_create_changed",
                     window.creativeUiLast.commandCreateChanged);
  appendReceiptField(receipt,
                     "creative_ui_last_command_create_object_id",
                     window.creativeUiLast.commandCreateObjectId);
  appendReceiptField(receipt,
                     "creative_ui_input_downstream_click_requested",
                     window.creativeUiInputDownstreamClickRequested);
  appendReceiptField(receipt,
                     "creative_ui_input_downstream_click_present",
                     window.creativeUiInputDownstreamClickPresent);
  appendReceiptField(receipt,
                     "creative_ui_input_downstream_click_higher_priority",
                     window.creativeUiInputDownstreamClickHigherPriority);
  appendReceiptField(receipt,
                     "creative_ui_input_downstream_click_suppressed",
                     window.creativeUiInputDownstreamClickSuppressed);
  appendReceiptField(receipt,
                     "creative_ui_input_downstream_click_status",
                     window.creativeUiInputDownstreamClickStatus);
  appendReceiptField(receipt,
                     "creative_ui_input_downstream_click_reason_code",
                     window.creativeUiInputDownstreamClickReasonCode);
  appendProductCreativeUiCommandFields(receipt, window.creativeUiCommand);
  appendProductCreativeBakedRoomAutoRefreshFields(
      receipt, window.creativeBakedRoomAutoRefresh);
  appendReceiptField(receipt, "creative_document_revision_observed",
                     window.creativeDocumentRevisionObserved);
  appendReceiptField(receipt, "creative_document_changed_this_frame",
                     window.creativeDocumentChangedThisFrame);
  appendReceiptField(receipt, "creative_document_revision_document_id",
                     window.creativeDocumentRevisionDocumentId);
  appendReceiptField(receipt, "creative_document_revision_before_frame",
                     window.creativeDocumentRevisionBeforeFrame);
  appendReceiptField(receipt, "creative_document_revision_after_frame",
                     window.creativeDocumentRevisionAfterFrame);
  appendReceiptField(receipt, "creative_undo_available",
                     window.creativeUndo.available);
  appendReceiptField(receipt, "creative_undo_depth",
                     window.creativeUndo.depth);
  appendReceiptField(receipt, "creative_baked_room_stale",
                     window.creativeBakedRoomStale);
  appendReceiptField(receipt, "creative_baked_room_stale_document_id",
                     window.creativeBakedRoomStaleDocumentId);
  appendReceiptField(receipt, "creative_baked_room_stale_revision",
                     window.creativeBakedRoomStaleRevision);
  appendReceiptField(receipt, "creative_baked_room_stale_status",
                     window.creativeBakedRoomStaleStatus);
  appendReceiptField(receipt, "creative_baked_room_stale_reason_code",
                     window.creativeBakedRoomStaleReasonCode);
  appendReceiptField(receipt, "creative_viewport_pick_requested",
                     window.creativeViewportPickRequested);
  appendReceiptField(receipt, "creative_viewport_pick_active",
                     window.creativeViewportPickActive);
  appendReceiptField(receipt, "creative_viewport_pick_click_present",
                     window.creativeViewportPickClickPresent);
  appendReceiptField(receipt, "creative_viewport_pick_click_suppressed",
                     window.creativeViewportPickClickSuppressed);
  appendReceiptField(receipt, "creative_viewport_pick_facade_available",
                     window.creativeViewportPickFacadeAvailable);
  appendReceiptField(receipt, "creative_viewport_pick_source_available",
                     window.creativeViewportPickSourceAvailable);
  appendReceiptField(receipt, "creative_viewport_pick_projected",
                     window.creativeViewportPickProjected);
  appendReceiptField(receipt, "creative_viewport_pick_picked",
                     window.creativeViewportPickPicked);
  appendReceiptField(receipt, "creative_viewport_pick_object_count",
                     window.creativeViewportPickObjectCount);
  appendReceiptField(receipt, "creative_viewport_pick_projection_cell_count",
                     window.creativeViewportPickProjectionCellCount);
  appendReceiptField(receipt, "creative_viewport_pick_status",
                     window.creativeViewportPickStatus);
  appendReceiptField(receipt, "creative_viewport_pick_reason_code",
                     window.creativeViewportPickReasonCode);
  appendReceiptField(receipt, "creative_viewport_pick_pick_status",
                     window.creativeViewportPickPickStatus);
  appendReceiptField(receipt, "creative_viewport_pick_message",
                     window.creativeViewportPickMessage);
  appendReceiptField(receipt, "creative_viewport_pick_coord_x",
                     std::to_string(window.creativeViewportPickCoordX));
  appendReceiptField(receipt, "creative_viewport_pick_coord_y",
                     std::to_string(window.creativeViewportPickCoordY));
  appendReceiptField(receipt, "creative_viewport_pick_coord_z",
                     std::to_string(window.creativeViewportPickCoordZ));
  appendReceiptField(receipt, "creative_viewport_pick_grid_index",
                     window.creativeViewportPickGridIndex);
  appendReceiptField(receipt, "creative_viewport_pick_object_id",
                     window.creativeViewportPickObjectId);
  appendReceiptField(receipt, "creative_viewport_pick_object_kind",
                     window.creativeViewportPickObjectKind);
  appendReceiptField(receipt, "creative_viewport_pick_occupancy_kind",
                     window.creativeViewportPickOccupancyKind);
  appendReceiptField(receipt, "creative_viewport_pick_target",
                     window.creativeViewportPickTarget);
  appendReceiptField(receipt, "creative_viewport_pick_cell_index",
                     window.creativeViewportPickCellIndex);
  appendReceiptField(receipt, "creative_wireframe_requested",
                     window.creativeWireframeRequested);
  appendReceiptField(receipt, "creative_wireframe_active",
                     window.creativeWireframeActive);
  appendReceiptField(receipt, "creative_wireframe_facade_available",
                     window.creativeWireframeFacadeAvailable);
  appendReceiptField(receipt, "creative_wireframe_document_available",
                     window.creativeWireframeDocumentAvailable);
  appendReceiptField(receipt, "creative_wireframe_source_available",
                     window.creativeWireframeSourceAvailable);
  appendReceiptField(receipt, "creative_wireframe_object_count",
                     window.creativeWireframeObjectCount);
  appendReceiptField(receipt, "creative_wireframe_visible_object_count",
                     window.creativeWireframeVisibleObjectCount);
  appendReceiptField(receipt, "creative_wireframe_item_count",
                     window.creativeWireframeItemCount);
  appendReceiptField(receipt, "creative_wireframe_segment_count",
                     window.creativeWireframeSegmentCount);
  appendReceiptField(receipt, "creative_wireframe_box_item_count",
                     window.creativeWireframeBoxItemCount);
  appendReceiptField(receipt, "creative_wireframe_line_item_count",
                     window.creativeWireframeLineItemCount);
  appendReceiptField(receipt, "creative_wireframe_point_item_count",
                     window.creativeWireframePointItemCount);
  appendReceiptField(receipt,
                     "creative_wireframe_skipped_degenerate_count",
                     window.creativeWireframeSkippedDegenerateCount);
  appendReceiptField(receipt, "creative_wireframe_status",
                     window.creativeWireframeStatus);
  appendReceiptField(receipt, "creative_wireframe_reason_code",
                     window.creativeWireframeReasonCode);
  appendReceiptField(receipt, "creative_wireframe_wireframe_status",
                     window.creativeWireframeWireframeStatus);
  appendReceiptField(receipt, "creative_wireframe_wireframe_reason_code",
                     window.creativeWireframeWireframeReasonCode);
  appendReceiptField(receipt, "creative_wireframe_segment_status",
                     window.creativeWireframeSegmentStatus);
  appendReceiptField(receipt, "creative_wireframe_segment_reason_code",
                     window.creativeWireframeSegmentReasonCode);
  appendReceiptField(receipt, "creative_wireframe_debug_line_requested",
                     window.creativeWireframeDebugLineRequested);
  appendReceiptField(receipt, "creative_wireframe_debug_line_source_available",
                     window.creativeWireframeDebugLineSourceAvailable);
  appendReceiptField(receipt,
                     "creative_wireframe_debug_line_input_segment_count",
                     window.creativeWireframeDebugLineInputSegmentCount);
  appendReceiptField(receipt, "creative_wireframe_debug_line_count",
                     window.creativeWireframeDebugLineCount);
  appendReceiptField(receipt,
                     "creative_wireframe_debug_line_skipped_degenerate_count",
                     window.creativeWireframeDebugLineSkippedDegenerateCount);
  appendReceiptField(receipt, "creative_wireframe_debug_line_status",
                     window.creativeWireframeDebugLineStatus);
  appendReceiptField(receipt, "creative_wireframe_debug_line_reason_code",
                     window.creativeWireframeDebugLineReasonCode);
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
