#pragma once

#include "EditorEdits.hpp"

#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "app/iggy3d/creative/mutation/Mutation.hpp"
#include "app/iggy3d/creative/tools/SelectionResolution.hpp"

namespace iggy3d_creative_app {

namespace creative = iggy3d::creative;

struct CreativeEditorResolvedAction {
  std::vector<creative::CreativeObjectId> objectIds;
  creative::CreativeSemanticObjectActionFacts facts;
  creative::CreativeSemanticObjectActionAdmission admission;
};

struct CreativeEditorResolvedObjectAction {
  creative::CreativeSemanticObjectActionFacts facts;
  creative::CreativeSemanticObjectActionAdmission admission;
};

[[nodiscard]] std::vector<creative::CreativeObjectId> gatherDesktopTargetIds(
    const creative::CreativeAppState& appState,
    std::span<const creative::CreativeObjectId> explicitIds);

[[nodiscard]] bool worldLayoutSourceSynchronized(
    const CreativeEditorWorldLayoutState* worldLayout) noexcept;

[[nodiscard]] CreativeEditorResolvedAction resolveEditorAction(
    const creative::CreativeAppState& appState,
    std::span<const creative::CreativeObjectId> explicitIds,
    const CreativeEditorWorldLayoutState* worldLayout,
    creative::CreativeSemanticObjectAction action);

[[nodiscard]] CreativeEditorResolvedObjectAction resolveEditorObjectAction(
    const creative::CreativeAppState& appState,
    creative::CreativeObjectId objectId,
    const CreativeEditorWorldLayoutState* worldLayout,
    creative::CreativeSemanticObjectAction action);

[[nodiscard]] CreativeEditorSemanticEditReceipt
applySemanticDocumentObjectMutationWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    creative::CreativeObjectId objectId,
    creative::CreativeMutationKind mutationKind,
    creative::CreativeMutationPayload payload,
    std::string_view operation,
    std::string_view source,
    CreativeEditorWorldLayoutState* worldLayout);

[[nodiscard]] creative::CreativeDocumentMutationReceipt
renameDocumentObjectWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    creative::CreativeObjectId objectId,
    std::string name,
    std::string_view source);

[[nodiscard]] CreativeStandaloneBatchEditReceipt
setDocumentObjectsVisibleWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    std::span<const creative::CreativeObjectId> objectIds,
    bool visible,
    std::string_view source);

[[nodiscard]] CreativeStandaloneBatchEditReceipt
setDocumentObjectsLockedWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    std::span<const creative::CreativeObjectId> objectIds,
    bool locked,
    std::string_view source);

[[nodiscard]] CreativeStandaloneBatchEditReceipt
setDocumentObjectTransformWithUndo(
    creative::CreativeAppState& appState,
    StandaloneEditHistory& history,
    creative::CreativeObjectId objectId,
    const creative::CreativeTransform& transform,
    bool setPosition,
    bool setRotation,
    bool setScale,
    std::string_view source);

}  // namespace iggy3d_creative_app
