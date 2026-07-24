#pragma once

#include <string>
#include <string_view>

#include "EditorToolOptions.hpp"

namespace iggy3d::creative {
struct CreativeAuthoredAssetDefinition;
}  // namespace iggy3d::creative

namespace iggy3d_creative_app {

struct CreativeEditorAuthoredAssetLibrary;
struct CreativeDesktopObjectActionContext;
struct CreativeEditorDesktopUiState;
struct CreativeEditorWorldLayoutState;

using CreativeEditorObjectActionCapability =
    iggy3d::creative::CreativeSemanticObjectActionAdmission;
using CreativeEditorObjectActionCapabilities =
    iggy3d::creative::CreativeSemanticObjectActionAdmissions;

// Resolves the current Facade selection once for execution hints and every UI
// surface. Selection identity, hierarchy locks, ownership, and source freshness
// are carried together so callers cannot reconstruct only part of admission.
[[nodiscard]] iggy3d::creative::CreativeSemanticObjectActionFacts
resolveCreativeEditorObjectActionFacts(
    const iggy3d::creative::CreativeAppState& appState,
    const CreativeEditorWorldLayoutState* worldLayout = nullptr);

[[nodiscard]] const CreativeDesktopObjectActionContext&
refreshCreativeEditorDesktopObjectActionContext(
    CreativeEditorDesktopUiState& desktopUi,
    const iggy3d::creative::CreativeAppState& appState,
    const CreativeEditorWorldLayoutState* worldLayout = nullptr);

[[nodiscard]] CreativeEditorObjectActionCapability
creativeEditorObjectActionCapability(
    const CreativeEditorObjectActionCapabilities& capabilities,
    iggy3d::creative::CreativeSemanticObjectAction action) noexcept;
[[nodiscard]] bool creativeEditorObjectActionAvailable(
    const CreativeEditorObjectActionCapabilities& capabilities,
    iggy3d::creative::CreativeSemanticObjectAction action) noexcept;

// Equips a saved authored-asset definition into the active hotbar slot
// (extracted from the SaveSelectionAsAsset object action so the desktop command
// dispatcher can equip too). Returns whether the hotbar accepted the asset. Does
// NOT set a status label — the caller owns messaging.
[[nodiscard]] bool equipCreativeEditorAuthoredAssetToHotbar(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    const iggy3d::creative::CreativeAuthoredAssetDefinition& definition);

[[nodiscard]] bool creativeEditorCommandIsObjectAction(
    CreativeEditorToolOptionsCommandId command) noexcept;

void refreshCreativeEditorObjectActionContext(
    const iggy3d::creative::CreativeAppState& appState,
    const CreativeEditorAuthoredAssetLibrary& authoredAssets,
    CreativeEditorToolOptionsState& state,
    const CreativeEditorWorldLayoutState* worldLayout = nullptr) noexcept;

[[nodiscard]] bool creativeEditorObjectActionEnabled(
    const CreativeEditorState& editor,
    const CreativeEditorToolOptionsState& state,
    CreativeEditorToolOptionsCommandId command) noexcept;

[[nodiscard]] std::string_view creativeEditorObjectActionLabel(
    CreativeEditorToolOptionsCommandId command) noexcept;

[[nodiscard]] std::string creativeEditorObjectActionValueLabel(
    const CreativeEditorToolOptionsState& state,
    CreativeEditorToolOptionsCommandId command);

[[nodiscard]] bool activateCreativeEditorObjectAction(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    CreativeEditorToolOptionsCommandId command,
    const iggy3d::StaticMeshAssetCatalog* assetCatalog = nullptr,
    const CreativePlacementClearanceCache* placementClearanceCache = nullptr);

}  // namespace iggy3d_creative_app
