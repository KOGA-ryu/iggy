#include "app/iggy3d/creative/tools/Clipboard.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numeric>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/tools/SelectionPlacement.hpp"

namespace iggy3d::creative {
namespace {

[[nodiscard]] CreativeVec3 add(CreativeVec3 lhs, CreativeVec3 rhs) noexcept {
  return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

void includePlacementPoint(CreativeObjectWorldExtent& extent,
                           CreativeVec3 point) noexcept {
  if (!extent.valid) {
    extent.min = point;
    extent.max = point;
    extent.valid = true;
    return;
  }
  extent.min.x = std::min(extent.min.x, point.x);
  extent.min.y = std::min(extent.min.y, point.y);
  extent.min.z = std::min(extent.min.z, point.z);
  extent.max.x = std::max(extent.max.x, point.x);
  extent.max.y = std::max(extent.max.y, point.y);
  extent.max.z = std::max(extent.max.z, point.z);
}

[[nodiscard]] bool resolveClipboardPlacementAnchor(
    std::span<const CreativeObject> objects,
    CreativeVec3& outAnchor) noexcept {
  if (objects.empty()) {
    return false;
  }
  CreativeObjectWorldExtent selection;
  for (const CreativeObject& object : objects) {
    const CreativeObjectWorldExtent objectExtent =
        resolveCreativeObjectWorldExtent(object);
    if (!objectExtent.valid || !isFiniteCreativeVec3(objectExtent.min) ||
        !isFiniteCreativeVec3(objectExtent.max)) {
      return false;
    }
    includePlacementPoint(selection, objectExtent.min);
    includePlacementPoint(selection, objectExtent.max);
  }
  outAnchor = {std::midpoint(selection.min.x, selection.max.x),
               selection.min.y,
               std::midpoint(selection.min.z, selection.max.z)};
  return isFiniteCreativeVec3(outAnchor);
}

[[nodiscard]] bool validExternalParentPolicy(
    CreativeClipboardExternalParentPolicy policy) noexcept {
  return policy == CreativeClipboardExternalParentPolicy::Detach ||
         policy == CreativeClipboardExternalParentPolicy::PreserveIfPresent;
}

[[nodiscard]] bool validClipboardObject(const CreativeObject& object) noexcept {
  if (object.id == kInvalidObjectId ||
      object.kind == CreativeObjectKind::Unknown ||
      object.kind == CreativeObjectKind::Count ||
      !isFiniteCreativeVec3(object.transform.position) ||
      !isFiniteCreativeVec3(object.transform.rotationEulerRadians) ||
      !isPositiveCreativeVec3(object.transform.scale) ||
      !isFiniteCreativeVec3(object.bounds.min) ||
      !isFiniteCreativeVec3(object.bounds.max)) {
    return false;
  }
  return std::all_of(object.pathPoints.begin(), object.pathPoints.end(),
                     [](const CreativePathPoint& point) {
                       return isFiniteCreativeVec3(point.position);
                     });
}

[[nodiscard]] bool buildParentFirstOrder(
    std::span<const CreativeObject> objects,
    std::vector<std::size_t>& order) {
  std::unordered_map<CreativeObjectId, std::size_t> indices;
  indices.reserve(objects.size());
  for (std::size_t index = 0; index < objects.size(); ++index) {
    if (!validClipboardObject(objects[index]) ||
        !indices.emplace(objects[index].id, index).second) {
      return false;
    }
  }

  std::vector<std::uint8_t> state(objects.size(), 0U);
  std::vector<std::size_t> depth(objects.size(), 0U);
  const auto visit = [&](auto&& self, std::size_t index) -> bool {
    if (state[index] == 2U) {
      return true;
    }
    if (state[index] == 1U) {
      return false;
    }
    state[index] = 1U;
    if (objects[index].parentId.has_value()) {
      const auto parent = indices.find(*objects[index].parentId);
      if (parent != indices.end()) {
        if (!self(self, parent->second)) {
          return false;
        }
        depth[index] = depth[parent->second] + 1U;
      }
    }
    state[index] = 2U;
    return true;
  };

  order.resize(objects.size());
  std::iota(order.begin(), order.end(), 0U);
  for (std::size_t index : order) {
    if (!visit(visit, index)) {
      return false;
    }
  }
  std::stable_sort(order.begin(), order.end(),
                   [&depth](std::size_t lhs, std::size_t rhs) {
                     return depth[lhs] < depth[rhs];
                   });
  return true;
}

[[nodiscard]] CreativeDocumentCreateRequest makePasteRequest(
    const CreativeDocument& targetDocument,
    const CreativeObject& object,
    const CreativeClipboardPasteRequest& request,
    const std::unordered_map<CreativeObjectId, CreativeObjectId>& remaps) {
  const CreativeObjectDescriptor& descriptor = describeObject(object.kind);
  CreativeDocumentCreateRequest create;
  create.kind = object.kind;
  create.name = request.appendCopySuffix ? object.name + " Copy" : object.name;
  create.transform = object.transform;
  create.hasTransformOverride = descriptor.hasTransform;
  create.bounds = object.bounds;
  create.hasBoundsOverride = descriptor.hasBounds;
  create.layerId = object.layerId;
  create.hasLayerOverride = true;
  create.visible = object.visible;
  create.hasVisibleOverride = true;
  create.locked = object.locked;
  create.hasLockedOverride = true;
  create.tags = object.tags;
  if (object.parentId.has_value()) {
    const auto remappedParent = remaps.find(*object.parentId);
    if (remappedParent != remaps.end()) {
      create.parentId = remappedParent->second;
    } else if (request.externalParentPolicy ==
                   CreativeClipboardExternalParentPolicy::PreserveIfPresent &&
               targetDocument.containsObject(*object.parentId)) {
      create.parentId = object.parentId;
    }
  }
  create.pathPoints = object.pathPoints;
  create.hasPathOverride = !object.pathPoints.empty();
  return create;
}

[[nodiscard]] bool validPasteRequest(
    const CreativeDocumentCreateRequest& request) noexcept {
  if (request.hasTransformOverride &&
      (!isFiniteCreativeVec3(request.transform.position) ||
       !isFiniteCreativeVec3(request.transform.rotationEulerRadians) ||
       !isPositiveCreativeVec3(request.transform.scale))) {
    return false;
  }
  if (request.hasBoundsOverride &&
      (!isFiniteCreativeVec3(request.bounds.min) ||
       !isFiniteCreativeVec3(request.bounds.max))) {
    return false;
  }
  return std::all_of(request.pathPoints.begin(), request.pathPoints.end(),
                     [](const CreativePathPoint& point) {
                       return isFiniteCreativeVec3(point.position);
                     });
}

}  // namespace

std::string_view toString(CreativeClipboardStatus status) noexcept {
  switch (status) {
    case CreativeClipboardStatus::NotRequested:
      return "NotRequested";
    case CreativeClipboardStatus::EmptySelection:
      return "EmptySelection";
    case CreativeClipboardStatus::MissingObject:
      return "MissingObject";
    case CreativeClipboardStatus::InvalidClipboard:
      return "InvalidClipboard";
    case CreativeClipboardStatus::InvalidRequest:
      return "InvalidRequest";
    case CreativeClipboardStatus::ObjectIdExhausted:
      return "ObjectIdExhausted";
    case CreativeClipboardStatus::CreateRejected:
      return "CreateRejected";
    case CreativeClipboardStatus::RemoveRejected:
      return "RemoveRejected";
    case CreativeClipboardStatus::Copied:
      return "Copied";
    case CreativeClipboardStatus::Cut:
      return "Cut";
    case CreativeClipboardStatus::Pasted:
      return "Pasted";
  }
  return "Unknown";
}

bool creativeClipboardEmpty(const CreativeClipboard& clipboard) noexcept {
  return clipboard.objects.empty();
}

void clearCreativeClipboard(CreativeClipboard& clipboard) noexcept {
  clipboard = {};
}

CreativeClipboardCopyReceipt copyDocumentObjectsToClipboard(
    const CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds,
    CreativeClipboard& outClipboard) {
  CreativeClipboardCopyReceipt receipt;
  receipt.requested = true;
  receipt.requestedObjectCount = objectIds.size();
  if (!document.isValid() || document.id() == kInvalidDocumentId) {
    receipt.status = CreativeClipboardStatus::InvalidClipboard;
    receipt.reasonCode = "creative_clipboard_source_document_invalid";
    return receipt;
  }
  if (objectIds.empty()) {
    receipt.status = CreativeClipboardStatus::EmptySelection;
    receipt.reasonCode = "creative_clipboard_selection_empty";
    return receipt;
  }

  std::unordered_set<CreativeObjectId> requested;
  requested.reserve(objectIds.size());
  for (CreativeObjectId objectId : objectIds) {
    if (objectId == kInvalidObjectId) {
      receipt.status = CreativeClipboardStatus::MissingObject;
      receipt.failedObjectId = objectId;
      receipt.reasonCode = "creative_clipboard_object_missing";
      return receipt;
    }
    requested.insert(objectId);
  }

  CreativeClipboard staged;
  staged.sourceDocumentId = document.id();
  staged.sourceRevision = document.revision();
  staged.objects.reserve(requested.size());
  for (const CreativeObject& object : document.objects()) {
    if (requested.contains(object.id)) {
      staged.objects.push_back(object);
    }
  }
  if (staged.objects.size() != requested.size()) {
    for (CreativeObjectId objectId : requested) {
      if (!document.containsObject(objectId)) {
        receipt.failedObjectId = objectId;
        break;
      }
    }
    receipt.status = CreativeClipboardStatus::MissingObject;
    receipt.reasonCode = "creative_clipboard_object_missing";
    return receipt;
  }

  std::vector<std::size_t> parentOrder;
  if (!buildParentFirstOrder(staged.objects, parentOrder)) {
    receipt.status = CreativeClipboardStatus::InvalidClipboard;
    receipt.reasonCode = "creative_clipboard_parent_graph_invalid";
    return receipt;
  }
  if (!resolveClipboardPlacementAnchor(staged.objects,
                                       staged.placementAnchor)) {
    receipt.status = CreativeClipboardStatus::InvalidClipboard;
    receipt.reasonCode = "creative_clipboard_placement_anchor_invalid";
    return receipt;
  }
  staged.hasPlacementAnchor = true;

  receipt.accepted = true;
  receipt.status = CreativeClipboardStatus::Copied;
  receipt.copiedObjectCount = staged.objects.size();
  receipt.reasonCode = "creative_clipboard_copied";
  outClipboard = std::move(staged);
  return receipt;
}

CreativeClipboardPasteReceipt pasteCreativeClipboardAtomically(
    CreativeDocument& document,
    const CreativeClipboard& clipboard,
    const CreativeClipboardPasteRequest& request) {
  const std::array requests{request};
  CreativeClipboardBatchPasteReceipt batch =
      pasteCreativeClipboardBatchAtomically(document, clipboard, requests);
  CreativeClipboardPasteReceipt receipt;
  receipt.requested = batch.requested;
  receipt.accepted = batch.accepted;
  receipt.changed = batch.changed;
  receipt.status = batch.status;
  receipt.requestedObjectCount = batch.requestedObjectCount;
  receipt.pastedObjectCount = batch.pastedObjectCount;
  receipt.failedObjectId = batch.failedObjectId;
  receipt.revisionBefore = batch.revisionBefore;
  receipt.revisionAfter = batch.revisionAfter;
  receipt.idRemaps = std::move(batch.idRemaps);
  receipt.pastedObjectIds = std::move(batch.pastedObjectIds);
  receipt.reasonCode = std::move(batch.reasonCode);
  return receipt;
}

CreativeClipboardBatchPasteReceipt pasteCreativeClipboardBatchAtomically(
    CreativeDocument& document,
    const CreativeClipboard& clipboard,
    std::span<const CreativeClipboardPasteRequest> requests) {
  CreativeClipboardBatchPasteReceipt receipt;
  receipt.requested = true;
  receipt.requestedPasteCount = requests.size();
  receipt.revisionBefore = document.revision();
  receipt.revisionAfter = receipt.revisionBefore;
  if (!document.isValid() || document.id() == kInvalidDocumentId) {
    receipt.status = CreativeClipboardStatus::InvalidRequest;
    receipt.reasonCode = "creative_clipboard_target_document_invalid";
    return receipt;
  }
  if (creativeClipboardEmpty(clipboard)) {
    receipt.status = CreativeClipboardStatus::InvalidClipboard;
    receipt.reasonCode = "creative_clipboard_empty";
    return receipt;
  }
  if (requests.empty()) {
    receipt.status = CreativeClipboardStatus::InvalidRequest;
    receipt.reasonCode = "creative_clipboard_batch_empty";
    return receipt;
  }

  if (clipboard.objects.size() >
      std::numeric_limits<std::uint64_t>::max() / requests.size()) {
    receipt.status = CreativeClipboardStatus::InvalidRequest;
    receipt.reasonCode = "creative_clipboard_batch_size_overflow";
    return receipt;
  }
  receipt.requestedObjectCount =
      static_cast<std::uint64_t>(clipboard.objects.size()) * requests.size();
  if (receipt.requestedObjectCount >
      std::numeric_limits<std::size_t>::max()) {
    receipt.status = CreativeClipboardStatus::InvalidRequest;
    receipt.reasonCode = "creative_clipboard_batch_size_overflow";
    return receipt;
  }

  for (std::size_t requestIndex = 0; requestIndex < requests.size();
       ++requestIndex) {
    const CreativeClipboardPasteRequest& request = requests[requestIndex];
    if (!isFiniteCreativeVec3(request.offset)) {
      receipt.failedPasteIndex = requestIndex;
      receipt.status = CreativeClipboardStatus::InvalidRequest;
      receipt.reasonCode = "creative_clipboard_offset_invalid";
      return receipt;
    }
    if (request.quarterTurns > 3U) {
      receipt.failedPasteIndex = requestIndex;
      receipt.status = CreativeClipboardStatus::InvalidRequest;
      receipt.reasonCode = "creative_clipboard_rotation_invalid";
      return receipt;
    }
    if ((request.hasTransformAnchor &&
         !isFiniteCreativeVec3(request.transformAnchor)) ||
        !isValidCreativeAxis3(request.rotationAxis) ||
        !std::isfinite(request.rotationRadians) ||
        (request.hasAxisAngleRotation &&
         (request.quarterTurns != 0U || request.mirrorX ||
          request.mirrorZ))) {
      receipt.failedPasteIndex = requestIndex;
      receipt.status = CreativeClipboardStatus::InvalidRequest;
      receipt.reasonCode = "creative_clipboard_rigid_transform_invalid";
      return receipt;
    }
    if (!validExternalParentPolicy(request.externalParentPolicy)) {
      receipt.failedPasteIndex = requestIndex;
      receipt.status = CreativeClipboardStatus::InvalidRequest;
      receipt.reasonCode = "creative_clipboard_parent_policy_invalid";
      return receipt;
    }
  }

  std::vector<std::size_t> parentOrder;
  if (!buildParentFirstOrder(clipboard.objects, parentOrder)) {
    receipt.status = CreativeClipboardStatus::InvalidClipboard;
    receipt.reasonCode = "creative_clipboard_parent_graph_invalid";
    return receipt;
  }

  const CreativeObjectId nextObjectId = document.nextObjectId();
  const CreativeObjectId remainingIds =
      std::numeric_limits<CreativeObjectId>::max() - nextObjectId;
  if (nextObjectId == kInvalidObjectId ||
      receipt.requestedObjectCount > remainingIds) {
    receipt.status = CreativeClipboardStatus::ObjectIdExhausted;
    receipt.reasonCode = "creative_clipboard_object_id_exhausted";
    return receipt;
  }

  CreativeDocument staged = document;
  const std::size_t totalObjectCount =
      static_cast<std::size_t>(receipt.requestedObjectCount);
  receipt.idRemaps.reserve(totalObjectCount);
  receipt.pastedObjectIds.reserve(totalObjectCount);
  for (std::size_t requestIndex = 0; requestIndex < requests.size();
       ++requestIndex) {
    const CreativeClipboardPasteRequest& request = requests[requestIndex];
    CreativeSelectionPlacementRequest placementRequest;
    placementRequest.mode = CreativeSelectionPlacementMode::Copy;
    placementRequest.sourceAnchor = request.hasTransformAnchor
                                        ? request.transformAnchor
                                        : clipboard.hasPlacementAnchor
                                              ? clipboard.placementAnchor
                                              : CreativeVec3{};
    placementRequest.targetAnchor =
        add(placementRequest.sourceAnchor, request.offset);
    placementRequest.quarterTurns = request.quarterTurns;
    placementRequest.mirrorX = request.mirrorX;
    placementRequest.mirrorZ = request.mirrorZ;
    placementRequest.hasAxisAngleRotation = request.hasAxisAngleRotation;
    placementRequest.rotationAxis = request.rotationAxis;
    placementRequest.rotationRadians = request.rotationRadians;
    const CreativeSelectionPlacementPlan placementPlan =
        planCreativeSelectionPlacement(clipboard.objects, placementRequest);
    if (!placementPlan.accepted) {
      receipt.failedPasteIndex = requestIndex;
      receipt.failedObjectId = placementPlan.failedObjectId;
      receipt.status = CreativeClipboardStatus::InvalidRequest;
      receipt.reasonCode = placementPlan.reasonCode;
      receipt.pastedPasteCount = 0U;
      receipt.idRemaps.clear();
      receipt.pastedObjectIds.clear();
      return receipt;
    }
    const CreativeObjectId pasteStartId = staged.nextObjectId();
    std::unordered_map<CreativeObjectId, CreativeObjectId> remaps;
    remaps.reserve(clipboard.objects.size());
    for (std::size_t ordinal = 0; ordinal < parentOrder.size(); ++ordinal) {
      const CreativeObjectId sourceId =
          clipboard.objects[parentOrder[ordinal]].id;
      const CreativeObjectId pastedId = pasteStartId + ordinal;
      remaps.emplace(sourceId, pastedId);
      receipt.idRemaps.push_back({sourceId, pastedId});
    }

    for (std::size_t index : parentOrder) {
      const CreativeObject& object = placementPlan.objects[index];
      const CreativeDocumentCreateRequest createRequest =
          makePasteRequest(staged, object, request, remaps);
      if (!validPasteRequest(createRequest)) {
        receipt.failedPasteIndex = requestIndex;
        receipt.failedObjectId = object.id;
        receipt.status = CreativeClipboardStatus::InvalidRequest;
        receipt.reasonCode = "creative_clipboard_output_invalid";
        receipt.pastedPasteCount = 0U;
        receipt.idRemaps.clear();
        receipt.pastedObjectIds.clear();
        return receipt;
      }
      const CreativeDocumentCreateReceipt createReceipt =
          staged.createObject(createRequest);
      if (!createReceipt.accepted || !createReceipt.objectCreated ||
          !createReceipt.changed ||
          createReceipt.objectId != remaps.at(object.id)) {
        receipt.failedPasteIndex = requestIndex;
        receipt.failedObjectId = object.id;
        receipt.status = CreativeClipboardStatus::CreateRejected;
        receipt.reasonCode = std::string(createReceipt.reasonCode);
        receipt.pastedPasteCount = 0U;
        receipt.idRemaps.clear();
        receipt.pastedObjectIds.clear();
        return receipt;
      }
      receipt.pastedObjectIds.push_back(createReceipt.objectId);
    }
    ++receipt.pastedPasteCount;
  }

  document = std::move(staged);
  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = CreativeClipboardStatus::Pasted;
  receipt.pastedObjectCount = receipt.pastedObjectIds.size();
  receipt.revisionAfter = document.revision();
  receipt.reasonCode = "creative_clipboard_pasted";
  return receipt;
}

CreativeClipboardCutReceipt cutDocumentObjectsAtomically(
    CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds,
    CreativeClipboard& outClipboard) {
  CreativeClipboardCutReceipt receipt;
  receipt.requested = true;
  receipt.requestedObjectCount = objectIds.size();
  receipt.revisionBefore = document.revision();
  receipt.revisionAfter = receipt.revisionBefore;
  if (!document.isValid() || document.id() == kInvalidDocumentId) {
    receipt.status = CreativeClipboardStatus::InvalidRequest;
    receipt.reasonCode = "creative_clipboard_source_document_invalid";
    return receipt;
  }

  CreativeClipboard stagedClipboard;
  receipt.copyReceipt = copyDocumentObjectsToClipboard(
      document, objectIds, stagedClipboard);
  if (!receipt.copyReceipt.accepted) {
    receipt.status = receipt.copyReceipt.status;
    receipt.failedObjectId = receipt.copyReceipt.failedObjectId;
    receipt.reasonCode = receipt.copyReceipt.reasonCode;
    return receipt;
  }

  std::vector<std::size_t> parentOrder;
  if (!buildParentFirstOrder(stagedClipboard.objects, parentOrder)) {
    receipt.status = CreativeClipboardStatus::InvalidClipboard;
    receipt.reasonCode = "creative_clipboard_parent_graph_invalid";
    return receipt;
  }

  CreativeDocument stagedDocument = document;
  receipt.removeReceipts.reserve(parentOrder.size());
  for (auto iterator = parentOrder.rbegin(); iterator != parentOrder.rend();
       ++iterator) {
    const CreativeObjectId objectId = stagedClipboard.objects[*iterator].id;
    const CreativeDocumentRemoveReceipt removeReceipt =
        stagedDocument.removeDocumentObject(objectId);
    receipt.removeReceipts.push_back(removeReceipt);
    if (!removeReceipt.accepted || !removeReceipt.objectRemoved ||
        !removeReceipt.changed) {
      receipt.failedObjectId = objectId;
      receipt.status = CreativeClipboardStatus::RemoveRejected;
      receipt.reasonCode = std::string(removeReceipt.reasonCode);
      return receipt;
    }
  }

  document = std::move(stagedDocument);
  outClipboard = std::move(stagedClipboard);
  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = CreativeClipboardStatus::Cut;
  receipt.cutObjectCount = receipt.removeReceipts.size();
  receipt.revisionAfter = document.revision();
  receipt.reasonCode = "creative_clipboard_cut";
  return receipt;
}

}  // namespace iggy3d::creative
