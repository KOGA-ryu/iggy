#include "EditorTransform.hpp"
#include "EditorTransformInternal.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <string_view>

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/input/UiInput.hpp"

namespace iggy3d_creative_app {
namespace {

[[nodiscard]] cr::CreativeSelectionPlacementAxis nextConstraint(
    cr::CreativeSelectionPlacementAxis constraint) noexcept {
  switch (constraint) {
    case cr::CreativeSelectionPlacementAxis::Free:
      return cr::CreativeSelectionPlacementAxis::X;
    case cr::CreativeSelectionPlacementAxis::X:
      return cr::CreativeSelectionPlacementAxis::Y;
    case cr::CreativeSelectionPlacementAxis::Y:
      return cr::CreativeSelectionPlacementAxis::Z;
    case cr::CreativeSelectionPlacementAxis::Z:
    case cr::CreativeSelectionPlacementAxis::Count:
      return cr::CreativeSelectionPlacementAxis::Free;
  }
  return cr::CreativeSelectionPlacementAxis::Free;
}

[[nodiscard]] cr::CreativeSelectionPlacementAxis stepConstraint(
    cr::CreativeSelectionPlacementAxis constraint,
    std::int32_t direction) noexcept {
  constexpr std::array rows{
      cr::CreativeSelectionPlacementAxis::Free,
      cr::CreativeSelectionPlacementAxis::X,
      cr::CreativeSelectionPlacementAxis::Y,
      cr::CreativeSelectionPlacementAxis::Z,
  };
  const auto found = std::find(rows.begin(), rows.end(), constraint);
  const std::size_t current =
      found == rows.end() ? 0U
                          : static_cast<std::size_t>(found - rows.begin());
  const cr::CreativeWrappedIndexResult next = cr::stepCreativeWrappedIndex(
      current, rows.size(), direction < 0 ? -1 : 1);
  return next.valid ? rows[next.index]
                    : cr::CreativeSelectionPlacementAxis::Free;
}

[[nodiscard]] cr::CreativeSelectionPlacementAxis stepPlanarConstraint(
    cr::CreativeSelectionPlacementAxis constraint,
    std::int32_t direction) noexcept {
  constexpr std::array rows{
      cr::CreativeSelectionPlacementAxis::Free,
      cr::CreativeSelectionPlacementAxis::X,
      cr::CreativeSelectionPlacementAxis::Z,
  };
  const auto found = std::find(rows.begin(), rows.end(), constraint);
  const std::size_t current =
      found == rows.end() ? 0U
                          : static_cast<std::size_t>(found - rows.begin());
  const cr::CreativeWrappedIndexResult next = cr::stepCreativeWrappedIndex(
      current, rows.size(), direction < 0 ? -1 : 1);
  return next.valid ? rows[next.index]
                    : cr::CreativeSelectionPlacementAxis::Free;
}

[[nodiscard]] bool transformScaleAvailable(
    const CreativeEditorSelectionTransformState& state) noexcept {
  return state.preflight.capabilities.scale !=
         cr::CreativeObjectScaleSupport::None;
}

[[nodiscard]] bool transformRotationAvailable(
    const CreativeEditorSelectionTransformState& state) noexcept {
  return state.preflight.capabilities.rotation !=
         cr::CreativeObjectRotationSupport::None;
}

[[nodiscard]] bool transformModeAvailable(
    const CreativeEditorSelectionTransformState& state,
    CreativeEditorTransformMode mode) noexcept {
  switch (mode) {
    case CreativeEditorTransformMode::Move:
      return detail::transformTranslationAvailable(state);
    case CreativeEditorTransformMode::Rotate:
      return transformRotationAvailable(state);
    case CreativeEditorTransformMode::Scale:
      return transformScaleAvailable(state);
    case CreativeEditorTransformMode::Count:
      return false;
  }
  return false;
}

}  // namespace

namespace detail {

[[nodiscard]] bool transformTranslationAvailable(
    const CreativeEditorSelectionTransformState& state) noexcept {
  return state.preflight.capabilities.translate;
}

[[nodiscard]] CreativeEditorTransformMode firstAvailableTransformMode(
    const CreativeEditorSelectionTransformState& state) noexcept {
  for (const CreativeEditorTransformMode mode : {
           CreativeEditorTransformMode::Move,
           CreativeEditorTransformMode::Rotate,
           CreativeEditorTransformMode::Scale}) {
    if (transformModeAvailable(state, mode)) {
      return mode;
    }
  }
  return CreativeEditorTransformMode::Count;
}

[[nodiscard]] bool quarterTurnDegrees(double degrees) noexcept {
  const double turns = degrees / 90.0;
  return std::isfinite(turns) &&
         std::abs(turns - std::round(turns)) <= 1.0e-9;
}

}  // namespace detail

namespace {

[[nodiscard]] std::size_t scaleAxisIndex(
    cr::CreativeSelectionPlacementAxis axis) noexcept {
  switch (axis) {
    case cr::CreativeSelectionPlacementAxis::X: return 0U;
    case cr::CreativeSelectionPlacementAxis::Y: return 1U;
    case cr::CreativeSelectionPlacementAxis::Z: return 2U;
    case cr::CreativeSelectionPlacementAxis::Free:
    case cr::CreativeSelectionPlacementAxis::Count:
      return 3U;
  }
  return 3U;
}

[[nodiscard]] cr::CreativeAxis3 nextRotationAxis(
    cr::CreativeAxis3 axis) noexcept {
  switch (axis) {
    case cr::CreativeAxis3::Y: return cr::CreativeAxis3::X;
    case cr::CreativeAxis3::X: return cr::CreativeAxis3::Z;
    case cr::CreativeAxis3::Z:
    case cr::CreativeAxis3::Count:
      return cr::CreativeAxis3::Y;
  }
  return cr::CreativeAxis3::Y;
}

[[nodiscard]] cr::CreativeVec3 scaleFactorFromIndices(
    const CreativeEditorSelectionTransformState& state) noexcept {
  const auto factor = [](std::size_t index) {
    return index < kCreativeEditorScaleFactors.size()
               ? kCreativeEditorScaleFactors[index]
               : 1.0;
  };
  return {factor(state.scaleFactorIndices[0]),
          factor(state.scaleFactorIndices[1]),
          factor(state.scaleFactorIndices[2])};
}

void syncScaleFactor(CreativeEditorSelectionTransformState& state) noexcept {
  state.request.scaleFactor = scaleFactorFromIndices(state);
}

void resetScaleFactor(CreativeEditorSelectionTransformState& state) noexcept {
  state.scaleFactorIndices.fill(kCreativeEditorDefaultScaleIndex);
  syncScaleFactor(state);
}

void syncRotation(CreativeEditorSelectionTransformState& state) noexcept {
  state.request.quarterTurns = 0U;
  state.request.rotationAxis = state.rotationAxis;
  state.request.rotationRadians =
      state.rotationDegrees * std::numbers::pi / 180.0;
  state.request.hasAxisAngleRotation =
      std::abs(state.rotationDegrees) > 1.0e-12;
}

[[nodiscard]] bool stepScaleIndex(std::size_t& index,
                                  std::int32_t direction) noexcept {
  const std::size_t before = index;
  if (direction < 0 && index > 0U) {
    --index;
  } else if (direction > 0 &&
             index + 1U < kCreativeEditorScaleFactors.size()) {
    ++index;
  }
  return index != before;
}

[[nodiscard]] bool adjustScaleFactor(
    CreativeEditorSelectionTransformState& state,
    std::int32_t direction) noexcept {
  bool changed = false;
  const std::size_t axisIndex = scaleAxisIndex(state.constraint);
  if (axisIndex < state.scaleFactorIndices.size()) {
    changed = stepScaleIndex(state.scaleFactorIndices[axisIndex], direction);
  } else {
    for (std::size_t& index : state.scaleFactorIndices) {
      changed = stepScaleIndex(index, direction) || changed;
    }
  }
  if (changed) {
    syncScaleFactor(state);
  }
  return changed;
}

[[nodiscard]] bool adjustRotation(
    CreativeEditorSelectionTransformState& state,
    std::int32_t direction) noexcept {
  if (direction == 0) {
    return false;
  }
  const std::int8_t before = state.rotationQuarterSteps;
  const std::int32_t next =
      (static_cast<std::int32_t>(before) + (direction < 0 ? -1 : 1)) % 4;
  state.rotationQuarterSteps = static_cast<std::int8_t>(next);
  state.rotationDegrees =
      static_cast<double>(state.rotationQuarterSteps) * 90.0;
  syncRotation(state);
  return state.rotationQuarterSteps != before;
}


[[nodiscard]] bool resolveTransformPivot(
    const CreativeEditorSelectionTransformState& state,
    CreativeEditorTransformPivot pivot,
    cr::CreativeVec3& sourceAnchor,
    cr::CreativeSelectionPlacementPivotMode& pivotMode) noexcept {
  sourceAnchor = state.sourceClipboard.placementAnchor;
  pivotMode = cr::CreativeSelectionPlacementPivotMode::SharedAnchor;
  switch (pivot) {
    case CreativeEditorTransformPivot::SelectionAnchor:
      return cr::isFiniteCreativeVec3(sourceAnchor);
    case CreativeEditorTransformPivot::ActiveObjectOrigin: {
      const cr::CreativeObject* active =
          detail::transformSourceObject(state, state.activeObjectId);
      return active != nullptr &&
             cr::resolveCreativeSelectionPlacementObjectOrigin(
                 *active, sourceAnchor);
    }
    case CreativeEditorTransformPivot::IndividualOrigins:
      pivotMode = cr::CreativeSelectionPlacementPivotMode::IndividualOrigins;
      return cr::isFiniteCreativeVec3(sourceAnchor);
    case CreativeEditorTransformPivot::Count:
      return false;
  }
  return false;
}

[[nodiscard]] bool resolveTransformCoordinateBasis(
    const CreativeEditorSelectionTransformState& state,
    cr::CreativeSelectionPlacementCoordinateSpace coordinateSpace,
    cr::CreativeVec3& basisEulerRadians) noexcept {
  basisEulerRadians = {};
  if (coordinateSpace ==
      cr::CreativeSelectionPlacementCoordinateSpace::World) {
    return true;
  }
  if (coordinateSpace !=
      cr::CreativeSelectionPlacementCoordinateSpace::Local) {
    return false;
  }
  const cr::CreativeObject* active =
      detail::transformSourceObject(state, state.activeObjectId);
  if (active == nullptr) {
    return false;
  }
  if (cr::objectHasTransform(active->kind)) {
    basisEulerRadians = active->transform.rotationEulerRadians;
  }
  return cr::isFiniteCreativeVec3(basisEulerRadians);
}


}  // namespace

std::string_view toString(CreativeEditorTransformControl control) noexcept {
  switch (control) {
    case CreativeEditorTransformControl::RotatePositive: return "RotatePositive";
    case CreativeEditorTransformControl::MirrorX: return "MirrorX";
    case CreativeEditorTransformControl::CycleConstraint:
      return "CycleConstraint";
    case CreativeEditorTransformControl::ToggleMode: return "ToggleMode";
    case CreativeEditorTransformControl::Confirm: return "Confirm";
    case CreativeEditorTransformControl::Cancel: return "Cancel";
    case CreativeEditorTransformControl::MirrorZ: return "MirrorZ";
    case CreativeEditorTransformControl::RotateNegative: return "RotateNegative";
    case CreativeEditorTransformControl::Reset: return "Reset";
    case CreativeEditorTransformControl::Count: return "Count";
  }
  return "Unknown";
}

std::string_view toString(CreativeEditorTransformMode mode) noexcept {
  switch (mode) {
    case CreativeEditorTransformMode::Move: return "MOVE";
    case CreativeEditorTransformMode::Rotate: return "ROTATE";
    case CreativeEditorTransformMode::Scale: return "SCALE";
    case CreativeEditorTransformMode::Count: break;
  }
  return "INVALID";
}

cr::CreativeVec3 creativeEditorTransformScaleFactor(
    const CreativeEditorSelectionTransformState& state) noexcept {
  return state.request.scaleFactor;
}


bool setCreativeEditorTransformConstraint(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeSelectionPlacementAxis constraint) {
  if (!state.active || !detail::transformTranslationAvailable(state) ||
      static_cast<std::size_t>(constraint) >=
          static_cast<std::size_t>(
              cr::CreativeSelectionPlacementAxis::Count) ||
      (detail::planarSourceRoute(state) &&
       constraint == cr::CreativeSelectionPlacementAxis::Y)) {
    return false;
  }
  if (state.constraint == constraint) {
    return false;
  }
  state.constraint = constraint;
  state.lastCommit = {};
  state.lastNudge = {};
  detail::refreshResolvedTarget(appState, state);
  return true;
}

bool setCreativeEditorTransformTargetAnchor(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeVec3 targetAnchor) {
  if (!state.active || !detail::transformTranslationAvailable(state) ||
      !cr::isFiniteCreativeVec3(targetAnchor)) {
    return false;
  }
  const bool changed =
      state.anchorPolicy != CreativeEditorTransformAnchorPolicy::FixedTarget ||
      !cr::creativeVec3ExactlyEqual(state.aimTargetAnchor, targetAnchor) ||
      !cr::creativeVec3ExactlyEqual(state.nudgeOffset, {});
  if (!changed) {
    return false;
  }
  state.anchorPolicy = CreativeEditorTransformAnchorPolicy::FixedTarget;
  state.aimTargetPositionable = true;
  state.aimTargetAnchor = targetAnchor;
  state.nudgeOffset = {};
  state.lastNudge = {};
  state.lastCommit = {};
  detail::refreshResolvedTarget(appState, state);
  return true;
}

bool setCreativeEditorTransformRotationDegrees(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeAxis3 axis, double degrees) {
  if (!state.active || !transformRotationAvailable(state) ||
      !cr::isValidCreativeAxis3(axis) ||
      !std::isfinite(degrees) ||
      (detail::worldLayoutBuildingRoute(state) && axis != cr::CreativeAxis3::Y)) {
    return false;
  }
  const double normalized = std::remainder(degrees, 360.0);
  if (state.preflight.capabilities.rotation ==
          cr::CreativeObjectRotationSupport::QuarterTurns &&
      !detail::quarterTurnDegrees(normalized)) {
    return false;
  }
  if (state.rotationAxis == axis && state.rotationDegrees == normalized) {
    return false;
  }
  state.transformMode = CreativeEditorTransformMode::Rotate;
  state.rotationAxis = axis;
  state.rotationDegrees = normalized;
  const double quarterTurns = normalized / 90.0;
  state.rotationQuarterSteps =
      std::abs(quarterTurns - std::round(quarterTurns)) <= 1.0e-12
          ? static_cast<std::int8_t>(
                static_cast<std::int32_t>(std::llround(quarterTurns)) % 4)
          : 0;
  syncRotation(state);
  state.lastCommit = {};
  detail::refreshTransformPlan(appState, state);
  return true;
}

bool setCreativeEditorTransformScaleFactor(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeVec3 scaleFactor) {
  if (!state.active || !transformScaleAvailable(state) ||
      !cr::isPositiveCreativeVec3(scaleFactor)) {
    return false;
  }
  if (cr::creativeVec3ExactlyEqual(state.request.scaleFactor, scaleFactor)) {
    return false;
  }
  if (state.preflight.capabilities.scale ==
          cr::CreativeObjectScaleSupport::Uniform &&
      (std::abs(scaleFactor.x - scaleFactor.y) > 1.0e-12 ||
       std::abs(scaleFactor.x - scaleFactor.z) > 1.0e-12)) {
    return false;
  }
  state.transformMode = CreativeEditorTransformMode::Scale;
  state.request.scaleFactor = scaleFactor;
  const auto nearestIndex = [](double value) {
    std::size_t best = 0U;
    double distance = std::abs(kCreativeEditorScaleFactors.front() - value);
    for (std::size_t index = 1U;
         index < kCreativeEditorScaleFactors.size(); ++index) {
      const double candidate =
          std::abs(kCreativeEditorScaleFactors[index] - value);
      if (candidate < distance) {
        best = index;
        distance = candidate;
      }
    }
    return best;
  };
  state.scaleFactorIndices = {
      nearestIndex(scaleFactor.x), nearestIndex(scaleFactor.y),
      nearestIndex(scaleFactor.z)};
  state.lastCommit = {};
  detail::refreshTransformPlan(appState, state);
  return true;
}

bool setCreativeEditorTransformPlacementMode(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeSelectionPlacementMode mode) {
  if (!state.active ||
      (mode != cr::CreativeSelectionPlacementMode::Move &&
       mode != cr::CreativeSelectionPlacementMode::Copy) ||
      state.mode == mode ||
      (mode == cr::CreativeSelectionPlacementMode::Move &&
       !state.moveAvailable) ||
      (mode == cr::CreativeSelectionPlacementMode::Copy &&
       (detail::patternRecipeRoute(state) || detail::terrainOperationRoute(state)))) {
    return false;
  }
  state.mode = mode;
  state.request.mode = mode;
  state.lastCommit = {};
  detail::refreshTransformPlan(appState, state);
  return true;
}

bool setCreativeEditorTransformPivot(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    CreativeEditorTransformPivot pivot) {
  if (!state.active || pivot == state.pivot ||
      pivot == CreativeEditorTransformPivot::Count ||
      (detail::planarSourceRoute(state) &&
       pivot != CreativeEditorTransformPivot::SelectionAnchor)) {
    return false;
  }

  cr::CreativeVec3 sourceAnchor{};
  cr::CreativeSelectionPlacementPivotMode pivotMode{};
  if (!resolveTransformPivot(state, pivot, sourceAnchor, pivotMode)) {
    return false;
  }

  state.pivot = pivot;
  state.request.sourceAnchor = sourceAnchor;
  state.request.pivotMode = pivotMode;
  if (state.anchorPolicy == CreativeEditorTransformAnchorPolicy::FixedSource) {
    state.aimTargetPositionable = true;
    state.aimTargetAnchor = sourceAnchor;
  }
  state.nudgeOffset = {};
  state.lastNudge = {};
  state.lastCommit = {};
  detail::refreshResolvedTarget(appState, state);
  return true;
}

bool setCreativeEditorTransformCoordinateSpace(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeSelectionPlacementCoordinateSpace coordinateSpace) {
  if (!state.active || state.request.coordinateSpace == coordinateSpace) {
    return false;
  }
  if (detail::planarSourceRoute(state) &&
      coordinateSpace !=
          cr::CreativeSelectionPlacementCoordinateSpace::World) {
    return false;
  }
  cr::CreativeVec3 basisEulerRadians{};
  if (!resolveTransformCoordinateBasis(state, coordinateSpace,
                                       basisEulerRadians)) {
    return false;
  }
  state.request.coordinateSpace = coordinateSpace;
  state.request.coordinateBasisEulerRadians = basisEulerRadians;
  state.nudgeOffset = {};
  state.lastNudge = {};
  state.lastCommit = {};
  detail::refreshResolvedTarget(appState, state);
  return true;
}

bool resumeCreativeEditorTransformAim(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state) {
  if (!state.active ||
      state.anchorPolicy == CreativeEditorTransformAnchorPolicy::FollowAim) {
    return false;
  }
  state.anchorPolicy = CreativeEditorTransformAnchorPolicy::FollowAim;
  state.lastCommit = {};
  detail::refreshResolvedTarget(appState, state);
  return true;
}


bool nudgeCreativeEditorSelectionTransform(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    std::int32_t steps,
    bool fine) {
  if (!state.active || !detail::transformTranslationAvailable(state)) {
    return false;
  }
  cr::CreativeSelectionPlacementNudgeRequest nudge;
  nudge.offset = state.nudgeOffset;
  nudge.axis = state.constraint;
  nudge.coordinateSpace = state.request.coordinateSpace;
  nudge.coordinateBasisEulerRadians =
      state.request.coordinateBasisEulerRadians;
  nudge.snapStepMeters = state.snapStepMeters;
  nudge.steps = steps;
  nudge.fine = detail::planarSourceRoute(state) ? false : fine;
  state.lastNudge = cr::nudgeCreativeSelectionPlacementOffset(nudge);
  if (!state.lastNudge.changed) {
    return false;
  }
  state.nudgeOffset = state.lastNudge.offset;
  state.lastCommit = {};
  detail::refreshResolvedTarget(appState, state);
  return true;
}

bool cycleCreativeEditorTransformMode(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state) {
  if (!state.active) {
    return false;
  }
  const CreativeEditorTransformMode before = state.transformMode;
  constexpr std::array modes{
      CreativeEditorTransformMode::Move,
      CreativeEditorTransformMode::Rotate,
      CreativeEditorTransformMode::Scale};
  const auto found = std::find(modes.begin(), modes.end(), before);
  const std::size_t current =
      found == modes.end() ? 0U : static_cast<std::size_t>(found - modes.begin());
  for (std::size_t offset = 1U; offset <= modes.size(); ++offset) {
    const CreativeEditorTransformMode candidate =
        modes[(current + offset) % modes.size()];
    if (transformModeAvailable(state, candidate)) {
      state.transformMode = candidate;
      break;
    }
  }
  if (state.transformMode == CreativeEditorTransformMode::Rotate) {
    syncRotation(state);
  }
  if (state.transformMode == before) {
    return false;
  }
  state.lastCommit = {};
  detail::refreshTransformPlan(appState, state);
  return true;
}

bool adjustCreativeEditorTransformSetting(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    std::int32_t direction) {
  if (!state.active || direction == 0) {
    return false;
  }
  switch (state.transformMode) {
    case CreativeEditorTransformMode::Move: {
      if (!detail::transformTranslationAvailable(state)) {
        return false;
      }
      const cr::CreativeSelectionPlacementAxis next =
          detail::planarSourceRoute(state)
              ? stepPlanarConstraint(state.constraint, direction)
              : stepConstraint(state.constraint, direction);
      if (next == state.constraint) {
        return false;
      }
      state.constraint = next;
      state.lastNudge = {};
      state.lastCommit = {};
      detail::refreshResolvedTarget(appState, state);
      return true;
    }
    case CreativeEditorTransformMode::Rotate: {
      if (!transformRotationAvailable(state)) {
        return false;
      }
      const bool changed = adjustRotation(state, direction);
      if (!changed) {
        return false;
      }
      state.lastCommit = {};
      detail::refreshTransformPlan(appState, state);
      return true;
    }
    case CreativeEditorTransformMode::Scale: {
      if (!transformScaleAvailable(state)) {
        return false;
      }
      if (!adjustScaleFactor(state, direction)) {
        return false;
      }
      state.lastCommit = {};
      detail::refreshTransformPlan(appState, state);
      return true;
    }
    case CreativeEditorTransformMode::Count:
      return false;
  }
  return false;
}

bool applyCreativeEditorTransformControl(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    CreativeEditorTransformControl control) {
  if (!state.active || control == CreativeEditorTransformControl::Count) {
    return false;
  }
  bool changed = true;
  bool targetChanged = false;
  switch (control) {
    case CreativeEditorTransformControl::RotatePositive:
      if (!transformRotationAvailable(state)) return false;
      state.transformMode = CreativeEditorTransformMode::Rotate;
      changed = adjustRotation(state, 1);
      break;
    case CreativeEditorTransformControl::MirrorX:
      if (!state.preflight.capabilities.mirror) return false;
      state.transformMode = CreativeEditorTransformMode::Rotate;
      state.request.mirrorX = !state.request.mirrorX;
      break;
    case CreativeEditorTransformControl::CycleConstraint:
      if (state.transformMode == CreativeEditorTransformMode::Rotate) {
        if (detail::planarSourceRoute(state)) {
          return false;
        }
        state.rotationAxis = nextRotationAxis(state.rotationAxis);
        syncRotation(state);
      } else {
        state.constraint =
            detail::planarSourceRoute(state)
                ? stepPlanarConstraint(state.constraint, 1)
                : nextConstraint(state.constraint);
      }
      state.lastNudge = {};
      targetChanged = state.transformMode == CreativeEditorTransformMode::Move;
      break;
    case CreativeEditorTransformControl::ToggleMode:
      if (detail::patternRecipeRoute(state) || detail::terrainOperationRoute(state)) {
        return false;
      }
      if (state.mode == cr::CreativeSelectionPlacementMode::Move) {
        state.mode = cr::CreativeSelectionPlacementMode::Copy;
        resetScaleFactor(state);
        if (state.transformMode == CreativeEditorTransformMode::Scale) {
          state.transformMode = CreativeEditorTransformMode::Move;
        }
      } else if (state.moveAvailable) {
        state.mode = cr::CreativeSelectionPlacementMode::Move;
      } else {
        changed = false;
      }
      break;
    case CreativeEditorTransformControl::Confirm:
      changed = requestCreativeEditorSelectionTransformCommit(state);
      state.controlsOpen = false;
      break;
    case CreativeEditorTransformControl::Cancel:
      return cancelCreativeEditorSelectionTransformPreview(
          state, "transform_control_cancel");
    case CreativeEditorTransformControl::MirrorZ:
      if (!state.preflight.capabilities.mirror) return false;
      state.transformMode = CreativeEditorTransformMode::Rotate;
      state.request.mirrorZ = !state.request.mirrorZ;
      break;
    case CreativeEditorTransformControl::RotateNegative:
      if (!transformRotationAvailable(state)) return false;
      state.transformMode = CreativeEditorTransformMode::Rotate;
      changed = adjustRotation(state, -1);
      break;
    case CreativeEditorTransformControl::Reset:
      changed = state.rotationQuarterSteps != 0 || state.request.mirrorX ||
                state.request.mirrorZ ||
                state.constraint != cr::CreativeSelectionPlacementAxis::Free ||
                state.rotationAxis != cr::CreativeAxis3::Y ||
                !cr::creativeVec3ExactlyEqual(state.nudgeOffset, {}) ||
                std::any_of(state.scaleFactorIndices.begin(),
                            state.scaleFactorIndices.end(),
                            [](std::size_t index) {
                              return index != kCreativeEditorDefaultScaleIndex;
                            }) ||
                state.transformMode != CreativeEditorTransformMode::Move;
      state.rotationQuarterSteps = 0;
      state.rotationDegrees = 0.0;
      state.constraint = cr::CreativeSelectionPlacementAxis::Free;
      state.rotationAxis = cr::CreativeAxis3::Y;
      syncRotation(state);
      state.request.mirrorX = false;
      state.request.mirrorZ = false;
      resetScaleFactor(state);
      state.transformMode = CreativeEditorTransformMode::Move;
      state.nudgeOffset = {};
      state.lastNudge = {};
      targetChanged = true;
      break;
    case CreativeEditorTransformControl::Count:
      return false;
  }
  if (changed && state.active) {
    state.lastCommit = {};
    if (targetChanged) {
      detail::refreshResolvedTarget(appState, state);
    } else {
      detail::refreshTransformPlan(appState, state);
    }
  }
  return changed;
}

}  // namespace iggy3d_creative_app
