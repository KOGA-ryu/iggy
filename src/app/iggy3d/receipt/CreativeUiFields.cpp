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

template <typename Fields>
struct CreativeUiCommandReceiptFieldRow {
  std::string_view key;
  void (*append)(RenderReceipt& receipt,
                 const Fields& fields,
                 std::string_view key);
};

const std::array<
    CreativeUiCommandReceiptFieldRow<ProductCreativeUiCommandMutationDiagnostics>,
    16>
    kCreativeUiCommandMutationReceiptFields{{
    {"creative_ui_command_mutation_requested",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandMutationDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.requested);
     }},
    {"creative_ui_command_mutation_accepted",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandMutationDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.accepted);
     }},
    {"creative_ui_command_mutation_changed",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandMutationDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.changed);
     }},
    {"creative_ui_command_mutation_status",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandMutationDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.status);
     }},
    {"creative_ui_command_document_mutation_status",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandMutationDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.documentStatus);
     }},
    {"creative_ui_command_mutation_kind",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandMutationDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.kind);
     }},
    {"creative_ui_command_mutation_target",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandMutationDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.target);
     }},
    {"creative_ui_command_mutation_object_id",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandMutationDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.objectId);
     }},
    {"creative_ui_command_mutation_object_kind",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandMutationDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.objectKind);
     }},
    {"creative_ui_command_visible_before",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandMutationDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.visibleBefore);
     }},
    {"creative_ui_command_visible_after",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandMutationDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.visibleAfter);
     }},
    {"creative_ui_command_locked_before",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandMutationDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.lockedBefore);
     }},
    {"creative_ui_command_locked_after",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandMutationDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.lockedAfter);
     }},
    {"creative_ui_command_revision_before",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandMutationDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.revisionBefore);
     }},
    {"creative_ui_command_revision_after",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandMutationDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.revisionAfter);
     }},
    {"creative_ui_command_mutation_message",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandMutationDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.message);
     }},
}};

const std::array<
    CreativeUiCommandReceiptFieldRow<ProductCreativeUiCommandCreateDiagnostics>,
    12>
    kCreativeUiCommandCreateReceiptFields{{
    {"creative_ui_command_create_requested",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandCreateDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.requested);
     }},
    {"creative_ui_command_create_accepted",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandCreateDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.accepted);
     }},
    {"creative_ui_command_create_changed",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandCreateDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.changed);
     }},
    {"creative_ui_command_create_status",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandCreateDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.status);
     }},
    {"creative_ui_command_create_object_id",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandCreateDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.objectId);
     }},
    {"creative_ui_command_create_object_kind",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandCreateDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.objectKind);
     }},
    {"creative_ui_command_create_object_name",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandCreateDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.objectName);
     }},
    {"creative_ui_command_create_revision_before",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandCreateDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.revisionBefore);
     }},
    {"creative_ui_command_create_revision_after",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandCreateDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.revisionAfter);
     }},
    {"creative_ui_command_create_dirty_flags",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandCreateDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.dirtyFlags);
     }},
    {"creative_ui_command_create_message",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandCreateDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.message);
     }},
    {"creative_ui_command_create_reason_code",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandCreateDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.reasonCode);
     }},
}};

const std::array<
    CreativeUiCommandReceiptFieldRow<ProductCreativeUiCommandDeleteDiagnostics>,
    13>
    kCreativeUiCommandDeleteReceiptFields{{
    {"creative_ui_command_delete_requested",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandDeleteDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.requested);
     }},
    {"creative_ui_command_delete_accepted",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandDeleteDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.accepted);
     }},
    {"creative_ui_command_delete_changed",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandDeleteDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.changed);
     }},
    {"creative_ui_command_delete_removed",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandDeleteDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.removed);
     }},
    {"creative_ui_command_delete_object_id",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandDeleteDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.objectId);
     }},
    {"creative_ui_command_delete_object_kind",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandDeleteDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.objectKind);
     }},
    {"creative_ui_command_delete_object_name",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandDeleteDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.objectName);
     }},
    {"creative_ui_command_delete_revision_before",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandDeleteDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.revisionBefore);
     }},
    {"creative_ui_command_delete_revision_after",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandDeleteDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.revisionAfter);
     }},
    {"creative_ui_command_delete_dirty_flags",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandDeleteDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.dirtyFlags);
     }},
    {"creative_ui_command_delete_status",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandDeleteDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.status);
     }},
    {"creative_ui_command_delete_message",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandDeleteDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.message);
     }},
    {"creative_ui_command_delete_reason_code",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandDeleteDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.reasonCode);
     }},
}};

