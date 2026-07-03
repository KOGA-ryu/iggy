#include "app/iggy3d/creative/Ghost.hpp"

namespace iggy3d::creative {
namespace {

[[nodiscard]] CreativeGhostPoint2 pointFromPointer(
    const CreativeToolPointerPacket& pointer) noexcept {
  return CreativeGhostPoint2{pointer.x, pointer.y};
}

[[nodiscard]] CreativeSnapPoint2 snapPointFromGhost(
    CreativeGhostPoint2 point) noexcept {
  return CreativeSnapPoint2{point.x, point.y};
}

[[nodiscard]] CreativeGhostPoint2 ghostPointFromSnap(
    CreativeSnapPoint2 point) noexcept {
  return CreativeGhostPoint2{point.x, point.y};
}

[[nodiscard]] bool samePoint(CreativeGhostPoint2 lhs,
                             CreativeGhostPoint2 rhs) noexcept {
  return lhs.x == rhs.x && lhs.y == rhs.y;
}

[[nodiscard]] CreativeGhostReceipt makeReceipt(
    const CreativeGhostState& state,
    CreativeGhostChangeKind requestedChange) noexcept {
  CreativeGhostReceipt receipt;
  receipt.requestedChange = requestedChange;
  receipt.visibleBefore = state.visible;
  receipt.visibleAfter = state.visible;
  receipt.sourceToolBefore = state.sourceTool;
  receipt.sourceToolAfter = state.sourceTool;
  receipt.rawPointBefore = state.rawPoint;
  receipt.rawPointAfter = state.rawPoint;
  receipt.snappedPointBefore = state.snappedPoint;
  receipt.snappedPointAfter = state.snappedPoint;
  receipt.targetBefore = state.target;
  receipt.targetAfter = state.target;
  receipt.snapAccepted = state.snapAccepted;
  receipt.snapApplied = state.snapApplied;
  receipt.snapChanged = state.snapChanged;
  receipt.updateCountBefore = state.updateCount;
  receipt.updateCountAfter = state.updateCount;
  return receipt;
}

void refreshAfter(CreativeGhostReceipt& receipt,
                  const CreativeGhostState& state) noexcept {
  receipt.visibleAfter = state.visible;
  receipt.sourceToolAfter = state.sourceTool;
  receipt.rawPointAfter = state.rawPoint;
  receipt.snappedPointAfter = state.snappedPoint;
  receipt.targetAfter = state.target;
  receipt.snapAccepted = state.snapAccepted;
  receipt.snapApplied = state.snapApplied;
  receipt.snapChanged = state.snapChanged;
  receipt.updateCountAfter = state.updateCount;
}

[[nodiscard]] bool samePreviewState(const CreativeGhostState& state,
                                    bool visible,
                                    Tool sourceTool,
                                    CreativeGhostPoint2 rawPoint,
                                    CreativeGhostPoint2 snappedPoint,
                                    TargetRef target,
                                    bool snapAccepted,
                                    bool snapApplied,
                                    bool snapChanged) noexcept {
  return state.visible == visible && state.sourceTool == sourceTool &&
         samePoint(state.rawPoint, rawPoint) &&
         samePoint(state.snappedPoint, snappedPoint) &&
         state.target.value == target.value &&
         state.snapAccepted == snapAccepted &&
         state.snapApplied == snapApplied &&
         state.snapChanged == snapChanged;
}

void writePreviewState(CreativeGhostState& state,
                       Tool sourceTool,
                       CreativeGhostPoint2 rawPoint,
                       CreativeGhostPoint2 snappedPoint,
                       TargetRef target,
                       bool snapAccepted,
                       bool snapApplied,
                       bool snapChanged) noexcept {
  state.visible = true;
  state.sourceTool = sourceTool;
  state.rawPoint = rawPoint;
  state.snappedPoint = snappedPoint;
  state.target = target;
  state.snapAccepted = snapAccepted;
  state.snapApplied = snapApplied;
  state.snapChanged = snapChanged;
}

}  // namespace

CreativeGhostState makeDefaultCreativeGhostState() noexcept {
  return {};
}

CreativeGhostReceipt hideGhost(CreativeGhostState& state) noexcept {
  CreativeGhostReceipt receipt =
      makeReceipt(state, CreativeGhostChangeKind::HideGhost);
  receipt.accepted = true;

  if (!state.visible) {
    receipt.message = "ghost_already_hidden";
    return receipt;
  }

  state = {};
  refreshAfter(receipt, state);
  receipt.changed = true;
  receipt.appliedChange = CreativeGhostChangeKind::HideGhost;
  receipt.message = "ghost_hidden";
  return receipt;
}

CreativeGhostReceipt updateGhostPreview(
    CreativeGhostState& state,
    const CreativeToolPointerPacket& pointer,
    Tool sourceTool,
    CreativeSnapSettings snapSettings) noexcept {
  CreativeGhostReceipt receipt =
      makeReceipt(state, CreativeGhostChangeKind::UpdatePreview);

  const CreativeGhostPoint2 rawPoint = pointFromPointer(pointer);
  CreativeGhostPoint2 snappedPoint = rawPoint;
  bool snapAccepted = false;
  bool snapApplied = false;
  bool snapChanged = false;
  std::string_view message = "invalid_snap_settings";

  const CreativeSnapReceipt snapReceipt =
      snapPoint(snapPointFromGhost(rawPoint), snapSettings);
  if (snapReceipt.accepted) {
    snappedPoint = ghostPointFromSnap(snapReceipt.outputPoint);
    snapAccepted = true;
    snapApplied = snapReceipt.snapped;
    snapChanged = snapReceipt.changed;
    receipt.accepted = true;
    message = "ghost_preview_updated";
  }

  const bool nextVisible = true;
  const bool sameState = samePreviewState(state,
                                          nextVisible,
                                          sourceTool,
                                          rawPoint,
                                          snappedPoint,
                                          pointer.target,
                                          snapAccepted,
                                          snapApplied,
                                          snapChanged);
  if (sameState) {
    receipt.message = snapReceipt.accepted ? "ghost_preview_unchanged" : message;
    return receipt;
  }

  writePreviewState(state,
                    sourceTool,
                    rawPoint,
                    snappedPoint,
                    pointer.target,
                    snapAccepted,
                    snapApplied,
                    snapChanged);
  ++state.updateCount;
  refreshAfter(receipt, state);
  receipt.changed = true;
  receipt.appliedChange = CreativeGhostChangeKind::UpdatePreview;
  receipt.message = message;
  return receipt;
}

CreativeGhostReceipt applyGhostToolIntent(
    CreativeGhostState& state,
    const CreativeToolIntent& intent,
    CreativeSnapSettings snapSettings) noexcept {
  if (intent.kind != CreativeToolIntentKind::PreviewPointer) {
    CreativeGhostReceipt receipt =
        makeReceipt(state, CreativeGhostChangeKind::None);
    receipt.message = "non_preview_intent";
    return receipt;
  }

  return updateGhostPreview(state,
                            intent.pointer,
                            intent.tool,
                            snapSettings);
}

}  // namespace iggy3d::creative
