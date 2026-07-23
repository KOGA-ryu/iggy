#include "EditorRoomPlacement.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>

#include "EditorInteraction.hpp"
#include "EditorPlacementFeedback.hpp"
#include "EditorPreviewProxies.hpp"
#include "EditorState.hpp"
#include "EditorWorldLayout.hpp"
#include "EditorWorldLayoutHistory.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRoomTopology.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

struct RoomSymbolDraft {
  cr::CreativeWorldLayoutRoom room;
  cr::CreativeWorldLayoutLevel level;
};

[[nodiscard]] bool validCellBounds(cr::CreativeBounds bounds) noexcept {
  const cr::CreativeBoundsMetrics metrics = cr::measureCreativeBounds(bounds);
  return metrics.valid && cr::isPositiveCreativeVec3(metrics.size);
}

[[nodiscard]] bool currentRoomTargetBounds(
    const cr::CreativeAppState& appState,
    const CreativeEditorState& editor,
    cr::CreativeBounds& bounds) noexcept {
  const cr::CreativeGridTarget& target = editor.interaction.target.grid;
  if (!target.valid) {
    return false;
  }
  bounds = target.adjacentCellBounds;
  if (!validCellBounds(bounds)) {
    const cr::CreativeGridSettings grid =
        appState.facade.document().gridSettings();
    bounds = cr::creativeVolumeCellBounds(target.adjacentCell,
                                          grid.cellSizeMeters, grid.origin);
  }
  return validCellBounds(bounds);
}

[[nodiscard]] bool exactLayerCount(double meters,
                                   double layerMeters,
                                   std::uint16_t& output) noexcept {
  if (!std::isfinite(meters) || !std::isfinite(layerMeters) || meters <= 0.0 ||
      layerMeters <= 0.0) {
    return false;
  }
  const double layers = meters / layerMeters;
  const double rounded = std::round(layers);
  if (std::abs(layers - rounded) > 1.0e-9 || rounded < 1.0 ||
      rounded > std::numeric_limits<std::uint16_t>::max()) {
    return false;
  }
  output = static_cast<std::uint16_t>(rounded);
  return true;
}

[[nodiscard]] bool incrementedCoordinate(std::int32_t value,
                                         std::int32_t& output) noexcept {
  if (value == std::numeric_limits<std::int32_t>::max()) {
    return false;
  }
  output = value + 1;
  return true;
}

[[nodiscard]] bool roomSymbol(
    const CreativeEditorRoomPlacementState& state,
    cr::CreativeGridCoord3 currentCell,
    cr::CreativeBounds currentCellBounds,
    const cr::CreativeGridSettings& grid,
    const cr::CreativeToolSettings& settings,
    RoomSymbolDraft& output) noexcept {
  if (!std::isfinite(grid.cellSizeMeters) || grid.cellSizeMeters <= 0.0 ||
      !std::isfinite(state.floorTopY) ||
      std::abs(currentCellBounds.min.y - state.floorTopY) > 1.0e-9) {
    return false;
  }
  const std::int32_t minimumX = std::min(state.firstCell.x, currentCell.x);
  const std::int32_t minimumZ = std::min(state.firstCell.z, currentCell.z);
  std::int32_t maximumX = 0;
  std::int32_t maximumZ = 0;
  if (!incrementedCoordinate(std::max(state.firstCell.x, currentCell.x),
                             maximumX) ||
      !incrementedCoordinate(std::max(state.firstCell.z, currentCell.z),
                             maximumZ)) {
    return false;
  }

  const double wallHeightMeters =
      cr::creativeRoomWallHeightMeters(settings.roomWallHeight);
  const double floorThicknessMeters =
      cr::creativeRoomFloorThicknessMeters(settings.roomFloorThickness);
  const double floorLayerMeters =
      cr::defaultCreativeStructuralLayerThicknessMeters(
          cr::CreativeObjectKind::Floor);
  if (!exactLayerCount(wallHeightMeters, grid.cellSizeMeters,
                       output.level.wallHeightCells) ||
      !exactLayerCount(floorThicknessMeters, floorLayerMeters,
                       output.level.floorThicknessLayers)) {
    return false;
  }
  output.room.footprint = {{minimumX, minimumZ}, {maximumX, maximumZ}};
  output.level.floorTopLayer =
      (state.floorTopY - grid.origin.y) / grid.cellSizeMeters;
  output.room.wallThicknessCells =
      cr::creativeRoomWallThicknessMeters(settings.roomWallThickness) /
      grid.cellSizeMeters;
  return std::isfinite(output.level.floorTopLayer) &&
         std::isfinite(output.room.wallThicknessCells) &&
         output.room.wallThicknessCells > 0.0;
}

