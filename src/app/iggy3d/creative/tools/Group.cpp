#include "app/iggy3d/creative/tools/Group.hpp"

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/Hierarchy.hpp"

#include <algorithm>
#include <optional>
#include <unordered_set>
#include <utility>

namespace iggy3d::creative {
namespace {

[[nodiscard]] bool hierarchyContainsAncestor(
    const CreativeDocument& document,
    const std::unordered_set<CreativeObjectId>& candidates,
    const CreativeObject& object) noexcept {
  std::optional<CreativeObjectId> parent = object.parentId;
  while (parent.has_value()) {
    if (candidates.contains(*parent)) {
      return true;
    }
    const CreativeObject* parentObject = document.findObject(*parent);
    if (parentObject == nullptr) {
      return false;
    }
    parent = parentObject->parentId;
  }
  return false;
}

[[nodiscard]] bool objectDescendsFromRoots(
    const CreativeDocument& document,
    const std::unordered_set<CreativeObjectId>& roots,
    const CreativeObject& object) noexcept {
  const CreativeObject* current = &object;
  while (current != nullptr) {
    if (roots.contains(current->id)) {
      return true;
    }
    current = current->parentId.has_value()
                  ? document.findObject(*current->parentId)
                  : nullptr;
  }
  return false;
}

[[nodiscard]] CreativeGroupCommandReceipt commandReceipt(
    const CreativeDocument& document,
    CreativeGroupCommandKind kind,
    std::size_t requestedObjectCount) noexcept {
  CreativeGroupCommandReceipt receipt;
  receipt.requested = true;
  receipt.kind = kind;
  receipt.requestedObjectCount = requestedObjectCount;
  receipt.revisionBefore = document.revision();
  receipt.revisionAfter = receipt.revisionBefore;
  return receipt;
}

void reject(CreativeGroupCommandReceipt& receipt,
            CreativeGroupCommandStatus status,
            std::string_view reasonCode,
            CreativeObjectId failedObjectId = kInvalidObjectId) noexcept {
  receipt.status = status;
  receipt.failedObjectId = failedObjectId;
  receipt.reasonCode = reasonCode;
}

void normalizeMutationBatchRevisionRange(
    CreativeDocumentBatchMutationReceipt& receipt,
    std::uint64_t revisionBefore,
    std::uint64_t revisionAfter) noexcept {
  if (receipt.status == CreativeDocumentMutationStatus::Unknown) {
    return;
  }
  receipt.revisionBefore = revisionBefore;
  receipt.revisionAfter = revisionAfter;
  for (CreativeDocumentMutationReceipt& item : receipt.receipts) {
    item.revisionBefore = revisionBefore;
    item.revisionAfter = revisionAfter;
  }
  if (receipt.publicationAttempted) {
    receipt.publicationReceipt.revisionBefore = revisionBefore;
    receipt.publicationReceipt.revisionAfter = revisionAfter;
  }
}

void normalizeGroupNestedRevisionRange(
    CreativeGroupCommandReceipt& receipt,
    std::uint64_t revisionAfter) noexcept {
  if (receipt.createReceipt.requested) {
    receipt.createReceipt.revisionBefore = receipt.revisionBefore;
    receipt.createReceipt.revisionAfter = revisionAfter;
  }
  normalizeMutationBatchRevisionRange(
      receipt.mutationReceipt, receipt.revisionBefore, revisionAfter);
  if (receipt.removeReceipt.requested) {
    receipt.removeReceipt.revisionBefore = receipt.revisionBefore;
    receipt.removeReceipt.revisionAfter = revisionAfter;
  }
}

void clearUnpublishedGroupOutputs(
    CreativeGroupCommandReceipt& receipt) noexcept {
  if (receipt.kind == CreativeGroupCommandKind::Group) {
    receipt.groupObjectId = kInvalidObjectId;
  }
  receipt.affectedObjectCount = 0U;
  receipt.selectionObjectIds.clear();
}

void normalizeRemoveReceiptRevisionRange(
    CreativeDocumentRemoveReceipt& receipt,
    std::uint64_t revisionBefore,
    std::uint64_t revisionAfter) noexcept {
  if (!receipt.requested) {
    return;
  }
  receipt.revisionBefore = revisionBefore;
  receipt.revisionAfter = revisionAfter;
}

[[nodiscard]] bool includeExtent(CreativeObjectWorldExtent& aggregate,
                                 const CreativeObjectWorldExtent& extent) {
  if (!extent.valid) {
    return false;
  }
  if (!aggregate.valid) {
    aggregate = extent;
    return true;
  }
  aggregate.min.x = std::min(aggregate.min.x, extent.min.x);
  aggregate.min.y = std::min(aggregate.min.y, extent.min.y);
  aggregate.min.z = std::min(aggregate.min.z, extent.min.z);
  aggregate.max.x = std::max(aggregate.max.x, extent.max.x);
  aggregate.max.y = std::max(aggregate.max.y, extent.max.y);
  aggregate.max.z = std::max(aggregate.max.z, extent.max.z);
  return true;
}

[[nodiscard]] bool hierarchyPivot(
    const CreativeDocument& document,
    const CreativeHierarchySelection& hierarchy,
    CreativeVec3& pivot) {
  CreativeObjectWorldExtent aggregate;
  for (CreativeObjectId objectId : hierarchy.objectIds) {
    const CreativeObject* object = document.findObject(objectId);
    if (object == nullptr || object->kind == CreativeObjectKind::Group) {
      continue;
    }
    static_cast<void>(
        includeExtent(aggregate, resolveCreativeObjectWorldExtent(*object)));
  }
  if (!aggregate.valid) {
    for (CreativeObjectId objectId : hierarchy.rootObjectIds) {
      const CreativeObject* object = document.findObject(objectId);
      if (object != nullptr) {
        static_cast<void>(includeExtent(
            aggregate, resolveCreativeObjectWorldExtent(*object)));
      }
    }
  }
  if (!aggregate.valid) {
    return false;
  }
  pivot = {(aggregate.min.x + aggregate.max.x) * 0.5,
           (aggregate.min.y + aggregate.max.y) * 0.5,
           (aggregate.min.z + aggregate.max.z) * 0.5};
  return isFiniteCreativeVec3(pivot);
}

[[nodiscard]] std::size_t objectDepthFromRoot(
    const CreativeDocument& document,
    CreativeObjectId rootObjectId,
    CreativeObjectId objectId) noexcept {
  std::size_t depth = 0U;
  const CreativeObject* object = document.findObject(objectId);
  while (object != nullptr && object->id != rootObjectId &&
         object->parentId.has_value()) {
    ++depth;
    object = document.findObject(*object->parentId);
  }
  return depth;
}

[[nodiscard]] std::size_t objectHierarchyDepth(
    const CreativeDocument& document,
    CreativeObjectId objectId) noexcept {
  std::size_t depth = 0U;
  const CreativeObject* object = document.findObject(objectId);
  while (object != nullptr && object->parentId.has_value()) {
    ++depth;
    object = document.findObject(*object->parentId);
  }
  return depth;
}

}  // namespace

std::string_view toString(CreativeHierarchySelectionStatus status) noexcept {
  switch (status) {
    case CreativeHierarchySelectionStatus::NotRequested:
      return "NotRequested";
    case CreativeHierarchySelectionStatus::EmptySelection:
      return "EmptySelection";
    case CreativeHierarchySelectionStatus::InvalidDocument:
      return "InvalidDocument";
    case CreativeHierarchySelectionStatus::InvalidHierarchy:
      return "InvalidHierarchy";
    case CreativeHierarchySelectionStatus::MissingObject:
      return "MissingObject";
    case CreativeHierarchySelectionStatus::Ready: return "Ready";
  }
  return "Unknown";
}

std::string_view toString(CreativeGroupCommandKind kind) noexcept {
  switch (kind) {
    case CreativeGroupCommandKind::Group: return "Group";
    case CreativeGroupCommandKind::Ungroup: return "Ungroup";
  }
  return "Unknown";
}

std::string_view toString(CreativeGroupCommandStatus status) noexcept {
  switch (status) {
    case CreativeGroupCommandStatus::NotRequested: return "NotRequested";
    case CreativeGroupCommandStatus::EmptySelection: return "EmptySelection";
    case CreativeGroupCommandStatus::RequiresMultipleRoots:
      return "RequiresMultipleRoots";
    case CreativeGroupCommandStatus::InvalidDocument:
      return "InvalidDocument";
    case CreativeGroupCommandStatus::InvalidHierarchy:
      return "InvalidHierarchy";
    case CreativeGroupCommandStatus::MissingObject: return "MissingObject";
    case CreativeGroupCommandStatus::LockedObject: return "LockedObject";
    case CreativeGroupCommandStatus::UnsupportedObject:
      return "UnsupportedObject";
    case CreativeGroupCommandStatus::MixedParents: return "MixedParents";
    case CreativeGroupCommandStatus::DepthExceeded: return "DepthExceeded";
    case CreativeGroupCommandStatus::NotGroup: return "NotGroup";
    case CreativeGroupCommandStatus::CreateRejected:
      return "CreateRejected";
    case CreativeGroupCommandStatus::MutationRejected:
      return "MutationRejected";
    case CreativeGroupCommandStatus::RemoveRejected:
      return "RemoveRejected";
    case CreativeGroupCommandStatus::Applied: return "Applied";
  }
  return "Unknown";
}

std::string_view toString(CreativeGroupPivotStatus status) noexcept {
  switch (status) {
    case CreativeGroupPivotStatus::NotRequested: return "NotRequested";
    case CreativeGroupPivotStatus::InvalidDocument: return "InvalidDocument";
    case CreativeGroupPivotStatus::MissingGroup: return "MissingGroup";
    case CreativeGroupPivotStatus::NotGroup: return "NotGroup";
    case CreativeGroupPivotStatus::LockedGroup: return "LockedGroup";
    case CreativeGroupPivotStatus::InvalidPivot: return "InvalidPivot";
    case CreativeGroupPivotStatus::NoChange: return "NoChange";
    case CreativeGroupPivotStatus::Applied: return "Applied";
    case CreativeGroupPivotStatus::Rejected: return "Rejected";
  }
  return "Unknown";
}

bool creativeObjectIsHierarchyContainer(CreativeObjectKind kind) noexcept {
  return kind == CreativeObjectKind::Group ||
         kind == CreativeObjectKind::PrefabInstance;
}

CreativeHierarchySelection resolveCreativeObjectHierarchy(
    const CreativeDocument& document,
    std::span<const CreativeObjectId> selectedObjectIds) {
  CreativeHierarchySelection result;
  result.requested = true;
  result.requestedObjectCount = selectedObjectIds.size();
  if (selectedObjectIds.empty()) {
    result.status = CreativeHierarchySelectionStatus::EmptySelection;
    result.reasonCode = "creative_hierarchy_selection_empty";
    return result;
  }
  if (!document.isValid()) {
    result.status = CreativeHierarchySelectionStatus::InvalidDocument;
    result.reasonCode = "creative_hierarchy_document_invalid";
    return result;
  }
  if (!validateCreativeObjectParentGraph(document.objects()).empty()) {
    result.status = CreativeHierarchySelectionStatus::InvalidHierarchy;
    result.reasonCode = "creative_hierarchy_parent_graph_invalid";
    return result;
  }

  std::unordered_set<CreativeObjectId> requested;
  requested.reserve(selectedObjectIds.size());
  for (CreativeObjectId objectId : selectedObjectIds) {
    if (objectId == kInvalidObjectId || !document.containsObject(objectId)) {
      result.missingObjectId = objectId;
      result.status = CreativeHierarchySelectionStatus::MissingObject;
      result.reasonCode = "creative_hierarchy_object_missing";
      return result;
    }
    requested.insert(objectId);
  }

  for (const CreativeObject& object : document.objects()) {
    if (requested.contains(object.id) &&
        !hierarchyContainsAncestor(document, requested, object)) {
      result.rootObjectIds.push_back(object.id);
    }
  }
  std::unordered_set<CreativeObjectId> roots(result.rootObjectIds.begin(),
                                             result.rootObjectIds.end());
  for (const CreativeObject& object : document.objects()) {
    if (objectDescendsFromRoots(document, roots, object)) {
      result.objectIds.push_back(object.id);
    }
  }
  result.accepted = true;
  result.status = CreativeHierarchySelectionStatus::Ready;
  result.reasonCode = "creative_hierarchy_ready";
  return result;
}

CreativeObjectId resolveCreativeHierarchyInteractionRoot(
    const CreativeDocument& document,
    std::span<const CreativeObjectId> selectedObjectIds,
    CreativeObjectId hitObjectId) {
  if (hitObjectId == kInvalidObjectId ||
      !document.containsObject(hitObjectId)) {
    return hitObjectId;
  }
  const CreativeHierarchySelection hierarchy =
      resolveCreativeObjectHierarchy(document, selectedObjectIds);
  if (!hierarchy.accepted ||
      std::find(hierarchy.objectIds.begin(), hierarchy.objectIds.end(),
                hitObjectId) == hierarchy.objectIds.end()) {
    return hitObjectId;
  }
  for (CreativeObjectId rootObjectId : hierarchy.rootObjectIds) {
    const CreativeObject* root = document.findObject(rootObjectId);
    if (root == nullptr ||
        !creativeObjectIsHierarchyContainer(root->kind)) {
      continue;
    }
    const CreativeHierarchySelection rootHierarchy =
        resolveCreativeObjectHierarchy(
            document, std::span{&rootObjectId, 1U});
    if (rootHierarchy.accepted &&
        std::find(rootHierarchy.objectIds.begin(),
                  rootHierarchy.objectIds.end(), hitObjectId) !=
            rootHierarchy.objectIds.end()) {
      return rootObjectId;
    }
  }
  return hitObjectId;
}

CreativeGroupCommandReceipt groupDocumentObjectsAtomically(
    CreativeDocument& document,
    std::span<const CreativeObjectId> selectedObjectIds) {
  CreativeGroupCommandReceipt receipt = commandReceipt(
      document, CreativeGroupCommandKind::Group, selectedObjectIds.size());
  const CreativeHierarchySelection hierarchy =
      resolveCreativeObjectHierarchy(document, selectedObjectIds);
  if (!hierarchy.accepted) {
    const CreativeGroupCommandStatus status =
        hierarchy.status == CreativeHierarchySelectionStatus::EmptySelection
            ? CreativeGroupCommandStatus::EmptySelection
        : hierarchy.status == CreativeHierarchySelectionStatus::InvalidDocument
            ? CreativeGroupCommandStatus::InvalidDocument
        : hierarchy.status == CreativeHierarchySelectionStatus::InvalidHierarchy
            ? CreativeGroupCommandStatus::InvalidHierarchy
            : CreativeGroupCommandStatus::MissingObject;
    reject(receipt, status, hierarchy.reasonCode, hierarchy.missingObjectId);
    return receipt;
  }
  if (hierarchy.rootObjectIds.size() < 2U) {
    reject(receipt, CreativeGroupCommandStatus::RequiresMultipleRoots,
           "creative_group_requires_multiple_roots");
    return receipt;
  }

  std::optional<CreativeObjectId> commonParent;
  bool parentInitialized = false;
  for (CreativeObjectId objectId : hierarchy.rootObjectIds) {
    const CreativeObject* object = document.findObject(objectId);
    if (object == nullptr) {
      reject(receipt, CreativeGroupCommandStatus::MissingObject,
             "creative_group_object_missing", objectId);
      return receipt;
    }
    const CreativeObjectHierarchyState hierarchyState =
        resolveCreativeObjectHierarchyState(document, objectId);
    if (!hierarchyState.resolved || hierarchyState.effectivelyLocked) {
      reject(receipt, CreativeGroupCommandStatus::LockedObject,
             "creative_group_object_locked",
             hierarchyState.lockedByObjectId != kInvalidObjectId
                 ? hierarchyState.lockedByObjectId
                 : objectId);
      return receipt;
    }
    if (!describeObject(object->kind).canHaveParent) {
      reject(receipt, CreativeGroupCommandStatus::UnsupportedObject,
             "creative_group_parent_unsupported", objectId);
      return receipt;
    }
    if (!parentInitialized) {
      commonParent = object->parentId;
      parentInitialized = true;
    } else if (commonParent != object->parentId) {
      reject(receipt, CreativeGroupCommandStatus::MixedParents,
             "creative_group_mixed_parents", objectId);
      return receipt;
    }
  }
  for (CreativeObjectId objectId : hierarchy.objectIds) {
    const CreativeObjectHierarchyState hierarchyState =
        resolveCreativeObjectHierarchyState(document, objectId);
    if (!hierarchyState.resolved ||
        hierarchyState.depth >= kCreativeHierarchyDepthCapacity) {
      reject(receipt, CreativeGroupCommandStatus::DepthExceeded,
             "creative_group_depth_exceeded", objectId);
      return receipt;
    }
  }
  if (!hierarchyPivot(document, hierarchy, receipt.pivot)) {
    reject(receipt, CreativeGroupCommandStatus::InvalidHierarchy,
           "creative_group_pivot_invalid");
    return receipt;
  }

  CreativeDocument staged = document;
  CreativeDocumentCreateRequest create;
  create.kind = CreativeObjectKind::Group;
  create.name = "Group";
  create.transform.position = receipt.pivot;
  create.hasTransformOverride = true;
  create.parentId = commonParent;
  receipt.createReceipt = staged.createObject(create);
  if (!receipt.createReceipt.accepted ||
      !receipt.createReceipt.objectCreated) {
    reject(receipt, CreativeGroupCommandStatus::CreateRejected,
           receipt.createReceipt.reasonCode);
    return receipt;
  }
  receipt.groupObjectId = receipt.createReceipt.objectId;

  std::vector<CreativeMutationRequest> mutations;
  mutations.reserve(hierarchy.rootObjectIds.size());
  for (CreativeObjectId objectId : hierarchy.rootObjectIds) {
    mutations.push_back({0U, objectId, CreativeMutationKind::SetParent,
                         makeParentPayload(receipt.groupObjectId)});
  }
  receipt.mutationReceipt =
      applyDocumentMutationsAtomically(staged, mutations);
  if (!receipt.mutationReceipt.committed ||
      !documentMutationSucceeded(receipt.mutationReceipt.status)) {
    reject(receipt, CreativeGroupCommandStatus::MutationRejected,
           "creative_group_parenting_rejected");
    normalizeGroupNestedRevisionRange(receipt, receipt.revisionBefore);
    clearUnpublishedGroupOutputs(receipt);
    return receipt;
  }

  const CreativeDocumentPublicationReceipt publication =
      document.commitStagedMutation(std::move(staged));
  normalizeGroupNestedRevisionRange(
      receipt, publication.accepted ? publication.revisionAfter
                                    : receipt.revisionBefore);
  if (!publication.accepted) {
    reject(receipt, CreativeGroupCommandStatus::MutationRejected,
           publication.reasonCode);
    clearUnpublishedGroupOutputs(receipt);
    return receipt;
  }
  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = CreativeGroupCommandStatus::Applied;
  receipt.affectedObjectCount = hierarchy.objectIds.size();
  receipt.revisionAfter = publication.revisionAfter;
  receipt.selectionObjectIds.push_back(receipt.groupObjectId);
  receipt.reasonCode = "creative_group_applied";
  return receipt;
}

CreativeGroupCommandReceipt ungroupDocumentObjectAtomically(
    CreativeDocument& document,
    CreativeObjectId groupObjectId) {
  CreativeGroupCommandReceipt receipt = commandReceipt(
      document, CreativeGroupCommandKind::Ungroup, 1U);
  receipt.groupObjectId = groupObjectId;
  const CreativeObject* group = document.findObject(groupObjectId);
  if (group == nullptr) {
    reject(receipt, CreativeGroupCommandStatus::MissingObject,
           "creative_ungroup_object_missing", groupObjectId);
    return receipt;
  }
  if (!creativeObjectIsHierarchyContainer(group->kind)) {
    reject(receipt, CreativeGroupCommandStatus::NotGroup,
           "creative_ungroup_requires_group", groupObjectId);
    return receipt;
  }
  const CreativeObjectHierarchyState hierarchyState =
      resolveCreativeObjectHierarchyState(document, groupObjectId);
  if (!hierarchyState.resolved || hierarchyState.effectivelyLocked) {
    reject(receipt, CreativeGroupCommandStatus::LockedObject,
           "creative_ungroup_group_locked",
           hierarchyState.lockedByObjectId != kInvalidObjectId
               ? hierarchyState.lockedByObjectId
               : groupObjectId);
    return receipt;
  }

  const std::optional<CreativeObjectId> nextParent = group->parentId;
  for (const CreativeObject& object : document.objects()) {
    if (object.parentId == groupObjectId) {
      receipt.selectionObjectIds.push_back(object.id);
    }
  }
  CreativeDocument staged = document;
  std::vector<CreativeMutationRequest> mutations;
  mutations.reserve(receipt.selectionObjectIds.size());
  for (CreativeObjectId objectId : receipt.selectionObjectIds) {
    mutations.push_back(
        nextParent.has_value()
            ? CreativeMutationRequest{0U, objectId,
                                      CreativeMutationKind::SetParent,
                                      makeParentPayload(*nextParent)}
            : CreativeMutationRequest{0U, objectId,
                                      CreativeMutationKind::ClearParent, {}});
  }
  if (!mutations.empty()) {
    receipt.mutationReceipt =
        applyDocumentMutationsAtomically(staged, mutations);
    if (!receipt.mutationReceipt.committed ||
        !documentMutationSucceeded(receipt.mutationReceipt.status)) {
      reject(receipt, CreativeGroupCommandStatus::MutationRejected,
             "creative_ungroup_parenting_rejected");
      normalizeGroupNestedRevisionRange(receipt, receipt.revisionBefore);
      clearUnpublishedGroupOutputs(receipt);
      return receipt;
    }
  }
  receipt.removeReceipt = staged.removeDocumentObject(groupObjectId);
  if (!receipt.removeReceipt.accepted ||
      !receipt.removeReceipt.objectRemoved) {
    reject(receipt, CreativeGroupCommandStatus::RemoveRejected,
           receipt.removeReceipt.reasonCode, groupObjectId);
    normalizeGroupNestedRevisionRange(receipt, receipt.revisionBefore);
    clearUnpublishedGroupOutputs(receipt);
    return receipt;
  }

  const CreativeDocumentPublicationReceipt publication =
      document.commitStagedMutation(std::move(staged));
  normalizeGroupNestedRevisionRange(
      receipt, publication.accepted ? publication.revisionAfter
                                    : receipt.revisionBefore);
  if (!publication.accepted) {
    reject(receipt, CreativeGroupCommandStatus::MutationRejected,
           publication.reasonCode);
    clearUnpublishedGroupOutputs(receipt);
    return receipt;
  }
  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = CreativeGroupCommandStatus::Applied;
  receipt.affectedObjectCount = receipt.selectionObjectIds.size();
  receipt.revisionAfter = publication.revisionAfter;
  receipt.reasonCode = "creative_ungroup_applied";
  return receipt;
}

CreativeGroupPivotReceipt setCreativeGroupPivot(
    CreativeDocument& document,
    CreativeObjectId groupObjectId,
    CreativeVec3 pivot) {
  CreativeGroupPivotReceipt receipt;
  receipt.requested = true;
  receipt.groupObjectId = groupObjectId;
  receipt.pivotAfter = pivot;
  receipt.revisionBefore = document.revision();
  receipt.revisionAfter = receipt.revisionBefore;
  if (!document.isValid()) {
    receipt.status = CreativeGroupPivotStatus::InvalidDocument;
    receipt.reasonCode = "creative_group_pivot_document_invalid";
    return receipt;
  }
  const CreativeObject* group = document.findObject(groupObjectId);
  if (group == nullptr) {
    receipt.status = CreativeGroupPivotStatus::MissingGroup;
    receipt.reasonCode = "creative_group_pivot_group_missing";
    return receipt;
  }
  receipt.pivotBefore = group->transform.position;
  if (group->kind != CreativeObjectKind::Group) {
    receipt.status = CreativeGroupPivotStatus::NotGroup;
    receipt.reasonCode = "creative_group_pivot_requires_group";
    return receipt;
  }
  const CreativeObjectHierarchyState hierarchyState =
      resolveCreativeObjectHierarchyState(document, groupObjectId);
  if (!hierarchyState.resolved || hierarchyState.effectivelyLocked) {
    receipt.status = CreativeGroupPivotStatus::LockedGroup;
    receipt.reasonCode = "creative_group_pivot_group_locked";
    return receipt;
  }
  if (!isFiniteCreativeVec3(pivot)) {
    receipt.status = CreativeGroupPivotStatus::InvalidPivot;
    receipt.reasonCode = "creative_group_pivot_invalid";
    return receipt;
  }
  receipt.mutationReceipt = moveDocumentObject(document, groupObjectId, pivot);
  receipt.revisionAfter = document.revision();
  if (!documentMutationSucceeded(receipt.mutationReceipt.status)) {
    receipt.status = CreativeGroupPivotStatus::Rejected;
    receipt.reasonCode = "creative_group_pivot_rejected";
    return receipt;
  }
  receipt.accepted = true;
  receipt.changed = receipt.mutationReceipt.changed;
  receipt.status = receipt.changed ? CreativeGroupPivotStatus::Applied
                                   : CreativeGroupPivotStatus::NoChange;
  receipt.reasonCode = receipt.changed ? "creative_group_pivot_applied"
                                       : "creative_group_pivot_no_change";
  return receipt;
}

CreativeHierarchyRemoveReceipt removeCreativeObjectHierarchyAtomically(
    CreativeDocument& document,
    CreativeObjectId rootObjectId) {
  CreativeHierarchyRemoveReceipt receipt;
  receipt.requested = true;
  receipt.rootObjectId = rootObjectId;
  receipt.revisionBefore = document.revision();
  receipt.revisionAfter = receipt.revisionBefore;
  const CreativeHierarchySelection hierarchy =
      resolveCreativeObjectHierarchy(document,
                                     std::span{&rootObjectId, 1U});
  if (!hierarchy.accepted) {
    receipt.failedObjectId = hierarchy.missingObjectId;
    receipt.reasonCode = hierarchy.reasonCode;
    return receipt;
  }

  std::vector<CreativeObjectId> removalOrder = hierarchy.objectIds;
  std::stable_sort(
      removalOrder.begin(), removalOrder.end(),
      [&document, rootObjectId](CreativeObjectId lhs, CreativeObjectId rhs) {
        return objectDepthFromRoot(document, rootObjectId, lhs) >
               objectDepthFromRoot(document, rootObjectId, rhs);
      });
  CreativeDocument staged = document;
  for (CreativeObjectId objectId : removalOrder) {
    CreativeDocumentRemoveReceipt item =
        staged.removeDocumentObject(objectId);
    if (!item.accepted || !item.objectRemoved) {
      receipt.failedObjectId = objectId;
      receipt.reasonCode = item.reasonCode;
      normalizeRemoveReceiptRevisionRange(
          receipt.rootReceipt, receipt.revisionBefore, receipt.revisionBefore);
      receipt.removedObjectIds.clear();
      return receipt;
    }
    if (objectId == rootObjectId) {
      receipt.rootReceipt = item;
    }
    receipt.removedObjectIds.push_back(objectId);
  }

  const CreativeDocumentPublicationReceipt publication =
      document.commitStagedMutation(std::move(staged));
  normalizeRemoveReceiptRevisionRange(
      receipt.rootReceipt, receipt.revisionBefore,
      publication.accepted ? publication.revisionAfter
                           : receipt.revisionBefore);
  if (!publication.accepted) {
    receipt.reasonCode = publication.reasonCode;
    receipt.removedObjectIds.clear();
    return receipt;
  }
  receipt.accepted = true;
  receipt.changed = true;
  receipt.revisionAfter = publication.revisionAfter;
  receipt.rootReceipt.reasonCode = "object_hierarchy_removed";
  receipt.rootReceipt.message = "object_hierarchy_removed";
  receipt.reasonCode = "creative_hierarchy_removed";
  return receipt;
}

CreativeHierarchyBatchRemoveReceipt
removeCreativeObjectHierarchiesAtomically(
    CreativeDocument& document,
    std::span<const CreativeObjectId> rootObjectIds) {
  CreativeHierarchyBatchRemoveReceipt receipt;
  receipt.requested = true;
  receipt.requestedObjectCount = rootObjectIds.size();
  receipt.revisionBefore = document.revision();
  receipt.revisionAfter = receipt.revisionBefore;

  const CreativeHierarchySelection hierarchy =
      resolveCreativeObjectHierarchy(document, rootObjectIds);
  if (!hierarchy.accepted) {
    receipt.failedObjectId = hierarchy.missingObjectId;
    receipt.reasonCode = hierarchy.reasonCode;
    return receipt;
  }

  receipt.rootObjectIds = hierarchy.rootObjectIds;
  std::vector<CreativeObjectId> removalOrder = hierarchy.objectIds;
  std::stable_sort(
      removalOrder.begin(), removalOrder.end(),
      [&document](CreativeObjectId lhs, CreativeObjectId rhs) {
        return objectHierarchyDepth(document, lhs) >
               objectHierarchyDepth(document, rhs);
      });

  CreativeDocument staged = document;
  receipt.removeReceipts.reserve(removalOrder.size());
  for (CreativeObjectId objectId : removalOrder) {
    CreativeDocumentRemoveReceipt item = staged.removeDocumentObject(objectId);
    receipt.removeReceipts.push_back(item);
    if (!item.accepted || !item.objectRemoved || !item.changed) {
      receipt.failedObjectId = objectId;
      receipt.reasonCode = item.reasonCode;
      receipt.removedObjectIds.clear();
      for (CreativeDocumentRemoveReceipt& stagedReceipt :
           receipt.removeReceipts) {
        normalizeRemoveReceiptRevisionRange(
            stagedReceipt, receipt.revisionBefore, receipt.revisionBefore);
      }
      return receipt;
    }
    receipt.removedObjectIds.push_back(objectId);
  }

  const CreativeDocumentPublicationReceipt publication =
      document.commitStagedMutation(std::move(staged));
  const std::uint64_t nestedRevisionAfter =
      publication.accepted ? publication.revisionAfter
                           : receipt.revisionBefore;
  for (CreativeDocumentRemoveReceipt& stagedReceipt :
       receipt.removeReceipts) {
    normalizeRemoveReceiptRevisionRange(
        stagedReceipt, receipt.revisionBefore, nestedRevisionAfter);
  }
  if (!publication.accepted) {
    receipt.reasonCode = publication.reasonCode;
    receipt.removedObjectIds.clear();
    return receipt;
  }
  receipt.accepted = true;
  receipt.changed = true;
  receipt.revisionAfter = publication.revisionAfter;
  receipt.reasonCode = "creative_hierarchy_batch_removed";
  return receipt;
}

}  // namespace iggy3d::creative
