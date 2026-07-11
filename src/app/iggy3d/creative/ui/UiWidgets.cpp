#include "app/iggy3d/creative/ui/UiWidgets.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <type_traits>

namespace iggy3d::creative {
namespace {

constexpr float kWidgetTextInset = 12.0F;
constexpr float kWidgetControlInset = 4.0F;
constexpr float kWidgetControlGap = 4.0F;
constexpr float kWidgetControlWidth = 40.0F;

[[nodiscard]] bool validRect(const CreativeUiRect& rect) noexcept {
  return std::isfinite(rect.x) && std::isfinite(rect.y) &&
         std::isfinite(rect.width) && std::isfinite(rect.height) &&
         rect.width >= 0.0F && rect.height >= 0.0F;
}

[[nodiscard]] bool visibleRect(const CreativeUiRect& rect) noexcept {
  return validRect(rect) && rect.width > 0.0F && rect.height > 0.0F;
}

[[nodiscard]] bool contains(const CreativeUiRect& rect,
                            float x,
                            float y) noexcept {
  return visibleRect(rect) && std::isfinite(x) && std::isfinite(y) &&
         x >= rect.x && y >= rect.y && x < rect.x + rect.width &&
         y < rect.y + rect.height;
}

[[nodiscard]] CreativeUiRect insetRect(CreativeUiRect rect,
                                       float inset) noexcept {
  rect.x += inset;
  rect.y += inset;
  rect.width = std::max(0.0F, rect.width - inset * 2.0F);
  rect.height = std::max(0.0F, rect.height - inset * 2.0F);
  return rect;
}

[[nodiscard]] CreativeUiRect textRect(CreativeUiRect rect,
                                      float xInset) noexcept {
  return {rect.x + xInset,
          rect.y + std::max(0.0F, (rect.height - 8.0F) * 0.5F),
          0.0F,
          0.0F};
}

[[nodiscard]] CreativeUiTone backgroundTone(
    const CreativeUiWidgetSpec& spec) noexcept {
  if (!spec.enabled) {
    return CreativeUiTone::Disabled;
  }
  return spec.selected || spec.focused ? CreativeUiTone::Selected
                                       : CreativeUiTone::SurfaceRaised;
}

[[nodiscard]] CreativeUiTone textTone(
    const CreativeUiWidgetSpec& spec) noexcept {
  return spec.enabled ? CreativeUiTone::TextPrimary
                      : CreativeUiTone::TextMuted;
}

void copyText(CreativeUiWidgetFrame& frame,
              CreativeUiFixedText& destination,
              std::string_view source) noexcept {
  const std::size_t count =
      std::min(source.size(), destination.bytes.size());
  std::copy_n(source.begin(), count, destination.bytes.begin());
  destination.length = static_cast<std::uint16_t>(count);
  if (source.size() > count) {
    frame.textTruncated = true;
    if (count >= 3U) {
      destination.bytes[count - 3U] = '.';
      destination.bytes[count - 2U] = '.';
      destination.bytes[count - 1U] = '.';
    }
  }
}

[[nodiscard]] bool reserveFrame(CreativeUiWidgetFrame& frame,
                                std::size_t widgetCount,
                                std::size_t visualCount) noexcept {
  if (frame.widgetCount + widgetCount > frame.widgets.size() ||
      frame.visualCount + visualCount > frame.visuals.size()) {
    frame.capacityExceeded = true;
    return false;
  }
  return true;
}

[[nodiscard]] bool appendVisual(CreativeUiWidgetFrame& frame,
                                CreativeUiWidgetVisualKind kind,
                                CreativeUiTone tone,
                                CreativeUiRect rect,
                                std::string_view text,
                                float opacity = 1.0F) noexcept {
  if (!validRect(rect) || !std::isfinite(opacity) || opacity < 0.0F ||
      opacity > 1.0F) {
    frame.invalidInput = true;
    return false;
  }
  if (!reserveFrame(frame, 0U, 1U)) {
    return false;
  }
  CreativeUiWidgetVisual& visual = frame.visuals[frame.visualCount++];
  visual.kind = kind;
  visual.tone = tone;
  visual.rect = rect;
  visual.opacity = opacity;
  if (kind == CreativeUiWidgetVisualKind::Text) {
    copyText(frame, visual.text, text);
  }
  return true;
}

[[nodiscard]] bool widgetIdAvailable(const CreativeUiWidgetFrame& frame,
                                     CreativeUiWidgetId id) noexcept {
  if (id == kInvalidCreativeUiWidgetId) {
    return false;
  }
  return std::none_of(
      frame.widgets.begin(), frame.widgets.begin() + frame.widgetCount,
      [id](const CreativeUiWidgetRecord& widget) {
        return widget.id == id;
      });
}

[[nodiscard]] CreativeUiWidgetRecord* appendWidgetRecord(
    CreativeUiWidgetFrame& frame,
    const CreativeUiWidgetSpec& spec,
    CreativeUiWidgetKind kind,
    bool bodyActivates,
    bool adjustable) noexcept {
  if (!widgetIdAvailable(frame, spec.id) ||
      !validRect(spec.rect) || kind == CreativeUiWidgetKind::Count) {
    frame.invalidInput = true;
    return nullptr;
  }
  if (!reserveFrame(frame, 1U, 0U)) {
    return nullptr;
  }
  CreativeUiWidgetRecord& record = frame.widgets[frame.widgetCount++];
  record.id = spec.id;
  record.kind = kind;
  record.focusRect = spec.rect;
  record.enabled = spec.enabled;
  record.visible = spec.visible;
  record.bodyActivates = bodyActivates;
  record.adjustable = adjustable;
  return &record;
}

[[nodiscard]] bool appendRowBase(CreativeUiWidgetFrame& frame,
                                 const CreativeUiWidgetSpec& spec,
                                 CreativeUiWidgetRecord& record) noexcept {
  if (!spec.visible) {
    return true;
  }
  if (!appendVisual(frame, CreativeUiWidgetVisualKind::Rect,
                    backgroundTone(spec), spec.rect, {}) ||
      !appendVisual(frame, CreativeUiWidgetVisualKind::Text, textTone(spec),
                    textRect(spec.rect, kWidgetTextInset), spec.label)) {
    return false;
  }
  record.visible = true;
  return true;
}

[[nodiscard]] bool appendValueText(CreativeUiWidgetFrame& frame,
                                   const CreativeUiWidgetSpec& spec,
                                   float x) noexcept {
  if (!spec.visible || spec.value.empty()) {
    return true;
  }
  return appendVisual(
      frame, CreativeUiWidgetVisualKind::Text,
      spec.enabled ? CreativeUiTone::Accent : CreativeUiTone::TextMuted,
      {x, spec.rect.y + std::max(0.0F, (spec.rect.height - 8.0F) * 0.5F),
       0.0F, 0.0F},
      spec.value);
}

[[nodiscard]] bool supportsAdjustment(
    const CreativeUiWidgetRecord& widget) noexcept {
  return widget.adjustable &&
         (widget.kind == CreativeUiWidgetKind::Stepper ||
          widget.kind == CreativeUiWidgetKind::Toggle);
}

[[nodiscard]] const CreativeUiWidgetRecord* widgetWithId(
    const CreativeUiWidgetFrame& frame,
    CreativeUiWidgetId id) noexcept {
  const auto found = std::find_if(
      frame.widgets.begin(), frame.widgets.begin() + frame.widgetCount,
      [id](const CreativeUiWidgetRecord& widget) {
        return widget.id == id && widget.enabled;
      });
  return found == frame.widgets.begin() + frame.widgetCount ? nullptr
                                                            : &*found;
}

[[nodiscard]] std::uint64_t saturatingNanoseconds(
    std::uint16_t milliseconds) noexcept {
  constexpr std::uint64_t kNanosecondsPerMillisecond = 1000000ULL;
  return static_cast<std::uint64_t>(milliseconds) *
         kNanosecondsPerMillisecond;
}

[[nodiscard]] std::uint64_t saturatingAdd(std::uint64_t lhs,
                                          std::uint64_t rhs) noexcept {
  return lhs > std::numeric_limits<std::uint64_t>::max() - rhs
             ? std::numeric_limits<std::uint64_t>::max()
             : lhs + rhs;
}

}  // namespace

static_assert(std::is_trivially_copyable_v<CreativeUiFixedText>);
static_assert(std::is_trivially_copyable_v<CreativeUiWidgetVisual>);
static_assert(std::is_trivially_copyable_v<CreativeUiWidgetRecord>);
static_assert(std::is_trivially_copyable_v<CreativeUiWidgetFrame>);
static_assert(std::is_trivially_copyable_v<CreativeUiRepeatState>);

void resetCreativeUiWidgetFrame(CreativeUiWidgetFrame& frame,
                                std::uint32_t virtualWidth,
                                std::uint32_t virtualHeight) noexcept {
  frame = {};
  frame.virtualWidth = virtualWidth;
  frame.virtualHeight = virtualHeight;
  if (virtualWidth == 0U || virtualHeight == 0U) {
    frame.invalidInput = true;
  }
}

bool appendCreativeUiScrim(CreativeUiWidgetFrame& frame,
                           float opacity) noexcept {
  return appendVisual(
      frame, CreativeUiWidgetVisualKind::Rect, CreativeUiTone::Surface,
      {0.0F, 0.0F, static_cast<float>(frame.virtualWidth),
       static_cast<float>(frame.virtualHeight)},
      {}, opacity);
}

bool appendCreativeUiPanel(CreativeUiWidgetFrame& frame,
                           CreativeUiRect rect) noexcept {
  if (!reserveFrame(frame, 0U, 2U)) {
    return false;
  }
  return appendVisual(frame, CreativeUiWidgetVisualKind::Rect,
                      CreativeUiTone::Border, rect, {}) &&
         appendVisual(frame, CreativeUiWidgetVisualKind::Rect,
                      CreativeUiTone::Surface, insetRect(rect, 1.0F), {});
}

bool appendCreativeUiLabel(CreativeUiWidgetFrame& frame,
                           CreativeUiRect rect,
                           std::string_view text,
                           CreativeUiTone tone) noexcept {
  return appendVisual(frame, CreativeUiWidgetVisualKind::Text, tone, rect,
                      text);
}

bool appendCreativeUiButton(CreativeUiWidgetFrame& frame,
                            const CreativeUiWidgetSpec& spec) noexcept {
  if (!reserveFrame(frame, 1U, spec.visible ? 2U : 0U)) {
    return false;
  }
  CreativeUiWidgetRecord* record = appendWidgetRecord(
      frame, spec, CreativeUiWidgetKind::Button, true, false);
  if (record == nullptr) {
    return false;
  }
  record->activateRect = spec.rect;
  return appendRowBase(frame, spec, *record);
}

bool appendCreativeUiListRow(CreativeUiWidgetFrame& frame,
                             const CreativeUiWidgetSpec& spec) noexcept {
  if (!reserveFrame(frame, 1U, spec.visible ? 3U : 0U)) {
    return false;
  }
  CreativeUiWidgetRecord* record = appendWidgetRecord(
      frame, spec, CreativeUiWidgetKind::ListRow, true, false);
  if (record == nullptr) {
    return false;
  }
  record->activateRect = {spec.rect.x + spec.rect.width * 0.5F,
                          spec.rect.y, spec.rect.width * 0.5F,
                          spec.rect.height};
  return appendRowBase(frame, spec, *record) &&
         appendValueText(frame, spec, record->activateRect.x + 8.0F);
}

bool appendCreativeUiStepper(CreativeUiWidgetFrame& frame,
                             const CreativeUiWidgetSpec& spec) noexcept {
  if (!reserveFrame(frame, 1U, spec.visible ? 7U : 0U)) {
    return false;
  }
  CreativeUiWidgetRecord* record = appendWidgetRecord(
      frame, spec, CreativeUiWidgetKind::Stepper, true, true);
  if (record == nullptr) {
    return false;
  }
  const float buttonWidth = std::min(
      kWidgetControlWidth,
      std::max(1.0F, spec.rect.width * 0.18F));
  record->increaseRect = {
      spec.rect.x + spec.rect.width - buttonWidth - kWidgetControlInset,
      spec.rect.y + kWidgetControlInset,
      buttonWidth,
      std::max(0.0F, spec.rect.height - kWidgetControlInset * 2.0F)};
  record->decreaseRect = {
      record->increaseRect.x - buttonWidth - kWidgetControlGap,
      record->increaseRect.y,
      buttonWidth,
      record->increaseRect.height};
  const float valueX = spec.rect.x + spec.rect.width * 0.5F;
  record->activateRect = {
      valueX,
      spec.rect.y,
      std::max(0.0F, record->decreaseRect.x - kWidgetControlGap - valueX),
      spec.rect.height};
  if (!appendRowBase(frame, spec, *record) ||
      !appendValueText(frame, spec, valueX + 8.0F)) {
    return false;
  }
  if (!spec.visible) {
    return true;
  }
  const CreativeUiTone controlTone = spec.enabled
                                         ? CreativeUiTone::SurfaceRaised
                                         : CreativeUiTone::Disabled;
  return appendVisual(frame, CreativeUiWidgetVisualKind::Rect, controlTone,
                      record->decreaseRect, {}) &&
         appendVisual(frame, CreativeUiWidgetVisualKind::Text, textTone(spec),
                      textRect(record->decreaseRect, 14.0F), "-") &&
         appendVisual(frame, CreativeUiWidgetVisualKind::Rect, controlTone,
                      record->increaseRect, {}) &&
         appendVisual(frame, CreativeUiWidgetVisualKind::Text, textTone(spec),
                      textRect(record->increaseRect, 14.0F), "+");
}

bool appendCreativeUiToggle(CreativeUiWidgetFrame& frame,
                            const CreativeUiWidgetSpec& spec) noexcept {
  if (!reserveFrame(frame, 1U, spec.visible ? 3U : 0U)) {
    return false;
  }
  CreativeUiWidgetRecord* record = appendWidgetRecord(
      frame, spec, CreativeUiWidgetKind::Toggle, true, true);
  if (record == nullptr) {
    return false;
  }
  record->activateRect = spec.rect;
  return appendRowBase(frame, spec, *record) &&
         appendValueText(frame, spec,
                         spec.rect.x + spec.rect.width * 0.5F + 8.0F);
}

bool appendCreativeUiTab(CreativeUiWidgetFrame& frame,
                         const CreativeUiWidgetSpec& spec) noexcept {
  if (!reserveFrame(frame, 1U, spec.visible ? 2U : 0U)) {
    return false;
  }
  CreativeUiWidgetRecord* record = appendWidgetRecord(
      frame, spec, CreativeUiWidgetKind::Tab, true, false);
  if (record == nullptr) {
    return false;
  }
  record->activateRect = spec.rect;
  return appendRowBase(frame, spec, *record);
}

bool appendCreativeUiTextField(CreativeUiWidgetFrame& frame,
                               const CreativeUiWidgetSpec& spec) noexcept {
  if (!reserveFrame(frame, 1U, spec.visible ? 3U : 0U)) {
    return false;
  }
  CreativeUiWidgetRecord* record = appendWidgetRecord(
      frame, spec, CreativeUiWidgetKind::TextField, true, false);
  if (record == nullptr) {
    return false;
  }
  record->activateRect = spec.rect;
  if (!spec.visible) {
    return true;
  }
  return appendVisual(frame, CreativeUiWidgetVisualKind::Rect,
                      CreativeUiTone::Border, spec.rect, {}) &&
         appendVisual(frame, CreativeUiWidgetVisualKind::Rect,
                      backgroundTone(spec), insetRect(spec.rect, 1.0F), {}) &&
         appendVisual(frame, CreativeUiWidgetVisualKind::Text, textTone(spec),
                      textRect(spec.rect, kWidgetTextInset), spec.value);
}

CreativeUiWidgetRouteResult routeCreativeUiWidgets(
    const CreativeUiWidgetFrame& frame,
    const CreativeUiWidgetRouteInput& input) noexcept {
  CreativeUiWidgetRouteResult result;
  result.focusedId = input.focusedId;
  if (frame.invalidInput || frame.capacityExceeded || frame.widgetCount == 0U ||
      (input.pointer.valid &&
       (!std::isfinite(input.pointer.x) ||
        !std::isfinite(input.pointer.y)))) {
    return result;
  }
  result.valid = true;

  if (input.pointer.valid && input.pointer.primaryPressed) {
    for (std::size_t index = frame.widgetCount; index > 0U; --index) {
      const CreativeUiWidgetRecord& widget = frame.widgets[index - 1U];
      if (!widget.enabled || !widget.visible ||
          !contains(widget.focusRect, input.pointer.x, input.pointer.y)) {
        continue;
      }
      result.pointerHandled = true;
      result.focusChanged = result.focusedId != widget.id;
      result.focusedId = widget.id;
      result.event.widgetId = widget.id;
      result.event.fromPointer = true;
      if (contains(widget.decreaseRect, input.pointer.x, input.pointer.y)) {
        result.event.kind = CreativeUiWidgetEventKind::Decrease;
      } else if (contains(widget.increaseRect, input.pointer.x,
                          input.pointer.y)) {
        result.event.kind = CreativeUiWidgetEventKind::Increase;
      } else if (widget.bodyActivates &&
                 contains(widget.activateRect, input.pointer.x,
                          input.pointer.y)) {
        result.event.kind = CreativeUiWidgetEventKind::Activate;
      }
      return result;
    }
  }

  std::array<std::size_t, kCreativeUiWidgetCapacity> focusable{};
  std::size_t focusableCount = 0U;
  std::size_t currentFocus = 0U;
  bool currentFound = false;
  for (std::size_t index = 0U; index < frame.widgetCount; ++index) {
    if (!frame.widgets[index].enabled) {
      continue;
    }
    if (frame.widgets[index].id == result.focusedId) {
      currentFocus = focusableCount;
      currentFound = true;
    }
    focusable[focusableCount++] = index;
  }
  if (focusableCount == 0U) {
    return result;
  }

  if (input.focusSteps != 0) {
    if (!currentFound) {
      currentFocus = input.focusSteps > 0 ? focusableCount - 1U : 0U;
    }
    const CreativeWrappedIndexResult next = stepCreativeWrappedIndex(
        currentFocus, focusableCount, input.focusSteps);
    if (next.valid) {
      const CreativeUiWidgetId nextId =
          frame.widgets[focusable[next.index]].id;
      result.focusChanged = result.focusedId != nextId;
      result.focusedId = nextId;
      currentFocus = next.index;
      currentFound = true;
    }
  }

  const CreativeUiWidgetRecord* focused =
      widgetWithId(frame, result.focusedId);
  if (focused == nullptr) {
    return result;
  }
  result.event.widgetId = focused->id;
  if (input.activate) {
    result.event.kind = CreativeUiWidgetEventKind::Activate;
  } else if (input.decrease != input.increase &&
             supportsAdjustment(*focused)) {
    result.event.kind = input.decrease
                            ? CreativeUiWidgetEventKind::Decrease
                            : CreativeUiWidgetEventKind::Increase;
  }
  return result;
}

CreativeUiVisibleRange resolveCreativeUiVisibleRange(
    std::size_t totalCount,
    std::size_t visibleCount,
    std::size_t offset) noexcept {
  CreativeUiVisibleRange result;
  if (totalCount == 0U || visibleCount == 0U) {
    return result;
  }
  result.first = std::min(offset,
                          totalCount > visibleCount
                              ? totalCount - visibleCount
                              : 0U);
  result.count = std::min(visibleCount, totalCount - result.first);
  result.valid = true;
  return result;
}

std::size_t keepCreativeUiListIndexVisible(std::size_t index,
                                           std::size_t totalCount,
                                           std::size_t visibleCount,
                                           std::size_t offset) noexcept {
  if (totalCount == 0U || visibleCount == 0U) {
    return 0U;
  }
  index = std::min(index, totalCount - 1U);
  const std::size_t maximumOffset =
      totalCount > visibleCount ? totalCount - visibleCount : 0U;
  offset = std::min(offset, maximumOffset);
  if (index < offset) {
    return index;
  }
  if (index >= offset + visibleCount) {
    return std::min(index - visibleCount + 1U, maximumOffset);
  }
  return offset;
}

CreativeUiRepeatResult stepCreativeUiRepeat(
    const CreativeUiRepeatRequest& request) noexcept {
  CreativeUiRepeatResult result;
  result.next = request.previous;
  if (request.command == CreativeUiRepeatCommand::Count ||
      request.previous.command == CreativeUiRepeatCommand::Count ||
      request.delayMilliseconds == 0U ||
      request.intervalMilliseconds == 0U) {
    return result;
  }
  result.valid = true;
  if (request.command == CreativeUiRepeatCommand::None) {
    result.next = {};
    return result;
  }
  if (request.previous.command != request.command ||
      request.previous.command == CreativeUiRepeatCommand::None) {
    result.next.command = request.command;
    result.next.nextFireNanoseconds = saturatingAdd(
        request.monotonicTimeNanoseconds,
        saturatingNanoseconds(request.delayMilliseconds));
    result.firedCommand = request.command;
    result.fired = true;
    return result;
  }
  if (request.monotonicTimeNanoseconds >=
      request.previous.nextFireNanoseconds) {
    result.next.nextFireNanoseconds = saturatingAdd(
        request.monotonicTimeNanoseconds,
        saturatingNanoseconds(request.intervalMilliseconds));
    result.firedCommand = request.command;
    result.fired = true;
  }
  return result;
}

}  // namespace iggy3d::creative