[[nodiscard]] cr::CreativeRectangularRoomGeometryPlan roomGeometryPlan(
    const cr::CreativeGridSettings& grid,
    RoomSymbolDraft draft) noexcept {
  cr::CreativeWorldLayout layout;
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "preview_building";
  building.name = "Preview Building";
  layout.buildings.push_back(std::move(building));
  draft.level.buildingIndex = 0U;
  draft.level.stableKey = "preview_level";
  draft.level.name = "Preview Level";
  layout.levels.push_back(std::move(draft.level));
  draft.room.buildingIndex = 0U;
  draft.room.levelIndex = 0U;
  draft.room.stableKey = "preview_room";
  draft.room.name = "Preview Room";
  layout.rooms.push_back(std::move(draft.room));
  return cr::planCreativeWorldLayoutRoomGeometry(grid, layout, 0U);
}

void rejectRoom(CreativeEditorState& editor) noexcept {
  setCreativeEditorPlacementFeedback(
      editor.interaction, CreativeEditorPlacementFeedbackStatus::Rejected,
      editor.frameIndex, cr::CreativeObjectKind::Room);
}

void appendRoomBounds(
    cr::CreativeBounds bounds,
    iggy3d::RenderLineColor color,
    float thickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines) {
  const cr::CreativeCoreVec3Conversion minimum =
      cr::creativeVec3ToCoreChecked(bounds.min);
  const cr::CreativeCoreVec3Conversion maximum =
      cr::creativeVec3ToCoreChecked(bounds.max);
  if (!minimum.converted || !maximum.converted) {
    return;
  }
  appendStandaloneWireframeBoxEdges(lines, minimum.value, maximum.value,
                                    color, thickness);
}

}  // namespace

cr::CreativeRectangularRoomGeometryPlan creativeEditorRoomPlacementPreview(
    const cr::CreativeAppState& appState,
    const CreativeEditorState& editor) noexcept {
  const CreativeEditorRoomPlacementState& state =
      editor.interaction.roomPlacement;
  if (!state.active || state.documentId != appState.facade.document().id()) {
    return {};
  }
  cr::CreativeBounds currentCell;
  if (!currentRoomTargetBounds(appState, editor, currentCell)) {
    return {};
  }
  RoomSymbolDraft draft;
  if (!roomSymbol(state, editor.interaction.target.grid.adjacentCell,
                  currentCell, appState.facade.document().gridSettings(),
                  editor.toolSettings, draft)) {
    return {};
  }
  return roomGeometryPlan(appState.facade.document().gridSettings(),
                          std::move(draft));
}

