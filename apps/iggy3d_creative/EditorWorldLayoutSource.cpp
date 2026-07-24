#include "EditorWorldLayout.hpp"

#include "EditorWorldLayoutInternal.hpp"

#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"
#include "app/iggy3d/creative/world/WorldLayoutSourceDuplication.hpp"
#include "app/iggy3d/creative/tools/SelectionResolution.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>
#include <utility>

namespace iggy3d_creative_app {
namespace {

[[nodiscard]] std::string* sourceName(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index) noexcept {
  switch (table) {
    case cr::CreativeWorldLayoutTable::Building:
      return index < state.source.buildings.size()
                 ? &state.source.buildings[index].name
                 : nullptr;
    case cr::CreativeWorldLayoutTable::Level:
      return index < state.source.levels.size()
                 ? &state.source.levels[index].name
                 : nullptr;
    case cr::CreativeWorldLayoutTable::Room:
      return index < state.source.rooms.size()
                 ? &state.source.rooms[index].name
                 : nullptr;
    case cr::CreativeWorldLayoutTable::VerticalConnector:
      return index < state.source.verticalConnectors.size()
                 ? &state.source.verticalConnectors[index].name
                 : nullptr;
    case cr::CreativeWorldLayoutTable::Box:
      return index < state.source.boxes.size()
                 ? &state.source.boxes[index].name
                 : nullptr;
    case cr::CreativeWorldLayoutTable::Wall:
      return index < state.source.walls.size()
                 ? &state.source.walls[index].name
                 : nullptr;
    case cr::CreativeWorldLayoutTable::Opening:
      return index < state.source.openings.size()
                 ? &state.source.openings[index].name
                 : nullptr;
    case cr::CreativeWorldLayoutTable::RoofAperture:
      return index < state.source.roofApertures.size()
                 ? &state.source.roofApertures[index].name
                 : nullptr;
    case cr::CreativeWorldLayoutTable::Object:
      return index < state.source.objects.size()
                 ? &state.source.objects[index].name
                 : nullptr;
    case cr::CreativeWorldLayoutTable::TopologyEdge:
      return nullptr;
    case cr::CreativeWorldLayoutTable::None:
    case cr::CreativeWorldLayoutTable::TerrainProfile:
    case cr::CreativeWorldLayoutTable::TerrainPath:
    case cr::CreativeWorldLayoutTable::TerrainPathPoint:
      return nullptr;
  }
  return nullptr;
}

[[nodiscard]] std::string_view sourceStableKey(
    const CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index) noexcept {
  switch (table) {
    case cr::CreativeWorldLayoutTable::Building:
      return index < state.source.buildings.size()
                 ? state.source.buildings[index].stableKey
                 : std::string_view{};
    case cr::CreativeWorldLayoutTable::Level:
      return index < state.source.levels.size()
                 ? state.source.levels[index].stableKey
                 : std::string_view{};
    case cr::CreativeWorldLayoutTable::Room:
      return index < state.source.rooms.size()
                 ? state.source.rooms[index].stableKey
                 : std::string_view{};
    case cr::CreativeWorldLayoutTable::VerticalConnector:
      return index < state.source.verticalConnectors.size()
                 ? state.source.verticalConnectors[index].stableKey
                 : std::string_view{};
    case cr::CreativeWorldLayoutTable::Box:
      return index < state.source.boxes.size()
                 ? state.source.boxes[index].stableKey
                 : std::string_view{};
    case cr::CreativeWorldLayoutTable::Wall:
      return index < state.source.walls.size()
                 ? state.source.walls[index].stableKey
                 : std::string_view{};
    case cr::CreativeWorldLayoutTable::Opening:
      return index < state.source.openings.size()
                 ? state.source.openings[index].stableKey
                 : std::string_view{};
    case cr::CreativeWorldLayoutTable::RoofAperture:
      return index < state.source.roofApertures.size()
                 ? state.source.roofApertures[index].stableKey
                 : std::string_view{};
    case cr::CreativeWorldLayoutTable::Object:
      return index < state.source.objects.size()
                 ? state.source.objects[index].stableKey
                 : std::string_view{};
    case cr::CreativeWorldLayoutTable::TerrainProfile:
      return index < state.source.terrainProfiles.size()
                 ? state.source.terrainProfiles[index].stableKey
                 : std::string_view{};
    case cr::CreativeWorldLayoutTable::TerrainPath:
      return index < state.source.terrainPaths.size()
                 ? state.source.terrainPaths[index].stableKey
                 : std::string_view{};
    case cr::CreativeWorldLayoutTable::TopologyEdge:
      return index < state.source.topologyEdges.size()
                 ? state.source.topologyEdges[index].stableKey
                 : std::string_view{};
    case cr::CreativeWorldLayoutTable::None:
    case cr::CreativeWorldLayoutTable::TerrainPathPoint:
      return {};
  }
  return {};
}

struct SourceTarget {
  bool valid = false;
  CreativeEditorWorldLayoutSelection selection;
  std::size_t activeLevelIndex = cr::kInvalidCreativeWorldLayoutIndex;
  std::size_t buildingIndex = cr::kInvalidCreativeWorldLayoutIndex;
  bool hasCenter = false;
  CreativeEditorWorldLayoutPoint center;
};

CreativeEditorWorldLayoutPoint rectCenter(cr::CreativeWorldLayoutRect value) {
  return {(static_cast<double>(value.minimum.x) + value.maximum.x) * 0.5,
          (static_cast<double>(value.minimum.z) + value.maximum.z) * 0.5};
}

bool resolveBuildingCenter(const CreativeEditorWorldLayoutState& state,
                           std::size_t buildingIndex,
                           CreativeEditorWorldLayoutPoint& center) noexcept {
  cr::CreativeWorldLayoutBuildingBounds bounds;
  if (!cr::measureCreativeWorldLayoutBuildingBounds(state.source,
                                                     buildingIndex, bounds)) {
    return false;
  }
  center = {(static_cast<double>(bounds.minimum.x) + bounds.maximum.x) * 0.5,
            (static_cast<double>(bounds.minimum.z) + bounds.maximum.z) * 0.5};
  return true;
}

SourceTarget resolveSourceTarget(
    const CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index) {
  SourceTarget target;
  const cr::CreativeWorldLayout& source = state.source;
  switch (table) {
    case cr::CreativeWorldLayoutTable::Building:
      if (index >= source.buildings.size()) return target;
      target.selection = {CreativeEditorWorldLayoutSelectionKind::Building,
                          index};
      target.buildingIndex = index;
      target.hasCenter = resolveBuildingCenter(state, index, target.center);
      break;
    case cr::CreativeWorldLayoutTable::Level:
      if (index >= source.levels.size()) return target;
      target.activeLevelIndex = index;
      target.buildingIndex = source.levels[index].buildingIndex;
      if (target.buildingIndex >= source.buildings.size()) return target;
      target.selection = {CreativeEditorWorldLayoutSelectionKind::Level,
                          index};
      target.hasCenter =
          resolveBuildingCenter(state, target.buildingIndex, target.center);
      break;
    case cr::CreativeWorldLayoutTable::Room:
      if (index >= source.rooms.size()) return target;
      target.selection = {CreativeEditorWorldLayoutSelectionKind::Room, index};
      target.activeLevelIndex = source.rooms[index].levelIndex;
      target.buildingIndex = source.rooms[index].buildingIndex;
      target.center = rectCenter(source.rooms[index].footprint);
      target.hasCenter = true;
      break;
    case cr::CreativeWorldLayoutTable::VerticalConnector:
      if (index >= source.verticalConnectors.size()) return target;
      target.selection = {
          CreativeEditorWorldLayoutSelectionKind::VerticalConnector, index};
      target.buildingIndex = source.verticalConnectors[index].buildingIndex;
      target.center = rectCenter(source.verticalConnectors[index].footprint);
      target.hasCenter = true;
      if (source.verticalConnectors[index].lowerRoomIndex < source.rooms.size()) {
        target.activeLevelIndex =
            source.rooms[source.verticalConnectors[index].lowerRoomIndex]
                .levelIndex;
      }
      break;
    case cr::CreativeWorldLayoutTable::Box:
      if (index >= source.boxes.size()) return target;
      target.selection = {CreativeEditorWorldLayoutSelectionKind::Box, index};
      target.buildingIndex = source.boxes[index].buildingIndex;
      target.center = rectCenter(source.boxes[index].footprint);
      target.hasCenter = true;
      break;
    case cr::CreativeWorldLayoutTable::Wall:
      if (index >= source.walls.size()) return target;
      target.selection = {CreativeEditorWorldLayoutSelectionKind::Wall, index};
      target.buildingIndex = source.walls[index].buildingIndex;
      target.center = {
          (static_cast<double>(source.walls[index].start.x) +
           source.walls[index].end.x) *
              0.5,
          (static_cast<double>(source.walls[index].start.z) +
           source.walls[index].end.z) *
              0.5};
      target.hasCenter = true;
      break;
    case cr::CreativeWorldLayoutTable::Opening: {
      if (index >= source.openings.size()) return target;
      target.selection = {CreativeEditorWorldLayoutSelectionKind::Opening,
                          index};
      const cr::CreativeWorldLayoutOpening& opening = source.openings[index];
      const CreativeEditorWorldLayoutOpeningHost host =
          resolveCreativeEditorWorldLayoutOpeningHost(state, index);
      if (host.valid && host.lengthCells > 0.0) {
        const double t = std::clamp(opening.centerOffsetCells /
                                        host.lengthCells,
                                    0.0, 1.0);
        target.center = {host.start.x + (host.end.x - host.start.x) * t,
                         host.start.z + (host.end.z - host.start.z) * t};
        target.hasCenter = true;
      }
      if (opening.hostKind == cr::CreativeWorldLayoutOpeningHostKind::Wall &&
          opening.wallIndex < source.walls.size()) {
        target.buildingIndex =
            source.walls[opening.wallIndex].buildingIndex;
      } else if (opening.hostKind ==
                     cr::CreativeWorldLayoutOpeningHostKind::RoomEdge &&
                 opening.roomIndex < source.rooms.size()) {
        target.buildingIndex =
            source.rooms[opening.roomIndex].buildingIndex;
        target.activeLevelIndex =
            source.rooms[opening.roomIndex].levelIndex;
      }
      break;
    }
    case cr::CreativeWorldLayoutTable::RoofAperture: {
      if (index >= source.roofApertures.size()) return target;
      const cr::CreativeWorldLayoutRoofAperture& aperture =
          source.roofApertures[index];
      if (aperture.levelIndex >= source.levels.size()) return target;
      target.selection = {
          CreativeEditorWorldLayoutSelectionKind::RoofAperture, index};
      target.activeLevelIndex = aperture.levelIndex;
      target.buildingIndex = source.levels[aperture.levelIndex].buildingIndex;
      target.center = {(aperture.minimumXCells + aperture.maximumXCells) * 0.5,
                       (aperture.minimumZCells + aperture.maximumZCells) * 0.5};
      target.hasCenter = std::isfinite(target.center.x) &&
                         std::isfinite(target.center.z);
      break;
    }
    case cr::CreativeWorldLayoutTable::Object: {
      if (index >= source.objects.size()) return target;
      target.selection = {CreativeEditorWorldLayoutSelectionKind::Object,
                          index};
      const cr::CreativeWorldLayoutObject& object = source.objects[index];
      target.center =
          object.mode == cr::CreativeObjectLibraryPlacementMode::Bounds
              ? CreativeEditorWorldLayoutPoint{
                    (object.boundsCells.min.x + object.boundsCells.max.x) * 0.5,
                    (object.boundsCells.min.z + object.boundsCells.max.z) * 0.5}
              : CreativeEditorWorldLayoutPoint{object.pointCells.x,
                                               object.pointCells.z};
      target.hasCenter = std::isfinite(target.center.x) &&
                         std::isfinite(target.center.z);
      break;
    }
    case cr::CreativeWorldLayoutTable::TopologyEdge: {
      if (index >= source.topologyEdges.size()) return target;
      const cr::CreativeWorldLayoutTopologyEdge& edge =
          source.topologyEdges[index];
      if (edge.levelIndex >= source.levels.size() ||
          edge.startVertexIndex >= source.topologyVertices.size() ||
          edge.endVertexIndex >= source.topologyVertices.size()) {
        return target;
      }
      target.selection = {
          CreativeEditorWorldLayoutSelectionKind::TopologyEdge, index};
      target.activeLevelIndex = edge.levelIndex;
      target.buildingIndex = source.levels[edge.levelIndex].buildingIndex;
      const cr::CreativeTerrainCoord2 start =
          source.topologyVertices[edge.startVertexIndex].position;
      const cr::CreativeTerrainCoord2 end =
          source.topologyVertices[edge.endVertexIndex].position;
      target.center = {
          (static_cast<double>(start.x) + end.x) * 0.5,
          (static_cast<double>(start.z) + end.z) * 0.5};
      target.hasCenter = true;
      break;
    }
    case cr::CreativeWorldLayoutTable::TerrainProfile:
      if (index >= source.terrainProfiles.size()) return target;
      target.selection = {
          CreativeEditorWorldLayoutSelectionKind::TerrainProfile, index};
      if (source.terrainProfiles[index].usesLandformRecipe) {
        const cr::CreativeTerrainHeightFieldBounds& bounds =
            source.terrainProfiles[index].landform.bounds;
        target.center = {
            static_cast<double>(bounds.minimum.x) + bounds.widthCells * 0.5,
            static_cast<double>(bounds.minimum.z) + bounds.depthCells * 0.5};
      } else {
        target.center = {
            static_cast<double>(source.terrainProfiles[index].center.x),
            static_cast<double>(source.terrainProfiles[index].center.z)};
      }
      target.hasCenter = true;
      break;
    case cr::CreativeWorldLayoutTable::TerrainPath: {
      if (index >= source.terrainPaths.size()) return target;
      const cr::CreativeWorldLayoutTerrainPath& path =
          source.terrainPaths[index];
      if (!cr::isValidCreativeTerrainPathSourceRecipe(path.recipe)) {
        return target;
      }
      target.selection = {
          CreativeEditorWorldLayoutSelectionKind::TerrainPath, index};
      double sumX = 0.0;
      double sumZ = 0.0;
      for (const cr::CreativeTerrainPathSourcePoint& point :
           path.recipe.points) {
        sumX += point.coord.x;
        sumZ += point.coord.z;
      }
      const double pointCount =
          static_cast<double>(path.recipe.points.size());
      target.center = {sumX / pointCount, sumZ / pointCount};
      target.hasCenter = true;
      break;
    }
    case cr::CreativeWorldLayoutTable::TerrainPathPoint: {
      std::size_t flattenedIndex = index;
      for (std::size_t pathIndex = 0U;
           pathIndex < source.terrainPaths.size(); ++pathIndex) {
        const auto& points = source.terrainPaths[pathIndex].recipe.points;
        if (flattenedIndex < points.size()) {
          target.selection = {
              CreativeEditorWorldLayoutSelectionKind::TerrainPath,
              pathIndex};
          target.center = {
              static_cast<double>(points[flattenedIndex].coord.x),
              static_cast<double>(points[flattenedIndex].coord.z)};
          target.hasCenter = true;
          break;
        }
        flattenedIndex -= points.size();
      }
      if (!target.hasCenter) return target;
      break;
    }
    case cr::CreativeWorldLayoutTable::None:
      return target;
  }
  target.valid = true;
  return target;
}

CreativeEditorWorldLayoutEditReceipt applySourceTarget(
    CreativeEditorWorldLayoutState& state,
    const SourceTarget& target,
    bool centerView) {
  if (!target.valid) {
    state.statusMessage = "layout source has no selectable symbol";
    return {false, false,
            "creative_editor_world_layout_source_target_invalid"};
  }
  const bool changed = state.tool != CreativeEditorWorldLayoutTool::Select ||
                       state.selection.kind != target.selection.kind ||
                       state.selection.index != target.selection.index ||
                       (target.activeLevelIndex !=
                            cr::kInvalidCreativeWorldLayoutIndex &&
                        state.activeLevelIndex != target.activeLevelIndex);
  if (!changed && !centerView) {
    state.statusMessage = "layout source scope selected";
    return {true, false,
            "creative_editor_world_layout_source_scope_selected"};
  }
  detail::clearWorldLayoutInteraction(state);
  state.anchorActive = false;
  state.tool = CreativeEditorWorldLayoutTool::Select;
  state.selection = target.selection;
  if (target.activeLevelIndex < state.source.levels.size()) {
    state.activeLevelIndex = target.activeLevelIndex;
  } else if (target.buildingIndex < state.source.buildings.size()) {
    repairCreativeEditorWorldLayoutActiveLevel(state, target.buildingIndex);
  }
  if (centerView && target.hasCenter) {
    state.canvasPanX =
        -static_cast<float>(target.center.x) * state.canvasPixelsPerCell;
    state.canvasPanZ =
        -static_cast<float>(target.center.z) * state.canvasPixelsPerCell;
    const double horizontal =
        state.elevationAxis == CreativeEditorWorldLayoutElevationAxis::X
            ? target.center.x
            : target.center.z;
    state.elevationPanHorizontal =
        -static_cast<float>(horizontal) * state.elevationPixelsPerCell;
  }
  state.statusMessage = centerView ? "layout source selected"
                                   : "layout source scope selected";
  return {true, changed,
          centerView
              ? "creative_editor_world_layout_source_target_focused"
              : "creative_editor_world_layout_source_scope_selected"};
}

}  // namespace

std::string_view creativeEditorWorldLayoutSourceStableKey(
    const CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index) noexcept {
  return sourceStableKey(state, table, index);
}

cr::CreativeWorldLayoutTable creativeEditorWorldLayoutSelectionTable(
    CreativeEditorWorldLayoutSelectionKind kind) noexcept {
  switch (kind) {
    case CreativeEditorWorldLayoutSelectionKind::Building:
      return cr::CreativeWorldLayoutTable::Building;
    case CreativeEditorWorldLayoutSelectionKind::Level:
      return cr::CreativeWorldLayoutTable::Level;
    case CreativeEditorWorldLayoutSelectionKind::Room:
      return cr::CreativeWorldLayoutTable::Room;
    case CreativeEditorWorldLayoutSelectionKind::VerticalConnector:
      return cr::CreativeWorldLayoutTable::VerticalConnector;
    case CreativeEditorWorldLayoutSelectionKind::Box:
      return cr::CreativeWorldLayoutTable::Box;
    case CreativeEditorWorldLayoutSelectionKind::Wall:
      return cr::CreativeWorldLayoutTable::Wall;
    case CreativeEditorWorldLayoutSelectionKind::Opening:
      return cr::CreativeWorldLayoutTable::Opening;
    case CreativeEditorWorldLayoutSelectionKind::RoofAperture:
      return cr::CreativeWorldLayoutTable::RoofAperture;
    case CreativeEditorWorldLayoutSelectionKind::TerrainProfile:
      return cr::CreativeWorldLayoutTable::TerrainProfile;
    case CreativeEditorWorldLayoutSelectionKind::TerrainPath:
      return cr::CreativeWorldLayoutTable::TerrainPath;
    case CreativeEditorWorldLayoutSelectionKind::Object:
      return cr::CreativeWorldLayoutTable::Object;
    case CreativeEditorWorldLayoutSelectionKind::TopologyEdge:
      return cr::CreativeWorldLayoutTable::TopologyEdge;
    case CreativeEditorWorldLayoutSelectionKind::None:
      return cr::CreativeWorldLayoutTable::None;
  }
  return cr::CreativeWorldLayoutTable::None;
}

bool creativeEditorWorldLayoutSourceCanRename(
    cr::CreativeWorldLayoutTable table) noexcept {
  return cr::resolveCreativeSemanticObjectAction(
             cr::CreativeSemanticSelectionOwner::WorldLayoutSource,
             cr::CreativeSemanticObjectAction::Rename, table, 1U)
      .allowed;
}

bool creativeEditorWorldLayoutSourceCanDuplicate(
    cr::CreativeWorldLayoutTable table) noexcept {
  return cr::resolveCreativeSemanticObjectAction(
             cr::CreativeSemanticSelectionOwner::WorldLayoutSource,
             cr::CreativeSemanticObjectAction::Duplicate, table, 1U)
      .allowed;
}

bool creativeEditorWorldLayoutSourceCanDelete(
    cr::CreativeWorldLayoutTable table) noexcept {
  return cr::resolveCreativeSemanticObjectAction(
             cr::CreativeSemanticSelectionOwner::WorldLayoutSource,
             cr::CreativeSemanticObjectAction::Delete, table, 1U)
      .allowed;
}

bool creativeEditorWorldLayoutSourceCanSetVisible(
    cr::CreativeWorldLayoutTable table) noexcept {
  return cr::resolveCreativeSemanticObjectAction(
             cr::CreativeSemanticSelectionOwner::WorldLayoutSource,
             cr::CreativeSemanticObjectAction::SetVisible, table, 1U)
      .allowed;
}

bool creativeEditorWorldLayoutSourceStableKeyMatches(
    const CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index,
    std::string_view stableKey) noexcept {
  return !stableKey.empty() && sourceStableKey(state, table, index) == stableKey;
}

CreativeEditorWorldLayoutEditReceipt renameCreativeEditorWorldLayoutSource(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index,
    std::string name) {
  std::string* current = sourceName(state, table, index);
  if (current == nullptr || !detail::hasVisibleWorldLayoutName(name)) {
    state.statusMessage = "source name is invalid";
    return {false, false,
            "creative_editor_world_layout_source_rename_invalid"};
  }
  if (*current == name) {
    state.statusMessage = "source name unchanged";
    return {true, false,
            "creative_editor_world_layout_source_rename_no_change"};
  }
  *current = std::move(name);
  detail::noteWorldLayoutSourceChange(state, "layout source renamed");
  return {true, true, "creative_editor_world_layout_source_renamed"};
}

CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutSourceVisible(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index,
    bool visible) {
  bool* current = nullptr;
  if (table == cr::CreativeWorldLayoutTable::Building &&
      index < state.source.buildings.size()) {
    current = &state.source.buildings[index].visible;
  } else if (table == cr::CreativeWorldLayoutTable::Object &&
             index < state.source.objects.size()) {
    current = &state.source.objects[index].visible;
  }
  if (current == nullptr) {
    state.statusMessage = "source visibility is not editable";
    return {false, false,
            "creative_editor_world_layout_source_visibility_unsupported"};
  }
  if (*current == visible) {
    state.statusMessage = "source visibility unchanged";
    return {true, false,
            "creative_editor_world_layout_source_visibility_no_change"};
  }
  *current = visible;
  detail::noteWorldLayoutSourceChange(
      state, visible ? "layout source shown" : "layout source hidden");
  return {true, true,
          "creative_editor_world_layout_source_visibility_updated"};
}

CreativeEditorWorldLayoutEditReceipt
rotateCreativeEditorWorldLayoutBuildingSource(
    CreativeEditorWorldLayoutState& state, std::size_t buildingIndex,
    cr::CreativeWorldLayoutBuildingTransformOperation operation) {
  cr::CreativeWorldLayoutBuildingTransformResult transformed =
      cr::transformCreativeWorldLayoutBuilding(
          state.source, {buildingIndex, operation});
  if (!transformed.accepted) {
    state.statusMessage = std::string(transformed.reasonCode);
    return {false, false, transformed.reasonCode};
  }
  state.source = std::move(transformed.transformed);
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Building,
                     transformed.buildingIndex};
  repairCreativeEditorWorldLayoutActiveLevel(state,
                                             transformed.buildingIndex);
  detail::noteWorldLayoutSourceChange(state, "building rotated");
  return {true, true,
          "creative_editor_world_layout_building_rotated"};
}

