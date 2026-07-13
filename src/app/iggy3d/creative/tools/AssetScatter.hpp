#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"

namespace iggy3d::creative {

inline constexpr std::size_t kCreativeAssetScatterCandidateCapacity = 128U;

enum class CreativeAssetScatterStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  CapacityExceeded,
  Empty,
  Ready,
};

struct CreativeAssetScatterRequest {
  CreativeVec3 center{};
  double radiusMeters = 4.0;
  double spacingMeters = 2.0;
  double densityFraction = 0.65;
  CreativeAssetScatterYaw yaw = CreativeAssetScatterYaw::Full;
  double scaleVariation = 0.10;
  std::uint64_t seed = 0;
  std::size_t maxCandidateCount = kCreativeAssetScatterCandidateCapacity;
};

struct CreativeAssetScatterCandidate {
  CreativeVec3 position{};
  double yawOffsetRadians = 0.0;
  double uniformScale = 1.0;
};

struct CreativeAssetScatterPlan {
  std::array<CreativeAssetScatterCandidate,
             kCreativeAssetScatterCandidateCapacity>
      candidates{};
  std::size_t candidateCount = 0;
  std::uint32_t examinedCellCount = 0;
  std::uint32_t densityRejectedCount = 0;
  std::uint32_t spacingRejectedCount = 0;
  CreativeAssetScatterStatus status =
      CreativeAssetScatterStatus::NotRequested;
  bool requested = false;
  bool accepted = false;
  bool truncated = false;

  [[nodiscard]] std::span<const CreativeAssetScatterCandidate> items()
      const noexcept {
    return {candidates.data(), candidateCount};
  }
};

static_assert(std::is_trivially_copyable_v<CreativeAssetScatterRequest>);
static_assert(std::is_standard_layout_v<CreativeAssetScatterRequest>);
static_assert(std::is_trivially_copyable_v<CreativeAssetScatterCandidate>);
static_assert(std::is_standard_layout_v<CreativeAssetScatterCandidate>);
static_assert(std::is_trivially_copyable_v<CreativeAssetScatterPlan>);
static_assert(std::is_standard_layout_v<CreativeAssetScatterPlan>);

// Bounded deterministic jittered-grid planner. Candidate spacing is enforced
// explicitly, so density/yaw/scale changes cannot create overlapping output.
[[nodiscard]] CreativeAssetScatterPlan planCreativeAssetScatter(
    const CreativeAssetScatterRequest& request) noexcept;

[[nodiscard]] std::uint64_t creativeAssetScatterSpatialKey(
    CreativeVec3 position,
    double quantumMeters) noexcept;

}  // namespace iggy3d::creative
