#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "app/iggy3d/creative/document/Object.hpp"
#include "core/math/Vec3.hpp"

namespace iggy3d::creative {

struct CreativeRuntimeMovingPlatformDefinition {
  std::array<Vec3, kCreativeMovingPlatformPathPointCapacity> pathPoints{};
  std::array<float, kCreativeMovingPlatformPathPointCapacity>
      cumulativeOpenMeters{};
  std::size_t pathPointCount = 0U;
  float openLengthMeters = 0.0F;
  float loopLengthMeters = 0.0F;
  float speedMetersPerSecond = 0.0F;
  CreativeMovingPlatformTraversalMode traversalMode =
      CreativeMovingPlatformTraversalMode::PingPong;
  bool startsActive = true;
  Vec3 originPositionMeters;
};

struct CreativeRuntimeMovingPlatformState {
  double phaseMeters = 0.0;
  std::int8_t travelSign = 1;
  Vec3 positionMeters;
  bool blocked = false;
  std::uint64_t movementTickCount = 0U;
};

enum class CreativeRuntimeMovingPlatformBuildStatus : std::uint8_t {
  InvalidPath,
  InvalidSettings,
  Built,
};

struct CreativeRuntimeMovingPlatformBuildResult {
  bool ok = false;
  CreativeRuntimeMovingPlatformBuildStatus status =
      CreativeRuntimeMovingPlatformBuildStatus::InvalidPath;
  std::string_view reasonCode = "creative_runtime_moving_platform_path_invalid";
  CreativeRuntimeMovingPlatformDefinition definition;
};

[[nodiscard]] CreativeRuntimeMovingPlatformBuildResult
buildCreativeRuntimeMovingPlatformDefinition(
    std::span<const CreativePathPoint> pathPoints,
    CreativeMovingPlatformSettings settings,
    Vec3 originPositionMeters) noexcept;

enum class CreativeRuntimeMovingPlatformStepStatus : std::uint8_t {
  InvalidRequest,
  Inactive,
  Stationary,
  Advanced,
};

struct CreativeRuntimeMovingPlatformStepRequest {
  const CreativeRuntimeMovingPlatformDefinition* definition = nullptr;
  const CreativeRuntimeMovingPlatformState* state = nullptr;
  std::uint32_t fixedTickRateHz = 0U;
  bool active = false;
};

struct CreativeRuntimeMovingPlatformStepResult {
  bool ok = false;
  bool moved = false;
  CreativeRuntimeMovingPlatformStepStatus status =
      CreativeRuntimeMovingPlatformStepStatus::InvalidRequest;
  std::string_view reasonCode = "creative_runtime_moving_platform_step_invalid";
  Vec3 displacementMeters;
  CreativeRuntimeMovingPlatformState nextState;
};

enum class CreativeRuntimeMovingPlatformSampleStatus : std::uint8_t {
  InvalidRequest,
  Sampled,
};

struct CreativeRuntimeMovingPlatformSampleResult {
  bool ok = false;
  CreativeRuntimeMovingPlatformSampleStatus status =
      CreativeRuntimeMovingPlatformSampleStatus::InvalidRequest;
  std::string_view reasonCode =
      "creative_runtime_moving_platform_sample_invalid";
  double normalizedProgress = 0.0;
  CreativeRuntimeMovingPlatformState state;
};

// Pure route sampler for editor previews and deterministic tooling. Progress
// is position along the authored route: PingPong 0/1 are the first/last
// waypoint, while Loop 1 closes back to the first. It never touches runtime
// world state.
[[nodiscard]] CreativeRuntimeMovingPlatformSampleResult
sampleCreativeRuntimeMovingPlatformProgress(
    const CreativeRuntimeMovingPlatformDefinition& definition,
    double normalizedProgress) noexcept;

// Pure deterministic route kernel. The runtime world owner may reject the
// planned displacement without committing nextState.
[[nodiscard]] CreativeRuntimeMovingPlatformStepResult
planCreativeRuntimeMovingPlatformStep(
    const CreativeRuntimeMovingPlatformStepRequest& request) noexcept;

struct CreativeRuntimeSandbox;

enum class CreativeRuntimeMovingPlatformUpdateStatus : std::uint8_t {
  NotRequested,
  NoPlatforms,
  NoMovement,
  Advanced,
  Blocked,
  Rejected,
};

struct CreativeRuntimeMovingPlatformUpdateReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeRuntimeMovingPlatformUpdateStatus status =
      CreativeRuntimeMovingPlatformUpdateStatus::NotRequested;
  std::string_view reasonCode =
      "creative_runtime_moving_platform_update_not_requested";
  std::size_t evaluatedPlatformCount = 0U;
  std::size_t movedPlatformCount = 0U;
  std::size_t blockedPlatformCount = 0U;
  std::size_t carriedActorCount = 0U;
  CreativeObjectId lastObjectId = kInvalidObjectId;
  std::uint64_t geometryRevision = 0U;
};

// Applies one fixed-tick kinematic pass. Room geometry, collision surfaces,
// reasoning, runtime entities, riders, and the session hash publish together.
[[nodiscard]] CreativeRuntimeMovingPlatformUpdateReceipt
updateCreativeRuntimeMovingPlatforms(CreativeRuntimeSandbox& sandbox,
                                     std::uint32_t fixedTickRateHz);

}  // namespace iggy3d::creative
