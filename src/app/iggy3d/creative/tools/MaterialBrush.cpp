#include "app/iggy3d/creative/tools/ShapeBrush.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace iggy3d::creative {
namespace {

[[nodiscard]] bool validMaterialBrushShape(
    CreativeMaterialBrushShape shape) noexcept {
  return static_cast<std::size_t>(shape) <
         static_cast<std::size_t>(CreativeMaterialBrushShape::Count);
}

[[nodiscard]] bool validMaterialBrushSize(
    CreativeMaterialBrushSize size) noexcept {
  return static_cast<std::size_t>(size) <
         static_cast<std::size_t>(CreativeMaterialBrushSize::Count);
}

[[nodiscard]] bool validMaterialBrushFill(
    CreativeMaterialBrushFill fill) noexcept {
  return static_cast<std::size_t>(fill) <
         static_cast<std::size_t>(CreativeMaterialBrushFill::Count);
}

[[nodiscard]] bool validMaterialBrushGuide(
    CreativeMaterialBrushGuide guide) noexcept {
  return static_cast<std::size_t>(guide) <
         static_cast<std::size_t>(CreativeMaterialBrushGuide::Count);
}

[[nodiscard]] bool materialBrushGuideIncludesOffset(
    CreativeMaterialBrushGuide guide,
    std::int32_t dx,
    std::int32_t dy,
    std::int32_t dz) noexcept {
  switch (guide) {
    case CreativeMaterialBrushGuide::Free:
    case CreativeMaterialBrushGuide::LineX:
    case CreativeMaterialBrushGuide::LineY:
    case CreativeMaterialBrushGuide::LineZ:
      return true;
    case CreativeMaterialBrushGuide::PlaneX: return dx == 0;
    case CreativeMaterialBrushGuide::PlaneY: return dy == 0;
    case CreativeMaterialBrushGuide::PlaneZ: return dz == 0;
    case CreativeMaterialBrushGuide::Count: return false;
  }
  return false;
}

void rejectMaterialBrushStamp(CreativeMaterialBrushStampPlan& plan,
                              CreativeMaterialBrushStampStatus status,
                              std::string_view reasonCode) noexcept {
  plan.accepted = false;
  plan.status = status;
  plan.cellCount = 0U;
  plan.reasonCode = reasonCode;
}

[[nodiscard]] bool materialBrushFilledCellIncluded(
    CreativeMaterialBrushShape shape,
    CreativeAxis3 axis,
    CreativeMaterialBrushGuide guide,
    std::int32_t radius,
    std::int32_t dx,
    std::int32_t dy,
    std::int32_t dz) noexcept {
  if (dx < -radius || dx > radius || dy < -radius || dy > radius ||
      dz < -radius || dz > radius) {
    return false;
  }
  if (!materialBrushGuideIncludesOffset(guide, dx, dy, dz)) {
    return false;
  }
  const std::int64_t x = dx;
  const std::int64_t y = dy;
  const std::int64_t z = dz;
  const std::int64_t radiusSquared =
      static_cast<std::int64_t>(radius) * radius;
  switch (shape) {
    case CreativeMaterialBrushShape::Cube:
      return true;
    case CreativeMaterialBrushShape::Sphere:
      return x * x + y * y + z * z <= radiusSquared;
    case CreativeMaterialBrushShape::Cylinder:
      switch (axis) {
        case CreativeAxis3::X: return y * y + z * z <= radiusSquared;
        case CreativeAxis3::Y: return x * x + z * z <= radiusSquared;
        case CreativeAxis3::Z: return x * x + y * y <= radiusSquared;
        case CreativeAxis3::Count: return false;
      }
      return false;
    case CreativeMaterialBrushShape::Count:
      return false;
  }
  return false;
}

[[nodiscard]] bool materialBrushCellIncluded(
    CreativeMaterialBrushShape shape,
    CreativeAxis3 axis,
    CreativeMaterialBrushGuide guide,
    CreativeMaterialBrushFill fill,
    std::int32_t radius,
    std::int32_t dx,
    std::int32_t dy,
    std::int32_t dz) noexcept {
  if (!materialBrushFilledCellIncluded(shape, axis, guide, radius, dx, dy,
                                       dz)) {
    return false;
  }
  if (fill == CreativeMaterialBrushFill::Solid) {
    return true;
  }
  if (fill != CreativeMaterialBrushFill::Shell) {
    return false;
  }
  constexpr std::array<std::array<std::int32_t, 3>, 6> kNeighbors{{
      {{-1, 0, 0}}, {{1, 0, 0}}, {{0, -1, 0}},
      {{0, 1, 0}},  {{0, 0, -1}}, {{0, 0, 1}},
  }};
  return std::any_of(kNeighbors.begin(), kNeighbors.end(),
                     [&](const auto& offset) {
                       return !materialBrushFilledCellIncluded(
                           shape, axis, guide, radius, dx + offset[0],
                           dy + offset[1], dz + offset[2]);
                     });
}

[[nodiscard]] bool offsetCell(CreativeGridCoord3 center,
                              std::int32_t dx,
                              std::int32_t dy,
                              std::int32_t dz,
                              CreativeGridCoord3& output) noexcept {
  const std::int64_t x = static_cast<std::int64_t>(center.x) + dx;
  const std::int64_t y = static_cast<std::int64_t>(center.y) + dy;
  const std::int64_t z = static_cast<std::int64_t>(center.z) + dz;
  if (x < std::numeric_limits<std::int32_t>::min() ||
      x > std::numeric_limits<std::int32_t>::max() ||
      y < std::numeric_limits<std::int32_t>::min() ||
      y > std::numeric_limits<std::int32_t>::max() ||
      z < std::numeric_limits<std::int32_t>::min() ||
      z > std::numeric_limits<std::int32_t>::max()) {
    return false;
  }
  output = {static_cast<std::int32_t>(x), static_cast<std::int32_t>(y),
            static_cast<std::int32_t>(z)};
  return true;
}

}  // namespace

