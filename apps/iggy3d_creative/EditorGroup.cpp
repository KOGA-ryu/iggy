#include "EditorGroup.hpp"

#include <SDL3/SDL_log.h>

#include <string>
#include <utility>

#include "EditorEdits.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;

cr::CreativeGroupCommandReceipt applyCreativeEditorGroupCommandWithHistory(
    cr::CreativeAppState& appState,
    std::string_view source) {
  const cr::CreativeSelectionState& selection =
      appState.facade.selectionState();
  const cr::CreativeObject* primary =
      selection.selectedTarget.value == cr::kInvalidId
          ? nullptr
          : appState.facade.findObject(
                static_cast<cr::CreativeObjectId>(
                    selection.selectedTarget.value));
  cr::CreativeObjectId ungroupObjectId = cr::kInvalidObjectId;
  if (primary != nullptr && cr::selectedTargetCount(selection) == 1U) {
    if (primary->kind == cr::CreativeObjectKind::Group) {
      ungroupObjectId = primary->id;
    } else if (primary->parentId.has_value()) {
      const cr::CreativeObject* parent =
          appState.facade.findObject(*primary->parentId);
      if (parent != nullptr && parent->kind == cr::CreativeObjectKind::Group) {
        ungroupObjectId = parent->id;
      }
    }
  }
  cr::CreativeDocumentHistoryTransaction transaction =
      beginEditTransaction(appState.facade, source);
  cr::CreativeGroupCommandReceipt receipt =
      ungroupObjectId != cr::kInvalidObjectId
          ? appState.facade.ungroupObject(ungroupObjectId)
          : appState.facade.groupSelectedObjects();
  static_cast<void>(completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      receipt.accepted && receipt.changed, receipt.reasonCode));
  SDL_Log("iggy3d_creative: %s accepted=%d changed=%d status='%s' "
          "requested=%llu affected=%llu groupId=%llu revision=%llu "
          "reasonCode='%s'",
          std::string(cr::toString(receipt.kind)).c_str(),
          receipt.accepted ? 1 : 0, receipt.changed ? 1 : 0,
          std::string(cr::toString(receipt.status)).c_str(),
          static_cast<unsigned long long>(receipt.requestedObjectCount),
          static_cast<unsigned long long>(receipt.affectedObjectCount),
          static_cast<unsigned long long>(receipt.groupObjectId),
          static_cast<unsigned long long>(receipt.revisionAfter),
          std::string(receipt.reasonCode).c_str());
  return receipt;
}

}  // namespace iggy3d_creative_app
