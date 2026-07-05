#include "app/iggy3d/creative/Ghost.hpp"

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

cr::CreativeToolPointerPacket pointer(double x,
                                      double y,
                                      cr::Id targetId = 0) {
  cr::CreativeToolPointerPacket packet;
  packet.x = x;
  packet.y = y;
  packet.button = cr::CreativeToolPointerButton::Primary;
  packet.modifiers = cr::kCreativeToolModifierShift;
  packet.target.value = targetId;
  return packet;
}

bool expectPoint(cr::CreativeGhostPoint2 point,
                 double x,
                 double y,
                 std::string_view message) {
  return expect(point.x == x, message) &&
         expect(point.y == y, message);
}

bool defaultStateHidden() {
  const cr::CreativeGhostState state = cr::makeDefaultCreativeGhostState();

  return expect(!state.visible, "default hidden") &&
         expect(state.sourceTool == cr::Tool::Select, "default source tool") &&
         expectPoint(state.rawPoint, 0.0, 0.0, "default raw") &&
         expectPoint(state.snappedPoint, 0.0, 0.0, "default snapped") &&
         expect(state.target.value == cr::kInvalidId, "default target") &&
         expect(!state.snapAccepted, "default snap accepted") &&
         expect(!state.snapApplied, "default snap applied") &&
         expect(!state.snapChanged, "default snap changed") &&
         expect(state.updateCount == 0U, "default update count");
}

bool hideHiddenIsNoChange() {
  cr::CreativeGhostState state = cr::makeDefaultCreativeGhostState();
  const cr::CreativeGhostReceipt receipt = cr::hideGhost(state);

  return expect(receipt.accepted, "hide hidden accepted") &&
         expect(!receipt.changed, "hide hidden unchanged") &&
         expect(receipt.message == "ghost_already_hidden",
                "hide hidden message") &&
         expect(!state.visible, "hide hidden state");
}

bool hideVisibleClearsPreviewState() {
  cr::CreativeGhostState state = cr::makeDefaultCreativeGhostState();
  const cr::CreativeGhostReceipt update =
      cr::updateGhostPreview(state,
                             pointer(1.2, 2.7, 42),
                             cr::Tool::Move,
                             cr::makeDefaultCreativeSnapSettings());
  const cr::CreativeGhostReceipt hide = cr::hideGhost(state);

  return expect(update.changed, "hide setup update") &&
         expect(hide.accepted, "hide visible accepted") &&
         expect(hide.changed, "hide visible changed") &&
         expect(hide.appliedChange == cr::CreativeGhostChangeKind::HideGhost,
                "hide visible applied") &&
         expect(hide.visibleBefore, "hide visible before") &&
         expect(!hide.visibleAfter, "hide visible after") &&
         expect(!state.visible, "hide state hidden") &&
         expectPoint(state.rawPoint, 0.0, 0.0, "hide raw cleared") &&
         expectPoint(state.snappedPoint, 0.0, 0.0, "hide snapped cleared") &&
         expect(state.target.value == cr::kInvalidId, "hide target cleared") &&
         expect(!state.snapAccepted, "hide snap accepted cleared") &&
         expect(!state.snapApplied, "hide snap applied cleared") &&
         expect(!state.snapChanged, "hide snap changed cleared") &&
         expect(state.updateCount == 0U, "hide update count cleared");
}

bool updatePreviewWithDefaultSnapStoresRawAndSnapped() {
  cr::CreativeGhostState state = cr::makeDefaultCreativeGhostState();
  const cr::CreativeGhostReceipt receipt =
      cr::updateGhostPreview(state,
                             pointer(1.2, 2.7, 42),
                             cr::Tool::Select,
                             cr::makeDefaultCreativeSnapSettings());

  return expect(receipt.accepted, "default snap accepted") &&
         expect(receipt.changed, "default snap changed") &&
         expect(receipt.visibleAfter, "default snap visible") &&
         expect(state.visible, "default state visible") &&
         expect(state.sourceTool == cr::Tool::Select, "default source") &&
         expectPoint(state.rawPoint, 1.2, 2.7, "default raw") &&
         expectPoint(state.snappedPoint, 1.0, 3.0, "default snapped") &&
         expect(state.target.value == 42U, "default target") &&
         expect(state.snapAccepted, "default snap accepted state") &&
         expect(state.snapApplied, "default snap applied") &&
         expect(state.snapChanged, "default snap changed state") &&
         expect(state.updateCount == 1U, "default update count");
}

