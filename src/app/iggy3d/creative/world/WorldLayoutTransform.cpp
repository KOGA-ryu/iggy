#include "app/iggy3d/creative/world/WorldLayoutBuildingOps.hpp"
#include "app/iggy3d/creative/world/WorldLayoutBlockout.hpp"
#include "app/iggy3d/creative/world/WorldLayoutOrthogonalRooms.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

namespace iggy3d::creative {
namespace {

enum class LayoutAxis : std::uint8_t { Horizontal, Vertical };

struct IntVector2 {
  std::int32_t x = 0;
  std::int32_t z = 0;
};

struct EdgeTransform {
  CreativeWorldLayoutRoomEdge edge = CreativeWorldLayoutRoomEdge::North;
  bool reversesCanonicalDirection = false;
};

void setFailure(CreativeWorldLayoutBuildingTransformResult& result,
                CreativeWorldLayoutBuildingTransformStatus status,
                std::string reasonCode) {
  result.accepted = false;
  result.status = status;
  result.transformed = {};
  result.reasonCode = std::move(reasonCode);
}

void includePoint(CreativeWorldLayoutBuildingBounds& bounds,
                  CreativeTerrainCoord2 point) noexcept {
  if (!bounds.valid) {
    bounds = {true, point, point};
    return;
  }
  bounds.minimum.x = std::min(bounds.minimum.x, point.x);
  bounds.minimum.z = std::min(bounds.minimum.z, point.z);
  bounds.maximum.x = std::max(bounds.maximum.x, point.x);
  bounds.maximum.z = std::max(bounds.maximum.z, point.z);
}

bool validOperation(
    CreativeWorldLayoutBuildingTransformOperation operation) noexcept {
  return operation < CreativeWorldLayoutBuildingTransformOperation::Count;
}

bool validRect(CreativeWorldLayoutRect rect) noexcept {
  return rect.minimum.x < rect.maximum.x && rect.minimum.z < rect.maximum.z;
}

bool validOpeningFacing(CreativeBuildingOpeningFacing facing) noexcept {
  return facing < CreativeBuildingOpeningFacing::Count;
}

bool checkedCoord(std::int64_t x,
                  std::int64_t z,
                  CreativeTerrainCoord2& output) noexcept {
  constexpr std::int64_t kMinimum = std::numeric_limits<std::int32_t>::min();
  constexpr std::int64_t kMaximum = std::numeric_limits<std::int32_t>::max();
  if (x < kMinimum || x > kMaximum || z < kMinimum || z > kMaximum) {
    return false;
  }
  output = {static_cast<std::int32_t>(x), static_cast<std::int32_t>(z)};
  return true;
}

IntVector2 transformVector(
    IntVector2 value,
    CreativeWorldLayoutBuildingTransformOperation operation) noexcept {
  switch (operation) {
    case CreativeWorldLayoutBuildingTransformOperation::RotateLeft90:
      return {value.z, static_cast<std::int32_t>(-value.x)};
    case CreativeWorldLayoutBuildingTransformOperation::RotateRight90:
      return {static_cast<std::int32_t>(-value.z), value.x};
    case CreativeWorldLayoutBuildingTransformOperation::MirrorX:
      return {static_cast<std::int32_t>(-value.x), value.z};
    case CreativeWorldLayoutBuildingTransformOperation::MirrorZ:
      return {value.x, static_cast<std::int32_t>(-value.z)};
    case CreativeWorldLayoutBuildingTransformOperation::Count:
      break;
  }
  return {};
}

IntVector2 verticalDirectionVector(
    CreativeWorldLayoutVerticalDirection direction) noexcept {
  switch (direction) {
  case CreativeWorldLayoutVerticalDirection::PositiveX:
    return {1, 0};
  case CreativeWorldLayoutVerticalDirection::NegativeX:
    return {-1, 0};
  case CreativeWorldLayoutVerticalDirection::PositiveZ:
    return {0, 1};
  case CreativeWorldLayoutVerticalDirection::NegativeZ:
    return {0, -1};
  case CreativeWorldLayoutVerticalDirection::Count:
    break;
  }
  return {};
}

CreativeWorldLayoutVerticalDirection
verticalDirectionFromVector(IntVector2 direction) noexcept {
  if (direction.x == 1 && direction.z == 0) {
    return CreativeWorldLayoutVerticalDirection::PositiveX;
  }
  if (direction.x == -1 && direction.z == 0) {
    return CreativeWorldLayoutVerticalDirection::NegativeX;
  }
  if (direction.x == 0 && direction.z == 1) {
    return CreativeWorldLayoutVerticalDirection::PositiveZ;
  }
  if (direction.x == 0 && direction.z == -1) {
    return CreativeWorldLayoutVerticalDirection::NegativeZ;
  }
  return CreativeWorldLayoutVerticalDirection::Count;
}

IntVector2 roofSlopeDirectionVector(
    CreativeStructuralRoofSlopeDirection direction) noexcept {
  switch (direction) {
    case CreativeStructuralRoofSlopeDirection::PositiveX:
      return {1, 0};
    case CreativeStructuralRoofSlopeDirection::NegativeX:
      return {-1, 0};
    case CreativeStructuralRoofSlopeDirection::PositiveZ:
      return {0, 1};
    case CreativeStructuralRoofSlopeDirection::NegativeZ:
      return {0, -1};
    case CreativeStructuralRoofSlopeDirection::Count:
      break;
  }
  return {};
}

CreativeStructuralRoofSlopeDirection roofSlopeDirectionFromVector(
    IntVector2 direction) noexcept {
  if (direction.x == 1 && direction.z == 0) {
    return CreativeStructuralRoofSlopeDirection::PositiveX;
  }
  if (direction.x == -1 && direction.z == 0) {
    return CreativeStructuralRoofSlopeDirection::NegativeX;
  }
  if (direction.x == 0 && direction.z == 1) {
    return CreativeStructuralRoofSlopeDirection::PositiveZ;
  }
  if (direction.x == 0 && direction.z == -1) {
    return CreativeStructuralRoofSlopeDirection::NegativeZ;
  }
  return CreativeStructuralRoofSlopeDirection::Count;
}

bool transformPoint(CreativeTerrainCoord2 point,
                    CreativeWorldLayoutBuildingBounds bounds,
                    CreativeWorldLayoutBuildingTransformOperation operation,
                    CreativeTerrainCoord2& output) noexcept {
  const std::int64_t minimumX = bounds.minimum.x;
  const std::int64_t minimumZ = bounds.minimum.z;
  const std::int64_t width =
      static_cast<std::int64_t>(bounds.maximum.x) - bounds.minimum.x;
  const std::int64_t depth =
      static_cast<std::int64_t>(bounds.maximum.z) - bounds.minimum.z;
  const std::int64_t localX = static_cast<std::int64_t>(point.x) - minimumX;
  const std::int64_t localZ = static_cast<std::int64_t>(point.z) - minimumZ;
  switch (operation) {
    case CreativeWorldLayoutBuildingTransformOperation::RotateLeft90:
      return checkedCoord(minimumX + localZ, minimumZ + width - localX, output);
    case CreativeWorldLayoutBuildingTransformOperation::RotateRight90:
      return checkedCoord(minimumX + depth - localZ, minimumZ + localX, output);
    case CreativeWorldLayoutBuildingTransformOperation::MirrorX:
      return checkedCoord(minimumX + width - localX, point.z, output);
    case CreativeWorldLayoutBuildingTransformOperation::MirrorZ:
      return checkedCoord(point.x, minimumZ + depth - localZ, output);
    case CreativeWorldLayoutBuildingTransformOperation::Count:
      break;
  }
  return false;
}

bool transformRect(CreativeWorldLayoutRect source,
                   CreativeWorldLayoutBuildingBounds bounds,
                   CreativeWorldLayoutBuildingTransformOperation operation,
                   CreativeWorldLayoutRect& output) noexcept {
  if (!validRect(source)) {
    return false;
  }
  const std::array<CreativeTerrainCoord2, 4U> corners{{
      source.minimum,
      {source.maximum.x, source.minimum.z},
      source.maximum,
      {source.minimum.x, source.maximum.z},
  }};
  CreativeWorldLayoutBuildingBounds transformed;
  for (CreativeTerrainCoord2 corner : corners) {
    CreativeTerrainCoord2 point;
    if (!transformPoint(corner, bounds, operation, point)) {
      return false;
    }
    includePoint(transformed, point);
  }
  output = {transformed.minimum, transformed.maximum};
  return validRect(output);
}

bool transformRoofAperture(
    const CreativeWorldLayoutRoofAperture& source,
    CreativeWorldLayoutBuildingBounds bounds,
    CreativeWorldLayoutBuildingTransformOperation operation,
    CreativeWorldLayoutRoofAperture& output) noexcept {
  const auto transform = [&](double x, double z, double& outputX,
                             double& outputZ) noexcept {
    if (!std::isfinite(x) || !std::isfinite(z)) {
      return false;
    }
    const double minimumX = bounds.minimum.x;
    const double minimumZ = bounds.minimum.z;
    const double width = static_cast<double>(bounds.maximum.x) - minimumX;
    const double depth = static_cast<double>(bounds.maximum.z) - minimumZ;
    const double localX = x - minimumX;
    const double localZ = z - minimumZ;
    switch (operation) {
      case CreativeWorldLayoutBuildingTransformOperation::RotateLeft90:
        outputX = minimumX + localZ;
        outputZ = minimumZ + width - localX;
        break;
      case CreativeWorldLayoutBuildingTransformOperation::RotateRight90:
        outputX = minimumX + depth - localZ;
        outputZ = minimumZ + localX;
        break;
      case CreativeWorldLayoutBuildingTransformOperation::MirrorX:
        outputX = minimumX + width - localX;
        outputZ = z;
        break;
      case CreativeWorldLayoutBuildingTransformOperation::MirrorZ:
        outputX = x;
        outputZ = minimumZ + depth - localZ;
        break;
      case CreativeWorldLayoutBuildingTransformOperation::Count:
        return false;
    }
    return std::isfinite(outputX) && std::isfinite(outputZ);
  };

  if (source.minimumXCells >= source.maximumXCells ||
      source.minimumZCells >= source.maximumZCells) {
    return false;
  }
  const std::array<std::pair<double, double>, 4U> corners{{
      {source.minimumXCells, source.minimumZCells},
      {source.maximumXCells, source.minimumZCells},
      {source.maximumXCells, source.maximumZCells},
      {source.minimumXCells, source.maximumZCells},
  }};
  double minimumX = std::numeric_limits<double>::infinity();
  double maximumX = -std::numeric_limits<double>::infinity();
  double minimumZ = std::numeric_limits<double>::infinity();
  double maximumZ = -std::numeric_limits<double>::infinity();
  for (const auto [x, z] : corners) {
    double transformedX = 0.0;
    double transformedZ = 0.0;
    if (!transform(x, z, transformedX, transformedZ)) {
      return false;
    }
    minimumX = std::min(minimumX, transformedX);
    maximumX = std::max(maximumX, transformedX);
    minimumZ = std::min(minimumZ, transformedZ);
    maximumZ = std::max(maximumZ, transformedZ);
  }
  output = source;
  output.minimumXCells = minimumX;
  output.maximumXCells = maximumX;
  output.minimumZCells = minimumZ;
  output.maximumZCells = maximumZ;
  return minimumX < maximumX && minimumZ < maximumZ;
}

bool wallAxis(const CreativeWorldLayoutWall& wall,
              LayoutAxis& output) noexcept {
  if (wall.start.z == wall.end.z && wall.start.x != wall.end.x) {
    output = LayoutAxis::Horizontal;
    return true;
  }
  if (wall.start.x == wall.end.x && wall.start.z != wall.end.z) {
    output = LayoutAxis::Vertical;
    return true;
  }
  return false;
}

bool segmentAxis(CreativeTerrainCoord2 start,
                 CreativeTerrainCoord2 end,
                 LayoutAxis& output) noexcept {
  if (start.z == end.z && start.x != end.x) {
    output = LayoutAxis::Horizontal;
    return true;
  }
  if (start.x == end.x && start.z != end.z) {
    output = LayoutAxis::Vertical;
    return true;
  }
  return false;
}

double segmentLength(CreativeTerrainCoord2 start,
                     CreativeTerrainCoord2 end,
                     LayoutAxis axis) noexcept {
  return axis == LayoutAxis::Horizontal
             ? std::fabs(static_cast<double>(end.x) - start.x)
             : std::fabs(static_cast<double>(end.z) - start.z);
}

LayoutAxis edgeAxis(CreativeWorldLayoutRoomEdge edge) noexcept {
  return edge == CreativeWorldLayoutRoomEdge::North ||
                 edge == CreativeWorldLayoutRoomEdge::South
             ? LayoutAxis::Horizontal
             : LayoutAxis::Vertical;
}

IntVector2 edgeOutward(CreativeWorldLayoutRoomEdge edge) noexcept {
  switch (edge) {
    case CreativeWorldLayoutRoomEdge::North:
      return {0, -1};
    case CreativeWorldLayoutRoomEdge::East:
      return {1, 0};
    case CreativeWorldLayoutRoomEdge::South:
      return {0, 1};
    case CreativeWorldLayoutRoomEdge::West:
      return {-1, 0};
    case CreativeWorldLayoutRoomEdge::Count:
      break;
  }
  return {};
}

IntVector2 edgeCanonicalDirection(CreativeWorldLayoutRoomEdge edge) noexcept {
  return edgeAxis(edge) == LayoutAxis::Horizontal ? IntVector2{1, 0}
                                                  : IntVector2{0, 1};
}

CreativeWorldLayoutRoomEdge edgeFromOutward(IntVector2 outward) noexcept {
  if (outward.x == 0 && outward.z == -1) {
    return CreativeWorldLayoutRoomEdge::North;
  }
  if (outward.x == 1 && outward.z == 0) {
    return CreativeWorldLayoutRoomEdge::East;
  }
  if (outward.x == 0 && outward.z == 1) {
    return CreativeWorldLayoutRoomEdge::South;
  }
  if (outward.x == -1 && outward.z == 0) {
    return CreativeWorldLayoutRoomEdge::West;
  }
  return CreativeWorldLayoutRoomEdge::Count;
}

EdgeTransform transformEdge(
    CreativeWorldLayoutRoomEdge edge,
    CreativeWorldLayoutBuildingTransformOperation operation) noexcept {
  const CreativeWorldLayoutRoomEdge transformedEdge =
      edgeFromOutward(transformVector(edgeOutward(edge), operation));
  const IntVector2 transformedDirection =
      transformVector(edgeCanonicalDirection(edge), operation);
  const IntVector2 canonicalDirection = edgeCanonicalDirection(transformedEdge);
  return {transformedEdge, transformedDirection.x != canonicalDirection.x ||
                               transformedDirection.z != canonicalDirection.z};
}

bool transformFlipsPositiveNormal(
    LayoutAxis sourceAxis,
    CreativeWorldLayoutBuildingTransformOperation operation) noexcept {
  const IntVector2 sourcePositiveNormal = sourceAxis == LayoutAxis::Horizontal
                                              ? IntVector2{0, 1}
                                              : IntVector2{1, 0};
  const IntVector2 transformed =
      transformVector(sourcePositiveNormal, operation);
  const LayoutAxis targetAxis =
      transformed.x == 0 ? LayoutAxis::Horizontal : LayoutAxis::Vertical;
  const IntVector2 targetPositiveNormal = targetAxis == LayoutAxis::Horizontal
                                              ? IntVector2{0, 1}
                                              : IntVector2{1, 0};
  return transformed.x != targetPositiveNormal.x ||
         transformed.z != targetPositiveNormal.z;
}

CreativeDoorSettings transformDoorSettings(
    CreativeDoorSettings settings,
    LayoutAxis sourceAxis,
    CreativeWorldLayoutBuildingTransformOperation operation) noexcept {
  const IntVector2 sourcePositiveAxis =
      sourceAxis == LayoutAxis::Horizontal ? IntVector2{1, 0}
                                           : IntVector2{0, 1};
  const IntVector2 transformedAxis =
      transformVector(sourcePositiveAxis, operation);
  const LayoutAxis targetAxis = transformedAxis.x == 0
                                    ? LayoutAxis::Horizontal
                                    : LayoutAxis::Vertical;
  const IntVector2 targetPositiveAxis =
      targetAxis == LayoutAxis::Horizontal ? IntVector2{1, 0}
                                           : IntVector2{0, 1};
  if (transformedAxis.x != targetPositiveAxis.x ||
      transformedAxis.z != targetPositiveAxis.z) {
    settings.hingeSide =
        settings.hingeSide == CreativeDoorHingeSide::MinimumEdge
            ? CreativeDoorHingeSide::MaximumEdge
            : CreativeDoorHingeSide::MinimumEdge;
  }
  if (transformFlipsPositiveNormal(sourceAxis, operation)) {
    settings.swingSide =
        settings.swingSide == CreativeDoorSwingSide::PositiveNormal
            ? CreativeDoorSwingSide::NegativeNormal
            : CreativeDoorSwingSide::PositiveNormal;
  }
  return settings;
}

CreativeBuildingOpeningFacing transformFacing(
    CreativeBuildingOpeningFacing facing, LayoutAxis sourceAxis,
    CreativeWorldLayoutBuildingTransformOperation operation) noexcept {
  if (!transformFlipsPositiveNormal(sourceAxis, operation)) {
    return facing;
  }
  return facing == CreativeBuildingOpeningFacing::PositiveNormal
             ? CreativeBuildingOpeningFacing::NegativeNormal
             : CreativeBuildingOpeningFacing::PositiveNormal;
}

double edgeLength(CreativeWorldLayoutRect room,
                  CreativeWorldLayoutRoomEdge edge) noexcept {
  return edgeAxis(edge) == LayoutAxis::Horizontal
             ? static_cast<double>(static_cast<std::int64_t>(room.maximum.x) -
                                   room.minimum.x)
             : static_cast<double>(static_cast<std::int64_t>(room.maximum.z) -
                                   room.minimum.z);
}

bool openingOwnedByBuilding(const CreativeWorldLayout& layout,
                            const CreativeWorldLayoutOpening& opening,
                            std::size_t buildingIndex) noexcept {
  if (opening.hostKind == CreativeWorldLayoutOpeningHostKind::Wall) {
    return layout.walls[opening.wallIndex].buildingIndex == buildingIndex;
  }
  return layout.rooms[opening.roomIndex].buildingIndex == buildingIndex;
}

bool transformOwnedGeometry(
    const CreativeWorldLayout& source,
    CreativeWorldLayout& candidate,
    std::size_t buildingIndex,
    CreativeWorldLayoutBuildingBounds bounds,
    CreativeWorldLayoutBuildingTransformOperation operation,
    CreativeWorldLayoutBuildingTransformStatus& failureStatus) noexcept {
  CreativeWorldLayoutBuilding& building = candidate.buildings[buildingIndex];
  if (building.rootMode == CreativeBuildingRootMode::CreateRoom ||
      (building.rootMode == CreativeBuildingRootMode::None &&
       building.groundingMode ==
           CreativeWorldLayoutGroundingMode::Foundation)) {
    const CreativeWorldLayoutRect sourceFootprint =
        source.buildings[buildingIndex].rootFootprint;
    if (!validRect(sourceFootprint)) {
      failureStatus =
          CreativeWorldLayoutBuildingTransformStatus::InvalidGeometry;
      return false;
    }
    if (!transformRect(sourceFootprint, bounds, operation,
                       building.rootFootprint)) {
      failureStatus =
          CreativeWorldLayoutBuildingTransformStatus::CoordinateOverflow;
      return false;
    }
  } else if (building.rootMode != CreativeBuildingRootMode::None) {
    failureStatus = CreativeWorldLayoutBuildingTransformStatus::InvalidGeometry;
    return false;
  }

  const bool swapsAxes =
      operation == CreativeWorldLayoutBuildingTransformOperation::RotateLeft90 ||
      operation == CreativeWorldLayoutBuildingTransformOperation::RotateRight90;
  for (std::size_t index = 0U; index < source.levels.size(); ++index) {
    if (source.levels[index].buildingIndex != buildingIndex) {
      continue;
    }
    CreativeStructuralRoofSlopeDirection& slopeDirection =
        candidate.levels[index].roofSlopeDirection;
    slopeDirection = roofSlopeDirectionFromVector(transformVector(
        roofSlopeDirectionVector(source.levels[index].roofSlopeDirection),
        operation));
    if (slopeDirection >= CreativeStructuralRoofSlopeDirection::Count) {
      failureStatus =
          CreativeWorldLayoutBuildingTransformStatus::InvalidGeometry;
      return false;
    }
    if (swapsAxes) {
      CreativeStructuralRoofRidgeAxis& axis =
          candidate.levels[index].roofRidgeAxis;
      if (axis == CreativeStructuralRoofRidgeAxis::X) {
        axis = CreativeStructuralRoofRidgeAxis::Z;
      } else if (axis == CreativeStructuralRoofRidgeAxis::Z) {
        axis = CreativeStructuralRoofRidgeAxis::X;
      } else {
        failureStatus =
            CreativeWorldLayoutBuildingTransformStatus::InvalidGeometry;
        return false;
      }
    }
  }

  for (std::size_t index = 0U; index < source.rooms.size(); ++index) {
    if (source.rooms[index].buildingIndex != buildingIndex) {
      continue;
    }
    if (!validRect(source.rooms[index].footprint)) {
      failureStatus =
          CreativeWorldLayoutBuildingTransformStatus::InvalidGeometry;
      return false;
    }
    if (!transformRect(source.rooms[index].footprint, bounds, operation,
                       candidate.rooms[index].footprint)) {
      failureStatus =
          CreativeWorldLayoutBuildingTransformStatus::CoordinateOverflow;
      return false;
    }
  }

  for (std::size_t index = 0U; index < source.roofApertures.size(); ++index) {
    const CreativeWorldLayoutRoofAperture& sourceAperture =
        source.roofApertures[index];
    if (source.levels[sourceAperture.levelIndex].buildingIndex !=
        buildingIndex) {
      continue;
    }
    if (!transformRoofAperture(sourceAperture, bounds, operation,
                               candidate.roofApertures[index])) {
      failureStatus =
          CreativeWorldLayoutBuildingTransformStatus::InvalidGeometry;
      return false;
    }
  }

  for (std::size_t index = 0U; index < source.topologyVertices.size();
       ++index) {
    const CreativeWorldLayoutTopologyVertex& sourceVertex =
        source.topologyVertices[index];
    if (source.levels[sourceVertex.levelIndex].buildingIndex !=
        buildingIndex) {
      continue;
    }
    if (!transformPoint(sourceVertex.position, bounds, operation,
                        candidate.topologyVertices[index].position)) {
      failureStatus =
          CreativeWorldLayoutBuildingTransformStatus::CoordinateOverflow;
      return false;
    }
  }

  std::vector<bool> topologyEdgeReversed(source.topologyEdges.size(), false);
  for (std::size_t index = 0U; index < source.topologyEdges.size(); ++index) {
    const CreativeWorldLayoutTopologyEdge& sourceEdge =
        source.topologyEdges[index];
    if (source.levels[sourceEdge.levelIndex].buildingIndex != buildingIndex) {
      continue;
    }
    CreativeWorldLayoutTopologyEdge& transformedEdge =
        candidate.topologyEdges[index];
    const CreativeTerrainCoord2 start =
        candidate.topologyVertices[transformedEdge.startVertexIndex].position;
    const CreativeTerrainCoord2 end =
        candidate.topologyVertices[transformedEdge.endVertexIndex].position;
    LayoutAxis transformedAxis;
    if (!segmentAxis(start, end, transformedAxis)) {
      failureStatus =
          CreativeWorldLayoutBuildingTransformStatus::InvalidGeometry;
      return false;
    }
    const bool reversed = transformedAxis == LayoutAxis::Horizontal
                              ? start.x > end.x
                              : start.z > end.z;
    topologyEdgeReversed[index] = reversed;
    if (!reversed) {
      continue;
    }
    std::swap(transformedEdge.startVertexIndex,
              transformedEdge.endVertexIndex);
    for (CreativeWorldLayoutRoomBoundary& boundary :
         candidate.roomBoundaries) {
      if (boundary.topologyEdgeIndex == index) {
        boundary.reversed = !boundary.reversed;
      }
    }
  }
  const bool reversesWinding =
      operation == CreativeWorldLayoutBuildingTransformOperation::MirrorX ||
      operation == CreativeWorldLayoutBuildingTransformOperation::MirrorZ;
  if (reversesWinding) {
    for (std::size_t roomIndex = 0U; roomIndex < source.rooms.size();
         ++roomIndex) {
      if (source.rooms[roomIndex].buildingIndex != buildingIndex) {
        continue;
      }
      const std::size_t boundaryCount =
          static_cast<std::size_t>(std::count_if(
              candidate.roomBoundaries.begin(),
              candidate.roomBoundaries.end(),
              [roomIndex](const CreativeWorldLayoutRoomBoundary& boundary) {
                return boundary.roomIndex == roomIndex;
              }));
      for (CreativeWorldLayoutRoomBoundary& boundary :
           candidate.roomBoundaries) {
        if (boundary.roomIndex != roomIndex) {
          continue;
        }
        boundary.order = boundaryCount - boundary.order - 1U;
        boundary.reversed = !boundary.reversed;
      }
    }
  }
  for (std::size_t index = 0U; index < source.verticalConnectors.size();
       ++index) {
    const CreativeWorldLayoutVerticalConnector& sourceConnector =
        source.verticalConnectors[index];
    if (sourceConnector.buildingIndex != buildingIndex) {
      continue;
    }
    if (!validRect(sourceConnector.footprint) ||
        sourceConnector.direction >=
            CreativeWorldLayoutVerticalDirection::Count) {
      failureStatus =
          CreativeWorldLayoutBuildingTransformStatus::InvalidGeometry;
      return false;
    }
    if (!transformRect(sourceConnector.footprint, bounds, operation,
                       candidate.verticalConnectors[index].footprint)) {
      failureStatus =
          CreativeWorldLayoutBuildingTransformStatus::CoordinateOverflow;
      return false;
    }
    const CreativeWorldLayoutVerticalDirection transformedDirection =
        verticalDirectionFromVector(transformVector(
            verticalDirectionVector(sourceConnector.direction), operation));
    if (transformedDirection >= CreativeWorldLayoutVerticalDirection::Count) {
      failureStatus =
          CreativeWorldLayoutBuildingTransformStatus::InvalidGeometry;
      return false;
    }
    candidate.verticalConnectors[index].direction = transformedDirection;
  }
  for (std::size_t index = 0U; index < source.boxes.size(); ++index) {
    if (source.boxes[index].buildingIndex != buildingIndex) {
      continue;
    }
    if (!validRect(source.boxes[index].footprint)) {
      failureStatus =
          CreativeWorldLayoutBuildingTransformStatus::InvalidGeometry;
      return false;
    }
    if (!transformRect(source.boxes[index].footprint, bounds, operation,
                       candidate.boxes[index].footprint)) {
      failureStatus =
          CreativeWorldLayoutBuildingTransformStatus::CoordinateOverflow;
      return false;
    }
  }
  for (std::size_t index = 0U; index < source.walls.size(); ++index) {
    const CreativeWorldLayoutWall& sourceWall = source.walls[index];
    if (sourceWall.buildingIndex != buildingIndex) {
      continue;
    }
    LayoutAxis axis;
    if (!wallAxis(sourceWall, axis)) {
      failureStatus =
          CreativeWorldLayoutBuildingTransformStatus::InvalidGeometry;
      return false;
    }
    if (!transformPoint(sourceWall.start, bounds, operation,
                        candidate.walls[index].start) ||
        !transformPoint(sourceWall.end, bounds, operation,
                        candidate.walls[index].end)) {
      failureStatus =
          CreativeWorldLayoutBuildingTransformStatus::CoordinateOverflow;
      return false;
    }
  }

  for (std::size_t index = 0U; index < source.openings.size(); ++index) {
    const CreativeWorldLayoutOpening& sourceOpening = source.openings[index];
    if (!openingOwnedByBuilding(source, sourceOpening, buildingIndex)) {
      continue;
    }
    if ((sourceOpening.kind == CreativeBuildingOpeningKind::Door &&
         !isValidCreativeDoorSettings(sourceOpening.door)) ||
        (sourceOpening.kind == CreativeBuildingOpeningKind::Window &&
         !isValidCreativeWindowSettings(sourceOpening.window)) ||
        !validOpeningFacing(sourceOpening.facing) ||
        !std::isfinite(sourceOpening.centerOffsetCells)) {
      failureStatus =
          CreativeWorldLayoutBuildingTransformStatus::InvalidGeometry;
      return false;
    }
    CreativeWorldLayoutOpening& transformedOpening = candidate.openings[index];
    if (sourceOpening.hostKind == CreativeWorldLayoutOpeningHostKind::Wall) {
      LayoutAxis axis;
      if (!wallAxis(source.walls[sourceOpening.wallIndex], axis)) {
        failureStatus =
            CreativeWorldLayoutBuildingTransformStatus::InvalidGeometry;
        return false;
      }
      transformedOpening.door =
          transformDoorSettings(sourceOpening.door, axis, operation);
      transformedOpening.facing =
          transformFacing(sourceOpening.facing, axis, operation);
      continue;
    }

    if (sourceOpening.roomTopologyEdgeIndex !=
        kInvalidCreativeWorldLayoutIndex) {
      const CreativeWorldLayoutTopologyEdge& sourceEdge =
          source.topologyEdges[sourceOpening.roomTopologyEdgeIndex];
      const CreativeTerrainCoord2 start =
          source.topologyVertices[sourceEdge.startVertexIndex].position;
      const CreativeTerrainCoord2 end =
          source.topologyVertices[sourceEdge.endVertexIndex].position;
      LayoutAxis axis;
      if (!segmentAxis(start, end, axis)) {
        failureStatus =
            CreativeWorldLayoutBuildingTransformStatus::InvalidGeometry;
        return false;
      }
      const bool reverseStartEnd =
          topologyEdgeReversed[sourceOpening.roomTopologyEdgeIndex];
      if (reverseStartEnd) {
        transformedOpening.centerOffsetCells =
            segmentLength(start, end, axis) -
            sourceOpening.centerOffsetCells;
      }
      transformedOpening.door =
          transformDoorSettings(sourceOpening.door, axis, operation);
      transformedOpening.facing =
          transformFacing(sourceOpening.facing, axis, operation);
      continue;
    }

    const CreativeWorldLayoutRoom& room = source.rooms[sourceOpening.roomIndex];
    if (!validRect(room.footprint)) {
      failureStatus =
          CreativeWorldLayoutBuildingTransformStatus::InvalidGeometry;
      return false;
    }
    const EdgeTransform edge = transformEdge(sourceOpening.roomEdge, operation);
    if (edge.edge >= CreativeWorldLayoutRoomEdge::Count) {
      failureStatus =
          CreativeWorldLayoutBuildingTransformStatus::InvalidGeometry;
      return false;
    }
    transformedOpening.roomEdge = edge.edge;
    if (edge.reversesCanonicalDirection) {
      transformedOpening.centerOffsetCells =
          edgeLength(room.footprint, sourceOpening.roomEdge) -
          sourceOpening.centerOffsetCells;
    }
    transformedOpening.door = transformDoorSettings(
        sourceOpening.door, edgeAxis(sourceOpening.roomEdge), operation);
    transformedOpening.facing = transformFacing(
        sourceOpening.facing, edgeAxis(sourceOpening.roomEdge), operation);
  }
  return true;
}

bool transformBlockoutRecipe(
    CreativeWorldLayoutBuildingBlockoutRecipe& recipe,
    CreativeWorldLayoutBuildingBounds sourceBounds,
    CreativeWorldLayoutBuildingTransformOperation operation) noexcept {
  CreativeWorldLayoutRect transformedFootprint;
  if (!transformRect(recipe.request.footprint, sourceBounds, operation,
                     transformedFootprint)) {
    return false;
  }
  recipe.request.footprint = transformedFootprint;

  const EdgeTransform entrance =
      transformEdge(recipe.request.facade.entranceEdge, operation);
  if (entrance.edge >= CreativeWorldLayoutRoomEdge::Count) {
    return false;
  }
  recipe.request.facade.entranceEdge = entrance.edge;
  if (entrance.reversesCanonicalDirection) {
    recipe.request.facade.entranceOffsetCells =
        -recipe.request.facade.entranceOffsetCells;
  }

  recipe.request.storeys.preferredDirection = verticalDirectionFromVector(
      transformVector(verticalDirectionVector(
                          recipe.request.storeys.preferredDirection),
                      operation));
  if (recipe.request.storeys.preferredDirection >=
      CreativeWorldLayoutVerticalDirection::Count) {
    return false;
  }
  recipe.roofSlopeDirection = roofSlopeDirectionFromVector(transformVector(
      roofSlopeDirectionVector(recipe.roofSlopeDirection), operation));
  if (recipe.roofSlopeDirection >=
      CreativeStructuralRoofSlopeDirection::Count) {
    return false;
  }

  const bool swapsAxes =
      operation ==
          CreativeWorldLayoutBuildingTransformOperation::RotateLeft90 ||
      operation ==
          CreativeWorldLayoutBuildingTransformOperation::RotateRight90;
  if (swapsAxes) {
    if (recipe.request.pattern ==
        CreativeWorldLayoutBuildingBlockoutPattern::SplitX) {
      recipe.request.pattern =
          CreativeWorldLayoutBuildingBlockoutPattern::SplitZ;
    } else if (recipe.request.pattern ==
               CreativeWorldLayoutBuildingBlockoutPattern::SplitZ) {
      recipe.request.pattern =
          CreativeWorldLayoutBuildingBlockoutPattern::SplitX;
    }
    recipe.roofRidgeAxis =
        recipe.roofRidgeAxis == CreativeStructuralRoofRidgeAxis::X
            ? CreativeStructuralRoofRidgeAxis::Z
            : CreativeStructuralRoofRidgeAxis::X;
  }
  return validCreativeWorldLayoutBuildingBlockoutRecipe(recipe);
}

}  // namespace

std::string_view toString(
    CreativeWorldLayoutBuildingTransformOperation operation) noexcept {
  switch (operation) {
    case CreativeWorldLayoutBuildingTransformOperation::RotateLeft90:
      return "RotateLeft90";
    case CreativeWorldLayoutBuildingTransformOperation::RotateRight90:
      return "RotateRight90";
    case CreativeWorldLayoutBuildingTransformOperation::MirrorX:
      return "MirrorX";
    case CreativeWorldLayoutBuildingTransformOperation::MirrorZ:
      return "MirrorZ";
    case CreativeWorldLayoutBuildingTransformOperation::Count:
      break;
  }
  return "Invalid";
}

std::string_view toString(
    CreativeWorldLayoutBuildingTransformStatus status) noexcept {
  switch (status) {
    case CreativeWorldLayoutBuildingTransformStatus::NotRequested:
      return "NotRequested";
    case CreativeWorldLayoutBuildingTransformStatus::InvalidRequest:
      return "InvalidRequest";
    case CreativeWorldLayoutBuildingTransformStatus::InvalidOwnership:
      return "InvalidOwnership";
    case CreativeWorldLayoutBuildingTransformStatus::EmptyBuilding:
      return "EmptyBuilding";
    case CreativeWorldLayoutBuildingTransformStatus::InvalidGeometry:
      return "InvalidGeometry";
    case CreativeWorldLayoutBuildingTransformStatus::CoordinateOverflow:
      return "CoordinateOverflow";
    case CreativeWorldLayoutBuildingTransformStatus::Ready:
      return "Ready";
  }
  return "Invalid";
}

CreativeWorldLayoutBuildingTransformResult transformCreativeWorldLayoutBuilding(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutBuildingTransformRequest& request) {
  CreativeWorldLayoutBuildingTransformResult result;
  result.requested = true;
  result.buildingIndex = request.buildingIndex;
  result.operation = request.operation;
  if (request.buildingIndex >= source.buildings.size() ||
      !validOperation(request.operation)) {
    setFailure(result,
               CreativeWorldLayoutBuildingTransformStatus::InvalidRequest,
               "creative_world_layout_building_transform_request_invalid");
    return result;
  }
  if (!validCreativeWorldLayoutBuildingOwnership(source)) {
    setFailure(result,
               CreativeWorldLayoutBuildingTransformStatus::InvalidOwnership,
               "creative_world_layout_building_transform_ownership_invalid");
    return result;
  }
  if (!measureCreativeWorldLayoutBuildingBounds(source, request.buildingIndex,
                                                result.sourceBounds)) {
    setFailure(result,
               CreativeWorldLayoutBuildingTransformStatus::EmptyBuilding,
               "creative_world_layout_building_transform_empty");
    return result;
  }

  CreativeWorldLayoutBuildingTemplateInstanceProvenance provenance =
      creativeWorldLayoutBuildingTemplateInstanceProvenance(
          source, request.buildingIndex);
  const CreativeWorldLayoutBuildingTemplateFingerprint sourceFingerprint =
      fingerprintCreativeWorldLayoutBuilding(source, request.buildingIndex);
  CreativeWorldLayoutBuildingBlockoutProvenance blockoutProvenance =
      creativeWorldLayoutBuildingBlockoutProvenance(source,
                                                    request.buildingIndex);
  const CreativeWorldLayoutBuildingBlockoutFingerprint
      sourceBlockoutFingerprint =
          fingerprintCreativeWorldLayoutBuildingBlockout(
              source, request.buildingIndex);

  result.transformed = source;
  CreativeWorldLayoutBuildingTransformStatus failureStatus =
      CreativeWorldLayoutBuildingTransformStatus::InvalidGeometry;
  if (!transformOwnedGeometry(source, result.transformed, request.buildingIndex,
                              result.sourceBounds, request.operation,
                              failureStatus)) {
    const char* reason =
        failureStatus ==
                CreativeWorldLayoutBuildingTransformStatus::CoordinateOverflow
            ? "creative_world_layout_building_transform_coordinate_overflow"
            : "creative_world_layout_building_transform_geometry_invalid";
    setFailure(result, failureStatus, reason);
    return result;
  }
  if (!validCreativeWorldLayoutBuildingOwnership(result.transformed) ||
      (!result.transformed.topologyEdges.empty() &&
       !buildCreativeWorldLayoutRoomGraph(result.transformed).accepted)) {
    setFailure(result,
               CreativeWorldLayoutBuildingTransformStatus::InvalidGeometry,
               "creative_world_layout_building_transform_topology_invalid");
    return result;
  }
  if (!measureCreativeWorldLayoutBuildingBounds(result.transformed,
                                                request.buildingIndex,
                                                result.transformedBounds)) {
    setFailure(result,
               CreativeWorldLayoutBuildingTransformStatus::InvalidGeometry,
               "creative_world_layout_building_transform_result_invalid");
    return result;
  }
  if (provenance.valid) {
    provenance.orientation =
        composeCreativeWorldLayoutBuildingTemplateOrientation(
            provenance.orientation, request.operation);
    provenance.anchor = result.transformedBounds.minimum;
    const CreativeWorldLayoutBuildingTemplateFingerprint transformedFingerprint =
        fingerprintCreativeWorldLayoutBuilding(result.transformed,
                                               request.buildingIndex);
    if (provenance.orientation >=
            CreativeWorldLayoutBuildingTemplateOrientation::Count ||
        !transformedFingerprint.valid) {
      setFailure(result,
                 CreativeWorldLayoutBuildingTransformStatus::InvalidGeometry,
                 "creative_world_layout_building_transform_provenance_invalid");
      return result;
    }
    if (sourceFingerprint.valid &&
        sourceFingerprint.value == provenance.instanceBaselineFingerprint) {
      provenance.instanceBaselineFingerprint = transformedFingerprint.value;
    }
    if (!setCreativeWorldLayoutBuildingTemplateInstanceProvenance(
            result.transformed, request.buildingIndex, provenance)) {
      setFailure(result,
                 CreativeWorldLayoutBuildingTransformStatus::InvalidGeometry,
                 "creative_world_layout_building_transform_provenance_invalid");
      return result;
    }
  }
  if (blockoutProvenance.valid) {
    if (!transformBlockoutRecipe(blockoutProvenance.recipe,
                                 result.sourceBounds, request.operation)) {
      setFailure(
          result, CreativeWorldLayoutBuildingTransformStatus::InvalidGeometry,
          "creative_world_layout_building_transform_blockout_provenance_"
          "invalid");
      return result;
    }
    result.transformed.buildings[request.buildingIndex].rootFootprint =
        blockoutProvenance.recipe.request.footprint;
    const CreativeWorldLayoutBuildingBlockoutFingerprint
        transformedFingerprint =
            fingerprintCreativeWorldLayoutBuildingBlockout(
                result.transformed, request.buildingIndex);
    if (!transformedFingerprint.valid) {
      setFailure(
          result, CreativeWorldLayoutBuildingTransformStatus::InvalidGeometry,
          "creative_world_layout_building_transform_blockout_fingerprint_"
          "invalid");
      return result;
    }
    if (sourceBlockoutFingerprint.valid &&
        sourceBlockoutFingerprint.value ==
            blockoutProvenance.instanceBaselineFingerprint) {
      blockoutProvenance.instanceBaselineFingerprint =
          transformedFingerprint.value;
    }
    if (!setCreativeWorldLayoutBuildingBlockoutProvenance(
            result.transformed, request.buildingIndex,
            blockoutProvenance)) {
      setFailure(
          result, CreativeWorldLayoutBuildingTransformStatus::InvalidGeometry,
          "creative_world_layout_building_transform_blockout_provenance_"
          "invalid");
      return result;
    }
  }
  result.accepted = true;
  result.status = CreativeWorldLayoutBuildingTransformStatus::Ready;
  result.reasonCode = "creative_world_layout_building_transform_ready";
  return result;
}

}  // namespace iggy3d::creative
