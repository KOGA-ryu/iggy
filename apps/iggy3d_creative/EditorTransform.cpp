#include "EditorTransform.hpp"

#include <SDL3/SDL_log.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

#include "EditorEdits.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/input/UiInput.hpp"
#include "app/platform/SdlWindow.hpp"

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

[[nodiscard]] bool beginTransformPreview(
    const cr::CreativeAppState& appState,
    const cr::CreativeClipboard& clipboard,
    CreativeEditorTransformSource transformSource,
    cr::CreativeSelectionPlacementMode mode,
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
  return request;
}

void moveControlSelection(CreativeEditorSelectionTransformState& state,
                          std::int32_t direction) noexcept {
  if (direction == 0) {
    return;
  }
  const cr::CreativeWrappedIndexResult next = cr::stepCreativeWrappedIndex(
      state.selectedControl, kCreativeEditorTransformControlCount, direction);
  if (next.valid) {
    state.selectedControl = next.index;
  }
}

void selectControlDirection(CreativeEditorSelectionTransformState& state,
                            float x,
                            float y,
                            float deadzone) noexcept {
  const cr::CreativeRadialSectorResult sector = cr::resolveCreativeRadialSector(
      x, y, kCreativeEditorTransformControlCount, deadzone);
  if (sector.valid) {
    state.selectedControl = sector.index;
  }
}

void applyTransformPreviewAction(
    const CreativeEditorTransformFrameRequest& request,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeInputActionId action) {
  switch (action) {
    case cr::CreativeInputActionId::ToggleTransformControls:
      state.controlsOpen = true;
      return;
    case cr::CreativeInputActionId::TransformConstraintX:
      static_cast<void>(setCreativeEditorTransformConstraint(
          request.appState, state, cr::CreativeSelectionPlacementAxis::X));
      return;
    case cr::CreativeInputActionId::TransformConstraintY:
      static_cast<void>(setCreativeEditorTransformConstraint(
          request.appState, state, cr::CreativeSelectionPlacementAxis::Y));
      return;
    case cr::CreativeInputActionId::TransformConstraintZ:
      static_cast<void>(setCreativeEditorTransformConstraint(
          request.appState, state, cr::CreativeSelectionPlacementAxis::Z));
      return;
    case cr::CreativeInputActionId::TransformNudgePositive:
      static_cast<void>(nudgeCreativeEditorSelectionTransform(
          request.appState, state, 1, request.fineNudge));
      return;
    case cr::CreativeInputActionId::TransformNudgeNegative:
      static_cast<void>(nudgeCreativeEditorSelectionTransform(
          request.appState, state, -1, request.fineNudge));
      return;
    case cr::CreativeInputActionId::ConfirmActiveTool:
      static_cast<void>(requestCreativeEditorSelectionTransformCommit(state));
      return;
    case cr::CreativeInputActionId::CancelActiveTool:
      static_cast<void>(cancelCreativeEditorSelectionTransformPreview(
          state, "transform_preview_cancel"));
      return;
    default:
      return;
  }
}

