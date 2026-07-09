#include "app/iggy3d/creative/bridge/UiCommandFrame.hpp"

#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/tools/Placement.hpp"
#include "app/iggy3d/creative/tools/RoomShell.hpp"

#include <array>
#include <string>
#include <string_view>
#include <utility>

namespace iggy3d {
namespace {

struct ProductCreativeUiCommandExecutionContext {
  ProductCreativeUiCommandFrameReceipt& receipt;
  creative::CreativeAppState& creative;
  creative::Facade& facade;
  const ProductCreativeUiCommandCatalogEntry& row;
};

using ProductCreativeUiCommandHandler =
    void (*)(ProductCreativeUiCommandExecutionContext& context);

struct ProductCreativeUiCommandHandlerEntry {
  ProductCreativeUiCommandKind commandKind =
      ProductCreativeUiCommandKind::None;
  ProductCreativeUiCommandHandler handler = nullptr;
};

void setNoopStatus(ProductCreativeUiCommandFrameReceipt& receipt,
                  std::string_view status) {
  receipt.status = std::string(status);
  receipt.reasonCode = std::string(status);
}

void setDeleteNoSelection(ProductCreativeUiCommandFrameReceipt& receipt,
                          std::uint64_t revision) {
  receipt.remove.noSelection = true;
  receipt.remove.document.requested = true;
  receipt.remove.document.revisionBefore = revision;
  receipt.remove.document.revisionAfter = revision;
  receipt.remove.document.message = "no_selection";
  receipt.remove.document.reasonCode = "no_selection";
}

void copyRoomShellReceipt(ProductCreativeUiCommandFrameReceipt& receipt,
                          const creative::CreativeRoomShellBuildReceipt&
                              shellReceipt) {
  receipt.shellRequested = shellReceipt.requested;
  receipt.shellAccepted = shellReceipt.accepted;
  receipt.shellRoomObjectId = shellReceipt.roomObjectId;
  receipt.shellGeneratedObjectCount = shellReceipt.generatedRequestCount;
  receipt.shellFloorCount = shellReceipt.floorRequestCount;
  receipt.shellWallCount = shellReceipt.wallRequestCount;
  receipt.shellStatus = std::string(creative::toString(shellReceipt.status));
  receipt.shellReasonCode = shellReceipt.reasonCode;
  receipt.shellMessage = shellReceipt.message;
}

void copyRoomShellRemoveReceipt(ProductCreativeUiCommandFrameReceipt& receipt,
                                const creative::CreativeRoomShellRemoveReceipt&
                                    shellReceipt) {
  receipt.shellRequested = shellReceipt.requested;
  receipt.shellAccepted = shellReceipt.accepted;
  receipt.shellRoomObjectId = shellReceipt.roomObjectId;
  receipt.shellRemovedObjectCount = shellReceipt.removedObjectCount;
  receipt.shellFloorCount = shellReceipt.floorObjectCount;
  receipt.shellWallCount = shellReceipt.wallObjectCount;
  receipt.shellStatus = std::string(creative::toString(shellReceipt.status));
  receipt.shellReasonCode = shellReceipt.reasonCode;
  receipt.shellMessage = shellReceipt.message;
}

void setRoomShellApplyStatus(ProductCreativeUiCommandFrameReceipt& receipt,
                             creative::CreativeRoomShellStatus status,
                             std::string_view reasonCode) {
  receipt.shellAccepted = false;
  receipt.shellChanged = false;
  receipt.shellStatus = std::string(creative::toString(status));
  receipt.shellReasonCode = std::string(reasonCode);
  receipt.shellMessage = std::string(reasonCode);
}

void setCommandOutcomeStatus(ProductCreativeUiCommandFrameReceipt& receipt,
                             bool accepted,
                             bool changed) {
  if (accepted && changed) {
    setNoopStatus(receipt, "product_creative_ui_command_applied");
  } else if (accepted) {
    setNoopStatus(receipt, "product_creative_ui_command_no_change");
  } else {
    setNoopStatus(receipt, "product_creative_ui_command_rejected");
  }
}

void handleSetActiveTool(ProductCreativeUiCommandExecutionContext& context) {
  ProductCreativeUiCommandFrameReceipt& receipt = context.receipt;
  receipt.commandTool = context.row.tool;
  const bool toolChanged = context.facade.setActiveTool(context.row.tool);
  receipt.toolAfter = context.facade.toolState().activeTool;
  receipt.accepted = true;
  receipt.changed = toolChanged;
  setCommandOutcomeStatus(receipt, true, toolChanged);
}

void handleToggleSelectedObjectVisibility(
    ProductCreativeUiCommandExecutionContext& context) {
  ProductCreativeUiCommandFrameReceipt& receipt = context.receipt;
  const creative::CreativeFacadeMutationReceipt mutationReceipt =
      context.facade.toggleSelectedObjectVisibility();
  receipt.mutation = mutationReceipt;
  receipt.accepted = mutationReceipt.accepted;
  receipt.changed = mutationReceipt.changed;
  setCommandOutcomeStatus(receipt,
                          receipt.mutation.accepted,
                          receipt.mutation.changed);
}

void handleToggleSelectedObjectLocked(
    ProductCreativeUiCommandExecutionContext& context) {
  ProductCreativeUiCommandFrameReceipt& receipt = context.receipt;
  const creative::CreativeFacadeMutationReceipt mutationReceipt =
      context.facade.toggleSelectedObjectLocked();
  receipt.mutation = mutationReceipt;
  receipt.accepted = mutationReceipt.accepted;
  receipt.changed = mutationReceipt.changed;
  setCommandOutcomeStatus(receipt,
                          receipt.mutation.accepted,
                          receipt.mutation.changed);
}

void handleRebuildRoom(ProductCreativeUiCommandExecutionContext& context) {
  ProductCreativeUiCommandFrameReceipt& receipt = context.receipt;
  receipt.accepted = true;
  receipt.changed = false;
  receipt.toolAfter = context.facade.toolState().activeTool;
  setNoopStatus(receipt, "product_creative_ui_command_rebuild_room_requested");
}

void handleUndoLastDocumentChange(
    ProductCreativeUiCommandExecutionContext& context) {
  ProductCreativeUiCommandFrameReceipt& receipt = context.receipt;
  const creative::CreativeDocumentUndoApplyReceipt undoReceipt =
      creative::applyLastCreativeUndoSnapshot(context.creative);
  receipt.undo = undoReceipt;
  receipt.accepted = undoReceipt.accepted;
  receipt.changed = undoReceipt.changed;
  receipt.toolAfter = context.facade.toolState().activeTool;
  setCommandOutcomeStatus(receipt, undoReceipt.accepted, undoReceipt.changed);
}

void handleDeleteSelectedObject(
    ProductCreativeUiCommandExecutionContext& context) {
  ProductCreativeUiCommandFrameReceipt& receipt = context.receipt;
  const creative::TargetRef selectedTarget =
      context.facade.selectionState().selectedTarget;
  if (selectedTarget.value == creative::kInvalidId) {
    setDeleteNoSelection(receipt, context.facade.document().revision());
    receipt.toolAfter = context.facade.toolState().activeTool;
    setNoopStatus(receipt, "product_creative_ui_command_rejected");
    return;
  }

  const creative::CreativeDocumentRemoveReceipt deleteReceipt =
      context.facade.removeDocumentObject(
          static_cast<creative::CreativeObjectId>(selectedTarget.value));
  receipt.remove.document = deleteReceipt;
  receipt.accepted = deleteReceipt.accepted;
  receipt.changed = deleteReceipt.changed;
  receipt.toolAfter = context.facade.toolState().activeTool;
  if (deleteReceipt.accepted && deleteReceipt.objectRemoved &&
      deleteReceipt.changed) {
    setNoopStatus(receipt, "product_creative_ui_command_applied");
  } else if (deleteReceipt.accepted) {
    setNoopStatus(receipt, "product_creative_ui_command_no_change");
  } else {
    setNoopStatus(receipt, "product_creative_ui_command_rejected");
  }
}

void handleGenerateSelectedRoomShell(
    ProductCreativeUiCommandExecutionContext& context) {
  ProductCreativeUiCommandFrameReceipt& receipt = context.receipt;
  const creative::TargetRef selectedTarget =
      context.facade.selectionState().selectedTarget;
  creative::CreativeRoomShellBuildRequest shellRequest;
  shellRequest.document = &context.facade.document();
  if (selectedTarget.value != creative::kInvalidId) {
    shellRequest.roomObjectId =
        static_cast<creative::CreativeObjectId>(selectedTarget.value);
  }

  const std::uint64_t revisionBefore = context.facade.document().revision();
  const creative::CreativeRoomShellBuildResult shell =
      creative::buildCreativeRoomShellCreateRequests(shellRequest);
  copyRoomShellReceipt(receipt, shell.receipt);
  receipt.shellRevisionBefore = revisionBefore;
  receipt.shellRevisionAfter = revisionBefore;

  if (!shell.receipt.accepted) {
    receipt.toolAfter = context.facade.toolState().activeTool;
    setNoopStatus(receipt, "product_creative_ui_command_rejected");
    return;
  }

  const creative::CreativeFacadeDocumentBatchCreateReceipt batch =
      context.facade.createDocumentObjectsAtomically(shell.createRequests);
  receipt.shellRevisionAfter = batch.revisionAfter;
  if (!batch.accepted || !batch.changed) {
    const bool installRejected =
        batch.status ==
        creative::CreativeFacadeDocumentBatchCreateStatus::InstallRejected;
    setRoomShellApplyStatus(
        receipt,
        installRejected
            ? creative::CreativeRoomShellStatus::InstallRejected
            : creative::CreativeRoomShellStatus::CreateRejected,
        installRejected ? "creative_room_shell_install_rejected"
                        : "creative_room_shell_create_rejected");
    receipt.toolAfter = context.facade.toolState().activeTool;
    setNoopStatus(receipt, "product_creative_ui_command_rejected");
    return;
  }

  receipt.shellAccepted = true;
  receipt.shellChanged = true;
  receipt.shellRevisionAfter = context.facade.document().revision();
  receipt.accepted = true;
  receipt.changed = true;
  receipt.toolAfter = context.facade.toolState().activeTool;
  setNoopStatus(receipt, "product_creative_ui_command_applied");
}

void handleRemoveSelectedRoomShell(
    ProductCreativeUiCommandExecutionContext& context) {
  ProductCreativeUiCommandFrameReceipt& receipt = context.receipt;
  const creative::TargetRef selectedTarget =
      context.facade.selectionState().selectedTarget;
  creative::CreativeRoomShellRemoveRequest shellRequest;
  shellRequest.document = &context.facade.document();
  if (selectedTarget.value != creative::kInvalidId) {
    shellRequest.roomObjectId =
        static_cast<creative::CreativeObjectId>(selectedTarget.value);
  }

  const std::uint64_t revisionBefore = context.facade.document().revision();
  const creative::CreativeRoomShellRemoveResult shell =
      creative::findCreativeRoomShellChildren(shellRequest);
  copyRoomShellRemoveReceipt(receipt, shell.receipt);
  receipt.shellRevisionBefore = revisionBefore;
  receipt.shellRevisionAfter = revisionBefore;

  if (!shell.receipt.accepted) {
    receipt.toolAfter = context.facade.toolState().activeTool;
    setNoopStatus(receipt, "product_creative_ui_command_rejected");
    return;
  }

  creative::CreativeDocument stagedDocument = context.facade.document();
  bool removeSucceeded = true;
  for (const creative::CreativeObjectId objectId : shell.objectIds) {
    const creative::CreativeDocumentRemoveReceipt removeReceipt =
        stagedDocument.removeDocumentObject(objectId);
    if (!removeReceipt.accepted || !removeReceipt.objectRemoved ||
        !removeReceipt.changed) {
      removeSucceeded = false;
      break;
    }
  }

  if (!removeSucceeded) {
    setRoomShellApplyStatus(receipt,
                            creative::CreativeRoomShellStatus::
                                RemoveRejected,
                            "creative_room_shell_remove_rejected");
    receipt.toolAfter = context.facade.toolState().activeTool;
    setNoopStatus(receipt, "product_creative_ui_command_rejected");
    return;
  }

  const creative::CreativeFacadeDocumentInstallReceipt installReceipt =
      context.facade.installDocument(std::move(stagedDocument));
  if (!installReceipt.accepted || !installReceipt.changed) {
    setRoomShellApplyStatus(receipt,
                            creative::CreativeRoomShellStatus::
                                InstallRejected,
                            "creative_room_shell_remove_install_rejected");
    receipt.toolAfter = context.facade.toolState().activeTool;
    setNoopStatus(receipt, "product_creative_ui_command_rejected");
    return;
  }

  receipt.shellAccepted = true;
  receipt.shellChanged = true;
  receipt.shellRevisionAfter = context.facade.document().revision();
  receipt.accepted = true;
  receipt.changed = true;
  receipt.toolAfter = context.facade.toolState().activeTool;
  setNoopStatus(receipt, "product_creative_ui_command_applied");
}

void handleCreateObject(ProductCreativeUiCommandExecutionContext& context) {
  ProductCreativeUiCommandFrameReceipt& receipt = context.receipt;
  receipt.commandObjectKind = context.row.objectKind;
  const creative::CreativePlacedCreateRequest placed =
      creative::buildPlacedCreateRequest(context.facade.document(),
                                         context.row.objectKind);
  const creative::CreativeDocumentCreateReceipt createReceipt =
      context.facade.createDocumentObject(placed.createRequest);
  receipt.create.document = createReceipt;
  receipt.create.placementOffsetApplied = placed.offsetApplied;
  receipt.create.placementOffsetX = placed.offsetX;
  receipt.accepted = createReceipt.accepted;
  receipt.changed = createReceipt.changed;
  receipt.toolAfter = context.facade.toolState().activeTool;
  setCommandOutcomeStatus(receipt,
                          createReceipt.accepted,
                          createReceipt.changed);
}

constexpr std::array<ProductCreativeUiCommandHandlerEntry, 9>
    kProductCreativeUiCommandHandlers = {{
        {ProductCreativeUiCommandKind::ToggleSelectedObjectVisibility,
         handleToggleSelectedObjectVisibility},
        {ProductCreativeUiCommandKind::ToggleSelectedObjectLocked,
         handleToggleSelectedObjectLocked},
        {ProductCreativeUiCommandKind::SetActiveTool, handleSetActiveTool},
        {ProductCreativeUiCommandKind::CreateObject, handleCreateObject},
        {ProductCreativeUiCommandKind::RebuildRoom, handleRebuildRoom},
        {ProductCreativeUiCommandKind::UndoLastDocumentChange,
         handleUndoLastDocumentChange},
        {ProductCreativeUiCommandKind::DeleteSelectedObject,
         handleDeleteSelectedObject},
        {ProductCreativeUiCommandKind::GenerateSelectedRoomShell,
         handleGenerateSelectedRoomShell},
        {ProductCreativeUiCommandKind::RemoveSelectedRoomShell,
         handleRemoveSelectedRoomShell},
    }};

[[nodiscard]] ProductCreativeUiCommandHandler findCommandHandler(
    ProductCreativeUiCommandKind commandKind) noexcept {
  for (const ProductCreativeUiCommandHandlerEntry& entry :
       kProductCreativeUiCommandHandlers) {
    if (entry.commandKind == commandKind) {
      return entry.handler;
    }
  }
  return nullptr;
}

}  // namespace

ProductCreativeUiCommandFrameReceipt routeProductCreativeUiCommandFrame(
    const ProductCreativeUiCommandFrameRequest& request) {
  ProductCreativeUiCommandFrameReceipt receipt;
  receipt.requested = true;
  receipt.inputClickPresent = request.inputReceipt.clickPresent;
  receipt.inputConsumed = request.inputReceipt.consumed;
  receipt.inputEnabled = request.inputReceipt.enabled;
  receipt.semanticId = request.inputReceipt.semanticId;

  if (request.creative == nullptr) {
    setNoopStatus(receipt, "product_creative_ui_command_facade_missing");
    return receipt;
  }

  creative::Facade& facade = request.creative->facade;
  receipt.facadeAvailable = true;
  receipt.toolBefore = facade.toolState().activeTool;
  receipt.toolAfter = receipt.toolBefore;

  if (!receipt.inputConsumed) {
    setNoopStatus(receipt, "product_creative_ui_command_not_consumed");
    return receipt;
  }

  if (!receipt.inputEnabled) {
    setNoopStatus(receipt, "product_creative_ui_command_disabled");
    return receipt;
  }

  const ProductCreativeUiCommandCatalogEntry* row =
      findProductCreativeUiCommandBySemanticId(request.inputReceipt.semanticId);
  if (row == nullptr) {
    setNoopStatus(receipt, "product_creative_ui_command_unknown_semantic");
    return receipt;
  }

  receipt.commandKind = row->commandKind;
  ProductCreativeUiCommandHandler handler =
      findCommandHandler(receipt.commandKind);
  if (handler != nullptr) {
    ProductCreativeUiCommandExecutionContext context{
        receipt,
        *request.creative,
        facade,
        *row,
    };
    handler(context);
    return receipt;
  }

  setNoopStatus(receipt, "product_creative_ui_command_unknown_command");
  return receipt;
}

bool productCreativeUiCommandKindHasHandler(
    ProductCreativeUiCommandKind commandKind) noexcept {
  return findCommandHandler(commandKind) != nullptr;
}

}  // namespace iggy3d