const std::array<
    CreativeUiCommandReceiptFieldRow<ProductCreativeUiCommandUndoDiagnostics>,
    14>
    kCreativeUiCommandUndoReceiptFields{{
    {"creative_ui_command_undo_requested",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandUndoDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.requested);
     }},
    {"creative_ui_command_undo_accepted",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandUndoDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.accepted);
     }},
    {"creative_ui_command_undo_changed",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandUndoDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.changed);
     }},
    {"creative_ui_command_undo_had_snapshot",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandUndoDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.hadSnapshot);
     }},
    {"creative_ui_command_undo_document_id",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandUndoDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.documentId);
     }},
    {"creative_ui_command_undo_revision_before",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandUndoDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.revisionBefore);
     }},
    {"creative_ui_command_undo_revision_after",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandUndoDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.revisionAfter);
     }},
    {"creative_ui_command_undo_object_count_before",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandUndoDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.objectCountBefore);
     }},
    {"creative_ui_command_undo_object_count_after",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandUndoDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.objectCountAfter);
     }},
    {"creative_ui_command_undo_depth_before",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandUndoDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.depthBefore);
     }},
    {"creative_ui_command_undo_depth_after",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandUndoDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.depthAfter);
     }},
    {"creative_ui_command_undo_status",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandUndoDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.status);
     }},
    {"creative_ui_command_undo_message",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandUndoDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.message);
     }},
    {"creative_ui_command_undo_reason_code",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandUndoDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.reasonCode);
     }},
}};

const std::array<
    CreativeUiCommandReceiptFieldRow<ProductCreativeUiCommandRoomShellDiagnostics>,
    13>
    kCreativeUiCommandRoomShellReceiptFields{{
    {"creative_ui_command_shell_requested",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandRoomShellDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.requested);
     }},
    {"creative_ui_command_shell_accepted",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandRoomShellDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.accepted);
     }},
    {"creative_ui_command_shell_changed",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandRoomShellDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.changed);
     }},
    {"creative_ui_command_shell_room_object_id",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandRoomShellDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.roomObjectId);
     }},
    {"creative_ui_command_shell_generated_object_count",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandRoomShellDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.generatedObjectCount);
     }},
    {"creative_ui_command_shell_removed_object_count",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandRoomShellDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.removedObjectCount);
     }},
    {"creative_ui_command_shell_floor_count",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandRoomShellDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.floorCount);
     }},
    {"creative_ui_command_shell_wall_count",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandRoomShellDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.wallCount);
     }},
    {"creative_ui_command_shell_revision_before",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandRoomShellDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.revisionBefore);
     }},
    {"creative_ui_command_shell_revision_after",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandRoomShellDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.revisionAfter);
     }},
    {"creative_ui_command_shell_status",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandRoomShellDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.status);
     }},
    {"creative_ui_command_shell_reason_code",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandRoomShellDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.reasonCode);
     }},
    {"creative_ui_command_shell_message",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandRoomShellDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.message);
     }},
}};

