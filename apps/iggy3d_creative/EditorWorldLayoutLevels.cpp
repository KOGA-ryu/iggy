#include "EditorWorldLayout.hpp"

#include "EditorWorldLayoutInternal.hpp"

#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"
#include "app/iggy3d/creative/world/WorldLayoutOrthogonalRooms.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRoomTopology.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace iggy3d_creative_app {
namespace {

[[nodiscard]] bool validLevelMutationCandidate(
    const cr::CreativeWorldLayout& layout) {
  if (!cr::validCreativeWorldLayoutLevelOwnership(layout)) {
    return false;
  }
  if (layout.rooms.empty()) {
    return layout.topologyVertices.empty() && layout.topologyEdges.empty() &&
           layout.roomBoundaries.empty();
  }
  return cr::buildCreativeWorldLayoutRoomGraph(layout).accepted;
}

[[nodiscard]] bool explicitRoomTopology(
    const cr::CreativeWorldLayout& layout) noexcept {
  return !layout.topologyVertices.empty() || !layout.topologyEdges.empty() ||
         !layout.roomBoundaries.empty();
}

void rejectInvalidLevelMutation(CreativeEditorWorldLayoutState& state,
                                std::string_view action) {
  state.statusMessage = std::string(action) +
                        " would invalidate the building floor plan";
}

[[nodiscard]] std::size_t firstLevelForBuilding(
    const cr::CreativeWorldLayout& layout,
    std::size_t buildingIndex) noexcept {
  for (std::size_t index = 0U; index < layout.levels.size(); ++index) {
    if (layout.levels[index].buildingIndex == buildingIndex) {
      return index;
    }
  }
  return cr::kInvalidCreativeWorldLayoutIndex;
}

[[nodiscard]] std::size_t levelCountForBuilding(
    const cr::CreativeWorldLayout& layout,
    std::size_t buildingIndex) noexcept {
  return static_cast<std::size_t>(std::count_if(
      layout.levels.begin(), layout.levels.end(),
      [buildingIndex](const cr::CreativeWorldLayoutLevel& level) {
        return level.buildingIndex == buildingIndex;
      }));
}

[[nodiscard]] std::size_t resolvedLevelIndex(
    const CreativeEditorWorldLayoutState& state,
    std::size_t requested) noexcept {
  if (requested < state.source.levels.size()) {
    return requested;
  }
  return state.activeLevelIndex < state.source.levels.size()
             ? state.activeLevelIndex
             : cr::kInvalidCreativeWorldLayoutIndex;
}

[[nodiscard]] bool nextLevelElevation(
    const cr::CreativeWorldLayout& layout, std::size_t buildingIndex,
    double& output) noexcept {
  if (buildingIndex >= layout.buildings.size()) {
    return false;
  }
  bool foundHighest = false;
  bool foundPrevious = false;
  long double highest = 0.0L;
  long double previous = 0.0L;
  const cr::CreativeWorldLayoutLevel* highestLevel = nullptr;
  for (const cr::CreativeWorldLayoutLevel& level : layout.levels) {
    if (level.buildingIndex != buildingIndex ||
        !std::isfinite(level.floorTopLayer) || level.wallHeightCells == 0U) {
      continue;
    }
    const long double floorTop =
        static_cast<long double>(level.floorTopLayer);
    if (!foundHighest || floorTop > highest) {
      if (foundHighest) {
        previous = highest;
        foundPrevious = true;
      }
      highest = floorTop;
      highestLevel = &level;
      foundHighest = true;
    } else if (!foundPrevious || floorTop > previous) {
      previous = floorTop;
      foundPrevious = true;
    }
  }
  if (!foundHighest || highestLevel == nullptr) {
    return false;
  }
  long double floorToFloor = 0.0L;
  if (foundPrevious) {
    floorToFloor = highest - previous;
  } else {
    const std::uint16_t buildingDefault =
        layout.buildings[buildingIndex].rootHeightCells;
    floorToFloor =
        buildingDefault > 0U ? static_cast<long double>(buildingDefault)
                             : highestLevel->wallHeightCells;
  }
  const long double next = highest + floorToFloor;
  if (!std::isfinite(next) || !std::isfinite(floorToFloor) ||
      floorToFloor <= 0.0L ||
      next < -static_cast<long double>(std::numeric_limits<double>::max()) ||
      next > static_cast<long double>(std::numeric_limits<double>::max())) {
    return false;
  }
  output = static_cast<double>(next);
  return std::isfinite(output);
}

void selectLevelOwner(CreativeEditorWorldLayoutState& state,
                      std::size_t levelIndex) {
  bool selectionVisible = false;
  if (state.selection.kind == CreativeEditorWorldLayoutSelectionKind::Room &&
      state.selection.index < state.source.rooms.size()) {
    selectionVisible =
        state.source.rooms[state.selection.index].levelIndex == levelIndex;
  } else if (state.selection.kind ==
                 CreativeEditorWorldLayoutSelectionKind::Opening &&
             state.selection.index < state.source.openings.size()) {
    const cr::CreativeWorldLayoutOpening& opening =
        state.source.openings[state.selection.index];
    selectionVisible =
        opening.hostKind != cr::CreativeWorldLayoutOpeningHostKind::RoomEdge ||
        (opening.roomIndex < state.source.rooms.size() &&
         state.source.rooms[opening.roomIndex].levelIndex == levelIndex);
  } else if (state.selection.kind ==
                 CreativeEditorWorldLayoutSelectionKind::VerticalConnector &&
             state.selection.index < state.source.verticalConnectors.size()) {
    const cr::CreativeWorldLayoutVerticalConnector& connector =
        state.source.verticalConnectors[state.selection.index];
    selectionVisible =
        (connector.lowerRoomIndex < state.source.rooms.size() &&
         state.source.rooms[connector.lowerRoomIndex].levelIndex ==
             levelIndex) ||
        (connector.upperRoomIndex < state.source.rooms.size() &&
         state.source.rooms[connector.upperRoomIndex].levelIndex == levelIndex);
  } else if (state.selection.kind ==
                 CreativeEditorWorldLayoutSelectionKind::RoofAperture &&
             state.selection.index < state.source.roofApertures.size()) {
    selectionVisible =
        state.source.roofApertures[state.selection.index].levelIndex ==
        levelIndex;
  }
  if (!selectionVisible) {
    state.selection = {CreativeEditorWorldLayoutSelectionKind::Level,
                       levelIndex};
  }
  state.activeLevelIndex = levelIndex;
}

[[nodiscard]] CreativeEditorWorldLayoutEditReceipt selectLevel(
    CreativeEditorWorldLayoutState& state, std::size_t levelIndex) {
  if (levelIndex >= state.source.levels.size()) {
    state.statusMessage = "building level is unavailable";
    return {false, false,
            "creative_editor_world_layout_level_selection_invalid"};
  }
  const std::size_t previousLevelIndex = state.activeLevelIndex;
  const CreativeEditorWorldLayoutSelection previousSelection = state.selection;
  detail::clearWorldLayoutInteraction(state);
  state.anchorActive = false;
  selectLevelOwner(state, levelIndex);
  const bool selectionChanged =
      previousSelection.kind != state.selection.kind ||
      previousSelection.index != state.selection.index;
  state.statusMessage = state.source.levels[levelIndex].name + " active";
  return {true, previousLevelIndex != state.activeLevelIndex || selectionChanged,
          "creative_editor_world_layout_level_selected"};
}

[[nodiscard]] CreativeEditorWorldLayoutEditReceipt addLevel(
    CreativeEditorWorldLayoutState& state, std::size_t buildingIndex) {
  if (buildingIndex >= state.source.buildings.size()) {
    state.statusMessage = "select a building before adding a level";
    return {false, false,
            "creative_editor_world_layout_level_building_invalid"};
  }
  const std::size_t first = firstLevelForBuilding(state.source, buildingIndex);
  if (first == cr::kInvalidCreativeWorldLayoutIndex) {
    const cr::CreativeWorldLayoutBuilding& building =
        state.source.buildings[buildingIndex];
    cr::CreativeWorldLayoutLevel level;
    level.buildingIndex = buildingIndex;
    level.stableKey = detail::mintWorldLayoutStableKey(state, "level");
    level.name = "Level 0";
    level.floorTopLayer = static_cast<double>(building.rootBaseLayer);
    level.wallHeightCells =
        building.rootHeightCells > 0U
            ? building.rootHeightCells
            : cr::kDefaultCreativeWorldLayoutWallHeightCells;
    state.source.levels.push_back(std::move(level));
    state.activeLevelIndex = state.source.levels.size() - 1U;
    state.selection = {CreativeEditorWorldLayoutSelectionKind::Level,
                       state.activeLevelIndex};
    detail::noteWorldLayoutSourceChange(state, "building level added");
    return {true, true, "creative_editor_world_layout_level_added"};
  }
  const std::size_t sourceIndex =
      state.activeLevelIndex < state.source.levels.size() &&
              state.source.levels[state.activeLevelIndex].buildingIndex ==
                  buildingIndex
          ? state.activeLevelIndex
          : first;
  double floorTopLayer = 0.0;
  if (!nextLevelElevation(state.source, buildingIndex, floorTopLayer)) {
    state.statusMessage = "next level elevation is not representable";
    return {false, false,
            "creative_editor_world_layout_level_elevation_invalid"};
  }

  cr::CreativeWorldLayoutLevel level = state.source.levels[sourceIndex];
  level.stableKey = detail::mintWorldLayoutStableKey(state, "level");
  level.name = "Level " +
               std::to_string(levelCountForBuilding(state.source,
                                                     buildingIndex));
  level.floorTopLayer = floorTopLayer;
  const std::size_t levelIndex = state.source.levels.size();
  state.source.levels.push_back(std::move(level));
  state.activeLevelIndex = levelIndex;
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Level,
                     levelIndex};
  detail::noteWorldLayoutSourceChange(state, "building level added");
  return {true, true, "creative_editor_world_layout_level_added"};
}

