#include "app/iggy3d/creative/world/WorldLayout.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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

struct PreparedRecipePatch {
  CreativeRecipeMaterializeResult materialized;
  const CreativeWorldLayoutRecipePatch* patch = nullptr;
};

struct SelectionSnapshot {
  std::vector<CreativeObjectId> objectIds;
  CreativeObjectId primaryObjectId = kInvalidObjectId;
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

[[nodiscard]] bool refreshRecipeManagementTags(
    CreativeObject& object,
    std::span<const std::string> desiredTags) {
  std::vector<std::string> refreshed;
  refreshed.reserve(object.tags.size() + desiredTags.size());
  for (const std::string& tag : object.tags) {
    if (!isCreativeRecipeManagementTag(tag)) {
      refreshed.push_back(tag);
    }
  }
  for (const std::string& tag : desiredTags) {
    if (isCreativeRecipeManagementTag(tag) &&
        std::find(refreshed.begin(), refreshed.end(), tag) ==
            refreshed.end()) {
      refreshed.push_back(tag);
    }
  }
  if (refreshed == object.tags) {
    return false;
  }
  object.tags = std::move(refreshed);
  return true;
}

[[nodiscard]] CreativeObjectDirtyFlags recipeMetadataDirtyFlags() noexcept {
  return static_cast<CreativeObjectDirtyFlags>(
             CreativeObjectDirtyFlag::Identity) |
         static_cast<CreativeObjectDirtyFlags>(
             CreativeObjectDirtyFlag::Preview) |
         static_cast<CreativeObjectDirtyFlags>(
             CreativeObjectDirtyFlag::Serialization);
}

[[nodiscard]] CreativeDocumentRestoreReceipt validateStagedDocument(
    const CreativeDocument& document) {
  CreativeDocumentRestoreRequest request;
  request.documentId = document.id();
  request.name = std::string(document.name());
  request.units = document.units();
  request.gridSettings = document.gridSettings();
  request.snapSettings = document.documentSnapSettings();
  request.worldBounds = document.worldBounds();
  request.nextObjectId = document.nextObjectId();
  request.objects.assign(document.objects().begin(), document.objects().end());
  request.logicLinks.assign(document.logicLinks().begin(),
                            document.logicLinks().end());
  request.voxelField = document.voxelField();
  request.terrainField = document.terrainField();
  request.terrainHeightField = document.terrainHeightField();
  request.terrainMaterialField = document.terrainMaterialField();
  CreativeDocument validationDocument;
  return validationDocument.restoreForLoad(request);
}

[[nodiscard]] SelectionSnapshot captureSelection(const Facade& facade) {
  SelectionSnapshot snapshot;
  const CreativeSelectionState& selection = facade.selectionState();
  const auto appendTarget = [&](TargetRef target) {
    const CreativeObjectId objectId =
        target.value == kInvalidId
            ? kInvalidObjectId
            : static_cast<CreativeObjectId>(target.value);
    if (objectId != kInvalidObjectId &&
        facade.document().containsObject(objectId) &&
        std::find(snapshot.objectIds.begin(), snapshot.objectIds.end(),
                  objectId) == snapshot.objectIds.end()) {
      snapshot.objectIds.push_back(objectId);
    }
  };
  for (TargetRef target : selectedTargetList(selection)) {
    appendTarget(target);
  }
  appendTarget(selection.selectedTarget);
  if (selection.selectedTarget.value != kInvalidId) {
    snapshot.primaryObjectId = static_cast<CreativeObjectId>(
        selection.selectedTarget.value);
  }
  return snapshot;
}

void restoreValidSelection(Facade& facade, SelectionSnapshot snapshot) {
  std::erase_if(snapshot.objectIds, [&](CreativeObjectId objectId) {
    return !facade.document().containsObject(objectId);
  });
  if (!facade.document().containsObject(snapshot.primaryObjectId)) {
    snapshot.primaryObjectId = kInvalidObjectId;
  }
  static_cast<void>(
      facade.selectTargets(snapshot.objectIds, snapshot.primaryObjectId));
}

[[nodiscard]] StageResult stagePlan(const CreativeDocument& source,
                                    const CreativeWorldLayoutPlan& plan) {
  StageResult result;
  result.document = source;

  bool detachedAny = false;
  for (const CreativeObjectId objectId : plan.objectDetachIds) {
    CreativeObject* object = result.document.findObject(objectId);
    if (object == nullptr ||
        !detachCreativeWorldLayoutObject(
            *object, creativeWorldLayoutTag(plan.layoutKey))) {
      result.status = CreativeWorldLayoutStatus::ObjectRejected;
      result.reasonCode = "creative_world_layout_detach_rejected";
      return result;
    }
    detachedAny = true;
  }
  if (detachedAny) {
    result.document.markObjectMutationChanged(
        static_cast<CreativeObjectDirtyFlags>(
            CreativeObjectDirtyFlag::Identity) |
        static_cast<CreativeObjectDirtyFlags>(
            CreativeObjectDirtyFlag::Serialization) |
        static_cast<CreativeObjectDirtyFlags>(CreativeObjectDirtyFlag::Preview));
    result.changed = true;
  }

  std::vector<PreparedRecipePatch> preparedPatches;
  preparedPatches.reserve(plan.objectRecipePatches.size());
  CreativeObjectDirtyFlags patchDirtyFlags = 0U;
  bool patchedExisting = false;
  for (std::size_t patchIndex = 0U;
       patchIndex < plan.objectRecipePatches.size(); ++patchIndex) {
    result.failedRecipeIndex = patchIndex;
    const CreativeWorldLayoutRecipePatch& patch =
        plan.objectRecipePatches[patchIndex];
    if (patch.objectIds.size() != patch.recipe.objects.size() ||
        patch.memberActions.size() != patch.recipe.objects.size()) {
      result.status = CreativeWorldLayoutStatus::ObjectRejected;
      result.reasonCode = "creative_world_layout_patch_shape_invalid";
      return result;
    }
    PreparedRecipePatch prepared;
    prepared.materialized =
        materializeCreativeRecipe(patch.recipe, patch.objectIds);
    prepared.patch = &patch;
    if (!prepared.materialized.receipt.accepted ||
        prepared.materialized.createRequests.size() !=
            patch.memberActions.size()) {
      result.status = CreativeWorldLayoutStatus::ObjectRejected;
      result.reasonCode = prepared.materialized.receipt.reasonCode;
      return result;
    }

    for (std::size_t objectIndex = 0U;
         objectIndex < patch.memberActions.size(); ++objectIndex) {
      result.failedObjectIndex = objectIndex;
      const CreativeWorldLayoutRecipeMemberAction action =
          patch.memberActions[objectIndex];
      if (action == CreativeWorldLayoutRecipeMemberAction::Create) {
        continue;
      }
      if (action != CreativeWorldLayoutRecipeMemberAction::Preserve &&
          action != CreativeWorldLayoutRecipeMemberAction::Update) {
        result.status = CreativeWorldLayoutStatus::ObjectRejected;
        result.reasonCode = "creative_world_layout_patch_action_invalid";
        return result;
      }
      CreativeObject* existing =
          result.document.findObject(patch.objectIds[objectIndex]);
      if (existing == nullptr) {
        result.status = CreativeWorldLayoutStatus::ObjectRejected;
        result.reasonCode = "creative_world_layout_patch_object_missing";
        return result;
      }
      if (action == CreativeWorldLayoutRecipeMemberAction::Preserve) {
        if (refreshRecipeManagementTags(
                *existing,
                prepared.materialized.createRequests[objectIndex].tags)) {
          patchedExisting = true;
          patchDirtyFlags |= recipeMetadataDirtyFlags();
        }
        continue;
      }

      const CreativeObjectId existingId = existing->id;
      const CreativeObjectKind previousKind = existing->kind;
      *existing = resolveCreativeDocumentCreateObject(
          prepared.materialized.createRequests[objectIndex], existingId);
      patchedExisting = true;
      patchDirtyFlags |= dirtyFlagsForCreation(previousKind) |
                         dirtyFlagsForCreation(existing->kind) |
                         recipeMetadataDirtyFlags();
    }
    preparedPatches.push_back(std::move(prepared));
  }
  if (patchedExisting) {
    result.document.markObjectMutationChanged(patchDirtyFlags);
    result.changed = true;
  }

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

  for (std::size_t patchIndex = 0U;
       patchIndex < preparedPatches.size(); ++patchIndex) {
    result.failedRecipeIndex = patchIndex;
    const PreparedRecipePatch& prepared = preparedPatches[patchIndex];
    for (std::size_t objectIndex = 0U;
         objectIndex < prepared.patch->memberActions.size(); ++objectIndex) {
      result.failedObjectIndex = objectIndex;
      if (prepared.patch->memberActions[objectIndex] !=
          CreativeWorldLayoutRecipeMemberAction::Create) {
        continue;
      }
      if (result.document.nextObjectId() !=
          prepared.patch->objectIds[objectIndex]) {
        result.status = CreativeWorldLayoutStatus::ObjectRejected;
        result.reasonCode = "creative_world_layout_patch_object_id_stale";
        return result;
      }
      const CreativeDocumentCreateReceipt created =
          result.document.createObject(
              prepared.materialized.createRequests[objectIndex]);
      if (!created.accepted || !created.changed || !created.objectCreated ||
          created.objectId != prepared.patch->objectIds[objectIndex]) {
        result.status = CreativeWorldLayoutStatus::ObjectRejected;
        result.reasonCode = std::string(created.reasonCode);
        return result;
      }
      result.changed = true;
    }
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

  const CreativeDocumentRestoreReceipt validation =
      validateStagedDocument(result.document);
  if (!validation.accepted) {
    result.status = CreativeWorldLayoutStatus::ObjectRejected;
    result.reasonCode = std::string(validation.reasonCode);
    return result;
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
  SelectionSnapshot selection = captureSelection(facade);
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
  restoreValidSelection(facade, std::move(selection));
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
