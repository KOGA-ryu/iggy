#include "EditorWorldLayoutPlan.hpp"

#include "EditorWorldLayoutInternal.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>

namespace iggy3d_creative_app {
namespace {

constexpr double kGeometryEpsilon = 1.0e-9;
constexpr double kOpeningEndClearanceCells = 0.25;

struct StructuralValidation {
  bool accepted = false;
  std::string reasonCode = "creative_editor_world_layout_structure_invalid";
  std::string message = "structural settings are invalid";
};

bool validRect(cr::CreativeWorldLayoutRect rect) noexcept {
  return rect.minimum.x < rect.maximum.x &&
         rect.minimum.z < rect.maximum.z;
}

bool sameRect(cr::CreativeWorldLayoutRect lhs,
              cr::CreativeWorldLayoutRect rhs) noexcept {
  return lhs.minimum == rhs.minimum && lhs.maximum == rhs.maximum;
}

CreativeEditorWorldLayoutBoxSettings boxSettings(
    const cr::CreativeWorldLayoutBox& box) noexcept {
  return {box.footprint, box.anchorLayer, box.layerCount};
}

StructuralValidation validateBoxSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t boxIndex,
    const CreativeEditorWorldLayoutBoxSettings& settings) {
  if (boxIndex >= state.source.boxes.size() || !validRect(settings.footprint) ||
      !std::isfinite(settings.anchorLayer) || settings.layerCount == 0U) {
    return {false, "creative_editor_world_layout_box_settings_invalid",
            "surface needs finite elevation and positive dimensions"};
  }
  const cr::CreativeWorldLayoutBox& box = state.source.boxes[boxIndex];
  if (box.buildingIndex >= state.source.buildings.size()) {
    return {false, "creative_editor_world_layout_box_building_invalid",
            "floor building owner is unavailable"};
  }
  return {true, "creative_editor_world_layout_box_settings_ready",
          "floor settings ready"};
}

CreativeEditorWorldLayoutEditReceipt commitBoxSettings(
    CreativeEditorWorldLayoutState& state, std::size_t boxIndex,
    const CreativeEditorWorldLayoutBoxSettings& settings,
    std::string statusMessage) {
  const StructuralValidation validation =
      validateBoxSettings(state, boxIndex, settings);
  if (!validation.accepted) {
    state.statusMessage = validation.message;
    return {false, false, validation.reasonCode};
  }
  cr::CreativeWorldLayoutBox& box = state.source.boxes[boxIndex];
  if (sameRect(box.footprint, settings.footprint) &&
      box.anchorLayer == settings.anchorLayer &&
      box.layerCount == settings.layerCount) {
    return {true, false,
            "creative_editor_world_layout_box_settings_no_change"};
  }
  box.footprint = settings.footprint;
  box.anchorLayer = settings.anchorLayer;
  box.layerCount = settings.layerCount;
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Box, boxIndex};
  detail::noteWorldLayoutSourceChange(state, std::move(statusMessage));
  return {true, true, "creative_editor_world_layout_box_settings_updated"};
}

CreativeEditorWorldLayoutBoxTarget boxTargetAt(
    const CreativeEditorWorldLayoutState& state, std::size_t boxIndex,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) noexcept {
  if (boxIndex >= state.source.boxes.size()) {
    return {};
  }
  const CreativeEditorWorldLayoutBoxHandle handle =
      detail::worldLayoutRectHandleAt(state.source.boxes[boxIndex].footprint,
                                      point, toleranceCells);
  return handle == CreativeEditorWorldLayoutBoxHandle::None
             ? CreativeEditorWorldLayoutBoxTarget{}
             : CreativeEditorWorldLayoutBoxTarget{boxIndex, handle};
}

CreativeEditorWorldLayoutWallSettings wallSettings(
    const cr::CreativeWorldLayoutWall& wall) noexcept {
  return {wall.start, wall.end, wall.baseLayer, wall.heightCells,
          wall.thicknessCells};
}

double wallLength(const cr::CreativeTerrainCoord2 start,
                  const cr::CreativeTerrainCoord2 end) noexcept {
  return std::hypot(static_cast<double>(end.x) - start.x,
                    static_cast<double>(end.z) - start.z);
}