bool disabledSnapStoresRawAsSnapped() {
  cr::CreativeGhostState state = cr::makeDefaultCreativeGhostState();
  cr::CreativeSnapSettings settings = cr::makeDefaultCreativeSnapSettings();
  settings.mode = cr::CreativeSnapMode::Disabled;

  const cr::CreativeGhostReceipt receipt =
      cr::updateGhostPreview(state,
                             pointer(1.2, 2.7, 42),
                             cr::Tool::Select,
                             settings);

  return expect(receipt.accepted, "disabled accepted") &&
         expect(receipt.changed, "disabled changed") &&
         expect(state.visible, "disabled visible") &&
         expectPoint(state.rawPoint, 1.2, 2.7, "disabled raw") &&
         expectPoint(state.snappedPoint, 1.2, 2.7, "disabled snapped") &&
         expect(state.snapAccepted, "disabled snap accepted") &&
         expect(!state.snapApplied, "disabled snap applied") &&
         expect(!state.snapChanged, "disabled snap changed");
}

bool noAxisSnapStoresRawAsSnapped() {
  cr::CreativeGhostState state = cr::makeDefaultCreativeGhostState();
  cr::CreativeSnapSettings settings = cr::makeDefaultCreativeSnapSettings();
  settings.axes = cr::kCreativeSnapAxisNone;

  const cr::CreativeGhostReceipt receipt =
      cr::updateGhostPreview(state,
                             pointer(1.2, 2.7, 42),
                             cr::Tool::Select,
                             settings);

  return expect(receipt.accepted, "no-axis accepted") &&
         expect(receipt.changed, "no-axis changed") &&
         expect(state.visible, "no-axis visible") &&
         expectPoint(state.rawPoint, 1.2, 2.7, "no-axis raw") &&
         expectPoint(state.snappedPoint, 1.2, 2.7, "no-axis snapped") &&
         expect(state.snapAccepted, "no-axis snap accepted") &&
         expect(!state.snapApplied, "no-axis snap applied") &&
         expect(!state.snapChanged, "no-axis snap changed");
}

bool invalidSnapSettingsPublishRawPreviewButRejectSnap() {
  cr::CreativeGhostState state = cr::makeDefaultCreativeGhostState();
  cr::CreativeSnapSettings settings = cr::makeDefaultCreativeSnapSettings();
  settings.stepX = 0.0;

  const cr::CreativeGhostReceipt receipt =
      cr::updateGhostPreview(state,
                             pointer(1.2, 2.7, 42),
                             cr::Tool::Measure,
                             settings);

  return expect(!receipt.accepted, "invalid not accepted") &&
         expect(receipt.changed, "invalid changed") &&
         expect(receipt.message == "invalid_snap_settings",
                "invalid message") &&
         expect(state.visible, "invalid visible") &&
         expect(state.sourceTool == cr::Tool::Measure, "invalid source") &&
         expectPoint(state.rawPoint, 1.2, 2.7, "invalid raw") &&
         expectPoint(state.snappedPoint, 1.2, 2.7, "invalid snapped") &&
         expect(state.target.value == 42U, "invalid target") &&
         expect(!state.snapAccepted, "invalid snap accepted") &&
         expect(!state.snapApplied, "invalid snap applied") &&
         expect(!state.snapChanged, "invalid snap changed");
}

bool repeatedIdenticalUpdateReportsNoChange() {
  cr::CreativeGhostState state = cr::makeDefaultCreativeGhostState();
  const cr::CreativeToolPointerPacket packet = pointer(1.2, 2.7, 42);
  const cr::CreativeSnapSettings settings =
      cr::makeDefaultCreativeSnapSettings();
  const cr::CreativeGhostReceipt first =
      cr::updateGhostPreview(state, packet, cr::Tool::Select, settings);
  const cr::CreativeGhostReceipt second =
      cr::updateGhostPreview(state, packet, cr::Tool::Select, settings);

  return expect(first.changed, "repeat setup changed") &&
         expect(second.accepted, "repeat accepted") &&
         expect(!second.changed, "repeat unchanged") &&
         expect(second.message == "ghost_preview_unchanged",
                "repeat message") &&
         expect(state.updateCount == 1U, "repeat update count");
}

