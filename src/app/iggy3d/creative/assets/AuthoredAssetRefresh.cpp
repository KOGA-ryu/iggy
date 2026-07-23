#include "app/iggy3d/creative/assets/AuthoredAsset.hpp"
#include "app/iggy3d/creative/assets/AuthoredAssetInternal.hpp"

#include "app/iggy3d/creative/mutation/Mutation.hpp"
#include "app/iggy3d/creative/tools/Group.hpp"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace iggy3d::creative {
namespace {

void reject(CreativeAuthoredAssetRefreshReceipt& receipt,
            CreativeAuthoredAssetRefreshStatus status,
            std::string_view reasonCode,
            CreativeObjectId failedInstanceRootObjectId = kInvalidObjectId,
            CreativeObjectId failedObjectId = kInvalidObjectId) noexcept {
  receipt.status = status;
  receipt.reasonCode = reasonCode;
  receipt.failedInstanceRootObjectId = failedInstanceRootObjectId;
  receipt.failedObjectId = failedObjectId;
  receipt.refreshedInstanceCount = 0U;
  receipt.removedObjectCount = 0U;
  receipt.createdObjectCount = 0U;
  receipt.revisionAfter = receipt.revisionBefore;
}

}  // namespace

CreativeAuthoredAssetRefreshReceipt
refreshCreativeAuthoredAssetInstancesAtomically(
    CreativeDocument& document,
    const CreativeAuthoredAssetDefinition& definition) {
  CreativeAuthoredAssetRefreshRequest request;
  request.definition = &definition;
  request.mode = CreativeAuthoredAssetRefreshMode::ForceAll;
  return refreshCreativeAuthoredAssetInstancesAtomically(document, request);
}

