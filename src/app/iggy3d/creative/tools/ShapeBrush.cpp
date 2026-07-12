#include "app/iggy3d/creative/tools/ShapeBrush.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <utility>

namespace iggy3d::creative {
namespace {

struct InclusiveBounds {
  std::int64_t minX = 0;
  std::int64_t minY = 0;
  std::int64_t minZ = 0;
  std::int64_t maxX = 0;
  std::int64_t maxY = 0;
  std::int64_t maxZ = 0;
  std::uint64_t width = 1;
  std::uint64_t height = 1;
  std::uint64_t depth = 1;
};

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

[[nodiscard]] bool validKind(CreativeShapeBrushKind kind) noexcept {
  return static_cast<std::size_t>(kind) <
         static_cast<std::size_t>(CreativeShapeBrushKind::Count);
}

[[nodiscard]] bool validAxis(CreativeShapeBrushAxis axis) noexcept {
  return static_cast<std::size_t>(axis) <
         static_cast<std::size_t>(CreativeShapeBrushAxis::Count);
}

[[nodiscard]] InclusiveBounds inclusiveBounds(
    CreativeGridCoord3 first,
    CreativeGridCoord3 second) noexcept {
  InclusiveBounds bounds;
  bounds.minX = std::min<std::int64_t>(first.x, second.x);
  bounds.minY = std::min<std::int64_t>(first.y, second.y);
  bounds.minZ = std::min<std::int64_t>(first.z, second.z);
  bounds.maxX = std::max<std::int64_t>(first.x, second.x);
  bounds.maxY = std::max<std::int64_t>(first.y, second.y);
  bounds.maxZ = std::max<std::int64_t>(first.z, second.z);
  bounds.width = static_cast<std::uint64_t>(bounds.maxX - bounds.minX) + 1U;
  bounds.height = static_cast<std::uint64_t>(bounds.maxY - bounds.minY) + 1U;
  bounds.depth = static_cast<std::uint64_t>(bounds.maxZ - bounds.minZ) + 1U;
  return bounds;
}

[[nodiscard]] bool checkedMultiply(std::uint64_t lhs,
                                   std::uint64_t rhs,
                                   std::uint64_t& output) noexcept {
  if (lhs != 0U && rhs > std::numeric_limits<std::uint64_t>::max() / lhs) {
    return false;
  }
  output = lhs * rhs;
  return true;
}

[[nodiscard]] bool boxCandidateCount(const InclusiveBounds& bounds,
                                     std::uint64_t& output) noexcept {
  std::uint64_t area = 0;
  return checkedMultiply(bounds.width, bounds.height, area) &&
         checkedMultiply(area, bounds.depth, output);
}

[[nodiscard]] bool cellLess(CreativeGridCoord3 lhs,
                            CreativeGridCoord3 rhs) noexcept {
  if (lhs.z != rhs.z) {
    return lhs.z < rhs.z;
  }
  if (lhs.y != rhs.y) {
    return lhs.y < rhs.y;
  }
  return lhs.x < rhs.x;
}

[[nodiscard]] double normalizedCenter(std::int64_t coordinate,
                                      std::int64_t minimum,
                                      std::uint64_t extent) noexcept {
  const double doubledOffset =
      static_cast<double>((coordinate - minimum) * 2 + 1);
  return doubledOffset / static_cast<double>(extent) - 1.0;
}

[[nodiscard]] bool insideBounds(const InclusiveBounds& bounds,
                                std::int64_t x,
                                std::int64_t y,
                                std::int64_t z) noexcept {
  return x >= bounds.minX && x <= bounds.maxX && y >= bounds.minY &&
         y <= bounds.maxY && z >= bounds.minZ && z <= bounds.maxZ;
}

[[nodiscard]] bool insideEllipsoid(const InclusiveBounds& bounds,
                                   std::int64_t x,
                                   std::int64_t y,
                                   std::int64_t z) noexcept {
  if (!insideBounds(bounds, x, y, z)) {
    return false;
  }
  const double nx = normalizedCenter(x, bounds.minX, bounds.width);
  const double ny = normalizedCenter(y, bounds.minY, bounds.height);
  const double nz = normalizedCenter(z, bounds.minZ, bounds.depth);
  constexpr double kBoundaryEpsilon = 1.0e-12;
  return nx * nx + ny * ny + nz * nz <= 1.0 + kBoundaryEpsilon;
}

[[nodiscard]] bool insideCylinder(const InclusiveBounds& bounds,
                                  CreativeShapeBrushAxis axis,
                                  std::int64_t x,
                                  std::int64_t y,
                                  std::int64_t z) noexcept {
  if (!insideBounds(bounds, x, y, z)) {
    return false;
  }
  const double nx = normalizedCenter(x, bounds.minX, bounds.width);
  const double ny = normalizedCenter(y, bounds.minY, bounds.height);
  const double nz = normalizedCenter(z, bounds.minZ, bounds.depth);
  double radialDistanceSquared = 0.0;
  switch (axis) {
    case CreativeShapeBrushAxis::X:
      radialDistanceSquared = ny * ny + nz * nz;
      break;
    case CreativeShapeBrushAxis::Y:
      radialDistanceSquared = nx * nx + nz * nz;
      break;
    case CreativeShapeBrushAxis::Z:
      radialDistanceSquared = nx * nx + ny * ny;
      break;
    case CreativeShapeBrushAxis::Count:
      return false;
  }
  constexpr double kBoundaryEpsilon = 1.0e-12;
  return radialDistanceSquared <= 1.0 + kBoundaryEpsilon;
}

[[nodiscard]] bool insideShape(CreativeShapeBrushKind kind,
                               CreativeShapeBrushAxis axis,
                               const InclusiveBounds& bounds,
                               std::int64_t x,
                               std::int64_t y,
                               std::int64_t z) noexcept {
  switch (kind) {
    case CreativeShapeBrushKind::Box:
      return insideBounds(bounds, x, y, z);
    case CreativeShapeBrushKind::Ellipsoid:
      return insideEllipsoid(bounds, x, y, z);
    case CreativeShapeBrushKind::Cylinder:
      return insideCylinder(bounds, axis, x, y, z);
    case CreativeShapeBrushKind::Line:
    case CreativeShapeBrushKind::Count:
      return false;
  }
  return false;
}

[[nodiscard]] bool boundaryCell(CreativeShapeBrushKind kind,
                                CreativeShapeBrushAxis axis,
                                const InclusiveBounds& bounds,
                                std::int64_t x,
                                std::int64_t y,
                                std::int64_t z) noexcept {
  constexpr std::array<std::array<std::int64_t, 3>, 6> kNeighbors{{
      {{-1, 0, 0}}, {{1, 0, 0}}, {{0, -1, 0}},
      {{0, 1, 0}},  {{0, 0, -1}}, {{0, 0, 1}},
  }};
  return std::any_of(kNeighbors.begin(), kNeighbors.end(),
                     [&](const auto& offset) {
                       return !insideShape(kind, axis, bounds, x + offset[0],
                                           y + offset[1], z + offset[2]);
                     });
}

void reject(CreativeShapeBrushPlanReceipt& receipt,
            CreativeShapeBrushPlanStatus status,
            std::string_view reasonCode) noexcept {
  receipt.accepted = false;
  receipt.status = status;
  receipt.generatedCellCount = 0;
  receipt.cells.clear();
  receipt.reasonCode = reasonCode;
}

[[nodiscard]] bool appendCell(CreativeShapeBrushPlanReceipt& receipt,
                              const CreativeShapeBrushPlanRequest& request,
                              CreativeGridCoord3 cell) {
  if (receipt.cells.size() >= request.maxGeneratedCellCount) {
    reject(receipt, CreativeShapeBrushPlanStatus::GeneratedLimitExceeded,
           "creative_shape_brush_generated_limit_exceeded");
    return false;
  }
  receipt.cells.push_back(cell);
  return true;
}

[[nodiscard]] bool planLine(const CreativeShapeBrushPlanRequest& request,
                            CreativeShapeBrushPlanReceipt& receipt) {
  CreativeGridCoord3 start = request.firstCell;
  CreativeGridCoord3 end = request.secondCell;
  if (cellLess(end, start)) {
    std::swap(start, end);
  }

  std::int64_t x = start.x;
  std::int64_t y = start.y;
  std::int64_t z = start.z;
  const std::int64_t endX = end.x;
  const std::int64_t endY = end.y;
  const std::int64_t endZ = end.z;
  const std::int64_t dx = std::llabs(endX - x);
  const std::int64_t dy = std::llabs(endY - y);
  const std::int64_t dz = std::llabs(endZ - z);
  const std::int64_t sx = x < endX ? 1 : -1;
  const std::int64_t sy = y < endY ? 1 : -1;
  const std::int64_t sz = z < endZ ? 1 : -1;

  if (dx >= dy && dx >= dz) {
    std::int64_t errorY = 2 * dy - dx;
    std::int64_t errorZ = 2 * dz - dx;
    for (std::int64_t step = 0; step <= dx; ++step) {
      if (!appendCell(receipt, request,
                      {static_cast<std::int32_t>(x),
                       static_cast<std::int32_t>(y),
                       static_cast<std::int32_t>(z)})) {
        return false;
      }
      if (errorY >= 0) {
        y += sy;
        errorY -= 2 * dx;
      }
      if (errorZ >= 0) {
        z += sz;
        errorZ -= 2 * dx;
      }
      errorY += 2 * dy;
      errorZ += 2 * dz;
      x += sx;
    }
    return true;
  }

  if (dy >= dx && dy >= dz) {
    std::int64_t errorX = 2 * dx - dy;
    std::int64_t errorZ = 2 * dz - dy;
    for (std::int64_t step = 0; step <= dy; ++step) {
      if (!appendCell(receipt, request,
                      {static_cast<std::int32_t>(x),
                       static_cast<std::int32_t>(y),
                       static_cast<std::int32_t>(z)})) {
        return false;
      }
      if (errorX >= 0) {
        x += sx;
        errorX -= 2 * dy;
      }
      if (errorZ >= 0) {
        z += sz;
        errorZ -= 2 * dy;
      }
      errorX += 2 * dx;
      errorZ += 2 * dz;
      y += sy;
    }
    return true;
  }

  std::int64_t errorX = 2 * dx - dz;
  std::int64_t errorY = 2 * dy - dz;
  for (std::int64_t step = 0; step <= dz; ++step) {
    if (!appendCell(receipt, request,
                    {static_cast<std::int32_t>(x),
                     static_cast<std::int32_t>(y),
                     static_cast<std::int32_t>(z)})) {
      return false;
    }
    if (errorX >= 0) {
      x += sx;
      errorX -= 2 * dz;
    }
    if (errorY >= 0) {
      y += sy;
      errorY -= 2 * dz;
    }
    errorX += 2 * dx;
    errorY += 2 * dy;
    z += sz;
  }
  return true;
}

}  // namespace

std::string_view toString(CreativeShapeBrushKind kind) noexcept {
  switch (kind) {
    case CreativeShapeBrushKind::Box: return "BOX";
    case CreativeShapeBrushKind::Line: return "LINE";
    case CreativeShapeBrushKind::Ellipsoid: return "ELLIPSOID";
    case CreativeShapeBrushKind::Cylinder: return "CYLINDER";
    case CreativeShapeBrushKind::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeShapeBrushAxis axis) noexcept {
  switch (axis) {
    case CreativeShapeBrushAxis::X: return "X";
    case CreativeShapeBrushAxis::Y: return "Y";
    case CreativeShapeBrushAxis::Z: return "Z";
    case CreativeShapeBrushAxis::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeShapeBrushPlanStatus status) noexcept {
  switch (status) {
    case CreativeShapeBrushPlanStatus::NotRequested: return "NotRequested";
    case CreativeShapeBrushPlanStatus::InvalidKind: return "InvalidKind";
    case CreativeShapeBrushPlanStatus::InvalidAxis: return "InvalidAxis";
    case CreativeShapeBrushPlanStatus::CandidateLimitExceeded:
      return "CandidateLimitExceeded";
    case CreativeShapeBrushPlanStatus::GeneratedLimitExceeded:
      return "GeneratedLimitExceeded";
    case CreativeShapeBrushPlanStatus::Planned: return "Planned";
  }
  return "Unknown";
}

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

CreativeShapeBrushPlanReceipt planCreativeShapeBrush(
    const CreativeShapeBrushPlanRequest& request) {
  CreativeShapeBrushPlanReceipt receipt;
  receipt.requested = true;
  receipt.kind = request.kind;
  receipt.axis = request.axis;
  receipt.hollow = request.hollow;

  if (!validKind(request.kind)) {
    reject(receipt, CreativeShapeBrushPlanStatus::InvalidKind,
           "creative_shape_brush_kind_invalid");
    return receipt;
  }
  if (!validAxis(request.axis)) {
    reject(receipt, CreativeShapeBrushPlanStatus::InvalidAxis,
           "creative_shape_brush_axis_invalid");
    return receipt;
  }

  const InclusiveBounds bounds =
      inclusiveBounds(request.firstCell, request.secondCell);
  if (request.kind == CreativeShapeBrushKind::Line) {
    const std::uint64_t dx = static_cast<std::uint64_t>(
        std::llabs(static_cast<std::int64_t>(request.secondCell.x) -
                   request.firstCell.x));
    const std::uint64_t dy = static_cast<std::uint64_t>(
        std::llabs(static_cast<std::int64_t>(request.secondCell.y) -
                   request.firstCell.y));
    const std::uint64_t dz = static_cast<std::uint64_t>(
        std::llabs(static_cast<std::int64_t>(request.secondCell.z) -
                   request.firstCell.z));
    receipt.candidateCellCount = std::max({dx, dy, dz}) + 1U;
  } else if (!boxCandidateCount(bounds, receipt.candidateCellCount)) {
    reject(receipt, CreativeShapeBrushPlanStatus::CandidateLimitExceeded,
           "creative_shape_brush_candidate_count_overflow");
    return receipt;
  }

  if (receipt.candidateCellCount > request.maxCandidateCellCount) {
    reject(receipt, CreativeShapeBrushPlanStatus::CandidateLimitExceeded,
           "creative_shape_brush_candidate_limit_exceeded");
    return receipt;
  }
  receipt.cells.reserve(static_cast<std::size_t>(std::min(
      receipt.candidateCellCount, request.maxGeneratedCellCount)));

  if (request.kind == CreativeShapeBrushKind::Line) {
    if (!planLine(request, receipt)) {
      return receipt;
    }
  } else {
    for (std::int64_t z = bounds.minZ; z <= bounds.maxZ; ++z) {
      for (std::int64_t y = bounds.minY; y <= bounds.maxY; ++y) {
        for (std::int64_t x = bounds.minX; x <= bounds.maxX; ++x) {
          if (!insideShape(request.kind, request.axis, bounds, x, y, z) ||
              (request.hollow &&
               !boundaryCell(request.kind, request.axis, bounds, x, y, z))) {
            continue;
          }
          if (!appendCell(receipt, request,
                          {static_cast<std::int32_t>(x),
                           static_cast<std::int32_t>(y),
                           static_cast<std::int32_t>(z)})) {
            return receipt;
          }
        }
      }
    }
  }

  receipt.accepted = true;
  receipt.status = CreativeShapeBrushPlanStatus::Planned;
  receipt.generatedCellCount = receipt.cells.size();
  receipt.reasonCode = "creative_shape_brush_planned";
  return receipt;
}

}  // namespace iggy3d::creative
