#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/world/WorldLayout.hpp"
#include "app/iggy3d/creative/world/WorldLayoutArchitecture.hpp"
#include "app/iggy3d/creative/world/WorldLayoutBuildingOps.hpp"
#include "app/iggy3d/creative/world/WorldLayoutTerrainReconciliation.hpp"

#include "EditorWorldLayoutElevation.hpp"
#include "EditorWorldLayoutDiagnostics.hpp"
#include "EditorWorldLayoutState.hpp"

namespace iggy3d::creative {
struct CreativeCatalogEntry;
}

namespace iggy3d_creative_app {

struct CreativeEditorWorldLayoutEditReceipt {
  bool accepted = false;
  bool changed = false;
  std::string reasonCode = "creative_editor_world_layout_not_requested";
};

struct CreativeEditorWorldLayoutPreviewReceipt {
  bool accepted = false;
  bool changed = false;
  cr::CreativeWorldLayoutStatus status =
      cr::CreativeWorldLayoutStatus::NotRequested;
  std::string reasonCode = "creative_editor_world_layout_preview_not_requested";
};

struct CreativeEditorWorldLayoutApplyReceipt {
  bool accepted = false;
  bool changed = false;
  cr::CreativeWorldLayoutApplyReceipt apply;
  std::string reasonCode = "creative_editor_world_layout_apply_not_requested";
};

struct CreativeEditorWorldLayoutAdoptionReceipt {
  bool accepted = false;
  bool changed = false;
  cr::CreativeWorldLayoutTable table = cr::CreativeWorldLayoutTable::None;
  std::size_t index = cr::kInvalidCreativeWorldLayoutIndex;
  cr::CreativeWorldLayoutApplyReceipt apply;
  std::string reasonCode =
      "creative_editor_world_layout_adoption_not_requested";
};

[[nodiscard]] const char* creativeEditorWorldLayoutToolLabel(
    CreativeEditorWorldLayoutTool tool) noexcept;
[[nodiscard]] bool creativeEditorWorldLayoutToolIsVerticalConnector(
    CreativeEditorWorldLayoutTool tool) noexcept;
[[nodiscard]] const char* creativeEditorWorldLayoutPaletteCategoryLabel(
    CreativeEditorWorldLayoutPaletteCategory category) noexcept;
[[nodiscard]] std::span<const CreativeEditorWorldLayoutPaletteEntry>
creativeEditorWorldLayoutPaletteEntries() noexcept;
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

void resetCreativeEditorWorldLayout(CreativeEditorWorldLayoutState& state,
                                    std::string layoutKey = "world_layout");
void installCreativeEditorWorldLayout(CreativeEditorWorldLayoutState& state,
                                      cr::CreativeWorldLayout layout);
void markCreativeEditorWorldLayoutSaved(
    CreativeEditorWorldLayoutState& state) noexcept;

[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
focusCreativeEditorWorldLayoutSource(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
selectCreativeEditorWorldLayoutSource(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index);
[[nodiscard]] cr::CreativeWorldLayoutTable
creativeEditorWorldLayoutSelectionTable(
    CreativeEditorWorldLayoutSelectionKind kind) noexcept;

[[nodiscard]] bool creativeEditorWorldLayoutSourceCanRename(
    cr::CreativeWorldLayoutTable table) noexcept;
[[nodiscard]] bool creativeEditorWorldLayoutSourceCanDuplicate(
    cr::CreativeWorldLayoutTable table) noexcept;
[[nodiscard]] bool creativeEditorWorldLayoutSourceCanDelete(
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
duplicateCreativeEditorWorldLayoutSource(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
deleteCreativeEditorWorldLayoutSource(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index);

// Links a picked generated object back to the synchronized 2D source symbol.
// The ordinary document selection remains available to 3D tools.
[[nodiscard]] bool selectCreativeEditorWorldLayoutObjectSource(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeObject& object);
[[nodiscard]] bool selectCreativeEditorWorldLayoutObjectSource(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeObject& object,
    cr::CreativeGridSettings grid,
    cr::CreativeVec3 worldPoint);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
focusCreativeEditorWorldLayoutObjectSource(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeObject& object);
[[nodiscard]] CreativeEditorWorldLayoutAdoptionReceipt
adoptCreativeEditorWorldLayoutObjectSource(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeAppState& appState,
    cr::CreativeObjectId objectId);

[[nodiscard]] bool creativeEditorWorldLayoutDirty(
    const CreativeEditorWorldLayoutState& state) noexcept;
[[nodiscard]] bool creativeEditorWorldLayoutPreviewActive(
    const CreativeEditorWorldLayoutState& state) noexcept;
[[nodiscard]] const cr::CreativeDocument&
creativeEditorWorldLayoutRenderDocument(
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& liveDocument) noexcept;
[[nodiscard]] const cr::CreativeWorldLayout&
creativeEditorWorldLayoutDisplaySource(
    const CreativeEditorWorldLayoutState& state) noexcept;

[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutTool(CreativeEditorWorldLayoutState& state,
                                 CreativeEditorWorldLayoutTool tool);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutPoint(CreativeEditorWorldLayoutState& state,
                                    CreativeEditorWorldLayoutPoint point);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutPoint(CreativeEditorWorldLayoutState& state,
                                    CreativeEditorWorldLayoutPoint point,
                                    cr::CreativeGridSettings grid);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutGesture(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutGesturePhase phase,
    CreativeEditorWorldLayoutPoint point = {});
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
createCreativeEditorWorldLayoutBuildingShell(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutRoomSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
createCreativeEditorWorldLayoutBuildingBlockout(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutBuildingBlockoutSettings settings);
[[nodiscard]] bool readCreativeEditorWorldLayoutBuildingBlockoutSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t buildingIndex,
    CreativeEditorWorldLayoutBuildingBlockoutSettings& output) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
updateCreativeEditorWorldLayoutBuildingBlockout(
    CreativeEditorWorldLayoutState& state, std::size_t buildingIndex,
    CreativeEditorWorldLayoutBuildingBlockoutSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
createCreativeEditorWorldLayoutRoom(
    CreativeEditorWorldLayoutState& state, std::size_t levelIndex,
    CreativeEditorWorldLayoutRoomSettings settings);
[[nodiscard]] bool readCreativeEditorWorldLayoutRoomSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t roomIndex,
    CreativeEditorWorldLayoutRoomSettings& output) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutRoomSettings(
    CreativeEditorWorldLayoutState& state, std::size_t roomIndex,
    CreativeEditorWorldLayoutRoomSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutApplyReceipt
applyCreativeEditorWorldLayoutRoomSettingsToDocument(
    CreativeEditorWorldLayoutState& state, cr::CreativeAppState& appState,
    std::size_t roomIndex, CreativeEditorWorldLayoutRoomSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutPreviewReceipt
previewCreativeEditorWorldLayoutRoomSettings(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document, std::size_t roomIndex,
    CreativeEditorWorldLayoutRoomSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutRoomTarget
findCreativeEditorWorldLayoutRoomTarget(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutRoomManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutRoomManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point = {},
    double toleranceCells = 0.25);
[[nodiscard]] bool creativeEditorWorldLayoutVerticalConnectorOnActiveLevel(
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeWorldLayout& source,
    std::size_t connectorIndex) noexcept;
[[nodiscard]] bool resolveCreativeEditorWorldLayoutVerticalConnectorAxis(
    cr::CreativeWorldLayoutRect footprint,
    cr::CreativeWorldLayoutVerticalDirection direction,
    CreativeEditorWorldLayoutPoint& low,
    CreativeEditorWorldLayoutPoint& high) noexcept;
[[nodiscard]] bool
resolveCreativeEditorWorldLayoutVerticalConnectorDirectionHandle(
    cr::CreativeWorldLayoutRect footprint,
    cr::CreativeWorldLayoutVerticalDirection direction,
    CreativeEditorWorldLayoutPoint& output) noexcept;
[[nodiscard]] bool readCreativeEditorWorldLayoutVerticalConnectorSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t connectorIndex,
    CreativeEditorWorldLayoutVerticalConnectorSettings& output) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutVerticalConnectorSettings(
    CreativeEditorWorldLayoutState& state, std::size_t connectorIndex,
    CreativeEditorWorldLayoutVerticalConnectorSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutApplyReceipt
applyCreativeEditorWorldLayoutVerticalConnectorSettingsToDocument(
    CreativeEditorWorldLayoutState& state, cr::CreativeAppState& appState,
    std::size_t connectorIndex,
    CreativeEditorWorldLayoutVerticalConnectorSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutPreviewReceipt
previewCreativeEditorWorldLayoutVerticalConnectorSettings(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document, std::size_t connectorIndex,
    CreativeEditorWorldLayoutVerticalConnectorSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutVerticalConnectorTarget
findCreativeEditorWorldLayoutVerticalConnectorTarget(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutVerticalConnectorManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutVerticalConnectorManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point = {},
    double toleranceCells = 0.25);
[[nodiscard]] bool readCreativeEditorWorldLayoutBoxSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t boxIndex,
    CreativeEditorWorldLayoutBoxSettings& output) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutBoxSettings(
    CreativeEditorWorldLayoutState& state, std::size_t boxIndex,
    CreativeEditorWorldLayoutBoxSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutBoxTarget
findCreativeEditorWorldLayoutBoxTarget(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutBoxManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutBoxManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point = {},
    double toleranceCells = 0.25);
[[nodiscard]] bool readCreativeEditorWorldLayoutWallSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t wallIndex,
    CreativeEditorWorldLayoutWallSettings& output) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutWallSettings(
    CreativeEditorWorldLayoutState& state, std::size_t wallIndex,
    CreativeEditorWorldLayoutWallSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutApplyReceipt
applyCreativeEditorWorldLayoutWallSettingsToDocument(
    CreativeEditorWorldLayoutState& state, cr::CreativeAppState& appState,
    std::size_t wallIndex, CreativeEditorWorldLayoutWallSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutPreviewReceipt
previewCreativeEditorWorldLayoutWallSettings(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document, std::size_t wallIndex,
    CreativeEditorWorldLayoutWallSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutWallTarget
findCreativeEditorWorldLayoutWallTarget(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutWallManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutWallManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point = {},
    double toleranceCells = 0.25);
[[nodiscard]] std::size_t creativeEditorWorldLayoutSelectedBuilding(
    const CreativeEditorWorldLayoutState& state) noexcept;
[[nodiscard]] bool readCreativeEditorWorldLayoutBuildingBounds(
    const CreativeEditorWorldLayoutState& state, std::size_t buildingIndex,
    CreativeEditorWorldLayoutBuildingBounds& output) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
selectCreativeEditorWorldLayoutBuilding(CreativeEditorWorldLayoutState& state,
                                        std::size_t buildingIndex);
void repairCreativeEditorWorldLayoutActiveLevel(
    CreativeEditorWorldLayoutState& state,
    std::size_t preferredBuildingIndex =
        cr::kInvalidCreativeWorldLayoutIndex) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutLevelOperation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutLevelOperation operation,
    std::size_t buildingIndex = cr::kInvalidCreativeWorldLayoutIndex,
    std::size_t levelIndex = cr::kInvalidCreativeWorldLayoutIndex);
[[nodiscard]] bool readCreativeEditorWorldLayoutLevelSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t levelIndex,
    CreativeEditorWorldLayoutLevelSettings& output);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutLevelSettings(
    CreativeEditorWorldLayoutState& state, std::size_t levelIndex,
    CreativeEditorWorldLayoutLevelSettings settings);
[[nodiscard]] bool readCreativeEditorWorldLayoutBuildingGroundingSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t buildingIndex,
    CreativeEditorWorldLayoutBuildingGroundingSettings& output) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutBuildingGroundingSettings(
    CreativeEditorWorldLayoutState& state, std::size_t buildingIndex,
    CreativeEditorWorldLayoutBuildingGroundingSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutApplyReceipt
applyCreativeEditorWorldLayoutBuildingGroundingSettingsToDocument(
    CreativeEditorWorldLayoutState& state, cr::CreativeAppState& appState,
    std::size_t buildingIndex,
    CreativeEditorWorldLayoutBuildingGroundingSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutApplyReceipt
applyCreativeEditorWorldLayoutLevelSettingsToDocument(
    CreativeEditorWorldLayoutState& state, cr::CreativeAppState& appState,
    std::size_t levelIndex, CreativeEditorWorldLayoutLevelSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutPreviewReceipt
previewCreativeEditorWorldLayoutLevelSettings(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document, std::size_t levelIndex,
    CreativeEditorWorldLayoutLevelSettings settings);
[[nodiscard]] bool readCreativeEditorWorldLayoutTerrainProfileSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t profileIndex,
    CreativeEditorWorldLayoutTerrainProfileSettings& output) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutTerrainProfileSettings(
    CreativeEditorWorldLayoutState& state, std::size_t profileIndex,
    CreativeEditorWorldLayoutTerrainProfileSettings settings);
[[nodiscard]] bool readCreativeEditorWorldLayoutTerrainPathSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t pathIndex,
    CreativeEditorWorldLayoutTerrainPathSettings& output);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutTerrainPathSettings(
    CreativeEditorWorldLayoutState& state, std::size_t pathIndex,
    CreativeEditorWorldLayoutTerrainPathSettings settings);
[[nodiscard]] bool readCreativeEditorWorldLayoutObjectSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t objectIndex,
    CreativeEditorWorldLayoutObjectSettings& output);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutObjectSettings(
    CreativeEditorWorldLayoutState& state, std::size_t objectIndex,
    CreativeEditorWorldLayoutObjectSettings settings);
[[nodiscard]] std::size_t findCreativeEditorWorldLayoutObjectAt(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point) noexcept;
[[nodiscard]] std::size_t findCreativeEditorWorldLayoutObjectAt(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point,
    cr::CreativeGridSettings grid) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
beginCreativeEditorWorldLayoutObjectManipulation(
    CreativeEditorWorldLayoutState& state, std::size_t objectIndex,
    CreativeEditorWorldLayoutPoint point);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
updateCreativeEditorWorldLayoutObjectManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
cancelCreativeEditorWorldLayoutObjectManipulation(
    CreativeEditorWorldLayoutState& state) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
clearCreativeEditorWorldLayoutSelection(
    CreativeEditorWorldLayoutState& state);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutBuildingManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutBuildingManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point = {},
    double toleranceCells = 0.25);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutBuildingTransform(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutBuildingTransformPhase phase,
    cr::CreativeWorldLayoutBuildingTransformOperation operation =
        cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90);
[[nodiscard]] CreativeEditorWorldLayoutPreviewReceipt
previewCreativeEditorWorldLayoutGeneratedBuildingOperation(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document, std::size_t buildingIndex,
    CreativeEditorWorldLayoutGeneratedBuildingOperation operation,
    std::int64_t deltaXCells = 0, std::int64_t deltaZCells = 0);
[[nodiscard]] CreativeEditorWorldLayoutApplyReceipt
applyCreativeEditorWorldLayoutGeneratedBuildingOperationToDocument(
    CreativeEditorWorldLayoutState& state, cr::CreativeAppState& appState,
    std::size_t buildingIndex,
    CreativeEditorWorldLayoutGeneratedBuildingOperation operation,
    std::int64_t deltaXCells = 0, std::int64_t deltaZCells = 0);
[[nodiscard]] CreativeEditorWorldLayoutPreviewReceipt
previewCreativeEditorWorldLayoutBuildingArchitecture(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document, std::size_t buildingIndex,
    cr::CreativeWorldLayoutArchitecturalProfile profile);
[[nodiscard]] CreativeEditorWorldLayoutApplyReceipt
applyCreativeEditorWorldLayoutBuildingArchitectureToDocument(
    CreativeEditorWorldLayoutState& state, cr::CreativeAppState& appState,
    std::size_t buildingIndex,
    cr::CreativeWorldLayoutArchitecturalProfile profile);
[[nodiscard]] bool defaultCreativeEditorWorldLayoutBuildingDuplicateOffset(
    const CreativeEditorWorldLayoutState& state, std::size_t buildingIndex,
    std::int64_t& deltaXCells, std::int64_t& deltaZCells) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
duplicateCreativeEditorWorldLayoutBuilding(
    CreativeEditorWorldLayoutState& state, std::size_t buildingIndex,
    std::int64_t deltaXCells, std::int64_t deltaZCells);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
deleteCreativeEditorWorldLayoutBuilding(CreativeEditorWorldLayoutState& state,
                                        std::size_t buildingIndex);
[[nodiscard]] CreativeEditorWorldLayoutBuildingTemplateLoadReceipt
loadCreativeEditorWorldLayoutBuildingTemplateLibrary(
    CreativeEditorWorldLayoutBuildingTemplateLibrary& library,
    const std::filesystem::path& creativeSaveRoot);
[[nodiscard]] CreativeEditorWorldLayoutBuildingTemplateInstallReceipt
installCreativeEditorWorldLayoutBuildingTemplate(
    CreativeEditorWorldLayoutBuildingTemplateLibrary& library,
    const cr::CreativeWorldLayoutBuildingTemplate& sourceTemplate);
// Built-in templates are application-owned source, not user-authored library
// entries. A newer built-in fingerprint replaces its cached copy atomically;
// the ordinary installer above continues to reject conflicting IDs.
[[nodiscard]] CreativeEditorWorldLayoutBuildingTemplateInstallReceipt
installCreativeEditorBuiltInWorldLayoutBuildingTemplate(
    CreativeEditorWorldLayoutBuildingTemplateLibrary& library,
    const cr::CreativeWorldLayoutBuildingTemplate& sourceTemplate);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
captureCreativeEditorWorldLayoutBuildingTemplate(
    CreativeEditorWorldLayoutState& state,
    std::size_t buildingIndex,
    std::string label = {});
[[nodiscard]] cr::CreativeWorldLayoutBuildingTemplateSyncReceipt
inspectCreativeEditorWorldLayoutBuildingTemplateSync(
    const CreativeEditorWorldLayoutState& state,
    std::size_t buildingIndex);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
updateCreativeEditorWorldLayoutBuildingTemplateFromInstance(
    CreativeEditorWorldLayoutState& state,
    std::size_t buildingIndex);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
refreshCreativeEditorWorldLayoutBuildingTemplateInstances(
    CreativeEditorWorldLayoutState& state,
    std::size_t buildingIndex,
    cr::CreativeWorldLayoutBuildingTemplateRefreshMode mode);
[[nodiscard]] CreativeEditorWorldLayoutApplyReceipt
applyCreativeEditorWorldLayoutBuildingTemplateRefreshToDocument(
    CreativeEditorWorldLayoutState& state, cr::CreativeAppState& appState,
    std::size_t buildingIndex,
    cr::CreativeWorldLayoutBuildingTemplateRefreshMode mode);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
selectCreativeEditorWorldLayoutBuildingTemplate(
    CreativeEditorWorldLayoutState& state,
    std::size_t templateIndex);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutBuildingTemplatePlacement(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutBuildingTemplatePlacementPhase phase,
    CreativeEditorWorldLayoutPoint point = {},
    cr::CreativeWorldLayoutBuildingTransformOperation operation =
        cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90);
[[nodiscard]] bool readCreativeEditorWorldLayoutOpeningSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t openingIndex,
    CreativeEditorWorldLayoutOpeningSettings& output) noexcept;
// Hover-time query: O(walls + room edges + openings), stable first-host ties,
// and no layout copies or container allocations.
[[nodiscard]] CreativeEditorWorldLayoutOpeningPlacementPlan
planCreativeEditorWorldLayoutOpeningPlacement(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point,
    cr::CreativeBuildingOpeningKind kind);
[[nodiscard]] CreativeEditorWorldLayoutOpeningPlacementPlan
planCreativeEditorWorldLayoutOpeningPlacement(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutOpeningPlacementRequest& request);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutOpeningPlacement(
    CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutOpeningPlacementRequest& request);
[[nodiscard]] CreativeEditorWorldLayoutOpeningHost
resolveCreativeEditorWorldLayoutOpeningHost(
    const CreativeEditorWorldLayoutState& state,
    std::size_t openingIndex) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutOpeningTarget
findCreativeEditorWorldLayoutOpeningTarget(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutOpeningSettings(
    CreativeEditorWorldLayoutState& state, std::size_t openingIndex,
    CreativeEditorWorldLayoutOpeningSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutApplyReceipt
applyCreativeEditorWorldLayoutOpeningSettingsToDocument(
    CreativeEditorWorldLayoutState& state, cr::CreativeAppState& appState,
    std::size_t openingIndex,
    CreativeEditorWorldLayoutOpeningSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutPreviewReceipt
previewCreativeEditorWorldLayoutOpeningSettings(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document, std::size_t openingIndex,
    CreativeEditorWorldLayoutOpeningSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutOpeningInsertPlan
planCreativeEditorWorldLayoutOpeningInsert(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutOpeningInsertRequest& request);
[[nodiscard]] CreativeEditorWorldLayoutOpeningAssetGeometryPlan
planCreativeEditorWorldLayoutOpeningAssetGeometry(
    const CreativeEditorWorldLayoutOpeningAssetGeometryRequest& request)
    noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutOpeningInsert(
    CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutOpeningInsertRequest& request);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutAssetBoundsUpdates(
    CreativeEditorWorldLayoutState& state,
    std::span<const CreativeEditorWorldLayoutAssetBoundsUpdate> updates);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutOpeningManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutOpeningManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point = {},
    double toleranceCells = 0.25);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
deleteCreativeEditorWorldLayoutSelection(CreativeEditorWorldLayoutState& state);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
cancelCreativeEditorWorldLayoutPreview(
    CreativeEditorWorldLayoutState& state) noexcept;

[[nodiscard]] CreativeEditorWorldLayoutPreviewReceipt
previewCreativeEditorWorldLayout(CreativeEditorWorldLayoutState& state,
                                 const cr::CreativeDocument& document);
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
