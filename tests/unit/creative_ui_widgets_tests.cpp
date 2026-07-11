#include "app/iggy3d/creative/render/CreativeOverlayFrame.hpp"
#include "app/iggy3d/creative/ui/UiWidgets.hpp"

#include <algorithm>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

const cr::CreativeUiWidgetRecord* findWidget(
    const cr::CreativeUiWidgetFrame& frame,
    cr::CreativeUiWidgetId id) {
  const auto found = std::find_if(
      frame.widgetItems().begin(), frame.widgetItems().end(),
      [id](const cr::CreativeUiWidgetRecord& widget) {
        return widget.id == id;
      });
  return found == frame.widgetItems().end() ? nullptr : &*found;
}

bool standardWidgetsEmitBoundedVisualsAndRender() {
  cr::CreativeUiWidgetFrame frame;
  cr::resetCreativeUiWidgetFrame(frame, 800U, 600U);
  bool appended = cr::appendCreativeUiScrim(frame) &&
                  cr::appendCreativeUiPanel(
                      frame, {20.0F, 20.0F, 500.0F, 500.0F}) &&
                  cr::appendCreativeUiLabel(
                      frame, {40.0F, 40.0F, 0.0F, 0.0F}, "STANDARD UI");
  cr::CreativeUiWidgetSpec spec;
  spec.rect = {40.0F, 80.0F, 420.0F, 32.0F};
  spec.id = 1U;
  spec.label = "Button";
  appended = cr::appendCreativeUiButton(frame, spec) && appended;
  spec.id = 2U;
  spec.rect.y += 36.0F;
  spec.label = "List row";
  spec.value = "VALUE";
  appended = cr::appendCreativeUiListRow(frame, spec) && appended;
  spec.id = 3U;
  spec.rect.y += 36.0F;
  spec.label = "Stepper";
  spec.value = "4";
  appended = cr::appendCreativeUiStepper(frame, spec) && appended;
  spec.id = 4U;
  spec.rect.y += 36.0F;
  spec.label = "Toggle";
  spec.value = "ON";
  appended = cr::appendCreativeUiToggle(frame, spec) && appended;
  spec.id = 5U;
  spec.rect.y += 36.0F;
  spec.label = "Tab";
  spec.value = {};
  spec.selected = true;
  appended = cr::appendCreativeUiTab(frame, spec) && appended;
  spec.id = 6U;
  spec.rect.y += 36.0F;
  spec.label = {};
  spec.value = "Search";
  spec.selected = false;
  appended = cr::appendCreativeUiTextField(frame, spec) && appended;

  std::vector<iggy3d::RenderUiRect> rects;
  std::vector<iggy3d::DebugHudGlyphQuad> glyphs;
  const iggy3d::CreativeUiWidgetOverlayAppendReceipt overlay =
      iggy3d::appendCreativeUiWidgetOverlay(frame, 800U, 600U, rects, glyphs);
  const iggy3d::CreativeUiColor surface = iggy3d::creativeUiToneColor(
      iggy3d::CreativeUiTone::Surface, iggy3d::creativeUiTheme());
  const iggy3d::CreativeUiColor invalidTone = iggy3d::creativeUiToneColor(
      static_cast<iggy3d::CreativeUiTone>(255U),
      iggy3d::creativeUiTheme());

  return expect(appended && !frame.invalidInput &&
                    !frame.capacityExceeded && !frame.textTruncated,
                "standard widget set emits without degradation") &&
         expect(frame.widgetCount == 6U && frame.visualCount == 24U,
                "widget and visual counts are deterministic") &&
         expect(overlay.accepted && overlay.ready && !overlay.partial &&
                    overlay.rectCount == rects.size() &&
                    overlay.glyphQuadCount == glyphs.size() &&
                    !rects.empty() && !glyphs.empty(),
                "bounded widget frame reaches the existing overlay renderer") &&
         expect(invalidTone.r == surface.r && invalidTone.g == surface.g &&
                    invalidTone.b == surface.b && invalidTone.a == surface.a,
                "invalid tone fails closed to the system surface");
}