CreativeEditorWorldLayoutEditReceipt duplicateCreativeEditorWorldLayoutSource(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index) {
  return duplicateCreativeEditorWorldLayoutSource(state, table, index, {});
}

CreativeEditorWorldLayoutEditReceipt duplicateCreativeEditorWorldLayoutSource(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index,
    cr::CreativeGridSettings grid) {
  if (table == cr::CreativeWorldLayoutTable::Building) {
    std::int64_t deltaX = 0;
    std::int64_t deltaZ = 0;
    if (!defaultCreativeEditorWorldLayoutBuildingDuplicateOffset(
            state, index, deltaX, deltaZ)) {
      state.statusMessage = "building has no valid duplicate offset";
      return {false, false,
              "creative_editor_world_layout_source_duplicate_invalid"};
    }
    return duplicateCreativeEditorWorldLayoutBuilding(state, index, deltaX,
                                                       deltaZ);
  }
  if (table == cr::CreativeWorldLayoutTable::Level) {
    return applyCreativeEditorWorldLayoutLevelOperation(
        state, CreativeEditorWorldLayoutLevelOperation::Duplicate,
        cr::kInvalidCreativeWorldLayoutIndex, index);
  }
  cr::CreativeWorldLayoutSourceDuplicateResult duplicated =
      cr::duplicateCreativeWorldLayoutSource(
          state.source, {table, index, grid, state.nextStableOrdinal});
  if (!duplicated.accepted || !duplicated.changed) {
    switch (duplicated.status) {
      case cr::CreativeWorldLayoutSourceDuplicateStatus::NoValidPlacement:
        state.statusMessage = "source has no valid duplicate location";
        break;
      case cr::CreativeWorldLayoutSourceDuplicateStatus::CoordinateOverflow:
        state.statusMessage = "source duplicate exceeds the coordinate range";
        break;
      case cr::CreativeWorldLayoutSourceDuplicateStatus::InvalidSource:
        state.statusMessage = "source is invalid and cannot be duplicated";
        break;
      case cr::CreativeWorldLayoutSourceDuplicateStatus::NotRequested:
      case cr::CreativeWorldLayoutSourceDuplicateStatus::InvalidRequest:
      case cr::CreativeWorldLayoutSourceDuplicateStatus::UnsupportedSource:
      case cr::CreativeWorldLayoutSourceDuplicateStatus::Ready:
        state.statusMessage = "source type cannot be duplicated";
        break;
    }
    return {false, false, duplicated.reasonCode};
  }
  cr::CreativeWorldLayout previousSource = std::move(state.source);
  const std::uint64_t previousNextStableOrdinal = state.nextStableOrdinal;
  const CreativeEditorWorldLayoutSelection previousSelection =
      state.selection;
  const std::size_t previousActiveLevelIndex = state.activeLevelIndex;
  const CreativeEditorWorldLayoutTool previousTool = state.tool;
  state.source = std::move(duplicated.edited);
  state.nextStableOrdinal = duplicated.nextStableOrdinal;
  const CreativeEditorWorldLayoutEditReceipt selected =
      selectCreativeEditorWorldLayoutSource(
          state, duplicated.duplicate.table, duplicated.duplicate.index);
  if (!selected.accepted) {
    state.source = std::move(previousSource);
    state.nextStableOrdinal = previousNextStableOrdinal;
    state.selection = previousSelection;
    state.activeLevelIndex = previousActiveLevelIndex;
    state.tool = previousTool;
    state.statusMessage = "duplicated source cannot be selected";
    return {false, false,
            "creative_editor_world_layout_source_duplicate_selection_invalid"};
  }
  const CreativeEditorWorldLayoutSelection duplicateSelection =
      state.selection;
  const std::size_t duplicateActiveLevelIndex = state.activeLevelIndex;
  state.selection = previousSelection;
  state.activeLevelIndex = previousActiveLevelIndex;
  state.tool = previousTool;
  detail::noteWorldLayoutSourceChange(state, "layout source duplicated");
  state.selection = duplicateSelection;
  state.activeLevelIndex = duplicateActiveLevelIndex;
  state.tool = CreativeEditorWorldLayoutTool::Select;
  return {true, true,
          "creative_editor_world_layout_source_duplicated"};
}

