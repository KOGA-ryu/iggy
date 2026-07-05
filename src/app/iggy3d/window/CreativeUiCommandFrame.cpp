#include "app/iggy3d/window/CreativeUiCommandFrame.hpp"

#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/Placement.hpp"

#include <array>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

namespace iggy3d {
namespace {

struct ProductCreativeUiCommandRow {
  std::string_view semanticId;
  ProductCreativeUiCommandKind commandKind =
      ProductCreativeUiCommandKind::None;
  creative::Tool tool = creative::Tool::Select;
  creative::CreativeObjectKind objectKind =
      creative::CreativeObjectKind::Unknown;
};

constexpr std::array<ProductCreativeUiCommandRow, 8>
    kProductCreativeUiCommandRows = {{
        {"creative.row.tools.tool_select",
         ProductCreativeUiCommandKind::SetActiveTool,
         creative::Tool::Select,
         creative::CreativeObjectKind::Unknown},
        {"creative.row.tools.tool_move",
         ProductCreativeUiCommandKind::SetActiveTool,
         creative::Tool::Move,
         creative::CreativeObjectKind::Unknown},
        {"creative.row.tools.tool_measure",
         ProductCreativeUiCommandKind::SetActiveTool,
         creative::Tool::Measure,
         creative::CreativeObjectKind::Unknown},
        {"creative.row.tools.tool_navigate",
         ProductCreativeUiCommandKind::SetActiveTool,
         creative::Tool::Navigate,
         creative::CreativeObjectKind::Unknown},
        {"creative.row.create.create_room",
         ProductCreativeUiCommandKind::CreateObject,
         creative::Tool::Select,
         creative::CreativeObjectKind::Room},
        {"creative.row.create.create_crate",
         ProductCreativeUiCommandKind::CreateObject,
         creative::Tool::Select,
         creative::CreativeObjectKind::Crate},
        {"creative.row.selection.inspector_visible",
         ProductCreativeUiCommandKind::ToggleSelectedObjectVisibility,
         creative::Tool::Select,
         creative::CreativeObjectKind::Unknown},
        {"creative.row.selection.inspector_locked",
         ProductCreativeUiCommandKind::ToggleSelectedObjectLocked,
         creative::Tool::Select,
         creative::CreativeObjectKind::Unknown},
    }};

[[nodiscard]] std::string formatPlacementOffset(double value) {
  std::ostringstream stream;
  stream << std::fixed << std::setprecision(2) << value;
  return stream.str();
}

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
  receipt.lockedBefore = mutationReceipt.lockedBefore;
  receipt.lockedAfter = mutationReceipt.lockedAfter;
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
          ProductCreativeUiCommandKind::ToggleSelectedObjectVisibility ||
      receipt.commandKind ==
          ProductCreativeUiCommandKind::ToggleSelectedObjectLocked) {
    const creative::CreativeFacadeMutationReceipt mutationReceipt =
        receipt.commandKind ==
                ProductCreativeUiCommandKind::ToggleSelectedObjectLocked
            ? facade.toggleSelectedObjectLocked()
            : facade.toggleSelectedObjectVisibility();
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

  if (receipt.commandKind == ProductCreativeUiCommandKind::SetActiveTool) {
    receipt.commandTool = row->tool;
    const bool toolChanged = facade.setActiveTool(row->tool);
    receipt.toolAfter = facade.toolState().activeTool;
    receipt.accepted = true;
    receipt.changed = toolChanged;
    setNoopStatus(receipt,
                  toolChanged ? "product_creative_ui_command_applied"
                              : "product_creative_ui_command_no_change");
    return receipt;
  }

  if (receipt.commandKind == ProductCreativeUiCommandKind::CreateObject) {
    receipt.commandObjectKind = row->objectKind;
    const creative::CreativePlacedCreateRequest placed =
        creative::buildPlacedCreateRequest(facade.document(),
                                           row->objectKind);
    const creative::CreativeDocumentCreateReceipt createReceipt =
        facade.createDocumentObject(placed.createRequest);
    copyCreateReceipt(receipt, createReceipt);
    receipt.accepted = createReceipt.accepted;
    receipt.changed = createReceipt.changed;
    receipt.toolAfter = facade.toolState().activeTool;
    if (createReceipt.accepted && createReceipt.changed &&
        placed.offsetApplied) {
      receipt.createMessage.append(" placement_offset_x=");
      receipt.createMessage.append(formatPlacementOffset(placed.offsetX));
      receipt.createReasonCode.append("_placement_offset");
    }
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