[[nodiscard]] CreativeEditorWorldLayoutEditReceipt duplicateLevel(
    CreativeEditorWorldLayoutState& state, std::size_t levelIndex) {
  if (levelIndex >= state.source.levels.size()) {
    state.statusMessage = "select a level before duplicating it";
    return {false, false,
            "creative_editor_world_layout_level_duplicate_invalid"};
  }
  if (!validLevelMutationCandidate(state.source)) {
    rejectInvalidLevelMutation(state, "duplicating this level");
    return {false, false,
            "creative_editor_world_layout_level_source_invalid"};
  }
  const std::size_t buildingIndex =
      state.source.levels[levelIndex].buildingIndex;
  double floorTopLayer = 0.0;
  if (!nextLevelElevation(state.source, buildingIndex, floorTopLayer)) {
    state.statusMessage = "duplicated level elevation is not representable";
    return {false, false,
            "creative_editor_world_layout_level_elevation_invalid"};
  }

  const cr::CreativeWorldLayout& source = state.source;
  cr::CreativeWorldLayout candidate = source;
  std::uint64_t nextStableOrdinal = state.nextStableOrdinal;
  const auto mint = [&](std::string_view prefix) {
    return cr::mintCreativeWorldLayoutStableKey(candidate, nextStableOrdinal,
                                                prefix);
  };
  const std::size_t originalRoomCount = source.rooms.size();
  const std::size_t originalVertexCount = source.topologyVertices.size();
  const std::size_t originalEdgeCount = source.topologyEdges.size();
  const std::size_t originalOpeningCount = source.openings.size();
  std::vector<std::size_t> roomMap(
      originalRoomCount, cr::kInvalidCreativeWorldLayoutIndex);
  std::vector<std::size_t> vertexMap(
      originalVertexCount, cr::kInvalidCreativeWorldLayoutIndex);
  std::vector<std::size_t> edgeMap(
      originalEdgeCount, cr::kInvalidCreativeWorldLayoutIndex);

  cr::CreativeWorldLayoutLevel level = source.levels[levelIndex];
  level.stableKey = mint("level");
  level.name += " Copy";
  level.floorTopLayer = floorTopLayer;
  const std::size_t duplicateLevelIndex = candidate.levels.size();
  candidate.levels.push_back(std::move(level));
  for (cr::CreativeWorldLayoutRoofAperture& aperture :
       candidate.roofApertures) {
    if (aperture.levelIndex == levelIndex) {
      aperture.levelIndex = duplicateLevelIndex;
    }
  }

  for (std::size_t index = 0U; index < originalRoomCount; ++index) {
    if (source.rooms[index].levelIndex != levelIndex) {
      continue;
    }
    cr::CreativeWorldLayoutRoom room = source.rooms[index];
    room.levelIndex = duplicateLevelIndex;
    room.stableKey = mint("room");
    room.name += " Copy";
    roomMap[index] = candidate.rooms.size();
    candidate.rooms.push_back(std::move(room));
  }

  if (explicitRoomTopology(source)) {
    for (std::size_t index = 0U; index < originalVertexCount; ++index) {
      const cr::CreativeWorldLayoutTopologyVertex& sourceVertex =
          source.topologyVertices[index];
      if (sourceVertex.levelIndex != levelIndex) {
        continue;
      }
      cr::CreativeWorldLayoutTopologyVertex vertex = sourceVertex;
      vertex.levelIndex = duplicateLevelIndex;
      vertex.stableKey = mint("room_vertex");
      vertexMap[index] = candidate.topologyVertices.size();
      candidate.topologyVertices.push_back(std::move(vertex));
    }
    for (std::size_t index = 0U; index < originalEdgeCount; ++index) {
      const cr::CreativeWorldLayoutTopologyEdge& sourceEdge =
          source.topologyEdges[index];
      if (sourceEdge.levelIndex != levelIndex) {
        continue;
      }
      if (sourceEdge.startVertexIndex >= vertexMap.size() ||
          sourceEdge.endVertexIndex >= vertexMap.size() ||
          vertexMap[sourceEdge.startVertexIndex] ==
              cr::kInvalidCreativeWorldLayoutIndex ||
          vertexMap[sourceEdge.endVertexIndex] ==
              cr::kInvalidCreativeWorldLayoutIndex) {
        rejectInvalidLevelMutation(state, "duplicating this level");
        return {false, false,
                "creative_editor_world_layout_level_topology_remap_invalid"};
      }
      cr::CreativeWorldLayoutTopologyEdge edge = sourceEdge;
      edge.levelIndex = duplicateLevelIndex;
      edge.stableKey = mint("room_edge");
      edge.startVertexIndex = vertexMap[sourceEdge.startVertexIndex];
      edge.endVertexIndex = vertexMap[sourceEdge.endVertexIndex];
      edgeMap[index] = candidate.topologyEdges.size();
      candidate.topologyEdges.push_back(std::move(edge));
    }
    for (const cr::CreativeWorldLayoutRoomBoundary& sourceBoundary :
         source.roomBoundaries) {
      if (sourceBoundary.roomIndex >= roomMap.size() ||
          roomMap[sourceBoundary.roomIndex] ==
              cr::kInvalidCreativeWorldLayoutIndex) {
        continue;
      }
      if (sourceBoundary.topologyEdgeIndex >= edgeMap.size() ||
          edgeMap[sourceBoundary.topologyEdgeIndex] ==
              cr::kInvalidCreativeWorldLayoutIndex) {
        rejectInvalidLevelMutation(state, "duplicating this level");
        return {false, false,
                "creative_editor_world_layout_level_boundary_remap_invalid"};
      }
      cr::CreativeWorldLayoutRoomBoundary boundary = sourceBoundary;
      boundary.roomIndex = roomMap[sourceBoundary.roomIndex];
      boundary.topologyEdgeIndex =
          edgeMap[sourceBoundary.topologyEdgeIndex];
      candidate.roomBoundaries.push_back(boundary);
    }
  }

  for (std::size_t index = 0U; index < originalOpeningCount; ++index) {
    const cr::CreativeWorldLayoutOpening& sourceOpening =
        source.openings[index];
    if (sourceOpening.hostKind !=
            cr::CreativeWorldLayoutOpeningHostKind::RoomEdge ||
        sourceOpening.roomIndex >= roomMap.size() ||
        roomMap[sourceOpening.roomIndex] ==
            cr::kInvalidCreativeWorldLayoutIndex) {
      continue;
    }
    cr::CreativeWorldLayoutOpening opening = sourceOpening;
    opening.roomIndex = roomMap[sourceOpening.roomIndex];
    if (sourceOpening.roomTopologyEdgeIndex !=
        cr::kInvalidCreativeWorldLayoutIndex) {
      if (sourceOpening.roomTopologyEdgeIndex >= edgeMap.size() ||
          edgeMap[sourceOpening.roomTopologyEdgeIndex] ==
              cr::kInvalidCreativeWorldLayoutIndex) {
        rejectInvalidLevelMutation(state, "duplicating this level");
        return {false, false,
                "creative_editor_world_layout_level_opening_remap_invalid"};
      }
      opening.roomTopologyEdgeIndex =
          edgeMap[sourceOpening.roomTopologyEdgeIndex];
    }
    opening.stableKey = mint("opening");
    opening.name += " Copy";
    candidate.openings.push_back(std::move(opening));
  }

  if (!validLevelMutationCandidate(candidate)) {
    rejectInvalidLevelMutation(state, "duplicating this level");
    return {false, false,
            "creative_editor_world_layout_level_duplicate_rejected"};
  }

  state.source = std::move(candidate);
  state.nextStableOrdinal = nextStableOrdinal;
  state.activeLevelIndex = duplicateLevelIndex;
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Level,
                     duplicateLevelIndex};
  detail::noteWorldLayoutSourceChange(state, "building level duplicated");
  return {true, true, "creative_editor_world_layout_level_duplicated"};
}

