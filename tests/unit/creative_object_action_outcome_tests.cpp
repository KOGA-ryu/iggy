#include "EditorObjectActionOutcome.hpp"

#include <array>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

namespace app = iggy3d_creative_app;
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool defaultOutcomeIsEmptyAndValid() {
  const app::CreativeEditorObjectActionOutcome outcome;
  return expect(app::creativeEditorObjectActionOutcomeValid(outcome),
                "default outcome is a valid empty state") &&
         expect(!app::creativeEditorObjectActionOutcomeRequested(outcome) &&
                    !app::creativeEditorObjectActionOutcomeAccepted(outcome) &&
                    !app::creativeEditorObjectActionOutcomeChanged(outcome) &&
                    app::formatCreativeEditorObjectActionOutcome(outcome)
                        .empty(),
                "default outcome reports no operation or presentation");
}

bool receiptFactsProduceClosedTypedStatuses() {
  using Action = cr::CreativeSemanticObjectAction;
  using Status = app::CreativeEditorObjectActionOutcomeStatus;
  using Target = app::CreativeEditorObjectActionTarget;
  struct Case {
    Action action = Action::Count;
    Target target = Target::None;
    bool accepted = false;
    bool changed = false;
    std::uint64_t affectedObjectCount = 0U;
    bool requiresAdoption = false;
    Status expectedStatus = Status::NotRequested;
    std::string_view expectedMessage;
  };
  constexpr std::array cases{
      Case{Action::Duplicate, Target::Selection, true, true, 2U, false,
           Status::Applied, "duplicated selection"},
      Case{Action::Duplicate, Target::Selection, true, false, 0U, false,
           Status::Unchanged, "nothing to duplicate"},
      Case{Action::Delete, Target::Selection, true, true, 3U, false,
           Status::Applied, "deleted selection"},
      Case{Action::Delete, Target::Objects, true, true, 3U, false,
           Status::Applied, "deleted objects"},
      Case{Action::Rename, Target::Object, true, false, 0U, false,
           Status::Unchanged, "renamed object"},
      Case{Action::SetVisible, Target::Objects, true, true, 2U, false,
           Status::Applied, "visibility updated"},
      Case{Action::SetLocked, Target::Objects, true, true, 2U, false,
           Status::Applied, "lock updated"},
      Case{Action::SetTransform, Target::Object, true, false, 0U, false,
           Status::Unchanged, "transform unchanged"},
      Case{Action::SetTransform, Target::Object, true, true, 1U, false,
           Status::Applied, "transform set"},
      Case{Action::SetTransform, Target::Object, true, true, 1U, true,
           Status::AppliedRequiresAdoption,
           "transform set; adopt 3D edit"},
  };

  bool ok = true;
  for (const Case& testCase : cases) {
    const app::CreativeEditorObjectActionOutcome outcome =
        app::makeCreativeEditorObjectActionOutcome(
            testCase.action, testCase.target, testCase.accepted,
            testCase.changed, testCase.affectedObjectCount,
            "fixture_reason", testCase.requiresAdoption);
    ok = expect(
             outcome.status == testCase.expectedStatus &&
                 outcome.action == testCase.action &&
                 outcome.target == testCase.target &&
                 outcome.affectedObjectCount ==
                     testCase.affectedObjectCount &&
                 app::creativeEditorObjectActionOutcomeValid(outcome) &&
                 app::creativeEditorObjectActionOutcomeAccepted(outcome) ==
                     testCase.accepted &&
                 app::creativeEditorObjectActionOutcomeChanged(outcome) ==
                     testCase.changed &&
                 app::formatCreativeEditorObjectActionOutcome(outcome) ==
                     testCase.expectedMessage,
             "receipt facts map to one valid typed outcome row") &&
         ok;
  }

  const app::CreativeEditorObjectActionOutcome rejected =
      app::makeCreativeEditorObjectActionOutcome(
          Action::Delete, Target::Selection, false, false, 0U,
          "creative_editor_object_action_selection_locked");
  return expect(
             rejected.status == Status::Rejected &&
                 !app::creativeEditorObjectActionOutcomeAccepted(rejected) &&
                 !app::creativeEditorObjectActionOutcomeChanged(rejected) &&
                 app::creativeEditorObjectActionOutcomeValid(rejected) &&
                 app::formatCreativeEditorObjectActionOutcome(rejected) ==
                     "selection is locked" &&
                 rejected.reasonCode ==
                     "creative_editor_object_action_selection_locked",
             "rejected receipt separates stable reason from presentation") &&
         ok;
}

