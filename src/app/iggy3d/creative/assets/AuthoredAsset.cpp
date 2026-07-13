#include "app/iggy3d/creative/assets/AuthoredAsset.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/mutation/Mutation.hpp"
#include "app/iggy3d/creative/tools/Group.hpp"

namespace iggy3d::creative {
namespace {

[[nodiscard]] bool nonBlank(std::string_view value) noexcept {
  return std::any_of(value.begin(), value.end(), [](char character) {
    return std::isspace(static_cast<unsigned char>(character)) == 0;
  });
}

[[nodiscard]] CreativeBounds translatedBounds(CreativeBounds bounds,
                                               CreativeVec3 offset) noexcept {
  return {{bounds.min.x + offset.x, bounds.min.y + offset.y,
           bounds.min.z + offset.z},
          {bounds.max.x + offset.x, bounds.max.y + offset.y,
           bounds.max.z + offset.z}};
}

void includeExtent(CreativeObjectWorldExtent& aggregate,
                   const CreativeObjectWorldExtent& item) noexcept {
  if (!item.valid) {
    return;
  }
  if (!aggregate.valid) {
    aggregate = item;
    return;
  }
  aggregate.min.x = std::min(aggregate.min.x, item.min.x);
  aggregate.min.y = std::min(aggregate.min.y, item.min.y);
  aggregate.min.z = std::min(aggregate.min.z, item.min.z);
  aggregate.max.x = std::max(aggregate.max.x, item.max.x);
  aggregate.max.y = std::max(aggregate.max.y, item.max.y);
  aggregate.max.z = std::max(aggregate.max.z, item.max.z);
}

[[nodiscard]] bool definitionBounds(const CreativeDocument& document,
                                    CreativeBounds& output) noexcept {
  CreativeObjectWorldExtent aggregate;
  for (const CreativeObject& object : document.objects()) {
    includeExtent(aggregate, resolveCreativeObjectWorldExtent(object));
  }
  if (!aggregate.valid) {
    return false;
  }
  output = {aggregate.min, aggregate.max};
  const CreativeBoundsMetrics metrics = measureCreativeBounds(output);
  return metrics.valid && isPositiveCreativeVec3(metrics.size);
}

[[nodiscard]] std::vector<CreativeObjectId> definitionRootIds(
    std::span<const CreativeObject> objects) {
  std::unordered_set<CreativeObjectId> ids;
  ids.reserve(objects.size());
  for (const CreativeObject& object : objects) {
    ids.insert(object.id);
  }
  std::vector<CreativeObjectId> roots;
  for (const CreativeObject& object : objects) {
    if (!object.parentId.has_value() || !ids.contains(*object.parentId)) {
      roots.push_back(object.id);
    }
  }
  return roots;
}

void reject(CreativeAuthoredAssetCaptureResult& result,
            CreativeAuthoredAssetStatus status,
            std::string_view reasonCode,
            CreativeObjectId failedObjectId = kInvalidObjectId) noexcept {
  result.status = status;
  result.reasonCode = reasonCode;
  result.failedObjectId = failedObjectId;
}

void reject(CreativeAuthoredAssetInstanceReceipt& receipt,
            CreativeAuthoredAssetStatus status,
            std::string_view reasonCode) noexcept {
  receipt.status = status;
  receipt.reasonCode = reasonCode;
}

}  // namespace

std::string_view toString(CreativeAuthoredAssetStatus status) noexcept {
  switch (status) {
    case CreativeAuthoredAssetStatus::NotRequested: return "NotRequested";
    case CreativeAuthoredAssetStatus::InvalidDocument: return "InvalidDocument";
    case CreativeAuthoredAssetStatus::InvalidIdentity: return "InvalidIdentity";
    case CreativeAuthoredAssetStatus::EmptySelection: return "EmptySelection";
    case CreativeAuthoredAssetStatus::MissingObject: return "MissingObject";
    case CreativeAuthoredAssetStatus::CapacityExceeded: return "CapacityExceeded";
    case CreativeAuthoredAssetStatus::InvalidGeometry: return "InvalidGeometry";
    case CreativeAuthoredAssetStatus::InvalidDefinition: return "InvalidDefinition";
    case CreativeAuthoredAssetStatus::CreateRejected: return "CreateRejected";
    case CreativeAuthoredAssetStatus::MutationRejected: return "MutationRejected";
    case CreativeAuthoredAssetStatus::Ready: return "Ready";
    case CreativeAuthoredAssetStatus::Instantiated: return "Instantiated";
  }
  return "Unknown";
}

bool isValidCreativeAuthoredAssetId(std::string_view assetId) noexcept {
  if (assetId.empty() || assetId.size() > kCreativeAuthoredAssetIdCapacity) {
    return false;
  }
  return std::all_of(assetId.begin(), assetId.end(), [](char character) {
    const unsigned char byte = static_cast<unsigned char>(character);
    return std::isalnum(byte) != 0 || character == '_' || character == '-';
  });
}

CreativeAuthoredAssetLoadResult loadCreativeAuthoredAssetDefinition(
    const CreativeDocument& storageDocument,
    std::string_view assetId,
    std::string_view label) {
  CreativeAuthoredAssetLoadResult result;
  result.requested = true;
  if (!storageDocument.isValid() ||
      storageDocument.id() == kInvalidDocumentId) {
    result.status = CreativeAuthoredAssetStatus::InvalidDocument;
    result.reasonCode = "creative_authored_asset_document_invalid";
    return result;
  }
  if (!isValidCreativeAuthoredAssetId(assetId) || !nonBlank(label)) {
    result.status = CreativeAuthoredAssetStatus::InvalidIdentity;
    result.reasonCode = "creative_authored_asset_identity_invalid";
    return result;
  }
  if (storageDocument.objectCount() == 0U) {
    result.status = CreativeAuthoredAssetStatus::EmptySelection;
    result.reasonCode = "creative_authored_asset_content_empty";
    return result;
  }
  if (storageDocument.objectCount() > kCreativeAuthoredAssetObjectCapacity) {
    result.status = CreativeAuthoredAssetStatus::CapacityExceeded;
    result.reasonCode = "creative_authored_asset_capacity_exceeded";
    return result;
  }

  std::vector<CreativeObjectId> objectIds;
  objectIds.reserve(storageDocument.objectCount());
  for (const CreativeObject& object : storageDocument.objects()) {
    objectIds.push_back(object.id);
  }
  CreativeClipboard content;
  const CreativeClipboardCopyReceipt copy = copyDocumentObjectsToClipboard(
      storageDocument, objectIds, content);
  CreativeBounds sourceBounds;
  if (!copy.accepted || !definitionBounds(storageDocument, sourceBounds)) {
    result.status = CreativeAuthoredAssetStatus::InvalidGeometry;
    result.reasonCode = "creative_authored_asset_geometry_invalid";
    return result;
  }

  result.definition.assetId = assetId;
  result.definition.label = label;
  result.definition.sourceBounds = sourceBounds;
  result.definition.rootObjectIds =
      definitionRootIds(storageDocument.objects());
  result.definition.content = std::move(content);
  if (result.definition.rootObjectIds.empty()) {
    result.status = CreativeAuthoredAssetStatus::InvalidDefinition;
    result.reasonCode = "creative_authored_asset_roots_missing";
    return result;
  }
  result.accepted = true;
  result.status = CreativeAuthoredAssetStatus::Ready;
  result.reasonCode = "creative_authored_asset_ready";
  return result;
}

CreativeAuthoredAssetCaptureResult captureCreativeAuthoredAsset(
    const CreativeAuthoredAssetCaptureRequest& request) {
  CreativeAuthoredAssetCaptureResult result;
  result.requested = true;
  if (request.sourceDocument == nullptr ||
      !request.sourceDocument->isValid() ||
      request.sourceDocument->id() == kInvalidDocumentId ||
      request.definitionDocumentId == kInvalidDocumentId) {
    reject(result, CreativeAuthoredAssetStatus::InvalidDocument,
           "creative_authored_asset_document_invalid");
    return result;
  }
  if (!isValidCreativeAuthoredAssetId(request.assetId) ||
      !nonBlank(request.label)) {
    reject(result, CreativeAuthoredAssetStatus::InvalidIdentity,
           "creative_authored_asset_identity_invalid");
    return result;
  }
  const CreativeHierarchySelection hierarchy =
      resolveCreativeObjectHierarchy(*request.sourceDocument,
                                     request.selectedObjectIds);
  if (!hierarchy.accepted) {
    const CreativeAuthoredAssetStatus status =
        hierarchy.status == CreativeHierarchySelectionStatus::EmptySelection
            ? CreativeAuthoredAssetStatus::EmptySelection
        : hierarchy.status == CreativeHierarchySelectionStatus::MissingObject
            ? CreativeAuthoredAssetStatus::MissingObject
            : CreativeAuthoredAssetStatus::InvalidDocument;
    reject(result, status, hierarchy.reasonCode, hierarchy.missingObjectId);
    return result;
  }
  if (hierarchy.objectIds.size() > kCreativeAuthoredAssetObjectCapacity) {
    reject(result, CreativeAuthoredAssetStatus::CapacityExceeded,
           "creative_authored_asset_capacity_exceeded");
    return result;
  }

  CreativeClipboard source;
  const CreativeClipboardCopyReceipt copied = copyDocumentObjectsToClipboard(
      *request.sourceDocument, hierarchy.objectIds, source);
  if (!copied.accepted || !source.hasPlacementAnchor) {
    reject(result, CreativeAuthoredAssetStatus::InvalidGeometry,
           "creative_authored_asset_geometry_invalid", copied.failedObjectId);
    return result;
  }

  CreativeDocument storage =
      CreativeDocument::create(std::string(request.label));
  if (!storage.assignId(request.definitionDocumentId)) {
    reject(result, CreativeAuthoredAssetStatus::InvalidDocument,
           "creative_authored_asset_document_id_invalid");
    return result;
  }
  CreativeClipboardPasteRequest paste;
  paste.offset = {-source.placementAnchor.x, -source.placementAnchor.y,
                  -source.placementAnchor.z};
  paste.appendCopySuffix = false;
  const CreativeClipboardPasteReceipt pasted =
      pasteCreativeClipboardAtomically(storage, source, paste);
  if (!pasted.accepted) {
    reject(result, CreativeAuthoredAssetStatus::CreateRejected,
           "creative_authored_asset_normalize_rejected",
           pasted.failedObjectId);
    return result;
  }

  CreativeAuthoredAssetLoadResult loaded =
      loadCreativeAuthoredAssetDefinition(storage, request.assetId,
                                          request.label);
  if (!loaded.accepted) {
    reject(result, loaded.status, loaded.reasonCode);
    return result;
  }
  result.accepted = true;
  result.status = CreativeAuthoredAssetStatus::Ready;
  result.definition = std::move(loaded.definition);
  result.storageDocument = std::move(storage);
  result.capturedObjectCount = hierarchy.objectIds.size();
  result.reasonCode = "creative_authored_asset_captured";
  return result;
}

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
  if (!isFiniteCreativeVec3(request.targetAnchor) ||
      !std::isfinite(request.yawRadians)) {
    plan.status = CreativeAuthoredAssetStatus::InvalidGeometry;
    plan.reasonCode = "creative_authored_asset_placement_invalid";
    return plan;
  }