CreativeEditorWorldLayoutEditReceipt deleteCreativeEditorWorldLayoutSource(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index) {
  if (table == cr::CreativeWorldLayoutTable::Building) {
    return deleteCreativeEditorWorldLayoutBuilding(state, index);
  }
  if (table == cr::CreativeWorldLayoutTable::Level) {
    return applyCreativeEditorWorldLayoutLevelOperation(
        state, CreativeEditorWorldLayoutLevelOperation::Delete,
        cr::kInvalidCreativeWorldLayoutIndex, index);
  }
  if (!creativeEditorWorldLayoutSourceCanDelete(table)) {
    state.statusMessage = "source type cannot be deleted";
    return {false, false,
            "creative_editor_world_layout_source_delete_unsupported"};
  }
  const CreativeEditorWorldLayoutEditReceipt focused =
      focusCreativeEditorWorldLayoutSource(state, table, index);
  if (!focused.accepted) {
    return focused;
  }
  return deleteCreativeEditorWorldLayoutSelection(state);
}

CreativeEditorWorldLayoutEditReceipt
selectCreativeEditorWorldLayoutSource(
    CreativeEditorWorldLayoutState& state, cr::CreativeWorldLayoutTable table,
    std::size_t index, std::size_t preferredLevelIndex) {
  SourceTarget target = resolveSourceTarget(state, table, index);
  if (preferredLevelIndex != cr::kInvalidCreativeWorldLayoutIndex) {
    if (!target.valid || !cr::creativeWorldLayoutSourceTouchesLevel(
                             state.source, table, index,
                             preferredLevelIndex)) {
      state.statusMessage = "layout source is not on the requested floor";
      return {false, false,
              "creative_editor_world_layout_source_level_mismatch"};
    }
    target.activeLevelIndex = preferredLevelIndex;
  }
  return applySourceTarget(state, target, false);
}

CreativeEditorWorldLayoutEditReceipt
focusCreativeEditorWorldLayoutSource(
    CreativeEditorWorldLayoutState& state, cr::CreativeWorldLayoutTable table,
    std::size_t index) {
  return applySourceTarget(state, resolveSourceTarget(state, table, index),
                           true);
}

}  // namespace iggy3d_creative_app