bool cardinalWall(cr::CreativeTerrainCoord2 start,
                  cr::CreativeTerrainCoord2 end) noexcept {
  return start != end && (start.x == end.x || start.z == end.z);
}

bool sameWallDirection(cr::CreativeTerrainCoord2 originalStart,
                       cr::CreativeTerrainCoord2 originalEnd,
                       cr::CreativeTerrainCoord2 candidateStart,
                       cr::CreativeTerrainCoord2 candidateEnd) noexcept {
  const long double originalX =
      static_cast<long double>(originalEnd.x) - originalStart.x;
  const long double originalZ =
      static_cast<long double>(originalEnd.z) - originalStart.z;
  const long double candidateX =
      static_cast<long double>(candidateEnd.x) - candidateStart.x;
  const long double candidateZ =
      static_cast<long double>(candidateEnd.z) - candidateStart.z;
  return originalX * candidateX + originalZ * candidateZ > 0;
}

StructuralValidation validateWallSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t wallIndex,
    const CreativeEditorWorldLayoutWallSettings& settings,
    double openingOffsetDeltaCells) {
  if (wallIndex >= state.source.walls.size() ||
      !cardinalWall(settings.start, settings.end) ||
      !std::isfinite(settings.baseLayer) || settings.heightCells == 0U ||
      !std::isfinite(settings.thicknessCells) ||
      settings.thicknessCells <= 0.0 ||
      !std::isfinite(openingOffsetDeltaCells)) {
    return {false, "creative_editor_world_layout_wall_settings_invalid",
            "partition needs cardinal length, height, and thickness"};
  }
  const cr::CreativeWorldLayoutWall& wall = state.source.walls[wallIndex];
  if (wall.buildingIndex >= state.source.buildings.size()) {
    return {false, "creative_editor_world_layout_wall_building_invalid",
            "partition building owner is unavailable"};
  }

  const double length = wallLength(settings.start, settings.end);
  for (std::size_t index = 0U; index < state.source.openings.size(); ++index) {
    const cr::CreativeWorldLayoutOpening& opening = state.source.openings[index];
    if (opening.hostKind != cr::CreativeWorldLayoutOpeningHostKind::Wall ||
        opening.wallIndex != wallIndex) {
      continue;
    }
    const double center =
        opening.centerOffsetCells + openingOffsetDeltaCells;
    const double halfWidth = opening.widthCells * 0.5;
    if (!std::isfinite(center) || !std::isfinite(opening.widthCells) ||
        opening.widthCells <= 0.0 ||
        center - halfWidth <
            kOpeningEndClearanceCells - kGeometryEpsilon ||
        center + halfWidth >
            length - kOpeningEndClearanceCells + kGeometryEpsilon) {
      return {false, "creative_editor_world_layout_wall_opening_fit_invalid",
              "partition edit would clip a hosted opening"};
    }
    if (!std::isfinite(opening.cutoutBottomCells) ||
        !std::isfinite(opening.cutoutHeightCells) ||
        opening.cutoutBottomCells < 0.0 ||
        opening.cutoutHeightCells <= 0.0 ||
        opening.cutoutBottomCells + opening.cutoutHeightCells >
            settings.heightCells + kGeometryEpsilon) {
      return {
          false, "creative_editor_world_layout_wall_opening_height_invalid",
          "partition height would clip a hosted opening"};
    }
    for (std::size_t prior = 0U; prior < index; ++prior) {
      const cr::CreativeWorldLayoutOpening& existing =
          state.source.openings[prior];
      if (existing.hostKind != cr::CreativeWorldLayoutOpeningHostKind::Wall ||
          existing.wallIndex != wallIndex) {
        continue;
      }
      if (std::fabs(opening.centerOffsetCells -
                    existing.centerOffsetCells) <=
          (opening.widthCells + existing.widthCells) * 0.5 +
              kGeometryEpsilon) {
        return {false, "creative_editor_world_layout_opening_overlap",
                "partition contains overlapping openings"};
      }
    }
  }
  return {true, "creative_editor_world_layout_wall_settings_ready",
          "partition settings ready"};
}

