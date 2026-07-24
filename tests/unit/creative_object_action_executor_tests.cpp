#include "EditorObjectActionExecutor.hpp"

#include "EditorWorldLayout.hpp"
#include "EditorWorldLayoutHistory.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/history/History.hpp"
#include "app/iggy3d/creative/recipes/PatternRecipe.hpp"
#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"

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

cr::CreativeObjectId addGeneratedWorldLayoutObject(
    cr::CreativeAppState& appState,
    app::CreativeEditorWorldLayoutState& worldLayout,
    std::string_view layoutKey) {
  app::resetCreativeEditorWorldLayout(worldLayout, std::string(layoutKey));
  cr::CreativeWorldLayoutObject source;
  source.kind = cr::CreativeObjectKind::Crate;
  source.stableKey = "executor_crate";
  source.name = "Executor Crate";
  source.assetId = "crate";
  source.boundsCells = {{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}};
  worldLayout.source.objects.push_back(source);
  worldLayout.generatedBaseline =
      app::captureCreativeEditorWorldLayoutSnapshot(worldLayout);
  worldLayout.sourceHistory.current.snapshot =
      app::captureCreativeEditorWorldLayoutSnapshot(worldLayout);

  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Crate;
  request.name = "Compiled Executor Crate";
  request.tags = {
      cr::creativeWorldLayoutTag(worldLayout.source.stableKey),
      cr::creativeWorldLayoutProvenanceTag(
          worldLayout.source, cr::CreativeWorldLayoutTable::Object, 0U)};
  return appState.facade.createDocumentObject(request).objectId;
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

bool generatedSourceActionsReturnExplicitWorldLayoutEffects() {
  cr::CreativeAppState duplicateState =
      makeAppState("Executor Generated Duplicate", 7004U);
  app::CreativeEditorWorldLayoutState duplicateLayout;
  const cr::CreativeObjectId duplicateTarget =
      addGeneratedWorldLayoutObject(duplicateState, duplicateLayout,
                                    "executor_generated_duplicate");
  selectOne(duplicateState.facade, duplicateTarget);
  duplicateState.history = {};
  const app::CreativeEditorObjectActionExecution duplicated =
      app::executeCreativeEditorSceneObjectAction(
          {duplicateState, &duplicateLayout},
          {app::CreativeEditorDuplicateSelectionAction{},
           "executor_generated_duplicate"});

  cr::CreativeAppState deleteState =
      makeAppState("Executor Generated Delete", 7005U);
  app::CreativeEditorWorldLayoutState deleteLayout;
  const cr::CreativeObjectId deleteTarget =
      addGeneratedWorldLayoutObject(deleteState, deleteLayout,
                                    "executor_generated_delete");
  selectOne(deleteState.facade, deleteTarget);
  deleteState.history = {};
  const app::CreativeEditorObjectActionExecution deleted =
      app::executeCreativeEditorSceneObjectAction(
          {deleteState, &deleteLayout},
          {app::CreativeEditorDeleteSelectionAction{},
           "executor_generated_delete"});

  return expect(
             app::creativeEditorObjectActionOutcomeAccepted(
                 duplicated.outcome) &&
                 app::creativeEditorObjectActionOutcomeChanged(
                     duplicated.outcome) &&
                 duplicated.outcome.affectedObjectCount == 1U &&
                 duplicated.status == "duplicated selection" &&
                 app::creativeEditorObjectActionHasIntegrationImpact(
                     duplicated,
                     app::CreativeEditorObjectActionIntegrationImpact::
                         WorldLayoutSourceChanged) &&
                 app::creativeEditorObjectActionHasIntegrationImpact(
                     duplicated,
                     app::CreativeEditorObjectActionIntegrationImpact::
                         WorldLayoutSourceDuplicated) &&
                 !duplicated.selectionSynchronizationAttempted &&
                 duplicateLayout.source.objects.size() == 2U &&
                 duplicateState.facade.document().objectCount() == 1U &&
                 cr::creativeUndoDepth(duplicateState.history) == 0U,
             "generated duplicate reports source effects without document "
             "history") &&
         expect(
             app::creativeEditorObjectActionOutcomeAccepted(deleted.outcome) &&
                 app::creativeEditorObjectActionOutcomeChanged(
                     deleted.outcome) &&
                 deleted.outcome.affectedObjectCount == 1U &&
                 deleted.status == "deleted selection" &&
                 app::creativeEditorObjectActionHasIntegrationImpact(
                     deleted,
                     app::CreativeEditorObjectActionIntegrationImpact::
                         WorldLayoutSourceChanged) &&
                 app::creativeEditorObjectActionHasIntegrationImpact(
                     deleted,
                     app::CreativeEditorObjectActionIntegrationImpact::
                         WorldLayoutSourceDeleted) &&
                 deleted.selectionSynchronizationAttempted &&
                 deleted.selectionSynchronizationAccepted &&
                 deleteLayout.source.objects.empty() &&
                 deleteState.facade.document().objectCount() == 1U &&
                 cr::creativeUndoDepth(deleteState.history) == 0U,
             "generated delete reports source and selection effects without "
             "mutating compiled geometry");
}

bool patternOwnedDeleteRemovesRecipeClosureAtomically() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Executor Pattern Delete");
  static_cast<void>(document.assignId(7006U));
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Crate;
  request.name = "Pattern Source";
  const cr::CreativeObjectId sourceId =
      document.createObject(request).objectId;
  request.name = "Pattern Output A";
  const cr::CreativeObjectId outputA =
      document.createObject(request).objectId;
  request.name = "Pattern Output B";
  const cr::CreativeObjectId outputB =
      document.createObject(request).objectId;

  cr::CreativePatternRecipeMutationRequest addRecipe;
  addRecipe.kind = cr::CreativePatternRecipeMutationKind::Add;
  addRecipe.recipe.kind = cr::CreativePatternRecipeKind::LinearArray;
  addRecipe.recipe.sourceObjectIds = {sourceId};
  addRecipe.recipe.generatedObjectIds = {outputA, outputB};
  const cr::CreativePatternRecipeMutationReceipt recipe =
      document.applyPatternRecipeMutation(addRecipe);

  cr::CreativeAppState appState;
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  selectOne(appState.facade, outputA);
  appState.history = {};
  const app::CreativeEditorObjectActionExecution deleted =
      app::executeCreativeEditorSceneObjectAction(
          {appState},
          {app::CreativeEditorDeleteSelectionAction{},
           "executor_pattern_delete"});

  return expect(recipe.accepted && recipe.changed,
                "pattern executor fixture is valid") &&
         expect(
             app::creativeEditorObjectActionOutcomeAccepted(deleted.outcome) &&
                 app::creativeEditorObjectActionOutcomeChanged(
                     deleted.outcome) &&
                 deleted.outcome.affectedObjectCount == 3U &&
                 deleted.status == "deleted selection" &&
                 appState.facade.document().objectCount() == 0U &&
                 appState.facade.document()
                     .patternRecipeStore()
                     .recipes.empty() &&
                 cr::creativeUndoDepth(appState.history) == 1U,
             "pattern output delete removes the recipe closure in one undo");
}

}  // namespace

int main() {
  const bool ok = duplicateAndDeleteShareOutcomeAndSelectionEffects() &&
                  transformVariantsPreserveCountsHistoryAndUnchangedStatus() &&
                  lockedSelectionAndInvalidAbsoluteTransformFailClosed() &&
                  generatedSourceActionsReturnExplicitWorldLayoutEffects() &&
                  patternOwnedDeleteRemovesRecipeClosureAtomically();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
