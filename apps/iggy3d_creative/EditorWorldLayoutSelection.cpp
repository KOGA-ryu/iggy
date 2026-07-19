#include "EditorWorldLayoutInternal.hpp"

#include "EditorWorldLayoutOpeningInternal.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace iggy3d_creative_app {

namespace {

constexpr double kSelectionHitToleranceCells = 0.35;

bool verticalConnectorOnLevel(
    const cr::CreativeWorldLayout& layout,
    const cr::CreativeWorldLayoutVerticalConnector& connector,
    std::size_t levelIndex) noexcept {
  if (levelIndex >= layout.levels.size() ||
      connector.lowerRoomIndex >= layout.rooms.size() ||
      connector.upperRoomIndex >= layout.rooms.size()) {
    return levelIndex >= layout.levels.size();
  }
  return layout.rooms[connector.lowerRoomIndex].levelIndex == levelIndex ||
         layout.rooms[connector.upperRoomIndex].levelIndex == levelIndex;
}


}  // namespace

using opening_detail::nearestOpeningHost;
using opening_detail::OpeningHostProjection;
using opening_detail::sameHost;

double distanceToSegment(CreativeEditorWorldLayoutPoint point,
                         cr::CreativeTerrainCoord2 start,
                         cr::CreativeTerrainCoord2 end) noexcept {
  const double dx = static_cast<double>(end.x) - start.x;
  const double dz = static_cast<double>(end.z) - start.z;
  const double lengthSquared = dx * dx + dz * dz;
  if (lengthSquared <= 0.0) {
    return std::hypot(point.x - start.x, point.z - start.z);
  }
  const double t = std::clamp(
      ((point.x - start.x) * dx + (point.z - start.z) * dz) /
          lengthSquared,
      0.0, 1.0);
  return std::hypot(point.x - (start.x + t * dx),
                    point.z - (start.z + t * dz));
}