CreativeEditorWorldLayoutEditReceipt commitWallSettings(
    CreativeEditorWorldLayoutState& state, std::size_t wallIndex,
    const CreativeEditorWorldLayoutWallSettings& settings,
    double openingOffsetDeltaCells, std::string statusMessage) {
  const StructuralValidation validation = validateWallSettings(
      state, wallIndex, settings, openingOffsetDeltaCells);
  if (!validation.accepted) {
    state.statusMessage = validation.message;
    return {false, false, validation.reasonCode};
  }
  cr::CreativeWorldLayoutWall& wall = state.source.walls[wallIndex];
  const bool wallChanged = wall.start != settings.start ||
                           wall.end != settings.end ||
                           wall.baseLayer != settings.baseLayer ||
                           wall.heightCells != settings.heightCells ||
                           wall.thicknessCells != settings.thicknessCells;
  if (!wallChanged && openingOffsetDeltaCells == 0.0) {
    return {true, false,
            "creative_editor_world_layout_wall_settings_no_change"};
  }
  wall.start = settings.start;
  wall.end = settings.end;
  wall.baseLayer = settings.baseLayer;
  wall.heightCells = settings.heightCells;
  wall.thicknessCells = settings.thicknessCells;
  if (openingOffsetDeltaCells != 0.0) {
    for (cr::CreativeWorldLayoutOpening& opening : state.source.openings) {
      if (opening.hostKind == cr::CreativeWorldLayoutOpeningHostKind::Wall &&
          opening.wallIndex == wallIndex) {
        opening.centerOffsetCells += openingOffsetDeltaCells;
      }
    }
  }
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Wall, wallIndex};
  detail::noteWorldLayoutSourceChange(state, std::move(statusMessage));
  return {true, true, "creative_editor_world_layout_wall_settings_updated"};
}

double pointDistance(CreativeEditorWorldLayoutPoint lhs,
                     CreativeEditorWorldLayoutPoint rhs) noexcept {
  return std::hypot(lhs.x - rhs.x, lhs.z - rhs.z);
}

CreativeEditorWorldLayoutPoint wallPoint(cr::CreativeTerrainCoord2 point) {
  return {static_cast<double>(point.x), static_cast<double>(point.z)};
}

double pointSegmentDistance(CreativeEditorWorldLayoutPoint point,
                            cr::CreativeTerrainCoord2 start,
                            cr::CreativeTerrainCoord2 end) noexcept {
  const double dx = static_cast<double>(end.x) - start.x;
  const double dz = static_cast<double>(end.z) - start.z;
  const double lengthSquared = dx * dx + dz * dz;
  if (lengthSquared <= 0.0) {
    return std::numeric_limits<double>::infinity();
  }
  const double t =
      std::clamp(((point.x - start.x) * dx + (point.z - start.z) * dz) /
                     lengthSquared,
                 0.0, 1.0);
  return std::hypot(point.x - (start.x + t * dx),
                    point.z - (start.z + t * dz));
}

CreativeEditorWorldLayoutWallTarget wallTargetAt(
    const CreativeEditorWorldLayoutState& state, std::size_t wallIndex,
    CreativeEditorWorldLayoutPoint point, double toleranceCells,
    bool includeEndpoints) noexcept {
  if (wallIndex >= state.source.walls.size() ||
      !detail::finiteWorldLayoutPoint(point) ||
      !std::isfinite(toleranceCells) || toleranceCells <= 0.0) {
    return {};
  }
  const cr::CreativeWorldLayoutWall& wall = state.source.walls[wallIndex];
  if (includeEndpoints) {
    const double startDistance = pointDistance(point, wallPoint(wall.start));
    const double endDistance = pointDistance(point, wallPoint(wall.end));
    if (startDistance <= toleranceCells || endDistance <= toleranceCells) {
      return {wallIndex,
              startDistance <= endDistance
                  ? CreativeEditorWorldLayoutWallHandle::Start
                  : CreativeEditorWorldLayoutWallHandle::End};
    }
  }
  return pointSegmentDistance(point, wall.start, wall.end) <= toleranceCells
             ? CreativeEditorWorldLayoutWallTarget{
                   wallIndex, CreativeEditorWorldLayoutWallHandle::Move}
             : CreativeEditorWorldLayoutWallTarget{};
}

