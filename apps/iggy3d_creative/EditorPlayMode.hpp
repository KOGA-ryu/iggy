#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "app/iggy3d/creative/play/RuntimeSandbox.hpp"
#include "projection/scene/SceneProjection.hpp"

namespace iggy3d_creative_app {

struct CreativeEditorPlayTuning {
  float walkSpeedMetersPerSecond = 4.5F;
  float sprintMultiplier = 1.75F;
  float minimumPitchDegrees = -80.0F;
  float maximumPitchDegrees = 80.0F;
  std::uint32_t maximumCatchUpTicks = 4U;
};

struct CreativeEditorPlayMode {
  std::optional<iggy3d::creative::CreativeRuntimeSandbox> sandbox;
  CreativeEditorPlayTuning tuning;
  float cameraYawDegrees = 0.0F;
  float cameraPitchDegrees = 0.0F;
  std::uint64_t lastFrameTimeNanoseconds = 0U;
  std::uint64_t accumulatedTimeNanoseconds = 0U;
  bool clockPrimed = false;
};

enum class CreativeEditorPlayStartStatus : std::uint8_t {
  NotRequested,
  AlreadyActive,
  InvalidTuning,
  PreparationRejected,
  ActivationRejected,
  Started,
};

struct CreativeEditorPlayStartRequest {
  const iggy3d::creative::CreativeDocument* document = nullptr;
  const iggy3d::StaticMeshAssetCatalog* staticMeshAssetCatalog = nullptr;
  iggy3d::creative::CreativeRuntimeSandboxConfig sandboxConfig;
  CreativeEditorPlayTuning tuning;
  std::string roomId = "creative_editor_play";
};

struct CreativeEditorPlayStartReceipt {
  bool requested = false;
  bool accepted = false;
  CreativeEditorPlayStartStatus status =
      CreativeEditorPlayStartStatus::NotRequested;
  std::string reasonCode = "creative_editor_play_not_requested";
  iggy3d::creative::CreativePlayPreparationStatus preparationStatus =
      iggy3d::creative::CreativePlayPreparationStatus::NotRequested;
  iggy3d::creative::CreativeRuntimeSandboxActivationReceipt activation;
};

[[nodiscard]] std::string_view toString(
    CreativeEditorPlayStartStatus status) noexcept;
[[nodiscard]] bool creativeEditorPlayModeActive(
    const CreativeEditorPlayMode& mode) noexcept;

[[nodiscard]] CreativeEditorPlayStartReceipt startCreativeEditorPlayMode(
    CreativeEditorPlayMode& mode,
    CreativeEditorPlayStartRequest request);

[[nodiscard]] iggy3d::creative::CreativeRuntimeSandboxStopReceipt
stopCreativeEditorPlayMode(CreativeEditorPlayMode& mode) noexcept;

struct CreativeEditorPlayInput {
  float moveRight = 0.0F;
  float moveForward = 0.0F;
  float yawDeltaDegrees = 0.0F;
  float pitchDeltaDegrees = 0.0F;
  bool sprinting = false;
  bool windowFocused = true;
};

enum class CreativeEditorPlayTickStatus : std::uint8_t {
  NotRequested,
  Inactive,
  SourceDocumentChanged,
  InvalidInput,
  Suspended,
  ClockPrimed,
  NoTickDue,
  CommandRejected,
  RuntimeTickFailed,
  Advanced,
};

struct CreativeEditorPlayTickRequest {
  const iggy3d::creative::CreativeDocument* sourceDocument = nullptr;
  CreativeEditorPlayInput input;
  std::uint64_t monotonicTimeNanoseconds = 0U;
};

struct CreativeEditorPlayTickReceipt {
  bool requested = false;
  bool active = false;
  CreativeEditorPlayTickStatus status =
      CreativeEditorPlayTickStatus::NotRequested;
  std::string reasonCode = "creative_editor_play_tick_not_requested";
  std::uint32_t ticksAdvanced = 0U;
  std::uint32_t commandsSubmitted = 0U;
  std::uint32_t movementCommandsSubmitted = 0U;
  std::uint64_t sourceTick = 0U;
  iggy3d::CommandRejectionReason commandRejection =
      iggy3d::CommandRejectionReason::None;
};

[[nodiscard]] std::string_view toString(
    CreativeEditorPlayTickStatus status) noexcept;
[[nodiscard]] CreativeEditorPlayTickReceipt tickCreativeEditorPlayMode(
    CreativeEditorPlayMode& mode,
    const CreativeEditorPlayTickRequest& request);

struct CreativeEditorPlayScene {
  bool available = false;
  iggy3d::Vec3 cameraAnchorMeters;
  iggy3d::SceneProjectionResult scene;
};

[[nodiscard]] CreativeEditorPlayScene buildCreativeEditorPlayScene(
    const CreativeEditorPlayMode& mode);

}  // namespace iggy3d_creative_app