bool changedPointerReportsChangedAndIncrements() {
  cr::CreativeGhostState state = cr::makeDefaultCreativeGhostState();
  const cr::CreativeSnapSettings settings =
      cr::makeDefaultCreativeSnapSettings();
  const cr::CreativeGhostReceipt first =
      cr::updateGhostPreview(state,
                             pointer(1.2, 2.7, 42),
                             cr::Tool::Select,
                             settings);
  const cr::CreativeGhostReceipt second =
      cr::updateGhostPreview(state,
                             pointer(2.2, 3.1, 43),
                             cr::Tool::Select,
                             settings);

  return expect(first.changed, "changed setup") &&
         expect(second.accepted, "changed accepted") &&
         expect(second.changed, "changed changed") &&
         expect(second.updateCountBefore == 1U, "changed count before") &&
         expect(second.updateCountAfter == 2U, "changed count after") &&
         expectPoint(state.rawPoint, 2.2, 3.1, "changed raw") &&
         expectPoint(state.snappedPoint, 2.0, 3.0, "changed snapped") &&
         expect(state.target.value == 43U, "changed target");
}

bool applyPreviewPointerRoutesToUpdate() {
  cr::CreativeGhostState state = cr::makeDefaultCreativeGhostState();
  cr::CreativeToolIntent intent;
  intent.kind = cr::CreativeToolIntentKind::PreviewPointer;
  intent.tool = cr::Tool::Move;
  intent.pointer = pointer(1.2, 2.7, 42);

  const cr::CreativeGhostReceipt receipt =
      cr::applyGhostToolIntent(state,
                               intent,
                               cr::makeDefaultCreativeSnapSettings());

  return expect(receipt.accepted, "apply preview accepted") &&
         expect(receipt.changed, "apply preview changed") &&
         expect(receipt.appliedChange == cr::CreativeGhostChangeKind::UpdatePreview,
                "apply preview applied") &&
         expect(state.sourceTool == cr::Tool::Move, "apply source") &&
         expectPoint(state.rawPoint, 1.2, 2.7, "apply raw") &&
         expectPoint(state.snappedPoint, 1.0, 3.0, "apply snapped");
}

bool nonPreviewIntentIsNoOp() {
  cr::CreativeGhostState state = cr::makeDefaultCreativeGhostState();
  const cr::CreativeGhostReceipt update =
      cr::updateGhostPreview(state,
                             pointer(1.2, 2.7, 42),
                             cr::Tool::Select,
                             cr::makeDefaultCreativeSnapSettings());

  cr::CreativeToolIntent intent;
  intent.kind = cr::CreativeToolIntentKind::SelectObjectCandidate;
  intent.tool = cr::Tool::Select;
  intent.pointer = pointer(5.0, 6.0, 99);

  const cr::CreativeGhostReceipt receipt =
      cr::applyGhostToolIntent(state,
                               intent,
                               cr::makeDefaultCreativeSnapSettings());

  return expect(update.changed, "non-preview setup") &&
         expect(!receipt.accepted, "non-preview not accepted") &&
         expect(!receipt.changed, "non-preview unchanged") &&
         expect(receipt.message == "non_preview_intent",
                "non-preview message") &&
         expectPoint(state.rawPoint, 1.2, 2.7, "non-preview raw preserved") &&
         expect(state.target.value == 42U, "non-preview target preserved");
}

}  // namespace

int main() {
  const bool ok = defaultStateHidden() &&
                  hideHiddenIsNoChange() &&
                  hideVisibleClearsPreviewState() &&
                  updatePreviewWithDefaultSnapStoresRawAndSnapped() &&
                  disabledSnapStoresRawAsSnapped() &&
                  noAxisSnapStoresRawAsSnapped() &&
                  invalidSnapSettingsPublishRawPreviewButRejectSnap() &&
                  repeatedIdenticalUpdateReportsNoChange() &&
                  changedPointerReportsChangedAndIncrements() &&
                  applyPreviewPointerRoutesToUpdate() &&
                  nonPreviewIntentIsNoOp();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