bool requestFailuresFormatAtThePresentationBoundary() {
  using Action = cr::CreativeSemanticObjectAction;
  using Status = app::CreativeEditorObjectActionOutcomeStatus;
  using Target = app::CreativeEditorObjectActionTarget;
  const app::CreativeEditorObjectActionOutcome deleteMismatch =
      app::rejectCreativeEditorObjectAction(
          Action::Delete, Target::Objects, Status::PayloadMismatch,
          "creative_desktop_delete_objects_payload_mismatch");
  const app::CreativeEditorObjectActionOutcome renameInvalid =
      app::rejectCreativeEditorObjectAction(
          Action::Rename, Target::Object, Status::InvalidRequest,
          "creative_desktop_rename_invalid_request");
  const app::CreativeEditorObjectActionOutcome transformContainer =
      app::rejectCreativeEditorObjectAction(
          Action::SetTransform, Target::Object, Status::InvalidRequest,
          "creative_editor_transform_container_requires_selection_transform");

  return expect(
             app::creativeEditorObjectActionOutcomeValid(deleteMismatch) &&
                 app::creativeEditorObjectActionOutcomeValid(renameInvalid) &&
                 app::creativeEditorObjectActionOutcomeValid(
                     transformContainer),
             "request failures remain valid typed outcomes") &&
         expect(app::formatCreativeEditorObjectActionOutcome(deleteMismatch) ==
                        "delete objects: payload mismatch" &&
                    app::formatCreativeEditorObjectActionOutcome(renameInvalid) ==
                        "rename: invalid request" &&
                    app::formatCreativeEditorObjectActionOutcome(
                        transformContainer) ==
                        "transform complete hierarchy through Transform "
                        "Selection",
                "request failures gain human text only in the formatter") &&
         expect(app::toString(Status::PayloadMismatch) ==
                        "payload_mismatch" &&
                    app::toString(Status::InvalidRequest) ==
                        "invalid_request" &&
                    app::toString(Status::AppliedRequiresAdoption) ==
                        "applied_requires_adoption",
                "typed statuses expose stable diagnostic labels");
}

bool malformedOutcomesFailValidation() {
  using Action = cr::CreativeSemanticObjectAction;
  using Status = app::CreativeEditorObjectActionOutcomeStatus;
  using Target = app::CreativeEditorObjectActionTarget;
  app::CreativeEditorObjectActionOutcome missingTarget{
      Action::Delete, Target::None, Status::Applied, 1U, {}};
  app::CreativeEditorObjectActionOutcome emptyApply{
      Action::Delete, Target::Selection, Status::Applied, 0U, {}};
  app::CreativeEditorObjectActionOutcome invalidAdoption{
      Action::Rename, Target::Object, Status::AppliedRequiresAdoption, 1U, {}};
  app::CreativeEditorObjectActionOutcome rejectedWithAffected{
      Action::Delete, Target::Objects, Status::Rejected, 1U, "rejected"};

  return expect(!app::creativeEditorObjectActionOutcomeValid(missingTarget) &&
                    !app::creativeEditorObjectActionOutcomeValid(emptyApply) &&
                    !app::creativeEditorObjectActionOutcomeValid(
                        invalidAdoption) &&
                    !app::creativeEditorObjectActionOutcomeValid(
                        rejectedWithAffected),
                "malformed outcome combinations fail closed");
}

}  // namespace

int main() {
  const bool ok = defaultOutcomeIsEmptyAndValid() &&
                  receiptFactsProduceClosedTypedStatuses() &&
                  requestFailuresFormatAtThePresentationBoundary() &&
                  malformedOutcomesFailValidation();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
