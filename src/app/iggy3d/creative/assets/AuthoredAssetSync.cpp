#include "app/iggy3d/creative/assets/AuthoredAsset.hpp"
#include "app/iggy3d/creative/assets/AuthoredAssetInternal.hpp"

#include "app/iggy3d/creative/document/Hierarchy.hpp"
#include "app/iggy3d/creative/mutation/Mutation.hpp"

#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace iggy3d::creative {

CreativeAuthoredAssetSyncReceipt inspectCreativeAuthoredAssetInstanceSync(
    const CreativeDocument& document,
    const CreativeAuthoredAssetDefinition& definition,
    CreativeObjectId instanceRootObjectId) {
  CreativeAuthoredAssetSyncReceipt receipt;
  receipt.requested = true;
  receipt.instanceRootObjectId = instanceRootObjectId;
  receipt.sourceFingerprint =
      fingerprintCreativeAuthoredAssetDefinition(definition);
  const CreativeObject* instance = document.findObject(instanceRootObjectId);
  if (!receipt.sourceFingerprint.valid || instance == nullptr ||
      instance->kind != CreativeObjectKind::PrefabInstance ||
      instance->assetId != definition.assetId) {
    receipt.reasonCode =
        "creative_authored_asset_sync_source_or_instance_invalid";
    return receipt;
  }

  CreativeAuthoredAssetInstanceCaptureRequest captureRequest;
  captureRequest.sourceDocument = &document;
  captureRequest.existingDefinition = &definition;
  captureRequest.instanceRootObjectId = instanceRootObjectId;
  captureRequest.definitionDocumentId =
      std::numeric_limits<CreativeDocumentId>::max();
  const CreativeAuthoredAssetCaptureResult captured =
      captureCreativeAuthoredAssetInstance(captureRequest);
  if (!captured.accepted) {
    receipt.reasonCode = captured.reasonCode;
    return receipt;
  }
  receipt.instanceFingerprint =
      fingerprintCreativeAuthoredAssetDefinition(captured.definition);
  if (!receipt.instanceFingerprint.valid) {
    receipt.reasonCode = "creative_authored_asset_sync_instance_invalid";
    return receipt;
  }

  receipt.storedSourceFingerprint =
      creativeAuthoredAssetStoredSourceFingerprint(*instance);
  receipt.accepted = true;
  if (receipt.instanceFingerprint.value == receipt.sourceFingerprint.value) {
    receipt.state = CreativeAuthoredAssetSyncState::Current;
    receipt.reasonCode = "creative_authored_asset_sync_current";
    return receipt;
  }
  if (!receipt.storedSourceFingerprint.has_value()) {
    receipt.state = CreativeAuthoredAssetSyncState::Conflict;
    receipt.reasonCode = "creative_authored_asset_sync_provenance_missing";
    return receipt;
  }

  const bool sourceChanged =
      receipt.sourceFingerprint.value != *receipt.storedSourceFingerprint;
  const bool instanceChanged =
      receipt.instanceFingerprint.value != *receipt.storedSourceFingerprint;
  if (sourceChanged && !instanceChanged) {
    receipt.state = CreativeAuthoredAssetSyncState::SourceChanged;
    receipt.reasonCode = "creative_authored_asset_sync_source_changed";
  } else if (!sourceChanged && instanceChanged) {
    receipt.state = CreativeAuthoredAssetSyncState::LocallyModified;
    receipt.reasonCode = "creative_authored_asset_sync_locally_modified";
  } else {
    receipt.state = CreativeAuthoredAssetSyncState::Conflict;
    receipt.reasonCode = "creative_authored_asset_sync_conflict";
  }
  return receipt;
}

