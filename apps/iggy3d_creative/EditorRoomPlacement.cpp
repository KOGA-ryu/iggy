#include "EditorRoomPlacement.hpp"

#include <algorithm>
#include <array>
#include <string>

#include "EditorInteraction.hpp"
#include "EditorPlacementFeedback.hpp"
#include "EditorPreviewProxies.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

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

[[nodiscard]] cr::CreativeRectangularRoomGeometryRequest geometryRequest(
    const CreativeEditorRoomPlacementState& state,
    cr::CreativeBounds currentCell,
    const cr::CreativeToolSettings& settings) noexcept {
  cr::CreativeRectangularRoomGeometryRequest request;
  request.firstFloorCorner = {
      std::min(state.firstCellBounds.min.x, currentCell.min.x),
      state.floorTopY,
      std::min(state.firstCellBounds.min.z, currentCell.min.z)};
  request.oppositeFloorCorner = {
      std::max(state.firstCellBounds.max.x, currentCell.max.x),
      currentCell.min.y,
      std::max(state.firstCellBounds.max.z, currentCell.max.z)};
  request.wallHeightMeters =
      cr::creativeRoomWallHeightMeters(settings.roomWallHeight);
  request.wallThicknessMeters =
      cr::creativeRoomWallThicknessMeters(settings.roomWallThickness);
  request.floorThicknessMeters =
      cr::creativeRoomFloorThicknessMeters(settings.roomFloorThickness);
  return request;
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
  return cr::planCreativeRectangularRoomGeometry(
      geometryRequest(state, currentCell, editor.toolSettings));
}

CreativeEditorRoomPlacementReceipt advanceCreativeEditorRoomPlacement(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source) {
  CreativeEditorRoomPlacementReceipt receipt;
  receipt.requested = true;
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
    state.floorTopY = currentCell.min.y;
    state.active = true;
    receipt.accepted = true;
    receipt.status = CreativeEditorRoomPlacementStatus::FirstCornerSet;
    receipt.reasonCode = "creative_editor_room_first_corner_set";
    clearCreativeEditorPlacementFeedback(editor.interaction);
    return receipt;
  }

  const cr::CreativeRectangularRoomGeometryRequest geometry =
      geometryRequest(state, currentCell, editor.toolSettings);
  const cr::CreativeRectangularRoomGeometryPlan geometryPlan =
      cr::planCreativeRectangularRoomGeometry(geometry);
  if (!geometryPlan.accepted) {
    receipt.status = CreativeEditorRoomPlacementStatus::InvalidGeometry;
    receipt.reasonCode = geometryPlan.reasonCode;
    rejectRoom(editor);
    return receipt;
  }

  cr::CreativeRectangularRoomRecipeRequest request;
  request.stableKey =
      "viewport-room-" + std::to_string(document.nextObjectId());
  request.name = "Room " + std::to_string(document.nextObjectId());
  request.geometry = geometry;
  const cr::CreativeBuildingRecipeResult recipe =
      cr::buildCreativeRectangularRoomRecipe(request);
  receipt.recipeStatus = recipe.receipt.status;
  receipt.generatedObjectCount = recipe.receipt.generatedObjectCount;
  if (!recipe.receipt.accepted) {
    receipt.status = CreativeEditorRoomPlacementStatus::InvalidRecipe;
    receipt.reasonCode = recipe.receipt.reasonCode;
    rejectRoom(editor);
    return receipt;
  }

  const cr::CreativeRecipeApplyReceipt applied =
      cr::applyCreativeRecipeWithHistory(appState, recipe.plan, source);
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
      applied.materializeReceipt.firstObjectId);
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
