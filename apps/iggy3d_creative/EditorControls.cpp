#include "EditorControls.hpp"

#include "EditorCatalog.hpp"
#include "EditorState.hpp"
#include "EditorToolWheelPreferences.hpp"

#include "app/iggy3d/creative/input/UiInput.hpp"
#include "app/iggy3d/creative/render/CreativeOverlayFrame.hpp"
#include "app/platform/SdlWindow.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

constexpr std::string_view kControlFileHeader =
    "iggy3d_creative_controls 2";
constexpr std::string_view kLegacyControlFileHeader =
    "iggy3d_creative_controls 1";
constexpr cr::CreativeUiWidgetId kControlRowWidgetIdBase = 1U;
constexpr cr::CreativeUiWidgetId kControlsResetWidgetId = 1000U;
constexpr cr::CreativeUiWidgetId kControlsDoneWidgetId = 1001U;
constexpr cr::CreativeUiWidgetId kControlsKeyboardTabWidgetId = 1002U;
constexpr cr::CreativeUiWidgetId kControlsPs5TabWidgetId = 1003U;
constexpr cr::CreativeUiWidgetId kControlsResetWheelWidgetId = 1004U;
constexpr cr::CreativeWheelProfile kControlsWheelProfile{
    cr::CreativeWheelPolarity::Reversed,
    cr::CreativeWheelStepMode::Unit,
    1.0e-4F};

constexpr std::array kKeyboardMouseSettings{
    cr::CreativeControlSettingId::MouseLookSensitivity,
    cr::CreativeControlSettingId::MenuRepeatDelay,
    cr::CreativeControlSettingId::MenuRepeatInterval,
};
constexpr std::array kPs5Settings{
    cr::CreativeControlSettingId::GamepadLookSensitivity,
    cr::CreativeControlSettingId::MovementDeadzone,
    cr::CreativeControlSettingId::MovementResponse,
    cr::CreativeControlSettingId::LookDeadzone,
    cr::CreativeControlSettingId::LookResponse,
    cr::CreativeControlSettingId::InvertLookX,
    cr::CreativeControlSettingId::InvertLookY,
    cr::CreativeControlSettingId::MenuRepeatDelay,
    cr::CreativeControlSettingId::MenuRepeatInterval,
};

static_assert(cr::kCreativeControlSettingCount + 1U +
                      cr::kCreativeInputBindingCapacity + 5U <=
              cr::kCreativeUiWidgetCapacity);

struct ControlsLayout {
  std::int32_t panelX = 0;
  std::int32_t panelY = 0;
  std::uint32_t panelWidth = 0;
  std::uint32_t panelHeight = 0;
  std::int32_t tabsY = 0;
  std::uint32_t tabHeight = 30;
  std::int32_t rowsY = 0;
  std::uint32_t rowHeight = 36;
  std::size_t visibleRows = 1;
  std::int32_t footerY = 0;
};

[[nodiscard]] ControlsLayout controlsLayout(std::uint32_t drawableWidth,
                                            std::uint32_t drawableHeight) {
  ControlsLayout layout;
  const std::int32_t width = static_cast<std::int32_t>(drawableWidth);
  const std::int32_t height = static_cast<std::int32_t>(drawableHeight);
  layout.panelWidth = static_cast<std::uint32_t>(
      std::max(1, std::min(820, width - 16)));
  layout.panelHeight = static_cast<std::uint32_t>(
      std::max(1, std::min(650, height - 16)));
  layout.panelX = std::max(
      0, (width - static_cast<std::int32_t>(layout.panelWidth)) / 2);
  layout.panelY = std::max(
      0, (height - static_cast<std::int32_t>(layout.panelHeight)) / 2);
  layout.tabsY = layout.panelY + 62;
  layout.rowsY = layout.panelY + 100;
  layout.footerY = std::max(
      layout.rowsY + static_cast<std::int32_t>(layout.rowHeight),
      layout.panelY + static_cast<std::int32_t>(layout.panelHeight) - 46);
  const std::int32_t rowsHeight = std::max(1, layout.footerY - layout.rowsY);
  layout.visibleRows = std::max<std::size_t>(
      1U, static_cast<std::size_t>(rowsHeight /
                                  static_cast<std::int32_t>(layout.rowHeight)));
  return layout;
}

