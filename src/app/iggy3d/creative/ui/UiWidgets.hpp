#pragma once

#include "app/iggy3d/creative/input/UiInput.hpp"
#include "app/iggy3d/creative/ui/UiTheme.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace iggy3d::creative {

using CreativeUiWidgetId = std::uint32_t;

inline constexpr CreativeUiWidgetId kInvalidCreativeUiWidgetId = 0U;
inline constexpr std::size_t kCreativeUiWidgetCapacity = 256U;
inline constexpr std::size_t kCreativeUiWidgetVisualCapacity = 256U;
inline constexpr std::size_t kCreativeUiWidgetTextCapacity = 96U;

enum class CreativeUiWidgetKind : std::uint8_t {
  Button,
  ListRow,
  Stepper,
  Toggle,
  Tab,
  TextField,
  Count,
};

enum class CreativeUiWidgetVisualKind : std::uint8_t {
  Rect,
  Text,
  Count,
};

enum class CreativeUiWidgetEventKind : std::uint8_t {
  None,
  Activate,
  Decrease,
  Increase,
  Count,
};

enum class CreativeUiRepeatCommand : std::uint8_t {
  None,
  Previous,
  Next,
  Decrease,
  Increase,
  Count,
};

struct CreativeUiFixedText {
  std::array<char, kCreativeUiWidgetTextCapacity> bytes{};
  std::uint16_t length = 0U;

  [[nodiscard]] std::string_view view() const noexcept {
    return {bytes.data(), length};
  }
};

struct CreativeUiWidgetVisual {
  CreativeUiWidgetVisualKind kind = CreativeUiWidgetVisualKind::Rect;
  CreativeUiTone tone = CreativeUiTone::Surface;
  CreativeUiRect rect{};
  CreativeUiFixedText text{};
  float opacity = 1.0F;
};

struct CreativeUiWidgetRecord {
  CreativeUiWidgetId id = kInvalidCreativeUiWidgetId;
  CreativeUiWidgetKind kind = CreativeUiWidgetKind::Button;
  CreativeUiRect focusRect{};
  CreativeUiRect activateRect{};
  CreativeUiRect decreaseRect{};
  CreativeUiRect increaseRect{};
  bool enabled = true;
  bool visible = true;
  bool bodyActivates = true;
  bool adjustable = false;
};

struct CreativeUiWidgetFrame {
  std::uint32_t virtualWidth = 0U;
  std::uint32_t virtualHeight = 0U;
  std::array<CreativeUiWidgetRecord, kCreativeUiWidgetCapacity> widgets{};
  std::array<CreativeUiWidgetVisual, kCreativeUiWidgetVisualCapacity>
      visuals{};
  std::size_t widgetCount = 0U;
  std::size_t visualCount = 0U;
  bool capacityExceeded = false;
  bool invalidInput = false;
  bool textTruncated = false;

  [[nodiscard]] std::span<const CreativeUiWidgetRecord>
  widgetItems() const noexcept {
    return {widgets.data(), widgetCount};
  }

  [[nodiscard]] std::span<const CreativeUiWidgetVisual>
  visualItems() const noexcept {
    return {visuals.data(), visualCount};
  }
};

struct CreativeUiWidgetSpec {
  CreativeUiWidgetId id = kInvalidCreativeUiWidgetId;
  CreativeUiRect rect{};
  std::string_view label;
  std::string_view value;
  bool enabled = true;
  bool selected = false;
  bool focused = false;
  bool visible = true;
};

struct CreativeUiWidgetRouteInput {
  CreativeUiWidgetId focusedId = kInvalidCreativeUiWidgetId;
  CreativeDrawablePointer pointer{};
  std::int64_t focusSteps = 0;
  bool activate = false;
  bool decrease = false;
  bool increase = false;
};

