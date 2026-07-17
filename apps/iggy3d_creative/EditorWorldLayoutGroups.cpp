#include "EditorWorldLayout.hpp"

#include "EditorWorldLayoutInternal.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace iggy3d_creative_app {
namespace {

bool containsPoint(CreativeEditorWorldLayoutBuildingBounds bounds,
                   CreativeEditorWorldLayoutPoint point,
                   double toleranceCells) noexcept {
  return bounds.valid && detail::finiteWorldLayoutPoint(point) &&
         std::isfinite(toleranceCells) && toleranceCells > 0.0 &&
         point.x >= bounds.minimum.x - toleranceCells &&
         point.x <= bounds.maximum.x + toleranceCells &&
         point.z >= bounds.minimum.z - toleranceCells &&
         point.z <= bounds.maximum.z + toleranceCells;
}

}  // namespace

std::size_t creativeEditorWorldLayoutSelectedBuilding(
    const CreativeEditorWorldLayoutState& state) noexcept {
  const CreativeEditorWorldLayoutSelection selection = state.selection;
  if (selection.kind == CreativeEditorWorldLayoutSelectionKind::Building) {
    return selection.index < state.source.buildings.size()
               ? selection.index
               : cr::kInvalidCreativeWorldLayoutIndex;
  }
  if (selection.kind == CreativeEditorWorldLayoutSelectionKind::Room) {
    return selection.index < state.source.rooms.size()
               ? state.source.rooms[selection.index].buildingIndex
               : cr::kInvalidCreativeWorldLayoutIndex;
  }
  if (selection.kind == CreativeEditorWorldLayoutSelectionKind::Box) {
    return selection.index < state.source.boxes.size()
               ? state.source.boxes[selection.index].buildingIndex
               : cr::kInvalidCreativeWorldLayoutIndex;
  }
  if (selection.kind == CreativeEditorWorldLayoutSelectionKind::Wall) {
    return selection.index < state.source.walls.size()
               ? state.source.walls[selection.index].buildingIndex
               : cr::kInvalidCreativeWorldLayoutIndex;
  }
  if (selection.kind != CreativeEditorWorldLayoutSelectionKind::Opening ||
      selection.index >= state.source.openings.size()) {
    return cr::kInvalidCreativeWorldLayoutIndex;
  }
  const cr::CreativeWorldLayoutOpening& opening =
      state.source.openings[selection.index];
  if (opening.hostKind == cr::CreativeWorldLayoutOpeningHostKind::Wall &&
      opening.wallIndex < state.source.walls.size()) {
    return state.source.walls[opening.wallIndex].buildingIndex;
  }
  if (opening.hostKind == cr::CreativeWorldLayoutOpeningHostKind::RoomEdge &&
      opening.roomIndex < state.source.rooms.size()) {
    return state.source.rooms[opening.roomIndex].buildingIndex;
  }
  return cr::kInvalidCreativeWorldLayoutIndex;
}

bool readCreativeEditorWorldLayoutBuildingBounds(
    const CreativeEditorWorldLayoutState& state, std::size_t buildingIndex,
    CreativeEditorWorldLayoutBuildingBounds& output) noexcept {
  return cr::measureCreativeWorldLayoutBuildingBounds(
      creativeEditorWorldLayoutDisplaySource(state), buildingIndex, output);
}

CreativeEditorWorldLayoutEditReceipt selectCreativeEditorWorldLayoutBuilding(
    CreativeEditorWorldLayoutState& state, std::size_t buildingIndex) {
  CreativeEditorWorldLayoutBuildingBounds bounds;
  if (buildingIndex >= state.source.buildings.size() ||
      !readCreativeEditorWorldLayoutBuildingBounds(state, buildingIndex,
                                                   bounds)) {
    state.statusMessage = "building group is empty or unavailable";
    return {false, false,
            "creative_editor_world_layout_building_selection_invalid"};
  }
  const bool changed =
      state.selection.kind != CreativeEditorWorldLayoutSelectionKind::Building ||
      state.selection.index != buildingIndex ||
      state.buildingTemplatePlacement.active;
  detail::clearWorldLayoutInteraction(state);
  state.anchorActive = false;
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Building,
                     buildingIndex};
  repairCreativeEditorWorldLayoutActiveLevel(state, buildingIndex);
  state.statusMessage = "building selected";
  return {true, changed, "creative_editor_world_layout_building_selected"};
}

