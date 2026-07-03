#include "app/iggy3d/creative/Select.hpp"

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

cr::TargetRef target(cr::Id id) {
  return cr::TargetRef{id};
}

cr::CreativeToolIntent selectIntent(cr::TargetRef selectedTarget) {
  cr::CreativeToolIntent intent;
  intent.kind = cr::CreativeToolIntentKind::SelectObjectCandidate;
  intent.tool = cr::Tool::Select;
  intent.pointer.target = selectedTarget;
  return intent;
}

bool defaultStateIsEmpty() {
  const cr::CreativeSelectionState state =
      cr::makeDefaultCreativeSelectionState();

  return expect(state.selectedTarget.value == cr::kInvalidId,
                "default selected empty") &&
         expect(state.candidateTarget.value == cr::kInvalidId,
                "default candidate empty");
}

bool settingSelectedTargetChangesState() {
  cr::CreativeSelectionState state = cr::makeDefaultCreativeSelectionState();
  const cr::CreativeSelectionReceipt receipt =
      cr::setSelectedTarget(state, target(7));

  return expect(receipt.accepted, "set selected accepted") &&
         expect(receipt.changed, "set selected changed") &&
         expect(receipt.requestedChange ==
                    cr::CreativeSelectionChangeKind::SetSelectedTarget,
                "set selected requested") &&
         expect(receipt.appliedChange ==
                    cr::CreativeSelectionChangeKind::SetSelectedTarget,
                "set selected applied") &&
         expect(receipt.selectedTargetBefore.value == cr::kInvalidId,
                "set selected before") &&
         expect(receipt.selectedTargetAfter.value == 7U,
                "set selected after") &&
         expect(state.selectedTarget.value == 7U, "set selected state");
}

bool settingSameSelectedTargetIsNoChange() {
  cr::CreativeSelectionState state = cr::makeDefaultCreativeSelectionState();
  const cr::CreativeSelectionReceipt first =
      cr::setSelectedTarget(state, target(7));
  const cr::CreativeSelectionReceipt second =
      cr::setSelectedTarget(state, target(7));

  return expect(first.changed, "same setup changed") &&
         expect(second.accepted, "same selected accepted") &&
         expect(!second.changed, "same selected unchanged") &&
         expect(second.appliedChange == cr::CreativeSelectionChangeKind::None,
                "same selected no applied change") &&
         expect(second.selectedTargetBefore.value == 7U,
                "same selected before") &&
         expect(second.selectedTargetAfter.value == 7U,
                "same selected after");
}

bool clearingEmptySelectionIsNoChange() {
  cr::CreativeSelectionState state = cr::makeDefaultCreativeSelectionState();
  const cr::CreativeSelectionReceipt receipt = cr::clearSelection(state);

  return expect(receipt.accepted, "clear empty accepted") &&
         expect(!receipt.changed, "clear empty unchanged") &&
         expect(receipt.selectedTargetAfter.value == cr::kInvalidId,
                "clear empty selected") &&
         expect(receipt.candidateTargetAfter.value == cr::kInvalidId,
                "clear empty candidate") &&
         expect(receipt.message == "selection_already_empty",
                "clear empty message");
}

bool clearingNonEmptySelectionClearsCandidate() {
  cr::CreativeSelectionState state = cr::makeDefaultCreativeSelectionState();
  const cr::CreativeSelectionReceipt selected =
      cr::setSelectedTarget(state, target(7));
  const cr::CreativeSelectionReceipt candidate =
      cr::updateSelectionCandidate(state, target(9));
  const cr::CreativeSelectionReceipt cleared = cr::clearSelection(state);

  return expect(selected.changed, "clear setup selected") &&
         expect(candidate.changed, "clear setup candidate") &&
         expect(cleared.accepted, "clear non-empty accepted") &&
         expect(cleared.changed, "clear non-empty changed") &&
         expect(cleared.appliedChange ==
                    cr::CreativeSelectionChangeKind::ClearSelection,
                "clear non-empty applied") &&
         expect(cleared.selectedTargetBefore.value == 7U,
                "clear non-empty selected before") &&
         expect(cleared.candidateTargetBefore.value == 9U,
                "clear non-empty candidate before") &&
         expect(state.selectedTarget.value == cr::kInvalidId,
                "clear non-empty selected state") &&
         expect(state.candidateTarget.value == cr::kInvalidId,
                "clear non-empty candidate state");
}

