#include "app/iggy3d/creative/world/WorldLayoutSourceDuplication.hpp"

#include "app/iggy3d/creative/world/WorldLayoutBuildingOps.hpp"
#include "app/iggy3d/creative/world/WorldLayoutOpenings.hpp"
#include "app/iggy3d/creative/world/WorldLayoutOrthogonalRooms.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRoomTopology.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRoofs.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace iggy3d::creative {
namespace {

struct GridOffset {
  std::int64_t x = 0;
  std::int64_t z = 0;
};

constexpr std::size_t kDefaultPlacementRings = 8U;

[[nodiscard]] bool validGrid(CreativeGridSettings grid) noexcept {
  return isFiniteCreativeVec3(grid.origin) &&
         std::isfinite(grid.cellSizeMeters) && grid.cellSizeMeters > 0.0;
}

[[nodiscard]] bool validRect(CreativeWorldLayoutRect value) noexcept {
  return value.maximum.x > value.minimum.x &&
         value.maximum.z > value.minimum.z;
}

[[nodiscard]] bool offsetCoordinate(std::int32_t value, std::int64_t delta,
                                    std::int32_t& output) noexcept {
  const std::int64_t widened = static_cast<std::int64_t>(value);
  constexpr std::int64_t minimum =
      std::numeric_limits<std::int32_t>::min();
  constexpr std::int64_t maximum =
      std::numeric_limits<std::int32_t>::max();
  if (delta < minimum - widened || delta > maximum - widened) {
    return false;
  }
  output = static_cast<std::int32_t>(widened + delta);
  return true;
}

[[nodiscard]] bool offsetCoordinate(double value, std::int64_t delta,
                                    double& output) noexcept {
  output = value + static_cast<double>(delta);
  return std::isfinite(output);
}

[[nodiscard]] bool canOffsetCoordinate(std::int32_t value,
                                       std::int64_t delta) noexcept {
  std::int32_t ignored = 0;
  return offsetCoordinate(value, delta, ignored);
}

[[nodiscard]] bool canOffsetCoordinate(double value,
                                       std::int64_t delta) noexcept {
  double ignored = 0.0;
  return offsetCoordinate(value, delta, ignored);
}

[[nodiscard]] bool offsetRect(CreativeWorldLayoutRect& value,
                              GridOffset offset) noexcept {
  return offsetCoordinate(value.minimum.x, offset.x, value.minimum.x) &&
         offsetCoordinate(value.maximum.x, offset.x, value.maximum.x) &&
         offsetCoordinate(value.minimum.z, offset.z, value.minimum.z) &&
         offsetCoordinate(value.maximum.z, offset.z, value.maximum.z);
}

[[nodiscard]] bool rectsOverlap(CreativeWorldLayoutRect lhs,
                                CreativeWorldLayoutRect rhs) noexcept {
  return lhs.minimum.x < rhs.maximum.x && lhs.maximum.x > rhs.minimum.x &&
         lhs.minimum.z < rhs.maximum.z && lhs.maximum.z > rhs.minimum.z;
}

[[nodiscard]] bool finiteBounds(CreativeBounds value) noexcept {
  return isFiniteCreativeVec3(value.min) && isFiniteCreativeVec3(value.max) &&
         value.max.x > value.min.x && value.max.y > value.min.y &&
         value.max.z > value.min.z;
}

[[nodiscard]] std::int64_t positiveSpan(std::int32_t minimum,
                                        std::int32_t maximum) noexcept {
  const std::int64_t span = static_cast<std::int64_t>(maximum) -
                            static_cast<std::int64_t>(minimum);
  return span > 0 ? span : 1;
}

[[nodiscard]] std::int64_t positiveSpan(double minimum,
                                        double maximum) noexcept {
  const long double span = static_cast<long double>(maximum) -
                           static_cast<long double>(minimum);
  if (!std::isfinite(span) || span <= 0.0L) {
    return 1;
  }
  const long double rounded = std::ceil(span);
  const long double limit = static_cast<long double>(
      std::numeric_limits<std::int64_t>::max());
  return rounded <= limit ? static_cast<std::int64_t>(rounded)
                          : std::numeric_limits<std::int64_t>::max();
}

template <typename Function>
[[nodiscard]] bool visitOffsets(
    const CreativeWorldLayoutSourceDuplicateRequest& request,
    std::int64_t stepX, std::int64_t stepZ, Function&& function) {
  if (request.useExplicitOffset) {
    return (request.deltaXCells != 0 || request.deltaZCells != 0) &&
           function(GridOffset{request.deltaXCells, request.deltaZCells});
  }
  stepX = std::max<std::int64_t>(stepX, 1);
  stepZ = std::max<std::int64_t>(stepZ, 1);
  for (std::size_t ring = 1U; ring <= kDefaultPlacementRings; ++ring) {
    const std::int64_t scale = static_cast<std::int64_t>(ring);
    if (stepX <= std::numeric_limits<std::int64_t>::max() / scale) {
      const std::int64_t delta = stepX * scale;
      if (function(GridOffset{delta, 0}) ||
          function(GridOffset{-delta, 0})) {
        return true;
      }
    }
    if (stepZ <= std::numeric_limits<std::int64_t>::max() / scale) {
      const std::int64_t delta = stepZ * scale;
      if (function(GridOffset{0, delta}) ||
          function(GridOffset{0, -delta})) {
        return true;
      }
    }
  }
  return false;
}

void reject(CreativeWorldLayoutSourceDuplicateResult& result,
            CreativeWorldLayoutSourceDuplicateStatus status,
            std::string_view reasonCode) {
  result.status = status;
  result.reasonCode = std::string(reasonCode);
}

void accept(CreativeWorldLayoutSourceDuplicateResult& result,
            CreativeWorldLayout edited, std::size_t duplicateIndex,
            std::uint64_t nextStableOrdinal, std::string_view reasonCode) {
  result.accepted = true;
  result.changed = true;
  result.status = CreativeWorldLayoutSourceDuplicateStatus::Ready;
  result.duplicate = {result.source.table, duplicateIndex};
  result.nextStableOrdinal = nextStableOrdinal;
  result.edited = std::move(edited);
  result.reasonCode = std::string(reasonCode);
}

[[nodiscard]] bool validSourceIndex(const CreativeWorldLayout& layout,
                                    CreativeWorldLayoutTable table,
                                    std::size_t index) noexcept {
  switch (table) {
    case CreativeWorldLayoutTable::Room:
      return index < layout.rooms.size();
    case CreativeWorldLayoutTable::VerticalConnector:
      return index < layout.verticalConnectors.size();
    case CreativeWorldLayoutTable::Box:
      return index < layout.boxes.size();
    case CreativeWorldLayoutTable::Opening:
      return index < layout.openings.size();
    case CreativeWorldLayoutTable::Object:
      return index < layout.objects.size();
    case CreativeWorldLayoutTable::RoofAperture:
      return index < layout.roofApertures.size();
    case CreativeWorldLayoutTable::Building:
      return index < layout.buildings.size();
    case CreativeWorldLayoutTable::Level:
      return index < layout.levels.size();
    case CreativeWorldLayoutTable::Wall:
      return index < layout.walls.size();
    case CreativeWorldLayoutTable::TerrainProfile:
      return index < layout.terrainProfiles.size();
    case CreativeWorldLayoutTable::TerrainPath:
      return index < layout.terrainPaths.size();
    case CreativeWorldLayoutTable::TopologyEdge:
      return index < layout.topologyEdges.size();
    case CreativeWorldLayoutTable::None:
    case CreativeWorldLayoutTable::TerrainPathPoint:
      return false;
  }
  return false;
}

[[nodiscard]] bool validSourceRoomGraph(
    const CreativeWorldLayout& source) {
  const bool hasExplicitTopology = !source.topologyVertices.empty() ||
                                   !source.topologyEdges.empty() ||
                                   !source.roomBoundaries.empty();
  if (source.rooms.empty()) {
    return !hasExplicitTopology;
  }
  return buildCreativeWorldLayoutRoomGraph(source).accepted;
}

[[nodiscard]] bool sourceCoordinatesAcceptOffset(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutSourceDuplicateRequest& request,
    GridOffset offset) noexcept {
  switch (request.table) {
    case CreativeWorldLayoutTable::Room: {
      const CreativeWorldLayoutRoom& room =
          source.rooms[request.sourceIndex];
      if (!canOffsetCoordinate(room.footprint.minimum.x, offset.x) ||
          !canOffsetCoordinate(room.footprint.maximum.x, offset.x) ||
          !canOffsetCoordinate(room.footprint.minimum.z, offset.z) ||
          !canOffsetCoordinate(room.footprint.maximum.z, offset.z)) {
        return false;
      }
      for (const CreativeWorldLayoutRoomBoundary& boundary :
           source.roomBoundaries) {
        if (boundary.roomIndex != request.sourceIndex ||
            boundary.topologyEdgeIndex >= source.topologyEdges.size()) {
          continue;
        }
        const CreativeWorldLayoutTopologyEdge& edge =
            source.topologyEdges[boundary.topologyEdgeIndex];
        if (edge.startVertexIndex >= source.topologyVertices.size() ||
            edge.endVertexIndex >= source.topologyVertices.size()) {
          continue;
        }
        for (const std::size_t vertexIndex :
             {edge.startVertexIndex, edge.endVertexIndex}) {
          const CreativeTerrainCoord2 position =
              source.topologyVertices[vertexIndex].position;
          if (!canOffsetCoordinate(position.x, offset.x) ||
              !canOffsetCoordinate(position.z, offset.z)) {
            return false;
          }
        }
      }
      return true;
    }
    case CreativeWorldLayoutTable::Box: {
      const CreativeWorldLayoutRect footprint =
          source.boxes[request.sourceIndex].footprint;
      return canOffsetCoordinate(footprint.minimum.x, offset.x) &&
             canOffsetCoordinate(footprint.maximum.x, offset.x) &&
             canOffsetCoordinate(footprint.minimum.z, offset.z) &&
             canOffsetCoordinate(footprint.maximum.z, offset.z);
    }
    case CreativeWorldLayoutTable::Opening:
      return canOffsetCoordinate(
          source.openings[request.sourceIndex].centerOffsetCells,
          offset.x != 0 ? offset.x : offset.z);
    case CreativeWorldLayoutTable::Object: {
      const CreativeWorldLayoutObject& object =
          source.objects[request.sourceIndex];
      if (object.mode == CreativeObjectLibraryPlacementMode::Bounds) {
        return canOffsetCoordinate(object.boundsCells.min.x, offset.x) &&
               canOffsetCoordinate(object.boundsCells.max.x, offset.x) &&
               canOffsetCoordinate(object.boundsCells.min.z, offset.z) &&
               canOffsetCoordinate(object.boundsCells.max.z, offset.z);
      }
      if (object.mode == CreativeObjectLibraryPlacementMode::Point) {
        return canOffsetCoordinate(object.pointCells.x, offset.x) &&
               canOffsetCoordinate(object.pointCells.z, offset.z);
      }
      return false;
    }
    case CreativeWorldLayoutTable::RoofAperture: {
      const CreativeWorldLayoutRoofAperture& aperture =
          source.roofApertures[request.sourceIndex];
      return canOffsetCoordinate(aperture.minimumXCells, offset.x) &&
             canOffsetCoordinate(aperture.maximumXCells, offset.x) &&
             canOffsetCoordinate(aperture.minimumZCells, offset.z) &&
             canOffsetCoordinate(aperture.maximumZCells, offset.z);
    }
    case CreativeWorldLayoutTable::Building:
    case CreativeWorldLayoutTable::Level:
    case CreativeWorldLayoutTable::VerticalConnector:
    case CreativeWorldLayoutTable::Wall:
    case CreativeWorldLayoutTable::TerrainProfile:
    case CreativeWorldLayoutTable::TerrainPath:
    case CreativeWorldLayoutTable::TerrainPathPoint:
    case CreativeWorldLayoutTable::TopologyEdge:
    case CreativeWorldLayoutTable::None:
      return false;
  }
  return false;
}

[[nodiscard]] bool duplicateObjectAtOffset(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutSourceDuplicateRequest& request,
    GridOffset offset, CreativeWorldLayout& output,
    std::uint64_t& nextStableOrdinal) {
  const CreativeWorldLayoutObject& original =
      source.objects[request.sourceIndex];
  if (original.usesBridgeRecipe ||
      original.mode >= CreativeObjectLibraryPlacementMode::Count) {
    return false;
  }
  output = source;
  CreativeWorldLayoutObject duplicate = original;
  if (duplicate.mode == CreativeObjectLibraryPlacementMode::Bounds) {
    if (!offsetCoordinate(duplicate.boundsCells.min.x, offset.x,
                          duplicate.boundsCells.min.x) ||
        !offsetCoordinate(duplicate.boundsCells.max.x, offset.x,
                          duplicate.boundsCells.max.x) ||
        !offsetCoordinate(duplicate.boundsCells.min.z, offset.z,
                          duplicate.boundsCells.min.z) ||
        !offsetCoordinate(duplicate.boundsCells.max.z, offset.z,
                          duplicate.boundsCells.max.z) ||
        !finiteBounds(duplicate.boundsCells)) {
      return false;
    }
  } else if (duplicate.mode == CreativeObjectLibraryPlacementMode::Point) {
    if (!offsetCoordinate(duplicate.pointCells.x, offset.x,
                          duplicate.pointCells.x) ||
        !offsetCoordinate(duplicate.pointCells.z, offset.z,
                          duplicate.pointCells.z) ||
        !isFiniteCreativeVec3(duplicate.pointCells)) {
      return false;
    }
  } else {
    return false;
  }
  duplicate.stableKey =
      mintCreativeWorldLayoutStableKey(output, nextStableOrdinal, "object");
  duplicate.name += " Copy";
  output.objects.push_back(std::move(duplicate));
  return true;
}

[[nodiscard]] bool duplicateBoxAtOffset(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutSourceDuplicateRequest& request,
    GridOffset offset, CreativeWorldLayout& output,
    std::uint64_t& nextStableOrdinal) {
  const CreativeWorldLayoutBox& original = source.boxes[request.sourceIndex];
  if (original.buildingIndex >= source.buildings.size() ||
      !validRect(original.footprint)) {
    return false;
  }
  output = source;
  CreativeWorldLayoutBox duplicate = original;
  if (!offsetRect(duplicate.footprint, offset)) {
    return false;
  }
  const bool overlaps = std::any_of(
      source.boxes.begin(), source.boxes.end(),
      [&](const CreativeWorldLayoutBox& box) {
        return box.buildingIndex == duplicate.buildingIndex &&
               box.kind == duplicate.kind &&
               box.anchorLayer == duplicate.anchorLayer &&
               rectsOverlap(box.footprint, duplicate.footprint);
      });
  if (overlaps) {
    return false;
  }
  duplicate.stableKey =
      mintCreativeWorldLayoutStableKey(output, nextStableOrdinal, "box");
  duplicate.name += " Copy";
  output.boxes.push_back(std::move(duplicate));
  return validCreativeWorldLayoutBuildingOwnership(output);
}

[[nodiscard]] bool duplicateOpeningAtOffset(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutSourceDuplicateRequest& request,
    GridOffset offset, CreativeWorldLayout& output,
    std::uint64_t& nextStableOrdinal) {
  output = source;
  CreativeWorldLayoutOpening duplicate =
      source.openings[request.sourceIndex];
  const std::int64_t along =
      offset.x != 0 ? offset.x : offset.z;
  if (!offsetCoordinate(duplicate.centerOffsetCells, along,
                        duplicate.centerOffsetCells)) {
    return false;
  }
  duplicate.stableKey =
      mintCreativeWorldLayoutStableKey(output, nextStableOrdinal, "opening");
  duplicate.name += " Copy";
  const std::size_t duplicateIndex = output.openings.size();
  output.openings.push_back(std::move(duplicate));
  return validateCreativeWorldLayoutOpening(
             {&output, &output.openings.back(), duplicateIndex})
      .accepted;
}

[[nodiscard]] bool duplicateRoofApertureAtOffset(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutSourceDuplicateRequest& request,
    GridOffset offset, CreativeWorldLayout& output,
    std::uint64_t& nextStableOrdinal) {
  const CreativeWorldLayoutRoofAperture& original =
      source.roofApertures[request.sourceIndex];
  if (original.levelIndex >= source.levels.size() ||
      !validGrid(request.grid)) {
    return false;
  }
  output = source;
  CreativeWorldLayoutRoofAperture duplicate = original;
  if (!offsetCoordinate(duplicate.minimumXCells, offset.x,
                        duplicate.minimumXCells) ||
      !offsetCoordinate(duplicate.maximumXCells, offset.x,
                        duplicate.maximumXCells) ||
      !offsetCoordinate(duplicate.minimumZCells, offset.z,
                        duplicate.minimumZCells) ||
      !offsetCoordinate(duplicate.maximumZCells, offset.z,
                        duplicate.maximumZCells)) {
    return false;
  }
  duplicate.stableKey = mintCreativeWorldLayoutStableKey(
      output, nextStableOrdinal, "roof_aperture");
  duplicate.name += " Copy";
  output.roofApertures.push_back(std::move(duplicate));
  return planCreativeWorldLayoutRoof(request.grid, output,
                                     original.levelIndex)
      .accepted;
}

[[nodiscard]] bool cloneRoomTopology(
    const CreativeWorldLayout& source, std::size_t roomIndex,
    GridOffset offset, CreativeWorldLayout& output,
    std::uint64_t& nextStableOrdinal, std::size_t& duplicateRoomIndex) {
  const CreativeWorldLayoutRoom& original = source.rooms[roomIndex];
  if (original.buildingIndex >= source.buildings.size() ||
      original.levelIndex >= source.levels.size() ||
      !validRect(original.footprint)) {
    return false;
  }

  output = source;
  CreativeWorldLayoutRoom duplicate = original;
  if (!offsetRect(duplicate.footprint, offset)) {
    return false;
  }
  const bool overlaps = std::any_of(
      source.rooms.begin(), source.rooms.end(),
      [&](const CreativeWorldLayoutRoom& room) {
        return room.levelIndex == duplicate.levelIndex &&
               rectsOverlap(room.footprint, duplicate.footprint);
      });
  if (overlaps) {
    return false;
  }
  duplicate.stableKey =
      mintCreativeWorldLayoutStableKey(output, nextStableOrdinal, "room");
  duplicate.name += " Copy";
  duplicateRoomIndex = output.rooms.size();
  output.rooms.push_back(std::move(duplicate));

  const bool explicitTopology = !source.topologyVertices.empty() ||
                                !source.topologyEdges.empty() ||
                                !source.roomBoundaries.empty();
  std::vector<std::size_t> vertexMap(
      source.topologyVertices.size(), kInvalidCreativeWorldLayoutIndex);
  std::vector<std::size_t> edgeMap(
      source.topologyEdges.size(), kInvalidCreativeWorldLayoutIndex);
  if (explicitTopology) {
    std::size_t boundaryCount = 0U;
    for (const CreativeWorldLayoutRoomBoundary& boundary :
         source.roomBoundaries) {
      if (boundary.roomIndex != roomIndex) {
        continue;
      }
      ++boundaryCount;
      if (boundary.topologyEdgeIndex >= source.topologyEdges.size()) {
        return false;
      }
      const std::size_t edgeIndex = boundary.topologyEdgeIndex;
      if (edgeMap[edgeIndex] == kInvalidCreativeWorldLayoutIndex) {
        const CreativeWorldLayoutTopologyEdge& sourceEdge =
            source.topologyEdges[edgeIndex];
        const std::array<std::size_t, 2U> sourceVertexIndices{
            sourceEdge.startVertexIndex, sourceEdge.endVertexIndex};
        for (const std::size_t vertexIndex : sourceVertexIndices) {
          if (vertexIndex >= source.topologyVertices.size()) {
            return false;
          }
          if (vertexMap[vertexIndex] != kInvalidCreativeWorldLayoutIndex) {
            continue;
          }
          CreativeWorldLayoutTopologyVertex vertex =
              source.topologyVertices[vertexIndex];
          if (!offsetCoordinate(vertex.position.x, offset.x,
                                vertex.position.x) ||
              !offsetCoordinate(vertex.position.z, offset.z,
                                vertex.position.z)) {
            return false;
          }
          vertex.stableKey = mintCreativeWorldLayoutStableKey(
              output, nextStableOrdinal, "room_vertex");
          vertexMap[vertexIndex] = output.topologyVertices.size();
          output.topologyVertices.push_back(std::move(vertex));
        }
        CreativeWorldLayoutTopologyEdge edge = sourceEdge;
        edge.startVertexIndex = vertexMap[sourceEdge.startVertexIndex];
        edge.endVertexIndex = vertexMap[sourceEdge.endVertexIndex];
        edge.stableKey = mintCreativeWorldLayoutStableKey(
            output, nextStableOrdinal, "room_edge");
        edgeMap[edgeIndex] = output.topologyEdges.size();
        output.topologyEdges.push_back(std::move(edge));
      }
      CreativeWorldLayoutRoomBoundary duplicateBoundary = boundary;
      duplicateBoundary.roomIndex = duplicateRoomIndex;
      duplicateBoundary.topologyEdgeIndex = edgeMap[edgeIndex];
      output.roomBoundaries.push_back(duplicateBoundary);
    }
    if (boundaryCount == 0U) {
      return false;
    }
  }

  for (const CreativeWorldLayoutOpening& sourceOpening : source.openings) {
    if (sourceOpening.hostKind !=
            CreativeWorldLayoutOpeningHostKind::RoomEdge ||
        sourceOpening.roomIndex != roomIndex) {
      continue;
    }
    CreativeWorldLayoutOpening opening = sourceOpening;
    opening.roomIndex = duplicateRoomIndex;
    if (opening.roomTopologyEdgeIndex != kInvalidCreativeWorldLayoutIndex) {
      if (opening.roomTopologyEdgeIndex >= edgeMap.size() ||
          edgeMap[opening.roomTopologyEdgeIndex] ==
              kInvalidCreativeWorldLayoutIndex) {
        return false;
      }
      opening.roomTopologyEdgeIndex =
          edgeMap[opening.roomTopologyEdgeIndex];
    }
    opening.stableKey =
        mintCreativeWorldLayoutStableKey(output, nextStableOrdinal, "opening");
    opening.name += " Copy";
    output.openings.push_back(std::move(opening));
  }

  static_cast<void>(refreshCreativeWorldLayoutBuildingRoomFootprint(
      output, original.buildingIndex));
  return validCreativeWorldLayoutBuildingOwnership(output) &&
         buildCreativeWorldLayoutRoomGraph(output).accepted &&
         validCreativeWorldLayoutOpenings(output);
}

}  // namespace