CreativeEditorWorldLayoutEditReceipt clearCreativeEditorWorldLayoutSelection(
    CreativeEditorWorldLayoutState& state) {
  const bool changed =
      state.selection.kind != CreativeEditorWorldLayoutSelectionKind::None ||
      state.anchorActive || state.buildingManipulation.active ||
      state.buildingTransform.active ||
      state.buildingTemplatePlacement.active;
  detail::clearWorldLayoutInteraction(state);
  state.anchorActive = false;
  state.selection = {};
  state.statusMessage = "selection cleared";
  return {true, changed, "creative_editor_world_layout_selection_cleared"};
}

CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutBuildingManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutBuildingManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) {
  if (phase >= CreativeEditorWorldLayoutBuildingManipulationPhase::Count) {
    return {
        false, false,
        "creative_editor_world_layout_building_manipulation_phase_invalid"};
  }
  if (phase == CreativeEditorWorldLayoutBuildingManipulationPhase::Cancel) {
    const bool changed = state.buildingManipulation.active;
    state.buildingManipulation = {};
    state.statusMessage = "building move cancelled";
    return {true, changed,
            "creative_editor_world_layout_building_manipulation_cancelled"};
  }
  if (state.tool != CreativeEditorWorldLayoutTool::Select) {
    return {false, false,
            "creative_editor_world_layout_building_manipulation_tool_invalid"};
  }
  if (state.buildingTransform.active) {
    return {false, false,
            "creative_editor_world_layout_building_transform_active"};
  }
  if (phase == CreativeEditorWorldLayoutBuildingManipulationPhase::Begin) {
    const std::size_t buildingIndex =
        creativeEditorWorldLayoutSelectedBuilding(state);
    CreativeEditorWorldLayoutBuildingBounds bounds;
    if (state.selection.kind !=
            CreativeEditorWorldLayoutSelectionKind::Building ||
        !readCreativeEditorWorldLayoutBuildingBounds(state, buildingIndex,
                                                     bounds) ||
        !containsPoint(bounds, point, toleranceCells)) {
      return {false, false,
              "creative_editor_world_layout_building_manipulation_target_missing"};
    }
    detail::clearWorldLayoutInteraction(state);
    state.selection = {CreativeEditorWorldLayoutSelectionKind::Building,
                       buildingIndex};
    state.anchorActive = false;
    state.buildingManipulation = {
        true,
        state.revision,
        buildingIndex,
        point,
        bounds,
        0,
        0,
        true,
        "creative_editor_world_layout_building_manipulation_ready",
    };
    state.statusMessage = "drag building";
    return {true, true,
            "creative_editor_world_layout_building_manipulation_started"};
  }
  if (!state.buildingManipulation.active ||
      state.buildingManipulation.buildingIndex >=
          state.source.buildings.size()) {
    return {false, false,
            "creative_editor_world_layout_building_manipulation_not_active"};
  }
  if (state.revision != state.buildingManipulation.sourceRevision) {
    state.buildingManipulation = {};
    state.statusMessage = "building changed while drag was active";
    return {false, false,
            "creative_editor_world_layout_building_manipulation_stale"};
  }
  if (phase == CreativeEditorWorldLayoutBuildingManipulationPhase::Update) {
    std::int64_t deltaXCells = 0;
    std::int64_t deltaZCells = 0;
    const bool deltaValid = detail::snappedWorldLayoutPointerDelta(
                                point.x,
                                state.buildingManipulation.startPoint.x,
                                deltaXCells) &&
                            detail::snappedWorldLayoutPointerDelta(
                                point.z,
                                state.buildingManipulation.startPoint.z,
                                deltaZCells);
    if (deltaValid &&
        deltaXCells == state.buildingManipulation.previewDeltaXCells &&
        deltaZCells == state.buildingManipulation.previewDeltaZCells) {
      return {true, false, state.buildingManipulation.reasonCode};
    }
    const bool previewValid =
        deltaValid &&
        cr::canMoveCreativeWorldLayoutBuilding(
            state.source,
            {state.buildingManipulation.buildingIndex, deltaXCells,
             deltaZCells});
    state.buildingManipulation.previewDeltaXCells = deltaXCells;
    state.buildingManipulation.previewDeltaZCells = deltaZCells;
    state.buildingManipulation.previewValid = previewValid;
    state.buildingManipulation.reasonCode =
        previewValid
            ? "creative_editor_world_layout_building_manipulation_ready"
            : "creative_editor_world_layout_building_manipulation_out_of_range";
    state.statusMessage =
        previewValid ? "building move preview"
                     : "building move exceeds the layout coordinate range";
    return {true, true, state.buildingManipulation.reasonCode};
  }

  const CreativeEditorWorldLayoutEditReceipt updated =
      applyCreativeEditorWorldLayoutBuildingManipulation(
          state,
          CreativeEditorWorldLayoutBuildingManipulationPhase::Update, point,
          toleranceCells);
  if (!updated.accepted) {
    return updated;
  }
  if (!state.buildingManipulation.previewValid) {
    const std::string reasonCode = state.buildingManipulation.reasonCode;
    state.buildingManipulation = {};
    return {false, false, reasonCode};
  }
  const std::size_t buildingIndex =
      state.buildingManipulation.buildingIndex;
  const std::int64_t deltaXCells =
      state.buildingManipulation.previewDeltaXCells;
  const std::int64_t deltaZCells =
      state.buildingManipulation.previewDeltaZCells;
  state.buildingManipulation = {};
  if (deltaXCells == 0 && deltaZCells == 0) {
    state.statusMessage = "building unchanged";
    return {true, false,
            "creative_editor_world_layout_building_manipulation_no_change"};
  }
  cr::CreativeWorldLayoutBuildingEditResult moved =
      cr::moveCreativeWorldLayoutBuilding(
          state.source, {buildingIndex, deltaXCells, deltaZCells});
  if (!moved.accepted || !moved.changed) {
    state.statusMessage = "building move exceeds the layout coordinate range";
    return {false, false,
            "creative_editor_world_layout_building_manipulation_out_of_range"};
  }
  state.source = std::move(moved.edited);
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Building,
                     buildingIndex};
  detail::noteWorldLayoutSourceChange(state, "building moved");
  return {true, true,
          "creative_editor_world_layout_building_manipulation_committed"};
}

CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutBuildingTransform(
    CreativeEditorWorldLayoutState &state,
    CreativeEditorWorldLayoutBuildingTransformPhase phase,
    cr::CreativeWorldLayoutBuildingTransformOperation operation) {
  if (phase >= CreativeEditorWorldLayoutBuildingTransformPhase::Count) {
    return {false, false,
            "creative_editor_world_layout_building_transform_phase_invalid"};
  }
  if (phase == CreativeEditorWorldLayoutBuildingTransformPhase::Cancel) {
    const bool changed = state.buildingTransform.active;
    state.buildingTransform = {};
    state.statusMessage = changed ? "building transform cancelled"
                                  : "no building transform to cancel";
    return {true, changed,
            "creative_editor_world_layout_building_transform_cancelled"};
  }
  if (phase == CreativeEditorWorldLayoutBuildingTransformPhase::Commit) {
    if (!state.buildingTransform.active) {
      return {false, false,
              "creative_editor_world_layout_building_transform_not_active"};
    }
    if (state.buildingTransform.sourceRevision != state.revision ||
        state.buildingTransform.buildingIndex >=
            state.source.buildings.size()) {
      state.buildingTransform = {};
      state.statusMessage = "building changed while transform was previewed";
      return {false, false,
              "creative_editor_world_layout_building_transform_stale"};
    }
    const std::size_t buildingIndex = state.buildingTransform.buildingIndex;
    cr::CreativeWorldLayout candidate =
        std::move(state.buildingTransform.candidate);
    state.buildingTransform = {};
    state.source = std::move(candidate);
    state.selection = {CreativeEditorWorldLayoutSelectionKind::Building,
                       buildingIndex};
    detail::noteWorldLayoutSourceChange(state, "building transformed");
    return {true, true,
            "creative_editor_world_layout_building_transform_committed"};
  }

  if (state.tool != CreativeEditorWorldLayoutTool::Select ||
      state.selection.kind !=
          CreativeEditorWorldLayoutSelectionKind::Building ||
      state.selection.index >= state.source.buildings.size()) {
    state.statusMessage = "select a building before transforming it";
    return {
        false, false,
        "creative_editor_world_layout_building_transform_selection_missing"};
  }
  cr::CreativeWorldLayoutBuildingTransformResult transformed =
      cr::transformCreativeWorldLayoutBuilding(
          state.source, {state.selection.index, operation});
  if (!transformed.accepted) {
    state.statusMessage = transformed.reasonCode;
    return {false, false, transformed.reasonCode};
  }
  const bool samePreview = state.buildingTransform.active &&
                           state.buildingTransform.operation == operation;
  detail::clearWorldLayoutInteraction(state);
  detail::invalidateWorldLayoutPreview(state);
  state.buildingTransform.active = true;
  state.buildingTransform.sourceRevision = state.revision;
  state.buildingTransform.buildingIndex = transformed.buildingIndex;
  state.buildingTransform.operation = transformed.operation;
  state.buildingTransform.sourceBounds = transformed.sourceBounds;
  state.buildingTransform.previewBounds = transformed.transformedBounds;
  state.buildingTransform.candidate = std::move(transformed.transformed);
  state.buildingTransform.reasonCode = transformed.reasonCode;
  state.statusMessage = std::string(cr::toString(operation)) + " preview";
  return {true, !samePreview,
          "creative_editor_world_layout_building_transform_previewed"};
}