struct CreativeUiWidgetEvent {
  CreativeUiWidgetId widgetId = kInvalidCreativeUiWidgetId;
  CreativeUiWidgetEventKind kind = CreativeUiWidgetEventKind::None;
  bool fromPointer = false;
};

struct CreativeUiWidgetRouteResult {
  CreativeUiWidgetId focusedId = kInvalidCreativeUiWidgetId;
  CreativeUiWidgetEvent event{};
  bool focusChanged = false;
  bool pointerHandled = false;
  bool valid = false;
};

struct CreativeUiVisibleRange {
  std::size_t first = 0U;
  std::size_t count = 0U;
  bool valid = false;
};

struct CreativeUiRepeatState {
  CreativeUiRepeatCommand command = CreativeUiRepeatCommand::None;
  std::uint64_t nextFireNanoseconds = 0U;
};

struct CreativeUiRepeatRequest {
  CreativeUiRepeatState previous{};
  CreativeUiRepeatCommand command = CreativeUiRepeatCommand::None;
  std::uint64_t monotonicTimeNanoseconds = 0U;
  std::uint16_t delayMilliseconds = 320U;
  std::uint16_t intervalMilliseconds = 110U;
};

struct CreativeUiRepeatResult {
  CreativeUiRepeatState next{};
  CreativeUiRepeatCommand firedCommand = CreativeUiRepeatCommand::None;
  bool fired = false;
  bool valid = false;
};

void resetCreativeUiWidgetFrame(
    CreativeUiWidgetFrame& frame,
    std::uint32_t virtualWidth,
    std::uint32_t virtualHeight) noexcept;

[[nodiscard]] bool appendCreativeUiScrim(
    CreativeUiWidgetFrame& frame,
    float opacity = 0.72F) noexcept;
[[nodiscard]] bool appendCreativeUiPanel(
    CreativeUiWidgetFrame& frame,
    CreativeUiRect rect) noexcept;
[[nodiscard]] bool appendCreativeUiLabel(
    CreativeUiWidgetFrame& frame,
    CreativeUiRect rect,
    std::string_view text,
    CreativeUiTone tone = CreativeUiTone::TextPrimary) noexcept;
[[nodiscard]] bool appendCreativeUiButton(
    CreativeUiWidgetFrame& frame,
    const CreativeUiWidgetSpec& spec) noexcept;
[[nodiscard]] bool appendCreativeUiListRow(
    CreativeUiWidgetFrame& frame,
    const CreativeUiWidgetSpec& spec) noexcept;
[[nodiscard]] bool appendCreativeUiStepper(
    CreativeUiWidgetFrame& frame,
    const CreativeUiWidgetSpec& spec) noexcept;
[[nodiscard]] bool appendCreativeUiToggle(
    CreativeUiWidgetFrame& frame,
    const CreativeUiWidgetSpec& spec) noexcept;
[[nodiscard]] bool appendCreativeUiTab(
    CreativeUiWidgetFrame& frame,
    const CreativeUiWidgetSpec& spec) noexcept;
[[nodiscard]] bool appendCreativeUiTextField(
    CreativeUiWidgetFrame& frame,
    const CreativeUiWidgetSpec& spec) noexcept;

[[nodiscard]] CreativeUiWidgetRouteResult routeCreativeUiWidgets(
    const CreativeUiWidgetFrame& frame,
    const CreativeUiWidgetRouteInput& input) noexcept;

[[nodiscard]] CreativeUiVisibleRange resolveCreativeUiVisibleRange(
    std::size_t totalCount,
    std::size_t visibleCount,
    std::size_t offset) noexcept;
[[nodiscard]] std::size_t keepCreativeUiListIndexVisible(
    std::size_t index,
    std::size_t totalCount,
    std::size_t visibleCount,
    std::size_t offset) noexcept;

[[nodiscard]] CreativeUiRepeatResult stepCreativeUiRepeat(
    const CreativeUiRepeatRequest& request) noexcept;

}  // namespace iggy3d::creative
