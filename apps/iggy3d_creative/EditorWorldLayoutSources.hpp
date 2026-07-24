#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include "EditorWorldLayoutContracts.hpp"

namespace iggy3d::creative {
struct CreativeCatalogEntry;
}

namespace iggy3d_creative_app {

[[nodiscard]] const char* creativeEditorWorldLayoutToolLabel(
    CreativeEditorWorldLayoutTool tool) noexcept;
[[nodiscard]] bool creativeEditorWorldLayoutToolIsVerticalConnector(
    CreativeEditorWorldLayoutTool tool) noexcept;
[[nodiscard]] const char* creativeEditorWorldLayoutPaletteCategoryLabel(
    CreativeEditorWorldLayoutPaletteCategory category) noexcept;
[[nodiscard]] const char* creativeEditorWorldLayoutAssetCategoryLabel(
    CreativeEditorWorldLayoutAssetCategory category) noexcept;
[[nodiscard]] const char* creativeEditorWorldLayoutCatalogSnapModeLabel(
    CreativeEditorWorldLayoutCatalogSnapMode mode) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutAssetCategory
classifyCreativeEditorWorldLayoutAsset(
    const cr::CreativeCatalogEntry& entry) noexcept;
[[nodiscard]] bool creativeEditorWorldLayoutCatalogAssetSupportsWallSnap(
    std::string_view categoryId) noexcept;
[[nodiscard]] bool creativeEditorWorldLayoutCatalogAssetIsHostedOpening(
    std::string_view categoryId) noexcept;
[[nodiscard]] bool creativeEditorWorldLayoutCatalogAssetMatchesOpening(
    std::string_view categoryId,
    cr::CreativeBuildingOpeningKind kind) noexcept;
[[nodiscard]] bool creativeEditorWorldLayoutAssetMatchesQuery(
    const cr::CreativeCatalogEntry& entry, std::string_view query);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
selectCreativeEditorWorldLayoutCatalogAsset(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeCatalogEntry& entry);
[[nodiscard]] CreativeEditorWorldLayoutObjectFootprint
planCreativeEditorWorldLayoutObjectFootprint(
    const cr::CreativeWorldLayoutObject& object,
    cr::CreativeGridSettings grid) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutCatalogPlacementPlan
planCreativeEditorWorldLayoutCatalogPlacement(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point,
    cr::CreativeGridSettings grid);

[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
focusCreativeEditorWorldLayoutSource(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
selectCreativeEditorWorldLayoutSource(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index,
    std::size_t preferredLevelIndex =
        cr::kInvalidCreativeWorldLayoutIndex);
[[nodiscard]] cr::CreativeWorldLayoutTable
creativeEditorWorldLayoutSelectionTable(
    CreativeEditorWorldLayoutSelectionKind kind) noexcept;
[[nodiscard]] bool creativeEditorWorldLayoutSourceCanRename(
    cr::CreativeWorldLayoutTable table) noexcept;
[[nodiscard]] bool creativeEditorWorldLayoutSourceCanDuplicate(
    cr::CreativeWorldLayoutTable table) noexcept;
[[nodiscard]] bool creativeEditorWorldLayoutSourceCanDelete(
    cr::CreativeWorldLayoutTable table) noexcept;
[[nodiscard]] bool creativeEditorWorldLayoutSourceCanSetVisible(
    cr::CreativeWorldLayoutTable table) noexcept;
[[nodiscard]] bool creativeEditorWorldLayoutSourceStableKeyMatches(
    const CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index,
    std::string_view stableKey) noexcept;
[[nodiscard]] std::string_view creativeEditorWorldLayoutSourceStableKey(
    const CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
renameCreativeEditorWorldLayoutSource(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index,
    std::string name);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutSourceVisible(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index,
    bool visible);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
rotateCreativeEditorWorldLayoutBuildingSource(
    CreativeEditorWorldLayoutState& state, std::size_t buildingIndex,
    cr::CreativeWorldLayoutBuildingTransformOperation operation);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
duplicateCreativeEditorWorldLayoutSource(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
duplicateCreativeEditorWorldLayoutSource(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index,
    cr::CreativeGridSettings grid);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
deleteCreativeEditorWorldLayoutSource(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index);

[[nodiscard]] bool selectCreativeEditorWorldLayoutObjectSource(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeObject& object);
[[nodiscard]] bool selectCreativeEditorWorldLayoutObjectSource(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeObject& object,
    cr::CreativeGridSettings grid,
    cr::CreativeVec3 worldPoint);
[[nodiscard]] CreativeEditorSelectionSynchronizationReceipt
synchronizeCreativeEditorWorldLayoutSelection(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document,
    const cr::CreativeSelectionState& selection,
    cr::CreativeWorldLayoutSourceRef preferredSource = {});
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
focusCreativeEditorWorldLayoutObjectSource(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeObject& object);
[[nodiscard]] CreativeEditorWorldLayoutAdoptionReceipt
adoptCreativeEditorWorldLayoutObjectSource(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeAppState& appState,
    cr::CreativeObjectId objectId);

}  // namespace iggy3d_creative_app
