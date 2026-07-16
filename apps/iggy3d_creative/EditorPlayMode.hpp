#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "EditorPlayLogicOverlay.hpp"
#include "app/iggy3d/creative/play/RuntimeSandbox.hpp"
#include "app/iggy3d/creative/input/InputRouter.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "render/FrameInput.hpp"
#include "runtime/physics/PhysicsSpatialSurfaceColliderBake.hpp"

namespace iggy3d_creative_app {

struct CreativeEditorPlayTuning {
  float walkSpeedMetersPerSecond = 4.5F;
  float sprintMultiplier = 1.75F;
  float minimumPitchDegrees = -80.0F;
  float maximumPitchDegrees = 80.0F;
  float targetProbeDistanceMeters = 12.0F;
  float targetRadiusMeters = 0.0F;
  float targetOcclusionMarginMeters = 0.01F;
  std::int32_t attackDamage = 1;
  std::uint32_t maximumCatchUpTicks = 4U;
};

enum class CreativeEditorPlayAction : std::uint8_t {
  None,
  Attack,
  Interact,
};

struct CreativeEditorPlayActionSample {
  bool attackDown = false;
  bool interactDown = false;
  bool enabled = true;
};

struct CreativeEditorPlayActionRouterState {
  bool attackDown = false;
  bool interactDown = false;
  bool rearmRequired = false;
};

[[nodiscard]] CreativeEditorPlayActionSample sampleCreativeEditorPlayActions(
    const iggy3d::creative::CreativeInputFrame& inputFrame,
    const iggy3d::creative::CreativeInputRouteResult& routedInput,
    std::span<const iggy3d::creative::CreativeInputBinding> bindings) noexcept;
[[nodiscard]] CreativeEditorPlayAction routeCreativeEditorPlayAction(
    CreativeEditorPlayActionRouterState& state,
    CreativeEditorPlayActionSample sample) noexcept;

enum class CreativeEditorPlayTargetStatus : std::uint8_t {
  None,
  Valid,
  Blocked,
  Friendly,
  OutOfRange,
  Unsupported,
  Defeated,
  Invalid,
};

struct CreativeEditorPlayTarget {
  CreativeEditorPlayTargetStatus status =
      CreativeEditorPlayTargetStatus::None;
  iggy3d::EntityId entity;
  std::string stableName = "none";
  iggy3d::Vec3 hitPointMeters;
  float hitDistanceMeters = 0.0F;
  float reachDistanceMeters = 0.0F;
  std::string displayName;
  std::string actionPrompt;
  bool supportsAttack = false;
  bool supportsInteract = false;
  bool friendly = false;
  bool defeated = false;
};

struct CreativeEditorPlayTargetRequest {
  const iggy3d::WorldState* world = nullptr;
  const iggy3d::CombatState* combat = nullptr;
  std::span<const iggy3d::PhysicsAabbCollider> colliders;
  iggy3d::EntityId actor;
  iggy3d::Vec3 eyeMeters;
  iggy3d::Vec3 forward;
  float probeDistanceMeters = 12.0F;
  float targetRadiusMeters = 0.0F;
  float interactionRangeMeters = 1.5F;
  float occlusionMarginMeters = 0.01F;
  std::int32_t attackDamage = 1;
};

[[nodiscard]] std::string_view toString(
    CreativeEditorPlayTargetStatus status) noexcept;
[[nodiscard]] CreativeEditorPlayTarget resolveCreativeEditorPlayTarget(
    const CreativeEditorPlayTargetRequest& request);
[[nodiscard]] bool creativeEditorPlayTargetAcceptsAction(
    const CreativeEditorPlayTarget& target,
    CreativeEditorPlayAction action) noexcept;

struct CreativeEditorPlayMode {
  std::optional<iggy3d::creative::CreativeRuntimeSandbox> sandbox;
  iggy3d::PhysicsSpatialSurfaceColliderBakeResult targetingBake;
  CreativeEditorPlayTuning tuning;
  CreativeEditorPlayActionRouterState actionRouter;
  CreativeEditorPlayTarget target;
  iggy3d::creative::CreativeRuntimeInteractionEffectReceipt
      lastInteractionEffect;
  iggy3d::creative::CreativeRuntimeAutomaticLogicReceipt lastAutomaticLogic;
  std::uint64_t targetingGeometryRevision = 0U;
  std::size_t processedRuntimeEventCount = 0U;
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
  TargetingBakeRejected,
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
  CreativeEditorPlayActionSample actions;
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
  std::uint32_t attackCommandsSubmitted = 0U;
  std::uint32_t interactionCommandsSubmitted = 0U;
  std::uint32_t interactionEffectsApplied = 0U;
  std::uint32_t automaticSourceTransitions = 0U;
  std::uint32_t automaticEffectsApplied = 0U;
  CreativeEditorPlayAction action = CreativeEditorPlayAction::None;
  bool actionAttempted = false;
  bool actionSubmitted = false;
  std::uint64_t sourceTick = 0U;
  iggy3d::CommandRejectionReason commandRejection =
      iggy3d::CommandRejectionReason::None;
  iggy3d::creative::CreativeRuntimeInteractionEffectStatus interactionEffect =
      iggy3d::creative::CreativeRuntimeInteractionEffectStatus::NotRequested;
  iggy3d::creative::CreativeRuntimeAutomaticLogicStatus automaticLogic =
      iggy3d::creative::CreativeRuntimeAutomaticLogicStatus::NotRequested;
  std::uint64_t runtimeGeometryRevision = 0U;
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
  CreativeEditorPlayLogicOverlay logicOverlay;
};

[[nodiscard]] CreativeEditorPlayScene buildCreativeEditorPlayScene(
    const CreativeEditorPlayMode& mode,
    iggy3d::creative::CreativeObjectId highlightedLogicSourceObjectId =
        iggy3d::creative::kInvalidObjectId);

struct CreativeEditorPlayView {
  bool available = false;
  iggy3d::Vec3 eyeMeters;
  iggy3d::Vec3 forward;
};

[[nodiscard]] CreativeEditorPlayView buildCreativeEditorPlayView(
    const CreativeEditorPlayMode& mode) noexcept;

inline constexpr std::size_t kCreativeEditorPlayHudRectCapacity = 10U;
inline constexpr std::size_t kCreativeEditorPlayHudGlyphQuadCapacity = 1664U;

struct CreativeEditorPlayHudFrame {
  std::array<iggy3d::RenderUiRect, kCreativeEditorPlayHudRectCapacity> rects{};
  std::array<iggy3d::DebugHudGlyphQuad,
             kCreativeEditorPlayHudGlyphQuadCapacity>
      glyphQuads{};
  std::size_t rectCount = 0U;
  std::size_t glyphQuadCount = 0U;
  std::size_t textGlyphCount = 0U;
  bool capacityExceeded = false;
};

[[nodiscard]] CreativeEditorPlayHudFrame buildCreativeEditorPlayHud(
    const CreativeEditorPlayMode& mode,
    const iggy3d::FrameInput& frame);
void attachCreativeEditorPlayHud(const CreativeEditorPlayHudFrame& hud,
                                 iggy3d::FrameInput& frame) noexcept;

}  // namespace iggy3d_creative_app
