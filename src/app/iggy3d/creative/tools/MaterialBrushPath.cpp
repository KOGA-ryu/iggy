#include "app/iggy3d/creative/tools/ShapeBrush.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace iggy3d::creative {
namespace {

[[nodiscard]] bool validMaterialBrushSymmetry(
    CreativeMaterialBrushSymmetry symmetry) noexcept {
  return static_cast<std::size_t>(symmetry) <
         static_cast<std::size_t>(CreativeMaterialBrushSymmetry::Count);
}

void rejectMaterialBrushSymmetry(
    CreativeMaterialBrushSymmetryPlan& plan,
    CreativeMaterialBrushSymmetryStatus status,
    std::string_view reasonCode) noexcept {
  plan.accepted = false;
  plan.status = status;
  plan.cellCount = 0U;
  plan.reasonCode = reasonCode;
}

[[nodiscard]] bool symmetryPlanContains(
    const CreativeMaterialBrushSymmetryPlan& plan,
    CreativeGridCoord3 cell) noexcept {
  for (std::size_t index = 0U; index < plan.cellCount; ++index) {
    if (plan.cells[index] == cell) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool appendSymmetryCell(
    CreativeMaterialBrushSymmetryPlan& plan,
    CreativeGridCoord3 cell,
    bool mirrored,
    std::size_t maxCellCount) noexcept {
  if (symmetryPlanContains(plan, cell)) {
    return true;
  }
  if (plan.cellCount >= maxCellCount ||
      plan.cellCount >= plan.cells.size()) {
    return false;
  }
  if (plan.cellCount == 0U) {
    plan.minCell = cell;
    plan.maxCell = cell;
  } else {
    plan.minCell.x = std::min(plan.minCell.x, cell.x);
    plan.minCell.y = std::min(plan.minCell.y, cell.y);
    plan.minCell.z = std::min(plan.minCell.z, cell.z);
    plan.maxCell.x = std::max(plan.maxCell.x, cell.x);
    plan.maxCell.y = std::max(plan.maxCell.y, cell.y);
    plan.maxCell.z = std::max(plan.maxCell.z, cell.z);
  }
  plan.cells[plan.cellCount] = cell;
  plan.mirrored[plan.cellCount] = mirrored ? 1U : 0U;
  ++plan.cellCount;
  return true;
}

[[nodiscard]] bool reflectCoordinate(std::int32_t pivot,
                                     std::int32_t value,
                                     std::int32_t& output) noexcept {
  const std::int64_t reflected =
      2LL * static_cast<std::int64_t>(pivot) -
      static_cast<std::int64_t>(value);
  if (reflected < std::numeric_limits<std::int32_t>::min() ||
      reflected > std::numeric_limits<std::int32_t>::max()) {
    return false;
  }
  output = static_cast<std::int32_t>(reflected);
  return true;
}

[[nodiscard]] bool reflectGridCell(CreativeGridCoord3 source,
                                   CreativeGridCoord3 pivot,
                                   bool reflectX,
                                   bool reflectY,
                                   bool reflectZ,
                                   CreativeGridCoord3& output) noexcept {
  output = source;
  return (!reflectX || reflectCoordinate(pivot.x, source.x, output.x)) &&
         (!reflectY || reflectCoordinate(pivot.y, source.y, output.y)) &&
         (!reflectZ || reflectCoordinate(pivot.z, source.z, output.z));
}

[[nodiscard]] std::uint64_t coordinateDelta(std::int32_t from,
                                            std::int32_t to) noexcept {
  const std::int64_t wideFrom = from;
  const std::int64_t wideTo = to;
  return static_cast<std::uint64_t>(
      wideFrom < wideTo ? wideTo - wideFrom : wideFrom - wideTo);
}

void rejectMaterialBrushPath(CreativeMaterialBrushPathPlan& plan,
                             CreativeMaterialBrushPathStatus status,
                             std::string_view reasonCode) noexcept {
  plan.accepted = false;
  plan.status = status;
  plan.centerCount = 0U;
  plan.reasonCode = reasonCode;
}

[[nodiscard]] bool appendMaterialBrushPathCenter(
    CreativeMaterialBrushPathPlan& plan,
    const CreativeMaterialBrushPathRequest& request,
    const std::array<std::int64_t, 3>& center) noexcept {
  if (plan.centerCount >= request.maxCenterCount ||
      plan.centerCount >= plan.centers.size()) {
    rejectMaterialBrushPath(
        plan, CreativeMaterialBrushPathStatus::CapacityExceeded,
        "creative_material_brush_path_capacity_exceeded");
    return false;
  }
  plan.centers[plan.centerCount++] = {
      static_cast<std::int32_t>(center[0]),
      static_cast<std::int32_t>(center[1]),
      static_cast<std::int32_t>(center[2])};
  return true;
}

}  // namespace

std::string_view toString(
    CreativeMaterialBrushSymmetry symmetry) noexcept {
  switch (symmetry) {
    case CreativeMaterialBrushSymmetry::Off: return "OFF";
    case CreativeMaterialBrushSymmetry::MirrorX: return "MIRROR X";
    case CreativeMaterialBrushSymmetry::MirrorY: return "MIRROR Y";
    case CreativeMaterialBrushSymmetry::MirrorZ: return "MIRROR Z";
    case CreativeMaterialBrushSymmetry::MirrorXZ: return "MIRROR XZ";
    case CreativeMaterialBrushSymmetry::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeMaterialBrushPathStatus status) noexcept {
  switch (status) {
    case CreativeMaterialBrushPathStatus::NotRequested: return "NotRequested";
    case CreativeMaterialBrushPathStatus::InvalidLimit: return "InvalidLimit";
    case CreativeMaterialBrushPathStatus::CapacityExceeded:
      return "CapacityExceeded";
    case CreativeMaterialBrushPathStatus::Planned: return "Planned";
  }
  return "Unknown";
}

std::string_view toString(
    CreativeMaterialBrushSymmetryStatus status) noexcept {
  switch (status) {
    case CreativeMaterialBrushSymmetryStatus::NotRequested:
      return "NotRequested";
    case CreativeMaterialBrushSymmetryStatus::InvalidSymmetry:
      return "InvalidSymmetry";
    case CreativeMaterialBrushSymmetryStatus::InvalidLimit:
      return "InvalidLimit";
    case CreativeMaterialBrushSymmetryStatus::EmptyInput:
      return "EmptyInput";
    case CreativeMaterialBrushSymmetryStatus::CoordinateOverflow:
      return "CoordinateOverflow";
    case CreativeMaterialBrushSymmetryStatus::CapacityExceeded:
      return "CapacityExceeded";
    case CreativeMaterialBrushSymmetryStatus::Planned:
      return "Planned";
  }
  return "Unknown";
}

CreativeMaterialBrushSymmetryPlan planCreativeMaterialBrushSymmetry(
    const CreativeMaterialBrushSymmetryRequest& request) noexcept {
  CreativeMaterialBrushSymmetryPlan plan;
  plan.requested = true;
  plan.symmetry = request.symmetry;
  plan.pivot = request.pivot;
  if (!validMaterialBrushSymmetry(request.symmetry)) {
    rejectMaterialBrushSymmetry(
        plan, CreativeMaterialBrushSymmetryStatus::InvalidSymmetry,
        "creative_material_brush_symmetry_invalid");
    return plan;
  }
  if (request.maxCellCount == 0U ||
      request.maxCellCount > plan.cells.size()) {
    rejectMaterialBrushSymmetry(
        plan, CreativeMaterialBrushSymmetryStatus::InvalidLimit,
        "creative_material_brush_symmetry_limit_invalid");
    return plan;
  }
  if (request.sourceCells.empty()) {
    rejectMaterialBrushSymmetry(
        plan, CreativeMaterialBrushSymmetryStatus::EmptyInput,
        "creative_material_brush_symmetry_empty");
    return plan;
  }

  const std::size_t maxCellCount = request.maxCellCount;
  for (CreativeGridCoord3 cell : request.sourceCells) {
    if (!appendSymmetryCell(plan, cell, false, maxCellCount)) {
      rejectMaterialBrushSymmetry(
          plan, CreativeMaterialBrushSymmetryStatus::CapacityExceeded,
          "creative_material_brush_symmetry_capacity_exceeded");
      return plan;
    }
  }

  const auto appendReflection = [&](CreativeGridCoord3 source,
                                    bool reflectX,
                                    bool reflectY,
                                    bool reflectZ) {
    CreativeGridCoord3 reflected{};
    if (!reflectGridCell(source, request.pivot, reflectX, reflectY, reflectZ,
                         reflected)) {
      rejectMaterialBrushSymmetry(
          plan, CreativeMaterialBrushSymmetryStatus::CoordinateOverflow,
          "creative_material_brush_symmetry_coordinate_overflow");
      return false;
    }
    if (!appendSymmetryCell(plan, reflected, true, maxCellCount)) {
      rejectMaterialBrushSymmetry(
          plan, CreativeMaterialBrushSymmetryStatus::CapacityExceeded,
          "creative_material_brush_symmetry_capacity_exceeded");
      return false;
    }
    return true;
  };

  for (CreativeGridCoord3 source : request.sourceCells) {
    switch (request.symmetry) {
      case CreativeMaterialBrushSymmetry::Off:
        break;
      case CreativeMaterialBrushSymmetry::MirrorX:
        if (!appendReflection(source, true, false, false)) return plan;
        break;
      case CreativeMaterialBrushSymmetry::MirrorY:
        if (!appendReflection(source, false, true, false)) return plan;
        break;
      case CreativeMaterialBrushSymmetry::MirrorZ:
        if (!appendReflection(source, false, false, true)) return plan;
        break;
      case CreativeMaterialBrushSymmetry::MirrorXZ:
        if (!appendReflection(source, true, false, false) ||
            !appendReflection(source, false, false, true) ||
            !appendReflection(source, true, false, true)) {
          return plan;
        }
        break;
      case CreativeMaterialBrushSymmetry::Count:
        return plan;
    }
  }

  plan.accepted = true;
  plan.status = CreativeMaterialBrushSymmetryStatus::Planned;
  plan.reasonCode = "creative_material_brush_symmetry_planned";
  return plan;
}

CreativeMaterialBrushPathPlan planCreativeMaterialBrushPath(
    const CreativeMaterialBrushPathRequest& request) noexcept {
  CreativeMaterialBrushPathPlan plan;
  plan.requested = true;
  plan.fromCell = request.fromCell;
  plan.toCell = request.toCell;
  if (request.maxCenterCount == 0U ||
      request.maxCenterCount > kCreativeMaterialBrushPathCapacity) {
    rejectMaterialBrushPath(plan,
                            CreativeMaterialBrushPathStatus::InvalidLimit,
                            "creative_material_brush_path_limit_invalid");
    return plan;
  }

  constexpr std::uint8_t kAxisX = 1U << 0U;
  constexpr std::uint8_t kAxisY = 1U << 1U;
  constexpr std::uint8_t kAxisZ = 1U << 2U;
  constexpr std::array<std::uint8_t, 3> kAxisMasks{kAxisX, kAxisY, kAxisZ};
  const std::array<std::uint64_t, 3> deltas{
      coordinateDelta(request.fromCell.x, request.toCell.x),
      coordinateDelta(request.fromCell.y, request.toCell.y),
      coordinateDelta(request.fromCell.z, request.toCell.z)};
  const std::uint64_t longestAxis =
      std::max({deltas[0], deltas[1], deltas[2]});
  if (longestAxis + 1U > request.maxCenterCount) {
    rejectMaterialBrushPath(
        plan, CreativeMaterialBrushPathStatus::CapacityExceeded,
        "creative_material_brush_path_capacity_exceeded");
    return plan;
  }

  std::array<std::int64_t, 3> current{
      request.fromCell.x, request.fromCell.y, request.fromCell.z};
  const std::array<std::int64_t, 3> target{
      request.toCell.x, request.toCell.y, request.toCell.z};
  const std::array<std::int64_t, 3> steps{
      request.toCell.x < request.fromCell.x ? -1 : 1,
      request.toCell.y < request.fromCell.y ? -1 : 1,
      request.toCell.z < request.fromCell.z ? -1 : 1};
  std::array<std::uint64_t, 3> crossings{};
  if (!appendMaterialBrushPathCenter(plan, request, current)) {
    return plan;
  }

  while (current != target) {
    std::size_t minimumAxis = deltas[0] != crossings[0]
                                  ? 0U
                                  : deltas[1] != crossings[1] ? 1U : 2U;
    for (std::size_t axis = minimumAxis + 1U; axis < deltas.size(); ++axis) {
      if (deltas[axis] == crossings[axis]) {
        continue;
      }
      const std::uint64_t candidateNumerator = crossings[axis] * 2U + 1U;
      const std::uint64_t minimumNumerator =
          crossings[minimumAxis] * 2U + 1U;
      if (candidateNumerator * deltas[minimumAxis] <
          minimumNumerator * deltas[axis]) {
        minimumAxis = axis;
      }
    }

    const std::uint64_t minimumNumerator =
        crossings[minimumAxis] * 2U + 1U;
    std::uint8_t tiedAxes = 0U;
    for (std::size_t axis = 0U; axis < deltas.size(); ++axis) {
      if (deltas[axis] == crossings[axis]) {
        continue;
      }
      const std::uint64_t candidateNumerator = crossings[axis] * 2U + 1U;
      if (candidateNumerator * deltas[minimumAxis] ==
          minimumNumerator * deltas[axis]) {
        tiedAxes |= kAxisMasks[axis];
      }
    }

    for (std::uint8_t subset = 1U; subset <= 7U; ++subset) {
      if ((subset & static_cast<std::uint8_t>(~tiedAxes)) != 0U) {
        continue;
      }
      std::array<std::int64_t, 3> candidate = current;
      for (std::size_t axis = 0U; axis < candidate.size(); ++axis) {
        if ((subset & kAxisMasks[axis]) != 0U) {
          candidate[axis] += steps[axis];
        }
      }
      if (!appendMaterialBrushPathCenter(plan, request, candidate)) {
        return plan;
      }
    }
    for (std::size_t axis = 0U; axis < current.size(); ++axis) {
      if ((tiedAxes & kAxisMasks[axis]) != 0U) {
        current[axis] += steps[axis];
        ++crossings[axis];
      }
    }
  }

  plan.accepted = true;
  plan.status = CreativeMaterialBrushPathStatus::Planned;
  plan.reasonCode = "creative_material_brush_path_planned";
  return plan;
}

}  // namespace iggy3d::creative
