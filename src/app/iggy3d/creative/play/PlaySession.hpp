#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "app/iggy3d/creative/play/PlayLogicOverlay.hpp"
#include "app/iggy3d/creative/play/RuntimeSandbox.hpp"
#include "app/iggy3d/creative/input/InputRouter.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "render/FrameInput.hpp"
#include "runtime/physics/PhysicsSpatialSurfaceColliderBake.hpp"

namespace iggy3d_creative_app {

struct CreativePlayTuning {
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

enum class CreativePlayAction : std::uint8_t {
  None,
  Attack,
  Interact,
};

struct CreativePlayActionSample {
  bool attackDown = false;
  bool interactDown = false;
  bool enabled = true;
};

struct CreativePlayActionRouterState {
  bool attackDown = false;
  bool interactDown = false;
  bool rearmRequired = false;
};

[[nodiscard]] CreativePlayActionSample sampleCreativePlayActions(
    const iggy3d::creative::CreativeInputRouteResult& routedInput) noexcept;
[[nodiscard]] CreativePlayAction routeCreativePlayAction(
    CreativePlayActionRouterState& state,
    CreativePlayActionSample sample) noexcept;

enum class CreativePlayTargetStatus : std::uint8_t {
  None,
  Valid,
  Blocked,
  Friendly,
  OutOfRange,
  Unsupported,
  Defeated,
  Invalid,
};

struct CreativePlayTarget {
  CreativePlayTargetStatus status =
      CreativePlayTargetStatus::None;
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

struct CreativePlayTargetRequest {
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
    CreativePlayTargetStatus status) noexcept;
[[nodiscard]] CreativePlayTarget resolveCreativePlayTarget(
    const CreativePlayTargetRequest& request);
[[nodiscard]] bool creativeEditorPlayTargetAcceptsAction(
    const CreativePlayTarget& target,
    CreativePlayAction action) noexcept;

struct CreativePlaySession {
  std::optional<iggy3d::creative::CreativeRuntimeSandbox> sandbox;
  iggy3d::PhysicsSpatialSurfaceColliderBakeResult targetingBake;
  CreativePlayTuning tuning;
  CreativePlayActionRouterState actionRouter;
  CreativePlayTarget target;
  iggy3d::creative::CreativeRuntimeInteractionEffectReceipt
      lastInteractionEffect;
  iggy3d::creative::CreativeRuntimeAutomaticLogicReceipt lastAutomaticLogic;
  iggy3d::creative::CreativeRuntimeDoorUpdateReceipt lastDoors;
  iggy3d::creative::CreativeRuntimeMovingPlatformUpdateReceipt
      lastMovingPlatforms;
  std::uint64_t targetingGeometryRevision = 0U;
  std::size_t processedRuntimeEventCount = 0U;
  float cameraYawDegrees = 0.0F;
  float cameraPitchDegrees = 0.0F;
  std::uint64_t lastFrameTimeNanoseconds = 0U;
  std::uint64_t accumulatedTimeNanoseconds = 0U;
  bool clockPrimed = false;
};

enum class CreativePlayStartStatus : std::uint8_t {
  NotRequested,
  AlreadyActive,
  InvalidTuning,
  PreparationRejected,
  ActivationRejected,
  TargetingBakeRejected,
  Started,
};

struct CreativePlayStartRequest {
  const iggy3d::creative::CreativeDocument* document = nullptr;
  const iggy3d::StaticMeshAssetCatalog* staticMeshAssetCatalog = nullptr;
  iggy3d::creative::CreativeRuntimeSandboxConfig sandboxConfig;
  CreativePlayTuning tuning;
  std::string roomId = "creative_editor_play";
};

struct CreativePlayStartReceipt {
  bool requested = false;
  bool accepted = false;
  CreativePlayStartStatus status =
      CreativePlayStartStatus::NotRequested;
  std::string reasonCode = "creative_editor_play_not_requested";
  iggy3d::creative::CreativePlayPreparationStatus preparationStatus =
      iggy3d::creative::CreativePlayPreparationStatus::NotRequested;
  iggy3d::creative::CreativeRuntimeSandboxActivationReceipt activation;
};

[[nodiscard]] std::string_view toString(
    CreativePlayStartStatus status) noexcept;
[[nodiscard]] bool creativePlaySessionActive(
    const CreativePlaySession& mode) noexcept;

[[nodiscard]] CreativePlayStartReceipt startCreativePlaySession(
    CreativePlaySession& mode,
    CreativePlayStartRequest request);

[[nodiscard]] iggy3d::creative::CreativeRuntimeSandboxStopReceipt
stopCreativePlaySession(CreativePlaySession& mode) noexcept;

struct CreativePlayInput {
  float moveRight = 0.0F;
  float moveForward = 0.0F;
  float yawDeltaDegrees = 0.0F;
  float pitchDeltaDegrees = 0.0F;
  bool sprinting = false;
  bool windowFocused = true;
  CreativePlayActionSample actions;
};

enum class CreativePlayTickStatus : std::uint8_t {
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

struct CreativePlayTickRequest {
  const iggy3d::creative::CreativeDocument* sourceDocument = nullptr;
  CreativePlayInput input;
  std::uint64_t monotonicTimeNanoseconds = 0U;
};

struct CreativePlayTickReceipt {
  bool requested = false;
  bool active = false;
  CreativePlayTickStatus status =
      CreativePlayTickStatus::NotRequested;
  std::string reasonCode = "creative_editor_play_tick_not_requested";
  std::uint32_t ticksAdvanced = 0U;
  std::uint32_t commandsSubmitted = 0U;
  std::uint32_t movementCommandsSubmitted = 0U;
  std::uint32_t attackCommandsSubmitted = 0U;
  std::uint32_t interactionCommandsSubmitted = 0U;
  std::uint32_t interactionEffectsApplied = 0U;
  std::uint32_t automaticSourceTransitions = 0U;
  std::uint32_t automaticEffectsApplied = 0U;
  std::uint32_t doorsAdvanced = 0U;
  std::uint32_t doorsBlocked = 0U;
  std::uint32_t movingPlatformsAdvanced = 0U;
  std::uint32_t movingPlatformsBlocked = 0U;
  std::uint32_t platformRidersCarried = 0U;
  CreativePlayAction action = CreativePlayAction::None;
  bool actionAttempted = false;
  bool actionSubmitted = false;
  std::uint64_t sourceTick = 0U;
  iggy3d::CommandRejectionReason commandRejection =
      iggy3d::CommandRejectionReason::None;
  iggy3d::creative::CreativeRuntimeInteractionEffectStatus interactionEffect =
      iggy3d::creative::CreativeRuntimeInteractionEffectStatus::NotRequested;
  iggy3d::creative::CreativeRuntimeAutomaticLogicStatus automaticLogic =
      iggy3d::creative::CreativeRuntimeAutomaticLogicStatus::NotRequested;
  iggy3d::creative::CreativeRuntimeDoorUpdateStatus doors =
      iggy3d::creative::CreativeRuntimeDoorUpdateStatus::NotRequested;
  iggy3d::creative::CreativeRuntimeMovingPlatformUpdateStatus movingPlatforms =
      iggy3d::creative::CreativeRuntimeMovingPlatformUpdateStatus::NotRequested;
  std::uint64_t runtimeGeometryRevision = 0U;
};

[[nodiscard]] std::string_view toString(
    CreativePlayTickStatus status) noexcept;
[[nodiscard]] CreativePlayTickReceipt tickCreativePlaySession(
    CreativePlaySession& mode,
    const CreativePlayTickRequest& request);

struct CreativePlayScene {
  bool available = false;
  iggy3d::Vec3 cameraAnchorMeters;
  iggy3d::SceneProjectionResult scene;
  CreativePlayLogicOverlay logicOverlay;
};

[[nodiscard]] CreativePlayScene buildCreativePlaySessionScene(
    const CreativePlaySession& mode,
    iggy3d::creative::CreativeObjectId highlightedLogicSourceObjectId =
        iggy3d::creative::kInvalidObjectId);

struct CreativePlayView {
  bool available = false;
  iggy3d::Vec3 eyeMeters;
  iggy3d::Vec3 forward;
};

[[nodiscard]] CreativePlayView buildCreativePlayView(
    const CreativePlaySession& mode) noexcept;

inline constexpr std::size_t kCreativePlayHudRectCapacity = 10U;
inline constexpr std::size_t kCreativePlayHudGlyphQuadCapacity = 1664U;

struct CreativePlayHudFrame {
  std::array<iggy3d::RenderUiRect, kCreativePlayHudRectCapacity> rects{};
  std::array<iggy3d::DebugHudGlyphQuad,
             kCreativePlayHudGlyphQuadCapacity>
      glyphQuads{};
  std::size_t rectCount = 0U;
  std::size_t glyphQuadCount = 0U;
  std::size_t textGlyphCount = 0U;
  bool capacityExceeded = false;
};

[[nodiscard]] CreativePlayHudFrame buildCreativePlayHud(
    const CreativePlaySession& mode,
    const iggy3d::FrameInput& frame);
void attachCreativePlayHud(const CreativePlayHudFrame& hud,
                                 iggy3d::FrameInput& frame) noexcept;

}  // namespace iggy3d_creative_app
