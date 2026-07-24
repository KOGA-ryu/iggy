#include "EditorObjectActionExecutor.hpp"

#include "EditorWorldLayout.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/history/History.hpp"

#include <cstdlib>
#include <iostream>
#include <span>
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

cr::CreativeObjectId createCrate(cr::Facade& facade,
                                 double x,
                                 bool locked = false) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Crate;
  request.transform.position = {x, 0.0, 0.0};
  request.hasTransformOverride = true;
  request.locked = locked;
  request.hasLockedOverride = true;
  return facade.createDocumentObject(request).objectId;
}

void selectOne(cr::Facade& facade, cr::CreativeObjectId objectId) {
  static_cast<void>(facade.selectTargets(
      std::span<const cr::CreativeObjectId>{&objectId, 1U}, objectId));
}

cr::CreativeAppState makeAppState(std::string_view name,
                                  cr::CreativeDocumentId id) {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create(std::string(name));
  static_cast<void>(document.assignId(id));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  return appState;
}

bool duplicateAndDeleteShareOutcomeAndSelectionEffects() {
  cr::CreativeAppState appState = makeAppState("Executor Duplicate", 7001U);
  const cr::CreativeObjectId original = createCrate(appState.facade, 0.0);
  selectOne(appState.facade, original);
  appState.history = {};
  app::CreativeEditorWorldLayoutState worldLayout;

  const app::CreativeEditorObjectActionExecution duplicated =
      app::executeCreativeEditorSceneObjectAction(
          {appState, &worldLayout},
          {app::CreativeEditorDuplicateSelectionAction{},
           "executor_duplicate"});
  const cr::CreativeObjectId duplicateId =
      static_cast<cr::CreativeObjectId>(
          appState.facade.selectionState().selectedTarget.value);
  const bool duplicateOk =
      app::creativeEditorObjectActionOutcomeAccepted(duplicated.outcome) &&
      app::creativeEditorObjectActionOutcomeChanged(duplicated.outcome) &&
      duplicated.outcome.action ==
          cr::CreativeSemanticObjectAction::Duplicate &&
      duplicated.outcome.affectedObjectCount == 1U &&
      duplicated.status == "duplicated selection" &&
      duplicated.selectionSynchronizationAttempted &&
      duplicated.selectionSynchronizationAccepted &&
      app::creativeEditorObjectActionHasIntegrationImpact(
          duplicated,
          app::CreativeEditorObjectActionIntegrationImpact::
              SelectionSynchronized) &&
      duplicateId != cr::kInvalidObjectId && duplicateId != original &&
      appState.facade.document().objectCount() == 2U &&
      cr::creativeUndoDepth(appState.history) == 1U;

  const app::CreativeEditorObjectActionExecution deleted =
      app::executeCreativeEditorSceneObjectAction(
          {appState, &worldLayout},
          {app::CreativeEditorDeleteSelectionAction{}, "executor_delete"});
  return expect(duplicateOk,
                "duplicate returns canonical outcome and selection effects") &&
         expect(
             app::creativeEditorObjectActionOutcomeAccepted(deleted.outcome) &&
                 app::creativeEditorObjectActionOutcomeChanged(
                     deleted.outcome) &&
                 deleted.outcome.action ==
                     cr::CreativeSemanticObjectAction::Delete &&
                 deleted.outcome.affectedObjectCount == 1U &&
                 deleted.status == "deleted selection" &&
                 deleted.selectionSynchronizationAttempted &&
                 deleted.selectionSynchronizationAccepted &&
                 appState.facade.document().objectCount() == 1U &&
                 appState.facade.findObject(duplicateId) == nullptr &&
                 cr::creativeUndoDepth(appState.history) == 2U,
             "delete returns canonical outcome and one additional undo step");
}