bool offsetPoint(cr::CreativeTerrainCoord2 point, std::int64_t deltaX,
                 std::int64_t deltaZ,
                 cr::CreativeTerrainCoord2& output) noexcept {
  return detail::offsetWorldLayoutCoordinate(point.x, deltaX, output.x) &&
         detail::offsetWorldLayoutCoordinate(point.z, deltaZ, output.z);
}

bool wallManipulationCandidate(
    const CreativeEditorWorldLayoutWallManipulationState& manipulation,
    CreativeEditorWorldLayoutPoint point, cr::CreativeTerrainCoord2& start,
    cr::CreativeTerrainCoord2& end,
    double& openingOffsetDeltaCells) noexcept {
  start = manipulation.originalStart;
  end = manipulation.originalEnd;
  openingOffsetDeltaCells = 0.0;
  std::int64_t deltaX = 0;
  std::int64_t deltaZ = 0;
  if (!detail::snappedWorldLayoutPointerDelta(
          point.x, manipulation.startPoint.x, deltaX) ||
      !detail::snappedWorldLayoutPointerDelta(
          point.z, manipulation.startPoint.z, deltaZ)) {
    return false;
  }
  switch (manipulation.target.handle) {
    case CreativeEditorWorldLayoutWallHandle::Move:
      return offsetPoint(manipulation.originalStart, deltaX, deltaZ, start) &&
             offsetPoint(manipulation.originalEnd, deltaX, deltaZ, end);
    case CreativeEditorWorldLayoutWallHandle::Start: {
      const bool horizontal =
          manipulation.originalStart.z == manipulation.originalEnd.z;
      if (horizontal) {
        if (!detail::offsetWorldLayoutCoordinate(manipulation.originalStart.x,
                                                 deltaX, start.x)) {
          return false;
        }
        const double direction = manipulation.originalEnd.x >
                                         manipulation.originalStart.x
                                     ? 1.0
                                     : -1.0;
        openingOffsetDeltaCells = -static_cast<double>(deltaX) * direction;
      } else {
        if (!detail::offsetWorldLayoutCoordinate(manipulation.originalStart.z,
                                                 deltaZ, start.z)) {
          return false;
        }
        const double direction = manipulation.originalEnd.z >
                                         manipulation.originalStart.z
                                     ? 1.0
                                     : -1.0;
        openingOffsetDeltaCells = -static_cast<double>(deltaZ) * direction;
      }
      return true;
    }
    case CreativeEditorWorldLayoutWallHandle::End:
      if (manipulation.originalStart.z == manipulation.originalEnd.z) {
        return detail::offsetWorldLayoutCoordinate(
            manipulation.originalEnd.x, deltaX, end.x);
      }
      return detail::offsetWorldLayoutCoordinate(manipulation.originalEnd.z,
                                                 deltaZ, end.z);
    case CreativeEditorWorldLayoutWallHandle::None:
    case CreativeEditorWorldLayoutWallHandle::Count:
      return false;
  }
  return false;
}

}  // namespace

bool readCreativeEditorWorldLayoutBoxSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t boxIndex,
    CreativeEditorWorldLayoutBoxSettings& output) noexcept {
  if (boxIndex >= state.source.boxes.size()) {
    return false;
  }
  output = boxSettings(state.source.boxes[boxIndex]);
  return true;
}

CreativeEditorWorldLayoutEditReceipt setCreativeEditorWorldLayoutBoxSettings(
    CreativeEditorWorldLayoutState& state, std::size_t boxIndex,
    CreativeEditorWorldLayoutBoxSettings settings) {
  return commitBoxSettings(state, boxIndex, settings,
                           "floor settings updated");
}

