#include "EditorWorldLayoutElevation.hpp"
#include "EditorWorldLayoutRoofs.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"

namespace iggy3d_creative_app {
namespace {

constexpr double kEditSnapCells = 0.25;
constexpr double kMinimumOpeningHeightCells = 0.25;
constexpr double kGeometryEpsilon = 1.0e-9;

struct OpeningHostFacts {
  bool valid = false;
  double baseLayer = 0.0;
  double heightCells = 0.0;
};

OpeningHostFacts resolveOpeningHost(
    const cr::CreativeWorldLayout& layout,
    const cr::CreativeWorldLayoutOpening& opening) noexcept {
  OpeningHostFacts host;
  if (opening.hostKind == cr::CreativeWorldLayoutOpeningHostKind::Wall) {
    if (opening.wallIndex >= layout.walls.size()) {
      return host;
    }
    host.baseLayer = layout.walls[opening.wallIndex].baseLayer;
    host.heightCells = layout.walls[opening.wallIndex].heightCells;
  } else if (opening.hostKind ==
             cr::CreativeWorldLayoutOpeningHostKind::RoomEdge) {
    if (opening.roomIndex >= layout.rooms.size()) {
      return host;
    }
    const cr::CreativeWorldLayoutLevel* level =
        cr::creativeWorldLayoutLevelForRoom(layout, opening.roomIndex);
    if (level == nullptr) {
      return host;
    }
    host.baseLayer = level->floorTopLayer;
    host.heightCells = level->wallHeightCells;
  } else {
    return host;
  }
  host.valid = std::isfinite(host.baseLayer) &&
               std::isfinite(host.heightCells) && host.heightCells > 0.0;
  return host;
}

double snapQuarterCell(double value) noexcept {
  return std::round(value / kEditSnapCells) * kEditSnapCells;
}

bool levelOwnsRoom(const cr::CreativeWorldLayout& layout,
                   std::size_t levelIndex,
                   std::size_t roomIndex) noexcept {
  return levelIndex < layout.levels.size() && roomIndex < layout.rooms.size() &&
         layout.rooms[roomIndex].levelIndex == levelIndex &&
         layout.rooms[roomIndex].buildingIndex ==
             layout.levels[levelIndex].buildingIndex;
}

double snapLevelDatum(const cr::CreativeWorldLayout& layout,
                      std::size_t levelIndex,
                      double requested) noexcept {
  const cr::CreativeWorldLayoutLevel& level = layout.levels[levelIndex];
  double anchor = level.floorTopLayer;
  for (std::size_t index = 0U; index < layout.levels.size(); ++index) {
    const cr::CreativeWorldLayoutLevel& candidate = layout.levels[index];
    if (candidate.buildingIndex == level.buildingIndex &&
        cr::creativeWorldLayoutLevelHasRooms(layout, index)) {
      anchor = std::min(anchor, candidate.floorTopLayer);
    }
  }
  return anchor + std::round(requested - anchor);
}

bool scopeIncludesLevel(
    CreativeEditorWorldLayoutLevelEditScope scope,
    std::size_t candidateIndex,
    std::size_t selectedIndex,
    double candidateDatum,
    double selectedDatum) noexcept {
  switch (scope) {
    case CreativeEditorWorldLayoutLevelEditScope::Selected:
      return candidateIndex == selectedIndex;
    case CreativeEditorWorldLayoutLevelEditScope::SelectedAndAbove:
      return candidateDatum >= selectedDatum - kGeometryEpsilon;
    case CreativeEditorWorldLayoutLevelEditScope::SelectedAndBelow:
      return candidateDatum <= selectedDatum + kGeometryEpsilon;
    case CreativeEditorWorldLayoutLevelEditScope::All:
      return true;
    case CreativeEditorWorldLayoutLevelEditScope::Count:
      return false;
  }
  return false;
}

bool plannedLevelDatum(
    const cr::CreativeWorldLayout& layout,
    std::size_t selectedLevelIndex,
    CreativeEditorWorldLayoutLevelEditScope scope,
    double delta,
    std::size_t levelIndex,
    double& output) noexcept {
  if (selectedLevelIndex >= layout.levels.size() ||
      levelIndex >= layout.levels.size()) {
    return false;
  }
  const cr::CreativeWorldLayoutLevel& selected =
      layout.levels[selectedLevelIndex];
  const cr::CreativeWorldLayoutLevel& level = layout.levels[levelIndex];
  output = level.floorTopLayer;
  if (level.buildingIndex != selected.buildingIndex ||
      !scopeIncludesLevel(scope, levelIndex, selectedLevelIndex,
                          level.floorTopLayer,
                          selected.floorTopLayer)) {
    return true;
  }
  const long double moved =
      static_cast<long double>(level.floorTopLayer) +
      static_cast<long double>(delta);
  if (!std::isfinite(moved) ||
      moved <
          -static_cast<long double>(std::numeric_limits<double>::max()) ||
      moved >
          static_cast<long double>(std::numeric_limits<double>::max())) {
    return false;
  }
  output = static_cast<double>(moved);
  return std::isfinite(output);
}

bool levelOrderPreserved(
    const cr::CreativeWorldLayout& layout,
    std::size_t buildingIndex,
    std::size_t selectedLevelIndex,
    CreativeEditorWorldLayoutLevelEditScope scope,
    double delta) noexcept {
  for (std::size_t first = 0U; first < layout.levels.size(); ++first) {
    if (layout.levels[first].buildingIndex != buildingIndex) {
      continue;
    }
    double firstDatum = 0.0;
    if (!plannedLevelDatum(layout, selectedLevelIndex, scope, delta, first,
                           firstDatum)) {
      return false;
    }
    for (std::size_t second = first + 1U; second < layout.levels.size();
         ++second) {
      if (layout.levels[second].buildingIndex != buildingIndex) {
        continue;
      }
      double secondDatum = 0.0;
      if (!plannedLevelDatum(layout, selectedLevelIndex, scope, delta, second,
                             secondDatum)) {
        return false;
      }
      const double originalDelta =
          layout.levels[first].floorTopLayer -
          layout.levels[second].floorTopLayer;
      const double candidateDelta = firstDatum - secondDatum;
      if ((originalDelta < -kGeometryEpsilon &&
           candidateDelta >= -kGeometryEpsilon) ||
          (originalDelta > kGeometryEpsilon &&
           candidateDelta <= kGeometryEpsilon)) {
        return false;
      }
    }
  }
  return true;
}

}  // namespace

std::string_view creativeEditorWorldLayoutLevelEditScopeLabel(
    CreativeEditorWorldLayoutLevelEditScope scope) noexcept {
  switch (scope) {
    case CreativeEditorWorldLayoutLevelEditScope::Selected:
      return "Selected";
    case CreativeEditorWorldLayoutLevelEditScope::SelectedAndAbove:
      return "Selected + above";
    case CreativeEditorWorldLayoutLevelEditScope::SelectedAndBelow:
      return "Selected + below";
    case CreativeEditorWorldLayoutLevelEditScope::All:
      return "All levels";
    case CreativeEditorWorldLayoutLevelEditScope::Count:
      return "Invalid";
  }
  return "Invalid";
}

std::string_view creativeEditorWorldLayoutSectionWallConstraintLabel(
    CreativeEditorWorldLayoutSectionWallConstraint constraint) noexcept {
  switch (constraint) {
    case CreativeEditorWorldLayoutSectionWallConstraint::FixedHeight:
      return "fixed";
    case CreativeEditorWorldLayoutSectionWallConstraint::TopLinked:
      return "top-linked";
    case CreativeEditorWorldLayoutSectionWallConstraint::Count:
      return "invalid";
  }
  return "invalid";
}

CreativeEditorWorldLayoutLevelDatumEditPlan
planCreativeEditorWorldLayoutLevelDatumEdit(
    const cr::CreativeWorldLayout& layout,
    CreativeEditorWorldLayoutLevelDatumEditRequest request) {
  CreativeEditorWorldLayoutLevelDatumEditPlan result;
  result.selectedLevelIndex = request.levelIndex;
  result.scope = request.scope;
  if (request.levelIndex >= layout.levels.size() ||
      request.scope >= CreativeEditorWorldLayoutLevelEditScope::Count ||
      !std::isfinite(request.requestedFloorTopLayer) ||
      cr::firstInvalidCreativeWorldLayoutLevelIndex(layout) !=
          cr::kInvalidCreativeWorldLayoutIndex) {
    result.reasonCode =
        "creative_editor_world_layout_level_datum_edit_request_invalid";
    return result;
  }

  const cr::CreativeWorldLayoutLevel& selected =
      layout.levels[request.levelIndex];
  if (selected.buildingIndex >= layout.buildings.size()) {
    result.reasonCode =
        "creative_editor_world_layout_level_datum_edit_request_invalid";
    return result;
  }
  result.buildingIndex = selected.buildingIndex;
  result.selectedFloorTopLayerBefore = selected.floorTopLayer;
  result.snappedFloorTopLayer =
      snapLevelDatum(layout, request.levelIndex,
                     request.requestedFloorTopLayer);
  if (!std::isfinite(result.snappedFloorTopLayer)) {
    result.reasonCode =
        "creative_editor_world_layout_level_datum_edit_unrepresentable";
    return result;
  }

  result.deltaCells =
      result.snappedFloorTopLayer - selected.floorTopLayer;
  result.accepted = true;
  if (std::abs(result.deltaCells) <= kGeometryEpsilon) {
    result.reasonCode =
        "creative_editor_world_layout_level_datum_edit_no_change";
    return result;
  }

  for (std::size_t levelIndex = 0U; levelIndex < layout.levels.size();
       ++levelIndex) {
    const cr::CreativeWorldLayoutLevel& level = layout.levels[levelIndex];
    if (level.buildingIndex != selected.buildingIndex ||
        !scopeIncludesLevel(request.scope, levelIndex, request.levelIndex,
                            level.floorTopLayer,
                            selected.floorTopLayer)) {
      continue;
    }
    double moved = 0.0;
    if (!plannedLevelDatum(layout, request.levelIndex, request.scope,
                           result.deltaCells, levelIndex, moved)) {
      result.accepted = false;
      result.reasonCode =
          "creative_editor_world_layout_level_datum_edit_unrepresentable";
      return result;
    }
    ++result.affectedLevelCount;
  }

  if (result.affectedLevelCount == 0U ||
      !levelOrderPreserved(layout, selected.buildingIndex,
                           request.levelIndex, request.scope,
                           result.deltaCells)) {
    result.accepted = false;
    result.affectedLevelCount = 0U;
    result.reasonCode =
        request.scope == CreativeEditorWorldLayoutLevelEditScope::Selected
            ? "creative_editor_world_layout_elevation_floor_crosses_level"
            : "creative_editor_world_layout_level_datum_scope_crosses_level";
    return result;
  }

  result.changed = true;
  result.reasonCode =
      "creative_editor_world_layout_level_datum_edit_ready";
  return result;
}

bool applyCreativeEditorWorldLayoutLevelDatumEditPlan(
    cr::CreativeWorldLayout& layout,
    const CreativeEditorWorldLayoutLevelDatumEditPlan& plan) noexcept {
  if (!plan.accepted || !plan.changed ||
      plan.selectedLevelIndex >= layout.levels.size() ||
      plan.scope >= CreativeEditorWorldLayoutLevelEditScope::Count) {
    return false;
  }
  const cr::CreativeWorldLayoutLevel& selected =
      layout.levels[plan.selectedLevelIndex];
  if (selected.buildingIndex != plan.buildingIndex ||
      std::abs(selected.floorTopLayer -
               plan.selectedFloorTopLayerBefore) >
          kGeometryEpsilon) {
    return false;
  }

  std::size_t affectedLevelCount = 0U;
  for (std::size_t levelIndex = 0U; levelIndex < layout.levels.size();
       ++levelIndex) {
    if (layout.levels[levelIndex].buildingIndex != plan.buildingIndex ||
        !scopeIncludesLevel(
            plan.scope, levelIndex, plan.selectedLevelIndex,
            layout.levels[levelIndex].floorTopLayer,
            plan.selectedFloorTopLayerBefore)) {
      continue;
    }
    const long double moved =
        static_cast<long double>(
            layout.levels[levelIndex].floorTopLayer) +
        static_cast<long double>(plan.deltaCells);
    if (!std::isfinite(moved) ||
        moved <
            -static_cast<long double>(std::numeric_limits<double>::max()) ||
        moved >
            static_cast<long double>(std::numeric_limits<double>::max())) {
      return false;
    }
    ++affectedLevelCount;
  }
  if (affectedLevelCount != plan.affectedLevelCount) {
    return false;
  }
  for (std::size_t levelIndex = 0U; levelIndex < layout.levels.size();
       ++levelIndex) {
    if (layout.levels[levelIndex].buildingIndex != plan.buildingIndex ||
        !scopeIncludesLevel(
            plan.scope, levelIndex, plan.selectedLevelIndex,
            layout.levels[levelIndex].floorTopLayer,
            plan.selectedFloorTopLayerBefore)) {
      continue;
    }
    layout.levels[levelIndex].floorTopLayer += plan.deltaCells;
  }
  return true;
}

CreativeEditorWorldLayoutElevationHandle
findCreativeEditorWorldLayoutElevationHandle(
    const CreativeEditorWorldLayoutElevationProjection& projection,
    CreativeEditorWorldLayoutElevationPoint point,
    double toleranceCells) noexcept {
  if (!projection.accepted || !std::isfinite(point.horizontal) ||
      !std::isfinite(point.vertical) || !std::isfinite(toleranceCells) ||
      toleranceCells <= 0.0) {
    return {};
  }
  const double toleranceSquared = toleranceCells * toleranceCells;
  for (auto iterator = projection.handles.rbegin();
       iterator != projection.handles.rend(); ++iterator) {
    const double deltaHorizontal =
        point.horizontal - iterator->position.horizontal;
    const double deltaVertical = point.vertical - iterator->position.vertical;
    if (deltaHorizontal * deltaHorizontal + deltaVertical * deltaVertical <=
        toleranceSquared) {
      return *iterator;
    }
  }
  return {};
}

const CreativeEditorWorldLayoutElevationItem*
findCreativeEditorWorldLayoutElevationItem(
    const CreativeEditorWorldLayoutElevationProjection& projection,
    CreativeEditorWorldLayoutElevationPoint point,
    double toleranceCells) noexcept {
  const CreativeEditorWorldLayoutElevationHitStack stack =
      findCreativeEditorWorldLayoutElevationItemStack(projection, point,
                                                      toleranceCells);
  return stack.count == 0U ? nullptr : stack.items[0U];
}

CreativeEditorWorldLayoutElevationHitStack
findCreativeEditorWorldLayoutElevationItemStack(
    const CreativeEditorWorldLayoutElevationProjection& projection,
    CreativeEditorWorldLayoutElevationPoint point,
    double toleranceCells) noexcept {
  CreativeEditorWorldLayoutElevationHitStack result;
  if (!projection.accepted || !std::isfinite(point.horizontal) ||
      !std::isfinite(point.vertical) || !std::isfinite(toleranceCells) ||
      toleranceCells < 0.0) {
    return result;
  }
  for (auto iterator = projection.items.rbegin();
       iterator != projection.items.rend(); ++iterator) {
    ++result.testedItemCount;
    if (point.horizontal >= iterator->minimumHorizontal - toleranceCells &&
        point.horizontal <= iterator->maximumHorizontal + toleranceCells &&
        point.vertical >= iterator->minimumVertical - toleranceCells &&
        point.vertical <= iterator->maximumVertical + toleranceCells) {
      ++result.totalHitItemCount;
      const bool duplicate = std::any_of(
          result.items.begin(), result.items.begin() + result.count,
          [&](const CreativeEditorWorldLayoutElevationItem* item) {
            return item->sourceKind == iterator->sourceKind &&
                   item->sourceIndex == iterator->sourceIndex;
          });
      if (duplicate) {
        continue;
      }
      if (result.count == result.items.size()) {
        result.truncated = true;
        continue;
      }
      result.items[result.count++] = &*iterator;
    }
  }
  return result;
}

const CreativeEditorWorldLayoutElevationItem*
cycleCreativeEditorWorldLayoutElevationItem(
    const CreativeEditorWorldLayoutElevationHitStack& stack,
    CreativeEditorWorldLayoutElevationSourceKind currentSourceKind,
    std::size_t currentSourceIndex) noexcept {
  if (stack.count == 0U) {
    return nullptr;
  }
  for (std::size_t index = 0U; index < stack.count; ++index) {
    const CreativeEditorWorldLayoutElevationItem* item = stack.items[index];
    if (item != nullptr && item->sourceKind == currentSourceKind &&
        item->sourceIndex == currentSourceIndex) {
      return stack.items[(index + 1U) % stack.count];
    }
  }
  return stack.items[0U];
}

CreativeEditorWorldLayoutElevationEditResult
planCreativeEditorWorldLayoutElevationEdit(
    const cr::CreativeWorldLayout& layout,
    const CreativeEditorWorldLayoutElevationProjection& projection,
    CreativeEditorWorldLayoutElevationHandle handle,
    double requestedVerticalCells,
    CreativeEditorWorldLayoutLevelEditScope levelScope) {
  CreativeEditorWorldLayoutElevationEditResult result;
  result.handle = handle;
  if (!projection.accepted ||
      handle.kind == CreativeEditorWorldLayoutElevationHandleKind::None ||
      handle.kind >= CreativeEditorWorldLayoutElevationHandleKind::Count ||
      !std::isfinite(requestedVerticalCells)) {
    result.reasonCode =
        "creative_editor_world_layout_elevation_edit_request_invalid";
    return result;
  }

  if (handle.sourceKind ==
      CreativeEditorWorldLayoutElevationSourceKind::Box) {
    if (handle.kind !=
            CreativeEditorWorldLayoutElevationHandleKind::LevelFloor ||
        handle.sourceIndex >= layout.boxes.size()) {
      result.reasonCode =
          "creative_editor_world_layout_elevation_edit_box_invalid";
      return result;
    }
    result.floorTopLayer = snapQuarterCell(requestedVerticalCells);
    if (!std::isfinite(result.floorTopLayer)) {
      result.reasonCode =
          "creative_editor_world_layout_elevation_floor_invalid";
      return result;
    }
  } else if (handle.sourceKind ==
             CreativeEditorWorldLayoutElevationSourceKind::Wall) {
    if (handle.kind != CreativeEditorWorldLayoutElevationHandleKind::WallTop ||
        handle.sourceIndex >= layout.walls.size()) {
      result.reasonCode =
          "creative_editor_world_layout_elevation_edit_wall_invalid";
      return result;
    }
    const cr::CreativeWorldLayoutWall& wall = layout.walls[handle.sourceIndex];
    const double height = std::round(requestedVerticalCells - wall.baseLayer);
    if (!std::isfinite(height) || height < 1.0 ||
        height > static_cast<double>(
                     std::numeric_limits<std::uint16_t>::max())) {
      result.reasonCode =
          "creative_editor_world_layout_elevation_wall_height_invalid";
      return result;
    }
    result.wallHeightCells = static_cast<std::uint16_t>(height);
  } else if (handle.sourceKind ==
      CreativeEditorWorldLayoutElevationSourceKind::Room) {
    if (!levelOwnsRoom(layout, handle.levelIndex, handle.sourceIndex)) {
      result.reasonCode =
          "creative_editor_world_layout_elevation_edit_room_invalid";
      return result;
    }
    const cr::CreativeWorldLayoutRoom& room = layout.rooms[handle.sourceIndex];
    const cr::CreativeWorldLayoutLevel& level = layout.levels[handle.levelIndex];
    result.floorTopLayer = level.floorTopLayer;
    result.wallHeightCells = level.wallHeightCells;
    result.roofPitchDegrees = level.roofPitchDegrees;
    if (handle.kind ==
        CreativeEditorWorldLayoutElevationHandleKind::LevelFloor) {
      result.levelDatumPlan =
          planCreativeEditorWorldLayoutLevelDatumEdit(
              layout, {handle.levelIndex, levelScope,
                       requestedVerticalCells});
      if (!result.levelDatumPlan.accepted) {
        result.reasonCode = result.levelDatumPlan.reasonCode;
        return result;
      }
      result.floorTopLayer =
          result.levelDatumPlan.snappedFloorTopLayer;
    } else if (handle.kind ==
               CreativeEditorWorldLayoutElevationHandleKind::WallTop) {
      const double height =
          std::round(requestedVerticalCells - level.floorTopLayer);
      if (!std::isfinite(height) || height < 1.0 ||
          height > static_cast<double>(
                       std::numeric_limits<std::uint16_t>::max())) {
        result.reasonCode =
            "creative_editor_world_layout_elevation_wall_height_invalid";
        return result;
      }
      for (std::size_t index = 0U; index < layout.levels.size(); ++index) {
        const cr::CreativeWorldLayoutLevel& other = layout.levels[index];
        if (index != handle.levelIndex &&
            other.buildingIndex == room.buildingIndex &&
            cr::creativeWorldLayoutLevelHasRooms(layout, index) &&
            other.floorTopLayer > level.floorTopLayer &&
            level.floorTopLayer + height >
                other.floorTopLayer + kGeometryEpsilon) {
          result.reasonCode =
              "creative_editor_world_layout_elevation_wall_crosses_level";
          return result;
        }
      }
      result.wallHeightCells = static_cast<std::uint16_t>(height);
    } else if (handle.kind ==
               CreativeEditorWorldLayoutElevationHandleKind::RoofRidge) {
      const CreativeEditorWorldLayoutRoofEditPlan roofEdit =
          planCreativeEditorWorldLayoutRoofEdit(
              layout, {},
              {handle.levelIndex,
               CreativeEditorWorldLayoutRoofHandleKind::RidgeHeight},
              requestedVerticalCells - handle.position.vertical);
      if (!roofEdit.accepted) {
        result.reasonCode = roofEdit.reasonCode;
        return result;
      }
      result.roofPitchDegrees = roofEdit.settings.roofPitchDegrees;
    } else {
      result.reasonCode =
          "creative_editor_world_layout_elevation_room_handle_invalid";
      return result;
    }
  } else if (handle.sourceKind ==
             CreativeEditorWorldLayoutElevationSourceKind::Opening) {
    if (handle.sourceIndex >= layout.openings.size()) {
      result.reasonCode =
          "creative_editor_world_layout_elevation_edit_opening_invalid";
      return result;
    }
    const cr::CreativeWorldLayoutOpening& opening =
        layout.openings[handle.sourceIndex];
    const OpeningHostFacts host = resolveOpeningHost(layout, opening);
    if (!host.valid) {
      result.reasonCode =
          "creative_editor_world_layout_elevation_edit_opening_host_invalid";
      return result;
    }
    result.openingSillCells = opening.cutoutBottomCells;
    result.openingHeightCells = opening.cutoutHeightCells;
    if (handle.kind ==
        CreativeEditorWorldLayoutElevationHandleKind::OpeningBottom) {
      if (opening.kind != cr::CreativeBuildingOpeningKind::Window) {
        result.reasonCode =
            "creative_editor_world_layout_elevation_door_sill_fixed";
        return result;
      }
      const double currentTop =
          opening.cutoutBottomCells + opening.cutoutHeightCells;
      result.openingSillCells = std::clamp(
          snapQuarterCell(requestedVerticalCells - host.baseLayer), 0.0,
          currentTop - kMinimumOpeningHeightCells);
      result.openingHeightCells = currentTop - result.openingSillCells;
    } else if (handle.kind ==
               CreativeEditorWorldLayoutElevationHandleKind::OpeningTop) {
      const double top = std::clamp(
          snapQuarterCell(requestedVerticalCells - host.baseLayer),
          opening.cutoutBottomCells + kMinimumOpeningHeightCells,
          host.heightCells);
      result.openingHeightCells = top - opening.cutoutBottomCells;
    } else {
      result.reasonCode =
          "creative_editor_world_layout_elevation_opening_handle_invalid";
      return result;
    }
  } else {
    result.reasonCode =
        "creative_editor_world_layout_elevation_edit_source_invalid";
    return result;
  }

  result.accepted = true;
  result.reasonCode = "creative_editor_world_layout_elevation_edit_ready";
  return result;
}


}  // namespace iggy3d_creative_app