const std::array<
    CreativeUiCommandReceiptFieldRow<ProductCreativeUiCommandDiagnostics>,
    14>
    kCreativeUiCommandReceiptFields{{
    {"creative_ui_command_requested",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.requested);
     }},
    {"creative_ui_command_facade_available",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.facadeAvailable);
     }},
    {"creative_ui_command_input_consumed",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.inputConsumed);
     }},
    {"creative_ui_command_input_enabled",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.inputEnabled);
     }},
    {"creative_ui_command_accepted",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.accepted);
     }},
    {"creative_ui_command_changed",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.changed);
     }},
    {"creative_ui_command_kind",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.kind);
     }},
    {"creative_ui_command_tool",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.tool);
     }},
    {"creative_ui_command_object_kind",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.objectKind);
     }},
    {"creative_ui_command_tool_before",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.toolBefore);
     }},
    {"creative_ui_command_tool_after",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.toolAfter);
     }},
    {"creative_ui_command_semantic_id",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.semanticId);
     }},
    {"creative_ui_command_status",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.status);
     }},
    {"creative_ui_command_reason_code",
     [](RenderReceipt& receipt,
        const ProductCreativeUiCommandDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           fields.reasonCode);
     }},
}};

void appendProductCreativeUiCommandMutationFields(
    RenderReceipt& receipt,
    const ProductCreativeUiCommandMutationDiagnostics& fields) {
  for (const CreativeUiCommandReceiptFieldRow<ProductCreativeUiCommandMutationDiagnostics>& row :
       kCreativeUiCommandMutationReceiptFields) {
    row.append(receipt, fields, row.key);
  }
}


void appendProductCreativeUiCommandCreateFields(
    RenderReceipt& receipt,
    const ProductCreativeUiCommandCreateDiagnostics& fields) {
  for (const CreativeUiCommandReceiptFieldRow<ProductCreativeUiCommandCreateDiagnostics>& row :
       kCreativeUiCommandCreateReceiptFields) {
    row.append(receipt, fields, row.key);
  }
}


void appendProductCreativeUiCommandDeleteFields(
    RenderReceipt& receipt,
    const ProductCreativeUiCommandDeleteDiagnostics& fields) {
  for (const CreativeUiCommandReceiptFieldRow<ProductCreativeUiCommandDeleteDiagnostics>& row :
       kCreativeUiCommandDeleteReceiptFields) {
    row.append(receipt, fields, row.key);
  }
}


void appendProductCreativeUiCommandUndoFields(
    RenderReceipt& receipt,
    const ProductCreativeUiCommandUndoDiagnostics& fields) {
  for (const CreativeUiCommandReceiptFieldRow<ProductCreativeUiCommandUndoDiagnostics>& row :
       kCreativeUiCommandUndoReceiptFields) {
    row.append(receipt, fields, row.key);
  }
}