bool transformVariantsPreserveCountsHistoryAndUnchangedStatus() {
  cr::CreativeAppState appState = makeAppState("Executor Transform", 7002U);
  const cr::CreativeObjectId objectId = createCrate(appState.facade, 2.0);
  selectOne(appState.facade, objectId);
  appState.history = {};

  cr::CreativeTransformCommandRequest rotate;
  rotate.kind = cr::CreativeTransformCommandKind::RotateYaw;
  rotate.yawDegrees = 90.0;
  const app::CreativeEditorObjectActionExecution rotated =
      app::executeCreativeEditorSceneObjectAction(
          {appState},
          {app::CreativeEditorTransformSelectionAction{rotate},
           "executor_rotate"});

  cr::CreativeTransformCommandRequest scale;
  scale.kind = cr::CreativeTransformCommandKind::Scale;
  scale.scaleFactor = {1.25, 1.25, 1.25};
  const app::CreativeEditorObjectActionExecution scaled =
      app::executeCreativeEditorSceneObjectAction(
          {appState},
          {app::CreativeEditorTransformSelectionAction{scale},
           "executor_scale"});

  cr::CreativeTransformCommandRequest reset;
  reset.kind = cr::CreativeTransformCommandKind::ResetRotationScale;
  const app::CreativeEditorObjectActionExecution resetApplied =
      app::executeCreativeEditorSceneObjectAction(
          {appState},
          {app::CreativeEditorTransformSelectionAction{reset},
           "executor_reset"});
  const app::CreativeEditorObjectActionExecution resetUnchanged =
      app::executeCreativeEditorSceneObjectAction(
          {appState},
          {app::CreativeEditorTransformSelectionAction{reset},
           "executor_reset_again"});
  const cr::CreativeObject* object = appState.facade.findObject(objectId);

  return expect(
             rotated.outcome.status ==
                     app::CreativeEditorObjectActionOutcomeStatus::Applied &&
                 rotated.outcome.affectedObjectCount == 1U &&
                 rotated.status == "transform set" &&
                 scaled.outcome.status ==
                     app::CreativeEditorObjectActionOutcomeStatus::Applied &&
                 scaled.outcome.affectedObjectCount == 1U &&
                 resetApplied.outcome.status ==
                     app::CreativeEditorObjectActionOutcomeStatus::Applied &&
                 resetApplied.outcome.affectedObjectCount == 1U &&
                 cr::creativeUndoDepth(appState.history) == 3U,
             "rotate scale and reset each produce one typed applied outcome") &&
         expect(
             resetUnchanged.outcome.status ==
                     app::CreativeEditorObjectActionOutcomeStatus::Unchanged &&
                 resetUnchanged.status == "transform unchanged" &&
                 cr::creativeUndoDepth(appState.history) == 3U &&
                 object != nullptr &&
                 cr::creativeVec3ExactlyEqual(
                     object->transform.rotationEulerRadians, {}) &&
                 cr::creativeVec3ExactlyEqual(object->transform.scale,
                                              {1.0, 1.0, 1.0}),
             "unchanged reset adds no history and preserves identity");
}

bool lockedSelectionAndInvalidAbsoluteTransformFailClosed() {
  cr::CreativeAppState appState = makeAppState("Executor Rejection", 7003U);
  const cr::CreativeObjectId locked =
      createCrate(appState.facade, 0.0, true);
  selectOne(appState.facade, locked);
  appState.history = {};
  const app::CreativeEditorObjectActionExecution deleted =
      app::executeCreativeEditorSceneObjectAction(
          {appState},
          {app::CreativeEditorDeleteSelectionAction{},
           "executor_locked_delete"});

  app::CreativeEditorSetObjectTransformAction invalid;
  invalid.objectId = 999999U;
  invalid.setPosition = true;
  const app::CreativeEditorObjectActionExecution transformed =
      app::executeCreativeEditorSceneObjectAction(
          {appState}, {invalid, "executor_missing_transform"});

  return expect(
             !app::creativeEditorObjectActionOutcomeAccepted(deleted.outcome) &&
                 deleted.outcome.reasonCode ==
                     "creative_editor_object_action_selection_locked" &&
                 deleted.status == "selection is locked" &&
                 deleted.outcome.affectedObjectCount == 0U &&
                 appState.facade.findObject(locked) != nullptr &&
                 cr::creativeUndoDepth(appState.history) == 0U,
             "locked selection rejection is stable and atomic") &&
         expect(
             !app::creativeEditorObjectActionOutcomeAccepted(
                 transformed.outcome) &&
                 transformed.status ==
                     "part of the selection is unavailable" &&
                 transformed.outcome.reasonCode ==
                     "creative_selection_set_object_missing" &&
                 transformed.outcome.affectedObjectCount == 0U &&
                 cr::creativeUndoDepth(appState.history) == 0U,
             "invalid absolute transform is rejected without history");
}

}  // namespace

int main() {
  const bool ok = duplicateAndDeleteShareOutcomeAndSelectionEffects() &&
                  transformVariantsPreserveCountsHistoryAndUnchangedStatus() &&
                  lockedSelectionAndInvalidAbsoluteTransformFailClosed();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
