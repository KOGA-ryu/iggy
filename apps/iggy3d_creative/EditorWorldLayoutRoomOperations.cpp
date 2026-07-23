#include "EditorWorldLayout.hpp"

#include "EditorWorldLayoutInternal.hpp"

#include "app/iggy3d/creative/world/WorldLayoutOrthogonalRooms.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRoomOperations.hpp"
#include "app/iggy3d/creative/world/WorldLayoutWallOperations.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

namespace iggy3d_creative_app {
namespace {

std::string_view roomOperationMessage(
    cr::CreativeWorldLayoutRoomOperationStatus status) noexcept {
  switch (status) {
    case cr::CreativeWorldLayoutRoomOperationStatus::InvalidRequest:
      return "room operation request is invalid";
    case cr::CreativeWorldLayoutRoomOperationStatus::InvalidSourceTopology:
      return "repair the floor plan before editing room boundaries";
    case cr::CreativeWorldLayoutRoomOperationStatus::DuplicateStableKey:
      return "room identity could not be allocated";
    case cr::CreativeWorldLayoutRoomOperationStatus::CutDoesNotBisectRoom:
      return "split line must cross the room interior";
    case cr::CreativeWorldLayoutRoomOperationStatus::DisconnectedResult:
      return "operation would create a disconnected room";
    case cr::CreativeWorldLayoutRoomOperationStatus::RoomsNotAdjacent:
      return "rooms must share a wall before they can be merged";
    case cr::CreativeWorldLayoutRoomOperationStatus::OpeningConflict:
      return "move the opening away from the edited room boundary first";
    case cr::CreativeWorldLayoutRoomOperationStatus::ConnectorConflict:
      return "move the stair or ramp away from the edited room boundary first";
    case cr::CreativeWorldLayoutRoomOperationStatus::BoundaryCapacityExceeded:
      return "room boundary is too complex for the current topology limit";
    case cr::CreativeWorldLayoutRoomOperationStatus::ResultingTopologyInvalid:
      return "operation would create an invalid floor plan";
    case cr::CreativeWorldLayoutRoomOperationStatus::NoChange:
      return "room boundary unchanged";
    case cr::CreativeWorldLayoutRoomOperationStatus::NotRequested:
      return "room operation was not requested";
    case cr::CreativeWorldLayoutRoomOperationStatus::Ready:
      return "room operation ready";
  }
  return "room operation failed";
}

std::string_view wallOperationMessage(
    cr::CreativeWorldLayoutWallOperationStatus status) noexcept {
  switch (status) {
    case cr::CreativeWorldLayoutWallOperationStatus::InvalidRequest:
      return "wall operation request is invalid";
    case cr::CreativeWorldLayoutWallOperationStatus::InvalidSourceTopology:
      return "repair the floor plan before editing walls";
    case cr::CreativeWorldLayoutWallOperationStatus::DuplicateStableKey:
      return "wall identity could not be allocated";
    case cr::CreativeWorldLayoutWallOperationStatus::SplitOutsideInterior:
      return "split position must be inside the wall";
    case cr::CreativeWorldLayoutWallOperationStatus::OpeningConflict:
      return "move the opening away from the wall junction first";
    case cr::CreativeWorldLayoutWallOperationStatus::BoundaryCapacityExceeded:
      return "wall split exceeds the room topology limit";
    case cr::CreativeWorldLayoutWallOperationStatus::EdgesNotMergeable:
      return "walls must be adjacent parts of the same boundary";
    case cr::CreativeWorldLayoutWallOperationStatus::AttributesDiffer:
      return "wall settings must match before merging";
    case cr::CreativeWorldLayoutWallOperationStatus::JunctionInUse:
      return "wall junction is connected to another wall";
    case cr::CreativeWorldLayoutWallOperationStatus::ResultingTopologyInvalid:
      return "wall operation would create an invalid floor plan";
    case cr::CreativeWorldLayoutWallOperationStatus::NotRequested:
      return "wall operation was not requested";
    case cr::CreativeWorldLayoutWallOperationStatus::Ready:
      return "wall operation ready";
  }
  return "wall operation failed";
}

std::size_t topologyEdgeIndexWithStableKey(
    const cr::CreativeWorldLayout& source,
    std::string_view stableKey) noexcept {
  const auto found = std::find_if(
      source.topologyEdges.begin(), source.topologyEdges.end(),
      [stableKey](const cr::CreativeWorldLayoutTopologyEdge& edge) {
        return edge.stableKey == stableKey;
      });
  return found == source.topologyEdges.end()
             ? cr::kInvalidCreativeWorldLayoutIndex
             : static_cast<std::size_t>(found - source.topologyEdges.begin());
}

CreativeEditorWorldLayoutEditReceipt commitRoomOperation(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutRoomOperationResult operation,
    std::uint64_t nextStableOrdinal, std::string statusMessage,
    std::string reasonCode) {
  if (!operation.accepted || !operation.changed ||
      operation.primaryRoomIndex >= operation.edited.rooms.size()) {
    state.statusMessage = roomOperationMessage(operation.status);
    return {operation.accepted && !operation.changed, false,
            operation.reasonCode};
  }

  const bool preserveTopologyEdge =
      state.selection.kind ==
          CreativeEditorWorldLayoutSelectionKind::TopologyEdge &&
      state.selection.index < state.source.topologyEdges.size();
  const std::string selectedTopologyEdgeKey =
      preserveTopologyEdge
          ? state.source.topologyEdges[state.selection.index].stableKey
          : std::string{};
  state.source = std::move(operation.edited);
  state.nextStableOrdinal = nextStableOrdinal;
  state.activeLevelIndex =
      state.source.rooms[operation.primaryRoomIndex].levelIndex;
  const std::size_t selectedTopologyEdgeIndex =
      topologyEdgeIndexWithStableKey(state.source, selectedTopologyEdgeKey);
  state.selection =
      preserveTopologyEdge &&
              selectedTopologyEdgeIndex < state.source.topologyEdges.size()
          ? CreativeEditorWorldLayoutSelection{
                CreativeEditorWorldLayoutSelectionKind::TopologyEdge,
                selectedTopologyEdgeIndex}
          : CreativeEditorWorldLayoutSelection{
                CreativeEditorWorldLayoutSelectionKind::Room,
                operation.primaryRoomIndex};
  detail::noteWorldLayoutSourceChange(state, std::move(statusMessage));
  return {true, true, std::move(reasonCode)};
}

CreativeEditorWorldLayoutEditReceipt commitWallOperation(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutWallOperationResult operation,
    std::uint64_t nextStableOrdinal, std::string statusMessage,
    std::string reasonCode) {
  if (!operation.accepted || !operation.changed ||
      operation.topologyEdgeIndex >= operation.edited.topologyEdges.size()) {
    state.statusMessage = wallOperationMessage(operation.status);
    return {operation.accepted && !operation.changed, false,
            operation.reasonCode};
  }
  state.source = std::move(operation.edited);
  state.nextStableOrdinal = nextStableOrdinal;
  state.selection = {CreativeEditorWorldLayoutSelectionKind::TopologyEdge,
                     operation.topologyEdgeIndex};
  state.selectedRoomTopologyEdgeStableKey =
      state.source.topologyEdges[operation.topologyEdgeIndex].stableKey;
  state.activeLevelIndex =
      state.source.topologyEdges[operation.topologyEdgeIndex].levelIndex;
  detail::noteWorldLayoutSourceChange(state, std::move(statusMessage));
  return {true, true, std::move(reasonCode)};
}

}  // namespace

CreativeEditorWorldLayoutEditReceipt splitCreativeEditorWorldLayoutRoom(
    CreativeEditorWorldLayoutState& state, std::size_t roomIndex,
    cr::CreativeWorldLayoutRoomSplitAxis axis, std::int32_t coordinate) {
  if (roomIndex >= state.source.rooms.size() ||
      axis >= cr::CreativeWorldLayoutRoomSplitAxis::Count) {
    state.statusMessage = "select a valid room and split direction";
    return {false, false,
            "creative_editor_world_layout_room_split_request_invalid"};
  }

  std::uint64_t nextStableOrdinal = state.nextStableOrdinal;
  cr::CreativeWorldLayoutRoomOperationResult operation =
      cr::splitCreativeWorldLayoutRoom(
          state.source,
          {roomIndex, axis, coordinate,
           cr::mintCreativeWorldLayoutStableKey(
               state.source, nextStableOrdinal, "room"),
           "Room " + std::to_string(state.source.rooms.size() + 1U)});
  return commitRoomOperation(
      state, std::move(operation), nextStableOrdinal, "room split",
      "creative_editor_world_layout_room_split_applied");
}

CreativeEditorWorldLayoutEditReceipt mergeCreativeEditorWorldLayoutRooms(
    CreativeEditorWorldLayoutState& state, std::size_t primaryRoomIndex,
    std::size_t secondaryRoomIndex) {
  cr::CreativeWorldLayoutRoomOperationResult operation =
      cr::mergeCreativeWorldLayoutRooms(
          state.source, {primaryRoomIndex, secondaryRoomIndex});
  return commitRoomOperation(
      state, std::move(operation), state.nextStableOrdinal, "rooms merged",
      "creative_editor_world_layout_rooms_merged");
}

CreativeEditorWorldLayoutEditReceipt moveCreativeEditorWorldLayoutRoomBoundary(
    CreativeEditorWorldLayoutState& state, std::size_t topologyEdgeIndex,
    std::int32_t coordinate) {
  cr::CreativeWorldLayoutRoomOperationResult operation =
      cr::moveCreativeWorldLayoutRoomBoundary(
          state.source, {topologyEdgeIndex, coordinate});
  return commitRoomOperation(
      state, std::move(operation), state.nextStableOrdinal,
      "room boundary moved",
      "creative_editor_world_layout_room_boundary_moved");
}

CreativeEditorWorldLayoutEditReceipt moveCreativeEditorWorldLayoutRoomCorner(
    CreativeEditorWorldLayoutState& state, std::size_t roomIndex,
    std::size_t topologyVertexIndex, cr::CreativeTerrainCoord2 position) {
  cr::CreativeWorldLayoutRoomOperationResult operation =
      cr::moveCreativeWorldLayoutRoomCorner(
          state.source, {roomIndex, topologyVertexIndex, position});
  return commitRoomOperation(
      state, std::move(operation), state.nextStableOrdinal,
      "room corner moved", "creative_editor_world_layout_room_corner_moved");
}

CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutRoomEdgeSettings(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutRoomEdgeSettingsRequest request) {
  const cr::CreativeWorldLayoutRoomGraph graph =
      cr::buildCreativeWorldLayoutRoomGraph(state.source);
  if (graph.accepted && request.topologyEdgeIndex < graph.edges.size()) {
    state.selectedRoomTopologyEdgeStableKey =
        graph.edges[request.topologyEdgeIndex].stableKey;
  }
  cr::CreativeWorldLayoutRoomOperationResult operation =
      cr::setCreativeWorldLayoutRoomEdgeSettings(state.source, request);
  return commitRoomOperation(
      state, std::move(operation), state.nextStableOrdinal,
      "wall dimensions updated",
      "creative_editor_world_layout_room_edge_settings_updated");
}

CreativeEditorWorldLayoutEditReceipt splitCreativeEditorWorldLayoutWall(
    CreativeEditorWorldLayoutState& state, std::size_t topologyEdgeIndex,
    std::uint32_t offsetCells) {
  std::uint64_t nextStableOrdinal = state.nextStableOrdinal;
  cr::CreativeWorldLayoutWallOperationResult operation =
      cr::splitCreativeWorldLayoutWall(
          state.source,
          {topologyEdgeIndex, offsetCells,
           cr::mintCreativeWorldLayoutStableKey(
               state.source, nextStableOrdinal, "wall_vertex"),
           cr::mintCreativeWorldLayoutStableKey(
               state.source, nextStableOrdinal, "wall")});
  return commitWallOperation(
      state, std::move(operation), nextStableOrdinal, "wall split",
      "creative_editor_world_layout_wall_split_applied");
}

CreativeEditorWorldLayoutEditReceipt mergeCreativeEditorWorldLayoutWalls(
    CreativeEditorWorldLayoutState& state,
    std::size_t primaryTopologyEdgeIndex,
    std::size_t secondaryTopologyEdgeIndex) {
  cr::CreativeWorldLayoutWallOperationResult operation =
      cr::mergeCreativeWorldLayoutWalls(
          state.source,
          {primaryTopologyEdgeIndex, secondaryTopologyEdgeIndex});
  return commitWallOperation(
      state, std::move(operation), state.nextStableOrdinal, "walls merged",
      "creative_editor_world_layout_walls_merged");
}

CreativeEditorWorldLayoutRoomBoundaryTarget
findCreativeEditorWorldLayoutRoomBoundaryTarget(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) {
  const bool roomSelected =
      state.selection.kind == CreativeEditorWorldLayoutSelectionKind::Room &&
      state.selection.index < state.source.rooms.size();
  const bool topologyEdgeSelected =
      state.selection.kind ==
          CreativeEditorWorldLayoutSelectionKind::TopologyEdge &&
      state.selection.index < state.source.topologyEdges.size();
  if ((!roomSelected && !topologyEdgeSelected) ||
      state.source.roomBoundaries.empty() || !std::isfinite(point.x) ||
      !std::isfinite(point.z) || !std::isfinite(toleranceCells) ||
      toleranceCells <= 0.0) {
    return {};
  }
  const cr::CreativeWorldLayoutRoomGraph graph =
      cr::buildCreativeWorldLayoutRoomGraph(state.source);
  if (!graph.accepted) {
    return {};
  }
  std::size_t roomIndex = roomSelected
                              ? state.selection.index
                              : cr::kInvalidCreativeWorldLayoutIndex;
  if (topologyEdgeSelected) {
    const auto owner = std::find_if(
        graph.boundaries.begin(), graph.boundaries.end(),
        [&](const cr::CreativeWorldLayoutRoomBoundary& boundary) {
          return boundary.topologyEdgeIndex == state.selection.index;
        });
    if (owner == graph.boundaries.end()) {
      return {};
    }
    roomIndex = owner->roomIndex;
  }
  if (roomIndex >= graph.rooms.size()) {
    return {};
  }

  CreativeEditorWorldLayoutRoomBoundaryTarget target;
  double bestDistance = toleranceCells;
  for (const cr::CreativeWorldLayoutRoomBoundary& boundary :
       cr::creativeWorldLayoutRoomBoundaries(graph, roomIndex)) {
    if (topologyEdgeSelected &&
        boundary.topologyEdgeIndex != state.selection.index) {
      continue;
    }
    const cr::CreativeWorldLayoutTopologyEdge& edge =
        graph.edges[boundary.topologyEdgeIndex];
    const cr::CreativeTerrainCoord2 start =
        graph.vertices[edge.startVertexIndex].position;
    const cr::CreativeTerrainCoord2 end =
        graph.vertices[edge.endVertexIndex].position;
    const bool horizontal = start.z == end.z;
    const double nearestX = horizontal
                                ? std::clamp(point.x,
                                             static_cast<double>(start.x),
                                             static_cast<double>(end.x))
                                : static_cast<double>(start.x);
    const double nearestZ = horizontal
                                ? static_cast<double>(start.z)
                                : std::clamp(point.z,
                                             static_cast<double>(start.z),
                                             static_cast<double>(end.z));
    const double distance = std::hypot(point.x - nearestX,
                                       point.z - nearestZ);
    if (distance <= bestDistance) {
      bestDistance = distance;
      target = {roomIndex, boundary.topologyEdgeIndex, horizontal};
    }
  }
  return target;
}

CreativeEditorWorldLayoutRoomCornerTarget
findCreativeEditorWorldLayoutRoomCornerTarget(
    const CreativeEditorWorldLayoutState& state, std::size_t roomIndex,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) {
  if (roomIndex >= state.source.rooms.size() ||
      state.source.roomBoundaries.empty() || !std::isfinite(point.x) ||
      !std::isfinite(point.z) || !std::isfinite(toleranceCells) ||
      toleranceCells <= 0.0) {
    return {};
  }
  const cr::CreativeWorldLayoutRoomGraph graph =
      cr::buildCreativeWorldLayoutRoomGraph(state.source);
  if (!graph.accepted || roomIndex >= graph.rooms.size()) {
    return {};
  }

  CreativeEditorWorldLayoutRoomCornerTarget target;
  double bestDistance = toleranceCells;
  for (const cr::CreativeWorldLayoutRoomBoundary& boundary :
       cr::creativeWorldLayoutRoomBoundaries(graph, roomIndex)) {
    const cr::CreativeWorldLayoutTopologyEdge& edge =
        graph.edges[boundary.topologyEdgeIndex];
    for (const std::size_t vertexIndex :
         {edge.startVertexIndex, edge.endVertexIndex}) {
      const cr::CreativeTerrainCoord2 position =
          graph.vertices[vertexIndex].position;
      const double distance =
          std::hypot(point.x - static_cast<double>(position.x),
                     point.z - static_cast<double>(position.z));
      if (distance <= bestDistance) {
        bestDistance = distance;
        target.roomIndex = roomIndex;
        target.topologyVertexIndex = vertexIndex;
      }
    }
  }
  if (target.topologyVertexIndex >= graph.vertices.size()) {
    return {};
  }

  const cr::CreativeTerrainCoord2 corner =
      graph.vertices[target.topologyVertexIndex].position;
  bool horizontalPositive = false;
  bool verticalPositive = false;
  bool foundHorizontal = false;
  bool foundVertical = false;
  for (const cr::CreativeWorldLayoutRoomBoundary& boundary :
       cr::creativeWorldLayoutRoomBoundaries(graph, roomIndex)) {
    const cr::CreativeWorldLayoutTopologyEdge& edge =
        graph.edges[boundary.topologyEdgeIndex];
    if (edge.startVertexIndex != target.topologyVertexIndex &&
        edge.endVertexIndex != target.topologyVertexIndex) {
      continue;
    }
    const std::size_t otherIndex =
        edge.startVertexIndex == target.topologyVertexIndex
            ? edge.endVertexIndex
            : edge.startVertexIndex;
    const cr::CreativeTerrainCoord2 other =
        graph.vertices[otherIndex].position;
    if (other.z == corner.z) {
      horizontalPositive = other.x > corner.x;
      foundHorizontal = true;
    } else if (other.x == corner.x) {
      verticalPositive = other.z > corner.z;
      foundVertical = true;
    }
  }
  if (!foundHorizontal || !foundVertical) {
    return {};
  }
  target.northWestSouthEast = horizontalPositive == verticalPositive;
  return target;
}

CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutRoomBoundaryManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutRoomBoundaryManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) {
  if (phase >= CreativeEditorWorldLayoutRoomBoundaryManipulationPhase::Count) {
    return {false, false,
            "creative_editor_world_layout_room_boundary_phase_invalid"};
  }
  if (phase ==
      CreativeEditorWorldLayoutRoomBoundaryManipulationPhase::Cancel) {
    const bool changed = state.roomBoundaryManipulation.active;
    state.roomBoundaryManipulation = {};
    state.statusMessage = "room boundary edit cancelled";
    return {true, changed,
            "creative_editor_world_layout_room_boundary_cancelled"};
  }
  if (state.tool != CreativeEditorWorldLayoutTool::Select) {
    return {false, false,
            "creative_editor_world_layout_room_boundary_tool_invalid"};
  }
  if (phase == CreativeEditorWorldLayoutRoomBoundaryManipulationPhase::Begin) {
    const bool preserveTopologyEdgeSelection =
        state.selection.kind ==
        CreativeEditorWorldLayoutSelectionKind::TopologyEdge;
    const CreativeEditorWorldLayoutRoomBoundaryTarget target =
        findCreativeEditorWorldLayoutRoomBoundaryTarget(
            state, point, toleranceCells);
    const cr::CreativeWorldLayoutRoomGraph graph =
        cr::buildCreativeWorldLayoutRoomGraph(state.source);
    if (target.topologyEdgeIndex >= graph.edges.size()) {
      return {false, false,
              "creative_editor_world_layout_room_boundary_target_missing"};
    }
    const cr::CreativeWorldLayoutTopologyEdge& edge =
        graph.edges[target.topologyEdgeIndex];
    const cr::CreativeTerrainCoord2 start =
        graph.vertices[edge.startVertexIndex].position;
    const std::int32_t coordinate = target.horizontal ? start.z : start.x;
    detail::clearWorldLayoutInteraction(state);
    state.selection = preserveTopologyEdgeSelection
                          ? CreativeEditorWorldLayoutSelection{
                                CreativeEditorWorldLayoutSelectionKind::TopologyEdge,
                                target.topologyEdgeIndex}
                          : CreativeEditorWorldLayoutSelection{
                                CreativeEditorWorldLayoutSelectionKind::Room,
                                target.roomIndex};
    state.anchorActive = false;
    state.roomBoundaryManipulation.active = true;
    state.roomBoundaryManipulation.sourceRevision = state.revision;
    state.roomBoundaryManipulation.target = target;
    state.selectedRoomTopologyEdgeStableKey = edge.stableKey;
    state.roomBoundaryManipulation.originalCoordinate = coordinate;
    state.roomBoundaryManipulation.previewCoordinate = coordinate;
    state.roomBoundaryManipulation.previewEdit =
        cr::moveCreativeWorldLayoutRoomBoundary(
            state.source, {target.topologyEdgeIndex, coordinate});
    state.roomBoundaryManipulation.previewValid = true;
    state.roomBoundaryManipulation.reasonCode =
        state.roomBoundaryManipulation.previewEdit.reasonCode;
    state.statusMessage = target.horizontal
                              ? "drag to move wall north or south"
                              : "drag to move wall east or west";
    return {true, true,
            "creative_editor_world_layout_room_boundary_started"};
  }
  if (!state.roomBoundaryManipulation.active ||
      state.roomBoundaryManipulation.sourceRevision != state.revision) {
    state.roomBoundaryManipulation = {};
    return {false, false,
            "creative_editor_world_layout_room_boundary_stale"};
  }

  const double rawCoordinate = state.roomBoundaryManipulation.target.horizontal
                                   ? point.z
                                   : point.x;
  if (!std::isfinite(rawCoordinate) ||
      rawCoordinate < std::numeric_limits<std::int32_t>::min() ||
      rawCoordinate > std::numeric_limits<std::int32_t>::max()) {
    state.roomBoundaryManipulation.previewValid = false;
    state.roomBoundaryManipulation.reasonCode =
        "creative_editor_world_layout_room_boundary_coordinate_invalid";
    state.statusMessage = "room boundary is outside the layout grid";
    return {true, true, state.roomBoundaryManipulation.reasonCode};
  }
  const std::int32_t coordinate =
      static_cast<std::int32_t>(std::llround(rawCoordinate));
  if (phase == CreativeEditorWorldLayoutRoomBoundaryManipulationPhase::Update &&
      coordinate == state.roomBoundaryManipulation.previewCoordinate) {
    return {true, false, state.roomBoundaryManipulation.reasonCode};
  }

  cr::CreativeWorldLayoutRoomOperationResult preview =
      cr::moveCreativeWorldLayoutRoomBoundary(
          state.source,
          {state.roomBoundaryManipulation.target.topologyEdgeIndex,
           coordinate});
  state.roomBoundaryManipulation.previewCoordinate = coordinate;
  state.roomBoundaryManipulation.previewValid = preview.accepted;
  state.roomBoundaryManipulation.reasonCode = preview.reasonCode;
  state.statusMessage = std::string(roomOperationMessage(preview.status));
  state.roomBoundaryManipulation.previewEdit = std::move(preview);
  if (phase == CreativeEditorWorldLayoutRoomBoundaryManipulationPhase::Update) {
    return {true, true, state.roomBoundaryManipulation.reasonCode};
  }

  if (!state.roomBoundaryManipulation.previewValid) {
    const std::string reasonCode =
        state.roomBoundaryManipulation.reasonCode;
    state.roomBoundaryManipulation = {};
    return {false, false, reasonCode};
  }
  if (!state.roomBoundaryManipulation.previewEdit.changed) {
    state.roomBoundaryManipulation = {};
    state.statusMessage = "room boundary unchanged";
    return {true, false,
            "creative_editor_world_layout_room_boundary_no_change"};
  }

  const std::size_t roomIndex =
      state.roomBoundaryManipulation.target.roomIndex;
  const std::string selectedEdgeKey =
      state.selectedRoomTopologyEdgeStableKey;
  const bool preserveTopologyEdgeSelection =
      state.selection.kind ==
      CreativeEditorWorldLayoutSelectionKind::TopologyEdge;
  state.source =
      std::move(state.roomBoundaryManipulation.previewEdit.edited);
  state.roomBoundaryManipulation = {};
  const std::size_t selectedEdgeIndex =
      topologyEdgeIndexWithStableKey(state.source, selectedEdgeKey);
  state.selection =
      preserveTopologyEdgeSelection &&
              selectedEdgeIndex < state.source.topologyEdges.size()
          ? CreativeEditorWorldLayoutSelection{
                CreativeEditorWorldLayoutSelectionKind::TopologyEdge,
                selectedEdgeIndex}
          : CreativeEditorWorldLayoutSelection{
                CreativeEditorWorldLayoutSelectionKind::Room, roomIndex};
  state.activeLevelIndex = state.source.rooms[roomIndex].levelIndex;
  detail::noteWorldLayoutSourceChange(state, "room boundary moved");
  return {true, true,
          "creative_editor_world_layout_room_boundary_moved"};
}

CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutRoomCornerManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutRoomCornerManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) {
  if (phase >= CreativeEditorWorldLayoutRoomCornerManipulationPhase::Count) {
    return {false, false,
            "creative_editor_world_layout_room_corner_phase_invalid"};
  }
  if (phase == CreativeEditorWorldLayoutRoomCornerManipulationPhase::Cancel) {
    const bool changed = state.roomCornerManipulation.active;
    state.roomCornerManipulation = {};
    state.statusMessage = "room corner edit cancelled";
    return {true, changed,
            "creative_editor_world_layout_room_corner_cancelled"};
  }
  if (state.tool != CreativeEditorWorldLayoutTool::Select) {
    return {false, false,
            "creative_editor_world_layout_room_corner_tool_invalid"};
  }
  if (phase == CreativeEditorWorldLayoutRoomCornerManipulationPhase::Begin) {
    if (state.selection.kind !=
        CreativeEditorWorldLayoutSelectionKind::Room) {
      return {false, false,
              "creative_editor_world_layout_room_corner_selection_missing"};
    }
    const CreativeEditorWorldLayoutRoomCornerTarget target =
        findCreativeEditorWorldLayoutRoomCornerTarget(
            state, state.selection.index, point, toleranceCells);
    const cr::CreativeWorldLayoutRoomGraph graph =
        cr::buildCreativeWorldLayoutRoomGraph(state.source);
    if (target.topologyVertexIndex >= graph.vertices.size()) {
      return {false, false,
              "creative_editor_world_layout_room_corner_target_missing"};
    }
    const cr::CreativeTerrainCoord2 position =
        graph.vertices[target.topologyVertexIndex].position;
    detail::clearWorldLayoutInteraction(state);
    state.selection = {CreativeEditorWorldLayoutSelectionKind::Room,
                       target.roomIndex};
    state.anchorActive = false;
    state.roomCornerManipulation.active = true;
    state.roomCornerManipulation.sourceRevision = state.revision;
    state.roomCornerManipulation.target = target;
    state.roomCornerManipulation.originalPosition = position;
    state.roomCornerManipulation.previewPosition = position;
    state.roomCornerManipulation.previewEdit =
        cr::moveCreativeWorldLayoutRoomCorner(
            state.source,
            {target.roomIndex, target.topologyVertexIndex, position});
    state.roomCornerManipulation.previewValid = true;
    state.roomCornerManipulation.reasonCode =
        state.roomCornerManipulation.previewEdit.reasonCode;
    state.statusMessage = "drag the room corner on the grid";
    return {true, true,
            "creative_editor_world_layout_room_corner_started"};
  }
  if (!state.roomCornerManipulation.active ||
      state.roomCornerManipulation.sourceRevision != state.revision) {
    state.roomCornerManipulation = {};
    return {false, false,
            "creative_editor_world_layout_room_corner_stale"};
  }

  if (!std::isfinite(point.x) || !std::isfinite(point.z) ||
      point.x < std::numeric_limits<std::int32_t>::min() ||
      point.x > std::numeric_limits<std::int32_t>::max() ||
      point.z < std::numeric_limits<std::int32_t>::min() ||
      point.z > std::numeric_limits<std::int32_t>::max()) {
    state.roomCornerManipulation.previewValid = false;
    state.roomCornerManipulation.reasonCode =
        "creative_editor_world_layout_room_corner_position_invalid";
    state.statusMessage = "room corner is outside the layout grid";
    return {true, true, state.roomCornerManipulation.reasonCode};
  }
  const cr::CreativeTerrainCoord2 position{
      static_cast<std::int32_t>(std::llround(point.x)),
      static_cast<std::int32_t>(std::llround(point.z))};
  if (phase == CreativeEditorWorldLayoutRoomCornerManipulationPhase::Update &&
      position == state.roomCornerManipulation.previewPosition) {
    return {true, false, state.roomCornerManipulation.reasonCode};
  }

  cr::CreativeWorldLayoutRoomOperationResult preview =
      cr::moveCreativeWorldLayoutRoomCorner(
          state.source,
          {state.roomCornerManipulation.target.roomIndex,
           state.roomCornerManipulation.target.topologyVertexIndex, position});
  state.roomCornerManipulation.previewPosition = position;
  state.roomCornerManipulation.previewValid = preview.accepted;
  state.roomCornerManipulation.reasonCode = preview.reasonCode;
  state.statusMessage = std::string(roomOperationMessage(preview.status));
  state.roomCornerManipulation.previewEdit = std::move(preview);
  if (phase == CreativeEditorWorldLayoutRoomCornerManipulationPhase::Update) {
    return {true, true, state.roomCornerManipulation.reasonCode};
  }

  if (!state.roomCornerManipulation.previewValid) {
    const std::string reasonCode = state.roomCornerManipulation.reasonCode;
    state.roomCornerManipulation = {};
    return {false, false, reasonCode};
  }
  if (!state.roomCornerManipulation.previewEdit.changed) {
    state.roomCornerManipulation = {};
    state.statusMessage = "room corner unchanged";
    return {true, false,
            "creative_editor_world_layout_room_corner_no_change"};
  }

  const std::size_t roomIndex =
      state.roomCornerManipulation.target.roomIndex;
  state.source = std::move(state.roomCornerManipulation.previewEdit.edited);
  state.roomCornerManipulation = {};
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Room, roomIndex};
  state.activeLevelIndex = state.source.rooms[roomIndex].levelIndex;
  detail::noteWorldLayoutSourceChange(state, "room corner moved");
  return {true, true, "creative_editor_world_layout_room_corner_moved"};
}

}  // namespace iggy3d_creative_app
