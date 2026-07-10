#include "app/iggy3d/creative/tools/Select.hpp"

#include <algorithm>
#include <utility>

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
  receipt.selectedCountBefore = selectedTargetCount(state);
  receipt.selectedCountAfter = receipt.selectedCountBefore;
  return receipt;
}

void refreshAfter(CreativeSelectionReceipt& receipt,
                  const CreativeSelectionState& state) noexcept {
  receipt.selectedTargetAfter = state.selectedTarget;
  receipt.candidateTargetAfter = state.candidateTarget;
  receipt.selectedCountAfter = selectedTargetCount(state);
}

[[nodiscard]] bool sameTargets(std::span<const TargetRef> lhs,
                               std::span<const TargetRef> rhs) noexcept {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (std::size_t index = 0; index < lhs.size(); ++index) {
    if (lhs[index].value != rhs[index].value) {
      return false;
    }
  }
  return true;
}

}  // namespace

CreativeSelectionState makeDefaultCreativeSelectionState() noexcept {
  return {};
}

std::uint64_t selectedTargetCount(
    const CreativeSelectionState& state) noexcept {
  if (!state.selectedTargets.empty()) {
    return state.selectedTargets.size();
  }
  return isValidTarget(state.selectedTarget) ? 1U : 0U;
}

bool selectionContainsTarget(const CreativeSelectionState& state,
                             TargetRef target) noexcept {
  if (!isValidTarget(target)) {
    return false;
  }
  if (state.selectedTarget.value == target.value) {
    return true;
  }
  return std::any_of(state.selectedTargets.begin(), state.selectedTargets.end(),
                     [target](TargetRef selected) {
                       return selected.value == target.value;
                     });
}

std::span<const TargetRef> selectedTargetList(
    const CreativeSelectionState& state) noexcept {
  return state.selectedTargets;
}

CreativeSelectionReceipt clearSelection(CreativeSelectionState& state) noexcept {
  CreativeSelectionReceipt receipt =
      makeReceipt(state, CreativeSelectionChangeKind::ClearSelection);
  receipt.accepted = true;

  if (!isValidTarget(state.selectedTarget) && state.selectedTargets.empty() &&
      !isValidTarget(state.candidateTarget)) {
    receipt.message = "selection_already_empty";
    return receipt;
  }

  state.selectedTarget = {};
  state.selectedTargets.clear();
  state.candidateTarget = {};
  refreshAfter(receipt, state);
  receipt.changed = true;
  receipt.appliedChange = CreativeSelectionChangeKind::ClearSelection;
  receipt.message = "selection_cleared";
  return receipt;
}

CreativeSelectionReceipt setSelectedTarget(CreativeSelectionState& state,
                                           TargetRef target) {
  CreativeSelectionReceipt receipt =
      makeReceipt(state, CreativeSelectionChangeKind::SetSelectedTarget);
  receipt.accepted = true;

  const bool targetAlreadySoleSelection =
      state.selectedTarget.value == target.value &&
      ((!isValidTarget(target) && state.selectedTargets.empty()) ||
       (state.selectedTargets.size() == 1U &&
        state.selectedTargets.front().value == target.value));
  if (targetAlreadySoleSelection) {
    receipt.message = "selected_target_unchanged";
    return receipt;
  }

  state.selectedTarget = target;
  state.selectedTargets.clear();
  if (isValidTarget(target)) {
    state.selectedTargets.push_back(target);
  }
  refreshAfter(receipt, state);
  receipt.changed = true;
  receipt.appliedChange = CreativeSelectionChangeKind::SetSelectedTarget;
  receipt.message = "selected_target_changed";
  return receipt;
}

CreativeSelectionReceipt setSelectedTargets(
    CreativeSelectionState& state,
    std::span<const TargetRef> targets,
    TargetRef primaryTarget) {
  CreativeSelectionReceipt receipt =
      makeReceipt(state, CreativeSelectionChangeKind::ReplaceSelectedTargets);
  receipt.accepted = true;

  std::vector<TargetRef> normalized;
  normalized.reserve(targets.size());
  for (TargetRef target : targets) {
    if (!isValidTarget(target)) {
      continue;
    }
    const bool duplicate = std::any_of(
        normalized.begin(), normalized.end(), [target](TargetRef existing) {
          return existing.value == target.value;
        });
    if (!duplicate) {
      normalized.push_back(target);
    }
  }

  const bool requestedPrimaryPresent =
      isValidTarget(primaryTarget) &&
      std::any_of(normalized.begin(), normalized.end(),
                  [primaryTarget](TargetRef target) {
                    return target.value == primaryTarget.value;
                  });
  const TargetRef nextPrimary =
      requestedPrimaryPresent
          ? primaryTarget
          : normalized.empty() ? TargetRef{} : normalized.back();
  if (sameTargets(state.selectedTargets, normalized) &&
      state.selectedTarget.value == nextPrimary.value) {
    receipt.message = "selected_targets_unchanged";
    return receipt;
  }

  state.selectedTargets = std::move(normalized);
  state.selectedTarget = nextPrimary;
  refreshAfter(receipt, state);
  receipt.changed = true;
  receipt.appliedChange = CreativeSelectionChangeKind::ReplaceSelectedTargets;
  receipt.message = "selected_targets_changed";
  return receipt;
}

