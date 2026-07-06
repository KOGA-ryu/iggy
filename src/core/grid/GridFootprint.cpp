#include "core/grid/GridFootprint.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace iggy3d {

namespace {

[[nodiscard]] bool validCellSize(float cellSizeMeters) noexcept {
  return std::isfinite(cellSizeMeters) && cellSizeMeters > 0.0F;
}

[[nodiscard]] bool finiteBounds(float minX,
                                float minZ,
                                float maxX,
                                float maxZ) noexcept {
  return std::isfinite(minX) && std::isfinite(minZ) && std::isfinite(maxX) &&
         std::isfinite(maxZ);
}

void setStatus(GridFootprintResult& result,
               GridFootprintStatus status,
               std::string_view reasonCode,
               bool ok = false) noexcept {
  result.status = status;
  result.reasonCode = reasonCode;
  result.ok = ok;
}

void setStatus(GridCellCoordResult& result,
               GridFootprintStatus status,
               std::string_view reasonCode,
               bool ok = false) noexcept {
  result.status = status;
  result.reasonCode = reasonCode;
  result.ok = ok;
}

[[nodiscard]] bool checkedInt32(double value, std::int32_t& out) noexcept {
  if (!std::isfinite(value) ||
      value < static_cast<double>(std::numeric_limits<std::int32_t>::min()) ||
      value > static_cast<double>(std::numeric_limits<std::int32_t>::max())) {
    return false;
  }
  out = static_cast<std::int32_t>(value);
  return true;
}

[[nodiscard]] bool containingMinCell(float value,
                                     float cellSizeMeters,
                                     std::int32_t& out) noexcept {
  return checkedInt32(
      std::floor(static_cast<double>(value) /
                 static_cast<double>(cellSizeMeters)),
      out);
}

[[nodiscard]] bool containingMaxCell(float value,
                                     float cellSizeMeters,
                                     std::int32_t& out) noexcept {
  return checkedInt32(
      std::ceil(static_cast<double>(value) /
                static_cast<double>(cellSizeMeters)),
      out);
}

[[nodiscard]] bool alignedCell(float value,
                               float cellSizeMeters,
                               float toleranceCells,
                               std::int32_t& out) noexcept {
  const double scaled =
      static_cast<double>(value) / static_cast<double>(cellSizeMeters);
  const double rounded = std::round(scaled);
  if (!std::isfinite(scaled) || !std::isfinite(rounded) ||
      std::fabs(scaled - rounded) >
          static_cast<double>(std::max(0.0F, toleranceCells))) {
    return false;
  }
  return checkedInt32(rounded, out);
}

}  // namespace

std::int64_t GridFootprint::width() const noexcept {
  return static_cast<std::int64_t>(maxCellXExclusive) -
         static_cast<std::int64_t>(minCellX);
}

std::int64_t GridFootprint::depth() const noexcept {
  return static_cast<std::int64_t>(maxCellZExclusive) -
         static_cast<std::int64_t>(minCellZ);
}

std::string_view toString(GridFootprintStatus status) noexcept {
  switch (status) {
    case GridFootprintStatus::Unknown:
      return "unknown";
    case GridFootprintStatus::Ok:
      return "ok";
    case GridFootprintStatus::InvalidCellSize:
      return "invalid_cell_size";
    case GridFootprintStatus::NonFiniteInput:
      return "non_finite_input";
    case GridFootprintStatus::OutOfRange:
      return "out_of_range";
    case GridFootprintStatus::EmptyFootprint:
      return "empty_footprint";
    case GridFootprintStatus::Misaligned:
      return "misaligned";
  }
  return "unknown";
}