CreativeEditorWorldLayoutSelection detail::hitTestWorldLayout(
    const cr::CreativeWorldLayout& layout,
    CreativeEditorWorldLayoutPoint point,
    std::size_t activeLevelIndex) {
  const OpeningHostProjection host =
      nearestOpeningHost(layout, point, kSelectionHitToleranceCells,
                         activeLevelIndex);
  if (host.hit) {
    for (std::size_t index = layout.openings.size(); index > 0U; --index) {
      const cr::CreativeWorldLayoutOpening& opening =
          layout.openings[index - 1U];
      if (!sameHost(opening, host)) {
        continue;
      }
      const double halfWidth = std::max(0.25, opening.widthCells * 0.5);
      if (std::fabs(opening.centerOffsetCells -
                    host.centerOffsetCells) <= halfWidth) {
        return {CreativeEditorWorldLayoutSelectionKind::Opening, index - 1U};
      }
    }
    if (host.hostKind == cr::CreativeWorldLayoutOpeningHostKind::Wall) {
      return {CreativeEditorWorldLayoutSelectionKind::Wall,
              host.wallIndex};
    }
    return {CreativeEditorWorldLayoutSelectionKind::Room, host.roomIndex};
  }
  for (std::size_t index = layout.objects.size(); index > 0U; --index) {
    const cr::CreativeWorldLayoutObject& object = layout.objects[index - 1U];
    const bool hit =
        object.mode == cr::CreativeObjectLibraryPlacementMode::Bounds
            ? point.x >= object.boundsCells.min.x &&
                  point.x <= object.boundsCells.max.x &&
                  point.z >= object.boundsCells.min.z &&
                  point.z <= object.boundsCells.max.z
            : std::hypot(point.x - object.pointCells.x,
                         point.z - object.pointCells.z) <= 0.6;
    if (hit) {
      return {CreativeEditorWorldLayoutSelectionKind::Object, index - 1U};
    }
  }
  for (std::size_t index = layout.verticalConnectors.size(); index > 0U;
       --index) {
    const cr::CreativeWorldLayoutVerticalConnector& connector =
        layout.verticalConnectors[index - 1U];
    if (!verticalConnectorOnLevel(layout, connector, activeLevelIndex)) {
      continue;
    }
    const cr::CreativeWorldLayoutRect& rect = connector.footprint;
    if (point.x >= rect.minimum.x && point.x <= rect.maximum.x &&
        point.z >= rect.minimum.z && point.z <= rect.maximum.z) {
      return {CreativeEditorWorldLayoutSelectionKind::VerticalConnector,
              index - 1U};
    }
  }
  for (std::size_t index = layout.rooms.size(); index > 0U; --index) {
    if (activeLevelIndex < layout.levels.size() &&
        layout.rooms[index - 1U].levelIndex != activeLevelIndex) {
      continue;
    }
    const cr::CreativeWorldLayoutRect& rect =
        layout.rooms[index - 1U].footprint;
    if (point.x >= rect.minimum.x && point.x <= rect.maximum.x &&
        point.z >= rect.minimum.z && point.z <= rect.maximum.z) {
      return {CreativeEditorWorldLayoutSelectionKind::Room, index - 1U};
    }
  }
  for (std::size_t index = layout.boxes.size(); index > 0U; --index) {
    const cr::CreativeWorldLayoutRect& rect =
        layout.boxes[index - 1U].footprint;
    if (point.x >= rect.minimum.x && point.x <= rect.maximum.x &&
        point.z >= rect.minimum.z && point.z <= rect.maximum.z) {
      return {CreativeEditorWorldLayoutSelectionKind::Box, index - 1U};
    }
  }
  for (std::size_t index = layout.terrainProfiles.size(); index > 0U; --index) {
    const cr::CreativeWorldLayoutTerrainProfile& profile =
        layout.terrainProfiles[index - 1U];
    if (std::hypot(point.x - profile.center.x, point.z - profile.center.z) <=
        0.65) {
      return {CreativeEditorWorldLayoutSelectionKind::TerrainProfile,
              index - 1U};
    }
  }
  for (std::size_t index = layout.terrainPaths.size(); index > 0U; --index) {
    const cr::CreativeWorldLayoutTerrainPath& path =
        layout.terrainPaths[index - 1U];
    if (path.pointCount < 2U ||
        path.firstPointIndex > layout.terrainPathPoints.size() ||
        path.pointCount >
            layout.terrainPathPoints.size() - path.firstPointIndex) {
      continue;
    }
    for (std::size_t pointIndex = path.firstPointIndex + 1U;
         pointIndex < path.firstPointIndex + path.pointCount; ++pointIndex) {
      if (distanceToSegment(
              point, layout.terrainPathPoints[pointIndex - 1U].coord,
              layout.terrainPathPoints[pointIndex].coord) <=
          std::max(0.65, static_cast<double>(path.halfWidthCells))) {
        return {CreativeEditorWorldLayoutSelectionKind::TerrainPath,
                index - 1U};
      }
    }
  }
  return {};
}

CreativeEditorWorldLayoutEditReceipt detail::selectWorldLayoutAtPoint(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point) {
  const CreativeEditorWorldLayoutSelection selected =
      hitTestWorldLayout(state.source, point, state.activeLevelIndex);
  const bool changed = selected.kind != state.selection.kind ||
                       selected.index != state.selection.index;
  state.selection = selected;
  if (selected.kind == CreativeEditorWorldLayoutSelectionKind::Room &&
      selected.index < state.source.rooms.size()) {
    state.activeLevelIndex = state.source.rooms[selected.index].levelIndex;
  } else if (selected.kind ==
                 CreativeEditorWorldLayoutSelectionKind::VerticalConnector &&
             selected.index < state.source.verticalConnectors.size()) {
    const cr::CreativeWorldLayoutVerticalConnector& connector =
        state.source.verticalConnectors[selected.index];
    if (state.activeLevelIndex >= state.source.levels.size() &&
        connector.lowerRoomIndex < state.source.rooms.size()) {
      state.activeLevelIndex =
          state.source.rooms[connector.lowerRoomIndex].levelIndex;
    }
  }
  state.anchorActive = false;
  detail::clearWorldLayoutInteraction(state);
  state.statusMessage =
      selected.kind == CreativeEditorWorldLayoutSelectionKind::None
          ? "selection cleared"
          : "layout symbol selected";
  return {true, changed, "creative_editor_world_layout_selected"};
}

}  // namespace iggy3d_creative_app
