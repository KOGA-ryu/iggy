#include "app/iggy3d/creative/assets/AuthoredAsset.hpp"
#include "app/iggy3d/creative/assets/AuthoredAssetInternal.hpp"

#include "app/iggy3d/creative/mutation/Mutation.hpp"

#include <algorithm>
#include <unordered_map>
#include <utility>
#include <vector>

namespace iggy3d::creative {
namespace {

void reject(CreativeAuthoredAssetInstanceReceipt& receipt,
            CreativeAuthoredAssetStatus status,
            std::string_view reasonCode) noexcept {
  receipt.status = status;
  receipt.reasonCode = reasonCode;
}

}  // namespace

CreativeAuthoredAssetPlacementPlan planCreativeAuthoredAssetPlacement(
    const CreativeAuthoredAssetPlacementRequest& request) noexcept {
  CreativeAuthoredAssetPlacementPlan plan;
  plan.requested = true;
  if (request.definition == nullptr ||
      !isValidCreativeAuthoredAssetId(request.definition->assetId) ||
      request.definition->content.objects.empty() ||
      request.definition->content.objects.size() >
          kCreativeAuthoredAssetObjectCapacity ||
      request.definition->rootObjectIds.empty() ||
      request.definition->rootObjectIds.size() >
          kCreativeAuthoredAssetObjectCapacity) {
    plan.status = CreativeAuthoredAssetStatus::InvalidDefinition;
    plan.reasonCode = "creative_authored_asset_definition_invalid";
    return plan;
  }
  const CreativeAuthoredAssetFingerprint sourceFingerprint =
      fingerprintCreativeAuthoredAssetDefinition(*request.definition);
  if (!sourceFingerprint.valid) {
    plan.status = CreativeAuthoredAssetStatus::InvalidDefinition;
    plan.reasonCode = "creative_authored_asset_definition_fingerprint_invalid";
    return plan;
  }
  if (!isFiniteCreativeVec3(request.instanceTransform.position) ||
      !isFiniteCreativeVec3(request.instanceTransform.rotationEulerRadians) ||
      !isPositiveCreativeVec3(request.instanceTransform.scale)) {
    plan.status = CreativeAuthoredAssetStatus::InvalidGeometry;
    plan.reasonCode = "creative_authored_asset_placement_invalid";
    return plan;
  }

  plan.rootRequest.kind = CreativeObjectKind::PrefabInstance;
  plan.rootRequest.name = request.definition->label;
  plan.rootRequest.assetId = request.definition->assetId;
  plan.rootRequest.transform = request.instanceTransform;
  plan.rootRequest.hasTransformOverride = true;
  plan.rootRequest.bounds =
      authored_asset_internal::translateAssetBounds(request.definition->sourceBounds,
                       request.instanceTransform.position);
  plan.rootRequest.hasBoundsOverride = true;
  plan.rootRequest.visible = false;
  plan.rootRequest.hasVisibleOverride = true;
  plan.rootRequest.locked = false;
  plan.rootRequest.hasLockedOverride = true;
  plan.rootRequest.parentId = request.parentId;
  plan.rootRequest.attachmentSocket = request.attachmentSocket;
  plan.rootRequest.tags.push_back(
      authored_asset_internal::makeSourceFingerprintTag(sourceFingerprint.value));

  const CreativeVec3 sourceAnchor =
      request.definition->content.hasPlacementAnchor
          ? request.definition->content.placementAnchor
          : CreativeVec3{};
  plan.contentPasteRequest.offset = {
      request.instanceTransform.position.x - sourceAnchor.x,
      request.instanceTransform.position.y - sourceAnchor.y,
      request.instanceTransform.position.z - sourceAnchor.z,
  };
  plan.contentPasteRequest.appendCopySuffix = false;
  plan.contentPasteRequest.externalParentPolicy =
      CreativeClipboardExternalParentPolicy::Detach;
  std::copy(request.definition->rootObjectIds.begin(),
            request.definition->rootObjectIds.end(),
            plan.sourceRootObjectIds.begin());
  plan.sourceRootObjectCount = static_cast<std::uint16_t>(
      request.definition->rootObjectIds.size());
  plan.accepted = true;
  plan.status = CreativeAuthoredAssetStatus::Ready;
  plan.reasonCode = "creative_authored_asset_placement_ready";
  return plan;
}

CreativeAuthoredAssetInstanceReceipt instantiateCreativeAuthoredAssetAtomically(
    CreativeDocument& document,
    const CreativeAuthoredAssetPlacementRequest& request) {
  CreativeAuthoredAssetInstanceReceipt receipt;
  receipt.requested = true;
  receipt.revisionBefore = document.revision();
  receipt.revisionAfter = receipt.revisionBefore;
  if (!document.isValid() || document.id() == kInvalidDocumentId) {
    reject(receipt, CreativeAuthoredAssetStatus::InvalidDocument,
           "creative_authored_asset_target_document_invalid");
    return receipt;
  }
  const CreativeAuthoredAssetPlacementPlan plan =
      planCreativeAuthoredAssetPlacement(request);
  if (!plan.accepted) {
    reject(receipt, plan.status, plan.reasonCode);
    return receipt;
  }

  CreativeDocument staged = document;
  receipt.rootCreateReceipt = staged.createObject(plan.rootRequest);
  if (!receipt.rootCreateReceipt.accepted ||
      !receipt.rootCreateReceipt.objectCreated) {
    reject(receipt, CreativeAuthoredAssetStatus::CreateRejected,
           receipt.rootCreateReceipt.reasonCode);
    return receipt;
  }
  receipt.instanceRootObjectId = receipt.rootCreateReceipt.objectId;
  CreativeClipboard transformedContent = request.definition->content;
  const CreativeVec3 sourceAnchor = transformedContent.hasPlacementAnchor
                                        ? transformedContent.placementAnchor
                                        : CreativeVec3{};
  if (!authored_asset_internal::applyContentTransform(transformedContent, sourceAnchor,
                                     request.instanceTransform, false)) {
    reject(receipt, CreativeAuthoredAssetStatus::InvalidGeometry,
           "creative_authored_asset_content_transform_invalid");
    return receipt;
  }
  receipt.contentPasteReceipt = pasteCreativeClipboardAtomically(
      staged, transformedContent, plan.contentPasteRequest);
  if (!receipt.contentPasteReceipt.accepted) {
    reject(receipt, CreativeAuthoredAssetStatus::CreateRejected,
           receipt.contentPasteReceipt.reasonCode);
    return receipt;
  }

  std::unordered_map<CreativeObjectId, CreativeObjectId> remaps;
  remaps.reserve(receipt.contentPasteReceipt.idRemaps.size());
  for (const CreativeClipboardIdRemap& remap :
       receipt.contentPasteReceipt.idRemaps) {
    remaps.emplace(remap.sourceObjectId, remap.pastedObjectId);
  }
  std::vector<CreativeMutationRequest> parentMutations;
  parentMutations.reserve(plan.sourceRootObjectCount);
  for (CreativeObjectId sourceRootId : plan.sourceRoots()) {
    const auto pasted = remaps.find(sourceRootId);
    if (pasted == remaps.end()) {
      reject(receipt, CreativeAuthoredAssetStatus::InvalidDefinition,
             "creative_authored_asset_root_remap_missing");
      return receipt;
    }
    parentMutations.push_back(
        {0U, pasted->second, CreativeMutationKind::SetParent,
         makeParentPayload(receipt.instanceRootObjectId)});
  }
  receipt.parentMutationReceipt =
      applyDocumentMutationsAtomically(staged, parentMutations);
  if (!receipt.parentMutationReceipt.committed ||
      !documentMutationSucceeded(receipt.parentMutationReceipt.status)) {
    reject(receipt, CreativeAuthoredAssetStatus::MutationRejected,
           "creative_authored_asset_parenting_rejected");
    return receipt;
  }

  document = std::move(staged);
  receipt.instanceObjectIds = receipt.contentPasteReceipt.pastedObjectIds;
  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = CreativeAuthoredAssetStatus::Instantiated;
  receipt.revisionAfter = document.revision();
  receipt.rootCreateReceipt.revisionBefore = receipt.revisionBefore;
  receipt.rootCreateReceipt.revisionAfter = receipt.revisionAfter;
  receipt.reasonCode = "creative_authored_asset_instantiated";
  return receipt;
}

}  // namespace iggy3d::creative
