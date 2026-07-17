#include "app/iggy3d/creative/world/WorldLayoutBuildingOps.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>

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

bool validOpeningPose(CreativeBuildingOpeningPose pose) noexcept {
  switch (pose) {
    case CreativeBuildingOpeningPose::Closed:
    case CreativeBuildingOpeningPose::OpenFromStartNegativeNormal:
    case CreativeBuildingOpeningPose::OpenFromStartPositiveNormal:
    case CreativeBuildingOpeningPose::OpenFromEndNegativeNormal:
    case CreativeBuildingOpeningPose::OpenFromEndPositiveNormal:
      return true;
  }
  return false;
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

CreativeBuildingOpeningPose encodePose(bool usesStart,
                                       bool positiveNormal) noexcept {
  if (usesStart) {
    return positiveNormal
               ? CreativeBuildingOpeningPose::OpenFromStartPositiveNormal
               : CreativeBuildingOpeningPose::OpenFromStartNegativeNormal;
  }
  return positiveNormal
             ? CreativeBuildingOpeningPose::OpenFromEndPositiveNormal
             : CreativeBuildingOpeningPose::OpenFromEndNegativeNormal;
}

CreativeBuildingOpeningPose transformPose(
    CreativeBuildingOpeningPose pose,
    LayoutAxis sourceAxis,
    CreativeWorldLayoutBuildingTransformOperation operation,
    bool reverseStartEnd) noexcept {
  if (pose == CreativeBuildingOpeningPose::Closed) {
    return pose;
  }
  bool usesStart =
      pose == CreativeBuildingOpeningPose::OpenFromStartNegativeNormal ||
      pose == CreativeBuildingOpeningPose::OpenFromStartPositiveNormal;
  bool positiveNormal =
      pose == CreativeBuildingOpeningPose::OpenFromStartPositiveNormal ||
      pose == CreativeBuildingOpeningPose::OpenFromEndPositiveNormal;
  usesStart = reverseStartEnd ? !usesStart : usesStart;
  positiveNormal = transformFlipsPositiveNormal(sourceAxis, operation)
                       ? !positiveNormal
                       : positiveNormal;
  return encodePose(usesStart, positiveNormal);
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
  if (building.rootMode == CreativeBuildingRootMode::CreateRoom) {
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
  if (swapsAxes) {
    for (std::size_t index = 0U; index < source.levels.size(); ++index) {
      if (source.levels[index].buildingIndex != buildingIndex) {
        continue;
      }
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
    if (!validOpeningPose(sourceOpening.pose) ||
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
      transformedOpening.pose =
          transformPose(sourceOpening.pose, axis, operation, false);
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
    transformedOpening.pose =
        transformPose(sourceOpening.pose, edgeAxis(sourceOpening.roomEdge),
                      operation, edge.reversesCanonicalDirection);
  }
  return true;
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
  result.accepted = true;
  result.status = CreativeWorldLayoutBuildingTransformStatus::Ready;
  result.reasonCode = "creative_world_layout_building_transform_ready";
  return result;
}

}  // namespace iggy3d::creative