bool pointerPartsAndFocusNavigationAreDeterministic() {
  cr::CreativeUiWidgetFrame frame;
  cr::resetCreativeUiWidgetFrame(frame, 640U, 480U);
  cr::CreativeUiWidgetSpec spec;
  spec.id = 1U;
  spec.rect = {20.0F, 20.0F, 400.0F, 32.0F};
  spec.label = "Hidden";
  spec.visible = false;
  static_cast<void>(cr::appendCreativeUiListRow(frame, spec));
  spec.id = 2U;
  spec.rect.y = 60.0F;
  spec.label = "Row";
  spec.value = "BIND";
  spec.visible = true;
  static_cast<void>(cr::appendCreativeUiListRow(frame, spec));
  spec.id = 3U;
  spec.rect.y = 100.0F;
  spec.label = "Step";
  spec.value = "2";
  static_cast<void>(cr::appendCreativeUiStepper(frame, spec));
  spec.id = 4U;
  spec.rect.y = 140.0F;
  spec.label = "Done";
  spec.value = {};
  static_cast<void>(cr::appendCreativeUiButton(frame, spec));

  cr::CreativeUiWidgetRouteInput navigation;
  navigation.focusedId = 1U;
  navigation.focusSteps = 1;
  const cr::CreativeUiWidgetRouteResult navigated =
      cr::routeCreativeUiWidgets(frame, navigation);
  navigation.pointer.x = std::numeric_limits<float>::quiet_NaN();
  navigation.pointer.valid = false;
  const cr::CreativeUiWidgetRouteResult ignoredInvalidPointer =
      cr::routeCreativeUiWidgets(frame, navigation);

  const cr::CreativeUiWidgetRecord* stepper = findWidget(frame, 3U);
  if (stepper == nullptr) {
    return expect(false, "stepper record exists");
  }
  cr::CreativeUiWidgetRouteInput decrement;
  decrement.focusedId = 2U;
  decrement.pointer = {
      stepper->decreaseRect.x + stepper->decreaseRect.width * 0.5F,
      stepper->decreaseRect.y + stepper->decreaseRect.height * 0.5F,
      0.0F,
      0.0F,
      true,
      false,
      true};
  const cr::CreativeUiWidgetRouteResult decremented =
      cr::routeCreativeUiWidgets(frame, decrement);

  cr::CreativeUiWidgetRouteInput rowLabel;
  rowLabel.focusedId = 3U;
  rowLabel.pointer = {30.0F, 70.0F, 0.0F, 0.0F, true, false, true};
  const cr::CreativeUiWidgetRouteResult labelClick =
      cr::routeCreativeUiWidgets(frame, rowLabel);
  rowLabel.pointer.x = 300.0F;
  const cr::CreativeUiWidgetRouteResult valueClick =
      cr::routeCreativeUiWidgets(frame, rowLabel);

  cr::CreativeUiWidgetRouteInput simultaneous;
  simultaneous.focusedId = 3U;
  simultaneous.decrease = true;
  simultaneous.increase = true;
  const cr::CreativeUiWidgetRouteResult cancelledAdjust =
      cr::routeCreativeUiWidgets(frame, simultaneous);

  return expect(navigated.valid && navigated.focusChanged &&
                    navigated.focusedId == 2U,
                "focus walks hidden and visible widget records in order") &&
         expect(ignoredInvalidPointer.valid &&
                    ignoredInvalidPointer.focusedId == 2U,
                "invalid inactive pointer does not poison keyboard focus") &&
         expect(decremented.pointerHandled &&
                    decremented.focusedId == 3U &&
                    decremented.event.kind ==
                        cr::CreativeUiWidgetEventKind::Decrease &&
                    decremented.event.fromPointer,
                "stepper decrement owns its pointer region") &&
         expect(labelClick.pointerHandled && labelClick.focusedId == 2U &&
                    labelClick.event.kind ==
                        cr::CreativeUiWidgetEventKind::None,
                "list label focuses without activating") &&
         expect(valueClick.event.kind ==
                    cr::CreativeUiWidgetEventKind::Activate,
                "list value activates the row") &&
         expect(cancelledAdjust.event.kind ==
                    cr::CreativeUiWidgetEventKind::None,
                "simultaneous decrease and increase cancel deterministically");
}

