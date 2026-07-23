#include "app/iggy3d/creative/world/WorldLayoutRoomTopology.hpp"

#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>
#include <vector>

namespace iggy3d::creative {
namespace {

constexpr double kGeometryEpsilon = 1.0e-9;

bool validRect(CreativeWorldLayoutRect rect) noexcept {
  return rect.minimum.x < rect.maximum.x &&
         rect.minimum.z < rect.maximum.z;
}

bool sameRect(CreativeWorldLayoutRect lhs,
              CreativeWorldLayoutRect rhs) noexcept {
  return lhs.minimum == rhs.minimum && lhs.maximum == rhs.maximum;
}

bool containsRect(CreativeWorldLayoutRect outer,
                  CreativeWorldLayoutRect inner) noexcept {
  return validRect(outer) && validRect(inner) &&
         outer.minimum.x <= inner.minimum.x &&
         outer.minimum.z <= inner.minimum.z &&
         outer.maximum.x >= inner.maximum.x &&
         outer.maximum.z >= inner.maximum.z;
}

bool horizontalEdge(CreativeWorldLayoutRoomEdge edge) noexcept {
  return edge == CreativeWorldLayoutRoomEdge::North ||
         edge == CreativeWorldLayoutRoomEdge::South;
}

std::int32_t edgeLine(CreativeWorldLayoutRect rect,
                      CreativeWorldLayoutRoomEdge edge) noexcept {
  switch (edge) {
    case CreativeWorldLayoutRoomEdge::North:
      return rect.minimum.z;
    case CreativeWorldLayoutRoomEdge::East:
      return rect.maximum.x;
    case CreativeWorldLayoutRoomEdge::South:
      return rect.maximum.z;
    case CreativeWorldLayoutRoomEdge::West:
      return rect.minimum.x;
    case CreativeWorldLayoutRoomEdge::Count:
      break;
  }
  return 0;
}

void setEdgeLine(CreativeWorldLayoutRect& rect,
                 CreativeWorldLayoutRoomEdge edge,
                 std::int32_t line) noexcept {
  switch (edge) {
    case CreativeWorldLayoutRoomEdge::North:
      rect.minimum.z = line;
      return;
    case CreativeWorldLayoutRoomEdge::East:
      rect.maximum.x = line;
      return;
    case CreativeWorldLayoutRoomEdge::South:
      rect.maximum.z = line;
      return;
    case CreativeWorldLayoutRoomEdge::West:
      rect.minimum.x = line;
      return;
    case CreativeWorldLayoutRoomEdge::Count:
      return;
  }
}

double edgeLength(CreativeWorldLayoutRect rect,
                  CreativeWorldLayoutRoomEdge edge) noexcept {
  return horizontalEdge(edge)
             ? static_cast<double>(rect.maximum.x) - rect.minimum.x
             : static_cast<double>(rect.maximum.z) - rect.minimum.z;
}

double edgeOrigin(CreativeWorldLayoutRect rect,
                  CreativeWorldLayoutRoomEdge edge) noexcept {
  return horizontalEdge(edge) ? static_cast<double>(rect.minimum.x)
                              : static_cast<double>(rect.minimum.z);
}

bool validRoom(const CreativeWorldLayoutRoom& room) noexcept {
  if (!validRect(room.footprint) ||
      !std::isfinite(room.wallThicknessCells) ||
      room.wallThicknessCells <= 0.0) {
    return false;
  }
  return edgeLength(room.footprint, CreativeWorldLayoutRoomEdge::North) >
             room.wallThicknessCells * 2.0 &&
         edgeLength(room.footprint, CreativeWorldLayoutRoomEdge::East) >
             room.wallThicknessCells * 2.0;
}

bool pureTranslation(CreativeWorldLayoutRect source,
                     CreativeWorldLayoutRect target) noexcept {
  const std::int64_t minimumDeltaX =
      static_cast<std::int64_t>(target.minimum.x) - source.minimum.x;
  const std::int64_t maximumDeltaX =
      static_cast<std::int64_t>(target.maximum.x) - source.maximum.x;
  const std::int64_t minimumDeltaZ =
      static_cast<std::int64_t>(target.minimum.z) - source.minimum.z;
  const std::int64_t maximumDeltaZ =
      static_cast<std::int64_t>(target.maximum.z) - source.maximum.z;
  return (minimumDeltaX != 0 || minimumDeltaZ != 0) &&
         minimumDeltaX == maximumDeltaX && minimumDeltaZ == maximumDeltaZ;
}

void fail(CreativeWorldLayoutRoomFootprintEditResult& result,
          CreativeWorldLayoutRoomFootprintEditStatus status,
          std::string reasonCode) {
  result.accepted = false;
  result.changed = false;
  result.status = status;
  result.edited = {};
  result.roomChanges.clear();
  result.adjustedOpeningCount = 0U;
  result.reasonCode = std::move(reasonCode);
}

bool spanSideForRoom(const CreativeWorldLayoutSharedRoomEdgeSpan& span,
                     std::size_t roomIndex,
                     CreativeWorldLayoutRoomEdge roomEdge,
                     std::size_t& otherRoomIndex,
                     CreativeWorldLayoutRoomEdge& otherRoomEdge) noexcept {
  if (span.firstRoomIndex == roomIndex && span.firstRoomEdge == roomEdge) {
    otherRoomIndex = span.secondRoomIndex;
    otherRoomEdge = span.secondRoomEdge;
    return true;
  }
  if (span.secondRoomIndex == roomIndex && span.secondRoomEdge == roomEdge) {
    otherRoomIndex = span.firstRoomIndex;
    otherRoomEdge = span.firstRoomEdge;
    return true;
  }
  return false;
}

bool openingFitsRoom(const CreativeWorldLayout& layout,
                     std::size_t openingIndex,
                     double minimumEndClearance) noexcept {
  const CreativeWorldLayoutOpening& opening = layout.openings[openingIndex];
  if (opening.hostKind != CreativeWorldLayoutOpeningHostKind::RoomEdge) {
    return true;
  }
  if (opening.roomIndex >= layout.rooms.size() ||
      opening.roomEdge >= CreativeWorldLayoutRoomEdge::Count ||
      !std::isfinite(opening.centerOffsetCells) ||
      !std::isfinite(opening.widthCells) || opening.widthCells <= 0.0) {
    return false;
  }
  const double halfWidth = opening.widthCells * 0.5;
  const double length =
      edgeLength(layout.rooms[opening.roomIndex].footprint, opening.roomEdge);
  return opening.centerOffsetCells - halfWidth >=
             minimumEndClearance - kGeometryEpsilon &&
         opening.centerOffsetCells + halfWidth <=
             length - minimumEndClearance + kGeometryEpsilon;
}

bool expandedOpeningsOverlap(const CreativeWorldLayout& expanded,
                             std::size_t& failedOpeningIndex) noexcept {
  for (std::size_t index = 0U; index < expanded.openings.size(); ++index) {
    const CreativeWorldLayoutOpening& opening = expanded.openings[index];
    for (std::size_t prior = 0U; prior < index; ++prior) {
      const CreativeWorldLayoutOpening& existing = expanded.openings[prior];
      const bool horizontallyOverlapping =
          std::fabs(opening.centerOffsetCells - existing.centerOffsetCells) <=
          (opening.widthCells + existing.widthCells) * 0.5 +
              kGeometryEpsilon;
      const double openingTop =
          opening.cutoutBottomCells + opening.cutoutHeightCells;
      const double existingTop =
          existing.cutoutBottomCells + existing.cutoutHeightCells;
      const bool verticallySeparated =
          openingTop + kGeometryEpsilon < existing.cutoutBottomCells ||
          existingTop + kGeometryEpsilon < opening.cutoutBottomCells;
      if (opening.wallIndex == existing.wallIndex &&
          horizontallyOverlapping && !verticallySeparated) {
        failedOpeningIndex = index;
        return true;
      }
    }
  }
  return false;
}

bool expandedOpeningFitsWall(const CreativeWorldLayout& expanded,
                             std::size_t openingIndex) noexcept {
  const CreativeWorldLayoutOpening& opening = expanded.openings[openingIndex];
  if (opening.wallIndex >= expanded.walls.size() ||
      !std::isfinite(opening.cutoutBottomCells) ||
      !std::isfinite(opening.cutoutHeightCells) ||
      opening.cutoutBottomCells < 0.0 || opening.cutoutHeightCells <= 0.0) {
    return false;
  }
  return opening.cutoutBottomCells + opening.cutoutHeightCells <=
         static_cast<double>(expanded.walls[opening.wallIndex].heightCells) +
             kGeometryEpsilon;
}

}  // namespace

bool refreshCreativeWorldLayoutBuildingRoomFootprint(
    CreativeWorldLayout& layout, std::size_t buildingIndex) noexcept {
  if (buildingIndex >= layout.buildings.size()) {
    return false;
  }
  CreativeWorldLayoutBuilding& building = layout.buildings[buildingIndex];
  if (building.rootMode != CreativeBuildingRootMode::None) {
    return validRect(building.rootFootprint);
  }
  CreativeWorldLayoutRect bounds;
  bool found = false;
  for (const CreativeWorldLayoutRoom& room : layout.rooms) {
    if (room.buildingIndex != buildingIndex) {
      continue;
    }
    if (!validRect(room.footprint)) {
      return false;
    }
    if (!found) {
      bounds = room.footprint;
      found = true;
      continue;
    }
    bounds.minimum.x = std::min(bounds.minimum.x, room.footprint.minimum.x);
    bounds.minimum.z = std::min(bounds.minimum.z, room.footprint.minimum.z);
    bounds.maximum.x = std::max(bounds.maximum.x, room.footprint.maximum.x);
    bounds.maximum.z = std::max(bounds.maximum.z, room.footprint.maximum.z);
  }
  if (found) {
    building.rootFootprint = bounds;
  }
  return found;
}

CreativeWorldLayoutRoomFootprintEditResult
editCreativeWorldLayoutRoomFootprint(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutRoomFootprintEditRequest& request) {
  CreativeWorldLayoutRoomFootprintEditResult result;
  result.requested = true;
  result.sourceRoomIndex = request.roomIndex;
  if (request.roomIndex >= source.rooms.size() ||
      !std::isfinite(request.minimumOpeningEndClearanceCells) ||
      request.minimumOpeningEndClearanceCells < 0.0) {
    fail(result, CreativeWorldLayoutRoomFootprintEditStatus::InvalidRequest,
         "creative_world_layout_room_footprint_edit_request_invalid");
    return result;
  }

  const CreativeWorldLayoutRoom& sourceRoom = source.rooms[request.roomIndex];
  if (sourceRoom.buildingIndex >= source.buildings.size() ||
      sourceRoom.levelIndex >= source.levels.size() ||
      source.levels[sourceRoom.levelIndex].buildingIndex !=
          sourceRoom.buildingIndex) {
    result.failedRoomIndex = request.roomIndex;
    fail(result, CreativeWorldLayoutRoomFootprintEditStatus::InvalidOwnership,
         "creative_world_layout_room_footprint_edit_ownership_invalid");
    return result;
  }
  const CreativeWorldLayoutRoomCompileResult sourceTopology =
      expandCreativeWorldLayoutRooms(source);
  if (!sourceTopology.accepted) {
    result.failedRoomIndex = sourceTopology.failedIndex;
    fail(result,
         CreativeWorldLayoutRoomFootprintEditStatus::InvalidSourceTopology,
         sourceTopology.reasonCode);
    return result;
  }
  if (!validRect(request.targetFootprint)) {
    result.failedRoomIndex = request.roomIndex;
    fail(result, CreativeWorldLayoutRoomFootprintEditStatus::InvalidFootprint,
         "creative_world_layout_room_footprint_edit_geometry_invalid");
    return result;
  }

  const std::vector<CreativeWorldLayoutSharedRoomEdgeSpan> sharedSpans =
      inspectCreativeWorldLayoutSharedRoomEdges(source);
  const bool translating =
      pureTranslation(sourceRoom.footprint, request.targetFootprint);
  if (translating &&
      std::any_of(sharedSpans.begin(), sharedSpans.end(),
                  [&](const CreativeWorldLayoutSharedRoomEdgeSpan& span) {
                    return span.firstRoomIndex == request.roomIndex ||
                           span.secondRoomIndex == request.roomIndex;
                  })) {
    result.failedRoomIndex = request.roomIndex;
    fail(result,
         CreativeWorldLayoutRoomFootprintEditStatus::SharedRoomMoveUnsupported,
         "creative_world_layout_room_footprint_edit_shared_move_unsupported");
    return result;
  }

  result.edited = source;
  result.edited.rooms[request.roomIndex].footprint = request.targetFootprint;
  std::vector<bool> affected(source.rooms.size(), false);
  affected[request.roomIndex] = true;
  constexpr std::array<CreativeWorldLayoutRoomEdge, 4U> kEdges = {
      CreativeWorldLayoutRoomEdge::North,
      CreativeWorldLayoutRoomEdge::East,
      CreativeWorldLayoutRoomEdge::South,
      CreativeWorldLayoutRoomEdge::West,
  };
  if (!translating) {
    for (const CreativeWorldLayoutRoomEdge edge : kEdges) {
      if (edgeLine(sourceRoom.footprint, edge) ==
          edgeLine(request.targetFootprint, edge)) {
        continue;
      }
      const std::int32_t targetLine =
          edgeLine(request.targetFootprint, edge);
      for (const CreativeWorldLayoutSharedRoomEdgeSpan& span : sharedSpans) {
        std::size_t otherRoomIndex = kInvalidCreativeWorldLayoutIndex;
        CreativeWorldLayoutRoomEdge otherRoomEdge =
            CreativeWorldLayoutRoomEdge::Count;
        if (!spanSideForRoom(span, request.roomIndex, edge, otherRoomIndex,
                             otherRoomEdge)) {
          continue;
        }
        setEdgeLine(result.edited.rooms[otherRoomIndex].footprint,
                    otherRoomEdge, targetLine);
        affected[otherRoomIndex] = true;
      }
    }
  }

  for (std::size_t roomIndex = 0U; roomIndex < affected.size(); ++roomIndex) {
    if (!affected[roomIndex]) {
      continue;
    }
    if (!validRoom(result.edited.rooms[roomIndex])) {
      result.failedRoomIndex = roomIndex;
      fail(result, CreativeWorldLayoutRoomFootprintEditStatus::InvalidFootprint,
           "creative_world_layout_room_footprint_edit_geometry_invalid");
      return result;
    }
  }

  if (!translating) {
    for (CreativeWorldLayoutOpening& opening : result.edited.openings) {
      if (opening.hostKind != CreativeWorldLayoutOpeningHostKind::RoomEdge ||
          opening.roomIndex >= affected.size() ||
          !affected[opening.roomIndex]) {
        continue;
      }
      const double originDelta =
          edgeOrigin(source.rooms[opening.roomIndex].footprint,
                     opening.roomEdge) -
          edgeOrigin(result.edited.rooms[opening.roomIndex].footprint,
                     opening.roomEdge);
      if (originDelta != 0.0) {
        opening.centerOffsetCells += originDelta;
        ++result.adjustedOpeningCount;
      }
    }
  }

  for (std::size_t openingIndex = 0U;
       openingIndex < result.edited.openings.size(); ++openingIndex) {
    if (!openingFitsRoom(result.edited, openingIndex,
                         request.minimumOpeningEndClearanceCells)) {
      result.failedOpeningIndex = openingIndex;
      fail(result,
           CreativeWorldLayoutRoomFootprintEditStatus::OpeningDoesNotFit,
           "creative_world_layout_room_footprint_edit_opening_does_not_fit");
      return result;
    }
  }

  const CreativeWorldLayoutRoomCompileResult editedTopology =
      expandCreativeWorldLayoutRooms(result.edited);
  if (!editedTopology.accepted) {
    result.failedRoomIndex = editedTopology.failedIndex;
    fail(result,
         CreativeWorldLayoutRoomFootprintEditStatus::ResultingTopologyInvalid,
         editedTopology.reasonCode);
    return result;
  }
  if (creativeWorldLayoutHasInteriorRoomWindow(result.edited)) {
    fail(result, CreativeWorldLayoutRoomFootprintEditStatus::InteriorWindow,
         "creative_world_layout_room_footprint_edit_interior_window");
    return result;
  }
  for (std::size_t connectorIndex = 0U;
       connectorIndex < result.edited.verticalConnectors.size();
       ++connectorIndex) {
    const CreativeWorldLayoutVerticalConnector& connector =
        result.edited.verticalConnectors[connectorIndex];
    const bool lowerAffected = connector.lowerRoomIndex < affected.size() &&
                               affected[connector.lowerRoomIndex];
    const bool upperAffected = connector.upperRoomIndex < affected.size() &&
                               affected[connector.upperRoomIndex];
    if (!lowerAffected && !upperAffected) {
      continue;
    }
    if (connector.lowerRoomIndex >= result.edited.rooms.size() ||
        connector.upperRoomIndex >= result.edited.rooms.size() ||
        connector.buildingIndex >= result.edited.buildings.size() ||
        result.edited.rooms[connector.lowerRoomIndex].buildingIndex !=
            connector.buildingIndex ||
        result.edited.rooms[connector.upperRoomIndex].buildingIndex !=
            connector.buildingIndex ||
        !containsRect(
            result.edited.rooms[connector.lowerRoomIndex].footprint,
            connector.footprint) ||
        !containsRect(
            result.edited.rooms[connector.upperRoomIndex].footprint,
            connector.footprint)) {
      result.failedConnectorIndex = connectorIndex;
      fail(result,
           CreativeWorldLayoutRoomFootprintEditStatus::
               VerticalConnectorDoesNotFit,
           "creative_world_layout_room_footprint_edit_connector_does_not_fit");
      return result;
    }
  }
  for (std::size_t openingIndex = 0U;
       openingIndex < editedTopology.expanded.openings.size(); ++openingIndex) {
    if (!expandedOpeningFitsWall(editedTopology.expanded, openingIndex)) {
      result.failedOpeningIndex = openingIndex;
      fail(result,
           CreativeWorldLayoutRoomFootprintEditStatus::OpeningDoesNotFit,
           "creative_world_layout_room_footprint_edit_opening_height_invalid");
      return result;
    }
  }
  if (expandedOpeningsOverlap(editedTopology.expanded,
                              result.failedOpeningIndex)) {
    fail(result, CreativeWorldLayoutRoomFootprintEditStatus::OpeningOverlap,
         "creative_world_layout_room_footprint_edit_opening_overlap");
    return result;
  }

  const CreativeWorldLayoutRect originalBuildingFootprint =
      source.buildings[sourceRoom.buildingIndex].rootFootprint;
  if (!refreshCreativeWorldLayoutBuildingRoomFootprint(
          result.edited, sourceRoom.buildingIndex)) {
    result.failedRoomIndex = request.roomIndex;
    fail(result,
         CreativeWorldLayoutRoomFootprintEditStatus::ResultingTopologyInvalid,
         "creative_world_layout_room_footprint_edit_building_empty");
    return result;
  }
  for (std::size_t roomIndex = 0U; roomIndex < source.rooms.size();
       ++roomIndex) {
    if (!sameRect(source.rooms[roomIndex].footprint,
                  result.edited.rooms[roomIndex].footprint)) {
      result.roomChanges.push_back({roomIndex, source.rooms[roomIndex].footprint,
                                    result.edited.rooms[roomIndex].footprint});
    }
  }
  const bool rootChanged = !sameRect(
      originalBuildingFootprint,
      result.edited.buildings[sourceRoom.buildingIndex].rootFootprint);
  result.accepted = true;
  result.changed = !result.roomChanges.empty() || rootChanged;
  result.status = result.changed
                      ? CreativeWorldLayoutRoomFootprintEditStatus::Ready
                      : CreativeWorldLayoutRoomFootprintEditStatus::NoChange;
  result.reasonCode =
      result.changed ? "creative_world_layout_room_footprint_edit_ready"
                     : "creative_world_layout_room_footprint_edit_no_change";
  return result;
}

}  // namespace iggy3d::creative