CreativeAuthoredAssetSyncSummary summarizeCreativeAuthoredAssetSync(
    const CreativeDocument& document,
    const CreativeAuthoredAssetDefinition& definition) {
  CreativeAuthoredAssetSyncSummary summary;
  summary.requested = true;
  summary.assetId = definition.assetId;
  if (!document.isValid() || document.id() == kInvalidDocumentId ||
      !fingerprintCreativeAuthoredAssetDefinition(definition).valid) {
    summary.reasonCode = "creative_authored_asset_sync_summary_invalid";
    return summary;
  }

  for (const CreativeObject& object : document.objects()) {
    if (object.kind != CreativeObjectKind::PrefabInstance ||
        object.assetId != definition.assetId) {
      continue;
    }
    ++summary.matchedInstanceCount;
    const CreativeAuthoredAssetSyncReceipt inspected =
        inspectCreativeAuthoredAssetInstanceSync(document, definition,
                                                 object.id);
    const CreativeAuthoredAssetSyncState state =
        inspected.accepted ? inspected.state
                           : CreativeAuthoredAssetSyncState::Conflict;
    switch (state) {
      case CreativeAuthoredAssetSyncState::Current:
        ++summary.currentInstanceCount;
        break;
      case CreativeAuthoredAssetSyncState::SourceChanged:
        ++summary.sourceChangedInstanceCount;
        break;
      case CreativeAuthoredAssetSyncState::LocallyModified:
        ++summary.locallyModifiedInstanceCount;
        break;
      case CreativeAuthoredAssetSyncState::Conflict:
        ++summary.conflictInstanceCount;
        break;
    }
  }
  summary.accepted = true;
  summary.reasonCode = summary.matchedInstanceCount == 0U
                           ? "creative_authored_asset_sync_instances_missing"
                           : "creative_authored_asset_sync_summarized";
  return summary;
}

CreativeDocumentBatchMutationReceipt
acknowledgeCreativeAuthoredAssetInstanceSource(
    CreativeDocument& document,
    const CreativeAuthoredAssetDefinition& definition,
    CreativeObjectId instanceRootObjectId) {
  CreativeDocumentBatchMutationReceipt rejected;
  rejected.status = CreativeDocumentMutationStatus::InvalidRequest;
  rejected.revisionBefore = document.revision();
  rejected.revisionAfter = rejected.revisionBefore;
  rejected.atomic = true;
  rejected.message = "authored asset provenance request invalid";
  const CreativeAuthoredAssetFingerprint fingerprint =
      fingerprintCreativeAuthoredAssetDefinition(definition);
  const CreativeObject* root = document.findObject(instanceRootObjectId);
  if (!fingerprint.valid || root == nullptr ||
      creativeObjectEffectivelyLocked(document, instanceRootObjectId) ||
      root->kind != CreativeObjectKind::PrefabInstance ||
      root->assetId != definition.assetId) {
    return rejected;
  }
  CreativeAuthoredAssetPlacementRequest placementRequest;
  placementRequest.definition = &definition;
  placementRequest.instanceTransform = root->transform;
  placementRequest.parentId = root->parentId;
  placementRequest.attachmentSocket = root->attachmentSocket;
  const CreativeAuthoredAssetPlacementPlan placementPlan =
      planCreativeAuthoredAssetPlacement(placementRequest);
  if (!placementPlan.accepted) {
    return rejected;
  }

  const std::string fingerprintTag = authored_asset_internal::makeSourceFingerprintTag(fingerprint.value);
  std::vector<CreativeMutationRequest> mutations;
  std::size_t provenanceTagCount = 0U;
  for (const std::string& tag : root->tags) {
    if (authored_asset_internal::isSourceFingerprintTag(tag)) {
      ++provenanceTagCount;
    }
  }
  if (provenanceTagCount == 1U &&
      creativeAuthoredAssetStoredSourceFingerprint(*root) ==
          std::optional<std::uint64_t>{fingerprint.value}) {
    mutations.push_back(
        {0U, instanceRootObjectId, CreativeMutationKind::AddTag,
         CreativeMutationPayload{TagMutation{fingerprintTag}}});
  } else {
    mutations.reserve(provenanceTagCount + 2U);
    for (const std::string& tag : root->tags) {
      if (authored_asset_internal::isSourceFingerprintTag(tag)) {
        mutations.push_back(
            {0U, instanceRootObjectId, CreativeMutationKind::RemoveTag,
             CreativeMutationPayload{TagMutation{tag}}});
      }
    }
    mutations.push_back(
        {0U, instanceRootObjectId, CreativeMutationKind::AddTag,
         CreativeMutationPayload{TagMutation{fingerprintTag}}});
  }
  mutations.push_back(
      {0U, instanceRootObjectId, CreativeMutationKind::SetBounds,
       makeBoundsPayload(placementPlan.rootRequest.bounds)});
  return applyDocumentMutationsAtomically(document, mutations);
}

}  // namespace iggy3d::creative
