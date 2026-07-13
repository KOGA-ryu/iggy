#include "app/iggy3d/creative/tools/AssetScatter.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numbers>

#include "app/iggy3d/creative/Geometry.hpp"

namespace iggy3d::creative {
namespace {

inline constexpr std::int32_t kMaximumGridRadius = 16;
inline constexpr std::size_t kMaximumOrderedCellCount =
    static_cast<std::size_t>((kMaximumGridRadius * 2 + 1) *
                             (kMaximumGridRadius * 2 + 1));

struct OrderedCell {
  std::int32_t x = 0;
  std::int32_t z = 0;
  std::uint64_t order = 0;
};

[[nodiscard]] std::uint64_t mix64(std::uint64_t value) noexcept {
  value += 0x9e3779b97f4a7c15ULL;
  value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
  value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
  return value ^ (value >> 31U);
}

[[nodiscard]] std::uint64_t cellHash(std::uint64_t seed,
                                     std::int32_t x,
                                     std::int32_t z,
                                     std::uint64_t stream = 0U) noexcept {
  const std::uint64_t packed =
      (static_cast<std::uint64_t>(static_cast<std::uint32_t>(x)) << 32U) |
      static_cast<std::uint32_t>(z);
  return mix64(seed ^ mix64(packed) ^ mix64(stream));
}

[[nodiscard]] double unitDouble(std::uint64_t value) noexcept {
  return static_cast<double>(value >> 11U) * 0x1.0p-53;
}

[[nodiscard]] bool validYaw(CreativeAssetScatterYaw yaw) noexcept {
  return static_cast<std::size_t>(yaw) <
         static_cast<std::size_t>(CreativeAssetScatterYaw::Count);
}

[[nodiscard]] double candidateYaw(CreativeAssetScatterYaw yaw,
                                  std::uint64_t hash) noexcept {
  switch (yaw) {
    case CreativeAssetScatterYaw::Fixed:
      return 0.0;
    case CreativeAssetScatterYaw::QuarterTurns:
      return static_cast<double>(hash & 3U) * (std::numbers::pi * 0.5);
    case CreativeAssetScatterYaw::Full:
      return unitDouble(hash) * (std::numbers::pi * 2.0);
    case CreativeAssetScatterYaw::Count:
      return 0.0;
  }
  return 0.0;
}

[[nodiscard]] bool spacingAllows(
    const CreativeAssetScatterPlan& plan,
    CreativeVec3 position,
    double spacingSquared) noexcept {
  for (const CreativeAssetScatterCandidate& candidate : plan.items()) {
    const double dx = position.x - candidate.position.x;
    const double dz = position.z - candidate.position.z;
    if (dx * dx + dz * dz < spacingSquared) {
      return false;
    }
  }
  return true;
}

void appendCandidate(CreativeAssetScatterPlan& plan,
                     const CreativeAssetScatterRequest& request,
                     CreativeVec3 position,
                     std::uint64_t hash) noexcept {
  CreativeAssetScatterCandidate& candidate =
      plan.candidates[plan.candidateCount++];
  candidate.position = position;
  candidate.yawOffsetRadians =
      candidateYaw(request.yaw, mix64(hash ^ 0xa24baed4963ee407ULL));
  const double centered =
      unitDouble(mix64(hash ^ 0x9fb21c651e98df25ULL)) * 2.0 - 1.0;
  candidate.uniformScale = 1.0 + centered * request.scaleVariation;
}

[[nodiscard]] bool quantizedCoordinate(double value,
                                       double quantum,
                                       std::int64_t& output) noexcept {
  const double quantized = std::nearbyint(value / quantum);
  if (!std::isfinite(quantized) ||
      quantized < static_cast<double>(std::numeric_limits<std::int64_t>::min()) ||
      quantized > static_cast<double>(std::numeric_limits<std::int64_t>::max())) {
    return false;
  }
  output = static_cast<std::int64_t>(quantized);
  return true;
}

}  // namespace

CreativeAssetScatterPlan planCreativeAssetScatter(
    const CreativeAssetScatterRequest& request) noexcept {
  CreativeAssetScatterPlan plan;
  plan.requested = true;
  if (!isFiniteCreativeVec3(request.center) ||
      !std::isfinite(request.radiusMeters) || request.radiusMeters <= 0.0 ||
      !std::isfinite(request.spacingMeters) || request.spacingMeters <= 0.0 ||
      !std::isfinite(request.densityFraction) ||
      request.densityFraction <= 0.0 || request.densityFraction > 1.0 ||
      !std::isfinite(request.scaleVariation) ||
      request.scaleVariation < 0.0 || request.scaleVariation >= 1.0 ||
      !validYaw(request.yaw) || request.maxCandidateCount == 0U ||
      request.maxCandidateCount > plan.candidates.size()) {
    plan.status = CreativeAssetScatterStatus::InvalidRequest;
    return plan;
  }
  const double radiusInCells = request.radiusMeters / request.spacingMeters;
  if (!std::isfinite(radiusInCells) ||
      radiusInCells > static_cast<double>(kMaximumGridRadius)) {
    plan.status = CreativeAssetScatterStatus::CapacityExceeded;
    return plan;
  }

  const std::int32_t gridRadius =
      static_cast<std::int32_t>(std::ceil(radiusInCells));
  std::array<OrderedCell, kMaximumOrderedCellCount> ordered{};
  std::size_t orderedCount = 0U;
  for (std::int32_t z = -gridRadius; z <= gridRadius; ++z) {
    for (std::int32_t x = -gridRadius; x <= gridRadius; ++x) {
      if (x == 0 && z == 0) {
        continue;
      }
      ordered[orderedCount++] = {x, z, cellHash(request.seed, x, z)};
    }
  }
  std::sort(ordered.begin(), ordered.begin() + orderedCount,
            [](const OrderedCell& lhs, const OrderedCell& rhs) {
              if (lhs.order != rhs.order) {
                return lhs.order < rhs.order;
              }
              return lhs.x != rhs.x ? lhs.x < rhs.x : lhs.z < rhs.z;
            });

  appendCandidate(plan, request, request.center,
                  cellHash(request.seed, 0, 0));
  const double radiusSquared = request.radiusMeters * request.radiusMeters;
  const double spacingSquared =
      request.spacingMeters * request.spacingMeters * (1.0 - 1.0e-10);
  constexpr double kJitterFraction = 0.28;
  for (std::size_t index = 0U; index < orderedCount; ++index) {
    const OrderedCell& cell = ordered[index];
    ++plan.examinedCellCount;
    const std::uint64_t hash = cellHash(request.seed, cell.x, cell.z);
    if (unitDouble(mix64(hash ^ 0xd6e8feb86659fd93ULL)) >
        request.densityFraction) {
      ++plan.densityRejectedCount;
      continue;
    }
    const double jitterX =
        (unitDouble(mix64(hash ^ 0x94d049bb133111ebULL)) * 2.0 - 1.0) *
        request.spacingMeters * kJitterFraction;
    const double jitterZ =
        (unitDouble(mix64(hash ^ 0xbf58476d1ce4e5b9ULL)) * 2.0 - 1.0) *
        request.spacingMeters * kJitterFraction;
    const CreativeVec3 position{
        request.center.x + static_cast<double>(cell.x) *
                               request.spacingMeters +
            jitterX,
        request.center.y,
        request.center.z + static_cast<double>(cell.z) *
                               request.spacingMeters +
            jitterZ,
    };
    const double dx = position.x - request.center.x;
    const double dz = position.z - request.center.z;
    if (dx * dx + dz * dz > radiusSquared) {
      continue;
    }
    if (!spacingAllows(plan, position, spacingSquared)) {
      ++plan.spacingRejectedCount;
      continue;
    }
    if (plan.candidateCount >= request.maxCandidateCount) {
      plan.truncated = true;
      break;
    }
    appendCandidate(plan, request, position, hash);
  }

  plan.accepted = plan.candidateCount > 0U;
  plan.status = plan.accepted ? CreativeAssetScatterStatus::Ready
                              : CreativeAssetScatterStatus::Empty;
  return plan;
}

std::uint64_t creativeAssetScatterSpatialKey(CreativeVec3 position,
                                             double quantumMeters) noexcept {
  if (!isFiniteCreativeVec3(position) || !std::isfinite(quantumMeters) ||
      quantumMeters <= 0.0) {
    return 0U;
  }
  std::int64_t x = 0;
  std::int64_t y = 0;
  std::int64_t z = 0;
  if (!quantizedCoordinate(position.x, quantumMeters, x) ||
      !quantizedCoordinate(position.y, quantumMeters, y) ||
      !quantizedCoordinate(position.z, quantumMeters, z)) {
    return 0U;
  }
  return mix64(static_cast<std::uint64_t>(x)) ^
         mix64(static_cast<std::uint64_t>(y) ^ 0xa24baed4963ee407ULL) ^
         mix64(static_cast<std::uint64_t>(z) ^ 0x9fb21c651e98df25ULL);
}

}  // namespace iggy3d::creative
