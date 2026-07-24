#include "EditorTransform.hpp"
#include "EditorTransformInternal.hpp"

#include <SDL3/SDL_log.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <span>
#include <string>
#include <utility>

#include "EditorEdits.hpp"
#include "EditorWorldLayout.hpp"
#include "EditorWorldLayoutHistory.hpp"
#include "EditorWorldLayoutInternal.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/input/UiInput.hpp"

namespace iggy3d_creative_app {
namespace detail {

[[nodiscard]] cr::CreativeVec3 subtract(cr::CreativeVec3 lhs,
                                        cr::CreativeVec3 rhs) noexcept {
  return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

}  // namespace detail

namespace {

[[nodiscard]] cr::CreativeVec3 add(cr::CreativeVec3 lhs,
                                   cr::CreativeVec3 rhs) noexcept {
  return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

[[nodiscard]] cr::CreativeVec3 scale(cr::CreativeVec3 value,
                                     double factor) noexcept {
  return {value.x * factor, value.y * factor, value.z * factor};
}

[[nodiscard]] double dot(cr::CreativeVec3 lhs,
                         cr::CreativeVec3 rhs) noexcept {
  return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

[[nodiscard]] bool normalized(cr::CreativeVec3 value,
                              cr::CreativeVec3& output) noexcept {
  const double lengthSquared = dot(value, value);
  if (!std::isfinite(lengthSquared) || lengthSquared <= 1.0e-16) {
    return false;
  }
  output = scale(value, 1.0 / std::sqrt(lengthSquared));
  return cr::isFiniteCreativeVec3(output);
}


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

[[nodiscard]] bool transformTranslationAvailable(
    const CreativeEditorSelectionTransformState& state) noexcept {
  return state.preflight.capabilities.translate;
}

[[nodiscard]] bool transformModeAvailable(
    const CreativeEditorSelectionTransformState& state,
    CreativeEditorTransformMode mode) noexcept {
  switch (mode) {
    case CreativeEditorTransformMode::Move:
      return transformTranslationAvailable(state);
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

[[nodiscard]] cr::CreativeSelectionPlacementReceipt placeObjectsWithHistory(
    cr::CreativeAppState& appState,
    std::span<const cr::CreativeObjectId> objectIds,
    const cr::CreativeSelectionPlacementRequest& request,
    std::string_view source) {
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  cr::CreativeSelectionPlacementReceipt receipt =
      appState.facade.placeObjects(objectIds, request);
  static_cast<void>(completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      receipt.accepted && receipt.changed, receipt.reasonCode));
  SDL_Log("iggy3d_creative: TRANSFORM move source='%s' accepted=%d changed=%d "
          "status='%s' objects=%llu reasonCode='%s'",
          std::string(source).c_str(), receipt.accepted ? 1 : 0,
          receipt.changed ? 1 : 0,
          std::string(cr::toString(receipt.status)).c_str(),
          static_cast<unsigned long long>(receipt.objectCount),
          receipt.reasonCode.c_str());
  return receipt;
}

[[nodiscard]] cr::CreativeClipboardPasteRequest clipboardPasteRequest(
    const CreativeEditorSelectionTransformState& state) noexcept {
  cr::CreativeClipboardPasteRequest request;
  request.offset = detail::subtract(state.request.targetAnchor,
                            state.request.sourceAnchor);
  request.hasTransformAnchor = true;
  request.transformAnchor = state.request.sourceAnchor;
  request.pivotMode = state.request.pivotMode;
  request.coordinateSpace = state.request.coordinateSpace;
  request.coordinateBasisEulerRadians =
      state.request.coordinateBasisEulerRadians;
  request.scaleFactor = state.request.scaleFactor;
  request.quarterTurns = state.request.quarterTurns;
  request.mirrorX = state.request.mirrorX;
  request.mirrorZ = state.request.mirrorZ;
  request.hasAxisAngleRotation = state.request.hasAxisAngleRotation;
  request.rotationAxis = state.request.rotationAxis;
  request.rotationRadians = state.request.rotationRadians;
  return request;
}

[[nodiscard]] CreativeEditorTransformCommitReceipt
commitPatternRecipeTransform(
    cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    std::string_view source) {
  CreativeEditorTransformCommitReceipt receipt;
  receipt.requested = true;
  receipt.mode = state.mode;
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  receipt.patternReceipt = appState.facade.applyPatternRecipeTranslation(
      state.candidatePatternTranslation);
  static_cast<void>(completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      receipt.patternReceipt.accepted && receipt.patternReceipt.changed,
      receipt.patternReceipt.reasonCode));
  receipt.accepted = receipt.patternReceipt.accepted;
  receipt.changed = receipt.patternReceipt.changed;
  receipt.reasonCode = std::string{receipt.patternReceipt.reasonCode};
  return receipt;
}

[[nodiscard]] CreativeEditorTransformCommitReceipt
commitTerrainOperationTransform(
    cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    std::string_view source) {
  CreativeEditorTransformCommitReceipt receipt;
  receipt.requested = true;
  receipt.mode = state.mode;
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  receipt.terrainReceipt = appState.facade.applyTerrainOperationTranslation(
      state.candidateTerrainTranslation);
  static_cast<void>(completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      receipt.terrainReceipt.accepted && receipt.terrainReceipt.changed,
      receipt.terrainReceipt.reasonCode));
  receipt.accepted = receipt.terrainReceipt.accepted;
  receipt.changed = receipt.terrainReceipt.changed;
  receipt.reasonCode = std::string{receipt.terrainReceipt.reasonCode};
  return receipt;
}

[[nodiscard]] CreativeEditorTransformCommitReceipt
commitWorldLayoutBuildingTransform(
    cr::CreativeAppState& appState,
    CreativeEditorWorldLayoutState* worldLayout,
    CreativeEditorSelectionTransformState& state,
    std::string_view source) {
  CreativeEditorTransformCommitReceipt receipt;
  receipt.requested = true;
  receipt.mode = state.mode;
  if (worldLayout == nullptr || !state.candidateWorldLayoutReady ||
      state.preflight.worldLayoutSource.table !=
          cr::CreativeWorldLayoutTable::Building) {
    receipt.reasonCode =
        "editor_transform_world_layout_commit_owner_missing";
    return receipt;
  }
  if (worldLayout->revision != state.preflight.worldLayoutRevision ||
      worldLayout->sourceEpoch != state.preflight.worldLayoutSourceEpoch ||
      worldLayout->generatedRevision != worldLayout->revision) {
    receipt.reasonCode = "editor_transform_world_layout_commit_stale";
    return receipt;
  }
  if (!state.candidateWorldLayoutChanged) {
    receipt.accepted = true;
    receipt.reasonCode = "editor_transform_world_layout_no_change";
    return receipt;
  }
  if (worldLayout->revision ==
      std::numeric_limits<std::uint64_t>::max()) {
    receipt.reasonCode =
        "editor_transform_world_layout_revision_exhausted";
    return receipt;
  }

  CreativeEditorWorldLayoutSourceHistoryEntry sourceOnlyUndo =
      detail::captureWorldLayoutSourceHistoryEntry(*worldLayout);
  sourceOnlyUndo.source = std::string(source);
  CreativeEditorWorldLayoutSnapshot committed =
      captureCreativeEditorWorldLayoutSnapshot(*worldLayout);
  committed.source = state.candidateWorldLayout;
  committed.nextStableOrdinal =
      state.candidateWorldLayoutNextStableOrdinal;
  ++committed.revision;
  receipt.worldLayoutReceipt = applyCreativeEditorWorldLayoutPlanWithHistory(
      *worldLayout, appState, state.candidateWorldLayoutPlan,
      std::move(committed), source);
  receipt.accepted = receipt.worldLayoutReceipt.accepted;
  receipt.changed = receipt.accepted;
  receipt.reasonCode = receipt.worldLayoutReceipt.reasonCode;
  if (!receipt.accepted) {
    worldLayout->statusMessage = receipt.reasonCode;
    return receipt;
  }

  worldLayout->selection = {
      CreativeEditorWorldLayoutSelectionKind::Building,
      state.candidateWorldLayoutBuildingIndex};
  worldLayout->activeLevelIndex = cr::kInvalidCreativeWorldLayoutIndex;
  repairCreativeEditorWorldLayoutActiveLevel(
      *worldLayout, state.candidateWorldLayoutBuildingIndex);
  if (!receipt.worldLayoutReceipt.changed) {
    detail::appendWorldLayoutSourceHistoryEntry(
        worldLayout->sourceHistory.undoEntries, std::move(sourceOnlyUndo),
        worldLayout->sourceHistory.maxDepth);
    worldLayout->sourceHistory.redoEntries.clear();
  }
  worldLayout->statusMessage =
      state.mode == cr::CreativeSelectionPlacementMode::Copy
          ? "building duplicated in 3D"
          : "building transformed in 3D";
  receipt.reasonCode =
      state.mode == cr::CreativeSelectionPlacementMode::Copy
          ? "editor_transform_world_layout_building_duplicated"
          : "editor_transform_world_layout_building_applied";
  return receipt;
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

bool requestCreativeEditorSelectionTransformCommit(
    CreativeEditorSelectionTransformState& state) noexcept {
  if (!state.active || state.commitRequested) {
    return false;
  }
  state.commitRequested = true;
  return true;
}

bool cancelCreativeEditorSelectionTransformPreview(
    CreativeEditorSelectionTransformState& state,
    std::string_view source) {
  if (!state.active) {
    return false;
  }
  SDL_Log("iggy3d_creative: TRANSFORM preview cancelled source='%s'",
          std::string(source).c_str());
  state = {};
  return true;
}

bool setCreativeEditorTransformConstraint(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeSelectionPlacementAxis constraint) {
  if (!state.active || !transformTranslationAvailable(state) ||
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
  if (!state.active || !transformTranslationAvailable(state) ||
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

CreativeEditorTransformAxisRaySample sampleCreativeEditorTransformAxisRay(
    cr::CreativeVec3 rayOrigin,
    cr::CreativeVec3 rayDirection,
    cr::CreativeVec3 axisOrigin,
    cr::CreativeVec3 axisDirection) noexcept {
  CreativeEditorTransformAxisRaySample result;
  if (!cr::isFiniteCreativeVec3(rayOrigin) ||
      !cr::isFiniteCreativeVec3(axisOrigin)) {
    return result;
  }
  cr::CreativeVec3 ray{};
  cr::CreativeVec3 axis{};
  if (!normalized(rayDirection, ray) || !normalized(axisDirection, axis)) {
    return result;
  }

  const cr::CreativeVec3 fromAxis = detail::subtract(rayOrigin, axisOrigin);
  const double rayAxisDot = dot(ray, axis);
  const double rayFromAxisDot = dot(ray, fromAxis);
  const double axisFromAxisDot = dot(axis, fromAxis);
  const double denominator = 1.0 - rayAxisDot * rayAxisDot;
  if (!std::isfinite(denominator) || denominator <= 1.0e-8) {
    return result;
  }
  result.rayParameter =
      (rayAxisDot * axisFromAxisDot - rayFromAxisDot) / denominator;
  result.axisParameter =
      (axisFromAxisDot - rayAxisDot * rayFromAxisDot) / denominator;
  result.valid = std::isfinite(result.rayParameter) &&
                 std::isfinite(result.axisParameter) &&
                 result.rayParameter >= 0.0;
  if (!result.valid) {
    result = {};
  }
  return result;
}

bool beginCreativeEditorFreeTransformPointerGesture(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeVec3 initialAimAnchor) {
  if (!state.active || !transformTranslationAvailable(state) ||
      !cr::isFiniteCreativeVec3(initialAimAnchor)) {
    return false;
  }
  if (state.constraint != cr::CreativeSelectionPlacementAxis::Free) {
    static_cast<void>(setCreativeEditorTransformConstraint(
        appState, state, cr::CreativeSelectionPlacementAxis::Free));
  }
  state.pointerGesture = {};
  state.pointerGesture.kind = CreativeEditorTransformPointerGestureKind::Free;
  state.pointerGesture.initialAimAnchor = initialAimAnchor;
  return true;
}

bool beginCreativeEditorAxisTransformPointerGesture(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeSelectionPlacementAxis axis,
    cr::CreativeVec3 axisOrigin,
    cr::CreativeVec3 axisDirection,
    cr::CreativeVec3 rayOrigin,
    cr::CreativeVec3 rayDirection) {
  if (!state.active || !transformTranslationAvailable(state) ||
      axis == cr::CreativeSelectionPlacementAxis::Free ||
      axis == cr::CreativeSelectionPlacementAxis::Count) {
    return false;
  }
  cr::CreativeVec3 normalizedAxis{};
  if (!normalized(axisDirection, normalizedAxis)) {
    return false;
  }
  const CreativeEditorTransformAxisRaySample sample =
      sampleCreativeEditorTransformAxisRay(
          rayOrigin, rayDirection, axisOrigin, normalizedAxis);
  if (!sample.valid) {
    return false;
  }
  if (state.constraint != axis) {
    static_cast<void>(setCreativeEditorTransformConstraint(appState, state,
                                                           axis));
  }
  if (state.constraint != axis) {
    return false;
  }
  state.pointerGesture = {};
  state.pointerGesture.kind = CreativeEditorTransformPointerGestureKind::Axis;
  state.pointerGesture.axis = axis;
  state.pointerGesture.axisOrigin = axisOrigin;
  state.pointerGesture.axisDirection = normalizedAxis;
  state.pointerGesture.initialAxisParameter = sample.axisParameter;
  return true;
}

bool updateCreativeEditorTransformPointerGesture(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    bool targetPositionable,
    cr::CreativeVec3 targetAnchor,
    cr::CreativeVec3 rayOrigin,
    cr::CreativeVec3 rayDirection) {
  if (!state.active ||
      state.pointerGesture.kind ==
          CreativeEditorTransformPointerGestureKind::None) {
    return false;
  }

  cr::CreativeVec3 resolvedTarget{};
  if (state.pointerGesture.kind ==
      CreativeEditorTransformPointerGestureKind::Free) {
    if (!targetPositionable || !cr::isFiniteCreativeVec3(targetAnchor)) {
      return false;
    }
    resolvedTarget = add(
        state.request.sourceAnchor,
        detail::subtract(targetAnchor, state.pointerGesture.initialAimAnchor));
  } else {
    const CreativeEditorTransformAxisRaySample sample =
        sampleCreativeEditorTransformAxisRay(
            rayOrigin, rayDirection, state.pointerGesture.axisOrigin,
            state.pointerGesture.axisDirection);
    if (!sample.valid) {
      return false;
    }
    resolvedTarget = add(
        state.request.sourceAnchor,
        scale(state.pointerGesture.axisDirection,
              sample.axisParameter -
                  state.pointerGesture.initialAxisParameter));
  }
  if (!cr::isFiniteCreativeVec3(resolvedTarget)) {
    return false;
  }
  static_cast<void>(
      setCreativeEditorTransformTargetAnchor(appState, state, resolvedTarget));
  state.pointerGesture.changed =
      state.pointerGesture.changed ||
      !cr::creativeVec3ExactlyEqual(state.request.targetAnchor,
                                    state.request.sourceAnchor);
  return true;
}

bool finishCreativeEditorTransformPointerGesture(
    CreativeEditorSelectionTransformState& state,
    std::string_view source) {
  if (!state.active ||
      state.pointerGesture.kind ==
          CreativeEditorTransformPointerGestureKind::None) {
    return false;
  }
  const bool changed =
      state.pointerGesture.changed && state.targetPositionable &&
      state.plan.accepted &&
      !cr::creativeVec3ExactlyEqual(state.request.targetAnchor,
                                    state.request.sourceAnchor);
  state.pointerGesture = {};
  if (!changed) {
    return cancelCreativeEditorSelectionTransformPreview(state, source);
  }
  return requestCreativeEditorSelectionTransformCommit(state);
}

bool nudgeCreativeEditorSelectionTransform(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    std::int32_t steps,
    bool fine) {
  if (!state.active || !transformTranslationAvailable(state)) {
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
      if (!transformTranslationAvailable(state)) {
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

CreativeEditorTransformCommitReceipt
processCreativeEditorSelectionTransformPreview(
    cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    bool targetPositionable,
    cr::CreativeVec3 targetAnchor,
    bool secondaryPressed,
    std::string_view source,
    double snapStepMeters,
    CreativeEditorWorldLayoutState* worldLayout,
    const CreativePlacementClearanceCache* clearanceCache) {
  if (!state.active) {
    return {};
  }

  const bool previousPositionable = state.targetPositionable;
  const cr::CreativeVec3 previousTarget = state.request.targetAnchor;
  if (state.anchorPolicy == CreativeEditorTransformAnchorPolicy::FixedSource) {
    state.aimTargetPositionable = true;
    state.aimTargetAnchor = state.request.sourceAnchor;
  } else if (state.anchorPolicy ==
             CreativeEditorTransformAnchorPolicy::FollowAim) {
    state.aimTargetPositionable =
        targetPositionable && cr::isFiniteCreativeVec3(targetAnchor);
    if (state.aimTargetPositionable) {
      state.aimTargetAnchor = targetAnchor;
    }
  }
  state.snapStepMeters = snapStepMeters;
  if (detail::planarSourceRoute(state)) {
    const cr::CreativeGridSettings grid =
        appState.facade.document().gridSettings();
    if (std::isfinite(grid.cellSizeMeters) && grid.cellSizeMeters > 0.0) {
      state.snapStepMeters = grid.cellSizeMeters;
    }
  }
  detail::refreshResolvedTarget(appState, state);
  detail::refreshTransformClearance(appState, state, clearanceCache);
  const bool targetChanged =
      previousPositionable != state.targetPositionable ||
      (state.targetPositionable &&
       !cr::creativeVec3ExactlyEqual(previousTarget,
                                     state.request.targetAnchor));
  if (targetChanged) {
    state.lastCommit = {};
  }
  if (secondaryPressed) {
    static_cast<void>(requestCreativeEditorSelectionTransformCommit(state));
  }
  if (!state.commitRequested) {
    return {};
  }
  state.commitRequested = false;

  CreativeEditorTransformCommitReceipt receipt;
  receipt.requested = true;
  receipt.mode = state.mode;
  if (!state.targetPositionable || !state.plan.accepted) {
    receipt.reasonCode = state.targetPositionable
                             ? state.plan.reasonCode
                             : "editor_transform_target_unavailable";
    state.lastCommit = receipt;
    return receipt;
  }

  if (detail::worldLayoutBuildingRoute(state)) {
    receipt = commitWorldLayoutBuildingTransform(
        appState, worldLayout, state, source);
  } else if (detail::patternRecipeRoute(state)) {
    receipt = commitPatternRecipeTransform(appState, state, source);
  } else if (detail::terrainOperationRoute(state)) {
    receipt = commitTerrainOperationTransform(appState, state, source);
  } else if (state.mode == cr::CreativeSelectionPlacementMode::Copy) {
    receipt.copyReceipt = pasteClipboardWithHistory(
        appState, state.sourceClipboard, clipboardPasteRequest(state), source);
    receipt.accepted = receipt.copyReceipt.accepted;
    receipt.changed = receipt.copyReceipt.changed;
    receipt.reasonCode = receipt.copyReceipt.reasonCode;
  } else {
    receipt.moveReceipt =
        placeObjectsWithHistory(appState, state.sourceObjectIds, state.request,
                                source);
    receipt.accepted = receipt.moveReceipt.accepted;
    receipt.changed = receipt.moveReceipt.changed;
    receipt.reasonCode = receipt.moveReceipt.reasonCode;
  }
  state.lastCommit = receipt;
  if (receipt.accepted && receipt.changed) {
    state.active = false;
    state.controlsOpen = false;
    state.targetPositionable = false;
  }
  return receipt;
}

}  // namespace iggy3d_creative_app