[[nodiscard]] CreativeEditorWorldLayoutEditReceipt moveLevel(
    CreativeEditorWorldLayoutState& state, std::size_t levelIndex,
    bool later) {
  if (levelIndex >= state.source.levels.size()) {
    return {false, false,
            "creative_editor_world_layout_level_reorder_invalid"};
  }
  const std::size_t buildingIndex =
      state.source.levels[levelIndex].buildingIndex;
  std::size_t otherIndex = cr::kInvalidCreativeWorldLayoutIndex;
  if (later) {
    for (std::size_t index = levelIndex + 1U; index < state.source.levels.size();
         ++index) {
      if (state.source.levels[index].buildingIndex == buildingIndex) {
        otherIndex = index;
        break;
      }
    }
  } else {
    for (std::size_t index = levelIndex; index > 0U; --index) {
      if (state.source.levels[index - 1U].buildingIndex == buildingIndex) {
        otherIndex = index - 1U;
        break;
      }
    }
  }
  if (otherIndex == cr::kInvalidCreativeWorldLayoutIndex) {
    state.statusMessage = later ? "level is already last" :
                                  "level is already first";
    return {true, false,
            "creative_editor_world_layout_level_reorder_no_change"};
  }
  if (!validLevelMutationCandidate(state.source)) {
    rejectInvalidLevelMutation(state, "reordering these levels");
    return {false, false,
            "creative_editor_world_layout_level_source_invalid"};
  }
  cr::CreativeWorldLayout candidate = state.source;
  std::swap(candidate.levels[levelIndex], candidate.levels[otherIndex]);
  for (cr::CreativeWorldLayoutRoom& room : candidate.rooms) {
    if (room.levelIndex == levelIndex) {
      room.levelIndex = otherIndex;
    } else if (room.levelIndex == otherIndex) {
      room.levelIndex = levelIndex;
    }
  }
  for (cr::CreativeWorldLayoutTopologyVertex& vertex :
       candidate.topologyVertices) {
    if (vertex.levelIndex == levelIndex) {
      vertex.levelIndex = otherIndex;
    } else if (vertex.levelIndex == otherIndex) {
      vertex.levelIndex = levelIndex;
    }
  }
  for (cr::CreativeWorldLayoutTopologyEdge& edge : candidate.topologyEdges) {
    if (edge.levelIndex == levelIndex) {
      edge.levelIndex = otherIndex;
    } else if (edge.levelIndex == otherIndex) {
      edge.levelIndex = levelIndex;
    }
  }
  for (cr::CreativeWorldLayoutRoofAperture& aperture :
       candidate.roofApertures) {
    if (aperture.levelIndex == levelIndex) {
      aperture.levelIndex = otherIndex;
    } else if (aperture.levelIndex == otherIndex) {
      aperture.levelIndex = levelIndex;
    }
  }
  if (!validLevelMutationCandidate(candidate)) {
    rejectInvalidLevelMutation(state, "reordering these levels");
    return {false, false,
            "creative_editor_world_layout_level_reorder_rejected"};
  }
  state.source = std::move(candidate);
  state.activeLevelIndex = otherIndex;
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Level,
                     otherIndex};
  detail::noteWorldLayoutSourceChange(state, "building levels reordered");
  return {true, true, "creative_editor_world_layout_level_reordered"};
}

