#include "app/iggy3d/creative/Inspect.hpp"

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

cr::CreativeToolIntent inspectIntent(cr::TargetRef inspectedTarget) {
  cr::CreativeToolIntent intent;
  intent.kind = cr::CreativeToolIntentKind::InspectObjectCandidate;
  intent.tool = cr::Tool::Inspect;
  intent.pointer.target = inspectedTarget;
  return intent;
}

bool defaultStateIsEmpty() {
  const cr::CreativeInspectionState state =
      cr::makeDefaultCreativeInspectionState();

  return expect(state.inspectedTarget.value == cr::kInvalidId,
                "default inspected empty") &&
         expect(state.candidateTarget.value == cr::kInvalidId,
                "default candidate empty");
}

bool settingInspectedTargetChangesState() {
  cr::CreativeInspectionState state = cr::makeDefaultCreativeInspectionState();
  const cr::CreativeInspectionReceipt receipt =
      cr::setInspectedTarget(state, target(7));

  return expect(receipt.accepted, "set inspected accepted") &&
         expect(receipt.changed, "set inspected changed") &&
         expect(receipt.requestedChange ==
                    cr::CreativeInspectionChangeKind::SetInspectedTarget,
                "set inspected requested") &&
         expect(receipt.appliedChange ==
                    cr::CreativeInspectionChangeKind::SetInspectedTarget,
                "set inspected applied") &&
         expect(receipt.inspectedTargetBefore.value == cr::kInvalidId,
                "set inspected before") &&
         expect(receipt.inspectedTargetAfter.value == 7U,
                "set inspected after") &&
         expect(state.inspectedTarget.value == 7U, "set inspected state");
}

bool settingSameInspectedTargetIsNoChange() {
  cr::CreativeInspectionState state = cr::makeDefaultCreativeInspectionState();
  const cr::CreativeInspectionReceipt first =
      cr::setInspectedTarget(state, target(7));
  const cr::CreativeInspectionReceipt second =
      cr::setInspectedTarget(state, target(7));

  return expect(first.changed, "same setup changed") &&
         expect(second.accepted, "same inspected accepted") &&
         expect(!second.changed, "same inspected unchanged") &&
         expect(second.appliedChange == cr::CreativeInspectionChangeKind::None,
                "same inspected no applied change") &&
         expect(second.inspectedTargetBefore.value == 7U,
                "same inspected before") &&
         expect(second.inspectedTargetAfter.value == 7U,
                "same inspected after");
}

bool clearingEmptyInspectionIsNoChange() {
  cr::CreativeInspectionState state = cr::makeDefaultCreativeInspectionState();
  const cr::CreativeInspectionReceipt receipt = cr::clearInspection(state);

  return expect(receipt.accepted, "clear empty accepted") &&
         expect(!receipt.changed, "clear empty unchanged") &&
         expect(receipt.inspectedTargetAfter.value == cr::kInvalidId,
                "clear empty inspected") &&
         expect(receipt.candidateTargetAfter.value == cr::kInvalidId,
                "clear empty candidate") &&
         expect(receipt.message == "inspection_already_empty",
                "clear empty message");
}

bool clearingNonEmptyInspectionClearsCandidate() {
  cr::CreativeInspectionState state = cr::makeDefaultCreativeInspectionState();
  const cr::CreativeInspectionReceipt inspected =
      cr::setInspectedTarget(state, target(7));
  const cr::CreativeInspectionReceipt candidate =
      cr::updateInspectionCandidate(state, target(9));
  const cr::CreativeInspectionReceipt cleared = cr::clearInspection(state);

  return expect(inspected.changed, "clear setup inspected") &&
         expect(candidate.changed, "clear setup candidate") &&
         expect(cleared.accepted, "clear non-empty accepted") &&
         expect(cleared.changed, "clear non-empty changed") &&
         expect(cleared.appliedChange ==
                    cr::CreativeInspectionChangeKind::ClearInspection,
                "clear non-empty applied") &&
         expect(cleared.inspectedTargetBefore.value == 7U,
                "clear non-empty inspected before") &&
         expect(cleared.candidateTargetBefore.value == 9U,
                "clear non-empty candidate before") &&
         expect(state.inspectedTarget.value == cr::kInvalidId,
                "clear non-empty inspected state") &&
         expect(state.candidateTarget.value == cr::kInvalidId,
                "clear non-empty candidate state");
}

bool updatingCandidateDoesNotMutateInspected() {
  cr::CreativeInspectionState state = cr::makeDefaultCreativeInspectionState();
  const cr::CreativeInspectionReceipt inspected =
      cr::setInspectedTarget(state, target(7));
  const cr::CreativeInspectionReceipt candidate =
      cr::updateInspectionCandidate(state, target(9));

  return expect(inspected.changed, "candidate setup inspected") &&
         expect(candidate.accepted, "candidate accepted") &&
         expect(candidate.changed, "candidate changed") &&
         expect(candidate.appliedChange ==
                    cr::CreativeInspectionChangeKind::UpdateCandidate,
                "candidate applied") &&
         expect(state.inspectedTarget.value == 7U,
                "candidate leaves inspected") &&
         expect(state.candidateTarget.value == 9U,
                "candidate updates candidate");
}

