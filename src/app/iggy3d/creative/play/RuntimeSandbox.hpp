#pragma once

#include "app/iggy3d/creative/play/PlayPreparation.hpp"
#include "config/RuntimeConfig.hpp"
#include "runtime/ai/ReasoningGraph.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/session/Session.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

enum class CreativeRuntimeScenarioSeedStatus : std::uint8_t {
  NotRequested,
  InvalidConfig,
  InvalidPayload,
  Built,
};

struct CreativeRuntimeSandboxConfig {
  RuntimeConfig runtimeConfig = makeDefaultRuntimeConfig();
  std::string packageId = "iggy3d.creative_play";
  std::string npcBehaviorProfileId = "passive";
  std::string monsterBehaviorProfileId = "default";
  std::int32_t playerHitPoints = 10;
  std::int32_t npcHitPoints = 3;
  std::int32_t monsterHitPoints = 3;
};

struct CreativeRuntimeScenarioSummary {
  std::size_t sourceAnchorCount = 0;
  std::size_t sourceInteractableCount = 0;
  std::size_t playerEntityCount = 0;
  std::size_t npcEntityCount = 0;
  std::size_t monsterEntityCount = 0;
  std::size_t doorEntityCount = 0;
  std::size_t controlEntityCount = 0;
  std::size_t pickupEntityCount = 0;
  std::size_t ignoredAnchorCount = 0;
};

struct CreativeRuntimeScenarioSeedResult {
  bool accepted = false;
  CreativeRuntimeScenarioSeedStatus status =
      CreativeRuntimeScenarioSeedStatus::NotRequested;
  std::string_view reasonCode = "creative_runtime_seed_not_requested";
  CreativeRuntimeScenarioSummary summary;
  FixtureScenarioSeed seed;
};

[[nodiscard]] std::string_view toString(
    CreativeRuntimeScenarioSeedStatus status) noexcept;

// Pure activation kernel. The player is always emitted first because Session's
// local-player contract binds slot zero to EntityId{1}. Runtime entity identity
// comes from the immutable RoomAsset anchor stable names.
[[nodiscard]] CreativeRuntimeScenarioSeedResult buildCreativeRuntimeScenarioSeed(
    const CreativePlayActivationPayload& payload,
    const CreativeRuntimeSandboxConfig& config = {});

enum class CreativeRuntimeSandboxActivationStatus : std::uint8_t {
  NotRequested,
  MissingDocument,
  StalePayload,
  InvalidConfig,
  InvalidPayload,
  InvalidInteractables,
  InvalidCollisionSurfaces,
  SessionCreationFailed,
  Activated,
};

struct CreativeRuntimeSandboxActivationReceipt {
  bool requested = false;
  bool accepted = false;
  CreativeRuntimeSandboxActivationStatus status =
      CreativeRuntimeSandboxActivationStatus::NotRequested;
  std::string reasonCode = "creative_runtime_sandbox_not_requested";
  CreativeDocumentId documentId = kInvalidDocumentId;
  std::uint64_t documentRevision = 0;
  std::string roomId;
  CreativeRuntimeScenarioSummary scenario;
  std::size_t collisionSurfaceCount = 0;
  ReasoningGraphSummary reasoningGraph;
  StateHashValue initialStateHash = 0;
};

struct CreativeRuntimeSandbox {
  CreativeDocumentId sourceDocumentId = kInvalidDocumentId;
  std::uint64_t sourceDocumentRevision = 0;
  std::string roomId;
  CreativeRuntimeScenarioSummary scenario;
  ReasoningGraphSummary reasoningGraph;
  RoomAsset room;
  std::vector<std::string> roomStaticMeshOrder;
  std::vector<std::string> roomSpatialSurfaceOrder;
  SpatialSurfaceSet collisionSurfaces;
  std::vector<CreativeRuntimeInteractableState> interactables;
  std::uint64_t geometryRevision = 0U;
  Session session;
};

struct CreativeRuntimeSandboxActivationRequest {
  // Passed by value so activateCreativeRuntimeSandbox can consume the baked
  // RoomAsset instead of copying the runtime snapshot into the sandbox.
  CreativePlayActivationPayload payload;
  const CreativeDocument* sourceDocument = nullptr;
  CreativeRuntimeSandboxConfig config;
};

struct CreativeRuntimeSandboxActivationResult {
  CreativeRuntimeSandboxActivationReceipt receipt;
  std::optional<CreativeRuntimeSandbox> sandbox;
};

[[nodiscard]] std::string_view toString(
    CreativeRuntimeSandboxActivationStatus status) noexcept;

[[nodiscard]] CreativeRuntimeSandboxActivationResult
activateCreativeRuntimeSandbox(CreativeRuntimeSandboxActivationRequest request);

[[nodiscard]] bool creativeRuntimeSandboxIsCurrent(
    const CreativeRuntimeSandbox& sandbox,
    const CreativeDocument& document) noexcept;

enum class CreativeRuntimeSandboxStopStatus : std::uint8_t {
  NotRequested,
  NoActiveSandbox,
  Stopped,
};

struct CreativeRuntimeSandboxStopReceipt {
  bool requested = false;
  bool stopped = false;
  CreativeRuntimeSandboxStopStatus status =
      CreativeRuntimeSandboxStopStatus::NotRequested;
  std::string_view reasonCode = "creative_runtime_sandbox_stop_not_requested";
  CreativeDocumentId documentId = kInvalidDocumentId;
  std::uint64_t documentRevision = 0;
  StateHashValue finalStateHash = 0;
};

[[nodiscard]] std::string_view toString(
    CreativeRuntimeSandboxStopStatus status) noexcept;

[[nodiscard]] CreativeRuntimeSandboxStopReceipt stopCreativeRuntimeSandbox(
    std::optional<CreativeRuntimeSandbox>& sandbox) noexcept;

}  // namespace iggy3d::creative
