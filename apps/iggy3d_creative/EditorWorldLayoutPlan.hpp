#pragma once

#include <cstddef>
#include <cstdint>

#include "EditorWorldLayoutContracts.hpp"

namespace iggy3d_creative_app {

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
    CreativeEditorWorldLayoutPoint point = {},
    cr::CreativeGridSettings grid = {});

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
[[nodiscard]] bool readCreativeEditorWorldLayoutRoomMetadata(
    const CreativeEditorWorldLayoutState& state, std::size_t roomIndex,
    CreativeEditorWorldLayoutRoomMetadata& output) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutRoomMetadata(
    CreativeEditorWorldLayoutState& state, std::size_t roomIndex,
    CreativeEditorWorldLayoutRoomMetadata metadata);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
splitCreativeEditorWorldLayoutRoom(
    CreativeEditorWorldLayoutState& state, std::size_t roomIndex,
    cr::CreativeWorldLayoutRoomSplitAxis axis, std::int32_t coordinate);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
mergeCreativeEditorWorldLayoutRooms(
    CreativeEditorWorldLayoutState& state, std::size_t primaryRoomIndex,
    std::size_t secondaryRoomIndex);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
moveCreativeEditorWorldLayoutRoomBoundary(
    CreativeEditorWorldLayoutState& state, std::size_t topologyEdgeIndex,
    std::int32_t coordinate);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
moveCreativeEditorWorldLayoutRoomCorner(
    CreativeEditorWorldLayoutState& state, std::size_t roomIndex,
    std::size_t topologyVertexIndex, cr::CreativeTerrainCoord2 position);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutRoomEdgeSettings(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutRoomEdgeSettingsRequest request);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
splitCreativeEditorWorldLayoutWall(
    CreativeEditorWorldLayoutState& state, std::size_t topologyEdgeIndex,
    std::uint32_t offsetCells);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
mergeCreativeEditorWorldLayoutWalls(
    CreativeEditorWorldLayoutState& state,
    std::size_t primaryTopologyEdgeIndex,
    std::size_t secondaryTopologyEdgeIndex);
[[nodiscard]] CreativeEditorWorldLayoutRoomBoundaryTarget
findCreativeEditorWorldLayoutRoomBoundaryTarget(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point, double toleranceCells);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutRoomBoundaryManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutRoomBoundaryManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point = {},
    double toleranceCells = 0.25);
[[nodiscard]] CreativeEditorWorldLayoutRoomCornerTarget
findCreativeEditorWorldLayoutRoomCornerTarget(
    const CreativeEditorWorldLayoutState& state, std::size_t roomIndex,
    CreativeEditorWorldLayoutPoint point, double toleranceCells);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutRoomCornerManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutRoomCornerManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point = {},
    double toleranceCells = 0.25);
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
    CreativeEditorWorldLayoutVerticalConnectorSettings settings,
    cr::CreativeGridSettings grid = {});
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
    double toleranceCells = 0.25,
    cr::CreativeGridSettings grid = {},
    CreativeEditorWorldLayoutVerticalConnectorTarget target = {});

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

}  // namespace iggy3d_creative_app
