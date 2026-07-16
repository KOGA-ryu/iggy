#include "app/iggy3d/creative/recipes/CreativeRecipe.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeRecipePlan parentedRecipe() {
  cr::CreativeRecipePlan plan;
  plan.kind = cr::CreativeRecipeKind::Building;
  plan.instanceKey = "recipe_house";
  plan.instanceName = "Recipe House";

  cr::CreativeRecipeObjectPlan root;
  root.createRequest.kind = cr::CreativeObjectKind::Room;
  root.createRequest.name = "Recipe House";
  root.role = cr::CreativeRecipeObjectRole::Source;
  root.stableKey = "root";
  plan.objects.push_back(root);

  cr::CreativeRecipeObjectPlan floor;
  floor.createRequest.kind = cr::CreativeObjectKind::Floor;
  floor.createRequest.name = "Recipe Floor";
  floor.role = cr::CreativeRecipeObjectRole::Generated;
  floor.stableKey = "floor.main";
  floor.parentObjectIndex = 0U;
  plan.objects.push_back(floor);
  return plan;
}

cr::CreativeAppState appStateWithDocument(cr::CreativeDocumentId id) {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Recipe Test");
  static_cast<void>(document.assignId(id));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  return appState;
}

bool symbolicParentAndProvenanceMaterializeDeterministically() {
  const cr::CreativeRecipeMaterializeResult result =
      cr::materializeCreativeRecipe(parentedRecipe(), 40U);

  return expect(result.receipt.accepted, "recipe materialization accepted") &&
         expect(result.receipt.status == cr::CreativeRecipeStatus::Ready,
                "recipe materialization ready") &&
         expect(result.receipt.objectCount == 2U,
                "recipe materialization object count") &&
         expect(result.receipt.sourceObjectCount == 1U &&
                    result.receipt.generatedObjectCount == 1U,
                "recipe materialization role counts") &&
         expect(result.receipt.resolvedParentCount == 1U,
                "recipe materialization parent count") &&
         expect(result.createRequests.size() == 2U,
                "recipe materialization request count") &&
         expect(!result.createRequests[0].parentId.has_value(),
                "recipe source remains root") &&
         expect(result.createRequests[1].parentId == 40U,
                "recipe child resolves source id") &&
         expect(cr::creativeRecipeRequestHasProvenance(
                    result.createRequests[0], cr::CreativeRecipeKind::Building,
                    cr::CreativeRecipeObjectRole::Source, "root"),
                "recipe source provenance") &&
         expect(cr::creativeRecipeRequestHasProvenance(
                    result.createRequests[1], cr::CreativeRecipeKind::Building,
                    cr::CreativeRecipeObjectRole::Generated, "floor.main"),
                "recipe generated provenance") &&
         expect(cr::creativeRecipeRequestHasInstanceProvenance(
                    result.createRequests[1], cr::CreativeRecipeKind::Building,
                    "recipe_house", cr::CreativeRecipeObjectRole::Generated,
                    "floor.main"),
                "recipe instance provenance");
}

bool invalidKeysParentsAndAllocatorOverflowFailClosed() {
  cr::CreativeRecipePlan duplicate = parentedRecipe();
  duplicate.objects[1].stableKey = "root";
  const cr::CreativeRecipeMaterializeResult duplicateResult =
      cr::materializeCreativeRecipe(duplicate, 1U);

  cr::CreativeRecipePlan forward = parentedRecipe();
  forward.objects[0].parentObjectIndex = 1U;
  const cr::CreativeRecipeMaterializeResult forwardResult =
      cr::materializeCreativeRecipe(forward, 1U);

  const cr::CreativeRecipeMaterializeResult overflowResult =
      cr::materializeCreativeRecipe(
          parentedRecipe(), std::numeric_limits<cr::CreativeObjectId>::max());
  cr::CreativeRecipePlan finalId = parentedRecipe();
  finalId.objects.resize(1U);
  const cr::CreativeRecipeMaterializeResult finalIdResult =
      cr::materializeCreativeRecipe(
          finalId, std::numeric_limits<cr::CreativeObjectId>::max());

  return expect(!duplicateResult.receipt.accepted &&
                    duplicateResult.receipt.status ==
                        cr::CreativeRecipeStatus::DuplicateStableKey,
                "duplicate recipe key rejected") &&
         expect(duplicateResult.createRequests.empty(),
                "duplicate recipe emits no partial requests") &&
         expect(!forwardResult.receipt.accepted &&
                    forwardResult.receipt.status ==
                        cr::CreativeRecipeStatus::InvalidParentReference,
                "forward recipe parent rejected") &&
         expect(!overflowResult.receipt.accepted &&
                    overflowResult.receipt.status ==
                        cr::CreativeRecipeStatus::ObjectIdOverflow,
                "recipe allocator overflow rejected") &&
         expect(!finalIdResult.receipt.accepted &&
                    finalIdResult.receipt.status ==
                        cr::CreativeRecipeStatus::ObjectIdOverflow,
                "recipe allocator preserves invalid-id sentinel after create");
}

