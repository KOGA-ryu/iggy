#include "app/iggy3d/creative/world/WorldLayoutCompileInternal.hpp"

#include <algorithm>
#include <limits>
#include <string_view>
#include <utility>
#include <vector>

namespace iggy3d::creative::world_layout_compile {

bool reconcileWorldLayoutRecipes(
    const CreativeDocument& document,
    std::string_view layoutTag,
    CreativeWorldLayoutCompileOptions options,
    std::vector<CreativeRecipePlan>& desiredObjectRecipes,
  CreativeWorldLayoutCompileResult& result) {
  CreativeWorldLayoutReconciliationResult reconciliation =
      reconcileCreativeWorldLayoutRecipes(
          {&document, layoutTag, desiredObjectRecipes,
           options.conflictDecisions});
  result.recipeChanges = std::move(reconciliation.changes);
  result.receipt.objectRecipeCreateCount =
      reconciliation.createRecipeCount;
  result.receipt.objectRecipeKeepCount = reconciliation.keepRecipeCount;
  result.receipt.objectRecipeRefinedCount =
      reconciliation.refinedRecipeCount;
  result.receipt.objectRecipePatchCount =
      reconciliation.patchRecipeCount;
  result.receipt.objectRecipeReplaceCount =
      reconciliation.replaceRecipeCount;
  result.receipt.objectRecipeConflictCount =
      reconciliation.conflictRecipeCount;
  result.receipt.objectRecipeDetachCount =
      reconciliation.detachRecipeCount;
  if (!reconciliation.accepted) {
    setStatus(result.receipt,
              reconciliation.blocked
                  ? CreativeWorldLayoutStatus::RefinementConflict
                  : CreativeWorldLayoutStatus::InvalidDocument,
              reconciliation.reasonCode);
    return false;
  }

  result.plan.objectDetachIds =
      std::move(reconciliation.detachObjectIds);
  if (!collectObjectRemovalOrder(document,
                                 reconciliation.removeObjectIds,
                                 result.plan.objectRemoveIds)) {
    setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidDocument,
              "creative_world_layout_owned_object_graph_invalid");
    return false;
  }
  std::vector<bool> claimedRecipes(desiredObjectRecipes.size(), false);
  result.plan.objectRecipePatches.reserve(
      reconciliation.patchDecisions.size());
  for (CreativeWorldLayoutRecipePatchDecision& decision :
       reconciliation.patchDecisions) {
    if (decision.desiredRecipeIndex >= desiredObjectRecipes.size() ||
        decision.existingObjectIds.size() !=
            desiredObjectRecipes[decision.desiredRecipeIndex].objects.size() ||
        decision.memberActions.size() !=
            desiredObjectRecipes[decision.desiredRecipeIndex].objects.size() ||
        claimedRecipes[decision.desiredRecipeIndex]) {
      setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidDocument,
                "creative_world_layout_reconciliation_patch_invalid");
      return false;
    }
    claimedRecipes[decision.desiredRecipeIndex] = true;
    CreativeWorldLayoutRecipePatch patch;
    patch.recipe =
        std::move(desiredObjectRecipes[decision.desiredRecipeIndex]);
    patch.objectIds = std::move(decision.existingObjectIds);
    patch.memberActions = std::move(decision.memberActions);
    result.plan.objectRecipePatches.push_back(std::move(patch));
  }
  result.plan.objectRecipes.reserve(
      reconciliation.applyRecipeIndices.size());
  for (const std::size_t recipeIndex : reconciliation.applyRecipeIndices) {
    if (recipeIndex >= desiredObjectRecipes.size() ||
        claimedRecipes[recipeIndex]) {
      setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidDocument,
                "creative_world_layout_reconciliation_index_invalid");
      return false;
    }
    claimedRecipes[recipeIndex] = true;
    result.plan.objectRecipes.push_back(
        std::move(desiredObjectRecipes[recipeIndex]));
  }

  CreativeObjectId nextObjectId = document.nextObjectId();
  for (std::size_t patchIndex = 0U;
       patchIndex < result.plan.objectRecipePatches.size(); ++patchIndex) {
    CreativeWorldLayoutRecipePatch& patch =
        result.plan.objectRecipePatches[patchIndex];
    const std::size_t createCount = static_cast<std::size_t>(std::count(
        patch.memberActions.begin(), patch.memberActions.end(),
        CreativeWorldLayoutRecipeMemberAction::Create));
    if (nextObjectId == kInvalidObjectId ||
        createCount > std::numeric_limits<CreativeObjectId>::max() -
                          nextObjectId) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Object;
      result.receipt.failedIndex = patchIndex;
      result.receipt.kernelReasonCode =
          "creative_recipe_object_id_overflow";
      setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                "creative_world_layout_object_materialize_rejected");
      return false;
    }
    for (std::size_t objectIndex = 0U;
         objectIndex < patch.objectIds.size(); ++objectIndex) {
      const CreativeWorldLayoutRecipeMemberAction action =
          patch.memberActions[objectIndex];
      if (action == CreativeWorldLayoutRecipeMemberAction::Create) {
        if (patch.objectIds[objectIndex] != kInvalidObjectId) {
          result.receipt.failedTable = CreativeWorldLayoutTable::Object;
          result.receipt.failedIndex = patchIndex;
          result.receipt.kernelReasonCode =
              "creative_world_layout_patch_create_id_present";
          setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                    "creative_world_layout_object_materialize_rejected");
          return false;
        }
        patch.objectIds[objectIndex] = nextObjectId++;
      } else if ((action !=
                      CreativeWorldLayoutRecipeMemberAction::Preserve &&
                  action != CreativeWorldLayoutRecipeMemberAction::Update) ||
                 patch.objectIds[objectIndex] == kInvalidObjectId) {
        result.receipt.failedTable = CreativeWorldLayoutTable::Object;
        result.receipt.failedIndex = patchIndex;
        result.receipt.kernelReasonCode =
            "creative_world_layout_patch_member_invalid";
        setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                  "creative_world_layout_object_materialize_rejected");
        return false;
      }
    }
    const CreativeRecipeMaterializeResult validated =
        materializeCreativeRecipe(patch.recipe, patch.objectIds);
    if (!validated.receipt.accepted) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Object;
      result.receipt.failedIndex = patchIndex;
      result.receipt.kernelReasonCode = validated.receipt.reasonCode;
      setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                "creative_world_layout_object_materialize_rejected");
      return false;
    }
    result.receipt.objectCount += validated.createRequests.size();
  }
  for (std::size_t index = 0U; index < result.plan.objectRecipes.size();
       ++index) {
    const CreativeRecipeMaterializeResult validated =
        materializeCreativeRecipe(result.plan.objectRecipes[index],
                                  nextObjectId);
    if (!validated.receipt.accepted) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Object;
      result.receipt.failedIndex = index;
      result.receipt.kernelReasonCode = validated.receipt.reasonCode;
      setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                "creative_world_layout_object_materialize_rejected");
      return false;
    }
    nextObjectId +=
        static_cast<CreativeObjectId>(validated.createRequests.size());
    result.receipt.objectCount += validated.createRequests.size();
  }
  return true;
}

}  // namespace iggy3d::creative::world_layout_compile
