#include "EditorControls.hpp"
#include "EditorControlsInternal.hpp"

#include "EditorState.hpp"

#include "app/iggy3d/creative/render/CreativeOverlayFrame.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <string>
#include <string_view>

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

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
                      cr::kCreativeControlBindingRowCapacity + 5U <=
              cr::kCreativeUiWidgetCapacity);

}  // namespace

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
    if (binding.controllerCommandLayer) {
      label.append(" command");
    }
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