[[nodiscard]] CreativeEditorWorldLayoutEditReceipt deleteLevel(
    CreativeEditorWorldLayoutState& state, std::size_t levelIndex) {
  if (levelIndex >= state.source.levels.size()) {
    state.statusMessage = "select a level before deleting it";
    return {false, false,
            "creative_editor_world_layout_level_delete_invalid"};
  }
  const std::size_t buildingIndex =
      state.source.levels[levelIndex].buildingIndex;
  if (levelCountForBuilding(state.source, buildingIndex) <= 1U) {
    state.statusMessage = "a building must retain one level";
    return {false, false,
            "creative_editor_world_layout_level_delete_last"};
  }
  if (!validLevelMutationCandidate(state.source)) {
    rejectInvalidLevelMutation(state, "deleting this level");
    return {false, false,
            "creative_editor_world_layout_level_source_invalid"};
  }

  const cr::CreativeWorldLayout& source = state.source;
  cr::CreativeWorldLayout candidate = source;
  std::vector<std::size_t> roomMap(
      source.rooms.size(), cr::kInvalidCreativeWorldLayoutIndex);
  std::vector<cr::CreativeWorldLayoutRoom> rooms;
  rooms.reserve(source.rooms.size());
  for (std::size_t index = 0U; index < source.rooms.size(); ++index) {
    cr::CreativeWorldLayoutRoom room = source.rooms[index];
    if (room.levelIndex == levelIndex) {
      continue;
    }
    if (room.levelIndex > levelIndex) {
      --room.levelIndex;
    }
    roomMap[index] = rooms.size();
    rooms.push_back(std::move(room));
  }

  std::vector<std::size_t> vertexMap(
      source.topologyVertices.size(), cr::kInvalidCreativeWorldLayoutIndex);
  std::vector<cr::CreativeWorldLayoutTopologyVertex> vertices;
  vertices.reserve(source.topologyVertices.size());
  for (std::size_t index = 0U; index < source.topologyVertices.size(); ++index) {
    cr::CreativeWorldLayoutTopologyVertex vertex =
        source.topologyVertices[index];
    if (vertex.levelIndex == levelIndex) {
      continue;
    }
    if (vertex.levelIndex > levelIndex) {
      --vertex.levelIndex;
    }
    vertexMap[index] = vertices.size();
    vertices.push_back(std::move(vertex));
  }

  std::vector<std::size_t> edgeMap(
      source.topologyEdges.size(), cr::kInvalidCreativeWorldLayoutIndex);
  std::vector<cr::CreativeWorldLayoutTopologyEdge> edges;
  edges.reserve(source.topologyEdges.size());
  for (std::size_t index = 0U; index < source.topologyEdges.size(); ++index) {
    cr::CreativeWorldLayoutTopologyEdge edge = source.topologyEdges[index];
    if (edge.levelIndex == levelIndex) {
      continue;
    }
    if (edge.startVertexIndex >= vertexMap.size() ||
        edge.endVertexIndex >= vertexMap.size() ||
        vertexMap[edge.startVertexIndex] ==
            cr::kInvalidCreativeWorldLayoutIndex ||
        vertexMap[edge.endVertexIndex] ==
            cr::kInvalidCreativeWorldLayoutIndex) {
      rejectInvalidLevelMutation(state, "deleting this level");
      return {false, false,
              "creative_editor_world_layout_level_topology_remap_invalid"};
    }
    if (edge.levelIndex > levelIndex) {
      --edge.levelIndex;
    }
    edge.startVertexIndex = vertexMap[edge.startVertexIndex];
    edge.endVertexIndex = vertexMap[edge.endVertexIndex];
    edgeMap[index] = edges.size();
    edges.push_back(std::move(edge));
  }

  std::vector<cr::CreativeWorldLayoutRoomBoundary> boundaries;
  boundaries.reserve(source.roomBoundaries.size());
  for (cr::CreativeWorldLayoutRoomBoundary boundary :
       source.roomBoundaries) {
    if (boundary.roomIndex >= roomMap.size() ||
        boundary.topologyEdgeIndex >= edgeMap.size()) {
      rejectInvalidLevelMutation(state, "deleting this level");
      return {false, false,
              "creative_editor_world_layout_level_boundary_remap_invalid"};
    }
    const std::size_t roomIndex = roomMap[boundary.roomIndex];
    const std::size_t edgeIndex = edgeMap[boundary.topologyEdgeIndex];
    if (roomIndex == cr::kInvalidCreativeWorldLayoutIndex ||
        edgeIndex == cr::kInvalidCreativeWorldLayoutIndex) {
      continue;
    }
    boundary.roomIndex = roomIndex;
    boundary.topologyEdgeIndex = edgeIndex;
    boundaries.push_back(boundary);
  }

  std::vector<cr::CreativeWorldLayoutOpening> openings;
  openings.reserve(source.openings.size());
  for (cr::CreativeWorldLayoutOpening opening : source.openings) {
    if (opening.hostKind ==
        cr::CreativeWorldLayoutOpeningHostKind::RoomEdge) {
      if (opening.roomIndex >= roomMap.size() ||
          roomMap[opening.roomIndex] ==
              cr::kInvalidCreativeWorldLayoutIndex) {
        continue;
      }
      opening.roomIndex = roomMap[opening.roomIndex];
      if (opening.roomTopologyEdgeIndex !=
          cr::kInvalidCreativeWorldLayoutIndex) {
        if (opening.roomTopologyEdgeIndex >= edgeMap.size() ||
            edgeMap[opening.roomTopologyEdgeIndex] ==
                cr::kInvalidCreativeWorldLayoutIndex) {
          continue;
        }
        opening.roomTopologyEdgeIndex =
            edgeMap[opening.roomTopologyEdgeIndex];
      }
    }
    openings.push_back(std::move(opening));
  }
  std::vector<cr::CreativeWorldLayoutVerticalConnector> verticalConnectors;
  verticalConnectors.reserve(source.verticalConnectors.size());
  for (cr::CreativeWorldLayoutVerticalConnector connector :
       source.verticalConnectors) {
    if (connector.lowerRoomIndex >= roomMap.size() ||
        connector.upperRoomIndex >= roomMap.size() ||
        roomMap[connector.lowerRoomIndex] ==
            cr::kInvalidCreativeWorldLayoutIndex ||
        roomMap[connector.upperRoomIndex] ==
            cr::kInvalidCreativeWorldLayoutIndex) {
      continue;
    }
    connector.lowerRoomIndex = roomMap[connector.lowerRoomIndex];
    connector.upperRoomIndex = roomMap[connector.upperRoomIndex];
    verticalConnectors.push_back(std::move(connector));
  }
  std::vector<cr::CreativeWorldLayoutRoofAperture> roofApertures;
  roofApertures.reserve(source.roofApertures.size());
  for (cr::CreativeWorldLayoutRoofAperture aperture :
       source.roofApertures) {
    if (aperture.levelIndex == levelIndex) {
      continue;
    }
    if (aperture.levelIndex > levelIndex) {
      --aperture.levelIndex;
    }
    roofApertures.push_back(std::move(aperture));
  }
  candidate.rooms = std::move(rooms);
  candidate.topologyVertices = std::move(vertices);
  candidate.topologyEdges = std::move(edges);
  candidate.roomBoundaries = std::move(boundaries);
  candidate.openings = std::move(openings);
  candidate.verticalConnectors = std::move(verticalConnectors);
  candidate.roofApertures = std::move(roofApertures);
  candidate.levels.erase(
      candidate.levels.begin() + static_cast<std::ptrdiff_t>(levelIndex));
  static_cast<void>(cr::refreshCreativeWorldLayoutBuildingRoomFootprint(
      candidate, buildingIndex));
  if (!validLevelMutationCandidate(candidate)) {
    rejectInvalidLevelMutation(state, "deleting this level");
    return {false, false,
            "creative_editor_world_layout_level_delete_rejected"};
  }

  state.source = std::move(candidate);
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Building,
                     buildingIndex};
  state.activeLevelIndex = cr::kInvalidCreativeWorldLayoutIndex;
  repairCreativeEditorWorldLayoutActiveLevel(state, buildingIndex);
  detail::noteWorldLayoutSourceChange(state, "building level deleted");
  return {true, true, "creative_editor_world_layout_level_deleted"};
}

}  // namespace

