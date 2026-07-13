#include "EditorTransform.hpp"

#include <SDL3/SDL_log.h>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <string>
#include <utility>

#include "EditorEdits.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/input/UiInput.hpp"

namespace iggy3d_creative_app {
namespace {

[[nodiscard]] cr::CreativeVec3 subtract(cr::CreativeVec3 lhs,
                                        cr::CreativeVec3 rhs) noexcept {
  return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

[[nodiscard]] std::vector<cr::CreativeObjectId> sourceObjectIds(
    const CreativeEditorSelectionTransformState& state) {
  std::vector<cr::CreativeObjectId> ids;
  ids.reserve(state.sourceClipboard.objects.size());
  for (const cr::CreativeObject& object : state.sourceClipboard.objects) {
    ids.push_back(object.id);
  }
  return ids;
}

[[nodiscard]] bool moveSourceAvailable(
    const cr::CreativeAppState& appState,
    const cr::CreativeClipboard& clipboard) noexcept {
  const cr::CreativeDocument& document = appState.facade.document();
  if (clipboard.sourceDocumentId != document.id() ||
      clipboard.sourceRevision != document.revision() ||
      cr::creativeClipboardEmpty(clipboard)) {
    return false;
  }
  return std::all_of(
      clipboard.objects.begin(), clipboard.objects.end(),
      [&document](const cr::CreativeObject& object) {
        return document.containsObject(object.id);
      });
}

void refreshTransformPlan(const cr::CreativeAppState& appState,
                          CreativeEditorSelectionTransformState& state) {
  if (!state.active || !state.targetPositionable) {
    state.plan = {};
    return;
  }
  state.request.mode = state.mode;
  state.request.sourceAnchor = state.sourceClipboard.placementAnchor;
  state.plan = cr::planCreativeSelectionPlacement(
      state.sourceClipboard.objects, state.request);
  state.moveAvailable = moveSourceAvailable(appState, state.sourceClipboard);
  if (state.mode == cr::CreativeSelectionPlacementMode::Move &&
      !state.moveAvailable) {
    state.plan.accepted = false;
    state.plan.status = cr::CreativeSelectionPlacementStatus::InvalidSource;
    state.plan.reasonCode = "editor_transform_move_source_stale";
  }
}

void refreshResolvedTarget(const cr::CreativeAppState& appState,
                           CreativeEditorSelectionTransformState& state) {
  state.targetResolution = {};
  state.targetPositionable = false;
  if (!state.active || !state.aimTargetPositionable) {
    refreshTransformPlan(appState, state);
    return;
  }
  state.targetResolution = cr::resolveCreativeSelectionPlacementTarget(
      {state.request.sourceAnchor, state.aimTargetAnchor, state.nudgeOffset,
       state.constraint, state.snapStepMeters});
  if (state.targetResolution.accepted) {
    state.targetPositionable = true;
    state.request.targetAnchor = state.targetResolution.targetAnchor;
  }
  refreshTransformPlan(appState, state);
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

[[nodiscard]] bool transformScaleAvailable(
    const CreativeEditorSelectionTransformState& state) noexcept {
  return state.source == CreativeEditorTransformSource::Selection &&
         state.mode == cr::CreativeSelectionPlacementMode::Move;
}

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

[[nodiscard]] cr::CreativeAxis3 rotationAxis(
    cr::CreativeSelectionPlacementAxis axis) noexcept {
  switch (axis) {
    case cr::CreativeSelectionPlacementAxis::X: return cr::CreativeAxis3::X;
    case cr::CreativeSelectionPlacementAxis::Z: return cr::CreativeAxis3::Z;
    case cr::CreativeSelectionPlacementAxis::Free:
    case cr::CreativeSelectionPlacementAxis::Y:
    case cr::CreativeSelectionPlacementAxis::Count:
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
  state.request.rotationAxis = rotationAxis(state.constraint);
  state.request.rotationRadians =
      static_cast<double>(state.rotationQuarterSteps) * std::numbers::pi * 0.5;
  state.request.hasAxisAngleRotation = state.rotationQuarterSteps != 0;
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
  syncRotation(state);
  return state.rotationQuarterSteps != before;
}

[[nodiscard]] bool beginTransformPreview(
    const cr::CreativeAppState& appState,
    const cr::CreativeClipboard& clipboard,
    CreativeEditorTransformSource transformSource,
    cr::CreativeSelectionPlacementMode mode,
    CreativeEditorTransformAnchorPolicy anchorPolicy,
    CreativeEditorSelectionTransformState& state,
    std::string_view source) {
  state = {};
  if (cr::creativeClipboardEmpty(clipboard) || !clipboard.hasPlacementAnchor ||
      !cr::isFiniteCreativeVec3(clipboard.placementAnchor)) {
    SDL_Log("iggy3d_creative: TRANSFORM preview rejected source='%s' "
            "objectCount=%zu hasAnchor=%d",
            std::string(source).c_str(), clipboard.objects.size(),
            clipboard.hasPlacementAnchor ? 1 : 0);
    return false;
  }
  state.active = true;
  state.source = transformSource;
  state.anchorPolicy = anchorPolicy;
  state.mode = mode;
  state.sourceClipboard = clipboard;
  state.request.mode = mode;
  state.request.sourceAnchor = clipboard.placementAnchor;
  state.moveAvailable = moveSourceAvailable(appState, clipboard);
  if (mode == cr::CreativeSelectionPlacementMode::Move &&
      !state.moveAvailable) {
    state = {};
    return false;
  }
  if (anchorPolicy == CreativeEditorTransformAnchorPolicy::FixedSource) {
    state.aimTargetPositionable = true;
    state.aimTargetAnchor = clipboard.placementAnchor;
    refreshResolvedTarget(appState, state);
  }
  SDL_Log("iggy3d_creative: TRANSFORM preview begun source='%s' mode='%s' "
          "objectCount=%zu anchor=(%.3f, %.3f, %.3f)",
          std::string(source).c_str(),
          std::string(cr::toString(mode)).c_str(), clipboard.objects.size(),
          clipboard.placementAnchor.x, clipboard.placementAnchor.y,
          clipboard.placementAnchor.z);
  return true;
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
  request.offset = subtract(state.request.targetAnchor,
                            state.request.sourceAnchor);
  request.quarterTurns = state.request.quarterTurns;
  request.mirrorX = state.request.mirrorX;
  request.mirrorZ = state.request.mirrorZ;
  request.hasAxisAngleRotation = state.request.hasAxisAngleRotation;
  request.rotationAxis = state.request.rotationAxis;
  request.rotationRadians = state.request.rotationRadians;
  return request;
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
  return scaleFactorFromIndices(state);
}

bool beginCreativeEditorClipboardTransformPreview(
    const cr::CreativeAppState& appState,
    const cr::CreativeClipboard& clipboard,
    CreativeEditorSelectionTransformState& state,
    std::string_view source) {
  return beginTransformPreview(
      appState, clipboard, CreativeEditorTransformSource::Clipboard,
      cr::CreativeSelectionPlacementMode::Copy,
      CreativeEditorTransformAnchorPolicy::FollowAim, state, source);
}

bool beginCreativeEditorSelectionTransformPreview(
    cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    std::string_view source,
    CreativeEditorTransformAnchorPolicy anchorPolicy) {
  cr::CreativeClipboard selection;
  const cr::CreativeClipboardCopyReceipt copied =
      appState.facade.copySelectedObjectsToClipboard(selection);
  if (!copied.accepted) {
    return false;
  }
  return beginTransformPreview(
      appState, selection, CreativeEditorTransformSource::Selection,
      cr::CreativeSelectionPlacementMode::Move, anchorPolicy, state, source);
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
  if (!state.active ||
      static_cast<std::size_t>(constraint) >=
          static_cast<std::size_t>(
              cr::CreativeSelectionPlacementAxis::Count)) {
    return false;
  }
  const cr::CreativeSelectionPlacementAxis next =
      state.constraint == constraint
          ? cr::CreativeSelectionPlacementAxis::Free
          : constraint;
  if (state.constraint == next) {
    return false;
  }
  state.constraint = next;
  if (state.transformMode == CreativeEditorTransformMode::Rotate) {
    syncRotation(state);
  }
  state.lastCommit = {};
  state.lastNudge = {};
  refreshResolvedTarget(appState, state);
  return true;
}

bool nudgeCreativeEditorSelectionTransform(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    std::int32_t steps,
    bool fine) {
  if (!state.active) {
    return false;
  }
  state.lastNudge = cr::nudgeCreativeSelectionPlacementOffset(
      {state.nudgeOffset, state.constraint, state.snapStepMeters, steps, fine});
  if (!state.lastNudge.changed) {
    return false;
  }
  state.nudgeOffset = state.lastNudge.offset;
  state.lastCommit = {};
  refreshResolvedTarget(appState, state);
  return true;
}

bool cycleCreativeEditorTransformMode(
    const cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state) {
  if (!state.active) {
    return false;
  }
  const CreativeEditorTransformMode before = state.transformMode;
  switch (state.transformMode) {
    case CreativeEditorTransformMode::Move:
      state.transformMode = CreativeEditorTransformMode::Rotate;
      syncRotation(state);
      break;
    case CreativeEditorTransformMode::Rotate:
      state.transformMode = transformScaleAvailable(state)
                                ? CreativeEditorTransformMode::Scale
                                : CreativeEditorTransformMode::Move;
      break;
    case CreativeEditorTransformMode::Scale:
    case CreativeEditorTransformMode::Count:
      state.transformMode = CreativeEditorTransformMode::Move;
      break;
  }
  if (state.transformMode == before) {
    return false;
  }
  state.lastCommit = {};
  refreshTransformPlan(appState, state);
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
      const cr::CreativeSelectionPlacementAxis next =
          stepConstraint(state.constraint, direction);
      if (next == state.constraint) {
        return false;
      }
      state.constraint = next;
      state.lastNudge = {};
      state.lastCommit = {};
      refreshResolvedTarget(appState, state);
      return true;
    }
    case CreativeEditorTransformMode::Rotate: {
      const bool changed = adjustRotation(state, direction);
      if (!changed) {
        return false;
      }
      state.lastCommit = {};
      refreshTransformPlan(appState, state);
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
      refreshTransformPlan(appState, state);
      return true;
    }
    case CreativeEditorTransformMode::Count:
      return false;
  }
  return false;
}

bool applyCreativeEditorTransformControl(
    cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    CreativeEditorTransformControl control) {
  if (!state.active || control == CreativeEditorTransformControl::Count) {
    return false;
  }
  bool changed = true;
  bool targetChanged = false;
  switch (control) {
    case CreativeEditorTransformControl::RotatePositive:
      state.transformMode = CreativeEditorTransformMode::Rotate;
      changed = adjustRotation(state, 1);
      break;
    case CreativeEditorTransformControl::MirrorX:
      state.transformMode = CreativeEditorTransformMode::Rotate;
      state.request.mirrorX = !state.request.mirrorX;
      break;
    case CreativeEditorTransformControl::CycleConstraint:
      state.constraint = nextConstraint(state.constraint);
      if (state.transformMode == CreativeEditorTransformMode::Rotate) {
        syncRotation(state);
      }
      state.lastNudge = {};
      targetChanged = state.transformMode == CreativeEditorTransformMode::Move;
      break;
    case CreativeEditorTransformControl::ToggleMode:
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
      state.transformMode = CreativeEditorTransformMode::Rotate;
      state.request.mirrorZ = !state.request.mirrorZ;
      break;
    case CreativeEditorTransformControl::RotateNegative:
      state.transformMode = CreativeEditorTransformMode::Rotate;
      changed = adjustRotation(state, -1);
      break;
    case CreativeEditorTransformControl::Reset:
      changed = state.rotationQuarterSteps != 0 || state.request.mirrorX ||
                state.request.mirrorZ ||
                state.constraint != cr::CreativeSelectionPlacementAxis::Free ||
                !cr::creativeVec3ExactlyEqual(state.nudgeOffset, {}) ||
                std::any_of(state.scaleFactorIndices.begin(),
                            state.scaleFactorIndices.end(),
                            [](std::size_t index) {
                              return index != kCreativeEditorDefaultScaleIndex;
                            }) ||
                state.transformMode != CreativeEditorTransformMode::Move;
      state.rotationQuarterSteps = 0;
      state.constraint = cr::CreativeSelectionPlacementAxis::Free;
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
      refreshResolvedTarget(appState, state);
    } else {
      refreshTransformPlan(appState, state);
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
    double snapStepMeters) {
  if (!state.active) {
    return {};
  }

  const bool previousPositionable = state.targetPositionable;
  const cr::CreativeVec3 previousTarget = state.request.targetAnchor;
  if (state.anchorPolicy == CreativeEditorTransformAnchorPolicy::FixedSource) {
    state.aimTargetPositionable = true;
    state.aimTargetAnchor = state.sourceClipboard.placementAnchor;
  } else {
    state.aimTargetPositionable =
        targetPositionable && cr::isFiniteCreativeVec3(targetAnchor);
    if (state.aimTargetPositionable) {
      state.aimTargetAnchor = targetAnchor;
    }
  }
  state.snapStepMeters = snapStepMeters;
  refreshResolvedTarget(appState, state);
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

  if (state.mode == cr::CreativeSelectionPlacementMode::Copy) {
    receipt.copyReceipt = pasteClipboardWithHistory(
        appState, state.sourceClipboard, clipboardPasteRequest(state), source);
    receipt.accepted = receipt.copyReceipt.accepted;
    receipt.changed = receipt.copyReceipt.changed;
    receipt.reasonCode = receipt.copyReceipt.reasonCode;
  } else {
    const std::vector<cr::CreativeObjectId> ids = sourceObjectIds(state);
    receipt.moveReceipt =
        placeObjectsWithHistory(appState, ids, state.request, source);
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
