#include "EditorControls.hpp"
#include "EditorControlsInternal.hpp"

#include "EditorCatalog.hpp"
#include "EditorState.hpp"
#include "EditorToolWheelPreferences.hpp"

#include "app/iggy3d/creative/input/UiInput.hpp"
#include "app/platform/SdlWindow.hpp"

#include <algorithm>
#include <array>
#include <string>

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

constexpr cr::CreativeWheelProfile kControlsWheelProfile{
    cr::CreativeWheelPolarity::Reversed,
    cr::CreativeWheelStepMode::Unit,
    1.0e-4F};

[[nodiscard]] bool actionPresent(
    const cr::CreativeInputRouteResult& routedInput,
    cr::CreativeInputActionId action) noexcept {
  return std::any_of(
      routedInput.actionEvents().begin(), routedInput.actionEvents().end(),
      [action](const cr::CreativeInputActionEvent& event) {
        return event.action == action;
      });
}

[[nodiscard]] bool keyMatchesDevice(cr::CreativeInputKey key,
                                    cr::CreativeControlDevice device) noexcept {
  return key != cr::CreativeInputKey::Unbound &&
         key != cr::CreativeInputKey::Count &&
         cr::creativeInputKeyIsGamepad(key) ==
             (device == cr::CreativeControlDevice::Gamepad);
}

