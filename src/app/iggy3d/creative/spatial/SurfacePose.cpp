#include "app/iggy3d/creative/spatial/SurfacePose.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include "app/iggy3d/creative/Geometry.hpp"

namespace iggy3d::creative {
namespace {

[[nodiscard]] bool worldCellCoordinate(double world,
                                       double origin,
                                       double cellSize,
                                       std::int32_t& output) noexcept {
  const double value = std::floor((world - origin) / cellSize);
  if (!std::isfinite(value) ||
      value < static_cast<double>(std::numeric_limits<std::int32_t>::min()) ||
      value > static_cast<double>(std::numeric_limits<std::int32_t>::max())) {
    return false;
  }
  output = static_cast<std::int32_t>(value);
  return true;
}

[[nodiscard]] double terrainCornerHeightCells(
    const CreativeTerrainField& field,
    std::int64_t cornerX,
    std::int64_t cornerZ) noexcept {
  constexpr std::array<std::array<std::int32_t, 2U>, 4U> offsets{{
      {-1, -1},
      {0, -1},
      {-1, 0},
      {0, 0},
  }};
  std::uint64_t heightSum = 0U;
  std::uint32_t count = 0U;
  for (const auto& offset : offsets) {
    const std::int64_t x = cornerX + offset[0];
    const std::int64_t z = cornerZ + offset[1];
    if (x < std::numeric_limits<std::int32_t>::min() ||
        x > std::numeric_limits<std::int32_t>::max() ||
        z < std::numeric_limits<std::int32_t>::min() ||
        z > std::numeric_limits<std::int32_t>::max()) {
      continue;
    }
    const CreativeTerrainHeightSample sample = sampleCreativeTerrainHeight(
        field, {static_cast<std::int32_t>(x), static_cast<std::int32_t>(z)});
    if (!sample.present) {
      continue;
    }
    heightSum += sample.heightCells;
    ++count;
  }
  return count == 0U
             ? 0.0
             : static_cast<double>(heightSum) / static_cast<double>(count);
}

}  // namespace

CreativeTerrainSurfacePose sampleCreativeTerrainSurfacePose(
    const CreativeTerrainSurfacePoseRequest& request) noexcept {
  CreativeTerrainSurfacePose result;
  result.requested = true;
  if (request.field == nullptr || !request.field->validateInvariants()) {
    result.status = CreativeTerrainSurfacePoseStatus::InvalidField;
    return result;
  }
  if (!isFiniteCreativeVec3(request.worldPoint) ||
      !isFiniteCreativeVec3(request.gridOrigin) ||
      !std::isfinite(request.cellSizeMeters) ||
      request.cellSizeMeters <= 0.0 ||
      !worldCellCoordinate(request.worldPoint.x, request.gridOrigin.x,
                           request.cellSizeMeters, result.cell.x) ||
      !worldCellCoordinate(request.worldPoint.z, request.gridOrigin.z,
                           request.cellSizeMeters, result.cell.z)) {
    result.status = CreativeTerrainSurfacePoseStatus::InvalidRequest;
    return result;
  }
  result.accepted = true;
  const CreativeTerrainHeightSample center =
      sampleCreativeTerrainHeight(*request.field, result.cell);
  if (!center.present) {
    result.status = CreativeTerrainSurfacePoseStatus::MissingSurface;
    return result;
  }

  const std::int64_t x = result.cell.x;
  const std::int64_t z = result.cell.z;
  const double h00 = terrainCornerHeightCells(*request.field, x, z);
  const double h10 = terrainCornerHeightCells(*request.field, x + 1, z);
  const double h11 = terrainCornerHeightCells(*request.field, x + 1, z + 1);
  const double h01 = terrainCornerHeightCells(*request.field, x, z + 1);
  const double cellMinX = request.gridOrigin.x +
                          static_cast<double>(result.cell.x) *
                              request.cellSizeMeters;
  const double cellMinZ = request.gridOrigin.z +
                          static_cast<double>(result.cell.z) *
                              request.cellSizeMeters;
  const double u = std::clamp(
      (request.worldPoint.x - cellMinX) / request.cellSizeMeters, 0.0, 1.0);
  const double v = std::clamp(
      (request.worldPoint.z - cellMinZ) / request.cellSizeMeters, 0.0, 1.0);
  const double heightCells =
      (1.0 - v) * ((1.0 - u) * h00 + u * h10) +
      v * ((1.0 - u) * h01 + u * h11);
  const double gradientX = (1.0 - v) * (h10 - h00) + v * (h11 - h01);
  const double gradientZ = (1.0 - u) * (h01 - h00) + u * (h11 - h10);
  const double normalLength =
      std::sqrt(gradientX * gradientX + 1.0 + gradientZ * gradientZ);
  if (!std::isfinite(heightCells) || !std::isfinite(normalLength) ||
      normalLength <= 0.0) {
    result.accepted = false;
    result.status = CreativeTerrainSurfacePoseStatus::InvalidRequest;
    return result;
  }
  result.position = {
      request.worldPoint.x,
      request.gridOrigin.y + heightCells * request.cellSizeMeters,
      request.worldPoint.z,
  };
  result.normal = {-gradientX / normalLength, 1.0 / normalLength,
                   -gradientZ / normalLength};
  result.slopeRadians = std::acos(std::clamp(result.normal.y, 0.0, 1.0));
  if (!isFiniteCreativeVec3(result.position) ||
      !isFiniteCreativeVec3(result.normal) ||
      !std::isfinite(result.slopeRadians)) {
    result.accepted = false;
    result.status = CreativeTerrainSurfacePoseStatus::InvalidRequest;
    return result;
  }
  result.present = true;
  result.status = CreativeTerrainSurfacePoseStatus::Ready;
  return result;
}

}  // namespace iggy3d::creative