CreativeWorldLayoutSourceDuplicatePolicy
creativeWorldLayoutSourceDuplicatePolicy(
    CreativeWorldLayoutTable table) noexcept {
  switch (table) {
    case CreativeWorldLayoutTable::Building:
    case CreativeWorldLayoutTable::Level:
      return CreativeWorldLayoutSourceDuplicatePolicy::SpecializedOwner;
    case CreativeWorldLayoutTable::Room:
    case CreativeWorldLayoutTable::Box:
    case CreativeWorldLayoutTable::Opening:
    case CreativeWorldLayoutTable::Object:
    case CreativeWorldLayoutTable::RoofAperture:
      return CreativeWorldLayoutSourceDuplicatePolicy::OffsetCopy;
    case CreativeWorldLayoutTable::VerticalConnector:
    case CreativeWorldLayoutTable::Wall:
    case CreativeWorldLayoutTable::TerrainProfile:
    case CreativeWorldLayoutTable::TerrainPath:
    case CreativeWorldLayoutTable::TerrainPathPoint:
    case CreativeWorldLayoutTable::TopologyEdge:
    case CreativeWorldLayoutTable::None:
      return CreativeWorldLayoutSourceDuplicatePolicy::Unsupported;
  }
  return CreativeWorldLayoutSourceDuplicatePolicy::Unsupported;
}