CreativeEditorWorldLayoutBoxTarget findCreativeEditorWorldLayoutBoxTarget(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) noexcept {
  if (!detail::finiteWorldLayoutPoint(point) ||
      !std::isfinite(toleranceCells) || toleranceCells <= 0.0) {
    return {};
  }
  if (state.selection.kind == CreativeEditorWorldLayoutSelectionKind::Box &&
      state.selection.index < state.source.boxes.size()) {
    const CreativeEditorWorldLayoutBoxTarget selected = boxTargetAt(
        state, state.selection.index, point, toleranceCells);
    if (selected.handle != CreativeEditorWorldLayoutBoxHandle::None) {
      return selected;
    }
  }
  for (std::size_t index = state.source.boxes.size(); index > 0U; --index) {
    const CreativeEditorWorldLayoutBoxTarget target =
        boxTargetAt(state, index - 1U, point, toleranceCells);
    if (target.handle != CreativeEditorWorldLayoutBoxHandle::None) {
      return target;
    }
  }
  return {};
}

CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutBoxManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutBoxManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) {
  if (phase >= CreativeEditorWorldLayoutBoxManipulationPhase::Count) {
    return {false, false,
            "creative_editor_world_layout_box_manipulation_phase_invalid"};
  }
  if (phase == CreativeEditorWorldLayoutBoxManipulationPhase::Cancel) {
    const bool changed = state.boxManipulation.active;
    state.boxManipulation = {};
    state.statusMessage = "floor manipulation cancelled";
    return {true, changed,
            "creative_editor_world_layout_box_manipulation_cancelled"};
  }
  if (state.tool != CreativeEditorWorldLayoutTool::Select) {
    return {false, false,
            "creative_editor_world_layout_box_manipulation_tool_invalid"};
  }
  if (phase == CreativeEditorWorldLayoutBoxManipulationPhase::Begin) {
    const CreativeEditorWorldLayoutBoxTarget target =
        findCreativeEditorWorldLayoutBoxTarget(state, point, toleranceCells);
    if (target.handle == CreativeEditorWorldLayoutBoxHandle::None ||
        target.boxIndex >= state.source.boxes.size()) {
      return {false, false,
              "creative_editor_world_layout_box_manipulation_target_missing"};
    }
    const cr::CreativeWorldLayoutRect footprint =
        state.source.boxes[target.boxIndex].footprint;
    detail::clearWorldLayoutInteraction(state);
    state.selection = {CreativeEditorWorldLayoutSelectionKind::Box,
                       target.boxIndex};
    state.anchorActive = false;
    state.boxManipulation = {
        true,
        state.revision,
        target,
        point,
        footprint,
        footprint,
        true,
        "creative_editor_world_layout_box_manipulation_ready",
    };
    state.statusMessage =
        target.handle == CreativeEditorWorldLayoutBoxHandle::Move
            ? "drag to move floor"
            : "drag to resize floor";
    return {true, true,
            "creative_editor_world_layout_box_manipulation_started"};
  }
  if (!state.boxManipulation.active ||
      state.boxManipulation.target.boxIndex >= state.source.boxes.size()) {
    return {false, false,
            "creative_editor_world_layout_box_manipulation_not_active"};
  }
  const std::size_t boxIndex = state.boxManipulation.target.boxIndex;
  const cr::CreativeWorldLayoutBox& box = state.source.boxes[boxIndex];
  if (state.revision != state.boxManipulation.sourceRevision ||
      !sameRect(box.footprint,
                state.boxManipulation.originalFootprint)) {
    state.boxManipulation = {};
    state.statusMessage = "floor changed while drag was active";
    return {false, false,
            "creative_editor_world_layout_box_manipulation_stale"};
  }
  if (phase == CreativeEditorWorldLayoutBoxManipulationPhase::Update) {
    cr::CreativeWorldLayoutRect footprint;
    const bool coordinateValid = detail::worldLayoutManipulatedRect(
        state.boxManipulation, point, footprint);
    if (coordinateValid &&
        sameRect(footprint, state.boxManipulation.previewFootprint)) {
      return {true, false, state.boxManipulation.reasonCode};
    }
    StructuralValidation validation;
    if (coordinateValid) {
      CreativeEditorWorldLayoutBoxSettings settings = boxSettings(box);
      settings.footprint = footprint;
      validation = validateBoxSettings(state, boxIndex, settings);
    } else {
      validation.reasonCode =
          "creative_editor_world_layout_box_manipulation_out_of_range";
      validation.message = "floor drag exceeds the layout coordinate range";
    }
    state.boxManipulation.previewFootprint = footprint;
    state.boxManipulation.previewValid = validation.accepted;
    state.boxManipulation.reasonCode = validation.reasonCode;
    state.statusMessage = validation.accepted ? "floor drag preview"
                                              : validation.message;
    return {true, true, validation.reasonCode};
  }

  const CreativeEditorWorldLayoutEditReceipt updated =
      applyCreativeEditorWorldLayoutBoxManipulation(
          state, CreativeEditorWorldLayoutBoxManipulationPhase::Update, point,
          toleranceCells);
  if (!updated.accepted) {
    return updated;
  }
  if (!state.boxManipulation.previewValid) {
    const std::string reasonCode = state.boxManipulation.reasonCode;
    state.boxManipulation = {};
    return {false, false, reasonCode};
  }
  const cr::CreativeWorldLayoutRect footprint =
      state.boxManipulation.previewFootprint;
  state.boxManipulation = {};
  CreativeEditorWorldLayoutBoxSettings settings = boxSettings(box);
  settings.footprint = footprint;
  return commitBoxSettings(state, boxIndex, settings, "floor updated");
}

bool readCreativeEditorWorldLayoutWallSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t wallIndex,
    CreativeEditorWorldLayoutWallSettings& output) noexcept {
  if (wallIndex >= state.source.walls.size()) {
    return false;
  }
  output = wallSettings(state.source.walls[wallIndex]);
  return true;
}

CreativeEditorWorldLayoutEditReceipt setCreativeEditorWorldLayoutWallSettings(
    CreativeEditorWorldLayoutState& state, std::size_t wallIndex,
    CreativeEditorWorldLayoutWallSettings settings) {
  return commitWallSettings(state, wallIndex, settings, 0.0,
                            "partition settings updated");
}

CreativeEditorWorldLayoutWallTarget findCreativeEditorWorldLayoutWallTarget(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) noexcept {
  if (!detail::finiteWorldLayoutPoint(point) ||
      !std::isfinite(toleranceCells) || toleranceCells <= 0.0) {
    return {};
  }
  if (state.selection.kind == CreativeEditorWorldLayoutSelectionKind::Wall &&
      state.selection.index < state.source.walls.size()) {
    const CreativeEditorWorldLayoutWallTarget selected = wallTargetAt(
        state, state.selection.index, point, toleranceCells, true);
    if (selected.handle != CreativeEditorWorldLayoutWallHandle::None) {
      return selected;
    }
  }
  for (std::size_t index = state.source.walls.size(); index > 0U; --index) {
    const CreativeEditorWorldLayoutWallTarget target = wallTargetAt(
        state, index - 1U, point, toleranceCells, false);
    if (target.handle != CreativeEditorWorldLayoutWallHandle::None) {
      return target;
    }
  }
  return {};
}

CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutWallManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutWallManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) {
  if (phase >= CreativeEditorWorldLayoutWallManipulationPhase::Count) {
    return {false, false,
            "creative_editor_world_layout_wall_manipulation_phase_invalid"};
  }
  if (phase == CreativeEditorWorldLayoutWallManipulationPhase::Cancel) {
    const bool changed = state.wallManipulation.active;
    state.wallManipulation = {};
    state.statusMessage = "partition manipulation cancelled";
    return {true, changed,
            "creative_editor_world_layout_wall_manipulation_cancelled"};
  }
  if (state.tool != CreativeEditorWorldLayoutTool::Select) {
    return {false, false,
            "creative_editor_world_layout_wall_manipulation_tool_invalid"};
  }
  if (phase == CreativeEditorWorldLayoutWallManipulationPhase::Begin) {
    const CreativeEditorWorldLayoutWallTarget target =
        findCreativeEditorWorldLayoutWallTarget(state, point, toleranceCells);
    if (target.handle == CreativeEditorWorldLayoutWallHandle::None ||
        target.wallIndex >= state.source.walls.size()) {
      return {false, false,
              "creative_editor_world_layout_wall_manipulation_target_missing"};
    }
    const cr::CreativeWorldLayoutWall& wall =
        state.source.walls[target.wallIndex];
    detail::clearWorldLayoutInteraction(state);
    state.selection = {CreativeEditorWorldLayoutSelectionKind::Wall,
                       target.wallIndex};
    state.anchorActive = false;
    state.wallManipulation = {
        true,
        state.revision,
        target,
        point,
        wall.start,
        wall.end,
        wall.start,
        wall.end,
        0.0,
        true,
        "creative_editor_world_layout_wall_manipulation_ready",
    };
    state.statusMessage =
        target.handle == CreativeEditorWorldLayoutWallHandle::Move
            ? "drag to move partition"
            : "drag partition endpoint";
    return {true, true,
            "creative_editor_world_layout_wall_manipulation_started"};
  }
  if (!state.wallManipulation.active ||
      state.wallManipulation.target.wallIndex >= state.source.walls.size()) {
    return {false, false,
            "creative_editor_world_layout_wall_manipulation_not_active"};
  }
  const std::size_t wallIndex = state.wallManipulation.target.wallIndex;
  const cr::CreativeWorldLayoutWall& wall = state.source.walls[wallIndex];
  if (state.revision != state.wallManipulation.sourceRevision ||
      wall.start != state.wallManipulation.originalStart ||
      wall.end != state.wallManipulation.originalEnd) {
    state.wallManipulation = {};
    state.statusMessage = "partition changed while drag was active";
    return {false, false,
            "creative_editor_world_layout_wall_manipulation_stale"};
  }
  if (phase == CreativeEditorWorldLayoutWallManipulationPhase::Update) {
    cr::CreativeTerrainCoord2 start;
    cr::CreativeTerrainCoord2 end;
    double openingOffsetDeltaCells = 0.0;
    const bool coordinateValid = wallManipulationCandidate(
        state.wallManipulation, point, start, end, openingOffsetDeltaCells);
    if (coordinateValid && start == state.wallManipulation.previewStart &&
        end == state.wallManipulation.previewEnd &&
        openingOffsetDeltaCells ==
            state.wallManipulation.previewOpeningOffsetDeltaCells) {
      return {true, false, state.wallManipulation.reasonCode};
    }
    StructuralValidation validation;
    if (coordinateValid &&
        sameWallDirection(state.wallManipulation.originalStart,
                          state.wallManipulation.originalEnd, start, end)) {
      CreativeEditorWorldLayoutWallSettings settings = wallSettings(wall);
      settings.start = start;
      settings.end = end;
      validation = validateWallSettings(state, wallIndex, settings,
                                        openingOffsetDeltaCells);
    } else {
      validation.reasonCode =
          "creative_editor_world_layout_wall_manipulation_out_of_range";
      validation.message =
          "partition endpoint cannot cross or leave its authored axis";
    }
    state.wallManipulation.previewStart = start;
    state.wallManipulation.previewEnd = end;
    state.wallManipulation.previewOpeningOffsetDeltaCells =
        openingOffsetDeltaCells;
    state.wallManipulation.previewValid = validation.accepted;
    state.wallManipulation.reasonCode = validation.reasonCode;
    state.statusMessage = validation.accepted ? "partition drag preview"
                                              : validation.message;
    return {true, true, validation.reasonCode};
  }

  const CreativeEditorWorldLayoutEditReceipt updated =
      applyCreativeEditorWorldLayoutWallManipulation(
          state, CreativeEditorWorldLayoutWallManipulationPhase::Update, point,
          toleranceCells);
  if (!updated.accepted) {
    return updated;
  }
  if (!state.wallManipulation.previewValid) {
    const std::string reasonCode = state.wallManipulation.reasonCode;
    state.wallManipulation = {};
    return {false, false, reasonCode};
  }
  const cr::CreativeTerrainCoord2 start = state.wallManipulation.previewStart;
  const cr::CreativeTerrainCoord2 end = state.wallManipulation.previewEnd;
  const double openingOffsetDeltaCells =
      state.wallManipulation.previewOpeningOffsetDeltaCells;
  state.wallManipulation = {};
  CreativeEditorWorldLayoutWallSettings settings = wallSettings(wall);
  settings.start = start;
  settings.end = end;
  return commitWallSettings(state, wallIndex, settings,
                            openingOffsetDeltaCells, "partition updated");
}

}  // namespace iggy3d_creative_app