CreativeSelectionReceipt toggleSelectedTarget(CreativeSelectionState& state,
                                              TargetRef target) {
  CreativeSelectionReceipt receipt =
      makeReceipt(state, CreativeSelectionChangeKind::ToggleSelectedTarget);
  if (!isValidTarget(target)) {
    receipt.message = "toggle_target_invalid";
    return receipt;
  }
  receipt.accepted = true;

  if (state.selectedTargets.empty() && isValidTarget(state.selectedTarget)) {
    state.selectedTargets.push_back(state.selectedTarget);
  }
  const auto found = std::find_if(
      state.selectedTargets.begin(), state.selectedTargets.end(),
      [target](TargetRef selected) { return selected.value == target.value; });
  if (found == state.selectedTargets.end()) {
    state.selectedTargets.push_back(target);
    state.selectedTarget = target;
    receipt.message = "selected_target_added";
  } else {
    state.selectedTargets.erase(found);
    if (state.selectedTarget.value == target.value) {
      state.selectedTarget = state.selectedTargets.empty()
                                 ? TargetRef{}
                                 : state.selectedTargets.back();
    }
    receipt.message = "selected_target_removed";
  }

  refreshAfter(receipt, state);
  receipt.changed = true;
  receipt.appliedChange = CreativeSelectionChangeKind::ToggleSelectedTarget;
  return receipt;
}

CreativeSelectionReceipt removeSelectedTarget(CreativeSelectionState& state,
                                              TargetRef target) noexcept {
  CreativeSelectionReceipt receipt =
      makeReceipt(state, CreativeSelectionChangeKind::RemoveSelectedTarget);
  receipt.accepted = true;
  const auto found = std::find_if(
      state.selectedTargets.begin(), state.selectedTargets.end(),
      [target](TargetRef selected) { return selected.value == target.value; });
  const bool primaryOnlyFallback = state.selectedTargets.empty() &&
                                   state.selectedTarget.value == target.value;
  if (found == state.selectedTargets.end() && !primaryOnlyFallback) {
    receipt.message = "selected_target_not_present";
    return receipt;
  }

  if (found != state.selectedTargets.end()) {
    state.selectedTargets.erase(found);
  }
  if (state.selectedTarget.value == target.value) {
    state.selectedTarget = state.selectedTargets.empty()
                               ? TargetRef{}
                               : state.selectedTargets.back();
  }
  refreshAfter(receipt, state);
  receipt.changed = true;
  receipt.appliedChange = CreativeSelectionChangeKind::RemoveSelectedTarget;
  receipt.message = "selected_target_removed";
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
    const CreativeToolIntent& intent) {
  if (intent.kind != CreativeToolIntentKind::SelectObjectCandidate) {
    CreativeSelectionReceipt receipt =
        makeReceipt(state, CreativeSelectionChangeKind::None);
    receipt.message = "non_selection_intent";
    return receipt;
  }

  if (!isValidTarget(intent.pointer.target)) {
    if ((intent.pointer.modifiers & kCreativeToolModifierShift) != 0U) {
      CreativeSelectionReceipt receipt =
          makeReceipt(state, CreativeSelectionChangeKind::None);
      receipt.accepted = true;
      receipt.message = "additive_select_no_target";
      return receipt;
    }
    CreativeSelectionReceipt receipt = clearSelection(state);
    receipt.requestedChange = CreativeSelectionChangeKind::ClearSelection;
    if (receipt.changed) {
      receipt.message = "select_candidate_cleared_selection";
    } else {
      receipt.message = "select_candidate_no_target";
    }
    return receipt;
  }

  const bool additive =
      (intent.pointer.modifiers & kCreativeToolModifierShift) != 0U;
  if (!additive && intent.tool == Tool::Move &&
      selectionContainsTarget(state, intent.pointer.target)) {
    CreativeSelectionReceipt receipt =
        makeReceipt(state, CreativeSelectionChangeKind::None);
    receipt.accepted = true;
    receipt.message = "move_preserved_selected_group";
    return receipt;
  }
  CreativeSelectionReceipt receipt =
      additive ? toggleSelectedTarget(state, intent.pointer.target)
               : setSelectedTarget(state, intent.pointer.target);
  receipt.requestedChange = additive
                                ? CreativeSelectionChangeKind::ToggleSelectedTarget
                                : CreativeSelectionChangeKind::SetSelectedTarget;
  if (receipt.changed) {
    receipt.message = additive ? "select_candidate_toggled"
                               : "select_candidate_selected";
  } else {
    receipt.message = "select_candidate_unchanged";
  }
  return receipt;
}

}  // namespace iggy3d::creative
