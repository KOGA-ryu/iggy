#include "app/iggy3d/creative/Select.hpp"

namespace iggy3d::creative {
namespace {

[[nodiscard]] bool isValidTarget(TargetRef target) noexcept {
  return target.value != kInvalidId;
}

[[nodiscard]] CreativeSelectionReceipt makeReceipt(
    const CreativeSelectionState& state,
    CreativeSelectionChangeKind requestedChange) noexcept {
  CreativeSelectionReceipt receipt;
  receipt.requestedChange = requestedChange;
  receipt.selectedTargetBefore = state.selectedTarget;
  receipt.selectedTargetAfter = state.selectedTarget;
  receipt.candidateTargetBefore = state.candidateTarget;
  receipt.candidateTargetAfter = state.candidateTarget;
  return receipt;
}

void refreshAfter(CreativeSelectionReceipt& receipt,
                  const CreativeSelectionState& state) noexcept {
  receipt.selectedTargetAfter = state.selectedTarget;
  receipt.candidateTargetAfter = state.candidateTarget;
}

}  // namespace

CreativeSelectionState makeDefaultCreativeSelectionState() noexcept {
  return {};
}

CreativeSelectionReceipt clearSelection(CreativeSelectionState& state) noexcept {
  CreativeSelectionReceipt receipt =
      makeReceipt(state, CreativeSelectionChangeKind::ClearSelection);
  receipt.accepted = true;

  if (!isValidTarget(state.selectedTarget) &&
      !isValidTarget(state.candidateTarget)) {
    receipt.message = "selection_already_empty";
    return receipt;
  }

  state.selectedTarget = {};
  state.candidateTarget = {};
  refreshAfter(receipt, state);
  receipt.changed = true;
  receipt.appliedChange = CreativeSelectionChangeKind::ClearSelection;
  receipt.message = "selection_cleared";
  return receipt;
}

CreativeSelectionReceipt setSelectedTarget(CreativeSelectionState& state,
                                           TargetRef target) noexcept {
  CreativeSelectionReceipt receipt =
      makeReceipt(state, CreativeSelectionChangeKind::SetSelectedTarget);
  receipt.accepted = true;

  if (state.selectedTarget.value == target.value) {
    receipt.message = "selected_target_unchanged";
    return receipt;
  }

  state.selectedTarget = target;
  refreshAfter(receipt, state);
  receipt.changed = true;
  receipt.appliedChange = CreativeSelectionChangeKind::SetSelectedTarget;
  receipt.message = "selected_target_changed";
  return receipt;
}

CreativeSelectionReceipt updateSelectionCandidate(
    CreativeSelectionState& state,
    TargetRef target) noexcept {
  CreativeSelectionReceipt receipt =
      makeReceipt(state, CreativeSelectionChangeKind::UpdateCandidate);
  receipt.accepted = true;

  if (state.candidateTarget.value == target.value) {
    receipt.message = "selection_candidate_unchanged";
    return receipt;
  }

  state.candidateTarget = target;
  refreshAfter(receipt, state);
  receipt.changed = true;
  receipt.appliedChange = CreativeSelectionChangeKind::UpdateCandidate;
  receipt.message = "selection_candidate_changed";
  return receipt;
}

CreativeSelectionReceipt applySelectionToolIntent(
    CreativeSelectionState& state,
    const CreativeToolIntent& intent) noexcept {
  if (intent.kind != CreativeToolIntentKind::SelectObjectCandidate) {
    CreativeSelectionReceipt receipt =
        makeReceipt(state, CreativeSelectionChangeKind::None);
    receipt.message = "non_selection_intent";
    return receipt;
  }

  if (!isValidTarget(intent.pointer.target)) {
    CreativeSelectionReceipt receipt = clearSelection(state);
    receipt.requestedChange = CreativeSelectionChangeKind::ClearSelection;
    if (receipt.changed) {
      receipt.message = "select_candidate_cleared_selection";
    } else {
      receipt.message = "select_candidate_no_target";
    }
    return receipt;
  }

  CreativeSelectionReceipt receipt =
      setSelectedTarget(state, intent.pointer.target);
  receipt.requestedChange = CreativeSelectionChangeKind::SetSelectedTarget;
  if (receipt.changed) {
    receipt.message = "select_candidate_selected";
  } else {
    receipt.message = "select_candidate_unchanged";
  }
  return receipt;
}

}  // namespace iggy3d::creative
