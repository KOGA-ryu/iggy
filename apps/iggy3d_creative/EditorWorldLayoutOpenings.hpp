#pragma once

#include <cstddef>
#include <span>

#include "EditorWorldLayoutContracts.hpp"

namespace iggy3d_creative_app {

[[nodiscard]] bool readCreativeEditorWorldLayoutOpeningSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t openingIndex,
    CreativeEditorWorldLayoutOpeningSettings& output) noexcept;
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

}  // namespace iggy3d_creative_app