CreativeEditorRoomPlacementReceipt advanceCreativeEditorRoomPlacement(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source) {
  CreativeEditorRoomPlacementReceipt receipt;
  receipt.requested = true;
  if (editor.assetEdit.active) {
    receipt.status = CreativeEditorRoomPlacementStatus::InvalidTarget;
    receipt.reasonCode = "creative_editor_room_asset_workspace_unsupported";
    rejectRoom(editor);
    return receipt;
  }
  const cr::CreativeDocument& document = appState.facade.document();
  cr::CreativeBounds currentCell;
  if (!document.isValid() || document.id() == cr::kInvalidDocumentId ||
      !currentRoomTargetBounds(appState, editor, currentCell)) {
    receipt.status = CreativeEditorRoomPlacementStatus::InvalidTarget;
    receipt.reasonCode = "creative_editor_room_target_invalid";
    rejectRoom(editor);
    return receipt;
  }

  CreativeEditorRoomPlacementState& state =
      editor.interaction.roomPlacement;
  if (!state.active || state.documentId != document.id()) {
    state = {};
    state.documentId = document.id();
    state.firstCellBounds = currentCell;
    state.firstCell = editor.interaction.target.grid.adjacentCell;
    state.floorTopY = currentCell.min.y;
    state.active = true;
    receipt.accepted = true;
    receipt.status = CreativeEditorRoomPlacementStatus::FirstCornerSet;
    receipt.reasonCode = "creative_editor_room_first_corner_set";
    clearCreativeEditorPlacementFeedback(editor.interaction);
    return receipt;
  }

  RoomSymbolDraft draft;
  if (!roomSymbol(state, editor.interaction.target.grid.adjacentCell,
                  currentCell, document.gridSettings(), editor.toolSettings,
                  draft)) {
    receipt.status = CreativeEditorRoomPlacementStatus::InvalidGeometry;
    receipt.reasonCode = "creative_editor_room_layout_conversion_invalid";
    rejectRoom(editor);
    return receipt;
  }
  const cr::CreativeRectangularRoomGeometryPlan geometryPlan =
      roomGeometryPlan(document.gridSettings(), draft);
  if (!geometryPlan.accepted) {
    receipt.status = CreativeEditorRoomPlacementStatus::InvalidGeometry;
    receipt.reasonCode = geometryPlan.reasonCode;
    rejectRoom(editor);
    return receipt;
  }

  CreativeEditorWorldLayoutSnapshot committed =
      captureCreativeEditorWorldLayoutSnapshot(editor.worldLayout);
  if (committed.revision == std::numeric_limits<std::uint64_t>::max()) {
    receipt.status = CreativeEditorRoomPlacementStatus::InvalidLayout;
    receipt.reasonCode = "creative_editor_room_layout_revision_exhausted";
    rejectRoom(editor);
    return receipt;
  }
  cr::CreativeWorldLayout& candidate = committed.source;
  std::uint64_t& nextOrdinal = committed.nextStableOrdinal;
  if (candidate.buildings.empty()) {
    cr::CreativeWorldLayoutBuilding building;
    building.stableKey = cr::mintCreativeWorldLayoutStableKey(
        candidate, nextOrdinal, "building");
    building.name = "Building 1";
    building.rootMode = cr::CreativeBuildingRootMode::None;
    candidate.buildings.push_back(std::move(building));
  }
  std::size_t levelIndex = cr::kInvalidCreativeWorldLayoutIndex;
  for (std::size_t index = 0U; index < candidate.levels.size(); ++index) {
    const cr::CreativeWorldLayoutLevel& level = candidate.levels[index];
    if (level.buildingIndex == 0U &&
        std::fabs(level.floorTopLayer - draft.level.floorTopLayer) <= 1.0e-9 &&
        level.wallHeightCells == draft.level.wallHeightCells &&
        level.floorThicknessLayers == draft.level.floorThicknessLayers) {
      levelIndex = index;
      break;
    }
  }
  if (levelIndex == cr::kInvalidCreativeWorldLayoutIndex) {
    draft.level.buildingIndex = 0U;
    draft.level.stableKey = cr::mintCreativeWorldLayoutStableKey(
        candidate, nextOrdinal, "level");
    draft.level.name = "Level " + std::to_string(candidate.levels.size());
    levelIndex = candidate.levels.size();
    candidate.levels.push_back(std::move(draft.level));
  }
  cr::CreativeWorldLayoutRoom& room = draft.room;
  room.buildingIndex = 0U;
  room.levelIndex = levelIndex;
  room.stableKey =
      cr::mintCreativeWorldLayoutStableKey(candidate, nextOrdinal, "room");
  room.name = "Room " + std::to_string(candidate.rooms.size() + 1U);
  candidate.rooms.push_back(std::move(room));
  if (!cr::refreshCreativeWorldLayoutBuildingRoomFootprint(candidate, 0U)) {
    receipt.status = CreativeEditorRoomPlacementStatus::InvalidLayout;
    receipt.reasonCode = "creative_editor_room_building_footprint_invalid";
    rejectRoom(editor);
    return receipt;
  }
  ++committed.revision;

  const cr::CreativeWorldLayoutTerrainReconciliationResult terrain =
      reconcileCreativeEditorWorldLayoutTerrain(editor.worldLayout, document,
                                                candidate);
  if (!terrain.accepted) {
    receipt.status = CreativeEditorRoomPlacementStatus::InvalidLayout;
    receipt.reasonCode = terrain.reasonCode;
    rejectRoom(editor);
    return receipt;
  }

  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, candidate);
  receipt.layoutStatus = compiled.receipt.status;
  receipt.generatedObjectCount = compiled.receipt.objectCount;
  if (!compiled.receipt.accepted) {
    receipt.status = CreativeEditorRoomPlacementStatus::InvalidLayout;
    receipt.reasonCode = compiled.receipt.reasonCode;
    rejectRoom(editor);
    return receipt;
  }

  const cr::CreativeObjectId firstGeneratedObjectId = document.nextObjectId();
  const cr::CreativeWorldLayoutApplyReceipt applied =
      applyCreativeEditorWorldLayoutPlanWithHistory(
          editor.worldLayout, appState, compiled.plan, std::move(committed),
          source);
  receipt.applyStatus = applied.status;
  if (!applied.accepted || !applied.changed) {
    receipt.status = CreativeEditorRoomPlacementStatus::ApplyRejected;
    receipt.reasonCode = applied.reasonCode;
    rejectRoom(editor);
    return receipt;
  }

  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = CreativeEditorRoomPlacementStatus::Applied;
  receipt.reasonCode = "creative_editor_room_applied";
  setCreativeEditorPlacementFeedback(
      editor.interaction, CreativeEditorPlacementFeedbackStatus::Placed,
      editor.frameIndex, cr::CreativeObjectKind::Floor,
      firstGeneratedObjectId);
  state = {};
  return receipt;
}