[[nodiscard]] bool anyCaptureKeyDown(
    const cr::CreativeInputFrame& frame,
    cr::CreativeControlDevice device) noexcept {
  for (std::size_t index = 0;
       index < static_cast<std::size_t>(cr::CreativeInputKey::Count);
       ++index) {
    const cr::CreativeInputKey key = static_cast<cr::CreativeInputKey>(index);
    if (keyMatchesDevice(key, device) &&
        cr::creativeInputKeyDown(frame, key)) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] cr::CreativeInputKey firstCaptureKeyDown(
    const cr::CreativeInputFrame& frame,
    cr::CreativeControlDevice device) noexcept {
  for (std::size_t index = 0;
       index < static_cast<std::size_t>(cr::CreativeInputKey::Count);
       ++index) {
    const cr::CreativeInputKey key = static_cast<cr::CreativeInputKey>(index);
    if (keyMatchesDevice(key, device) &&
        !cr::creativeInputKeyIsModifier(key) &&
        cr::creativeInputKeyDown(frame, key)) {
      return key;
    }
  }
  for (std::size_t index = 0;
       index < static_cast<std::size_t>(cr::CreativeInputKey::Count);
       ++index) {
    const cr::CreativeInputKey key = static_cast<cr::CreativeInputKey>(index);
    if (keyMatchesDevice(key, device) &&
        cr::creativeInputKeyIsModifier(key) &&
        cr::creativeInputKeyDown(frame, key)) {
      return key;
    }
  }
  return cr::CreativeInputKey::Unbound;
}

[[nodiscard]] bool captureCancelKey(cr::CreativeInputKey key) noexcept {
  return key == cr::CreativeInputKey::Escape ||
         key == cr::CreativeInputKey::GamepadCancel ||
         key == cr::CreativeInputKey::GamepadStart;
}

void beginCapture(CreativeEditorControlsState& state,
                  const cr::CreativeControlBindingRow& row) {
  state.capturing = true;
  state.captureWaitingForRelease = true;
  state.captureGroup = row.group;
  state.captureDevice = row.device;
  state.statusLabel = "LISTENING";
}

[[nodiscard]] bool persistProfile(
    CreativeEditorState& editor,
    const std::filesystem::path& path) {
  const CreativeEditorControlPersistenceReceipt receipt =
      saveCreativeEditorControlProfile(editor.controlProfile, path,
                                       &editor.playtestWindowPreferences);
  editor.controls.statusLabel =
      receipt.status == CreativeEditorControlPersistenceStatus::Saved
          ? "SAVED"
          : "SAVE FAILED";
  return receipt.status == CreativeEditorControlPersistenceStatus::Saved;
}

[[nodiscard]] bool persistToolWheel(
    CreativeEditorState& editor,
    const std::filesystem::path& path) {
  const CreativeEditorToolWheelPersistenceReceipt receipt =
      saveCreativeEditorToolWheel(editor.catalog.toolWheel,
                                  editor.catalog.model, path);
  editor.controls.statusLabel =
      receipt.status == CreativeEditorToolWheelPersistenceStatus::Saved
          ? "TOOL WHEEL SAVED"
          : "TOOL WHEEL SAVE FAILED";
  return receipt.status == CreativeEditorToolWheelPersistenceStatus::Saved;
}

void processCapture(const CreativeEditorControlsFrameRequest& request,
                    CreativeEditorControlsFrameResult& result) {
  CreativeEditorControlsState& state = request.editor.controls;
  const cr::CreativeInputKey key =
      firstCaptureKeyDown(request.inputFrame, state.captureDevice);
  if (captureCancelKey(key)) {
    state.capturing = false;
    state.captureWaitingForRelease = false;
    state.statusLabel = "CANCELLED";
    return;
  }
  if (state.captureWaitingForRelease) {
    if (!anyCaptureKeyDown(request.inputFrame, state.captureDevice)) {
      state.captureWaitingForRelease = false;
    }
    return;
  }
  if (key == cr::CreativeInputKey::Unbound) {
    return;
  }
  const cr::CreativeInputModifierMask modifiers =
      state.captureDevice == cr::CreativeControlDevice::KeyboardMouse
          ? request.inputFrame.modifiers
          : cr::kCreativeInputModifierNone;
  state.lastRebind = cr::rebindCreativeControl(
      request.editor.controlProfile,
      {state.captureGroup, key, modifiers, state.conflictPolicy});
  state.capturing = false;
  state.captureWaitingForRelease = false;
  state.statusLabel = std::string(cr::toString(state.lastRebind.status));
  if (state.lastRebind.changed) {
    refreshBindingList(request.editor);
    result.profileChanged = true;
    result.profileSaved = persistProfile(request.editor, request.settingsPath);
  }
}

[[nodiscard]] bool adjustSelected(CreativeEditorState& editor,
                                  std::int32_t direction) {
  cr::CreativeControlSettingId setting =
      cr::CreativeControlSettingId::Count;
  if (selectedSetting(editor.controls, setting)) {
    return cr::adjustCreativeControlSetting(editor.controlProfile, setting,
                                            direction);
  }
  if (editor.controls.selectedIndex ==
      settingsForDevice(editor.controls.activeDevice).size()) {
    cycleConflictPolicy(editor.controls, direction);
  }
  return false;
}

void activateSelected(CreativeEditorState& editor,
                      CreativeEditorControlsFrameResult& result,
                      const std::filesystem::path& settingsPath) {
  if (cr::CreativeControlBindingRow* row =
          selectedBindingRow(editor.controls)) {
    beginCapture(editor.controls, *row);
    return;
  }
  const bool changed = adjustSelected(editor, 1);
  if (changed) {
    result.profileChanged = true;
    result.profileSaved = persistProfile(editor, settingsPath);
  }
}

void resetDefaults(CreativeEditorState& editor,
                   CreativeEditorControlsFrameResult& result,
                   const std::filesystem::path& settingsPath) {
  editor.controlProfile = cr::makeDefaultCreativeControlProfile();
  refreshBindingList(editor);
  editor.controls.statusLabel = "DEFAULTS RESTORED";
  result.profileChanged = true;
  result.profileSaved = persistProfile(editor, settingsPath);
}

void resetToolWheel(CreativeEditorState& editor,
                    CreativeEditorControlsFrameResult& result,
                    const std::filesystem::path& settingsPath) {
  result.toolWheelChanged =
      cr::resetCreativeToolWheel(editor.catalog.toolWheel,
                                 editor.catalog.model);
  result.toolWheelSaved = persistToolWheel(editor, settingsPath);
  if (result.toolWheelSaved) {
    editor.controls.statusLabel = result.toolWheelChanged
                                      ? "TOOL WHEEL RESTORED"
                                      : "TOOL WHEEL ALREADY DEFAULT";
  }
}

[[nodiscard]] cr::CreativeUiRepeatCommand heldRepeatCommand(
    const CreativeEditorControlsFrameRequest& request) noexcept {
  struct RepeatBinding {
    cr::CreativeInputActionId action;
    cr::CreativeUiRepeatCommand command;
  };
  constexpr std::array bindings{
      RepeatBinding{cr::CreativeInputActionId::ControlsPrevious,
                    cr::CreativeUiRepeatCommand::Previous},
      RepeatBinding{cr::CreativeInputActionId::ControlsNext,
                    cr::CreativeUiRepeatCommand::Next},
      RepeatBinding{cr::CreativeInputActionId::ControlsDecrease,
                    cr::CreativeUiRepeatCommand::Decrease},
      RepeatBinding{cr::CreativeInputActionId::ControlsIncrease,
                    cr::CreativeUiRepeatCommand::Increase},
  };
  for (const RepeatBinding& binding : bindings) {
    if (cr::creativeInputActionDown(
            request.inputFrame, binding.action,
            request.editor.controlProfile.bindingSpan())) {
      return binding.command;
    }
  }
  return cr::CreativeUiRepeatCommand::None;
}

void applyWidgetEvent(CreativeEditorState& editor,
                      const cr::CreativeUiWidgetEvent& event,
                      CreativeEditorControlsFrameResult& result,
                      const std::filesystem::path& settingsPath,
                      const std::filesystem::path& toolWheelSettingsPath) {
  if (event.kind == cr::CreativeUiWidgetEventKind::None) {
    return;
  }
  if (event.widgetId == kControlsResetWidgetId &&
      event.kind == cr::CreativeUiWidgetEventKind::Activate) {
    resetDefaults(editor, result, settingsPath);
    return;
  }
  if (event.widgetId == kControlsResetWheelWidgetId &&
      event.kind == cr::CreativeUiWidgetEventKind::Activate) {
    resetToolWheel(editor, result, toolWheelSettingsPath);
    return;
  }
  if (event.widgetId == kControlsDoneWidgetId &&
      event.kind == cr::CreativeUiWidgetEventKind::Activate) {
    editor.controls.open = false;
    editor.controls.repeatState = {};
    return;
  }
  if (event.kind == cr::CreativeUiWidgetEventKind::Activate &&
      event.widgetId == kControlsKeyboardTabWidgetId) {
    static_cast<void>(selectCreativeEditorControlsTab(
        editor, cr::CreativeControlDevice::KeyboardMouse));
    return;
  }
  if (event.kind == cr::CreativeUiWidgetEventKind::Activate &&
      event.widgetId == kControlsPs5TabWidgetId) {
    static_cast<void>(selectCreativeEditorControlsTab(
        editor, cr::CreativeControlDevice::Gamepad));
    return;
  }

  std::size_t row = 0U;
  if (!widgetRowIndex(event.widgetId, totalRowCount(editor.controls), row)) {
    return;
  }
  editor.controls.selectedIndex = row;
  if (event.kind == cr::CreativeUiWidgetEventKind::Activate) {
    activateSelected(editor, result, settingsPath);
    return;
  }
  const std::int32_t direction =
      event.kind == cr::CreativeUiWidgetEventKind::Decrease
          ? -1
          : event.kind == cr::CreativeUiWidgetEventKind::Increase ? 1 : 0;
  if (direction == 0) {
    return;
  }
  const bool changed = adjustSelected(editor, direction);
  if (changed) {
    result.profileChanged = true;
    result.profileSaved = persistProfile(editor, settingsPath);
  }
}

void processWidgetInput(const CreativeEditorControlsFrameRequest& request,
                        CreativeEditorControlsFrameResult& result) {
  CreativeEditorControlsState& state = request.editor.controls;
  const cr::CreativeUiWidgetFrame widgetFrame = buildControlsWidgetFrame(
      request.editor, request.drawableWidth, request.drawableHeight);
  if (widgetFrame.invalidInput || widgetFrame.capacityExceeded) {
    state.statusLabel = "UI CAPACITY ERROR";
    return;
  }

  const cr::CreativeUiRepeatResult repeat = cr::stepCreativeUiRepeat(
      {state.repeatState,
       heldRepeatCommand(request),
       request.monotonicTimeNanoseconds,
       request.editor.controlProfile.menuRepeatDelayMilliseconds,
       request.editor.controlProfile.menuRepeatIntervalMilliseconds});
  state.repeatState = repeat.next;

  cr::CreativeUiWidgetRouteInput input;
  input.focusedId = state.focusedWidgetId;
  if (repeat.fired) {
    switch (repeat.firedCommand) {
      case cr::CreativeUiRepeatCommand::Previous:
        input.focusSteps = -1;
        break;
      case cr::CreativeUiRepeatCommand::Next:
        input.focusSteps = 1;
        break;
      case cr::CreativeUiRepeatCommand::Decrease:
        input.decrease = true;
        break;
      case cr::CreativeUiRepeatCommand::Increase:
        input.increase = true;
        break;
      case cr::CreativeUiRepeatCommand::None:
      case cr::CreativeUiRepeatCommand::Count:
        break;
    }
  }
  input.activate = actionPresent(
      request.routedInput, cr::CreativeInputActionId::ControlsActivate);

  const iggy3d::SdlWindowEventState& events = request.window.eventState();
  input.pointer = cr::resolveCreativeDrawablePointer(
      {events.pointerX, events.pointerY, events.windowWidth,
       events.windowHeight, request.drawableWidth, request.drawableHeight,
       events.pointerMoved, events.primaryPointerPressed});
  input.focusSteps += cr::quantizeCreativeWheelSteps(
      events.mouseWheelY, kControlsWheelProfile);

  const cr::CreativeUiWidgetRouteResult routed =
      cr::routeCreativeUiWidgets(widgetFrame, input);
  if (!routed.valid) {
    return;
  }
  state.focusedWidgetId = routed.focusedId;
  const ControlsLayout layout =
      controlsLayout(request.drawableWidth, request.drawableHeight);
  keepFocusedRowVisible(state, layout.visibleRows);
  applyWidgetEvent(request.editor, routed.event, result, request.settingsPath,
                   request.toolWheelSettingsPath);
}

}  // namespace