void applyTransformControlAction(
    const CreativeEditorTransformFrameRequest& request,
    CreativeEditorSelectionTransformState& state,
    cr::CreativeInputActionId action) {
  switch (action) {
    case cr::CreativeInputActionId::ToggleTransformControls:
    case cr::CreativeInputActionId::CancelActiveTool:
      state.controlsOpen = false;
      return;
    case cr::CreativeInputActionId::TransformControlPrevious:
      moveControlSelection(state, -1);
      return;
    case cr::CreativeInputActionId::TransformControlNext:
      moveControlSelection(state, 1);
      return;
    case cr::CreativeInputActionId::ConfirmActiveTool:
      static_cast<void>(applyCreativeEditorTransformControl(
          request.appState, state,
          static_cast<CreativeEditorTransformControl>(state.selectedControl)));
      return;
    default:
      return;
  }
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

bool beginCreativeEditorClipboardTransformPreview(
    const cr::CreativeAppState& appState,
    const cr::CreativeClipboard& clipboard,
    CreativeEditorSelectionTransformState& state,
    std::string_view source) {
  return beginTransformPreview(
      appState, clipboard, CreativeEditorTransformSource::Clipboard,
      cr::CreativeSelectionPlacementMode::Copy, state, source);
}

bool beginCreativeEditorSelectionTransformPreview(
    cr::CreativeAppState& appState,
    CreativeEditorSelectionTransformState& state,
    std::string_view source) {
  cr::CreativeClipboard selection;
  const cr::CreativeClipboardCopyReceipt copied =
      appState.facade.copySelectedObjectsToClipboard(selection);
  if (!copied.accepted) {
    return false;
  }
  return beginTransformPreview(
      appState, selection, CreativeEditorTransformSource::Selection,
      cr::CreativeSelectionPlacementMode::Move, state, source);
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
      state.request.quarterTurns =
          static_cast<std::uint8_t>((state.request.quarterTurns + 1U) % 4U);
      break;
    case CreativeEditorTransformControl::MirrorX:
      state.request.mirrorX = !state.request.mirrorX;
      break;
    case CreativeEditorTransformControl::CycleConstraint:
      state.constraint = nextConstraint(state.constraint);
      state.lastNudge = {};
      targetChanged = true;
      break;
    case CreativeEditorTransformControl::ToggleMode:
      if (state.mode == cr::CreativeSelectionPlacementMode::Move) {
        state.mode = cr::CreativeSelectionPlacementMode::Copy;
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
      state.request.mirrorZ = !state.request.mirrorZ;
      break;
    case CreativeEditorTransformControl::RotateNegative:
      state.request.quarterTurns =
          static_cast<std::uint8_t>((state.request.quarterTurns + 3U) % 4U);
      break;
    case CreativeEditorTransformControl::Reset:
      changed = state.request.quarterTurns != 0U || state.request.mirrorX ||
                state.request.mirrorZ ||
                state.constraint != cr::CreativeSelectionPlacementAxis::Free ||
                !cr::creativeVec3ExactlyEqual(state.nudgeOffset, {});
      state.request.quarterTurns = 0U;
      state.request.mirrorX = false;
      state.request.mirrorZ = false;
      state.constraint = cr::CreativeSelectionPlacementAxis::Free;
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
  state.aimTargetPositionable =
      targetPositionable && cr::isFiniteCreativeVec3(targetAnchor);
  if (state.aimTargetPositionable) {
    state.aimTargetAnchor = targetAnchor;
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

CreativeEditorTransformFrameResult processCreativeEditorTransformFrame(
    const CreativeEditorTransformFrameRequest& request) {
  CreativeEditorTransformFrameResult result;
  CreativeEditorSelectionTransformState& state = request.editor.transform;
  if (!state.active) {
    return result;
  }
  const bool wasOpen = state.controlsOpen;
  state.fineNudgeActive = request.fineNudge;
  if (request.routedInput.context == cr::CreativeInputContext::TransformPreview) {
    for (const cr::CreativeInputActionEvent& event :
         request.routedInput.actionEvents()) {
      if (!state.active) {
        break;
      }
      applyTransformPreviewAction(request, state, event.action);
    }
  } else if (request.routedInput.context ==
             cr::CreativeInputContext::TransformControls) {
    for (const cr::CreativeInputActionEvent& event :
         request.routedInput.actionEvents()) {
      if (!state.active) {
        break;
      }
      applyTransformControlAction(request, state, event.action);
    }
  }

  if (state.active && !state.controlsOpen) {
    if (request.nudgeWheelSteps != 0) {
      static_cast<void>(nudgeCreativeEditorSelectionTransform(
          request.appState, state, request.nudgeWheelSteps,
          request.fineNudge));
    }
  }

  if (state.active && state.controlsOpen) {
    selectControlDirection(state, request.directionX, request.directionY, 0.35F);
    const iggy3d::SdlWindowEventState& events = request.window.eventState();
    const cr::CreativeDrawablePointer pointer =
        cr::resolveCreativeDrawablePointer(
            {events.pointerX, events.pointerY, events.windowWidth,
             events.windowHeight, request.drawableWidth,
             request.drawableHeight, events.pointerMoved,
             events.primaryPointerPressed});
    if (pointer.valid && pointer.moved) {
      selectControlDirection(state, pointer.centeredX, pointer.centeredY,
                             24.0F);
    }
    if (events.primaryPointerPressed) {
      static_cast<void>(applyCreativeEditorTransformControl(
          request.appState, state,
          static_cast<CreativeEditorTransformControl>(state.selectedControl)));
    }
  }

  result.openChanged = wasOpen != (state.active && state.controlsOpen);
  if (result.openChanged) {
    const bool controlsOpen = state.active && state.controlsOpen;
    static_cast<void>(request.window.setTextInputActive(false));
    static_cast<void>(request.window.setRelativeMouseMode(!controlsOpen));
    if (controlsOpen) {
      request.window.centerPointer();
    }
  }
  result.blockWorldActions = wasOpen || (state.active && state.controlsOpen) ||
                             result.openChanged;
  return result;
}

}  // namespace iggy3d_creative_app