[[nodiscard]] bool actionPresent(
    const cr::CreativeInputRouteResult& routedInput,
    cr::CreativeInputActionId action) noexcept {
  return std::any_of(
      routedInput.actionEvents().begin(), routedInput.actionEvents().end(),
      [action](const cr::CreativeInputActionEvent& event) {
        return event.action == action;
      });
}

[[nodiscard]] bool parseDevice(std::string_view value,
                               cr::CreativeControlDevice& out) noexcept {
  if (value == "KeyboardMouse") {
    out = cr::CreativeControlDevice::KeyboardMouse;
    return true;
  }
  if (value == "Gamepad") {
    out = cr::CreativeControlDevice::Gamepad;
    return true;
  }
  return false;
}

[[nodiscard]] std::span<const cr::CreativeControlSettingId>
settingsForDevice(cr::CreativeControlDevice device) noexcept {
  switch (device) {
    case cr::CreativeControlDevice::KeyboardMouse:
      return kKeyboardMouseSettings;
    case cr::CreativeControlDevice::Gamepad:
      return kPs5Settings;
    case cr::CreativeControlDevice::Count:
      return {};
  }
  return {};
}

[[nodiscard]] std::size_t staticRowCount(
    const CreativeEditorControlsState& state) noexcept {
  return settingsForDevice(state.activeDevice).size() + 1U;
}

[[nodiscard]] std::size_t totalRowCount(
    const CreativeEditorControlsState& state) noexcept {
  return staticRowCount(state) + state.bindingList.count;
}

[[nodiscard]] cr::CreativeUiWidgetId rowWidgetId(
    std::size_t row) noexcept {
  return kControlRowWidgetIdBase +
         static_cast<cr::CreativeUiWidgetId>(row);
}

[[nodiscard]] bool widgetRowIndex(cr::CreativeUiWidgetId widgetId,
                                  std::size_t rowCount,
                                  std::size_t& row) noexcept {
  if (widgetId < kControlRowWidgetIdBase) {
    return false;
  }
  const std::size_t candidate = static_cast<std::size_t>(
      widgetId - kControlRowWidgetIdBase);
  if (candidate >= rowCount) {
    return false;
  }
  row = candidate;
  return true;
}

[[nodiscard]] bool controlsChromeWidgetId(
    cr::CreativeUiWidgetId widgetId) noexcept {
  return widgetId == kControlsResetWidgetId ||
         widgetId == kControlsResetWheelWidgetId ||
         widgetId == kControlsDoneWidgetId ||
         widgetId == kControlsKeyboardTabWidgetId ||
         widgetId == kControlsPs5TabWidgetId;
}

[[nodiscard]] cr::CreativeControlBindingList bindingListForDevice(
    const cr::CreativeControlProfile& profile,
    cr::CreativeControlDevice device) noexcept {
  cr::CreativeControlBindingList result;
  if (device == cr::CreativeControlDevice::Count) {
    result.capacityExceeded = true;
    return result;
  }
  const cr::CreativeControlBindingList all =
      cr::buildCreativeControlBindingList(profile);
  result.capacityExceeded = all.capacityExceeded;
  for (const cr::CreativeControlBindingRow& row : all.items()) {
    if (row.device != device) {
      continue;
    }
    if (result.count >= result.rows.size()) {
      result.capacityExceeded = true;
      break;
    }
    result.rows[result.count++] = row;
  }
  return result;
}

