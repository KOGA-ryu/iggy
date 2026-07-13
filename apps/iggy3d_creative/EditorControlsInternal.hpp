#pragma once

#include "EditorControls.hpp"

#include <span>
#include <string>

namespace iggy3d_creative_app {

inline constexpr iggy3d::creative::CreativeUiWidgetId
    kControlRowWidgetIdBase = 1U;
inline constexpr iggy3d::creative::CreativeUiWidgetId
    kControlsResetWidgetId = 1000U;
inline constexpr iggy3d::creative::CreativeUiWidgetId
    kControlsDoneWidgetId = 1001U;
inline constexpr iggy3d::creative::CreativeUiWidgetId
    kControlsKeyboardTabWidgetId = 1002U;
inline constexpr iggy3d::creative::CreativeUiWidgetId
    kControlsPs5TabWidgetId = 1003U;
inline constexpr iggy3d::creative::CreativeUiWidgetId
    kControlsResetWheelWidgetId = 1004U;

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

[[nodiscard]] ControlsLayout controlsLayout(
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight);
[[nodiscard]] std::span<const iggy3d::creative::CreativeControlSettingId>
settingsForDevice(
    iggy3d::creative::CreativeControlDevice device) noexcept;
[[nodiscard]] std::size_t totalRowCount(
    const CreativeEditorControlsState& state) noexcept;
[[nodiscard]] iggy3d::creative::CreativeUiWidgetId rowWidgetId(
    std::size_t row) noexcept;
[[nodiscard]] bool widgetRowIndex(
    iggy3d::creative::CreativeUiWidgetId widgetId,
    std::size_t rowCount,
    std::size_t& row) noexcept;
void refreshBindingList(CreativeEditorState& editor);
void keepFocusedRowVisible(
    CreativeEditorControlsState& state,
    std::size_t visibleRows) noexcept;
[[nodiscard]] iggy3d::creative::CreativeControlBindingRow*
selectedBindingRow(
    CreativeEditorControlsState& state) noexcept;
[[nodiscard]] bool selectedSetting(
    const CreativeEditorControlsState& state,
    iggy3d::creative::CreativeControlSettingId& setting) noexcept;
void cycleConflictPolicy(
    CreativeEditorControlsState& state,
    std::int32_t direction) noexcept;
[[nodiscard]] iggy3d::creative::CreativeUiWidgetFrame
buildControlsWidgetFrame(
    const CreativeEditorState& editor,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight);

}  // namespace iggy3d_creative_app
