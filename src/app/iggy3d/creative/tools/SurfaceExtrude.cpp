#include "app/iggy3d/creative/tools/SurfaceExtrude.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>

#include "app/iggy3d/creative/document/VoxelField.hpp"

namespace iggy3d::creative {
namespace {

inline constexpr std::size_t kVisitedCapacity =
    kCreativeSurfaceExtrudeCapacity * 2U;
static_assert((kVisitedCapacity & (kVisitedCapacity - 1U)) == 0U);

struct VisitedCells {
  std::array<CreativeGridCoord3, kVisitedCapacity> cells{};
  std::array<std::uint8_t, kVisitedCapacity> occupied{};
};

[[nodiscard]] bool sameCell(CreativeGridCoord3 lhs,
                            CreativeGridCoord3 rhs) noexcept {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

[[nodiscard]] std::uint64_t coordinateHash(
    CreativeGridCoord3 cell) noexcept {
  std::uint64_t hash = 1469598103934665603ULL;
  for (std::int32_t component : {cell.x, cell.y, cell.z}) {
    hash ^= static_cast<std::uint32_t>(component);
    hash *= 1099511628211ULL;
  }
  return hash;
}

[[nodiscard]] bool markVisited(VisitedCells& visited,
                               CreativeGridCoord3 cell) noexcept {
  const std::size_t first =
      static_cast<std::size_t>(coordinateHash(cell)) &
      (kVisitedCapacity - 1U);
  for (std::size_t probe = 0U; probe < kVisitedCapacity; ++probe) {
    const std::size_t index = (first + probe) & (kVisitedCapacity - 1U);
    if (visited.occupied[index] == 0U) {
      visited.occupied[index] = 1U;
      visited.cells[index] = cell;
      return true;
    }
    if (sameCell(visited.cells[index], cell)) {
      return false;
    }
  }
  return false;
}

[[nodiscard]] bool validFaceOffset(CreativeGridCoord3 offset) noexcept {
  const bool x = offset.x == -1 || offset.x == 1;
  const bool y = offset.y == -1 || offset.y == 1;
  const bool z = offset.z == -1 || offset.z == 1;
  const std::uint8_t axisCount = static_cast<std::uint8_t>(x) +
                                 static_cast<std::uint8_t>(y) +
                                 static_cast<std::uint8_t>(z);
  return axisCount == 1U && (!x || (offset.y == 0 && offset.z == 0)) &&
         (!y || (offset.x == 0 && offset.z == 0)) &&
         (!z || (offset.x == 0 && offset.y == 0));
}

[[nodiscard]] bool validVoxelCell(CreativeGridCoord3 cell) noexcept {
  constexpr std::int32_t kMaximum =
      std::numeric_limits<std::int32_t>::max() - 1;
  return cell.x <= kMaximum && cell.y <= kMaximum && cell.z <= kMaximum;
}

[[nodiscard]] bool offsetCell(CreativeGridCoord3 cell,
                              CreativeGridCoord3 offset,
                              CreativeGridCoord3& output) noexcept {
  const std::int64_t x = static_cast<std::int64_t>(cell.x) + offset.x;
  const std::int64_t y = static_cast<std::int64_t>(cell.y) + offset.y;
  const std::int64_t z = static_cast<std::int64_t>(cell.z) + offset.z;
  constexpr std::int64_t kMinimum =
      std::numeric_limits<std::int32_t>::min();
  constexpr std::int64_t kMaximum =
      std::numeric_limits<std::int32_t>::max();
  if (x < kMinimum || x > kMaximum || y < kMinimum || y > kMaximum ||
      z < kMinimum || z > kMaximum) {
    return false;
  }
  output = {static_cast<std::int32_t>(x), static_cast<std::int32_t>(y),
            static_cast<std::int32_t>(z)};
  return true;
}

[[nodiscard]] bool stepCell(CreativeGridCoord3 cell,
                            CreativeGridCoord3 direction,
                            std::uint8_t distance,
                            CreativeGridCoord3& output) noexcept {
  output = cell;
  for (std::uint8_t step = 0U; step < distance; ++step) {
    if (!offsetCell(output, direction, output) || !validVoxelCell(output)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] std::array<CreativeGridCoord3, 4U> tangentOffsets(
    CreativeGridCoord3 outward) noexcept {
  if (outward.x != 0) {
    return {{{0, -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}}};
  }
  if (outward.y != 0) {
    return {{{-1, 0, 0}, {1, 0, 0}, {0, 0, -1}, {0, 0, 1}}};
  }
  return {{{-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0}}};
}

[[nodiscard]] bool exposedAt(const CreativeVoxelField& field,
                             CreativeGridCoord3 cell,
                             CreativeGridCoord3 outward) noexcept {
  CreativeGridCoord3 outside{};
  return !offsetCell(cell, outward, outside) ||
         field.materialAt(outside) == CreativeObjectKind::Unknown;
}

void includeMutationCell(CreativeSurfaceExtrudePlan& plan,
                         CreativeGridCoord3 cell) noexcept {
  if (plan.mutationCellCount == 0U) {
    plan.minMutationCell = cell;
    plan.maxMutationCell = cell;
    return;
  }
  plan.minMutationCell.x = std::min(plan.minMutationCell.x, cell.x);
  plan.minMutationCell.y = std::min(plan.minMutationCell.y, cell.y);
  plan.minMutationCell.z = std::min(plan.minMutationCell.z, cell.z);
  plan.maxMutationCell.x = std::max(plan.maxMutationCell.x, cell.x);
  plan.maxMutationCell.y = std::max(plan.maxMutationCell.y, cell.y);
  plan.maxMutationCell.z = std::max(plan.maxMutationCell.z, cell.z);
}

[[nodiscard]] CreativeSurfaceExtrudePlan reject(
    CreativeSurfaceExtrudePlan plan,
    CreativeSurfaceExtrudeStatus status,
    std::string_view reasonCode) noexcept {
  plan.accepted = false;
  plan.status = status;
  plan.surfaceCellCount = 0U;
  plan.mutationCellCount = 0U;
  plan.reasonCode = reasonCode;
  return plan;
}

}  // namespace

std::uint8_t creativeSurfaceExtrudeDepthCells(
    CreativeSurfaceExtrudeDepth depth) noexcept {
  constexpr std::array<std::uint8_t, 3U> depths{1U, 2U, 4U};
  const std::size_t index = static_cast<std::size_t>(depth);
  return index < depths.size() ? depths[index] : 0U;
}

std::string_view toString(CreativeSurfaceExtrudeDepth depth) noexcept {
  switch (depth) {
    case CreativeSurfaceExtrudeDepth::OneCell: return "1 CELL";
    case CreativeSurfaceExtrudeDepth::TwoCells: return "2 CELLS";
    case CreativeSurfaceExtrudeDepth::FourCells: return "4 CELLS";
    case CreativeSurfaceExtrudeDepth::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeSurfaceExtrudeKind kind) noexcept {
  switch (kind) {
    case CreativeSurfaceExtrudeKind::Extrude: return "Extrude";
    case CreativeSurfaceExtrudeKind::Inset: return "Inset";
    case CreativeSurfaceExtrudeKind::Count: break;
  }
  return "Unknown";
}

std::string_view toString(CreativeSurfaceExtrudeStatus status) noexcept {
  switch (status) {
    case CreativeSurfaceExtrudeStatus::NotRequested: return "NotRequested";
    case CreativeSurfaceExtrudeStatus::InvalidField: return "InvalidField";
    case CreativeSurfaceExtrudeStatus::InvalidFace: return "InvalidFace";
    case CreativeSurfaceExtrudeStatus::InvalidKind: return "InvalidKind";
    case CreativeSurfaceExtrudeStatus::InvalidDepth: return "InvalidDepth";
    case CreativeSurfaceExtrudeStatus::InvalidLimit: return "InvalidLimit";
    case CreativeSurfaceExtrudeStatus::EmptySeed: return "EmptySeed";
    case CreativeSurfaceExtrudeStatus::FaceOccluded: return "FaceOccluded";
    case CreativeSurfaceExtrudeStatus::CapacityExceeded:
      return "CapacityExceeded";
    case CreativeSurfaceExtrudeStatus::CoordinateOverflow:
      return "CoordinateOverflow";
    case CreativeSurfaceExtrudeStatus::DestinationOccupied:
      return "DestinationOccupied";
    case CreativeSurfaceExtrudeStatus::SourceMissing: return "SourceMissing";
    case CreativeSurfaceExtrudeStatus::Planned: return "Planned";
  }
  return "Unknown";
}

CreativeSurfaceExtrudePlan planCreativeSurfaceExtrude(
    const CreativeSurfaceExtrudeRequest& request) noexcept {
  CreativeSurfaceExtrudePlan plan;
  plan.requested = true;
  plan.kind = request.kind;
  plan.depth = request.depth;
  plan.affectedCellLimit = request.affectedCellLimit;
  plan.seedCell = request.seedCell;
  plan.outward = request.outward;

  if (request.field == nullptr || !request.field->isValid()) {
    return reject(plan, CreativeSurfaceExtrudeStatus::InvalidField,
                  "creative_surface_extrude_field_invalid");
  }
  if (!validFaceOffset(request.outward)) {
    return reject(plan, CreativeSurfaceExtrudeStatus::InvalidFace,
                  "creative_surface_extrude_face_invalid");
  }
  if (request.kind >= CreativeSurfaceExtrudeKind::Count) {
    return reject(plan, CreativeSurfaceExtrudeStatus::InvalidKind,
                  "creative_surface_extrude_kind_invalid");
  }
  const std::uint8_t depth = creativeSurfaceExtrudeDepthCells(request.depth);
  if (depth == 0U) {
    return reject(plan, CreativeSurfaceExtrudeStatus::InvalidDepth,
                  "creative_surface_extrude_depth_invalid");
  }
  const std::uint16_t affectedLimit =
      connectedFillCellLimit(request.affectedCellLimit);
  if (affectedLimit == 0U || affectedLimit > plan.mutationCells.size()) {
    return reject(plan, CreativeSurfaceExtrudeStatus::InvalidLimit,
                  "creative_surface_extrude_limit_invalid");
  }

  plan.sourceMaterial = request.field->materialAt(request.seedCell);
  if (plan.sourceMaterial == CreativeObjectKind::Unknown) {
    return reject(plan, CreativeSurfaceExtrudeStatus::EmptySeed,
                  "creative_surface_extrude_seed_empty");
  }
  if (!exposedAt(*request.field, request.seedCell, request.outward)) {
    return reject(plan, CreativeSurfaceExtrudeStatus::FaceOccluded,
                  "creative_surface_extrude_face_occluded");
  }

  const std::uint16_t surfaceLimit =
      static_cast<std::uint16_t>(affectedLimit / depth);
  const std::array<CreativeGridCoord3, 4U> tangents =
      tangentOffsets(request.outward);
  VisitedCells visited;
  static_cast<void>(markVisited(visited, request.seedCell));
  plan.surfaceCells[0] = request.seedCell;
  plan.surfaceCellCount = 1U;

  std::size_t readIndex = 0U;
  while (readIndex < plan.surfaceCellCount) {
    const CreativeGridCoord3 current = plan.surfaceCells[readIndex++];
    for (CreativeGridCoord3 tangent : tangents) {
      CreativeGridCoord3 candidate{};
      if (!offsetCell(current, tangent, candidate) ||
          request.field->materialAt(candidate) != plan.sourceMaterial ||
          !exposedAt(*request.field, candidate, request.outward) ||
          !markVisited(visited, candidate)) {
        continue;
      }
      if (plan.surfaceCellCount >= surfaceLimit) {
        return reject(plan, CreativeSurfaceExtrudeStatus::CapacityExceeded,
                      "creative_surface_extrude_capacity_exceeded");
      }
      plan.surfaceCells[plan.surfaceCellCount++] = candidate;
    }
  }

  const CreativeGridCoord3 layerDirection =
      request.kind == CreativeSurfaceExtrudeKind::Extrude
          ? request.outward
          : CreativeGridCoord3{-request.outward.x, -request.outward.y,
                               -request.outward.z};
  for (std::uint8_t layer = 0U; layer < depth; ++layer) {
    const std::uint8_t distance =
        request.kind == CreativeSurfaceExtrudeKind::Extrude
            ? static_cast<std::uint8_t>(layer + 1U)
            : layer;
    for (std::size_t surfaceIndex = 0U;
         surfaceIndex < plan.surfaceCellCount; ++surfaceIndex) {
      CreativeGridCoord3 cell{};
      if (!stepCell(plan.surfaceCells[surfaceIndex], layerDirection,
                    distance, cell)) {
        return reject(plan, CreativeSurfaceExtrudeStatus::CoordinateOverflow,
                      "creative_surface_extrude_coordinate_overflow");
      }
      const CreativeObjectKind material = request.field->materialAt(cell);
      if (request.kind == CreativeSurfaceExtrudeKind::Extrude &&
          material != CreativeObjectKind::Unknown) {
        return reject(plan,
                      CreativeSurfaceExtrudeStatus::DestinationOccupied,
                      "creative_surface_extrude_destination_occupied");
      }
      if (request.kind == CreativeSurfaceExtrudeKind::Inset &&
          material != plan.sourceMaterial) {
        return reject(plan, CreativeSurfaceExtrudeStatus::SourceMissing,
                      "creative_surface_extrude_source_missing");
      }
      includeMutationCell(plan, cell);
      plan.mutationCells[plan.mutationCellCount++] = cell;
    }
  }

  plan.accepted = true;
  plan.status = CreativeSurfaceExtrudeStatus::Planned;
  plan.reasonCode = "creative_surface_extrude_planned";
  return plan;
}

}  // namespace iggy3d::creative