CreativeAuthoredAssetRefreshReceipt
refreshCreativeAuthoredAssetInstancesAtomically(
    CreativeDocument& document,
    const CreativeAuthoredAssetRefreshRequest& request) {
  CreativeAuthoredAssetRefreshReceipt receipt;
  receipt.requested = true;
  receipt.mode = request.mode;
  receipt.revisionBefore = document.revision();
  receipt.revisionAfter = receipt.revisionBefore;
  if (request.definition == nullptr) {
    reject(receipt, CreativeAuthoredAssetRefreshStatus::InvalidDefinition,
           "creative_authored_asset_refresh_definition_missing");
    return receipt;
  }
  const CreativeAuthoredAssetDefinition& definition = *request.definition;
  receipt.assetId = definition.assetId;
  if (!document.isValid() || document.id() == kInvalidDocumentId) {
    reject(receipt, CreativeAuthoredAssetRefreshStatus::InvalidDocument,
           "creative_authored_asset_refresh_document_invalid");
    return receipt;
  }

  CreativeAuthoredAssetPlacementRequest validationRequest;
  validationRequest.definition = &definition;
  const CreativeAuthoredAssetPlacementPlan validationPlan =
      planCreativeAuthoredAssetPlacement(validationRequest);
  const CreativeBoundsMetrics bounds = measureCreativeBounds(
      definition.sourceBounds);
  if (!validationPlan.accepted || !bounds.valid ||
      !isPositiveCreativeVec3(bounds.size)) {
    reject(receipt, CreativeAuthoredAssetRefreshStatus::InvalidDefinition,
           "creative_authored_asset_refresh_definition_invalid");
    return receipt;
  }
  for (const CreativeObject& object : definition.content.objects) {
    if (object.kind == CreativeObjectKind::PrefabInstance &&
        object.assetId == definition.assetId) {
      reject(receipt, CreativeAuthoredAssetRefreshStatus::InvalidDefinition,
             "creative_authored_asset_refresh_recursive_definition");
      return receipt;
    }
  }
  switch (request.mode) {
    case CreativeAuthoredAssetRefreshMode::SelectedInstance:
    case CreativeAuthoredAssetRefreshMode::SafeInstances:
    case CreativeAuthoredAssetRefreshMode::ForceAll:
      break;
    default:
      reject(receipt, CreativeAuthoredAssetRefreshStatus::InvalidDefinition,
             "creative_authored_asset_refresh_mode_invalid");
      return receipt;
  }
  if (!validateCreativeObjectParentGraph(document.objects()).empty()) {
    reject(receipt, CreativeAuthoredAssetRefreshStatus::InvalidHierarchy,
           "creative_authored_asset_refresh_hierarchy_invalid");
    return receipt;
  }

  std::vector<CreativeObjectId> matchingInstanceRootObjectIds;
  for (const CreativeObject& object : document.objects()) {
    if (object.kind == CreativeObjectKind::PrefabInstance &&
        object.assetId == definition.assetId) {
      matchingInstanceRootObjectIds.push_back(object.id);
      const CreativeAuthoredAssetSyncReceipt inspected =
          inspectCreativeAuthoredAssetInstanceSync(document, definition,
                                                   object.id);
      const CreativeAuthoredAssetSyncState syncState =
          inspected.accepted ? inspected.state
                             : CreativeAuthoredAssetSyncState::Conflict;
      switch (syncState) {
        case CreativeAuthoredAssetSyncState::Current:
          ++receipt.currentInstanceCount;
          break;
        case CreativeAuthoredAssetSyncState::SourceChanged:
          ++receipt.sourceChangedInstanceCount;
          break;
        case CreativeAuthoredAssetSyncState::LocallyModified:
          ++receipt.locallyModifiedInstanceCount;
          break;
        case CreativeAuthoredAssetSyncState::Conflict:
          ++receipt.conflictInstanceCount;
          break;
      }

      bool selectedForRefresh = false;
      switch (request.mode) {
        case CreativeAuthoredAssetRefreshMode::SelectedInstance:
          selectedForRefresh =
              object.id == request.selectedInstanceRootObjectId;
          break;
        case CreativeAuthoredAssetRefreshMode::SafeInstances:
          selectedForRefresh =
              syncState == CreativeAuthoredAssetSyncState::SourceChanged;
          break;
        case CreativeAuthoredAssetRefreshMode::ForceAll:
          selectedForRefresh = true;
          break;
      }
      if (selectedForRefresh) {
        receipt.instanceRootObjectIds.push_back(object.id);
      }
    }
  }
  receipt.matchedInstanceCount = matchingInstanceRootObjectIds.size();
  if (matchingInstanceRootObjectIds.empty()) {
    reject(receipt,
           CreativeAuthoredAssetRefreshStatus::NoMatchingInstances,
           "creative_authored_asset_refresh_instances_missing");
    return receipt;
  }
  if (receipt.instanceRootObjectIds.empty()) {
    reject(receipt, CreativeAuthoredAssetRefreshStatus::NoEligibleInstances,
           request.mode == CreativeAuthoredAssetRefreshMode::SelectedInstance
               ? "creative_authored_asset_refresh_selected_instance_missing"
               : "creative_authored_asset_refresh_safe_instances_empty");
    return receipt;
  }

  const std::unordered_set<CreativeObjectId> matchingRoots(
      matchingInstanceRootObjectIds.begin(),
      matchingInstanceRootObjectIds.end());
  for (CreativeObjectId rootObjectId : receipt.instanceRootObjectIds) {
    const CreativeObject* root = document.findObject(rootObjectId);
    if (root == nullptr ||
        !creativeAuthoredAssetInstanceTransformSupported(*root)) {
      reject(receipt,
             CreativeAuthoredAssetRefreshStatus::UnsupportedInstance,
             "creative_authored_asset_refresh_instance_unsupported",
             rootObjectId, rootObjectId);
      return receipt;
    }
    if (creativeObjectEffectivelyLocked(document, rootObjectId)) {
      reject(receipt, CreativeAuthoredAssetRefreshStatus::LockedObject,
             "creative_authored_asset_refresh_object_locked", rootObjectId,
             rootObjectId);
      return receipt;
    }

    std::vector<CreativeObjectId> directChildren;
    for (const CreativeObject& object : document.objects()) {
      if (object.parentId == rootObjectId) {
        directChildren.push_back(object.id);
      }
    }
    if (directChildren.empty()) {
      continue;
    }
    const CreativeHierarchySelection hierarchy =
        resolveCreativeObjectHierarchy(document, directChildren);
    if (!hierarchy.accepted) {
      reject(receipt, CreativeAuthoredAssetRefreshStatus::InvalidHierarchy,
             hierarchy.reasonCode, rootObjectId, hierarchy.missingObjectId);
      return receipt;
    }
    for (CreativeObjectId objectId : hierarchy.objectIds) {
      const CreativeObject* object = document.findObject(objectId);
      if (object == nullptr) {
        reject(receipt, CreativeAuthoredAssetRefreshStatus::InvalidHierarchy,
               "creative_authored_asset_refresh_descendant_missing",
               rootObjectId, objectId);
        return receipt;
      }
      if (matchingRoots.contains(objectId)) {
        reject(receipt, CreativeAuthoredAssetRefreshStatus::InvalidHierarchy,
               "creative_authored_asset_refresh_nested_instance",
               rootObjectId, objectId);
        return receipt;
      }
      if (creativeObjectEffectivelyLocked(document, objectId)) {
        reject(receipt, CreativeAuthoredAssetRefreshStatus::LockedObject,
               "creative_authored_asset_refresh_object_locked", rootObjectId,
               objectId);
        return receipt;
      }
    }
  }

  const CreativeAuthoredAssetFingerprint sourceFingerprint =
      fingerprintCreativeAuthoredAssetDefinition(definition);
  const std::string currentSourceFingerprintTag =
      authored_asset_internal::makeSourceFingerprintTag(sourceFingerprint.value);
  CreativeDocument staged = document;
  for (CreativeObjectId rootObjectId : receipt.instanceRootObjectIds) {
    const CreativeObject* root = staged.findObject(rootObjectId);
    if (root == nullptr) {
      reject(receipt, CreativeAuthoredAssetRefreshStatus::InvalidHierarchy,
             "creative_authored_asset_refresh_instance_missing",
             rootObjectId, rootObjectId);
      return receipt;
    }
    const CreativeTransform rootTransform = root->transform;
    const std::optional<CreativeObjectId> rootParentId = root->parentId;
    const std::string rootAttachmentSocket = root->attachmentSocket;
    std::vector<std::string> previousSourceFingerprintTags;
    for (const std::string& tag : root->tags) {
      if (authored_asset_internal::isSourceFingerprintTag(tag)) {
        previousSourceFingerprintTags.push_back(tag);
      }
    }

    std::vector<CreativeObjectId> directChildren;
    for (const CreativeObject& object : staged.objects()) {
      if (object.parentId == rootObjectId) {
        directChildren.push_back(object.id);
      }
    }
    if (!directChildren.empty()) {
      const CreativeHierarchySelection hierarchy =
          resolveCreativeObjectHierarchy(staged, directChildren);
      if (!hierarchy.accepted) {
        reject(receipt, CreativeAuthoredAssetRefreshStatus::InvalidHierarchy,
               hierarchy.reasonCode, rootObjectId,
               hierarchy.missingObjectId);
        return receipt;
      }
      CreativeClipboard discarded;
      const CreativeClipboardCutReceipt removed =
          cutDocumentObjectsAtomically(staged, hierarchy.objectIds,
                                       discarded);
      if (!removed.accepted) {
        reject(receipt, CreativeAuthoredAssetRefreshStatus::MutationRejected,
               "creative_authored_asset_refresh_remove_rejected",
               rootObjectId, removed.failedObjectId);
        return receipt;
      }
      receipt.removedObjectCount += removed.cutObjectCount;
    }

    CreativeAuthoredAssetPlacementRequest placementRequest;
    placementRequest.definition = &definition;
    placementRequest.instanceTransform = rootTransform;
    placementRequest.parentId = rootParentId;
    placementRequest.attachmentSocket = rootAttachmentSocket;
    const CreativeAuthoredAssetPlacementPlan plan =
        planCreativeAuthoredAssetPlacement(placementRequest);
    if (!plan.accepted) {
      reject(receipt, CreativeAuthoredAssetRefreshStatus::InvalidDefinition,
             plan.reasonCode, rootObjectId);
      return receipt;
    }

    CreativeClipboard transformedContent = definition.content;
    const CreativeVec3 sourceAnchor = transformedContent.hasPlacementAnchor
                                          ? transformedContent.placementAnchor
                                          : CreativeVec3{};
    if (!authored_asset_internal::applyContentTransform(transformedContent, sourceAnchor,
                                       rootTransform, false)) {
      reject(receipt, CreativeAuthoredAssetRefreshStatus::InvalidDefinition,
             "creative_authored_asset_refresh_transform_invalid",
             rootObjectId);
      return receipt;
    }
    const CreativeClipboardPasteReceipt pasted =
        pasteCreativeClipboardAtomically(staged, transformedContent,
                                         plan.contentPasteRequest);
    if (!pasted.accepted) {
      reject(receipt, CreativeAuthoredAssetRefreshStatus::MutationRejected,
             "creative_authored_asset_refresh_paste_rejected", rootObjectId,
             pasted.failedObjectId);
      return receipt;
    }

    std::unordered_map<CreativeObjectId, CreativeObjectId> remaps;
    remaps.reserve(pasted.idRemaps.size());
    for (const CreativeClipboardIdRemap& remap : pasted.idRemaps) {
      remaps.emplace(remap.sourceObjectId, remap.pastedObjectId);
    }
    std::vector<CreativeMutationRequest> mutations;
    mutations.reserve(plan.sourceRootObjectCount +
                      previousSourceFingerprintTags.size() + 2U);
    for (CreativeObjectId sourceRootId : plan.sourceRoots()) {
      const auto pastedRoot = remaps.find(sourceRootId);
      if (pastedRoot == remaps.end()) {
        reject(receipt, CreativeAuthoredAssetRefreshStatus::InvalidDefinition,
               "creative_authored_asset_refresh_root_remap_missing",
               rootObjectId, sourceRootId);
        return receipt;
      }
      mutations.push_back(
          {0U, pastedRoot->second, CreativeMutationKind::SetParent,
           makeParentPayload(rootObjectId)});
    }
    for (const std::string& tag : previousSourceFingerprintTags) {
      mutations.push_back(
          {0U, rootObjectId, CreativeMutationKind::RemoveTag,
           CreativeMutationPayload{TagMutation{tag}}});
    }
    mutations.push_back(
        {0U, rootObjectId, CreativeMutationKind::AddTag,
         CreativeMutationPayload{
             TagMutation{currentSourceFingerprintTag}}});
    mutations.push_back(
        {0U, rootObjectId, CreativeMutationKind::SetBounds,
         makeBoundsPayload(plan.rootRequest.bounds)});
    const CreativeDocumentBatchMutationReceipt mutated =
        applyDocumentMutationsAtomically(staged, mutations);
    if (!mutated.committed || !documentMutationSucceeded(mutated.status)) {
      reject(receipt, CreativeAuthoredAssetRefreshStatus::MutationRejected,
             "creative_authored_asset_refresh_mutation_rejected",
             rootObjectId);
      return receipt;
    }

    receipt.createdObjectCount += pasted.pastedObjectCount;
    ++receipt.refreshedInstanceCount;
  }

  const CreativeDocumentPublicationReceipt publication =
      document.commitStagedMutation(std::move(staged));
  if (!publication.accepted) {
    reject(receipt, CreativeAuthoredAssetRefreshStatus::MutationRejected,
           publication.reasonCode);
    return receipt;
  }
  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = CreativeAuthoredAssetRefreshStatus::Refreshed;
  receipt.revisionAfter = publication.revisionAfter;
  receipt.reasonCode = "creative_authored_asset_instances_refreshed";
  return receipt;
}

}  // namespace iggy3d::creative
