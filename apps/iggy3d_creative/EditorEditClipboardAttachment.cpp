#include "EditorEdits.hpp"
#include "EditorEditsInternal.hpp"

#include "EditorAttachmentPlacement.hpp"
#include "EditorWorldLayoutLifecycle.hpp"
#include "EditorWorldLayoutSources.hpp"
#include "EditorWorldLayoutHistory.hpp"

#include <SDL3/SDL_log.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/mutation/Mutation.hpp"
#include "app/iggy3d/creative/recipes/PatternRecipe.hpp"
#include "app/iggy3d/creative/tools/Group.hpp"
#include "app/iggy3d/creative/tools/Select.hpp"
#include "app/iggy3d/creative/tools/SelectionResolution.hpp"

namespace iggy3d_creative_app {
namespace creative = iggy3d::creative;
CreativeEditorSemanticEditReceipt detachCreativeEditorObjectWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    creative::CreativeObjectId objectId,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout) {
  return applySemanticDocumentObjectMutationWithUndo(
      appState, history, objectId,
      creative::CreativeMutationKind::DetachFrom,
      creative::CreativeMutationPayload{}, "DETACH", source, worldLayout);
}

CreativeEditorObjectReattachmentReceipt
reattachCreativeEditorObjectWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    const CreativeEditorObjectReattachmentPlan& plan,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout) {
  const creative::CreativeDocument& document = appState.facade.document();
  CreativeEditorObjectReattachmentReceipt receipt;
  if (!plan.accepted ||
      plan.status != CreativeEditorObjectReattachmentStatus::Ready ||
      document.id() != plan.documentId ||
      document.revision() != plan.documentRevision) {
    receipt = applyCreativeEditorObjectReattachment(appState.facade, plan);
  } else {
    const creative::CreativeStructuralMutationAdmission admission =
        creative::resolveCreativeStructuralMutationAdmission(
            document, plan.sourceObjectId,
            worldLayout != nullptr ? &worldLayout->source : nullptr);
    if (!admission.allowed) {
      receipt.status =
          CreativeEditorObjectReattachmentStatus::SourceOwned;
      receipt.sourceObjectId = plan.sourceObjectId;
      receipt.targetObjectId = plan.targetObjectId;
      receipt.revisionBefore = document.revision();
      receipt.revisionAfter = document.revision();
    } else {
      StandaloneEditTransaction transaction =
          beginEditTransaction(appState.facade, source);
      receipt = applyCreativeEditorObjectReattachment(appState.facade, plan);
      static_cast<void>(completeEditTransaction(
          history, std::move(transaction), appState.facade,
          receipt.accepted && receipt.changed, toString(receipt.status)));
    }
  }
  SDL_Log("iggy3d_creative: REATTACH source='%s' objectId=%llu targetId=%llu "
          "accepted=%d changed=%d revisionBefore=%llu revisionAfter=%llu "
          "status='%s'",
          std::string(source).c_str(),
          static_cast<unsigned long long>(receipt.sourceObjectId),
          static_cast<unsigned long long>(receipt.targetObjectId),
          receipt.accepted ? 1 : 0, receipt.changed ? 1 : 0,
          static_cast<unsigned long long>(receipt.revisionBefore),
          static_cast<unsigned long long>(receipt.revisionAfter),
          std::string(toString(receipt.status)).c_str());
  return receipt;
}

creative::CreativeClipboardCopyReceipt copySelectionToClipboard(
    creative::CreativeAppState& appState,
    std::string_view source) {
  const creative::CreativeClipboardCopyReceipt receipt =
      appState.facade.copySelectedObjectsToClipboard(appState.clipboard);
  SDL_Log("iggy3d_creative: CLIPBOARD copy source='%s' accepted=%d "
          "status='%s' requested=%llu copied=%llu failedObjectId=%llu "
          "reasonCode='%s'",
          std::string(source).c_str(), receipt.accepted ? 1 : 0,
          std::string(creative::toString(receipt.status)).c_str(),
          static_cast<unsigned long long>(receipt.requestedObjectCount),
          static_cast<unsigned long long>(receipt.copiedObjectCount),
          static_cast<unsigned long long>(receipt.failedObjectId),
          receipt.reasonCode.c_str());
  return receipt;
}