std::string_view toString(CreativeMaterialBrushShape shape) noexcept {
  switch (shape) {
    case CreativeMaterialBrushShape::Cube: return "CUBE";
    case CreativeMaterialBrushShape::Sphere: return "SPHERE";
    case CreativeMaterialBrushShape::Cylinder: return "CYLINDER";
    case CreativeMaterialBrushShape::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeMaterialBrushSize size) noexcept {
  switch (size) {
    case CreativeMaterialBrushSize::OneCell: return "1 CELL";
    case CreativeMaterialBrushSize::ThreeCells: return "3 CELLS";
    case CreativeMaterialBrushSize::FiveCells: return "5 CELLS";
    case CreativeMaterialBrushSize::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeMaterialBrushFill fill) noexcept {
  switch (fill) {
    case CreativeMaterialBrushFill::Solid: return "SOLID";
    case CreativeMaterialBrushFill::Shell: return "SHELL";
    case CreativeMaterialBrushFill::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeMaterialBrushMask mask) noexcept {
  switch (mask) {
    case CreativeMaterialBrushMask::AddOnly: return "ADD ONLY";
    case CreativeMaterialBrushMask::Replace: return "REPLACE";
    case CreativeMaterialBrushMask::Overwrite: return "OVERWRITE";
    case CreativeMaterialBrushMask::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeMaterialBrushGuide guide) noexcept {
  switch (guide) {
    case CreativeMaterialBrushGuide::Free: return "FREE";
    case CreativeMaterialBrushGuide::LineX: return "LINE X";
    case CreativeMaterialBrushGuide::LineY: return "LINE Y";
    case CreativeMaterialBrushGuide::LineZ: return "LINE Z";
    case CreativeMaterialBrushGuide::PlaneX: return "PLANE X";
    case CreativeMaterialBrushGuide::PlaneY: return "PLANE Y";
    case CreativeMaterialBrushGuide::PlaneZ: return "PLANE Z";
    case CreativeMaterialBrushGuide::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeMaterialBrushStampStatus status) noexcept {
  switch (status) {
    case CreativeMaterialBrushStampStatus::NotRequested:
      return "NotRequested";
    case CreativeMaterialBrushStampStatus::InvalidShape: return "InvalidShape";
    case CreativeMaterialBrushStampStatus::InvalidSize: return "InvalidSize";
    case CreativeMaterialBrushStampStatus::InvalidFill: return "InvalidFill";
    case CreativeMaterialBrushStampStatus::InvalidAxis: return "InvalidAxis";
    case CreativeMaterialBrushStampStatus::InvalidGuide:
      return "InvalidGuide";
    case CreativeMaterialBrushStampStatus::CoordinateOverflow:
      return "CoordinateOverflow";
    case CreativeMaterialBrushStampStatus::CapacityExceeded:
      return "CapacityExceeded";
    case CreativeMaterialBrushStampStatus::Planned: return "Planned";
  }
  return "Unknown";
}

std::uint8_t creativeMaterialBrushRadiusCells(
    CreativeMaterialBrushSize size) noexcept {
  switch (size) {
    case CreativeMaterialBrushSize::OneCell: return 0U;
    case CreativeMaterialBrushSize::ThreeCells: return 1U;
    case CreativeMaterialBrushSize::FiveCells: return 2U;
    case CreativeMaterialBrushSize::Count: break;
  }
  return 0U;
}

bool creativeMaterialBrushMaskAllows(CreativeMaterialBrushMask mask,
                                     bool occupied) noexcept {
  switch (mask) {
    case CreativeMaterialBrushMask::AddOnly: return !occupied;
    case CreativeMaterialBrushMask::Replace: return occupied;
    case CreativeMaterialBrushMask::Overwrite: return true;
    case CreativeMaterialBrushMask::Count: return false;
  }
  return false;
}

bool guideCreativeMaterialBrushCenter(CreativeMaterialBrushGuide guide,
                                      CreativeGridCoord3 anchor,
                                      CreativeGridCoord3 candidate,
                                      CreativeGridCoord3& output) noexcept {
  CreativeGridCoord3 constrained = candidate;
  switch (guide) {
    case CreativeMaterialBrushGuide::Free:
      break;
    case CreativeMaterialBrushGuide::LineX:
      constrained.y = anchor.y;
      constrained.z = anchor.z;
      break;
    case CreativeMaterialBrushGuide::LineY:
      constrained.x = anchor.x;
      constrained.z = anchor.z;
      break;
    case CreativeMaterialBrushGuide::LineZ:
      constrained.x = anchor.x;
      constrained.y = anchor.y;
      break;
    case CreativeMaterialBrushGuide::PlaneX:
      constrained.x = anchor.x;
      break;
    case CreativeMaterialBrushGuide::PlaneY:
      constrained.y = anchor.y;
      break;
    case CreativeMaterialBrushGuide::PlaneZ:
      constrained.z = anchor.z;
      break;
    case CreativeMaterialBrushGuide::Count:
      return false;
  }
  output = constrained;
  return true;
}

bool creativeMaterialBrushLineAxis(CreativeMaterialBrushGuide guide,
                                   CreativeAxis3& output) noexcept {
  switch (guide) {
    case CreativeMaterialBrushGuide::LineX:
      output = CreativeAxis3::X;
      return true;
    case CreativeMaterialBrushGuide::LineY:
      output = CreativeAxis3::Y;
      return true;
    case CreativeMaterialBrushGuide::LineZ:
      output = CreativeAxis3::Z;
      return true;
    case CreativeMaterialBrushGuide::Free:
    case CreativeMaterialBrushGuide::PlaneX:
    case CreativeMaterialBrushGuide::PlaneY:
    case CreativeMaterialBrushGuide::PlaneZ:
    case CreativeMaterialBrushGuide::Count:
      return false;
  }
  return false;
}

CreativeMaterialBrushStampPlan planCreativeMaterialBrushStamp(
    const CreativeMaterialBrushStampRequest& request) noexcept {
  CreativeMaterialBrushStampPlan plan;
  plan.requested = true;
  plan.shape = request.shape;
  plan.size = request.size;
  plan.fill = request.fill;
  plan.axis = request.axis;
  plan.guide = request.guide;
  plan.centerCell = request.centerCell;
  if (!validMaterialBrushShape(request.shape)) {
    rejectMaterialBrushStamp(plan,
                             CreativeMaterialBrushStampStatus::InvalidShape,
                             "creative_material_brush_shape_invalid");
    return plan;
  }
  if (!validMaterialBrushSize(request.size)) {
    rejectMaterialBrushStamp(plan, CreativeMaterialBrushStampStatus::InvalidSize,
                             "creative_material_brush_size_invalid");
    return plan;
  }
  if (!validMaterialBrushFill(request.fill)) {
    rejectMaterialBrushStamp(plan, CreativeMaterialBrushStampStatus::InvalidFill,
                             "creative_material_brush_fill_invalid");
    return plan;
  }
  if (!isValidCreativeAxis3(request.axis)) {
    rejectMaterialBrushStamp(plan, CreativeMaterialBrushStampStatus::InvalidAxis,
                             "creative_material_brush_axis_invalid");
    return plan;
  }
  if (!validMaterialBrushGuide(request.guide)) {
    rejectMaterialBrushStamp(
        plan, CreativeMaterialBrushStampStatus::InvalidGuide,
        "creative_material_brush_guide_invalid");
    return plan;
  }

  const std::int32_t radius =
      static_cast<std::int32_t>(creativeMaterialBrushRadiusCells(request.size));
  std::int32_t minDx = -radius;
  std::int32_t minDy = -radius;
  std::int32_t minDz = -radius;
  std::int32_t maxDx = radius;
  std::int32_t maxDy = radius;
  std::int32_t maxDz = radius;
  switch (request.guide) {
    case CreativeMaterialBrushGuide::Free:
    case CreativeMaterialBrushGuide::LineX:
    case CreativeMaterialBrushGuide::LineY:
    case CreativeMaterialBrushGuide::LineZ:
      break;
    case CreativeMaterialBrushGuide::PlaneX:
      minDx = 0;
      maxDx = 0;
      break;
    case CreativeMaterialBrushGuide::PlaneY:
      minDy = 0;
      maxDy = 0;
      break;
    case CreativeMaterialBrushGuide::PlaneZ:
      minDz = 0;
      maxDz = 0;
      break;
    case CreativeMaterialBrushGuide::Count:
      return plan;
  }
  if (!offsetCell(request.centerCell, minDx, minDy, minDz,
                  plan.minCell) ||
      !offsetCell(request.centerCell, maxDx, maxDy, maxDz, plan.maxCell)) {
    rejectMaterialBrushStamp(
        plan, CreativeMaterialBrushStampStatus::CoordinateOverflow,
        "creative_material_brush_coordinate_overflow");
    return plan;
  }

  for (std::int32_t dz = -radius; dz <= radius; ++dz) {
    for (std::int32_t dy = -radius; dy <= radius; ++dy) {
      for (std::int32_t dx = -radius; dx <= radius; ++dx) {
        if (!materialBrushCellIncluded(
                request.shape, request.axis, request.guide, request.fill,
                radius, dx, dy, dz)) {
          continue;
        }
        if (plan.cellCount >= plan.cells.size()) {
          rejectMaterialBrushStamp(
              plan, CreativeMaterialBrushStampStatus::CapacityExceeded,
              "creative_material_brush_capacity_exceeded");
          return plan;
        }
        CreativeGridCoord3 cell;
        if (!offsetCell(request.centerCell, dx, dy, dz, cell)) {
          rejectMaterialBrushStamp(
              plan, CreativeMaterialBrushStampStatus::CoordinateOverflow,
              "creative_material_brush_coordinate_overflow");
          return plan;
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
        plan.cells[plan.cellCount++] = cell;
      }
    }
  }

  plan.accepted = plan.cellCount > 0U;
  plan.status = plan.accepted ? CreativeMaterialBrushStampStatus::Planned
                              : CreativeMaterialBrushStampStatus::InvalidShape;
  plan.reasonCode = plan.accepted ? "creative_material_brush_planned"
                                  : "creative_material_brush_empty";
  return plan;
}

}  // namespace iggy3d::creative