void refreshBindingList(CreativeEditorState& editor) {
  editor.controls.bindingList = bindingListForDevice(
      editor.controlProfile, editor.controls.activeDevice);
  const std::size_t count = totalRowCount(editor.controls);
  if (count == 0U) {
    editor.controls.selectedIndex = 0U;
    editor.controls.scrollOffset = 0U;
    editor.controls.focusedWidgetId = cr::kInvalidCreativeUiWidgetId;
  } else {
    editor.controls.selectedIndex =
        std::min(editor.controls.selectedIndex, count - 1U);
    std::size_t focusedRow = 0U;
    if (!controlsChromeWidgetId(editor.controls.focusedWidgetId) &&
        !widgetRowIndex(editor.controls.focusedWidgetId, count, focusedRow)) {
      editor.controls.focusedWidgetId =
          rowWidgetId(editor.controls.selectedIndex);
    }
  }
}

void keepFocusedRowVisible(CreativeEditorControlsState& state,
                           std::size_t visibleRows) noexcept {
  const std::size_t count = totalRowCount(state);
  if (count == 0U) {
    state.scrollOffset = 0U;
    return;
  }
  std::size_t focusedRow = 0U;
  if (widgetRowIndex(state.focusedWidgetId, count, focusedRow)) {
    state.selectedIndex = focusedRow;
    state.scrollOffset = cr::keepCreativeUiListIndexVisible(
        focusedRow, count, visibleRows, state.scrollOffset);
  }
}

[[nodiscard]] cr::CreativeControlBindingRow* selectedBindingRow(
    CreativeEditorControlsState& state) noexcept {
  const std::size_t firstBindingRow = staticRowCount(state);
  if (state.selectedIndex < firstBindingRow) {
    return nullptr;
  }
  const std::size_t index = state.selectedIndex - firstBindingRow;
  return index < state.bindingList.count ? &state.bindingList.rows[index]
                                         : nullptr;
}

[[nodiscard]] bool selectedSetting(
    const CreativeEditorControlsState& state,
    cr::CreativeControlSettingId& setting) noexcept {
  const std::span<const cr::CreativeControlSettingId> settings =
      settingsForDevice(state.activeDevice);
  if (state.selectedIndex >= settings.size()) {
    return false;
  }
  setting = settings[state.selectedIndex];
  return true;
}

void cycleConflictPolicy(CreativeEditorControlsState& state,
                         std::int32_t direction) noexcept {
  const cr::CreativeWrappedIndexResult next = cr::stepCreativeWrappedIndex(
      static_cast<std::size_t>(state.conflictPolicy),
      static_cast<std::size_t>(cr::CreativeControlConflictPolicy::Count),
      direction);
  if (next.valid) {
    state.conflictPolicy =
        static_cast<cr::CreativeControlConflictPolicy>(next.index);
  }
}

[[nodiscard]] std::string settingValueLabel(
    const cr::CreativeControlProfile& profile,
    cr::CreativeControlSettingId setting) {
  char buffer[64]{};
  switch (setting) {
    case cr::CreativeControlSettingId::MouseLookSensitivity:
      std::snprintf(buffer, sizeof(buffer), "%.2f",
                    profile.mouseLookSensitivity);
      return buffer;
    case cr::CreativeControlSettingId::GamepadLookSensitivity:
      std::snprintf(buffer, sizeof(buffer), "%.1f",
                    profile.gamepadLookSensitivity);
      return buffer;
    case cr::CreativeControlSettingId::MovementDeadzone:
      std::snprintf(buffer, sizeof(buffer), "%.2f",
                    profile.movementStick.deadzone);
      return buffer;
    case cr::CreativeControlSettingId::MovementResponse:
      std::snprintf(buffer, sizeof(buffer), "%.1f",
                    profile.movementStick.responseExponent);
      return buffer;
    case cr::CreativeControlSettingId::LookDeadzone:
      std::snprintf(buffer, sizeof(buffer), "%.2f",
                    profile.lookStick.deadzone);
      return buffer;
    case cr::CreativeControlSettingId::LookResponse:
      std::snprintf(buffer, sizeof(buffer), "%.1f",
                    profile.lookStick.responseExponent);
      return buffer;
    case cr::CreativeControlSettingId::InvertLookX:
      return profile.lookStick.invertX ? "ON" : "OFF";
    case cr::CreativeControlSettingId::InvertLookY:
      return profile.lookStick.invertY ? "ON" : "OFF";
    case cr::CreativeControlSettingId::MenuRepeatDelay:
      return std::to_string(profile.menuRepeatDelayMilliseconds) + " ms";
    case cr::CreativeControlSettingId::MenuRepeatInterval:
      return std::to_string(profile.menuRepeatIntervalMilliseconds) + " ms";
    case cr::CreativeControlSettingId::Count:
      return {};
  }
  return {};
}