bool historyApplyIsAtomicAndCreatesOneUndoStep() {
  cr::CreativeAppState appState = appStateWithDocument(71U);
  const cr::CreativeRecipeApplyReceipt applied =
      cr::applyCreativeRecipeWithHistory(appState, parentedRecipe(),
                                         "recipe_test");
  const cr::CreativeObject* root = appState.facade.document().findObject(1U);
  const cr::CreativeObject* floor = appState.facade.document().findObject(2U);
  const bool hierarchyCommitted = floor != nullptr && floor->parentId == 1U;
  const bool sourceProvenanceCommitted =
      root != nullptr && cr::creativeRecipeObjectHasProvenance(
                             *root, cr::CreativeRecipeKind::Building,
                             cr::CreativeRecipeObjectRole::Source, "root");
  const cr::CreativeHistoryApplyReceipt undone = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Undo);

  return expect(applied.accepted && applied.changed,
                "recipe history apply accepted") &&
         expect(applied.status == cr::CreativeRecipeStatus::Applied,
                "recipe history apply status") &&
         expect(applied.createReceipt.appliedCreateCount == 2U,
                "recipe atomic create count") &&
         expect(applied.historyReceipt.recorded &&
                    cr::creativeUndoDepth(appState.history) == 0U,
                "recipe undo consumes sole snapshot") &&
         expect(root != nullptr && floor != nullptr,
                "recipe objects existed before undo") &&
         expect(hierarchyCommitted, "recipe hierarchy committed") &&
         expect(sourceProvenanceCommitted,
                "recipe committed source provenance") &&
         expect(undone.accepted && undone.changed,
                "recipe one-step undo accepted") &&
         expect(appState.facade.document().objectCount() == 0U,
                "recipe undo removes complete transaction") &&
         expect(cr::creativeRedoDepth(appState.history) == 1U,
                "recipe undo creates one redo snapshot");
}

bool rejectedAtomicApplyPreservesDocumentAndHistory() {
  cr::CreativeAppState appState = appStateWithDocument(72U);
  cr::CreativeRecipePlan plan = parentedRecipe();
  plan.objects.erase(plan.objects.begin());
  plan.objects[0].parentObjectIndex.reset();
  plan.objects[0].createRequest.parentId = 999U;

  const cr::CreativeRecipeApplyReceipt applied =
      cr::applyCreativeRecipeWithHistory(appState, plan, "recipe_reject");
  return expect(!applied.accepted && !applied.changed,
                "rejected recipe not accepted") &&
         expect(applied.status == cr::CreativeRecipeStatus::ApplyRejected,
                "rejected recipe status") &&
         expect(appState.facade.document().objectCount() == 0U &&
                    appState.facade.document().revision() == 0U,
                "rejected recipe preserves document") &&
         expect(cr::creativeUndoDepth(appState.history) == 0U &&
                    cr::creativeRedoDepth(appState.history) == 0U,
                "rejected recipe records no history");
}

}  // namespace

int main() {
  const bool ok =
      symbolicParentAndProvenanceMaterializeDeterministically() &&
      invalidKeysParentsAndAllocatorOverflowFailClosed() &&
      historyApplyIsAtomicAndCreatesOneUndoStep() &&
      rejectedAtomicApplyPreservesDocumentAndHistory();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
