#include "app/iggy3d/creative/tools/ConnectedFill.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>

#include "app/iggy3d/creative/document/VoxelField.hpp"

namespace iggy3d::creative {
namespace {

inline constexpr std::size_t kVisitedCapacity =
    kCreativeConnectedFillCapacity * 2U;
static_assert((kVisitedCapacity & (kVisitedCapacity - 1U)) == 0U);

struct VisitedCells {
  std::array<CreativeGridCoord3, kVisitedCapacity> cells{};
  std::array<std::uint8_t, kVisitedCapacity> occupied{};
};

[[nodiscard]] std::uint64_t coordinateHash(
    CreativeGridCoord3 cell) noexcept {
  std::uint64_t hash = 1469598103934665603ULL;
  for (std::int32_t component : {cell.x, cell.y, cell.z}) {
    hash ^= static_cast<std::uint32_t>(component);
    hash *= 1099511628211ULL;
  }
  return hash;
}

// Returns true only when the cell was not already present and was inserted.
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
    if (visited.cells[index] == cell) {
      return false;
    }
  }
  return false;
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

void includeCell(CreativeConnectedFillPlan& plan,
                 CreativeGridCoord3 cell) noexcept {
  plan.minCell.x = std::min(plan.minCell.x, cell.x);
  plan.minCell.y = std::min(plan.minCell.y, cell.y);
  plan.minCell.z = std::min(plan.minCell.z, cell.z);
  plan.maxCell.x = std::max(plan.maxCell.x, cell.x);
  plan.maxCell.y = std::max(plan.maxCell.y, cell.y);
  plan.maxCell.z = std::max(plan.maxCell.z, cell.z);
}

[[nodiscard]] CreativeConnectedFillPlan reject(
    CreativeConnectedFillPlan plan,
    CreativeConnectedFillStatus status,
    std::string_view reasonCode) noexcept {
  plan.accepted = false;
  plan.status = status;
  plan.cellCount = 0U;
  plan.reasonCode = reasonCode;
  return plan;
}

}  // namespace

std::uint16_t connectedFillCellLimit(
    CreativeConnectedFillLimit limit) noexcept {
  constexpr std::array<std::uint16_t, 4U> limits{64U, 128U, 256U, 512U};
  const std::size_t index = static_cast<std::size_t>(limit);
  return index < limits.size() ? limits[index] : 0U;
}

std::string_view toString(CreativeConnectedFillLimit limit) noexcept {
  switch (limit) {
    case CreativeConnectedFillLimit::Cells64: return "64 CELLS";
    case CreativeConnectedFillLimit::Cells128: return "128 CELLS";
    case CreativeConnectedFillLimit::Cells256: return "256 CELLS";
    case CreativeConnectedFillLimit::Cells512: return "512 CELLS";
    case CreativeConnectedFillLimit::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeConnectedFillStatus status) noexcept {
  switch (status) {
    case CreativeConnectedFillStatus::NotRequested: return "NotRequested";
    case CreativeConnectedFillStatus::InvalidField: return "InvalidField";
    case CreativeConnectedFillStatus::InvalidLimit: return "InvalidLimit";
    case CreativeConnectedFillStatus::EmptySeed: return "EmptySeed";
    case CreativeConnectedFillStatus::CapacityExceeded:
      return "CapacityExceeded";
    case CreativeConnectedFillStatus::Planned: return "Planned";
  }
  return "Unknown";
}

CreativeConnectedFillPlan planCreativeConnectedFill(
    const CreativeConnectedFillRequest& request) noexcept {
  CreativeConnectedFillPlan plan;
  plan.requested = true;
  plan.limit = request.limit;
  plan.seedCell = request.seedCell;
  plan.minCell = request.seedCell;
  plan.maxCell = request.seedCell;

  if (request.field == nullptr || !request.field->isValid()) {
    return reject(plan, CreativeConnectedFillStatus::InvalidField,
                  "creative_connected_fill_field_invalid");
  }
  const std::uint16_t limit = connectedFillCellLimit(request.limit);
  if (limit == 0U || limit > plan.cells.size()) {
    return reject(plan, CreativeConnectedFillStatus::InvalidLimit,
                  "creative_connected_fill_limit_invalid");
  }
  plan.sourceMaterial = request.field->materialAt(request.seedCell);
  if (plan.sourceMaterial == CreativeObjectKind::Unknown) {
    return reject(plan, CreativeConnectedFillStatus::EmptySeed,
                  "creative_connected_fill_seed_empty");
  }

  constexpr std::array<CreativeGridCoord3, 6U> neighbors{
      CreativeGridCoord3{-1, 0, 0}, CreativeGridCoord3{1, 0, 0},
      CreativeGridCoord3{0, -1, 0}, CreativeGridCoord3{0, 1, 0},
      CreativeGridCoord3{0, 0, -1}, CreativeGridCoord3{0, 0, 1},
  };
  VisitedCells visited;
  static_cast<void>(markVisited(visited, request.seedCell));
  plan.cells[0] = request.seedCell;
  plan.cellCount = 1U;

  std::size_t readIndex = 0U;
  while (readIndex < plan.cellCount) {
    const CreativeGridCoord3 current = plan.cells[readIndex++];
    for (CreativeGridCoord3 offset : neighbors) {
      CreativeGridCoord3 candidate{};
      if (!offsetCell(current, offset, candidate) ||
          request.field->materialAt(candidate) != plan.sourceMaterial ||
          !markVisited(visited, candidate)) {
        continue;
      }
      if (plan.cellCount >= limit) {
        return reject(plan, CreativeConnectedFillStatus::CapacityExceeded,
                      "creative_connected_fill_capacity_exceeded");
      }
      plan.cells[plan.cellCount++] = candidate;
      includeCell(plan, candidate);
    }
  }

  plan.accepted = true;
  plan.status = CreativeConnectedFillStatus::Planned;
  plan.reasonCode = "creative_connected_fill_planned";
  return plan;
}

}  // namespace iggy3d::creative