void appendProductCreativeUiCommandRoomShellFields(
    RenderReceipt& receipt,
    const ProductCreativeUiCommandRoomShellDiagnostics& fields) {
  for (const CreativeUiCommandReceiptFieldRow<ProductCreativeUiCommandRoomShellDiagnostics>& row :
       kCreativeUiCommandRoomShellReceiptFields) {
    row.append(receipt, fields, row.key);
  }
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

struct ProductCreativeBakedRoomRefreshReceiptFieldRow {
  std::string_view ProductCreativeBakedRoomRefreshReceiptKeySet::* key;
  void (*append)(RenderReceipt& receipt,
                 const ProductCreativeBakedRoomRefreshDiagnostics& fields,
                 std::string_view key);
};

const std::array<ProductCreativeBakedRoomRefreshReceiptFieldRow, 2>
    kProductCreativeBakedRoomRefreshPreOptionalReceiptFields{{
    {&ProductCreativeBakedRoomRefreshReceiptKeySet::requested,
     [](RenderReceipt& receipt,
        const ProductCreativeBakedRoomRefreshDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(receipt, key, fields.requested);
     }},
    {&ProductCreativeBakedRoomRefreshReceiptKeySet::accepted,
     [](RenderReceipt& receipt,
        const ProductCreativeBakedRoomRefreshDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(receipt, key, fields.accepted);
     }},
}};

const std::array<ProductCreativeBakedRoomRefreshReceiptFieldRow, 10>
    kProductCreativeBakedRoomRefreshPostOptionalReceiptFields{{
    {&ProductCreativeBakedRoomRefreshReceiptKeySet::status,
     [](RenderReceipt& receipt,
        const ProductCreativeBakedRoomRefreshDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(receipt, key, fields.status);
     }},
    {&ProductCreativeBakedRoomRefreshReceiptKeySet::reasonCode,
     [](RenderReceipt& receipt,
        const ProductCreativeBakedRoomRefreshDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(receipt, key, fields.reasonCode);
     }},
    {&ProductCreativeBakedRoomRefreshReceiptKeySet::bakeMeasured,
     [](RenderReceipt& receipt,
        const ProductCreativeBakedRoomRefreshDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(receipt, key, fields.bakeMeasured);
     }},
    {&ProductCreativeBakedRoomRefreshReceiptKeySet::bakeElapsedMicroseconds,
     [](RenderReceipt& receipt,
        const ProductCreativeBakedRoomRefreshDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(receipt, key, fields.bakeElapsedMicroseconds);
     }},
    {&ProductCreativeBakedRoomRefreshReceiptKeySet::bakedDocumentRevision,
     [](RenderReceipt& receipt,
        const ProductCreativeBakedRoomRefreshDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(receipt, key, fields.bakedDocumentRevision);
     }},
    {&ProductCreativeBakedRoomRefreshReceiptKeySet::staticMeshCount,
     [](RenderReceipt& receipt,
        const ProductCreativeBakedRoomRefreshDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(receipt, key, fields.staticMeshCount);
     }},
    {&ProductCreativeBakedRoomRefreshReceiptKeySet::anchorCount,
     [](RenderReceipt& receipt,
        const ProductCreativeBakedRoomRefreshDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(receipt, key, fields.anchorCount);
     }},
    {&ProductCreativeBakedRoomRefreshReceiptKeySet::spatialSurfaceCount,
     [](RenderReceipt& receipt,
        const ProductCreativeBakedRoomRefreshDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(receipt, key, fields.spatialSurfaceCount);
     }},
    {&ProductCreativeBakedRoomRefreshReceiptKeySet::collisionReady,
     [](RenderReceipt& receipt,
        const ProductCreativeBakedRoomRefreshDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(receipt, key, fields.collisionReady);
     }},
    {&ProductCreativeBakedRoomRefreshReceiptKeySet::collisionQuerySurfaceCount,
     [](RenderReceipt& receipt,
        const ProductCreativeBakedRoomRefreshDiagnostics& fields,
        std::string_view key) {
       appendReceiptField(receipt, key, fields.collisionQuerySurfaceCount);
     }},
}};

void appendProductCreativeBakedRoomRefreshDiagnosticFields(
    RenderReceipt& receipt,
    const ProductCreativeBakedRoomRefreshDiagnostics& fields,
    const ProductCreativeBakedRoomRefreshReceiptKeySet& keys) {
  for (const ProductCreativeBakedRoomRefreshReceiptFieldRow& row :
       kProductCreativeBakedRoomRefreshPreOptionalReceiptFields) {
    row.append(receipt, fields, keys.*(row.key));
  }
  if (!keys.clearedActiveRoom.empty()) {
    appendReceiptField(receipt,
                       keys.clearedActiveRoom,
                       fields.clearedActiveRoom);
  }
  for (const ProductCreativeBakedRoomRefreshReceiptFieldRow& row :
       kProductCreativeBakedRoomRefreshPostOptionalReceiptFields) {
    row.append(receipt, fields, keys.*(row.key));
  }
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
  for (const CreativeUiCommandReceiptFieldRow<ProductCreativeUiCommandDiagnostics>& row :
       kCreativeUiCommandReceiptFields) {
    row.append(receipt, fields, row.key);
  }
  appendProductCreativeUiCommandMutationFields(receipt, fields.mutation);
  appendProductCreativeUiCommandCreateFields(receipt, fields.create);
  appendProductCreativeUiCommandDeleteFields(receipt, fields.deleteObject);
  appendProductCreativeUiCommandUndoFields(receipt, fields.undo);
  appendProductCreativeUiCommandRoomShellFields(receipt, fields.shell);
  appendProductCreativeUiCommandBakedRoomRefreshFields(
      receipt, fields.bakedRoomRefresh);
}


struct CreativeUiReceiptContext {
  const ProductAppWindowState& window;
  const decltype(std::declval<const ProductAppWindowState&>().creativeAuthoring)&
      authoring;
};

struct CreativeUiReceiptFieldRow {
  std::string_view key;
  void (*append)(RenderReceipt& receipt,
                 const CreativeUiReceiptContext& context,
                 std::string_view key);
};

const std::array<CreativeUiReceiptFieldRow, 52>
    kCreativeUiPreCommandReceiptFields{{
    {"creative_ui_projection_requested",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiProjection.requested);
     }},
    {"creative_ui_projection_ready",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiProjection.ready);
     }},
    {"creative_ui_projection_partial",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiProjection.partial);
     }},
    {"creative_ui_projection_status",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiProjection.status);
     }},
    {"creative_ui_projection_reason_code",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiProjection.reasonCode);
     }},
    {"creative_ui_projection_used_model",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiProjection.usedModel);
     }},
    {"creative_ui_projection_used_facade",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiProjection.usedFacade);
     }},
    {"creative_ui_projection_virtual_width",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           static_cast<std::uint64_t>(
                                    context.authoring.creativeUiProjection.virtualWidth));
     }},
    {"creative_ui_projection_virtual_height",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           static_cast<std::uint64_t>(
                                    context.authoring.creativeUiProjection.virtualHeight));
     }},
    {"creative_ui_projection_theme",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiProjection.theme);
     }},
    {"creative_ui_projection_panel_count",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiProjection.panelCount);
     }},
    {"creative_ui_projection_model_row_count",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiProjection.modelRowCount);
     }},
    {"creative_ui_projection_primitive_count",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiProjection.primitiveCount);
     }},
    {"creative_ui_projection_text_count",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiProjection.textCount);
     }},
    {"creative_ui_projection_rect_count",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiProjection.rectCount);
     }},
    {"creative_ui_projection_row_count",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiProjection.rowCount);
     }},
    {"creative_ui_projection_disabled_row_count",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiProjection.disabledRowCount);
     }},
    {"creative_ui_projection_hit_region_count",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiProjection.hitRegionCount);
     }},
    {"creative_ui_input_requested",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiInput.requested);
     }},
    {"creative_ui_input_click_present",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiInput.clickPresent);
     }},
    {"creative_ui_input_draw_list_available",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiInput.drawListAvailable);
     }},
    {"creative_ui_input_routed",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiInput.routed);
     }},
    {"creative_ui_input_hit",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiInput.hit);
     }},
    {"creative_ui_input_consumed",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiInput.consumed);
     }},
    {"creative_ui_input_enabled",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiInput.enabled);
     }},
    {"creative_ui_input_surface",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiInput.surface);
     }},
    {"creative_ui_input_kind",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiInput.kind);
     }},
    {"creative_ui_input_action",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiInput.action);
     }},
    {"creative_ui_input_layer_index",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiInput.layerIndex);
     }},
    {"creative_ui_input_region_index",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiInput.regionIndex);
     }},
    {"creative_ui_input_semantic_id",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiInput.semanticId);
     }},
    {"creative_ui_input_status",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiInput.status);
     }},
    {"creative_ui_input_reason_code",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiInput.reasonCode);
     }},
    {"creative_ui_last_click_seen",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiLast.clickSeen);
     }},
    {"creative_ui_last_click_x",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiLast.clickX);
     }},
    {"creative_ui_last_click_y",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiLast.clickY);
     }},
    {"creative_ui_last_input_hit",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiLast.inputHit);
     }},
    {"creative_ui_last_input_consumed",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiLast.inputConsumed);
     }},
    {"creative_ui_last_input_status",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiLast.inputStatus);
     }},
    {"creative_ui_last_input_semantic_id",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiLast.inputSemanticId);
     }},
    {"creative_ui_last_command_kind",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiLast.commandKind);
     }},
    {"creative_ui_last_command_status",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiLast.commandStatus);
     }},
    {"creative_ui_last_command_create_requested",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiLast.commandCreateRequested);
     }},
    {"creative_ui_last_command_create_accepted",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiLast.commandCreateAccepted);
     }},
    {"creative_ui_last_command_create_changed",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiLast.commandCreateChanged);
     }},
    {"creative_ui_last_command_create_object_id",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiLast.commandCreateObjectId);
     }},
    {"creative_ui_input_downstream_click_requested",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiInput.downstreamClickRequested);
     }},
    {"creative_ui_input_downstream_click_present",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiInput.downstreamClickPresent);
     }},
    {"creative_ui_input_downstream_click_higher_priority",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiInput.downstreamClickHigherPriority);
     }},
    {"creative_ui_input_downstream_click_suppressed",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiInput.downstreamClickSuppressed);
     }},
    {"creative_ui_input_downstream_click_status",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiInput.downstreamClickStatus);
     }},
    {"creative_ui_input_downstream_click_reason_code",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUiInput.downstreamClickReasonCode);
     }},
}};