[[nodiscard]] std::string actionDisplayLabel(
    cr::CreativeInputActionId action) {
  const std::string_view identifier = cr::toString(action);
  std::string label;
  label.reserve(identifier.size() + 8U);
  for (std::size_t index = 0; index < identifier.size(); ++index) {
    const char value = identifier[index];
    const bool upper = value >= 'A' && value <= 'Z';
    const bool digit = value >= '0' && value <= '9';
    if (index > 0U) {
      const char previous = identifier[index - 1U];
      const bool previousLower = previous >= 'a' && previous <= 'z';
      const bool previousDigit = previous >= '0' && previous <= '9';
      if ((upper && (previousLower || previousDigit)) ||
          (digit && !previousDigit)) {
        label.push_back(' ');
      }
    }
    label.push_back(value);
  }
  return label;
}

[[nodiscard]] constexpr cr::CreativeInputPlatform controlInputPlatform()
    noexcept {
#if defined(__APPLE__)
  return cr::CreativeInputPlatform::MacOS;
#else
  return cr::CreativeInputPlatform::WindowsLinux;
#endif
}

[[nodiscard]] cr::CreativeUiWidgetFrame buildControlsWidgetFrame(
    const CreativeEditorState& editor,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight) {
  cr::CreativeUiWidgetFrame frame;
  cr::resetCreativeUiWidgetFrame(frame, drawableWidth, drawableHeight);
  const CreativeEditorControlsState& state = editor.controls;
  const ControlsLayout layout =
      controlsLayout(drawableWidth, drawableHeight);
  const iggy3d::CreativeUiRect panelRect{
      static_cast<float>(layout.panelX),
      static_cast<float>(layout.panelY),
      static_cast<float>(layout.panelWidth),
      static_cast<float>(layout.panelHeight)};
  static_cast<void>(cr::appendCreativeUiScrim(frame));
  static_cast<void>(cr::appendCreativeUiPanel(frame, panelRect));
  static_cast<void>(cr::appendCreativeUiLabel(
      frame,
      {static_cast<float>(layout.panelX + 18),
       static_cast<float>(layout.panelY + 16), 0.0F, 0.0F},
      "CONTROLS"));
  const std::string conflictLabel =
      std::string("CONFLICT ") + std::string(cr::toString(state.conflictPolicy));
  static_cast<void>(cr::appendCreativeUiLabel(
      frame,
      {static_cast<float>(layout.panelX + 190),
       static_cast<float>(layout.panelY + 16), 0.0F, 0.0F},
      conflictLabel, iggy3d::CreativeUiTone::TextMuted));
  if (!state.statusLabel.empty()) {
    static_cast<void>(cr::appendCreativeUiLabel(
        frame,
        {static_cast<float>(layout.panelX + 18),
         static_cast<float>(layout.panelY + 42), 0.0F, 0.0F},
        state.statusLabel, iggy3d::CreativeUiTone::Accent));
  }

  const float tabsX = static_cast<float>(layout.panelX + 12);
  const float tabsAreaWidth = static_cast<float>(
      std::max<std::uint32_t>(
          1U, layout.panelWidth > 24U ? layout.panelWidth - 24U : 1U));
  const float tabGap = std::min(4.0F, tabsAreaWidth * 0.1F);
  const float firstTabWidth = (tabsAreaWidth - tabGap) * 0.5F;
  const float secondTabWidth = tabsAreaWidth - tabGap - firstTabWidth;
  const bool compactTabs = firstTabWidth < 150.0F;
  cr::CreativeUiWidgetSpec keyboardTab;
  keyboardTab.id = kControlsKeyboardTabWidgetId;
  keyboardTab.rect = {tabsX, static_cast<float>(layout.tabsY), firstTabWidth,
                      static_cast<float>(layout.tabHeight)};
  keyboardTab.label = compactTabs ? "KEYBOARD" : "KEYBOARD + MOUSE";
  keyboardTab.selected =
      state.activeDevice == cr::CreativeControlDevice::KeyboardMouse;
  keyboardTab.focused = state.focusedWidgetId == keyboardTab.id;
  static_cast<void>(cr::appendCreativeUiTab(frame, keyboardTab));

  cr::CreativeUiWidgetSpec ps5Tab;
  ps5Tab.id = kControlsPs5TabWidgetId;
  ps5Tab.rect = {tabsX + firstTabWidth + tabGap,
                 static_cast<float>(layout.tabsY), secondTabWidth,
                 static_cast<float>(layout.tabHeight)};
  ps5Tab.label = compactTabs ? "PS5" : "PS5 CONTROLLER";
  ps5Tab.selected = state.activeDevice == cr::CreativeControlDevice::Gamepad;
  ps5Tab.focused = state.focusedWidgetId == ps5Tab.id;
  static_cast<void>(cr::appendCreativeUiTab(frame, ps5Tab));

  const std::size_t totalRows = totalRowCount(state);
  const cr::CreativeUiVisibleRange visibleRange =
      cr::resolveCreativeUiVisibleRange(totalRows, layout.visibleRows,
                                        state.scrollOffset);
  const float rowX = static_cast<float>(layout.panelX + 12);
  const float rowWidth =
      static_cast<float>(std::max<std::uint32_t>(
          1U, layout.panelWidth > 24U ? layout.panelWidth - 24U : 1U));
  for (std::size_t row = 0U; row < totalRows; ++row) {
    const bool visible =
        visibleRange.valid && row >= visibleRange.first &&
        row < visibleRange.first + visibleRange.count;
    const std::size_t localRow =
        visible ? row - visibleRange.first : 0U;
    cr::CreativeUiWidgetSpec spec;
    spec.id = rowWidgetId(row);
    spec.rect = {
        rowX,
        visible
            ? static_cast<float>(
                  layout.rowsY +
                  static_cast<std::int32_t>(localRow * layout.rowHeight))
            : 0.0F,
        rowWidth,
        static_cast<float>(layout.rowHeight - 3U)};
    spec.focused = state.focusedWidgetId == spec.id;
    spec.visible = visible;

    const std::span<const cr::CreativeControlSettingId> settings =
        settingsForDevice(state.activeDevice);
    if (row < settings.size()) {
      const cr::CreativeControlSettingId setting = settings[row];
      spec.label = cr::toString(setting);
      const std::string value = settingValueLabel(editor.controlProfile,
                                                  setting);
      spec.value = value;
      const bool toggle =
          setting == cr::CreativeControlSettingId::InvertLookX ||
          setting == cr::CreativeControlSettingId::InvertLookY;
      if (toggle) {
        static_cast<void>(cr::appendCreativeUiToggle(frame, spec));
      } else {
        static_cast<void>(cr::appendCreativeUiStepper(frame, spec));
      }
      continue;
    }
    if (row == settings.size()) {
      spec.label = "Conflict mode";
      const std::string value = std::string(cr::toString(state.conflictPolicy));
      spec.value = value;
      static_cast<void>(cr::appendCreativeUiStepper(frame, spec));
      continue;
    }

    const cr::CreativeControlBindingRow& binding =
        state.bindingList.rows[row - staticRowCount(state)];
    std::string label = actionDisplayLabel(binding.action);
    std::string value =
        state.capturing && binding.group == state.captureGroup
            ? "LISTENING"
            : cr::creativeControlBindingDisplayLabel(
                  binding, controlInputPlatform());
    spec.label = label;
    spec.value = value;
    spec.selected = state.capturing && binding.group == state.captureGroup;
    static_cast<void>(cr::appendCreativeUiListRow(frame, spec));
  }

  const float footerWidth = static_cast<float>(layout.panelWidth) / 3.0F;
  cr::CreativeUiWidgetSpec reset;
  reset.id = kControlsResetWidgetId;
  reset.rect = {static_cast<float>(layout.panelX),
                static_cast<float>(layout.footerY), footerWidth, 38.0F};
  reset.label = "RESET CONTROLS";
  reset.focused = state.focusedWidgetId == reset.id;
  static_cast<void>(cr::appendCreativeUiButton(frame, reset));

  cr::CreativeUiWidgetSpec resetWheel;
  resetWheel.id = kControlsResetWheelWidgetId;
  resetWheel.rect = {static_cast<float>(layout.panelX) + footerWidth,
                     static_cast<float>(layout.footerY), footerWidth, 38.0F};
  resetWheel.label = "RESET WHEEL";
  resetWheel.focused = state.focusedWidgetId == resetWheel.id;
  static_cast<void>(cr::appendCreativeUiButton(frame, resetWheel));

  cr::CreativeUiWidgetSpec done;
  done.id = kControlsDoneWidgetId;
  done.rect = {static_cast<float>(layout.panelX) + footerWidth * 2.0F,
               static_cast<float>(layout.footerY),
               static_cast<float>(layout.panelWidth) - footerWidth * 2.0F,
               38.0F};
  done.label = "DONE";
  done.selected = true;
  done.focused = state.focusedWidgetId == done.id;
  static_cast<void>(cr::appendCreativeUiButton(frame, done));
  return frame;
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
      saveCreativeEditorControlProfile(editor.controlProfile, path);
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

namespace {

CreativeEditorControlPersistenceReceipt parseControlProfileStream(
    std::istream& input,
    cr::CreativeControlProfile& profile) {
  CreativeEditorControlPersistenceReceipt receipt;
  std::string header;
  std::getline(input, header);
  const bool legacyV1 = header == kLegacyControlFileHeader;
  if (header != kControlFileHeader && !legacyV1) {
    receipt.status = CreativeEditorControlPersistenceStatus::Invalid;
    return receipt;
  }

  cr::CreativeControlProfile candidate =
      cr::makeDefaultCreativeControlProfile();
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty()) {
      continue;
    }
    std::istringstream row(line);
    std::string kind;
    row >> kind;
    if (kind == "mouse_sensitivity") {
      row >> candidate.mouseLookSensitivity;
    } else if (kind == "gamepad_sensitivity") {
      row >> candidate.gamepadLookSensitivity;
    } else if (kind == "movement_stick") {
      int invertX = 0;
      int invertY = 0;
      row >> candidate.movementStick.deadzone >>
          candidate.movementStick.responseExponent >> invertX >> invertY;
      candidate.movementStick.invertX = invertX != 0;
      candidate.movementStick.invertY = invertY != 0;
    } else if (kind == "look_stick") {
      int invertX = 0;
      int invertY = 0;
      row >> candidate.lookStick.deadzone >>
          candidate.lookStick.responseExponent >> invertX >> invertY;
      candidate.lookStick.invertX = invertX != 0;
      candidate.lookStick.invertY = invertY != 0;
    } else if (kind == "menu_repeat") {
      row >> candidate.menuRepeatDelayMilliseconds >>
          candidate.menuRepeatIntervalMilliseconds;
    } else if (kind == "bind") {
      std::string actionName;
      std::string deviceName;
      std::uint16_t ordinal = 0;
      std::string keyName;
      unsigned requiredAll = 0;
      unsigned requiredAny = 0;
      unsigned allowed = 0;
      row >> actionName >> deviceName >> ordinal >> keyName >> requiredAll >>
          requiredAny >> allowed;
      cr::CreativeInputActionId action = cr::CreativeInputActionId::Count;
      cr::CreativeControlDevice device = cr::CreativeControlDevice::Count;
      cr::CreativeInputKey key = cr::CreativeInputKey::Count;
      if (!cr::parseCreativeInputActionId(actionName, action) ||
          !parseDevice(deviceName, device) ||
          !cr::parseCreativeInputKey(keyName, key)) {
        receipt.status = CreativeEditorControlPersistenceStatus::Invalid;
        return receipt;
      }
      if (legacyV1 && action == cr::CreativeInputActionId::PickAction &&
          device == cr::CreativeControlDevice::Gamepad && ordinal == 0U &&
          key == cr::CreativeInputKey::GamepadWest) {
        key = cr::CreativeInputKey::GamepadTouchpad;
      }
      std::size_t groupIndexValue = candidate.groupCount;
      for (std::size_t index = 0; index < candidate.groupCount; ++index) {
        if (candidate.groupActions[index] == action &&
            candidate.groupDevices[index] == device &&
            candidate.groupOrdinals[index] == ordinal) {
          groupIndexValue = index;
          break;
        }
      }
      if (groupIndexValue == candidate.groupCount) {
        receipt.status = CreativeEditorControlPersistenceStatus::Invalid;
        return receipt;
      }
      const std::uint16_t groupIndex = static_cast<std::uint16_t>(
          groupIndexValue);
      if (!cr::applyStoredCreativeControlChord(
              candidate, groupIndex, key,
              static_cast<cr::CreativeInputModifierMask>(requiredAll),
              static_cast<cr::CreativeInputModifierMask>(requiredAny),
              static_cast<cr::CreativeInputModifierMask>(allowed))) {
        receipt.status = CreativeEditorControlPersistenceStatus::Invalid;
        return receipt;
      }
    } else {
      receipt.status = CreativeEditorControlPersistenceStatus::Invalid;
      return receipt;
    }
    if (!row) {
      receipt.status = CreativeEditorControlPersistenceStatus::Invalid;
      return receipt;
    }
    row >> std::ws;
    if (!row.eof()) {
      receipt.status = CreativeEditorControlPersistenceStatus::Invalid;
      return receipt;
    }
  }
  if (!input.eof() || !cr::isValidCreativeControlProfile(candidate)) {
    receipt.status = CreativeEditorControlPersistenceStatus::Invalid;
    return receipt;
  }
  profile = candidate;
  receipt.status = CreativeEditorControlPersistenceStatus::Loaded;
  receipt.bindingCount = profile.bindingCount;
  receipt.accepted = true;
  return receipt;
}

