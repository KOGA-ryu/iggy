#pragma once

#include <string>
#include <string_view>

#include "EditorToolOptions.hpp"

namespace iggy3d::creative {
struct CreativeAuthoredAssetDefinition;
}  // namespace iggy3d::creative

namespace iggy3d_creative_app {

struct CreativeEditorAuthoredAssetLibrary;
struct CreativeEditorWorldLayoutState;

// Resolves presentation-time availability from the same semantic policy used
// by mutation owners. Owners still validate at execution time; this snapshot
// keeps every UI surface and input hint consistent before dispatch.
[[nodiscard]] CreativeEditorObjectActionCapabilities
buildCreativeEditorObjectActionCapabilities(
    const iggy3d::creative::CreativeSemanticSelectionResolution& selection,
    bool allUnlocked, bool worldLayoutSynchronized) noexcept;
[[nodiscard]] CreativeEditorObjectActionCapabilities
buildCreativeEditorObjectActionCapabilities(
    const iggy3d::creative::CreativeSemanticSelectionSetResolution& selection,
    bool allUnlocked, bool worldLayoutSynchronized) noexcept;

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