const std::array<CreativeUiReceiptFieldRow, 12>
    kCreativeUiPostRefreshReceiptFields{{
    {"creative_document_revision_observed",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeDocumentRevision.observed);
     }},
    {"creative_document_changed_this_frame",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeDocumentChangedThisFrame);
     }},
    {"creative_document_revision_document_id",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeDocumentRevision.documentId);
     }},
    {"creative_document_revision_before_frame",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeDocumentRevision.beforeFrame);
     }},
    {"creative_document_revision_after_frame",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeDocumentRevision.afterFrame);
     }},
    {"creative_undo_available",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUndo.available);
     }},
    {"creative_undo_depth",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeUndo.depth);
     }},
    {"creative_baked_room_stale",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeBakedRoomStale);
     }},
    {"creative_baked_room_stale_document_id",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeBakedRoomStaleDocumentId);
     }},
    {"creative_baked_room_stale_revision",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeBakedRoomStaleRevision);
     }},
    {"creative_baked_room_stale_status",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeBakedRoomStaleStatus);
     }},
    {"creative_baked_room_stale_reason_code",
     [](RenderReceipt& receipt,
        const CreativeUiReceiptContext& context,
        std::string_view key) {
       appendReceiptField(
           receipt,
           key,
           context.authoring.creativeBakedRoomStaleReasonCode);
     }},
}};

}  // namespace

void appendProductCreativeUiFields(RenderReceipt& receipt,
                                   const ProductAppWindowState& window) {
  const auto& authoring = window.creativeAuthoring;
  const CreativeUiReceiptContext context{
      window,
      authoring,
  };

  for (const CreativeUiReceiptFieldRow& row :
       kCreativeUiPreCommandReceiptFields) {
    row.append(receipt, context, row.key);
  }

  appendProductCreativeUiCommandFields(receipt, authoring.creativeUiCommand);
  appendProductCreativeBakedRoomAutoRefreshFields(
      receipt, authoring.creativeBakedRoomAutoRefresh);

  for (const CreativeUiReceiptFieldRow& row :
       kCreativeUiPostRefreshReceiptFields) {
    row.append(receipt, context, row.key);
  }
}


}  // namespace iggy3d
