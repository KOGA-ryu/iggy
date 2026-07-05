#include "app/iggy3d/creative/tools/Tools.hpp"

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
  bool ok = true;
  const cr::Tool tools[] = {cr::Tool::Move,
                            cr::Tool::Measure,
                            cr::Tool::Navigate,
                            cr::Tool::Select};
  for (const cr::Tool tool : tools) {
    ok = expect(cr::setActiveTool(state, tool), "active tool changed") && ok;
    ok = expect(state.activeTool == tool, "active tool stored") && ok;
  }
  return ok;
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

bool movePressSelectsAndBeginsDrag() {
  // TV1-G: a Move-tool press selects (TV1-C) AND begins a drag (TD-6).
  cr::CreativeToolState state = cr::makeDefaultCreativeToolState();
  const bool toolChanged = cr::setActiveTool(state, cr::Tool::Move);

  const cr::CreativeToolDispatchReceipt receipt =
      cr::dispatchToolInput(state,
                            pointerInput(cr::CreativeToolInputKind::PointerPress));

  return expect(toolChanged, "move setup changed tool") &&
         expect(receipt.accepted, "move press accepted") &&
         expect(receipt.activeToolBefore == cr::Tool::Move,
                "move press tool before") &&
         expect(receipt.activeToolAfter == cr::Tool::Move,
                "move press tool after") &&
         expect(receipt.emittedIntentCount == 2U, "move press count") &&
         expect(receipt.intents[0].kind ==
                    cr::CreativeToolIntentKind::SelectObjectCandidate,
                "move press select intent") &&
         expect(receipt.intents[1].kind ==
                    cr::CreativeToolIntentKind::BeginMove,
                "move press begin-move intent") &&
         expect(receipt.intents[0].pointer.target.value == 42U,
                "move press select target forwarded") &&
         expect(receipt.intents[1].pointer.target.value == 42U,
                "move press begin target forwarded") &&
         expect(state.moveDragActive, "move press activates drag") &&
         expect(state.moveDragTarget.value == 42U, "move press records target") &&
         expect(receipt.message == "move_drag_begin", "move press message");
}

bool moveDragPreviewCommitLifecycle() {
  // Press -> Move (preview) -> Release (commit) drives the drag state machine.
  cr::CreativeToolState state = cr::makeDefaultCreativeToolState();
  static_cast<void>(cr::setActiveTool(state, cr::Tool::Move));
  static_cast<void>(cr::dispatchToolInput(
      state, pointerInput(cr::CreativeToolInputKind::PointerPress)));

  const cr::CreativeToolDispatchReceipt preview = cr::dispatchToolInput(
      state, pointerInput(cr::CreativeToolInputKind::PointerMove, 30.0, 40.0));
  const bool previewOk =
      expect(preview.emittedIntentCount == 1U, "drag preview count") &&
      expect(preview.intents[0].kind ==
                 cr::CreativeToolIntentKind::PreviewMove,
             "drag preview intent") &&
      expect(preview.message == "move_preview", "drag preview message") &&
      expect(state.moveDragActive, "drag still active during preview");

  const cr::CreativeToolDispatchReceipt commit = cr::dispatchToolInput(
      state, pointerInput(cr::CreativeToolInputKind::PointerRelease, 30.0, 40.0));
  const bool commitOk =
      expect(commit.emittedIntentCount == 1U, "drag commit count") &&
      expect(commit.intents[0].kind ==
                 cr::CreativeToolIntentKind::CommitMove,
             "drag commit intent") &&
      expect(commit.message == "move_drag_commit", "drag commit message") &&
      expect(!state.moveDragActive, "drag cleared after commit") &&
      expect(state.moveDragTarget.value == cr::kInvalidId,
             "drag target cleared after commit");
  return previewOk && commitOk;
}

bool releaseWithoutDragIsNoOp() {
  // TV1-F entry req (ii): a Release with no active drag is a harmless no-op.
  cr::CreativeToolState state = cr::makeDefaultCreativeToolState();
  static_cast<void>(cr::setActiveTool(state, cr::Tool::Move));

  const cr::CreativeToolDispatchReceipt receipt = cr::dispatchToolInput(
      state, pointerInput(cr::CreativeToolInputKind::PointerRelease));

  return expect(receipt.accepted, "orphan release accepted") &&
         expect(receipt.emittedIntentCount == 0U, "orphan release no intent") &&
         expect(receipt.message == "no_intent", "orphan release message") &&
         expect(!state.moveDragActive, "orphan release leaves drag inactive");
}