creative::CreativeClipboardCutReceipt cutSelectionToClipboardWithHistory(
    creative::CreativeAppState& appState,
    std::string_view source) {
  const std::uint64_t depthBefore =
      creative::creativeUndoDepth(appState.history);
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  creative::CreativeClipboardCutReceipt receipt =
      appState.facade.cutSelectedObjectsToClipboard(appState.clipboard);
  (void)completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      receipt.accepted && receipt.changed, receipt.reasonCode);
  SDL_Log("iggy3d_creative: CLIPBOARD cut source='%s' accepted=%d changed=%d "
          "status='%s' requested=%llu cut=%llu failedObjectId=%llu "
          "undoBefore=%llu undoAfter=%llu reasonCode='%s'",
          std::string(source).c_str(), receipt.accepted ? 1 : 0,
          receipt.changed ? 1 : 0,
          std::string(creative::toString(receipt.status)).c_str(),
          static_cast<unsigned long long>(receipt.requestedObjectCount),
          static_cast<unsigned long long>(receipt.cutObjectCount),
          static_cast<unsigned long long>(receipt.failedObjectId),
          static_cast<unsigned long long>(depthBefore),
          static_cast<unsigned long long>(
              creative::creativeUndoDepth(appState.history)),
          receipt.reasonCode.c_str());
  return receipt;
}

CreativeEditorSemanticEditReceipt
cutCreativeEditorSelectionToClipboardWithHistory(
    creative::CreativeAppState& appState,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout) {
  CreativeEditorSemanticEditReceipt outcome;
  const CreativeEditorResolvedAction action = resolveEditorAction(
      appState, {}, worldLayout,
      creative::CreativeSemanticObjectAction::Cut);
  if (!action.admission.allowed ||
      (action.admission.route !=
           creative::CreativeSemanticObjectActionRoute::Document &&
       action.admission.route !=
           creative::CreativeSemanticObjectActionRoute::SemanticDocument)) {
    outcome.reasonCode = std::string(action.admission.reasonCode);
    return outcome;
  }
  const creative::CreativeClipboardCutReceipt cut =
      cutSelectionToClipboardWithHistory(appState, source);
  outcome.accepted = cut.accepted;
  outcome.changed = cut.changed;
  outcome.affectedObjectCount = cut.cutObjectCount;
  outcome.reasonCode = cut.reasonCode;
  return outcome;
}

creative::CreativeClipboardPasteReceipt pasteClipboardWithHistory(
    creative::CreativeAppState& appState,
    const creative::CreativeClipboardPasteRequest& request,
    std::string_view source) {
  return pasteClipboardWithHistory(appState, appState.clipboard, request, source);
}

creative::CreativeClipboardPasteReceipt pasteClipboardWithHistory(
    creative::CreativeAppState& appState,
    const creative::CreativeClipboard& clipboard,
    const creative::CreativeClipboardPasteRequest& request,
    std::string_view source) {
  const std::uint64_t depthBefore =
      creative::creativeUndoDepth(appState.history);
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  creative::CreativeClipboardPasteReceipt receipt =
      appState.facade.pasteClipboard(clipboard, request);
  (void)completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      receipt.accepted && receipt.changed, receipt.reasonCode);
  SDL_Log("iggy3d_creative: CLIPBOARD paste source='%s' accepted=%d changed=%d "
          "status='%s' requested=%llu pasted=%llu failedObjectId=%llu "
          "undoBefore=%llu undoAfter=%llu reasonCode='%s'",
          std::string(source).c_str(), receipt.accepted ? 1 : 0,
          receipt.changed ? 1 : 0,
          std::string(creative::toString(receipt.status)).c_str(),
          static_cast<unsigned long long>(receipt.requestedObjectCount),
          static_cast<unsigned long long>(receipt.pastedObjectCount),
          static_cast<unsigned long long>(receipt.failedObjectId),
          static_cast<unsigned long long>(depthBefore),
          static_cast<unsigned long long>(
              creative::creativeUndoDepth(appState.history)),
          receipt.reasonCode.c_str());
  return receipt;
}

}  // namespace iggy3d_creative_app