bool scrollingAndRepeatTimingArePinned() {
  const cr::CreativeUiVisibleRange range =
      cr::resolveCreativeUiVisibleRange(100U, 10U, 95U);
  const std::size_t movedUp =
      cr::keepCreativeUiListIndexVisible(2U, 100U, 10U, 90U);
  const std::size_t movedDown =
      cr::keepCreativeUiListIndexVisible(50U, 100U, 10U, 0U);

  cr::CreativeUiRepeatRequest request;
  request.command = cr::CreativeUiRepeatCommand::Next;
  request.monotonicTimeNanoseconds = 0U;
  request.delayMilliseconds = 320U;
  request.intervalMilliseconds = 110U;
  const cr::CreativeUiRepeatResult first = cr::stepCreativeUiRepeat(request);
  request.previous = first.next;
  request.monotonicTimeNanoseconds = 319000000ULL;
  const cr::CreativeUiRepeatResult early = cr::stepCreativeUiRepeat(request);
  request.previous = early.next;
  request.monotonicTimeNanoseconds = 320000000ULL;
  const cr::CreativeUiRepeatResult due = cr::stepCreativeUiRepeat(request);
  request.previous = due.next;
  request.command = cr::CreativeUiRepeatCommand::None;
  const cr::CreativeUiRepeatResult released =
      cr::stepCreativeUiRepeat(request);

  return expect(range.valid && range.first == 90U && range.count == 10U,
                "visible range clamps to the list tail") &&
         expect(movedUp == 2U && movedDown == 41U,
                "scroll offset minimally follows focused row") &&
         expect(first.valid && first.fired &&
                    first.next.nextFireNanoseconds == 320000000ULL,
                "repeat fires immediately and schedules the delay") &&
         expect(early.valid && !early.fired,
                "repeat does not fire one millisecond early") &&
         expect(due.fired &&
                    due.next.nextFireNanoseconds == 430000000ULL,
                "repeat fires at the exact delay and schedules interval") &&
         expect(released.valid && !released.fired &&
                    released.next.command ==
                        cr::CreativeUiRepeatCommand::None &&
                    released.next.nextFireNanoseconds == 0U,
                "release clears repeat state");
}

bool invalidAndCapacityInputsFailClosed() {
  cr::CreativeUiWidgetFrame duplicate;
  cr::resetCreativeUiWidgetFrame(duplicate, 320U, 240U);
  cr::CreativeUiWidgetSpec spec;
  spec.id = 1U;
  spec.rect = {0.0F, 0.0F, 100.0F, 30.0F};
  const std::string oversizedLabel(160U, 'A');
  spec.label = oversizedLabel;
  const bool first = cr::appendCreativeUiButton(duplicate, spec);
  const bool second = cr::appendCreativeUiButton(duplicate, spec);

  cr::CreativeUiWidgetFrame capacity;
  cr::resetCreativeUiWidgetFrame(capacity, 320U, 240U);
  bool finalAppend = true;
  spec.label = "Hidden";
  spec.visible = false;
  for (std::size_t index = 0U;
       index <= cr::kCreativeUiWidgetCapacity; ++index) {
    spec.id = static_cast<cr::CreativeUiWidgetId>(index + 1U);
    finalAppend = cr::appendCreativeUiButton(capacity, spec);
  }

  return expect(first && duplicate.textTruncated,
                "oversized text truncates visibly inside fixed storage") &&
         expect(!second && duplicate.invalidInput,
                "duplicate semantic widget id fails closed") &&
         expect(!finalAppend && capacity.capacityExceeded &&
                    capacity.widgetCount == cr::kCreativeUiWidgetCapacity,
                "widget capacity rejects the first out-of-bounds append");
}

}  // namespace

int main() {
  bool ok = true;
  ok = standardWidgetsEmitBoundedVisualsAndRender() && ok;
  ok = pointerPartsAndFocusNavigationAreDeterministic() && ok;
  ok = scrollingAndRepeatTimingArePinned() && ok;
  ok = invalidAndCapacityInputsFailClosed() && ok;
  return ok ? 0 : 1;
}
