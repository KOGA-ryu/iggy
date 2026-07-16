#include "EditorMovingPlatformPreview.hpp"

#include <algorithm>
#include <cmath>

#include "app/iggy3d/creative/Geometry.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

constexpr std::uint32_t kPreviewFixedTickRateHz = 60U;
constexpr std::uint32_t kPreviewMaximumCatchUpTicks = 15U;
constexpr double kPreviewMaximumDeltaSeconds = 0.25;

[[nodiscard]] bool sameVec3(iggy3d::Vec3 lhs, iggy3d::Vec3 rhs) noexcept {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

[[nodiscard]] bool sameTransform(const cr::CreativeTransform& lhs,
                                 const cr::CreativeTransform& rhs) noexcept {
  return cr::creativeVec3ExactlyEqual(lhs.position, rhs.position) &&
         cr::creativeVec3ExactlyEqual(lhs.rotationEulerRadians,
                                      rhs.rotationEulerRadians) &&
         cr::creativeVec3ExactlyEqual(lhs.scale, rhs.scale);
}

[[nodiscard]] bool sameDefinition(
    const cr::CreativeRuntimeMovingPlatformDefinition& lhs,
    const cr::CreativeRuntimeMovingPlatformDefinition& rhs) noexcept {
  if (lhs.pathPointCount != rhs.pathPointCount ||
      lhs.openLengthMeters != rhs.openLengthMeters ||
      lhs.loopLengthMeters != rhs.loopLengthMeters ||
      lhs.speedMetersPerSecond != rhs.speedMetersPerSecond ||
      lhs.traversalMode != rhs.traversalMode ||
      lhs.startsActive != rhs.startsActive ||
      !sameVec3(lhs.originPositionMeters, rhs.originPositionMeters)) {
    return false;
  }
  for (std::size_t index = 0U; index < lhs.pathPointCount; ++index) {
    if (!sameVec3(lhs.pathPoints[index], rhs.pathPoints[index]) ||
        lhs.cumulativeOpenMeters[index] != rhs.cumulativeOpenMeters[index] ||
        lhs.waypointDwellSeconds[index] !=
            rhs.waypointDwellSeconds[index]) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] double routeCycleLength(
    const cr::CreativeRuntimeMovingPlatformDefinition& definition) noexcept {
  return definition.traversalMode ==
                 cr::CreativeMovingPlatformTraversalMode::Loop
             ? static_cast<double>(definition.loopLengthMeters)
             : static_cast<double>(definition.openLengthMeters) * 2.0;
}

[[nodiscard]] double normalizedRoutePosition(
    const cr::CreativeRuntimeMovingPlatformDefinition& definition,
    double phaseMeters) noexcept {
  if (definition.traversalMode ==
      cr::CreativeMovingPlatformTraversalMode::Loop) {
    return phaseMeters / static_cast<double>(definition.loopLengthMeters);
  }
  const double openLength = static_cast<double>(definition.openLengthMeters);
  const double cycleLength = openLength * 2.0;
  const double routeDistance =
      phaseMeters <= openLength ? phaseMeters : cycleLength - phaseMeters;
  return routeDistance / openLength;
}

void setUnavailable(CreativeMovingPlatformPreviewState& state,
                    std::string_view reasonCode) noexcept {
  state = {};
  state.reasonCode = reasonCode;
}

[[nodiscard]] bool seek(CreativeMovingPlatformPreviewState& state,
                        double normalizedProgress) noexcept {
  const cr::CreativeRuntimeMovingPlatformSampleResult sampled =
      cr::sampleCreativeRuntimeMovingPlatformProgress(
          state.definition, normalizedProgress);
  if (!sampled.ok) {
    state.status = CreativeMovingPlatformPreviewStatus::Invalid;
    state.reasonCode = sampled.reasonCode;
    state.playing = false;
    return false;
  }
  state.runtimeState = sampled.state;
  state.normalizedProgress = sampled.normalizedProgress;
  state.fixedTickAccumulatorSeconds = 0.0;
  state.reasonCode = "creative_platform_preview_ready";
  return true;
}

}  // namespace

CreativeMovingPlatformPreviewReceipt syncCreativeMovingPlatformPreview(
    CreativeMovingPlatformPreviewState& state,
    cr::CreativeDocumentId documentId,
    const cr::CreativeObject* selectedObject) noexcept {
  CreativeMovingPlatformPreviewReceipt result;
  result.accepted = true;
  if (selectedObject == nullptr ||
      selectedObject->kind != cr::CreativeObjectKind::MovingPlatform) {
    const bool changed = state.available || state.visible || state.playing;
    setUnavailable(state, "creative_platform_preview_no_selection");
    result.changed = changed;
    result.reasonCode = state.reasonCode;
    return result;
  }

  const cr::CreativeCoreVec3Conversion origin =
      cr::creativeVec3ToCoreChecked(selectedObject->transform.position);
  if (!origin.converted) {
    setUnavailable(state, "creative_platform_preview_origin_invalid");
    result.accepted = false;
    result.changed = true;
    result.reasonCode = state.reasonCode;
    return result;
  }
  const cr::CreativeRuntimeMovingPlatformBuildResult built =
      cr::buildCreativeRuntimeMovingPlatformDefinition(
          selectedObject->pathPoints, selectedObject->movingPlatform,
          origin.value);
  if (!built.ok) {
    setUnavailable(state, built.reasonCode);
    result.accepted = false;
    result.changed = true;
    result.reasonCode = state.reasonCode;
    return result;
  }

  const bool sameObject =
      state.available && state.documentId == documentId &&
      state.objectId == selectedObject->id;
  if (sameObject &&
      sameTransform(state.authoredTransform, selectedObject->transform) &&
      sameDefinition(state.definition, built.definition)) {
    result.reasonCode = "creative_platform_preview_unchanged";
    return result;
  }

  const bool resume = sameObject && state.playing;
  state = {};
  state.documentId = documentId;
  state.objectId = selectedObject->id;
  state.authoredTransform = selectedObject->transform;
  state.definition = built.definition;
  state.available = true;
  state.visible = true;
  state.playing = resume;
  if (!seek(state, 0.0)) {
    result.accepted = false;
    result.changed = true;
    result.reasonCode = state.reasonCode;
    return result;
  }
  state.status = resume ? CreativeMovingPlatformPreviewStatus::Playing
                        : CreativeMovingPlatformPreviewStatus::Paused;
  result.changed = true;
  result.reset = true;
  result.reasonCode = sameObject ? "creative_platform_preview_route_reset"
                                 : "creative_platform_preview_selected";
  return result;
}

CreativeMovingPlatformPreviewReceipt
applyCreativeMovingPlatformPreviewCommand(
    CreativeMovingPlatformPreviewState& state,
    CreativeMovingPlatformPreviewCommand command,
    cr::CreativeObjectId objectId,
    double normalizedProgress) noexcept {
  CreativeMovingPlatformPreviewReceipt result;
  if (!state.available || objectId == cr::kInvalidObjectId ||
      objectId != state.objectId) {
    result.reasonCode = "creative_platform_preview_target_mismatch";
    return result;
  }

  switch (command) {
    case CreativeMovingPlatformPreviewCommand::TogglePlayback:
      state.visible = true;
      state.playing = !state.playing;
      state.status = state.playing
                         ? CreativeMovingPlatformPreviewStatus::Playing
                         : CreativeMovingPlatformPreviewStatus::Paused;
      state.reasonCode = state.playing
                             ? "creative_platform_preview_playing"
                             : "creative_platform_preview_paused";
      result.accepted = true;
      result.changed = true;
      break;
    case CreativeMovingPlatformPreviewCommand::Restart:
      result.accepted = seek(state, 0.0);
      result.changed = result.accepted;
      result.reset = result.accepted;
      state.visible = result.accepted;
      state.status = state.playing
                         ? CreativeMovingPlatformPreviewStatus::Playing
                         : CreativeMovingPlatformPreviewStatus::Paused;
      state.reasonCode = result.accepted
                             ? "creative_platform_preview_restarted"
                             : state.reasonCode;
      break;
    case CreativeMovingPlatformPreviewCommand::Seek:
      result.accepted = seek(state, normalizedProgress);
      result.changed = result.accepted;
      if (result.accepted) {
        state.visible = true;
        state.playing = false;
        state.status = CreativeMovingPlatformPreviewStatus::Paused;
        state.reasonCode = "creative_platform_preview_seeked";
      }
      break;
    case CreativeMovingPlatformPreviewCommand::Count:
      result.reasonCode = "creative_platform_preview_command_invalid";
      return result;
  }
  result.reasonCode = state.reasonCode;
  return result;
}

CreativeMovingPlatformPreviewReceipt advanceCreativeMovingPlatformPreview(
    CreativeMovingPlatformPreviewState& state,
    double presentationDeltaSeconds) noexcept {
  CreativeMovingPlatformPreviewReceipt result;
  if (!state.available || !state.playing) {
    result.accepted = state.available;
    result.reasonCode = state.reasonCode;
    return result;
  }
  if (!std::isfinite(presentationDeltaSeconds) ||
      presentationDeltaSeconds < 0.0) {
    state.playing = false;
    state.status = CreativeMovingPlatformPreviewStatus::Invalid;
    state.reasonCode = "creative_platform_preview_delta_invalid";
    result.reasonCode = state.reasonCode;
    return result;
  }

  state.fixedTickAccumulatorSeconds +=
      std::min(presentationDeltaSeconds, kPreviewMaximumDeltaSeconds);
  constexpr double kTickSeconds =
      1.0 / static_cast<double>(kPreviewFixedTickRateHz);
  const std::uint32_t dueTicks = static_cast<std::uint32_t>(std::min(
      std::floor(state.fixedTickAccumulatorSeconds / kTickSeconds),
      static_cast<double>(kPreviewMaximumCatchUpTicks)));
  state.fixedTickAccumulatorSeconds -=
      static_cast<double>(dueTicks) * kTickSeconds;
  result.accepted = true;
  for (std::uint32_t tick = 0U; tick < dueTicks; ++tick) {
    const cr::CreativeRuntimeMovingPlatformStepResult step =
        cr::planCreativeRuntimeMovingPlatformStep(
            {&state.definition, &state.runtimeState,
             kPreviewFixedTickRateHz, true});
    if (!step.ok) {
      state.playing = false;
      state.status = CreativeMovingPlatformPreviewStatus::Invalid;
      state.reasonCode = step.reasonCode;
      result.accepted = false;
      result.reasonCode = step.reasonCode;
      return result;
    }
    state.runtimeState = step.nextState;
    ++result.advancedTickCount;
  }
  const double cycleLength = routeCycleLength(state.definition);
  if (cycleLength <= 0.0 || !std::isfinite(cycleLength)) {
    state.playing = false;
    state.status = CreativeMovingPlatformPreviewStatus::Invalid;
    state.reasonCode = "creative_platform_preview_cycle_invalid";
    result.accepted = false;
    result.reasonCode = state.reasonCode;
    return result;
  }
  state.normalizedProgress = std::clamp(
      normalizedRoutePosition(state.definition,
                              state.runtimeState.phaseMeters),
      0.0, 1.0);
  state.status = CreativeMovingPlatformPreviewStatus::Playing;
  state.reasonCode = "creative_platform_preview_playing";
  result.changed = result.advancedTickCount > 0U;
  result.reasonCode = state.reasonCode;
  return result;
}

std::string_view creativeMovingPlatformPreviewStatusLabel(
    const CreativeMovingPlatformPreviewState& state) noexcept {
  switch (state.status) {
    case CreativeMovingPlatformPreviewStatus::Unavailable:
      return "Unavailable";
    case CreativeMovingPlatformPreviewStatus::Paused:
      return "Paused";
    case CreativeMovingPlatformPreviewStatus::Playing:
      return state.runtimeState.dwellTicksRemaining > 0U ? "Waiting"
                                                         : "Playing";
    case CreativeMovingPlatformPreviewStatus::Invalid:
      return "Invalid";
  }
  return "Invalid";
}

}  // namespace iggy3d_creative_app
