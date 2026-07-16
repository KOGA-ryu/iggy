#include "EditorPlayMode.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

#include "core/math/OrientedBox.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/world/EntityState.hpp"

namespace iggy3d_creative_app {
namespace {

constexpr iggy3d::EntityId kLocalPlayerEntity{1U};
constexpr iggy3d::PlayerSlotId kLocalPlayerSlot = 0U;
constexpr float kInputEpsilon = 0.0001F;
constexpr float kPi = 3.14159265358979323846F;
constexpr std::uint32_t kMaximumCatchUpTickLimit = 16U;
bool validTuning(const CreativeEditorPlayTuning& tuning,
                 const iggy3d::RuntimeConfig& runtimeConfig) noexcept {
  if (!std::isfinite(tuning.walkSpeedMetersPerSecond) ||
      tuning.walkSpeedMetersPerSecond <= 0.0F ||
      !std::isfinite(tuning.sprintMultiplier) ||
      tuning.sprintMultiplier < 1.0F ||
      !std::isfinite(tuning.minimumPitchDegrees) ||
      !std::isfinite(tuning.maximumPitchDegrees) ||
      tuning.minimumPitchDegrees >= tuning.maximumPitchDegrees ||
      !std::isfinite(tuning.targetProbeDistanceMeters) ||
      tuning.targetProbeDistanceMeters <= 0.0F ||
      !std::isfinite(tuning.targetRadiusMeters) ||
      tuning.targetRadiusMeters < 0.0F ||
      !std::isfinite(tuning.targetOcclusionMarginMeters) ||
      tuning.targetOcclusionMarginMeters < 0.0F ||
      tuning.attackDamage <= 0 ||
      tuning.maximumCatchUpTicks == 0U ||
      tuning.maximumCatchUpTicks > kMaximumCatchUpTickLimit ||
      runtimeConfig.fixedTickRateHz == 0U) {
    return false;
  }
  const float maximumStep =
      tuning.walkSpeedMetersPerSecond * tuning.sprintMultiplier /
      static_cast<float>(runtimeConfig.fixedTickRateHz);
  return std::isfinite(maximumStep) &&
         maximumStep <= runtimeConfig.movementDistanceMeters;
}

void resetPlayClock(CreativeEditorPlayMode& mode) noexcept {
  mode.lastFrameTimeNanoseconds = 0U;
  mode.accumulatedTimeNanoseconds = 0U;
  mode.clockPrimed = false;
}

std::uint64_t tickDurationNanoseconds(std::uint32_t tickRateHz) noexcept {
  constexpr std::uint64_t kNanosecondsPerSecond = 1'000'000'000ULL;
  return std::max<std::uint64_t>(1U, kNanosecondsPerSecond / tickRateHz);
}

bool finiteInput(const CreativeEditorPlayInput& input) noexcept {
  return std::isfinite(input.moveRight) &&
         std::isfinite(input.moveForward) &&
         std::isfinite(input.yawDeltaDegrees) &&
         std::isfinite(input.pitchDeltaDegrees);
}

iggy3d::Vec3 horizontalStep(const CreativeEditorPlayInput& input,
                            float yawDegrees,
                            const CreativeEditorPlayTuning& tuning,
                            std::uint32_t tickRateHz) noexcept {
  const float moveRight = std::clamp(input.moveRight, -1.0F, 1.0F);
  const float moveForward = std::clamp(input.moveForward, -1.0F, 1.0F);
  const float magnitude =
      std::sqrt(moveRight * moveRight + moveForward * moveForward);
  if (!(magnitude > kInputEpsilon)) {
    return {};
  }

  const float yawRadians = yawDegrees * kPi / 180.0F;
  const iggy3d::Vec3 forward{std::sin(yawRadians), 0.0F,
                             -std::cos(yawRadians)};
  const iggy3d::Vec3 right{std::cos(yawRadians), 0.0F,
                           std::sin(yawRadians)};
  const float analogMagnitude = std::min(magnitude, 1.0F);
  const float speed =
      tuning.walkSpeedMetersPerSecond *
      (input.sprinting ? tuning.sprintMultiplier : 1.0F);
  const float distance =
      speed * analogMagnitude / static_cast<float>(tickRateHz);
  return (right * moveRight + forward * moveForward) *
         (distance / magnitude);
}

iggy3d::CommandRecord makeLocalPlayerCommand(
    const iggy3d::creative::CreativeRuntimeSandbox& sandbox,
    iggy3d::Vec3 movementStep) {
  iggy3d::CommandRecord command;
  command.playerSlot = kLocalPlayerSlot;
  command.actor = kLocalPlayerEntity;
  command.source = iggy3d::CommandSource::LocalPlayer;
  const iggy3d::EntityState* player =
      sandbox.session.state().world.findById(kLocalPlayerEntity);
  if (player == nullptr || iggy3d::lengthSquared(movementStep) <=
                               kInputEpsilon * kInputEpsilon) {
    command.kind = iggy3d::CommandKind::Wait;
    return command;
  }
  command.kind = iggy3d::CommandKind::Move;
  command.payload.target.hasPoint = true;
  command.payload.target.point = player->transform.position + movementStep;
  return command;
}

iggy3d::CommandRecord makeLocalPlayerActionCommand(
    const CreativeEditorPlayMode& mode,
    CreativeEditorPlayAction action) {
  iggy3d::CommandRecord command;
  command.playerSlot = kLocalPlayerSlot;
  command.actor = kLocalPlayerEntity;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.target.hasEntity = true;
  command.payload.target.entity = mode.target.entity;
  switch (action) {
    case CreativeEditorPlayAction::Attack:
      command.kind = iggy3d::CommandKind::Attack;
      command.payload.attackDamage = mode.tuning.attackDamage;
      break;
    case CreativeEditorPlayAction::Interact:
      command.kind = iggy3d::CommandKind::Interact;
      break;
    case CreativeEditorPlayAction::None:
      break;
  }
  return command;
}

CreativeEditorPlayTarget resolveModeTarget(
    const CreativeEditorPlayMode& mode) {
  if (!mode.sandbox.has_value() || !mode.targetingBake.ok) {
    CreativeEditorPlayTarget invalid;
    invalid.status = CreativeEditorPlayTargetStatus::Invalid;
    return invalid;
  }
  const CreativeEditorPlayView view = buildCreativeEditorPlayView(mode);
  if (!view.available) {
    CreativeEditorPlayTarget invalid;
    invalid.status = CreativeEditorPlayTargetStatus::Invalid;
    return invalid;
  }
  const iggy3d::SessionState& state = mode.sandbox->session.state();
  CreativeEditorPlayTarget result = resolveCreativeEditorPlayTarget(
      {&state.world,
       &state.combat,
       mode.targetingBake.colliders,
       kLocalPlayerEntity,
       view.eyeMeters,
       view.forward,
       mode.tuning.targetProbeDistanceMeters,
       mode.tuning.targetRadiusMeters,
       state.config.interactionRangeMeters,
       mode.tuning.targetOcclusionMarginMeters,
       mode.tuning.attackDamage});
  result.displayName = result.stableName;
  const iggy3d::creative::CreativeRuntimeInteractableState* interactable =
      iggy3d::creative::findCreativeRuntimeInteractable(*mode.sandbox,
                                                       result.entity);
  if (interactable != nullptr) {
    result.displayName = interactable->definition.displayName;
    switch (interactable->definition.kind) {
      case iggy3d::creative::CreativeRuntimeInteractableKind::Door:
        result.actionPrompt = interactable->doorOpen ? "CLOSE" : "OPEN";
        break;
      case iggy3d::creative::CreativeRuntimeInteractableKind::Control:
        result.actionPrompt = "ACTIVATE";
        break;
      case iggy3d::creative::CreativeRuntimeInteractableKind::Pickup:
        result.actionPrompt = "PICK UP";
        break;
    }
  } else if (result.friendly && result.supportsInteract) {
    result.actionPrompt = "INTERACT";
  } else if (result.supportsAttack) {
    result.actionPrompt = "ATTACK";
  } else if (result.supportsInteract) {
    result.actionPrompt = "INTERACT";
  }
  return result;
}

bool refreshTargetingBake(CreativeEditorPlayMode& mode) {
  if (!mode.sandbox.has_value()) {
    return false;
  }
  iggy3d::PhysicsSpatialSurfaceColliderBakeRequest request;
  request.surfaces = &mode.sandbox->collisionSurfaces;
  iggy3d::PhysicsSpatialSurfaceColliderBakeResult baked =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces(request);
  if (!baked.ok) {
    return false;
  }
  mode.targetingBake = std::move(baked);
  mode.targetingGeometryRevision = mode.sandbox->geometryRevision;
  return true;
}

bool applyRuntimeInteractionEvents(
    CreativeEditorPlayMode& mode,
    CreativeEditorPlayTickReceipt& receipt) {
  if (!mode.sandbox.has_value()) {
    return false;
  }
  iggy3d::creative::CreativeRuntimeSandbox& sandbox = *mode.sandbox;
  const std::vector<iggy3d::RuntimeEvent>& events =
      sandbox.session.state().transient.events;
  if (mode.processedRuntimeEventCount > events.size()) {
    return false;
  }
  for (std::size_t index = mode.processedRuntimeEventCount;
       index < events.size(); ++index) {
    const iggy3d::RuntimeEvent& event = events[index];
    if (event.kind != iggy3d::RuntimeEventKind::Interacted ||
        iggy3d::creative::findCreativeRuntimeInteractable(sandbox,
                                                         event.target) ==
            nullptr) {
      continue;
    }
    iggy3d::creative::CreativeRuntimeInteractionEffectReceipt effect =
        iggy3d::creative::applyCreativeRuntimeInteractionEffect(sandbox,
                                                               event.target);
    mode.lastInteractionEffect = effect;
    receipt.interactionEffect = effect.status;
    ++receipt.interactionEffectsApplied;
    if (!effect.accepted ||
        effect.status == iggy3d::creative::
                             CreativeRuntimeInteractionEffectStatus::
                                 GeometryRejected) {
      return false;
    }
  }
  mode.processedRuntimeEventCount = events.size();

  const iggy3d::creative::CreativeRuntimeAutomaticLogicReceipt automatic =
      iggy3d::creative::updateCreativeRuntimeAutomaticLogic(sandbox);
  mode.lastAutomaticLogic = automatic;
  receipt.automaticLogic = automatic.status;
  receipt.automaticSourceTransitions +=
      static_cast<std::uint32_t>(automatic.occupancyTransitionCount);
  receipt.automaticEffectsApplied +=
      static_cast<std::uint32_t>(automatic.activationEffectCount);
  if (!automatic.accepted) {
    return false;
  }
  if (mode.targetingGeometryRevision != sandbox.geometryRevision &&
      !refreshTargetingBake(mode)) {
    return false;
  }
  receipt.runtimeGeometryRevision = sandbox.geometryRevision;
  return true;
}

void failAndStop(CreativeEditorPlayMode& mode,
                 CreativeEditorPlayTickReceipt& receipt,
                 CreativeEditorPlayTickStatus status,
                 std::string reasonCode) noexcept {
  static_cast<void>(stopCreativeEditorPlayMode(mode));
  receipt.active = false;
  receipt.status = status;
  receipt.reasonCode = std::move(reasonCode);
}

}  // namespace

std::string_view toString(CreativeEditorPlayStartStatus status) noexcept {
  switch (status) {
    case CreativeEditorPlayStartStatus::NotRequested:
      return "not_requested";
    case CreativeEditorPlayStartStatus::AlreadyActive:
      return "already_active";
    case CreativeEditorPlayStartStatus::InvalidTuning:
      return "invalid_tuning";
    case CreativeEditorPlayStartStatus::PreparationRejected:
      return "preparation_rejected";
    case CreativeEditorPlayStartStatus::ActivationRejected:
      return "activation_rejected";
    case CreativeEditorPlayStartStatus::TargetingBakeRejected:
      return "targeting_bake_rejected";
    case CreativeEditorPlayStartStatus::Started:
      return "started";
  }
  return "not_requested";
}

bool creativeEditorPlayModeActive(const CreativeEditorPlayMode& mode) noexcept {
  return mode.sandbox.has_value();
}

CreativeEditorPlayStartReceipt startCreativeEditorPlayMode(
    CreativeEditorPlayMode& mode,
    CreativeEditorPlayStartRequest request) {
  CreativeEditorPlayStartReceipt receipt;
  receipt.requested = true;
  if (creativeEditorPlayModeActive(mode)) {
    receipt.status = CreativeEditorPlayStartStatus::AlreadyActive;
    receipt.reasonCode = "creative_editor_play_already_active";
    return receipt;
  }
  if (!validTuning(request.tuning, request.sandboxConfig.runtimeConfig)) {
    receipt.status = CreativeEditorPlayStartStatus::InvalidTuning;
    receipt.reasonCode = "creative_editor_play_tuning_invalid";
    return receipt;
  }

  iggy3d::creative::CreativePlayPreparationRequest preparationRequest;
  preparationRequest.document = request.document;
  preparationRequest.staticMeshAssetCatalog = request.staticMeshAssetCatalog;
  preparationRequest.roomId = std::move(request.roomId);
  iggy3d::creative::CreativePlayPreparationResult prepared =
      iggy3d::creative::prepareCreativePlay(preparationRequest);
  receipt.preparationStatus = prepared.status;
  if (!prepared.accepted || !prepared.payload.has_value()) {
    receipt.status = CreativeEditorPlayStartStatus::PreparationRejected;
    receipt.reasonCode = std::string(prepared.reasonCode);
    return receipt;
  }

  iggy3d::creative::CreativeRuntimeSandboxActivationRequest activationRequest;
  activationRequest.sourceDocument = request.document;
  activationRequest.payload = std::move(*prepared.payload);
  activationRequest.config = std::move(request.sandboxConfig);
  iggy3d::creative::CreativeRuntimeSandboxActivationResult activated =
      iggy3d::creative::activateCreativeRuntimeSandbox(
          std::move(activationRequest));
  receipt.activation = activated.receipt;
  if (!activated.receipt.accepted || !activated.sandbox.has_value()) {
    receipt.status = CreativeEditorPlayStartStatus::ActivationRejected;
    receipt.reasonCode = activated.receipt.reasonCode;
    return receipt;
  }

  mode.sandbox = std::move(activated.sandbox);
  iggy3d::PhysicsSpatialSurfaceColliderBakeRequest targetingBakeRequest;
  targetingBakeRequest.surfaces = &mode.sandbox->collisionSurfaces;
  mode.targetingBake =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces(
          targetingBakeRequest);
  if (!mode.targetingBake.ok) {
    static_cast<void>(
        iggy3d::creative::stopCreativeRuntimeSandbox(mode.sandbox));
    receipt.status = CreativeEditorPlayStartStatus::TargetingBakeRejected;
    receipt.reasonCode = "creative_editor_play_targeting_bake_rejected";
    return receipt;
  }
  mode.tuning = request.tuning;
  mode.targetingGeometryRevision = mode.sandbox->geometryRevision;
  mode.actionRouter = {};
  mode.target = {};
  mode.lastInteractionEffect = {};
  mode.lastAutomaticLogic = {};
  mode.processedRuntimeEventCount = 0U;
  mode.cameraYawDegrees = 0.0F;
  mode.cameraPitchDegrees = 0.0F;
  resetPlayClock(mode);
  receipt.accepted = true;
  receipt.status = CreativeEditorPlayStartStatus::Started;
  receipt.reasonCode = "creative_editor_play_started";
  return receipt;
}

iggy3d::creative::CreativeRuntimeSandboxStopReceipt stopCreativeEditorPlayMode(
    CreativeEditorPlayMode& mode) noexcept {
  iggy3d::creative::CreativeRuntimeSandboxStopReceipt receipt =
      iggy3d::creative::stopCreativeRuntimeSandbox(mode.sandbox);
  mode.targetingBake = {};
  mode.targetingGeometryRevision = 0U;
  mode.actionRouter = {};
  mode.target = {};
  mode.lastInteractionEffect = {};
  mode.lastAutomaticLogic = {};
  mode.processedRuntimeEventCount = 0U;
  resetPlayClock(mode);
  return receipt;
}

std::string_view toString(CreativeEditorPlayTickStatus status) noexcept {
  switch (status) {
    case CreativeEditorPlayTickStatus::NotRequested:
      return "not_requested";
    case CreativeEditorPlayTickStatus::Inactive:
      return "inactive";
    case CreativeEditorPlayTickStatus::SourceDocumentChanged:
      return "source_document_changed";
    case CreativeEditorPlayTickStatus::InvalidInput:
      return "invalid_input";
    case CreativeEditorPlayTickStatus::Suspended:
      return "suspended";
    case CreativeEditorPlayTickStatus::ClockPrimed:
      return "clock_primed";
    case CreativeEditorPlayTickStatus::NoTickDue:
      return "no_tick_due";
    case CreativeEditorPlayTickStatus::CommandRejected:
      return "command_rejected";
    case CreativeEditorPlayTickStatus::RuntimeTickFailed:
      return "runtime_tick_failed";
    case CreativeEditorPlayTickStatus::Advanced:
      return "advanced";
  }
  return "not_requested";
}

CreativeEditorPlayTickReceipt tickCreativeEditorPlayMode(
    CreativeEditorPlayMode& mode,
    const CreativeEditorPlayTickRequest& request) {
  CreativeEditorPlayTickReceipt receipt;
  receipt.requested = true;
  receipt.active = creativeEditorPlayModeActive(mode);
  if (!receipt.active) {
    receipt.status = CreativeEditorPlayTickStatus::Inactive;
    receipt.reasonCode = "creative_editor_play_inactive";
    return receipt;
  }

  iggy3d::creative::CreativeRuntimeSandbox& sandbox = *mode.sandbox;
  receipt.runtimeGeometryRevision = sandbox.geometryRevision;
  if (request.sourceDocument == nullptr ||
      !iggy3d::creative::creativeRuntimeSandboxIsCurrent(
          sandbox, *request.sourceDocument)) {
    failAndStop(mode, receipt,
                CreativeEditorPlayTickStatus::SourceDocumentChanged,
                "creative_editor_play_source_document_changed");
    return receipt;
  }
  if (!finiteInput(request.input)) {
    resetPlayClock(mode);
    mode.actionRouter = {};
    mode.actionRouter.rearmRequired = true;
    mode.target = {};
    mode.target.status = CreativeEditorPlayTargetStatus::Invalid;
    receipt.status = CreativeEditorPlayTickStatus::InvalidInput;
    receipt.reasonCode = "creative_editor_play_input_invalid";
    return receipt;
  }
  if (!request.input.windowFocused) {
    resetPlayClock(mode);
    mode.actionRouter = {};
    mode.actionRouter.rearmRequired = true;
    mode.target = {};
    receipt.status = CreativeEditorPlayTickStatus::Suspended;
    receipt.reasonCode = "creative_editor_play_suspended";
    receipt.sourceTick = sandbox.session.state().clock.tickIndex;
    return receipt;
  }

  mode.cameraYawDegrees = std::remainder(
      mode.cameraYawDegrees + request.input.yawDeltaDegrees, 360.0F);
  mode.cameraPitchDegrees = std::clamp(
      mode.cameraPitchDegrees + request.input.pitchDeltaDegrees,
      mode.tuning.minimumPitchDegrees, mode.tuning.maximumPitchDegrees);

  mode.target = resolveModeTarget(mode);
  receipt.action =
      routeCreativeEditorPlayAction(mode.actionRouter, request.input.actions);
  receipt.actionAttempted =
      receipt.action != CreativeEditorPlayAction::None;
  if (creativeEditorPlayTargetAcceptsAction(mode.target, receipt.action)) {
    const iggy3d::CommandRecord actionCommand =
        makeLocalPlayerActionCommand(mode, receipt.action);
    const iggy3d::SessionCommandResult submitted =
        sandbox.session.submitCommand(actionCommand);
    if (submitted.admission.command.admission !=
        iggy3d::CommandAdmissionStatus::Accepted) {
      receipt.status = CreativeEditorPlayTickStatus::CommandRejected;
      receipt.reasonCode = "creative_editor_play_action_rejected";
      receipt.commandRejection = submitted.admission.firstFailure;
      receipt.sourceTick = sandbox.session.state().clock.tickIndex;
      return receipt;
    }
    receipt.actionSubmitted = true;
    ++receipt.commandsSubmitted;
    receipt.attackCommandsSubmitted +=
        receipt.action == CreativeEditorPlayAction::Attack ? 1U : 0U;
    receipt.interactionCommandsSubmitted +=
        receipt.action == CreativeEditorPlayAction::Interact ? 1U : 0U;
  }

  if (!mode.clockPrimed) {
    mode.lastFrameTimeNanoseconds = request.monotonicTimeNanoseconds;
    mode.clockPrimed = true;
    receipt.status = CreativeEditorPlayTickStatus::ClockPrimed;
    receipt.reasonCode = "creative_editor_play_clock_primed";
    receipt.sourceTick = sandbox.session.state().clock.tickIndex;
    return receipt;
  }
  if (request.monotonicTimeNanoseconds < mode.lastFrameTimeNanoseconds) {
    resetPlayClock(mode);
    receipt.status = CreativeEditorPlayTickStatus::InvalidInput;
    receipt.reasonCode = "creative_editor_play_time_reversed";
    return receipt;
  }

  const std::uint32_t tickRateHz = sandbox.session.state().config.fixedTickRateHz;
  const std::uint64_t tickNanoseconds = tickDurationNanoseconds(tickRateHz);
  const std::uint64_t elapsed =
      request.monotonicTimeNanoseconds - mode.lastFrameTimeNanoseconds;
  mode.lastFrameTimeNanoseconds = request.monotonicTimeNanoseconds;
  const std::uint64_t maximumAccumulated =
      tickNanoseconds * mode.tuning.maximumCatchUpTicks;
  const std::uint64_t available =
      maximumAccumulated -
      std::min(mode.accumulatedTimeNanoseconds, maximumAccumulated);
  mode.accumulatedTimeNanoseconds =
      std::min(maximumAccumulated,
               mode.accumulatedTimeNanoseconds + std::min(elapsed, available));

  const iggy3d::Vec3 movementStep = horizontalStep(
      request.input, mode.cameraYawDegrees, mode.tuning, tickRateHz);
  while (mode.accumulatedTimeNanoseconds >= tickNanoseconds &&
         receipt.ticksAdvanced < mode.tuning.maximumCatchUpTicks) {
    iggy3d::CommandRecord command =
        makeLocalPlayerCommand(sandbox, movementStep);
    const bool movement = command.kind == iggy3d::CommandKind::Move;
    const iggy3d::SessionCommandResult submitted =
        sandbox.session.submitCommand(command);
    if (submitted.admission.command.admission !=
        iggy3d::CommandAdmissionStatus::Accepted) {
      receipt.status = CreativeEditorPlayTickStatus::CommandRejected;
      receipt.reasonCode = "creative_editor_play_command_rejected";
      receipt.commandRejection = submitted.admission.firstFailure;
      receipt.sourceTick = sandbox.session.state().clock.tickIndex;
      return receipt;
    }
    ++receipt.commandsSubmitted;
    receipt.movementCommandsSubmitted += movement ? 1U : 0U;

    const iggy3d::StatusResult tick = sandbox.session.tickWithOptions(
        {&sandbox.collisionSurfaces, true});
    if (tick.status != iggy3d::ResultStatus::Ok) {
      failAndStop(mode, receipt,
                  CreativeEditorPlayTickStatus::RuntimeTickFailed,
                  std::string(tick.error.code));
      return receipt;
    }
    if (!applyRuntimeInteractionEvents(mode, receipt)) {
      failAndStop(mode, receipt,
                  CreativeEditorPlayTickStatus::RuntimeTickFailed,
                  "creative_editor_play_interaction_effect_failed");
      return receipt;
    }
    mode.accumulatedTimeNanoseconds -= tickNanoseconds;
    ++receipt.ticksAdvanced;
  }

  receipt.active = true;
  receipt.sourceTick = sandbox.session.state().clock.tickIndex;
  if (receipt.ticksAdvanced == 0U) {
    receipt.status = CreativeEditorPlayTickStatus::NoTickDue;
    receipt.reasonCode = "creative_editor_play_no_tick_due";
    return receipt;
  }
  receipt.status = CreativeEditorPlayTickStatus::Advanced;
  receipt.reasonCode = "creative_editor_play_advanced";
  return receipt;
}

CreativeEditorPlayScene buildCreativeEditorPlayScene(
    const CreativeEditorPlayMode& mode,
    iggy3d::creative::CreativeObjectId highlightedLogicSourceObjectId) {
  CreativeEditorPlayScene result;
  if (!mode.sandbox.has_value()) {
    return result;
  }
  const iggy3d::creative::CreativeRuntimeSandbox& sandbox = *mode.sandbox;
  result.scene =
      iggy3d::buildSceneProjection(sandbox.session.state(), &sandbox.room);
  result.logicOverlay = buildCreativeEditorPlayLogicOverlay(
      sandbox, highlightedLogicSourceObjectId);
  const iggy3d::EntityState* player =
      sandbox.session.state().world.findById(kLocalPlayerEntity);
  if (player == nullptr) {
    return result;
  }
  result.cameraAnchorMeters = player->transform.position;
  result.available = result.scene.room.loaded && result.scene.playerCount == 1U;
  return result;
}

}  // namespace iggy3d_creative_app
