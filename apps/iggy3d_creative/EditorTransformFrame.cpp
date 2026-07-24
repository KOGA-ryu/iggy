#include "EditorTransformFrame.hpp"

#include "EditorTransform.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/input/InputRouter.hpp"
#include "app/iggy3d/creative/input/UiInput.hpp"
#include "app/platform/SdlWindow.hpp"

namespace iggy3d_creative_app {
namespace {

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

void setTransformAxis(const CreativeEditorTransformFrameRequest& request,
                      CreativeEditorSelectionTransformState& state,
                      cr::CreativeSelectionPlacementAxis placementAxis,
                      cr::CreativeAxis3 rotationAxis) {
  if (state.transformMode == CreativeEditorTransformMode::Rotate) {
    static_cast<void>(setCreativeEditorTransformRotationDegrees(
        request.appState, state, rotationAxis, state.rotationDegrees));
    return;
  }
  const cr::CreativeSelectionPlacementAxis next =
      state.constraint == placementAxis
          ? cr::CreativeSelectionPlacementAxis::Free
          : placementAxis;
  static_cast<void>(setCreativeEditorTransformConstraint(
      request.appState, state, next));
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
      setTransformAxis(request, state, cr::CreativeSelectionPlacementAxis::X,
                       cr::CreativeAxis3::X);
      return;
    case cr::CreativeInputActionId::TransformConstraintY:
      setTransformAxis(request, state, cr::CreativeSelectionPlacementAxis::Y,
                       cr::CreativeAxis3::Y);
      return;
    case cr::CreativeInputActionId::TransformConstraintZ:
      setTransformAxis(request, state, cr::CreativeSelectionPlacementAxis::Z,
                       cr::CreativeAxis3::Z);
      return;
    case cr::CreativeInputActionId::TransformNudgePositive:
      static_cast<void>(nudgeCreativeEditorSelectionTransform(
          request.appState, state, 1, request.fineNudge));
      return;
    case cr::CreativeInputActionId::TransformNudgeNegative:
      static_cast<void>(nudgeCreativeEditorSelectionTransform(
          request.appState, state, -1, request.fineNudge));
      return;
    case cr::CreativeInputActionId::QuickEditNext:
      static_cast<void>(cycleCreativeEditorTransformMode(
          request.appState, state));
      return;
    case cr::CreativeInputActionId::QuickEditDecrease:
      static_cast<void>(adjustCreativeEditorTransformSetting(
          request.appState, state, -1));
      return;
    case cr::CreativeInputActionId::QuickEditIncrease:
      static_cast<void>(adjustCreativeEditorTransformSetting(
          request.appState, state, 1));
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
    if (wasOpen && !controlsOpen) {
      request.editor.rightStickLookRearmRequired = true;
    }
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
