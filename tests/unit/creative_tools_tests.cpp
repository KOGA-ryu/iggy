#include "app/iggy3d/creative/Tools.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeToolInputPacket pointerInput(cr::CreativeToolInputKind kind,
                                         double x = 10.0,
                                         double y = 20.0) {
  cr::CreativeToolInputPacket input;
  input.kind = kind;
  input.pointer.x = x;
  input.pointer.y = y;
  input.pointer.button = cr::CreativeToolPointerButton::Primary;
  input.pointer.target.value = 42;
  return input;
}

bool defaultStateUsesSelect() {
  const cr::CreativeToolState state = cr::makeDefaultCreativeToolState();

  return expect(state.activeTool == cr::Tool::Select,
                "default active tool select") &&
         expect(!state.measurementActive, "default measurement inactive");
}

bool changingActiveToolWorks() {
  cr::CreativeToolState state = cr::makeDefaultCreativeToolState();
  const bool changed = cr::setActiveTool(state, cr::Tool::Inspect);

  return expect(changed, "active tool changed") &&
         expect(state.activeTool == cr::Tool::Inspect,
                "active tool inspect");
}

bool sameToolActivationIsNoChange() {
  cr::CreativeToolState state = cr::makeDefaultCreativeToolState();
  const bool changed = cr::setActiveTool(state, cr::Tool::Select);

  return expect(!changed, "same active tool no change") &&
         expect(state.activeTool == cr::Tool::Select,
                "same active tool remains select");
}

bool pointerMoveEmitsPreviewIntent() {
  cr::CreativeToolState state = cr::makeDefaultCreativeToolState();
  const cr::CreativeToolDispatchReceipt receipt =
      cr::dispatchToolInput(state,
                            pointerInput(cr::CreativeToolInputKind::PointerMove));

  return expect(receipt.accepted, "preview accepted") &&
         expect(receipt.emittedIntentCount == 1U, "preview intent count") &&
         expect(receipt.intents.size() == 1U, "preview intent size") &&
         expect(receipt.intents[0].kind ==
                    cr::CreativeToolIntentKind::PreviewPointer,
                "preview intent kind") &&
         expect(receipt.activeToolBefore == cr::Tool::Select,
                "preview tool before") &&
         expect(receipt.activeToolAfter == cr::Tool::Select,
                "preview tool after");
}

bool selectPressEmitsSelectObjectCandidate() {
  cr::CreativeToolState state = cr::makeDefaultCreativeToolState();
  const cr::CreativeToolDispatchReceipt receipt =
      cr::dispatchToolInput(state,
                            pointerInput(cr::CreativeToolInputKind::PointerPress));

  return expect(receipt.accepted, "select press accepted") &&
         expect(receipt.inputKind == cr::CreativeToolInputKind::PointerPress,
                "select press input kind") &&
         expect(receipt.emittedIntentCount == 1U, "select press count") &&
         expect(receipt.intents[0].kind ==
                    cr::CreativeToolIntentKind::SelectObjectCandidate,
                "select press intent") &&
         expect(receipt.intents[0].pointer.target.value == 42U,
                "select press target forwarded");
}

bool inspectPressEmitsInspectObjectCandidate() {
  cr::CreativeToolState state = cr::makeDefaultCreativeToolState();
  const bool toolChanged = cr::setActiveTool(state, cr::Tool::Inspect);

  const cr::CreativeToolDispatchReceipt receipt =
      cr::dispatchToolInput(state,
                            pointerInput(cr::CreativeToolInputKind::PointerPress));

  return expect(toolChanged, "inspect setup changed tool") &&
         expect(receipt.accepted, "inspect press accepted") &&
         expect(receipt.activeToolBefore == cr::Tool::Inspect,
                "inspect press tool before") &&
         expect(receipt.activeToolAfter == cr::Tool::Inspect,
                "inspect press tool after") &&
         expect(receipt.emittedIntentCount == 1U, "inspect press count") &&
         expect(receipt.intents[0].kind ==
                    cr::CreativeToolIntentKind::InspectObjectCandidate,
                "inspect press intent");
}

bool measurePressMoveReleaseEmitsMeasurementIntents() {
  cr::CreativeToolState state = cr::makeDefaultCreativeToolState();
  const bool toolChanged = cr::setActiveTool(state, cr::Tool::Measure);

  const cr::CreativeToolDispatchReceipt begin =
      cr::dispatchToolInput(state,
                            pointerInput(cr::CreativeToolInputKind::PointerPress,
                                         1.0,
                                         2.0));
  const bool began =
      expect(toolChanged, "measure setup changed tool") &&
      expect(begin.emittedIntentCount == 1U, "measure begin count") &&
      expect(begin.intents[0].kind ==
                 cr::CreativeToolIntentKind::BeginMeasurement,
             "measure begin intent") &&
      expect(state.measurementActive, "measure active after begin");

  const cr::CreativeToolDispatchReceipt update =
      cr::dispatchToolInput(state,
                            pointerInput(cr::CreativeToolInputKind::PointerMove,
                                         3.0,
                                         4.0));
  const bool updated =
      expect(update.emittedIntentCount == 1U, "measure update count") &&
      expect(update.intents[0].kind ==
                 cr::CreativeToolIntentKind::UpdateMeasurement,
             "measure update intent") &&
      expect(state.measurementActive, "measure active after update");

  const cr::CreativeToolDispatchReceipt end =
      cr::dispatchToolInput(state,
                            pointerInput(cr::CreativeToolInputKind::PointerRelease,
                                         5.0,
                                         6.0));
  const bool ended =
      expect(end.emittedIntentCount == 1U, "measure end count") &&
      expect(end.intents[0].kind ==
                 cr::CreativeToolIntentKind::EndMeasurement,
             "measure end intent") &&
      expect(!state.measurementActive, "measure inactive after end");

  return began && updated && ended;
}

bool unknownInputEmitsNoIntent() {
  cr::CreativeToolState state = cr::makeDefaultCreativeToolState();
  const cr::CreativeToolDispatchReceipt receipt =
      cr::dispatchToolInput(state, {});

  return expect(!receipt.accepted, "unknown input not accepted") &&
         expect(!receipt.changedState, "unknown input unchanged") &&
         expect(receipt.emittedIntentCount == 0U, "unknown input count") &&
         expect(receipt.intents.empty(), "unknown input no intents") &&
         expect(receipt.message == "unsupported_input",
                "unknown input message") &&
         expect(state.activeTool == cr::Tool::Select,
                "unknown input keeps active tool");
}

}  // namespace

int main() {
  const bool ok = defaultStateUsesSelect() &&
                  changingActiveToolWorks() &&
                  sameToolActivationIsNoChange() &&
                  pointerMoveEmitsPreviewIntent() &&
                  selectPressEmitsSelectObjectCandidate() &&
                  inspectPressEmitsInspectObjectCandidate() &&
                  measurePressMoveReleaseEmitsMeasurementIntents() &&
                  unknownInputEmitsNoIntent();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