CreativeWorldLayoutSourceDuplicateResult duplicateCreativeWorldLayoutSource(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutSourceDuplicateRequest& request) {
  CreativeWorldLayoutSourceDuplicateResult result;
  result.requested = true;
  result.source = {request.table, request.sourceIndex};
  result.nextStableOrdinal = request.nextStableOrdinal;
  const CreativeWorldLayoutSourceDuplicatePolicy policy =
      creativeWorldLayoutSourceDuplicatePolicy(request.table);
  if (request.nextStableOrdinal == 0U ||
      request.sourceIndex == kInvalidCreativeWorldLayoutIndex ||
      !validSourceIndex(source, request.table, request.sourceIndex)) {
    reject(result, CreativeWorldLayoutSourceDuplicateStatus::InvalidRequest,
           "creative_world_layout_source_duplicate_request_invalid");
    return result;
  }
  if (request.useExplicitOffset && request.deltaXCells == 0 &&
      request.deltaZCells == 0) {
    reject(result, CreativeWorldLayoutSourceDuplicateStatus::InvalidRequest,
           "creative_world_layout_source_duplicate_offset_zero");
    return result;
  }
  if (policy != CreativeWorldLayoutSourceDuplicatePolicy::OffsetCopy) {
    reject(result,
           policy == CreativeWorldLayoutSourceDuplicatePolicy::Unsupported
               ? CreativeWorldLayoutSourceDuplicateStatus::UnsupportedSource
               : CreativeWorldLayoutSourceDuplicateStatus::InvalidRequest,
           policy == CreativeWorldLayoutSourceDuplicatePolicy::Unsupported
               ? "creative_world_layout_source_duplicate_unsupported"
               : "creative_world_layout_source_duplicate_specialized");
    return result;
  }
  if (request.table == CreativeWorldLayoutTable::Object) {
    const CreativeWorldLayoutObject& object =
        source.objects[request.sourceIndex];
    if (object.usesBridgeRecipe) {
      reject(result,
             CreativeWorldLayoutSourceDuplicateStatus::UnsupportedSource,
             "creative_world_layout_source_duplicate_bridge_unsupported");
      return result;
    }
    if (object.mode >= CreativeObjectLibraryPlacementMode::Count) {
      reject(result, CreativeWorldLayoutSourceDuplicateStatus::InvalidSource,
             "creative_world_layout_source_duplicate_object_invalid");
      return result;
    }
  }
  if (!validCreativeWorldLayoutBuildingOwnership(source) ||
      !validSourceRoomGraph(source) ||
      !validCreativeWorldLayoutOpenings(source)) {
    reject(result, CreativeWorldLayoutSourceDuplicateStatus::InvalidSource,
           "creative_world_layout_source_duplicate_source_invalid");
    return result;
  }

  std::int64_t stepX = 0;
  std::int64_t stepZ = 0;
  switch (request.table) {
    case CreativeWorldLayoutTable::Room: {
      const CreativeWorldLayoutRect footprint =
          source.rooms[request.sourceIndex].footprint;
      stepX = positiveSpan(footprint.minimum.x, footprint.maximum.x);
      stepZ = positiveSpan(footprint.minimum.z, footprint.maximum.z);
      break;
    }
    case CreativeWorldLayoutTable::Box: {
      const CreativeWorldLayoutRect footprint =
          source.boxes[request.sourceIndex].footprint;
      stepX = positiveSpan(footprint.minimum.x, footprint.maximum.x);
      stepZ = positiveSpan(footprint.minimum.z, footprint.maximum.z);
      break;
    }
    case CreativeWorldLayoutTable::Opening: {
      const CreativeWorldLayoutOpening& opening =
          source.openings[request.sourceIndex];
      stepX = positiveSpan(0.0, opening.widthCells);
      stepZ = stepX;
      break;
    }
    case CreativeWorldLayoutTable::RoofAperture: {
      const CreativeWorldLayoutRoofAperture& aperture =
          source.roofApertures[request.sourceIndex];
      stepX = positiveSpan(aperture.minimumXCells,
                           aperture.maximumXCells);
      stepZ = positiveSpan(aperture.minimumZCells,
                           aperture.maximumZCells);
      break;
    }
    case CreativeWorldLayoutTable::Object: {
      const CreativeWorldLayoutObject& object =
          source.objects[request.sourceIndex];
      if (object.mode == CreativeObjectLibraryPlacementMode::Bounds) {
        stepX = positiveSpan(object.boundsCells.min.x,
                             object.boundsCells.max.x);
        stepZ = positiveSpan(object.boundsCells.min.z,
                             object.boundsCells.max.z);
      }
      break;
    }
    case CreativeWorldLayoutTable::Building:
    case CreativeWorldLayoutTable::Level:
    case CreativeWorldLayoutTable::VerticalConnector:
    case CreativeWorldLayoutTable::Wall:
    case CreativeWorldLayoutTable::TerrainProfile:
    case CreativeWorldLayoutTable::TerrainPath:
    case CreativeWorldLayoutTable::TerrainPathPoint:
    case CreativeWorldLayoutTable::TopologyEdge:
    case CreativeWorldLayoutTable::None:
      break;
  }
  if (stepX < std::numeric_limits<std::int64_t>::max()) {
    ++stepX;
  }
  if (stepZ < std::numeric_limits<std::int64_t>::max()) {
    ++stepZ;
  }

  bool coordinateCandidateSeen = false;
  const bool duplicated = visitOffsets(
      request, stepX, stepZ, [&](GridOffset offset) {
        CreativeWorldLayout candidate;
        std::uint64_t nextStableOrdinal = request.nextStableOrdinal;
        std::size_t duplicateIndex = kInvalidCreativeWorldLayoutIndex;
        bool acceptedCandidate = false;
        if (!sourceCoordinatesAcceptOffset(source, request, offset)) {
          return false;
        }
        coordinateCandidateSeen = true;
        switch (request.table) {
          case CreativeWorldLayoutTable::Room:
            acceptedCandidate = cloneRoomTopology(
                source, request.sourceIndex, offset, candidate,
                nextStableOrdinal, duplicateIndex);
            break;
          case CreativeWorldLayoutTable::Box:
            acceptedCandidate = duplicateBoxAtOffset(
                source, request, offset, candidate, nextStableOrdinal);
            duplicateIndex = source.boxes.size();
            break;
          case CreativeWorldLayoutTable::Opening:
            acceptedCandidate = duplicateOpeningAtOffset(
                source, request, offset, candidate, nextStableOrdinal);
            duplicateIndex = source.openings.size();
            break;
          case CreativeWorldLayoutTable::Object:
            acceptedCandidate = duplicateObjectAtOffset(
                source, request, offset, candidate, nextStableOrdinal);
            duplicateIndex = source.objects.size();
            break;
          case CreativeWorldLayoutTable::RoofAperture:
            acceptedCandidate = duplicateRoofApertureAtOffset(
                source, request, offset, candidate, nextStableOrdinal);
            duplicateIndex = source.roofApertures.size();
            break;
          case CreativeWorldLayoutTable::Building:
          case CreativeWorldLayoutTable::Level:
          case CreativeWorldLayoutTable::VerticalConnector:
          case CreativeWorldLayoutTable::Wall:
          case CreativeWorldLayoutTable::TerrainProfile:
          case CreativeWorldLayoutTable::TerrainPath:
          case CreativeWorldLayoutTable::TerrainPathPoint:
          case CreativeWorldLayoutTable::TopologyEdge:
          case CreativeWorldLayoutTable::None:
            break;
        }
        if (!acceptedCandidate) {
          return false;
        }
        accept(result, std::move(candidate), duplicateIndex,
               nextStableOrdinal,
               "creative_world_layout_source_duplicate_ready");
        return true;
      });
  if (!duplicated) {
    reject(result,
           coordinateCandidateSeen
               ? CreativeWorldLayoutSourceDuplicateStatus::NoValidPlacement
               : CreativeWorldLayoutSourceDuplicateStatus::CoordinateOverflow,
           coordinateCandidateSeen
               ? "creative_world_layout_source_duplicate_no_valid_placement"
               : "creative_world_layout_source_duplicate_coordinate_overflow");
  }
  return result;
}

}  // namespace iggy3d::creative