bool selectCreativeEditorControlsTab(
    CreativeEditorState& editor,
    cr::CreativeControlDevice device) noexcept {
  if (device == cr::CreativeControlDevice::Count ||
      device == editor.controls.activeDevice) {
    return false;
  }
  editor.controls.activeDevice = device;
  editor.controls.selectedIndex = 0U;
  editor.controls.scrollOffset = 0U;
  editor.controls.focusedWidgetId = rowWidgetId(0U);
  editor.controls.repeatState = {};
  refreshBindingList(editor);
  return true;
}

CreativeEditorControlsFrameResult processCreativeEditorControlsFrame(
    const CreativeEditorControlsFrameRequest& request) {
  CreativeEditorControlsFrameResult result;
  CreativeEditorControlsState& state = request.editor.controls;
  const bool wasOpen = state.open;
  if (!state.open &&
      actionPresent(request.routedInput,
                    cr::CreativeInputActionId::ToggleControls)) {
    state.open = true;
    state.statusLabel.clear();
    state.repeatState = {};
    refreshBindingList(request.editor);
  }
  if (state.open &&
      request.routedInput.context == cr::CreativeInputContext::Controls) {
    if (state.capturing) {
      state.repeatState = {};
      processCapture(request, result);
    } else {
      if (actionPresent(request.routedInput,
                        cr::CreativeInputActionId::ControlsClose)) {
        state.open = false;
        state.repeatState = {};
      }
      if (state.open) {
        if (actionPresent(
                request.routedInput,
                cr::CreativeInputActionId::ControlsResetDefaults)) {
          resetDefaults(request.editor, result, request.settingsPath);
        }
        processWidgetInput(request, result);
      }
    }
  }
  result.openChanged = wasOpen != state.open;
  if (result.openChanged) {
    static_cast<void>(request.window.setTextInputActive(false));
    static_cast<void>(request.window.setRelativeMouseMode(!state.open));
  }
  result.blockWorldActions =
      state.open || wasOpen ||
      request.routedInput.context == cr::CreativeInputContext::Controls;
  if (result.blockWorldActions) {
    request.editor.interaction.target = {};
    request.editor.volume.cursorValid = false;
  }
  return result;
}

}  // namespace iggy3d_creative_app
