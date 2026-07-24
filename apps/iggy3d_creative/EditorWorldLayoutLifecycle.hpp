#pragma once

#include <span>
#include <string>
#include <string_view>

#include "EditorWorldLayoutContracts.hpp"

namespace iggy3d_creative_app {

void resetCreativeEditorWorldLayout(CreativeEditorWorldLayoutState& state,
                                    std::string layoutKey = "world_layout");
void installCreativeEditorWorldLayout(CreativeEditorWorldLayoutState& state,
                                      cr::CreativeWorldLayout layout);
void markCreativeEditorWorldLayoutSaved(
    CreativeEditorWorldLayoutState& state);

[[nodiscard]] bool creativeEditorWorldLayoutDirty(
    const CreativeEditorWorldLayoutState& state);
[[nodiscard]] bool creativeEditorWorldLayoutPreviewActive(
    const CreativeEditorWorldLayoutState& state) noexcept;
[[nodiscard]] const cr::CreativeDocument&
creativeEditorWorldLayoutRenderDocument(
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& liveDocument) noexcept;
[[nodiscard]] const cr::CreativeWorldLayout&
creativeEditorWorldLayoutDisplaySource(
    const CreativeEditorWorldLayoutState& state) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutInspection
inspectCreativeEditorWorldLayout(
    const CreativeEditorWorldLayoutState& state) noexcept;

[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
deleteCreativeEditorWorldLayoutSelection(CreativeEditorWorldLayoutState& state);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
cancelCreativeEditorWorldLayoutPreview(
    CreativeEditorWorldLayoutState& state) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutPreviewReceipt
previewCreativeEditorWorldLayout(CreativeEditorWorldLayoutState& state,
                                 const cr::CreativeDocument& document);
[[nodiscard]] CreativeEditorWorldLayoutState
makeCreativeEditorWorldLayoutLiveEditCandidate(
    const CreativeEditorWorldLayoutState& state);
[[nodiscard]] CreativeEditorWorldLayoutPreviewReceipt
previewCreativeEditorWorldLayoutLiveEditCandidate(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document,
    CreativeEditorWorldLayoutState candidate,
    CreativeEditorWorldLayoutEditReceipt editReceipt,
    std::string_view successMessage);
[[nodiscard]] CreativeEditorWorldLayoutPreviewReceipt
previewCreativeEditorWorldLayoutTerrainPathDraft(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document);
[[nodiscard]] CreativeEditorWorldLayoutApplyReceipt
applyCreativeEditorWorldLayoutLiveEditCandidate(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeAppState& appState,
    CreativeEditorWorldLayoutState candidate,
    CreativeEditorWorldLayoutEditReceipt editReceipt,
    std::string_view historySource,
    std::string_view successMessage);
[[nodiscard]] bool clearCreativeEditorWorldLayoutLiveEditPreview(
    CreativeEditorWorldLayoutState& state) noexcept;
[[nodiscard]] cr::CreativeWorldLayoutTerrainReconciliationResult
reconcileCreativeEditorWorldLayoutTerrain(
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document,
    const cr::CreativeWorldLayout& desiredLayout,
    std::span<const cr::CreativeWorldLayoutTerrainConflictDecision> decisions =
        {});
[[nodiscard]] CreativeEditorWorldLayoutApplyReceipt
confirmCreativeEditorWorldLayout(CreativeEditorWorldLayoutState& state,
                                 cr::CreativeAppState& appState,
                                 std::span<const
                                     cr::CreativeWorldLayoutConflictDecision>
                                     conflictDecisions = {},
                                 std::span<const
                                     cr::CreativeWorldLayoutTerrainConflictDecision>
                                     terrainConflictDecisions = {});

}  // namespace iggy3d_creative_app