bool cancelMidDragDiscardsWithoutMutation() {
  // TD-6: Esc/Cancel mid-drag discards the drag, emitting CancelMove.
  cr::CreativeToolState state = cr::makeDefaultCreativeToolState();
  static_cast<void>(cr::setActiveTool(state, cr::Tool::Move));
  static_cast<void>(cr::dispatchToolInput(
      state, pointerInput(cr::CreativeToolInputKind::PointerPress)));

  cr::CreativeToolInputPacket cancel;
  cancel.kind = cr::CreativeToolInputKind::Cancel;
  const cr::CreativeToolDispatchReceipt receipt =
      cr::dispatchToolInput(state, cancel);

  return expect(receipt.emittedIntentCount == 1U, "cancel drag count") &&
         expect(receipt.intents[0].kind ==
                    cr::CreativeToolIntentKind::CancelMove,
                "cancel drag intent") &&
         expect(receipt.message == "move_drag_cancel", "cancel drag message") &&
         expect(!state.moveDragActive, "cancel clears drag");
}

bool toolSwitchAbandonsDrag() {
  cr::CreativeToolState state = cr::makeDefaultCreativeToolState();
  static_cast<void>(cr::setActiveTool(state, cr::Tool::Move));
  static_cast<void>(cr::dispatchToolInput(
      state, pointerInput(cr::CreativeToolInputKind::PointerPress)));

  const bool switched = cr::setActiveTool(state, cr::Tool::Select);

  return expect(switched, "tool switched") &&
         expect(!state.moveDragActive, "tool switch abandons drag") &&
         expect(state.moveDragTarget.value == cr::kInvalidId,
                "tool switch clears drag target");
}

bool moveToolPointerMoveKeepsGhostPreview() {
  cr::CreativeToolState state = cr::makeDefaultCreativeToolState();
  static_cast<void>(cr::setActiveTool(state, cr::Tool::Move));

  const cr::CreativeToolDispatchReceipt receipt =
      cr::dispatchToolInput(state,
                            pointerInput(cr::CreativeToolInputKind::PointerMove));

  return expect(receipt.accepted, "move preview accepted") &&
         expect(receipt.emittedIntentCount == 1U, "move preview count") &&
         expect(receipt.intents[0].kind ==
                    cr::CreativeToolIntentKind::PreviewPointer,
                "move preview intent") &&
         expect(receipt.message == "preview_pointer",
                "move preview message");
}

bool navigatePointerInputIsInert() {
  cr::CreativeToolState state = cr::makeDefaultCreativeToolState();
  static_cast<void>(cr::setActiveTool(state, cr::Tool::Navigate));

  bool ok = true;
  const cr::CreativeToolInputKind kinds[] = {
      cr::CreativeToolInputKind::PointerPress,
      cr::CreativeToolInputKind::PointerMove,
      cr::CreativeToolInputKind::PointerRelease,
  };
  for (const cr::CreativeToolInputKind kind : kinds) {
    const cr::CreativeToolDispatchReceipt receipt =
        cr::dispatchToolInput(state, pointerInput(kind));
    ok = expect(receipt.accepted, "navigate input accepted") && ok;
    ok = expect(receipt.emittedIntentCount == 0U,
                "navigate input no intents") && ok;
    ok = expect(receipt.intents.empty(), "navigate input intents empty") && ok;
    ok = expect(!receipt.changedState, "navigate input unchanged") && ok;
    ok = expect(receipt.message == "navigate_pointer_inert",
                "navigate input message") && ok;
  }
  ok = expect(state.pointer.target.value == cr::kInvalidId,
              "navigate pointer target untouched") && ok;
  return ok;
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
                  movePressSelectsAndBeginsDrag() &&
                  moveDragPreviewCommitLifecycle() &&
                  releaseWithoutDragIsNoOp() &&
                  cancelMidDragDiscardsWithoutMutation() &&
                  toolSwitchAbandonsDrag() &&
                  moveToolPointerMoveKeepsGhostPreview() &&
                  navigatePointerInputIsInert() &&
                  measurePressMoveReleaseEmitsMeasurementIntents() &&
                  unknownInputEmitsNoIntent();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
