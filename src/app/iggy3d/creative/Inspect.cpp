#include "app/iggy3d/creative/Inspect.hpp"

namespace iggy3d::creative {
namespace {

[[nodiscard]] bool isValidTarget(TargetRef target) noexcept {
  return target.value != kInvalidId;
}

[[nodiscard]] CreativeInspectionReceipt makeReceipt(
    const CreativeInspectionState& state,
    CreativeInspectionChangeKind requestedChange) noexcept {
  CreativeInspectionReceipt receipt;
  receipt.requestedChange = requestedChange;
  receipt.inspectedTargetBefore = state.inspectedTarget;
  receipt.inspectedTargetAfter = state.inspectedTarget;
  receipt.candidateTargetBefore = state.candidateTarget;
  receipt.candidateTargetAfter = state.candidateTarget;
  return receipt;
}

void refreshAfter(CreativeInspectionReceipt& receipt,
                  const CreativeInspectionState& state) noexcept {
  receipt.inspectedTargetAfter = state.inspectedTarget;
  receipt.candidateTargetAfter = state.candidateTarget;
}

}  // namespace

CreativeInspectionState makeDefaultCreativeInspectionState() noexcept {
  return {};
}

CreativeInspectionReceipt clearInspection(
    CreativeInspectionState& state) noexcept {
  CreativeInspectionReceipt receipt =
      makeReceipt(state, CreativeInspectionChangeKind::ClearInspection);
  receipt.accepted = true;

  if (!isValidTarget(state.inspectedTarget) &&
      !isValidTarget(state.candidateTarget)) {
    receipt.message = "inspection_already_empty";
    return receipt;
  }

  state.inspectedTarget = {};
  state.candidateTarget = {};
  refreshAfter(receipt, state);
  receipt.changed = true;
  receipt.appliedChange = CreativeInspectionChangeKind::ClearInspection;
  receipt.message = "inspection_cleared";
  return receipt;
}

CreativeInspectionReceipt setInspectedTarget(CreativeInspectionState& state,
                                             TargetRef target) noexcept {
  CreativeInspectionReceipt receipt =
      makeReceipt(state, CreativeInspectionChangeKind::SetInspectedTarget);
  receipt.accepted = true;

  if (state.inspectedTarget.value == target.value) {
    receipt.message = "inspected_target_unchanged";
    return receipt;
  }

  state.inspectedTarget = target;
  refreshAfter(receipt, state);
  receipt.changed = true;
  receipt.appliedChange = CreativeInspectionChangeKind::SetInspectedTarget;
  receipt.message = "inspected_target_changed";
  return receipt;
}

CreativeInspectionReceipt updateInspectionCandidate(
    CreativeInspectionState& state,
    TargetRef target) noexcept {
  CreativeInspectionReceipt receipt =
      makeReceipt(state, CreativeInspectionChangeKind::UpdateCandidate);
  receipt.accepted = true;

  if (state.candidateTarget.value == target.value) {
    receipt.message = "inspection_candidate_unchanged";
    return receipt;
  }

  state.candidateTarget = target;
  refreshAfter(receipt, state);
  receipt.changed = true;
  receipt.appliedChange = CreativeInspectionChangeKind::UpdateCandidate;
  receipt.message = "inspection_candidate_changed";
  return receipt;
}

CreativeInspectionReceipt applyInspectionToolIntent(
    CreativeInspectionState& state,
    const CreativeToolIntent& intent) noexcept {
  if (intent.kind != CreativeToolIntentKind::InspectObjectCandidate) {
    CreativeInspectionReceipt receipt =
        makeReceipt(state, CreativeInspectionChangeKind::None);
    receipt.message = "non_inspection_intent";
    return receipt;
  }

  if (!isValidTarget(intent.pointer.target)) {
    CreativeInspectionReceipt receipt = clearInspection(state);
    receipt.requestedChange = CreativeInspectionChangeKind::ClearInspection;
    if (receipt.changed) {
      receipt.message = "inspect_candidate_cleared_inspection";
    } else {
      receipt.message = "inspect_candidate_no_target";
    }
    return receipt;
  }

  CreativeInspectionReceipt receipt =
      setInspectedTarget(state, intent.pointer.target);
  receipt.requestedChange = CreativeInspectionChangeKind::SetInspectedTarget;
  if (receipt.changed) {
    receipt.message = "inspect_candidate_inspected";
  } else {
    receipt.message = "inspect_candidate_unchanged";
  }
  return receipt;
}

}  // namespace iggy3d::creative
