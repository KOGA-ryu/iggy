#include "app/iggy3d/creative/document/Document.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace iggy3d::creative {
namespace {

void setStatus(CreativePatternRecipeMutationReceipt& receipt,
               CreativePatternRecipeMutationStatus status,
               std::string_view reasonCode) noexcept {
  receipt.status = status;
  receipt.reasonCode = reasonCode;
}

}  // namespace

CreativePatternRecipeMutationReceipt
CreativeDocument::applyPatternRecipeMutation(
    const CreativePatternRecipeMutationRequest& request) {
  CreativePatternRecipeMutationReceipt receipt;
  receipt.requested = true;
  receipt.kind = request.kind;
  receipt.recipeId = request.recipeId;
  receipt.recipeCountBefore = patternRecipeStore_.recipes.size();
  receipt.recipeCountAfter = receipt.recipeCountBefore;
  receipt.revisionBefore = revision_;
  receipt.revisionAfter = revision_;
  if (!valid_) {
    setStatus(receipt,
              CreativePatternRecipeMutationStatus::InvalidDocument,
              "creative_pattern_recipe_document_invalid");
    return receipt;
  }
  if (!validateCreativePatternRecipeStore(patternRecipeStore_)) {
    setStatus(receipt, CreativePatternRecipeMutationStatus::InvalidStore,
              "creative_pattern_recipe_store_invalid");
    return receipt;
  }
  if (request.kind >= CreativePatternRecipeMutationKind::Count) {
    setStatus(receipt, CreativePatternRecipeMutationStatus::InvalidRequest,
              "creative_pattern_recipe_mutation_kind_invalid");
    return receipt;
  }

  CreativePatternRecipeStore staged = patternRecipeStore_;
  bool changed = false;
  switch (request.kind) {
    case CreativePatternRecipeMutationKind::Add: {
      if (staged.recipes.size() >= kCreativePatternRecipeCapacity) {
        setStatus(receipt,
                  CreativePatternRecipeMutationStatus::CapacityExceeded,
                  "creative_pattern_recipe_capacity_exceeded");
        return receipt;
      }
      if (staged.nextRecipeId ==
          std::numeric_limits<CreativePatternRecipeId>::max()) {
        setStatus(receipt, CreativePatternRecipeMutationStatus::IdExhausted,
                  "creative_pattern_recipe_id_exhausted");
        return receipt;
      }
      CreativePatternRecipe added = request.recipe;
      added.id = staged.nextRecipeId++;
      receipt.recipeId = added.id;
      staged.recipes.push_back(std::move(added));
      changed = true;
      break;
    }
    case CreativePatternRecipeMutationKind::Replace: {
      const auto found = std::find_if(
          staged.recipes.begin(), staged.recipes.end(),
          [&request](const CreativePatternRecipe& recipe) {
            return recipe.id == request.recipeId;
          });
      if (found == staged.recipes.end()) {
        setStatus(receipt, CreativePatternRecipeMutationStatus::NotFound,
                  "creative_pattern_recipe_not_found");
        return receipt;
      }
      CreativePatternRecipe replacement = request.recipe;
      replacement.id = request.recipeId;
      changed = *found != replacement;
      *found = std::move(replacement);
      break;
    }
    case CreativePatternRecipeMutationKind::Detach: {
      const auto found = std::find_if(
          staged.recipes.begin(), staged.recipes.end(),
          [&request](const CreativePatternRecipe& recipe) {
            return recipe.id == request.recipeId;
          });
      if (found == staged.recipes.end()) {
        setStatus(receipt, CreativePatternRecipeMutationStatus::NotFound,
                  "creative_pattern_recipe_not_found");
        return receipt;
      }
      staged.recipes.erase(found);
      changed = true;
      break;
    }
    case CreativePatternRecipeMutationKind::Count:
      break;
  }

  if (!validateCreativePatternRecipeReferences(staged, objects_)) {
    setStatus(receipt, CreativePatternRecipeMutationStatus::InvalidRequest,
              "creative_pattern_recipe_references_invalid");
    return receipt;
  }
  if (!changed) {
    receipt.accepted = true;
    setStatus(receipt, CreativePatternRecipeMutationStatus::NoChange,
              "creative_pattern_recipe_no_change");
    return receipt;
  }

  patternRecipeStore_ = std::move(staged);
  markObjectMutationChanged();
  receipt.accepted = true;
  receipt.changed = true;
  receipt.recipeCountAfter = patternRecipeStore_.recipes.size();
  receipt.revisionAfter = revision_;
  setStatus(receipt, CreativePatternRecipeMutationStatus::Applied,
            request.kind == CreativePatternRecipeMutationKind::Detach
                ? "creative_pattern_recipe_detached"
                : "creative_pattern_recipe_applied");
  return receipt;
}

}  // namespace iggy3d::creative
