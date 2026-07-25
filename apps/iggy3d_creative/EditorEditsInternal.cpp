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
// Resolves the object ids a batch desktop edit should act on: the explicit span
// when non-empty, otherwise the current selection (mirroring the facade's
// selectedObjectIds fallback — the ordered list, or the primary alone).
[[nodiscard]] std::vector<creative::CreativeObjectId> gatherDesktopTargetIds(
    const creative::CreativeAppState& appState,
    std::span<const creative::CreativeObjectId> explicitIds) {
  std::vector<creative::CreativeObjectId> ids;
  if (!explicitIds.empty()) {
    ids.assign(explicitIds.begin(), explicitIds.end());
    return ids;
  }
  const creative::CreativeSelectionState& selection =
      appState.facade.selectionState();
  const std::span<const creative::TargetRef> targets =
      creative::selectedTargetList(selection);
  if (targets.empty()) {
    if (selection.selectedTarget.value != creative::kInvalidId) {
      ids.push_back(static_cast<creative::CreativeObjectId>(
          selection.selectedTarget.value));
    }
    return ids;
  }
  ids.reserve(targets.size());
  for (const creative::TargetRef& target : targets) {
    if (target.value != creative::kInvalidId) {
      ids.push_back(static_cast<creative::CreativeObjectId>(target.value));
    }
  }
  return ids;
}

[[nodiscard]] bool worldLayoutSourceSynchronized(
    const CreativeEditorWorldLayoutState* worldLayout) noexcept {
  return worldLayout != nullptr &&
         worldLayout->generatedRevision == worldLayout->revision;
}

[[nodiscard]] CreativeEditorResolvedAction resolveEditorAction(
    const creative::CreativeAppState& appState,
    std::span<const creative::CreativeObjectId> explicitIds,
    const CreativeEditorWorldLayoutState* worldLayout,
    creative::CreativeSemanticObjectAction action) {
  CreativeEditorResolvedAction result;
  result.objectIds = gatherDesktopTargetIds(appState, explicitIds);
  creative::CreativeObjectId primaryObjectId = creative::kInvalidObjectId;
  const creative::Id selectedPrimary =
      appState.facade.selectionState().selectedTarget.value;
  if (selectedPrimary != creative::kInvalidId) {
    const creative::CreativeObjectId candidate =
        static_cast<creative::CreativeObjectId>(selectedPrimary);
    if (std::find(result.objectIds.begin(), result.objectIds.end(),
                  candidate) != result.objectIds.end()) {
      primaryObjectId = candidate;
    }
  }
  if (primaryObjectId == creative::kInvalidObjectId &&
      !result.objectIds.empty()) {
    primaryObjectId = result.objectIds.front();
  }
  result.facts = creative::resolveCreativeSemanticObjectActionFacts(
      appState.facade.document(), result.objectIds, primaryObjectId,
      worldLayout != nullptr ? &worldLayout->source : nullptr,
      worldLayoutSourceSynchronized(worldLayout));
  result.admission =
      creative::resolveCreativeSemanticObjectActionAdmission(
          result.facts, action);
  return result;
}

[[nodiscard]] CreativeEditorResolvedObjectAction resolveEditorObjectAction(
    const creative::CreativeAppState& appState,
    creative::CreativeObjectId objectId,
    const CreativeEditorWorldLayoutState* worldLayout,
    creative::CreativeSemanticObjectAction action) {
  CreativeEditorResolvedObjectAction result;
  result.facts = creative::resolveCreativeSemanticObjectActionFacts(
      appState.facade.document(), objectId,
      worldLayout != nullptr ? &worldLayout->source : nullptr,
      worldLayoutSourceSynchronized(worldLayout));
  result.admission =
      creative::resolveCreativeSemanticObjectActionAdmission(
          result.facts, action);
  return result;
}

[[nodiscard]] CreativeEditorSemanticEditReceipt
applySemanticDocumentObjectMutationWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    creative::CreativeObjectId objectId,
    creative::CreativeMutationKind mutationKind,
    creative::CreativeMutationPayload payload,
    std::string_view operation,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout) {
  CreativeEditorSemanticEditReceipt outcome;
  const creative::CreativeStructuralMutationAdmission admission =
      creative::resolveCreativeStructuralMutationAdmission(
          appState.facade.document(), objectId,
          worldLayout != nullptr ? &worldLayout->source : nullptr);
  if (!admission.allowed) {
    outcome.reasonCode = std::string(admission.reasonCode);
    return outcome;
  }

  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  const creative::CreativeDocumentMutationReceipt receipt =
      appState.facade.mutateObject(objectId, mutationKind, std::move(payload));
  outcome.accepted = creative::documentMutationSucceeded(receipt.status);
  outcome.changed = outcome.accepted && receipt.changed;
  outcome.affectedObjectCount = outcome.changed ? 1U : 0U;
  outcome.reasonCode = receipt.message;
  static_cast<void>(completeEditTransaction(
      history, std::move(transaction), appState.facade, outcome.changed,
      outcome.reasonCode));
  SDL_Log("iggy3d_creative: SEMANTIC OBJECT MUTATION operation='%s' "
          "source='%s' objectId=%llu accepted=%d changed=%d "
          "revisionBefore=%llu revisionAfter=%llu reasonCode='%s'",
          std::string(operation).c_str(), std::string(source).c_str(),
          static_cast<unsigned long long>(objectId),
          outcome.accepted ? 1 : 0, outcome.changed ? 1 : 0,
          static_cast<unsigned long long>(receipt.revisionBefore),
          static_cast<unsigned long long>(receipt.revisionAfter),
          outcome.reasonCode.c_str());
  return outcome;
}

}  // namespace iggy3d_creative_app
