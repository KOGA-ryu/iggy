#include "app/iggy3d/window/CreativeUiCommandFrame.hpp"

#include "app/iggy3d/creative/Facade.hpp"

#include <array>
#include <string_view>

namespace iggy3d {
namespace {

struct ProductCreativeUiCommandRow {
  std::string_view semanticId;
  ProductCreativeUiCommandKind commandKind =
      ProductCreativeUiCommandKind::None;
};

constexpr std::array<ProductCreativeUiCommandRow, 2>
    kProductCreativeUiCommandRows = {{
        {"creative.row.tools.create_room",
         ProductCreativeUiCommandKind::CreateRoom},
        {"creative.row.selection.selected_target",
         ProductCreativeUiCommandKind::ToggleSelectedObjectVisibility},
    }};

[[nodiscard]] const ProductCreativeUiCommandRow* findCommandRow(
    std::string_view semanticId) noexcept {
  for (const ProductCreativeUiCommandRow& row :
       kProductCreativeUiCommandRows) {
    if (row.semanticId == semanticId) {
      return &row;
    }
  }
  return nullptr;
}

void setNoopStatus(ProductCreativeUiCommandFrameReceipt& receipt,
                  std::string_view status) {
  receipt.status = std::string(status);
  receipt.reasonCode = std::string(status);
}

void copyMutationReceipt(ProductCreativeUiCommandFrameReceipt& receipt,
                         const creative::CreativeFacadeMutationReceipt&
                             mutationReceipt) {
  receipt.mutationRequested = mutationReceipt.requested;
  receipt.mutationAccepted = mutationReceipt.accepted;
  receipt.mutationChanged = mutationReceipt.changed;
  receipt.mutationStatus = mutationReceipt.status;
  receipt.documentMutationStatus = mutationReceipt.documentStatus;
  receipt.mutationKind = mutationReceipt.mutationKind;
  receipt.mutationTarget = mutationReceipt.target;
  receipt.mutationObjectId = mutationReceipt.objectId;
  receipt.mutationObjectKind = mutationReceipt.objectKind;
  receipt.visibleBefore = mutationReceipt.visibleBefore;
  receipt.visibleAfter = mutationReceipt.visibleAfter;
  receipt.revisionBefore = mutationReceipt.revisionBefore;
  receipt.revisionAfter = mutationReceipt.revisionAfter;
  receipt.mutationMessage = mutationReceipt.message;
}

void copyCreateReceipt(ProductCreativeUiCommandFrameReceipt& receipt,
                       const creative::CreativeDocumentCreateReceipt&
                           createReceipt) {
  receipt.createRequested = createReceipt.requested;
  receipt.createAccepted = createReceipt.accepted;
  receipt.createChanged = createReceipt.changed;
  receipt.createStatus = createReceipt.status;
  receipt.createObjectId = createReceipt.objectId;
  receipt.createObjectKind = createReceipt.objectKind;
  receipt.createObjectName = createReceipt.objectName;
  receipt.createRevisionBefore = createReceipt.revisionBefore;
  receipt.createRevisionAfter = createReceipt.revisionAfter;
  receipt.createDirtyFlags = createReceipt.creationDirtyFlags;
  receipt.createMessage = createReceipt.message;
  receipt.createReasonCode = createReceipt.reasonCode;
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

  if (request.facade == nullptr) {
    setNoopStatus(receipt, "product_creative_ui_command_facade_missing");
    return receipt;
  }

  creative::Facade& facade = *request.facade;
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

  const ProductCreativeUiCommandRow* row =
      findCommandRow(request.inputReceipt.semanticId);
  if (row == nullptr) {
    setNoopStatus(receipt, "product_creative_ui_command_unknown_semantic");
    return receipt;
  }

  receipt.commandKind = row->commandKind;
  if (receipt.commandKind ==
      ProductCreativeUiCommandKind::ToggleSelectedObjectVisibility) {
    const creative::CreativeFacadeMutationReceipt mutationReceipt =
        facade.toggleSelectedObjectVisibility();
    copyMutationReceipt(receipt, mutationReceipt);
    receipt.accepted = mutationReceipt.accepted;
    receipt.changed = mutationReceipt.changed;
    if (mutationReceipt.accepted && mutationReceipt.changed) {
      setNoopStatus(receipt, "product_creative_ui_command_applied");
    } else if (mutationReceipt.accepted) {
      setNoopStatus(receipt, "product_creative_ui_command_no_change");
    } else {
      setNoopStatus(receipt, "product_creative_ui_command_rejected");
    }
    return receipt;
  }

  if (receipt.commandKind == ProductCreativeUiCommandKind::CreateRoom) {
    const creative::CreativeDocumentCreateReceipt createReceipt =
        facade.createDocumentObject(creative::CreativeObjectKind::Room);
    copyCreateReceipt(receipt, createReceipt);
    receipt.accepted = createReceipt.accepted;
    receipt.changed = createReceipt.changed;
    receipt.toolAfter = facade.toolState().activeTool;
    if (createReceipt.accepted && createReceipt.changed) {
      setNoopStatus(receipt, "product_creative_ui_command_applied");
    } else if (createReceipt.accepted) {
      setNoopStatus(receipt, "product_creative_ui_command_no_change");
    } else {
      setNoopStatus(receipt, "product_creative_ui_command_rejected");
    }
    return receipt;
  }

  setNoopStatus(receipt, "product_creative_ui_command_unknown_command");
  return receipt;
}

}  // namespace iggy3d