bool cancelCreativeEditorRoomPlacement(
    CreativeEditorState& editor) noexcept {
  if (!editor.interaction.roomPlacement.active) {
    return false;
  }
  editor.interaction.roomPlacement = {};
  clearCreativeEditorPlacementFeedback(editor.interaction);
  return true;
}

std::size_t appendCreativeEditorRoomPlacementWireframe(
    const cr::CreativeAppState& appState,
    const CreativeEditorState& editor,
    float thickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines) {
  const CreativeEditorRoomPlacementState& state =
      editor.interaction.roomPlacement;
  if (!state.active || state.documentId != appState.facade.document().id()) {
    return 0U;
  }
  const std::size_t begin = wireLines.size();
  const cr::CreativeRectangularRoomGeometryPlan preview =
      creativeEditorRoomPlacementPreview(appState, editor);
  if (!preview.accepted) {
    const iggy3d::RenderLineColor color = editor.interaction.target.grid.valid
                                              ? iggy3d::RenderLineColor{
                                                    1.0F, 0.20F, 0.14F, 1.0F}
                                              : iggy3d::RenderLineColor{
                                                    1.0F, 0.78F, 0.18F, 1.0F};
    appendRoomBounds(state.firstCellBounds, color, thickness, wireLines);
    return wireLines.size() - begin;
  }

  constexpr iggy3d::RenderLineColor kReadyColor{
      0.20F, 1.0F, 0.36F, 1.0F};
  appendRoomBounds(preview.floorBounds, kReadyColor, thickness, wireLines);
  for (cr::CreativeBounds wall : preview.wallBounds) {
    appendRoomBounds(wall, kReadyColor, thickness, wireLines);
  }
  return wireLines.size() - begin;
}

}  // namespace iggy3d_creative_app
