#include "app/iggy3d/creative/document/TerrainField.hpp"

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/TerrainFieldInternal.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace iggy3d::creative {

using terrain_field_internal::sampleTerrainHeightUnchecked;

namespace {

[[nodiscard]] bool tryTerrainCell(double worldX,
                                  double worldZ,
                                  CreativeVec3 gridOrigin,
                                  double cellSize,
                                  CreativeTerrainCoord2& output) noexcept {
  const double x = std::floor((worldX - gridOrigin.x) / cellSize);
  const double z = std::floor((worldZ - gridOrigin.z) / cellSize);
  constexpr double minimum =
      static_cast<double>(std::numeric_limits<std::int32_t>::min());
  constexpr double maximum =
      static_cast<double>(std::numeric_limits<std::int32_t>::max());
  if (!std::isfinite(x) || !std::isfinite(z) || x < minimum || x > maximum ||
      z < minimum || z > maximum) {
    return false;
  }
  output = {static_cast<std::int32_t>(x), static_cast<std::int32_t>(z)};
  return true;
}

struct TerrainAabbHit {
  bool hit = false;
  bool startInside = false;
  double distance = 0.0;
  CreativeVec3 normal{};
};

[[nodiscard]] TerrainAabbHit intersectTerrainColumn(
    CreativeVec3 origin,
    CreativeVec3 direction,
    CreativeVec3 minimum,
    CreativeVec3 maximum,
    double maxDistance) noexcept {
  TerrainAabbHit result;
  result.startInside = origin.x > minimum.x && origin.x < maximum.x &&
                       origin.y > minimum.y && origin.y < maximum.y &&
                       origin.z > minimum.z && origin.z < maximum.z;
  double nearDistance = 0.0;
  double farDistance = maxDistance;
  CreativeVec3 nearNormal{};
  constexpr double epsilon = 1.0e-12;
  const auto applyAxis = [&](double axisOrigin, double axisDirection,
                             double axisMinimum, double axisMaximum,
                             CreativeVec3 minimumNormal,
                             CreativeVec3 maximumNormal) {
    if (std::fabs(axisDirection) <= epsilon) {
      return axisOrigin >= axisMinimum && axisOrigin <= axisMaximum;
    }
    double first = (axisMinimum - axisOrigin) / axisDirection;
    double second = (axisMaximum - axisOrigin) / axisDirection;
    CreativeVec3 firstNormal = minimumNormal;
    CreativeVec3 secondNormal = maximumNormal;
    if (first > second) {
      std::swap(first, second);
      std::swap(firstNormal, secondNormal);
    }
    if (first >= nearDistance) {
      nearDistance = first;
      nearNormal = firstNormal;
    }
    farDistance = std::min(farDistance, second);
    return nearDistance <= farDistance;
  };
  if (!applyAxis(origin.x, direction.x, minimum.x, maximum.x,
                 {-1.0, 0.0, 0.0}, {1.0, 0.0, 0.0}) ||
      !applyAxis(origin.y, direction.y, minimum.y, maximum.y,
                 {0.0, -1.0, 0.0}, {0.0, 1.0, 0.0}) ||
      !applyAxis(origin.z, direction.z, minimum.z, maximum.z,
                 {0.0, 0.0, -1.0}, {0.0, 0.0, 1.0}) ||
      farDistance < 0.0 || nearDistance > maxDistance) {
    return result;
  }
  result.hit = true;
  result.distance = result.startInside ? 0.0 : std::max(0.0, nearDistance);
  if (result.startInside) {
    const double absX = std::fabs(direction.x);
    const double absY = std::fabs(direction.y);
    const double absZ = std::fabs(direction.z);
    result.normal = absX >= absY && absX >= absZ
                        ? CreativeVec3{direction.x > 0.0 ? -1.0 : 1.0, 0.0,
                                       0.0}
                    : absY >= absZ
                        ? CreativeVec3{0.0, direction.y > 0.0 ? -1.0 : 1.0,
                                       0.0}
                        : CreativeVec3{0.0, 0.0,
                                       direction.z > 0.0 ? -1.0 : 1.0};
  } else {
    result.normal = nearNormal;
  }
  return result;
}

}  // namespace

std::string_view toString(CreativeTerrainRaycastStatus status) noexcept {
  switch (status) {
    case CreativeTerrainRaycastStatus::NotRequested:
      return "NotRequested";
    case CreativeTerrainRaycastStatus::InvalidField:
      return "InvalidField";
    case CreativeTerrainRaycastStatus::InvalidRequest:
      return "InvalidRequest";
    case CreativeTerrainRaycastStatus::TraversalLimitExceeded:
      return "TraversalLimitExceeded";
    case CreativeTerrainRaycastStatus::Miss:
      return "Miss";
    case CreativeTerrainRaycastStatus::Hit:
      return "Hit";
  }
  return "Unknown";
}

CreativeTerrainRaycastReceipt raycastCreativeTerrainField(
    const CreativeTerrainField& field,
    const CreativeTerrainRaycastRequest& request) noexcept {
  CreativeTerrainRaycastReceipt receipt;
  receipt.requested = true;
  if (!field.validateInvariants()) {
    receipt.status = CreativeTerrainRaycastStatus::InvalidField;
    receipt.reasonCode = "creative_terrain_raycast_field_invalid";
    return receipt;
  }
  const double directionLength =
      std::sqrt(request.rayDirection.x * request.rayDirection.x +
                request.rayDirection.y * request.rayDirection.y +
                request.rayDirection.z * request.rayDirection.z);
  if (!isFiniteCreativeVec3(request.rayOrigin) ||
      !isFiniteCreativeVec3(request.rayDirection) ||
      !isFiniteCreativeVec3(request.gridOrigin) ||
      !std::isfinite(request.cellSize) || request.cellSize <= 0.0 ||
      !std::isfinite(request.maxDistance) || request.maxDistance < 0.0 ||
      !std::isfinite(directionLength) || directionLength <= 1.0e-12 ||
      request.maxVisitedCells == 0U) {
    receipt.status = CreativeTerrainRaycastStatus::InvalidRequest;
    receipt.reasonCode = "creative_terrain_raycast_request_invalid";
    return receipt;
  }
  receipt.accepted = true;
  if (field.controls().empty()) {
    receipt.status = CreativeTerrainRaycastStatus::Miss;
    receipt.reasonCode = "creative_terrain_raycast_miss";
    return receipt;
  }

  const CreativeVec3 direction{request.rayDirection.x / directionLength,
                               request.rayDirection.y / directionLength,
                               request.rayDirection.z / directionLength};
  CreativeTerrainCoord2 cell{};
  if (!tryTerrainCell(request.rayOrigin.x, request.rayOrigin.z,
                      request.gridOrigin, request.cellSize, cell)) {
    receipt.accepted = false;
    receipt.status = CreativeTerrainRaycastStatus::InvalidRequest;
    receipt.reasonCode = "creative_terrain_raycast_origin_out_of_range";
    return receipt;
  }

  const std::int32_t stepX = direction.x > 0.0 ? 1 : direction.x < 0.0 ? -1 : 0;
  const std::int32_t stepZ = direction.z > 0.0 ? 1 : direction.z < 0.0 ? -1 : 0;
  const double infinity = std::numeric_limits<double>::infinity();
  const auto firstBoundaryDistance = [&](std::int32_t coordinate,
                                         std::int32_t step,
                                         double axisOrigin,
                                         double axisDirection,
                                         double gridAxisOrigin) {
    if (step == 0) {
      return infinity;
    }
    const double boundary =
        gridAxisOrigin +
        (static_cast<double>(coordinate) + (step > 0 ? 1.0 : 0.0)) *
            request.cellSize;
    return std::max(0.0, (boundary - axisOrigin) / axisDirection);
  };
  double nextX = firstBoundaryDistance(cell.x, stepX, request.rayOrigin.x,
                                       direction.x, request.gridOrigin.x);
  double nextZ = firstBoundaryDistance(cell.z, stepZ, request.rayOrigin.z,
                                       direction.z, request.gridOrigin.z);
  const double deltaX =
      stepX == 0 ? infinity : request.cellSize / std::fabs(direction.x);
  const double deltaZ =
      stepZ == 0 ? infinity : request.cellSize / std::fabs(direction.z);

  while (receipt.visitedCellCount < request.maxVisitedCells) {
    ++receipt.visitedCellCount;
    const CreativeTerrainHeightSample sample =
        sampleTerrainHeightUnchecked(field.controls(), cell);
    if (sample.present) {
      const CreativeVec3 minimum{
          request.gridOrigin.x + static_cast<double>(cell.x) * request.cellSize,
          request.gridOrigin.y,
          request.gridOrigin.z + static_cast<double>(cell.z) * request.cellSize};
      const CreativeVec3 maximum{
          minimum.x + request.cellSize,
          minimum.y + static_cast<double>(sample.heightCells) * request.cellSize,
          minimum.z + request.cellSize};
      const TerrainAabbHit hit = intersectTerrainColumn(
          request.rayOrigin, direction, minimum, maximum, request.maxDistance);
      if (hit.hit) {
        receipt.hit = true;
        receipt.startInside = hit.startInside;
        receipt.status = CreativeTerrainRaycastStatus::Hit;
        receipt.cell = cell;
        receipt.heightCells = sample.heightCells;
        receipt.distance = hit.distance;
        receipt.hitPoint = {request.rayOrigin.x + direction.x * hit.distance,
                            request.rayOrigin.y + direction.y * hit.distance,
                            request.rayOrigin.z + direction.z * hit.distance};
        receipt.faceNormal = hit.normal;
        receipt.reasonCode = "creative_terrain_raycast_hit";
        return receipt;
      }
    }

    const double nextDistance = std::min(nextX, nextZ);
    if (!std::isfinite(nextDistance) || nextDistance > request.maxDistance) {
      receipt.status = CreativeTerrainRaycastStatus::Miss;
      receipt.reasonCode = "creative_terrain_raycast_miss";
      return receipt;
    }
    if (nextX < nextZ) {
      if ((stepX > 0 && cell.x == std::numeric_limits<std::int32_t>::max()) ||
          (stepX < 0 && cell.x == std::numeric_limits<std::int32_t>::min())) {
        receipt.accepted = false;
        receipt.status = CreativeTerrainRaycastStatus::InvalidRequest;
        receipt.reasonCode = "creative_terrain_raycast_cell_overflow";
        return receipt;
      }
      cell.x += stepX;
      nextX += deltaX;
    } else if (nextZ < nextX) {
      if ((stepZ > 0 && cell.z == std::numeric_limits<std::int32_t>::max()) ||
          (stepZ < 0 && cell.z == std::numeric_limits<std::int32_t>::min())) {
        receipt.accepted = false;
        receipt.status = CreativeTerrainRaycastStatus::InvalidRequest;
        receipt.reasonCode = "creative_terrain_raycast_cell_overflow";
        return receipt;
      }
      cell.z += stepZ;
      nextZ += deltaZ;
    } else {
      if ((stepX > 0 && cell.x == std::numeric_limits<std::int32_t>::max()) ||
          (stepX < 0 && cell.x == std::numeric_limits<std::int32_t>::min()) ||
          (stepZ > 0 && cell.z == std::numeric_limits<std::int32_t>::max()) ||
          (stepZ < 0 && cell.z == std::numeric_limits<std::int32_t>::min())) {
        receipt.accepted = false;
        receipt.status = CreativeTerrainRaycastStatus::InvalidRequest;
        receipt.reasonCode = "creative_terrain_raycast_cell_overflow";
        return receipt;
      }
      cell.x += stepX;
      cell.z += stepZ;
      nextX += deltaX;
      nextZ += deltaZ;
    }
  }

  receipt.accepted = false;
  receipt.status = CreativeTerrainRaycastStatus::TraversalLimitExceeded;
  receipt.reasonCode = "creative_terrain_raycast_traversal_limit";
  return receipt;
}

}  // namespace iggy3d::creative