CreativeEditorControlPersistenceReceipt serializeControlProfileStream(
    std::ostream& output,
    const cr::CreativeControlProfile& profile) {
  CreativeEditorControlPersistenceReceipt receipt;
  receipt.status = CreativeEditorControlPersistenceStatus::IoError;
  if (!cr::isValidCreativeControlProfile(profile)) {
    receipt.status = CreativeEditorControlPersistenceStatus::Invalid;
    return receipt;
  }
  output << kControlFileHeader << '\n' << std::setprecision(
      std::numeric_limits<float>::max_digits10);
  output << "mouse_sensitivity " << profile.mouseLookSensitivity << '\n';
  output << "gamepad_sensitivity " << profile.gamepadLookSensitivity << '\n';
  output << "movement_stick " << profile.movementStick.deadzone << ' '
         << profile.movementStick.responseExponent << ' '
         << (profile.movementStick.invertX ? 1 : 0) << ' '
         << (profile.movementStick.invertY ? 1 : 0) << '\n';
  output << "look_stick " << profile.lookStick.deadzone << ' '
         << profile.lookStick.responseExponent << ' '
         << (profile.lookStick.invertX ? 1 : 0) << ' '
         << (profile.lookStick.invertY ? 1 : 0) << '\n';
  output << "menu_repeat " << profile.menuRepeatDelayMilliseconds << ' '
         << profile.menuRepeatIntervalMilliseconds << '\n';
  for (std::size_t group = 0; group < profile.groupCount; ++group) {
    if (cr::creativeControlActionIsReserved(profile.groupActions[group])) {
      continue;
    }
    const cr::CreativeInputBinding* binding = cr::creativeControlGroupBinding(
        profile, static_cast<std::uint16_t>(group));
    if (binding == nullptr) {
      receipt.status = CreativeEditorControlPersistenceStatus::Invalid;
      return receipt;
    }
    output << "bind " << cr::toString(profile.groupActions[group]) << ' '
           << cr::toString(profile.groupDevices[group]) << ' '
           << profile.groupOrdinals[group] << ' '
           << cr::toString(binding->trigger) << ' '
           << static_cast<unsigned>(binding->requiredAllModifiers) << ' '
           << static_cast<unsigned>(binding->requiredAnyModifiers) << ' '
           << static_cast<unsigned>(binding->allowedModifiers) << '\n';
  }
  if (!output) {
    return receipt;
  }
  receipt.status = CreativeEditorControlPersistenceStatus::Saved;
  receipt.bindingCount = profile.bindingCount;
  receipt.accepted = true;
  return receipt;
}

}  // namespace