bool updatingCandidateDoesNotMutateSelected() {
  cr::CreativeSelectionState state = cr::makeDefaultCreativeSelectionState();
  const cr::CreativeSelectionReceipt selected =
      cr::setSelectedTarget(state, target(7));
  const cr::CreativeSelectionReceipt candidate =
      cr::updateSelectionCandidate(state, target(9));

  return expect(selected.changed, "candidate setup selected") &&
         expect(candidate.accepted, "candidate accepted") &&
         expect(candidate.changed, "candidate changed") &&
         expect(candidate.appliedChange ==
                    cr::CreativeSelectionChangeKind::UpdateCandidate,
                "candidate applied") &&
         expect(state.selectedTarget.value == 7U,
                "candidate leaves selected") &&
         expect(state.candidateTarget.value == 9U,
                "candidate updates candidate");
}

bool applyingSelectObjectCandidateSelectsTarget() {
  cr::CreativeSelectionState state = cr::makeDefaultCreativeSelectionState();
  const cr::CreativeSelectionReceipt receipt =
      cr::applySelectionToolIntent(state, selectIntent(target(42)));

  return expect(receipt.accepted, "select intent accepted") &&
         expect(receipt.changed, "select intent changed") &&
         expect(receipt.selectedTargetBefore.value == cr::kInvalidId,
                "select intent before") &&
         expect(receipt.selectedTargetAfter.value == 42U,
                "select intent after") &&
         expect(state.selectedTarget.value == 42U,
                "select intent state selected") &&
         expect(receipt.message == "select_candidate_selected",
                "select intent message");
}

bool invalidSelectObjectCandidateClearsSelection() {
  cr::CreativeSelectionState state = cr::makeDefaultCreativeSelectionState();
  const cr::CreativeSelectionReceipt selected =
      cr::setSelectedTarget(state, target(42));
  const cr::CreativeSelectionReceipt candidate =
      cr::updateSelectionCandidate(state, target(7));
  const cr::CreativeSelectionReceipt cleared =
      cr::applySelectionToolIntent(state, selectIntent({}));

  return expect(selected.changed, "invalid setup selected") &&
         expect(candidate.changed, "invalid setup candidate") &&
         expect(cleared.accepted, "invalid select accepted") &&
         expect(cleared.changed, "invalid select changed") &&
         expect(cleared.appliedChange ==
                    cr::CreativeSelectionChangeKind::ClearSelection,
                "invalid select clears") &&
         expect(cleared.selectedTargetBefore.value == 42U,
                "invalid select selected before") &&
         expect(cleared.candidateTargetBefore.value == 7U,
                "invalid select candidate before") &&
         expect(state.selectedTarget.value == cr::kInvalidId,
                "invalid select selected cleared") &&
         expect(state.candidateTarget.value == cr::kInvalidId,
                "invalid select candidate cleared") &&
         expect(cleared.message == "select_candidate_cleared_selection",
                "invalid select message");
}

bool nonSelectionIntentIsNoOp() {
  cr::CreativeSelectionState state = cr::makeDefaultCreativeSelectionState();
  const cr::CreativeSelectionReceipt selected =
      cr::setSelectedTarget(state, target(42));

  cr::CreativeToolIntent intent;
  intent.kind = cr::CreativeToolIntentKind::PreviewPointer;
  intent.tool = cr::Tool::Select;
  intent.pointer.target = target(7);

  const cr::CreativeSelectionReceipt receipt =
      cr::applySelectionToolIntent(state, intent);

  return expect(selected.changed, "non-selection setup selected") &&
         expect(!receipt.accepted, "non-selection not accepted") &&
         expect(!receipt.changed, "non-selection unchanged") &&
         expect(receipt.appliedChange == cr::CreativeSelectionChangeKind::None,
                "non-selection no applied change") &&
         expect(state.selectedTarget.value == 42U,
                "non-selection selected preserved") &&
         expect(state.candidateTarget.value == cr::kInvalidId,
                "non-selection candidate preserved") &&
         expect(receipt.message == "non_selection_intent",
                "non-selection message");
}

}  // namespace

int main() {
  const bool ok = defaultStateIsEmpty() &&
                  settingSelectedTargetChangesState() &&
                  settingSameSelectedTargetIsNoChange() &&
                  clearingEmptySelectionIsNoChange() &&
                  clearingNonEmptySelectionClearsCandidate() &&
                  updatingCandidateDoesNotMutateSelected() &&
                  applyingSelectObjectCandidateSelectsTarget() &&
                  invalidSelectObjectCandidateClearsSelection() &&
                  nonSelectionIntentIsNoOp();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