bool applyingInspectObjectCandidateInspectsTarget() {
  cr::CreativeInspectionState state = cr::makeDefaultCreativeInspectionState();
  const cr::CreativeInspectionReceipt receipt =
      cr::applyInspectionToolIntent(state, inspectIntent(target(42)));

  return expect(receipt.accepted, "inspect intent accepted") &&
         expect(receipt.changed, "inspect intent changed") &&
         expect(receipt.inspectedTargetBefore.value == cr::kInvalidId,
                "inspect intent before") &&
         expect(receipt.inspectedTargetAfter.value == 42U,
                "inspect intent after") &&
         expect(state.inspectedTarget.value == 42U,
                "inspect intent state inspected") &&
         expect(receipt.message == "inspect_candidate_inspected",
                "inspect intent message");
}

bool invalidInspectObjectCandidateClearsInspection() {
  cr::CreativeInspectionState state = cr::makeDefaultCreativeInspectionState();
  const cr::CreativeInspectionReceipt inspected =
      cr::setInspectedTarget(state, target(42));
  const cr::CreativeInspectionReceipt candidate =
      cr::updateInspectionCandidate(state, target(7));
  const cr::CreativeInspectionReceipt cleared =
      cr::applyInspectionToolIntent(state, inspectIntent({}));

  return expect(inspected.changed, "invalid setup inspected") &&
         expect(candidate.changed, "invalid setup candidate") &&
         expect(cleared.accepted, "invalid inspect accepted") &&
         expect(cleared.changed, "invalid inspect changed") &&
         expect(cleared.appliedChange ==
                    cr::CreativeInspectionChangeKind::ClearInspection,
                "invalid inspect clears") &&
         expect(cleared.inspectedTargetBefore.value == 42U,
                "invalid inspect inspected before") &&
         expect(cleared.candidateTargetBefore.value == 7U,
                "invalid inspect candidate before") &&
         expect(state.inspectedTarget.value == cr::kInvalidId,
                "invalid inspect inspected cleared") &&
         expect(state.candidateTarget.value == cr::kInvalidId,
                "invalid inspect candidate cleared") &&
         expect(cleared.message == "inspect_candidate_cleared_inspection",
                "invalid inspect message");
}

bool invalidInspectObjectCandidateOnEmptyIsNoChange() {
  cr::CreativeInspectionState state = cr::makeDefaultCreativeInspectionState();
  const cr::CreativeInspectionReceipt receipt =
      cr::applyInspectionToolIntent(state, inspectIntent({}));

  return expect(receipt.accepted, "invalid empty accepted") &&
         expect(!receipt.changed, "invalid empty unchanged") &&
         expect(receipt.appliedChange == cr::CreativeInspectionChangeKind::None,
                "invalid empty no applied change") &&
         expect(receipt.inspectedTargetAfter.value == cr::kInvalidId,
                "invalid empty inspected") &&
         expect(receipt.candidateTargetAfter.value == cr::kInvalidId,
                "invalid empty candidate") &&
         expect(receipt.message == "inspect_candidate_no_target",
                "invalid empty message");
}

bool nonInspectionIntentIsNoOp() {
  cr::CreativeInspectionState state = cr::makeDefaultCreativeInspectionState();
  const cr::CreativeInspectionReceipt inspected =
      cr::setInspectedTarget(state, target(42));

  cr::CreativeToolIntent intent;
  intent.kind = cr::CreativeToolIntentKind::PreviewPointer;
  intent.tool = cr::Tool::Inspect;
  intent.pointer.target = target(7);

  const cr::CreativeInspectionReceipt receipt =
      cr::applyInspectionToolIntent(state, intent);

  return expect(inspected.changed, "non-inspection setup inspected") &&
         expect(!receipt.accepted, "non-inspection not accepted") &&
         expect(!receipt.changed, "non-inspection unchanged") &&
         expect(receipt.appliedChange == cr::CreativeInspectionChangeKind::None,
                "non-inspection no applied change") &&
         expect(state.inspectedTarget.value == 42U,
                "non-inspection inspected preserved") &&
         expect(state.candidateTarget.value == cr::kInvalidId,
                "non-inspection candidate preserved") &&
         expect(receipt.message == "non_inspection_intent",
                "non-inspection message");
}

}  // namespace

int main() {
  const bool ok = defaultStateIsEmpty() &&
                  settingInspectedTargetChangesState() &&
                  settingSameInspectedTargetIsNoChange() &&
                  clearingEmptyInspectionIsNoChange() &&
                  clearingNonEmptyInspectionClearsCandidate() &&
                  updatingCandidateDoesNotMutateInspected() &&
                  applyingInspectObjectCandidateInspectsTarget() &&
                  invalidInspectObjectCandidateClearsInspection() &&
                  invalidInspectObjectCandidateOnEmptyIsNoChange() &&
                  nonInspectionIntentIsNoOp();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