CreativeEditorControlPersistenceReceipt parseCreativeEditorControlProfile(
    std::string_view text,
    cr::CreativeControlProfile& profile) {
  std::istringstream input{std::string{text}};
  return parseControlProfileStream(input, profile);
}

CreativeEditorControlPersistenceReceipt serializeCreativeEditorControlProfile(
    const cr::CreativeControlProfile& profile,
    std::string& text) {
  std::ostringstream output;
  CreativeEditorControlPersistenceReceipt receipt =
      serializeControlProfileStream(output, profile);
  if (receipt.accepted) {
    text = output.str();
  } else {
    text.clear();
  }
  return receipt;
}

CreativeEditorControlPersistenceReceipt loadCreativeEditorControlProfile(
    cr::CreativeControlProfile& profile,
    const std::filesystem::path& path) {
  std::ifstream input(path);
  if (!input.is_open()) {
    return {};
  }
  return parseControlProfileStream(input, profile);
}

CreativeEditorControlPersistenceReceipt saveCreativeEditorControlProfile(
    const cr::CreativeControlProfile& profile,
    const std::filesystem::path& path) {
  std::string text;
  CreativeEditorControlPersistenceReceipt receipt =
      serializeCreativeEditorControlProfile(profile, text);
  if (!receipt.accepted) {
    return receipt;
  }

  receipt.accepted = false;
  receipt.status = CreativeEditorControlPersistenceStatus::IoError;
  std::error_code error;
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) {
      return receipt;
    }
  }
  const std::filesystem::path temporary = path.string() + ".tmp";
  std::ofstream output(temporary, std::ios::trunc);
  if (!output.is_open()) {
    return receipt;
  }
  output << text;
  output.close();
  if (!output) {
    std::filesystem::remove(temporary, error);
    return receipt;
  }
  std::filesystem::rename(temporary, path, error);
  if (error) {
    error.clear();
    std::filesystem::remove(path, error);
    error.clear();
    std::filesystem::rename(temporary, path, error);
  }
  if (error) {
    std::filesystem::remove(temporary, error);
    return receipt;
  }
  receipt.status = CreativeEditorControlPersistenceStatus::Saved;
  receipt.bindingCount = profile.bindingCount;
  receipt.accepted = true;
  return receipt;
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

void appendCreativeEditorControlsOverlay(
    const CreativeEditorState& editor,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    std::vector<iggy3d::RenderUiRect>& uiRects,
    std::vector<iggy3d::DebugHudGlyphQuad>& glyphs) {
  const CreativeEditorControlsState& state = editor.controls;
  if (!state.open || drawableWidth == 0U || drawableHeight == 0U) {
    return;
  }
  const cr::CreativeUiWidgetFrame widgetFrame =
      buildControlsWidgetFrame(editor, drawableWidth, drawableHeight);
  static_cast<void>(iggy3d::appendCreativeUiWidgetOverlay(
      widgetFrame, drawableWidth, drawableHeight, uiRects, glyphs));
}

}  // namespace iggy3d_creative_app