bool defaultCreativeEditorWorldLayoutBuildingDuplicateOffset(
    const CreativeEditorWorldLayoutState& state, std::size_t buildingIndex,
    std::int64_t& deltaXCells, std::int64_t& deltaZCells) noexcept {
  return !state.buildingTransform.active &&
         cr::defaultCreativeWorldLayoutBuildingDuplicateOffset(
             state.source, buildingIndex, deltaXCells, deltaZCells);
}

CreativeEditorWorldLayoutEditReceipt duplicateCreativeEditorWorldLayoutBuilding(
    CreativeEditorWorldLayoutState& state, std::size_t buildingIndex,
    std::int64_t deltaXCells, std::int64_t deltaZCells) {
  if (state.buildingTransform.active) {
    state.statusMessage = "building cannot be duplicated at that offset";
    return {false, false,
            "creative_editor_world_layout_building_duplicate_invalid"};
  }
  cr::CreativeWorldLayoutBuildingEditResult duplicated =
      cr::duplicateCreativeWorldLayoutBuilding(
          state.source,
          {buildingIndex, deltaXCells, deltaZCells, state.nextStableOrdinal});
  if (!duplicated.accepted || !duplicated.changed) {
    state.statusMessage = "building cannot be duplicated at that offset";
    return {false, false,
            "creative_editor_world_layout_building_duplicate_invalid"};
  }
  const std::size_t duplicateBuildingIndex = duplicated.resultBuildingIndex;
  state.source = std::move(duplicated.edited);
  state.nextStableOrdinal = duplicated.nextStableOrdinal;
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Building,
                     duplicateBuildingIndex};
  state.activeLevelIndex = cr::kInvalidCreativeWorldLayoutIndex;
  repairCreativeEditorWorldLayoutActiveLevel(state, duplicateBuildingIndex);
  detail::noteWorldLayoutSourceChange(state, "building duplicated");
  return {true, true,
          "creative_editor_world_layout_building_duplicated"};
}

CreativeEditorWorldLayoutEditReceipt deleteCreativeEditorWorldLayoutBuilding(
    CreativeEditorWorldLayoutState& state, std::size_t buildingIndex) {
  if (state.buildingTransform.active) {
    state.statusMessage = "building cannot be deleted";
    return {false, false,
            "creative_editor_world_layout_building_delete_invalid"};
  }
  cr::CreativeWorldLayoutBuildingEditResult removed =
      cr::deleteCreativeWorldLayoutBuilding(state.source, {buildingIndex});
  if (!removed.accepted || !removed.changed) {
    state.statusMessage = "building cannot be deleted";
    return {false, false,
            "creative_editor_world_layout_building_delete_invalid"};
  }
  state.source = std::move(removed.edited);
  state.selection = {};
  state.activeLevelIndex = cr::kInvalidCreativeWorldLayoutIndex;
  repairCreativeEditorWorldLayoutActiveLevel(state);
  detail::noteWorldLayoutSourceChange(state, "building deleted");
  return {true, true, "creative_editor_world_layout_building_deleted"};
}

}  // namespace iggy3d_creative_app