void repairCreativeEditorWorldLayoutActiveLevel(
    CreativeEditorWorldLayoutState& state,
    std::size_t preferredBuildingIndex) noexcept {
  if (state.activeLevelIndex < state.source.levels.size() &&
      (preferredBuildingIndex == cr::kInvalidCreativeWorldLayoutIndex ||
       state.source.levels[state.activeLevelIndex].buildingIndex ==
           preferredBuildingIndex)) {
    return;
  }
  state.activeLevelIndex =
      preferredBuildingIndex < state.source.buildings.size()
          ? firstLevelForBuilding(state.source, preferredBuildingIndex)
          : cr::kInvalidCreativeWorldLayoutIndex;
  if (preferredBuildingIndex == cr::kInvalidCreativeWorldLayoutIndex &&
      !state.source.levels.empty()) {
    state.activeLevelIndex = 0U;
  }
}

CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutLevelOperation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutLevelOperation operation,
    std::size_t buildingIndex, std::size_t levelIndex) {
  switch (operation) {
    case CreativeEditorWorldLayoutLevelOperation::Select:
      return selectLevel(state, levelIndex);
    case CreativeEditorWorldLayoutLevelOperation::Add:
      return addLevel(state, buildingIndex);
    case CreativeEditorWorldLayoutLevelOperation::Duplicate:
      return duplicateLevel(state, resolvedLevelIndex(state, levelIndex));
    case CreativeEditorWorldLayoutLevelOperation::MoveEarlier:
      return moveLevel(state, resolvedLevelIndex(state, levelIndex), false);
    case CreativeEditorWorldLayoutLevelOperation::MoveLater:
      return moveLevel(state, resolvedLevelIndex(state, levelIndex), true);
    case CreativeEditorWorldLayoutLevelOperation::Delete:
      return deleteLevel(state, resolvedLevelIndex(state, levelIndex));
    case CreativeEditorWorldLayoutLevelOperation::Count:
      break;
  }
  return {false, false,
          "creative_editor_world_layout_level_operation_invalid"};
}

}  // namespace iggy3d_creative_app
