#include "app/iggy3d/creative/CreativeUiCommandDiagnostics.hpp"

#include "app/iggy3d/creative/bridge/UiCommandFrame.hpp"
#include "app/iggy3d/creative/Facade.hpp"

#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

namespace iggy3d {
namespace {

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

std::string formatPlacementOffset(double value) {
  std::ostringstream stream;
  stream << std::fixed << std::setprecision(2) << value;
  return stream.str();
}

ProductCreativeUiCommandMutationDiagnostics toDiagnostics(
    const creative::CreativeFacadeMutationReceipt& receipt) {
  ProductCreativeUiCommandMutationDiagnostics fields;
  fields.requested = receipt.requested;
  fields.accepted = receipt.accepted;
  fields.changed = receipt.changed;
  fields.status = std::string(creative::toString(receipt.status));
  fields.documentStatus = std::string(creative::toString(receipt.documentStatus));
  fields.kind = std::string(creative::toString(receipt.mutationKind));
  fields.target = receipt.target.value;
  fields.objectId = receipt.objectId;
  fields.objectKind = std::string(creative::toString(receipt.objectKind));
  fields.visibleBefore = receipt.visibleBefore;
  fields.visibleAfter = receipt.visibleAfter;
  fields.lockedBefore = receipt.lockedBefore;
  fields.lockedAfter = receipt.lockedAfter;
  fields.revisionBefore = receipt.revisionBefore;
  fields.revisionAfter = receipt.revisionAfter;
  fields.message = receipt.message.empty() ? "none" : receipt.message;
  return fields;
}

ProductCreativeUiCommandCreateDiagnostics toDiagnostics(
    const ProductCreativeUiCommandCreateOutcome& outcome) {
  ProductCreativeUiCommandCreateDiagnostics fields;
  const creative::CreativeDocumentCreateReceipt& receipt = outcome.document;
  fields.requested = receipt.requested;
  fields.accepted = receipt.accepted;
  fields.changed = receipt.changed;
  fields.status = std::string(creative::toString(receipt.status));
  fields.objectId = receipt.objectId;
  fields.objectKind = std::string(creative::toString(receipt.objectKind));
  fields.objectName = receipt.objectName.empty() ? "none" : receipt.objectName;
  fields.revisionBefore = receipt.revisionBefore;
  fields.revisionAfter = receipt.revisionAfter;
  fields.dirtyFlags = receipt.creationDirtyFlags;
  fields.message = receipt.requested && !receipt.message.empty()
                       ? std::string(receipt.message)
                       : "none";
  fields.reasonCode = receipt.requested && !receipt.reasonCode.empty()
                          ? std::string(receipt.reasonCode)
                          : "none";
  if (receipt.accepted && receipt.changed && outcome.placementOffsetApplied) {
    fields.message += " placement_offset_x=" +
                      formatPlacementOffset(outcome.placementOffsetX);
    fields.reasonCode += "_placement_offset";
  }
  return fields;
}

ProductCreativeUiCommandDeleteDiagnostics toDiagnostics(
    const ProductCreativeUiCommandRemoveOutcome& outcome) {
  ProductCreativeUiCommandDeleteDiagnostics fields;
  const creative::CreativeDocumentRemoveReceipt& receipt = outcome.document;
  fields.requested = receipt.requested;
  fields.accepted = receipt.accepted;
  fields.changed = receipt.changed;
  fields.removed = receipt.objectRemoved;
  fields.objectId = receipt.objectId;
  fields.objectKind = std::string(creative::toString(receipt.objectKind));
  fields.objectName = receipt.objectName.empty() ? "none" : receipt.objectName;
  fields.revisionBefore = receipt.revisionBefore;
  fields.revisionAfter = receipt.revisionAfter;
  fields.dirtyFlags = receipt.removalDirtyFlags;
  if (outcome.noSelection) {
    fields.status = "NoSelection";
    fields.message = "no_selection";
    fields.reasonCode = "no_selection";
  } else if (receipt.requested) {
    fields.status = std::string(creative::toString(receipt.status));
    fields.message = receipt.message;
    fields.reasonCode = receipt.reasonCode;
  }
  return fields;
}

ProductCreativeUiCommandUndoDiagnostics toDiagnostics(
    const creative::CreativeDocumentUndoApplyReceipt& receipt) {
  ProductCreativeUiCommandUndoDiagnostics fields;
  fields.requested = receipt.requested;
  fields.accepted = receipt.accepted;
  fields.changed = receipt.changed;
  fields.hadSnapshot = receipt.hadSnapshot;
  fields.documentId = receipt.documentId;
  fields.revisionBefore = receipt.revisionBefore;
  fields.revisionAfter = receipt.revisionAfter;
  fields.objectCountBefore = receipt.objectCountBefore;
  fields.objectCountAfter = receipt.objectCountAfter;
  fields.depthBefore = receipt.depthBefore;
  fields.depthAfter = receipt.depthAfter;
  fields.status = receipt.status;
  fields.message = receipt.message;
  fields.reasonCode = receipt.reasonCode;
  return fields;
}

ProductCreativeUiCommandRoomShellDiagnostics toRoomShellDiagnostics(
    const ProductCreativeUiCommandFrameReceipt& receipt) {
  ProductCreativeUiCommandRoomShellDiagnostics fields;
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
  fields.status = receipt.shellStatus.empty()
                      ? "creative_room_shell_not_requested"
                      : receipt.shellStatus;
  fields.reasonCode = receipt.shellReasonCode.empty()
                          ? "none"
                          : receipt.shellReasonCode;
  fields.message = receipt.shellMessage.empty() ? "none" : receipt.shellMessage;
  return fields;
}

}  // namespace

ProductCreativeUiCommandDiagnostics toProductCreativeUiCommandDiagnostics(
    const ProductCreativeUiCommandFrameReceipt& receipt) {
  ProductCreativeUiCommandDiagnostics fields;
  fields.requested = receipt.requested;
  fields.facadeAvailable = receipt.facadeAvailable;
  fields.inputConsumed = receipt.inputConsumed;
  fields.inputEnabled = receipt.inputEnabled;
  fields.accepted = receipt.accepted;
  fields.changed = receipt.changed;
  fields.kind =
      std::string(productCreativeUiCommandKindReceiptName(receipt.commandKind));
  fields.tool = receipt.commandKind == ProductCreativeUiCommandKind::SetActiveTool
                    ? std::string(creativeToolReceiptName(receipt.commandTool))
                    : std::string("none");
  fields.objectKind = std::string(creative::toString(receipt.commandObjectKind));
  fields.toolBefore = std::string(creativeToolReceiptName(receipt.toolBefore));
  fields.toolAfter = std::string(creativeToolReceiptName(receipt.toolAfter));
  fields.semanticId = receipt.semanticId.empty() ? "none" : receipt.semanticId;
  fields.status = receipt.status;
  fields.reasonCode = receipt.reasonCode;
  fields.mutation = toDiagnostics(receipt.mutation);
  fields.create = toDiagnostics(receipt.create);
  fields.deleteObject = toDiagnostics(receipt.remove);
  fields.undo = toDiagnostics(receipt.undo);
  fields.shell = toRoomShellDiagnostics(receipt);
  return fields;
}

}  // namespace iggy3d
