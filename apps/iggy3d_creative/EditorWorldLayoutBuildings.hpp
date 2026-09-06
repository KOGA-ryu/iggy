#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>

#include "EditorWorldLayoutContracts.hpp"

namespace iggy3d_creative_app {

[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
createCreativeEditorWorldLayoutBuildingShell(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutRoomSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
createCreativeEditorWorldLayoutBuildingBlockout(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutBuildingBlockoutSettings settings);
[[nodiscard]] bool
applyCreativeEditorWorldLayoutBlockoutArchitecturalProfile(
    CreativeEditorWorldLayoutBuildingBlockoutSettings& settings,
    cr::CreativeGridSettings grid,
    cr::CreativeWorldLayoutArchitecturalProfileKind kind) noexcept;
[[nodiscard]] bool readCreativeEditorWorldLayoutBuildingBlockoutSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t buildingIndex,
    CreativeEditorWorldLayoutBuildingBlockoutSettings& output) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
updateCreativeEditorWorldLayoutBuildingBlockout(
    CreativeEditorWorldLayoutState& state, std::size_t buildingIndex,
    CreativeEditorWorldLayoutBuildingBlockoutSettings settings);

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
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutLevelDatum(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutLevelDatumEditRequest request);

[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
createCreativeEditorWorldLayoutRoofAperture(
    CreativeEditorWorldLayoutState& state, std::size_t levelIndex,
    cr::CreativeStructuralRoofApertureKind kind);
[[nodiscard]] bool readCreativeEditorWorldLayoutRoofApertureSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t apertureIndex,
    CreativeEditorWorldLayoutRoofApertureSettings& output) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutRoofApertureSettings(
    CreativeEditorWorldLayoutState& state, std::size_t apertureIndex,
    CreativeEditorWorldLayoutRoofApertureSettings settings,
    cr::CreativeGridSettings grid = {});
[[nodiscard]] CreativeEditorWorldLayoutRoofApertureTarget
findCreativeEditorWorldLayoutRoofApertureTarget(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutRoofApertureManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutRoofApertureManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point = {},
    double toleranceCells = 0.25,
    cr::CreativeGridSettings grid = {});

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
detachCreativeEditorWorldLayoutBuildingTemplateInstance(
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
        cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90,
    const cr::CreativeDocument* document = nullptr);

[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutBuildingRepair(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeGridSettings& grid,
    const cr::CreativeWorldLayoutBuildingUsabilityIssue& issue);

}  // namespace iggy3d_creative_app