GridFootprintResult gridFootprintForContainingBounds(
    float minX,
    float minZ,
    float maxX,
    float maxZ,
    float cellSizeMeters) noexcept {
  GridFootprintResult result;
  if (!validCellSize(cellSizeMeters)) {
    setStatus(result,
              GridFootprintStatus::InvalidCellSize,
              "grid_footprint_invalid_cell_size");
    return result;
  }
  if (!finiteBounds(minX, minZ, maxX, maxZ)) {
    setStatus(result,
              GridFootprintStatus::NonFiniteInput,
              "grid_footprint_non_finite_input");
    return result;
  }
  if (!(maxX > minX) || !(maxZ > minZ)) {
    setStatus(result,
              GridFootprintStatus::EmptyFootprint,
              "grid_footprint_empty");
    return result;
  }

  if (!containingMinCell(minX, cellSizeMeters, result.footprint.minCellX) ||
      !containingMinCell(minZ, cellSizeMeters, result.footprint.minCellZ) ||
      !containingMaxCell(maxX,
                         cellSizeMeters,
                         result.footprint.maxCellXExclusive) ||
      !containingMaxCell(maxZ,
                         cellSizeMeters,
                         result.footprint.maxCellZExclusive)) {
    setStatus(result,
              GridFootprintStatus::OutOfRange,
              "grid_footprint_out_of_range");
    return result;
  }
  if (result.footprint.width() <= 0 || result.footprint.depth() <= 0) {
    setStatus(result,
              GridFootprintStatus::EmptyFootprint,
              "grid_footprint_empty");
    return result;
  }

  setStatus(result, GridFootprintStatus::Ok, "grid_footprint_ok", true);
  return result;
}

GridFootprintResult gridFootprintForAlignedBounds(
    float minX,
    float minZ,
    float maxX,
    float maxZ,
    float cellSizeMeters,
    float toleranceCells) noexcept {
  GridFootprintResult result;
  if (!validCellSize(cellSizeMeters)) {
    setStatus(result,
              GridFootprintStatus::InvalidCellSize,
              "grid_footprint_invalid_cell_size");
    return result;
  }
  if (!finiteBounds(minX, minZ, maxX, maxZ) ||
      !std::isfinite(toleranceCells)) {
    setStatus(result,
              GridFootprintStatus::NonFiniteInput,
              "grid_footprint_non_finite_input");
    return result;
  }
  if (!(maxX > minX) || !(maxZ > minZ)) {
    setStatus(result,
              GridFootprintStatus::EmptyFootprint,
              "grid_footprint_empty");
    return result;
  }

  if (!alignedCell(minX,
                   cellSizeMeters,
                   toleranceCells,
                   result.footprint.minCellX) ||
      !alignedCell(minZ,
                   cellSizeMeters,
                   toleranceCells,
                   result.footprint.minCellZ) ||
      !alignedCell(maxX,
                   cellSizeMeters,
                   toleranceCells,
                   result.footprint.maxCellXExclusive) ||
      !alignedCell(maxZ,
                   cellSizeMeters,
                   toleranceCells,
                   result.footprint.maxCellZExclusive)) {
    setStatus(result,
              GridFootprintStatus::Misaligned,
              "grid_footprint_misaligned");
    return result;
  }
  if (result.footprint.width() <= 0 || result.footprint.depth() <= 0) {
    setStatus(result,
              GridFootprintStatus::EmptyFootprint,
              "grid_footprint_empty");
    return result;
  }

  setStatus(result, GridFootprintStatus::Ok, "grid_footprint_ok", true);
  return result;
}

GridCellCoordResult gridCellForPoint(float x,
                                     float z,
                                     float cellSizeMeters) noexcept {
  GridCellCoordResult result;
  if (!validCellSize(cellSizeMeters)) {
    setStatus(result,
              GridFootprintStatus::InvalidCellSize,
              "grid_cell_invalid_cell_size");
    return result;
  }
  if (!std::isfinite(x) || !std::isfinite(z)) {
    setStatus(result,
              GridFootprintStatus::NonFiniteInput,
              "grid_cell_non_finite_input");
    return result;
  }
  if (!containingMinCell(x, cellSizeMeters, result.cell.x) ||
      !containingMinCell(z, cellSizeMeters, result.cell.z)) {
    setStatus(result,
              GridFootprintStatus::OutOfRange,
              "grid_cell_out_of_range");
    return result;
  }

  setStatus(result, GridFootprintStatus::Ok, "grid_cell_ok", true);
  return result;
}

}  // namespace iggy3d
