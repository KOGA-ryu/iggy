#include "app/iggy3d/creative/world/WorldLayout.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <utility>

namespace iggy3d::creative {
namespace {

struct StageResult {
  bool accepted = false;
  bool changed = false;
  CreativeWorldLayoutStatus status = CreativeWorldLayoutStatus::NotRequested;
  CreativeDocument document;
  CreativeTerrainMutationReceipt terrainReceipt;
  CreativeTerrainMaterialMutationReceipt materialReceipt;
  std::size_t failedRecipeIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t failedObjectIndex = kInvalidCreativeWorldLayoutIndex;
  std::string reasonCode = "creative_world_layout_stage_not_requested";
};

void setStatus(CreativeWorldLayoutPreviewResult& result,
               CreativeWorldLayoutStatus status,
               std::string_view reasonCode,
               bool accepted = false) {
  result.status = status;
  result.reasonCode = std::string(reasonCode);
  result.accepted = accepted;
}

void setStatus(CreativeWorldLayoutApplyReceipt& receipt,
               CreativeWorldLayoutStatus status,
               std::string_view reasonCode,
               bool accepted = false) {
  receipt.status = status;
  receipt.reasonCode = std::string(reasonCode);
  receipt.accepted = accepted;
}

[[nodiscard]] bool validStableKey(std::string_view key) noexcept {
  if (key.empty() || key.size() > 128U) {
    return false;
  }
  return std::all_of(key.begin(), key.end(), [](char value) {
    const unsigned char character = static_cast<unsigned char>(value);
    return std::isalnum(character) != 0 || value == '_' || value == '-' ||
           value == '.';
  });
}

[[nodiscard]] bool sourceMatches(const CreativeDocument& document,
                                 const CreativeWorldLayoutPlan& plan) noexcept {
  return document.isValid() && document.id() != kInvalidDocumentId &&
         document.nextObjectId() != kInvalidObjectId &&
         document.terrainField().validateInvariants() &&
         document.terrainMaterialField().validateInvariants() &&
         plan.schemaVersion == kCreativeWorldLayoutSchemaVersion &&
         validStableKey(plan.layoutKey) &&
         document.id() == plan.sourceDocumentId &&
         document.revision() == plan.sourceDocumentRevision &&
         document.terrainField().revision() == plan.sourceTerrainRevision &&
         document.terrainMaterialField().revision() ==
             plan.sourceMaterialRevision;
}

[[nodiscard]] StageResult stagePlan(const CreativeDocument& source,
                                    const CreativeWorldLayoutPlan& plan) {
  StageResult result;
  result.document = source;

  for (const CreativeObjectId objectId : plan.objectRemoveIds) {
    const CreativeDocumentRemoveReceipt removed =
        result.document.removeDocumentObject(objectId);
    if (!removed.accepted || !removed.changed) {
      result.status = CreativeWorldLayoutStatus::ObjectRejected;
      result.reasonCode = std::string(removed.reasonCode);
      return result;
    }
    result.changed = true;
  }

  if (!plan.terrainEdits.empty()) {
    result.terrainReceipt =
        result.document.applyTerrainControlEdits(plan.terrainEdits);
    if (!result.terrainReceipt.accepted) {
      result.status = CreativeWorldLayoutStatus::MutationRejected;
      result.reasonCode = std::string(result.terrainReceipt.reasonCode);
      return result;
    }
    result.changed = result.changed || result.terrainReceipt.changed;
  }
  if (!plan.materialEdits.empty()) {
    result.materialReceipt =
        result.document.applyTerrainMaterialEdits(plan.materialEdits);
    if (!result.materialReceipt.accepted) {
      result.status = CreativeWorldLayoutStatus::MutationRejected;
      result.reasonCode = std::string(result.materialReceipt.reasonCode);
      return result;
    }
    result.changed = result.changed || result.materialReceipt.changed;
  }

  for (std::size_t recipeIndex = 0U;
       recipeIndex < plan.objectRecipes.size(); ++recipeIndex) {
    result.failedRecipeIndex = recipeIndex;
    const CreativeRecipeMaterializeResult materialized =
        materializeCreativeRecipe(plan.objectRecipes[recipeIndex],
                                  result.document.nextObjectId());
    if (!materialized.receipt.accepted) {
      result.status = CreativeWorldLayoutStatus::ObjectRejected;
      result.reasonCode = materialized.receipt.reasonCode;
      return result;
    }
    for (std::size_t objectIndex = 0U;
         objectIndex < materialized.createRequests.size(); ++objectIndex) {
      result.failedObjectIndex = objectIndex;
      const CreativeDocumentCreateReceipt created =
          result.document.createObject(materialized.createRequests[objectIndex]);
      if (!created.accepted || !created.changed || !created.objectCreated) {
        result.status = CreativeWorldLayoutStatus::ObjectRejected;
        result.reasonCode = std::string(created.reasonCode);
        return result;
      }
      result.changed = true;
    }
  }

  result.accepted = true;
  result.failedRecipeIndex = kInvalidCreativeWorldLayoutIndex;
  result.failedObjectIndex = kInvalidCreativeWorldLayoutIndex;
  result.status = result.changed ? CreativeWorldLayoutStatus::Ready
                                 : CreativeWorldLayoutStatus::NoChange;
  result.reasonCode = result.changed ? "creative_world_layout_stage_ready"
                                     : "creative_world_layout_no_change";
  return result;
}

}  // namespace

CreativeWorldLayoutPreviewResult previewCreativeWorldLayoutPlan(
    const CreativeDocument& document,
    const CreativeWorldLayoutPlan& plan) {
  CreativeWorldLayoutPreviewResult result;
  result.requested = true;
  if (!sourceMatches(document, plan)) {
    setStatus(result, CreativeWorldLayoutStatus::StalePlan,
              "creative_world_layout_plan_stale");
    return result;
  }
  StageResult staged = stagePlan(document, plan);
  result.failedRecipeIndex = staged.failedRecipeIndex;
  result.failedObjectIndex = staged.failedObjectIndex;
  if (!staged.accepted) {
    setStatus(result, staged.status, staged.reasonCode);
    return result;
  }
  result.document = std::move(staged.document);
  if (result.document.terrainField().controlCount() > 0U) {
    const CreativeTerrainSurfacePlan surface =
        buildCreativeTerrainSurfacePlan(result.document.terrainField());
    if (!surface.accepted) {
      setStatus(result, CreativeWorldLayoutStatus::MutationRejected,
                surface.reasonCode);
      return result;
    }
    result.terrainRenderPlan = buildCreativeTerrainRenderPlan(
        surface, result.document.terrainMaterialField(),
        result.document.gridSettings().origin,
        result.document.gridSettings().cellSizeMeters);
    if (!result.terrainRenderPlan.accepted) {
      setStatus(result, CreativeWorldLayoutStatus::MutationRejected,
                result.terrainRenderPlan.reasonCode);
      return result;
    }
    result.hasTerrainRenderPlan = true;
  }
  setStatus(result, staged.status,
            staged.changed ? "creative_world_layout_preview_ready"
                           : "creative_world_layout_preview_no_change",
            true);
  return result;
}

CreativeWorldLayoutApplyReceipt applyCreativeWorldLayoutPlan(
    Facade& facade,
    const CreativeWorldLayoutPlan& plan) {
  CreativeWorldLayoutApplyReceipt receipt;
  receipt.requested = true;
  if (!sourceMatches(facade.document(), plan)) {
    setStatus(receipt, CreativeWorldLayoutStatus::StalePlan,
              "creative_world_layout_plan_stale");
    return receipt;
  }
  StageResult staged = stagePlan(facade.document(), plan);
  receipt.terrainReceipt = staged.terrainReceipt;
  receipt.materialReceipt = staged.materialReceipt;
  receipt.failedRecipeIndex = staged.failedRecipeIndex;
  receipt.failedObjectIndex = staged.failedObjectIndex;
  if (!staged.accepted) {
    setStatus(receipt, staged.status, staged.reasonCode);
    return receipt;
  }
  if (!staged.changed) {
    setStatus(receipt, CreativeWorldLayoutStatus::NoChange,
              "creative_world_layout_no_change", true);
    return receipt;
  }
  receipt.installReceipt = facade.installDocument(std::move(staged.document));
  if (!receipt.installReceipt.accepted || !receipt.installReceipt.changed) {
    setStatus(receipt, CreativeWorldLayoutStatus::InstallRejected,
              receipt.installReceipt.reasonCode);
    return receipt;
  }
  receipt.changed = true;
  setStatus(receipt, CreativeWorldLayoutStatus::Applied,
            "creative_world_layout_applied", true);
  return receipt;
}

CreativeWorldLayoutApplyReceipt applyCreativeWorldLayoutPlanWithHistory(
    CreativeAppState& appState,
    const CreativeWorldLayoutPlan& plan,
    std::string_view source) {
  CreativeDocumentHistoryTransaction transaction =
      beginCreativeHistoryTransaction(appState.facade, source);
  CreativeWorldLayoutApplyReceipt receipt =
      applyCreativeWorldLayoutPlan(appState.facade, plan);
  if (!receipt.accepted || !receipt.changed) {
    cancelCreativeHistoryTransaction(transaction);
    return receipt;
  }
  receipt.historyReceipt = commitCreativeHistoryTransaction(
      appState.history, std::move(transaction), appState.facade);
  if (!receipt.historyReceipt.accepted || !receipt.historyReceipt.recorded) {
    receipt.reasonCode = std::string(receipt.historyReceipt.reasonCode);
  }
  return receipt;
}

}  // namespace iggy3d::creative
