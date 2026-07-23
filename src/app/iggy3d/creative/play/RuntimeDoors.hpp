#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

#include "app/iggy3d/creative/document/Object.hpp"
#include "core/math/Aabb3.hpp"
#include "core/math/Vec3.hpp"

namespace iggy3d::creative {

inline constexpr std::size_t kCreativeRuntimeDoorLeafCapacity = 2U;
inline constexpr std::size_t kCreativeRuntimeDoorPartCapacityPerLeaf = 4U;
inline constexpr std::uint8_t kCreativeRuntimeDoorNoLeaf = 0xffU;
inline constexpr float kCreativeRuntimeDoorTargetPaddingMeters = 0.08F;

struct CreativeRuntimeDoorLeafDefinition {
  std::array<std::string, kCreativeRuntimeDoorPartCapacityPerLeaf>
      roomMeshIds{};
  std::size_t roomMeshIdCount = 0U;
  Vec3 hingePivotMeters;
  float openAngleRadians = 0.0F;
  Aabb3 closedAssemblyBounds;
  Aabb3 sweepBounds;
};

struct CreativeRuntimeDoorDefinition {
  CreativeDoorSettings settings;
  std::array<CreativeRuntimeDoorLeafDefinition,
             kCreativeRuntimeDoorLeafCapacity>
      leaves{};
  std::size_t leafCount = 0U;
  Aabb3 fullSweepBounds;
};

struct CreativeRuntimeDoorState {
  double openFraction = 0.0;
  bool blocked = false;
  std::uint64_t transitionTickCount = 0U;
};

enum class CreativeRuntimeDoorBuildStatus : std::uint8_t {
  InvalidRoot,
  InvalidSettings,
  InvalidGeometry,
  InvalidHierarchy,
  CapacityExceeded,
  Built,
};

struct CreativeRuntimeDoorBuildResult {
  bool ok = false;
  CreativeRuntimeDoorBuildStatus status =
      CreativeRuntimeDoorBuildStatus::InvalidRoot;
  std::string_view reasonCode = "creative_runtime_door_root_invalid";
  CreativeRuntimeDoorDefinition definition;
};

// Builds the runtime hinge/group contract from the semantic Door root and its
// direct generated children. Frame parts are deliberately not children of the
// Door root, so they remain static while the leaf assemblies move.
[[nodiscard]] CreativeRuntimeDoorBuildResult
buildCreativeRuntimeDoorDefinition(
    const CreativeObject& root,
    std::span<const CreativeObject> documentObjects);

[[nodiscard]] std::uint8_t creativeRuntimeDoorLeafIndexForMesh(
    const CreativeRuntimeDoorDefinition& definition,
    std::string_view roomMeshId) noexcept;

enum class CreativeRuntimeDoorStepStatus : std::uint8_t {
  InvalidRequest,
  Stationary,
  Advanced,
  Arrived,
};

struct CreativeRuntimeDoorStepRequest {
  const CreativeRuntimeDoorDefinition* definition = nullptr;
  const CreativeRuntimeDoorState* state = nullptr;
  std::uint32_t fixedTickRateHz = 0U;
  bool targetOpen = false;
};

struct CreativeRuntimeDoorStepResult {
  bool ok = false;
  bool moved = false;
  bool arrived = false;
  CreativeRuntimeDoorStepStatus status =
      CreativeRuntimeDoorStepStatus::InvalidRequest;
  std::string_view reasonCode = "creative_runtime_door_step_invalid";
  CreativeRuntimeDoorState nextState;
};

[[nodiscard]] CreativeRuntimeDoorStepResult planCreativeRuntimeDoorStep(
    const CreativeRuntimeDoorStepRequest& request) noexcept;

struct CreativeRuntimeSandbox;
struct CreativeRuntimeInteractableState;

enum class CreativeRuntimeDoorTargetBlock : std::uint8_t {
  None,
  Locked,
  Occupied,
  Invalid,
};

[[nodiscard]] CreativeRuntimeDoorTargetBlock
creativeRuntimeDoorTargetBlock(
    const CreativeRuntimeSandbox& sandbox,
    const CreativeRuntimeInteractableState& door,
    bool targetOpen);

void queueCreativeRuntimeDoorTransitionSound(
    CreativeRuntimeSandbox& sandbox,
    const CreativeRuntimeInteractableState& door);

enum class CreativeRuntimeDoorUpdateStatus : std::uint8_t {
  NotRequested,
  NoDoors,
  Stationary,
  Advanced,
  Blocked,
  Rejected,
};

struct CreativeRuntimeDoorUpdateReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeRuntimeDoorUpdateStatus status =
      CreativeRuntimeDoorUpdateStatus::NotRequested;
  std::string_view reasonCode = "creative_runtime_door_update_not_requested";
  std::size_t evaluatedDoorCount = 0U;
  std::size_t movedDoorCount = 0U;
  std::size_t completedDoorCount = 0U;
  std::size_t blockedDoorCount = 0U;
  CreativeObjectId lastObjectId = kInvalidObjectId;
  std::uint64_t geometryRevision = 0U;
};

// Applies authored initial-open poses without consuming a runtime geometry
// revision. The activation owner calls this once after Session creation.
[[nodiscard]] CreativeRuntimeDoorUpdateReceipt
initializeCreativeRuntimeDoors(CreativeRuntimeSandbox& sandbox);

// Applies one deterministic fixed-tick pass. All moving leaf meshes, collision
// surfaces, LOS/reasoning geometry, and the geometry revision publish together.
[[nodiscard]] CreativeRuntimeDoorUpdateReceipt updateCreativeRuntimeDoors(
    CreativeRuntimeSandbox& sandbox,
    std::uint32_t fixedTickRateHz);

[[nodiscard]] std::string_view toString(
    CreativeRuntimeDoorUpdateStatus status) noexcept;

}  // namespace iggy3d::creative