  plan.rootRequest.kind = CreativeObjectKind::PrefabInstance;
  plan.rootRequest.name = request.definition->label;
  plan.rootRequest.assetId = request.definition->assetId;
  plan.rootRequest.transform.position = request.targetAnchor;
  plan.rootRequest.transform.rotationEulerRadians.y = request.yawRadians;
  plan.rootRequest.hasTransformOverride = true;
  plan.rootRequest.bounds =
      translatedBounds(request.definition->sourceBounds, request.targetAnchor);
  plan.rootRequest.hasBoundsOverride = true;
  plan.rootRequest.visible = false;
  plan.rootRequest.hasVisibleOverride = true;
  plan.rootRequest.locked = false;
  plan.rootRequest.hasLockedOverride = true;
  plan.rootRequest.parentId = request.parentId;

  const CreativeVec3 sourceAnchor =
      request.definition->content.hasPlacementAnchor
          ? request.definition->content.placementAnchor
          : CreativeVec3{};
  plan.contentPasteRequest.offset = {
      request.targetAnchor.x - sourceAnchor.x,
      request.targetAnchor.y - sourceAnchor.y,
      request.targetAnchor.z - sourceAnchor.z,
  };
  plan.contentPasteRequest.hasTransformAnchor = true;
  plan.contentPasteRequest.transformAnchor = sourceAnchor;
  plan.contentPasteRequest.hasAxisAngleRotation = true;
  plan.contentPasteRequest.rotationAxis = CreativeAxis3::Y;
  plan.contentPasteRequest.rotationRadians = request.yawRadians;
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
  receipt.contentPasteReceipt = pasteCreativeClipboardAtomically(
      staged, request.definition->content, plan.contentPasteRequest);
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
