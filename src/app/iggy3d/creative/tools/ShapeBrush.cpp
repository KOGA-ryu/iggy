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

[[nodiscard]] bool validKind(CreativeShapeBrushKind kind) noexcept {
  return static_cast<std::size_t>(kind) <
         static_cast<std::size_t>(CreativeShapeBrushKind::Count);
}

[[nodiscard]] bool validAxis(CreativeShapeBrushAxis axis) noexcept {
  return static_cast<std::size_t>(axis) <
         static_cast<std::size_t>(CreativeShapeBrushAxis::Count);
}

[[nodiscard]] bool validOpening(
    CreativeVolumeHollowOpening opening) noexcept {
  return opening < CreativeVolumeHollowOpening::Count;
}

[[nodiscard]] bool validCornerRule(
    CreativeVolumeHollowCornerRule rule) noexcept {
  return rule < CreativeVolumeHollowCornerRule::Count;
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

enum class ShellDirection : std::uint8_t {
  NegativeX,
  PositiveX,
  NegativeY,
  PositiveY,
  NegativeZ,
  PositiveZ,
};

[[nodiscard]] constexpr std::uint8_t directionBit(
    ShellDirection direction) noexcept {
  return static_cast<std::uint8_t>(
      1U << static_cast<std::uint8_t>(direction));
}

[[nodiscard]] constexpr std::uint8_t openingMask(
    CreativeShapeBrushAxis axis,
    CreativeVolumeHollowOpening opening) noexcept {
  ShellDirection negative = ShellDirection::NegativeY;
  ShellDirection positive = ShellDirection::PositiveY;
  switch (axis) {
    case CreativeShapeBrushAxis::X:
      negative = ShellDirection::NegativeX;
      positive = ShellDirection::PositiveX;
      break;
    case CreativeShapeBrushAxis::Y:
      break;
    case CreativeShapeBrushAxis::Z:
      negative = ShellDirection::NegativeZ;
      positive = ShellDirection::PositiveZ;
      break;
    case CreativeShapeBrushAxis::Count:
      return 0U;
  }
  switch (opening) {
    case CreativeVolumeHollowOpening::Closed: return 0U;
    case CreativeVolumeHollowOpening::NegativeEnd:
      return directionBit(negative);
    case CreativeVolumeHollowOpening::PositiveEnd:
      return directionBit(positive);
    case CreativeVolumeHollowOpening::BothEnds:
      return static_cast<std::uint8_t>(directionBit(negative) |
                                       directionBit(positive));
    case CreativeVolumeHollowOpening::Count: break;
  }
  return 0U;
}

[[nodiscard]] std::uint8_t shellCorridorMask(
    CreativeShapeBrushKind kind,
    CreativeShapeBrushAxis axis,
    const InclusiveBounds& bounds,
    std::int64_t x,
    std::int64_t y,
    std::int64_t z,
    std::uint8_t thickness) noexcept {
  constexpr std::array<std::array<std::int64_t, 3>, 6> kNeighbors{{
      {{-1, 0, 0}}, {{1, 0, 0}}, {{0, -1, 0}},
      {{0, 1, 0}},  {{0, 0, -1}}, {{0, 0, 1}},
  }};
  std::uint8_t mask = 0U;
  for (std::size_t direction = 0U; direction < kNeighbors.size(); ++direction) {
    const auto& offset = kNeighbors[direction];
    for (std::uint8_t distance = 1U; distance <= thickness; ++distance) {
      const std::int64_t scale = distance;
      if (!insideShape(kind, axis, bounds, x + offset[0] * scale,
                       y + offset[1] * scale, z + offset[2] * scale)) {
        mask = static_cast<std::uint8_t>(mask | (1U << direction));
        break;
      }
    }
  }
  return mask;
}

[[nodiscard]] bool shellCell(const CreativeShapeBrushPlanRequest& request,
                             const InclusiveBounds& bounds,
                             std::int64_t x,
                             std::int64_t y,
                             std::int64_t z) noexcept {
  const std::uint8_t corridors = shellCorridorMask(
      request.kind, request.axis, bounds, x, y, z,
      request.shellThicknessCells);
  if (corridors == 0U) {
    return false;
  }
  const std::uint8_t open = openingMask(request.axis, request.shellOpening);
  if ((corridors & open) == 0U) {
    return true;
  }
  if (request.shellCornerRule ==
      CreativeVolumeHollowCornerRule::CutThrough) {
    return false;
  }
  return (corridors & static_cast<std::uint8_t>(~open)) != 0U;
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
    case CreativeShapeBrushPlanStatus::InvalidShell: return "InvalidShell";
    case CreativeShapeBrushPlanStatus::CandidateLimitExceeded:
      return "CandidateLimitExceeded";
    case CreativeShapeBrushPlanStatus::GeneratedLimitExceeded:
      return "GeneratedLimitExceeded";
    case CreativeShapeBrushPlanStatus::Planned: return "Planned";
  }
  return "Unknown";
}

CreativeShapeBrushPlanReceipt planCreativeShapeBrush(
    const CreativeShapeBrushPlanRequest& request) {
  CreativeShapeBrushPlanReceipt receipt;
  receipt.requested = true;
  receipt.kind = request.kind;
  receipt.axis = request.axis;
  receipt.hollow = request.hollow;
  receipt.shellThicknessCells = request.shellThicknessCells;
  receipt.shellOpening = request.shellOpening;
  receipt.shellCornerRule = request.shellCornerRule;

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
  if (request.hollow &&
      (request.kind == CreativeShapeBrushKind::Line ||
       request.shellThicknessCells == 0U ||
       !validOpening(request.shellOpening) ||
       !validCornerRule(request.shellCornerRule))) {
    reject(receipt, CreativeShapeBrushPlanStatus::InvalidShell,
           "creative_shape_brush_shell_invalid");
    return receipt;
  }

  const InclusiveBounds bounds =
      inclusiveBounds(request.firstCell, request.secondCell);
  if (request.hollow &&
      (bounds.width <= 2U * request.shellThicknessCells ||
       bounds.height <= 2U * request.shellThicknessCells ||
       bounds.depth <= 2U * request.shellThicknessCells)) {
    reject(receipt, CreativeShapeBrushPlanStatus::InvalidShell,
           "creative_shape_brush_shell_does_not_fit");
    return receipt;
  }
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
              (request.hollow && !shellCell(request, bounds, x, y, z))) {
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
