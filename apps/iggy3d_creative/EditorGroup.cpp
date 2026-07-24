#include "EditorGroup.hpp"

#include <SDL3/SDL_log.h>

#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "EditorEdits.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/assets/AuthoredAsset.hpp"
#include "app/iggy3d/creative/tools/Select.hpp"
#include "app/iggy3d/creative/tools/SelectionResolution.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

void selectOnly(cr::Facade& facade, cr::CreativeObjectId objectId) {
  static_cast<void>(facade.setActiveTool(cr::Tool::Select));
  cr::CreativeToolInputPacket input;
  input.kind = cr::CreativeToolInputKind::PointerPress;
  input.pointer.button = cr::CreativeToolPointerButton::Primary;
  if (objectId != cr::kInvalidObjectId) {
    input.pointer.target.value = static_cast<cr::Id>(objectId);
  }
  static_cast<void>(facade.dispatchToolInput(input));
}

[[nodiscard]] CreativeEditorGroupFocusReceipt focusReceipt(
    const CreativeEditorGroupFocusState& state) noexcept {
  CreativeEditorGroupFocusReceipt receipt;
  receipt.requested = true;
  receipt.depthBefore = state.depth;
  receipt.depthAfter = state.depth;
  return receipt;
}

[[nodiscard]] std::string_view structuralMutationRejectionReason(
    const cr::CreativeDocument& document,
    cr::CreativeObjectId objectId) noexcept {
  const cr::CreativeSemanticSelectionResolution selection =
      cr::resolveCreativeSemanticSelection(document, objectId);
  if (!selection.accepted) {
    return {};
  }
  const cr::CreativeSemanticObjectActionPolicy policy =
      cr::resolveCreativeSemanticObjectAction(
          selection, cr::CreativeSemanticObjectAction::StructuralMutation);
  return cr::creativeSemanticActionUsesDocumentMutation(policy)
             ? std::string_view{}
             : policy.reasonCode;
}

[[nodiscard]] std::vector<cr::CreativeObjectId> selectedObjectIds(
    const cr::CreativeSelectionState& selection) {
  std::vector<cr::CreativeObjectId> objectIds;
  const std::span<const cr::TargetRef> targets =
      cr::selectedTargetList(selection);
  objectIds.reserve(targets.empty() ? 1U : targets.size());
  for (const cr::TargetRef target : targets) {
    if (target.value != cr::kInvalidId) {
      objectIds.push_back(static_cast<cr::CreativeObjectId>(target.value));
    }
  }
  if (objectIds.empty() &&
      selection.selectedTarget.value != cr::kInvalidId) {
    objectIds.push_back(static_cast<cr::CreativeObjectId>(
        selection.selectedTarget.value));
  }
  return objectIds;
}

[[nodiscard]] std::string_view selectionStructuralMutationRejectionReason(
    const cr::CreativeDocument& document,
    std::span<const cr::CreativeObjectId> objectIds,
    cr::CreativeObjectId primaryObjectId) noexcept {
  if (objectIds.empty()) {
    return {};
  }
  const cr::CreativeSemanticSelectionSetResolution selection =
      cr::resolveCreativeSemanticSelectionSet(document, objectIds,
                                              primaryObjectId);
  if (!selection.accepted) {
    return {};
  }
  const cr::CreativeSemanticObjectActionPolicy policy =
      cr::resolveCreativeSemanticObjectAction(
          selection, cr::CreativeSemanticObjectAction::StructuralMutation);
  return cr::creativeSemanticActionUsesDocumentMutation(policy)
             ? std::string_view{}
             : policy.reasonCode;
}

}  // namespace

bool creativeEditorGroupFocusActive(
    const CreativeEditorGroupFocusState& state) noexcept {
  return state.depth > 0U &&
         state.depth <= state.groupIds.size() &&
         state.documentId != cr::kInvalidDocumentId &&
         state.groupIds[state.depth - 1U] != cr::kInvalidObjectId;
}

cr::CreativeObjectId activeCreativeEditorGroupFocusId(
    const CreativeEditorGroupFocusState& state) noexcept {
  return creativeEditorGroupFocusActive(state)
             ? state.groupIds[state.depth - 1U]
             : cr::kInvalidObjectId;
}

bool syncCreativeEditorGroupFocus(
    CreativeEditorGroupFocusState& state,
    const cr::CreativeDocument& document) noexcept {
  if (!document.isValid() || document.id() == cr::kInvalidDocumentId ||
      state.documentId != document.id() ||
      state.depth > state.groupIds.size()) {
    const bool changed = state.depth != 0U ||
                         state.documentId != document.id();
    state = {};
    state.documentId = document.isValid() ? document.id()
                                          : cr::kInvalidDocumentId;
    return changed;
  }

  std::size_t validDepth = 0U;
  cr::CreativeObjectId expectedParent = cr::kInvalidObjectId;
  for (; validDepth < state.depth; ++validDepth) {
    const cr::CreativeObject* group =
        document.findObject(state.groupIds[validDepth]);
    if (group == nullptr ||
        !cr::creativeObjectIsHierarchyContainer(group->kind) ||
        (validDepth > 0U && group->parentId != expectedParent)) {
      break;
    }
    expectedParent = group->id;
  }
  if (validDepth == state.depth) {
    return false;
  }
  std::fill(state.groupIds.begin() + validDepth, state.groupIds.end(),
            cr::kInvalidObjectId);
  state.depth = static_cast<std::uint8_t>(validDepth);
  return true;
}

bool creativeEditorObjectInsideActiveGroup(
    const cr::CreativeDocument& document,
    const CreativeEditorGroupFocusState& state,
    cr::CreativeObjectId objectId) noexcept {
  const cr::CreativeObjectId activeGroup =
      activeCreativeEditorGroupFocusId(state);
  if (activeGroup == cr::kInvalidObjectId ||
      objectId == cr::kInvalidObjectId || objectId == activeGroup) {
    return false;
  }
  const cr::CreativeObject* object = document.findObject(objectId);
  std::size_t hops = 0U;
  while (object != nullptr && object->parentId.has_value() &&
         hops++ < document.objectCount()) {
    if (*object->parentId == activeGroup) {
      return true;
    }
    object = document.findObject(*object->parentId);
  }
  return false;
}

cr::CreativeObjectId resolveCreativeEditorGroupSelectionTarget(
    const cr::CreativeDocument& document,
    const CreativeEditorGroupFocusState& state,
    cr::CreativeObjectId hitObjectId) noexcept {
  const cr::CreativeObject* hit = document.findObject(hitObjectId);
  if (hit == nullptr) {
    return cr::kInvalidObjectId;
  }
  const cr::CreativeObjectId activeGroup =
      activeCreativeEditorGroupFocusId(state);
  if (activeGroup == cr::kInvalidObjectId) {
    cr::CreativeObjectId target = hit->id;
    const cr::CreativeObject* current = hit;
    std::size_t hops = 0U;
    while (current->parentId.has_value() &&
           hops++ < document.objectCount()) {
      current = document.findObject(*current->parentId);
      if (current == nullptr) {
        return cr::kInvalidObjectId;
      }
      if (cr::creativeObjectIsHierarchyContainer(current->kind)) {
        target = current->id;
      }
    }
    return target;
  }
  if (!creativeEditorObjectInsideActiveGroup(document, state, hitObjectId)) {
    return cr::kInvalidObjectId;
  }
  const cr::CreativeObject* current = hit;
  std::size_t hops = 0U;
  while (current->parentId != activeGroup &&
         current->parentId.has_value() &&
         hops++ < document.objectCount()) {
    current = document.findObject(*current->parentId);
    if (current == nullptr) {
      return cr::kInvalidObjectId;
    }
  }
  return current->parentId == activeGroup ? current->id
                                          : cr::kInvalidObjectId;
}

CreativeEditorGroupFocusReceipt enterCreativeEditorGroupFocus(
    cr::CreativeAppState& appState,
    CreativeEditorGroupFocusState& state,
    cr::CreativeObjectId groupObjectId) {
  static_cast<void>(syncCreativeEditorGroupFocus(
      state, appState.facade.document()));
  CreativeEditorGroupFocusReceipt receipt = focusReceipt(state);
  receipt.groupObjectId = groupObjectId;
  if (!appState.facade.document().isValid() ||
      appState.facade.document().id() == cr::kInvalidDocumentId) {
    receipt.status = CreativeEditorGroupFocusStatus::InvalidDocument;
    receipt.reasonCode = "creative_group_focus_document_invalid";
    return receipt;
  }
  const cr::CreativeObject* group = appState.facade.findObject(groupObjectId);
  if (group == nullptr) {
    receipt.status = CreativeEditorGroupFocusStatus::MissingGroup;
    receipt.reasonCode = "creative_group_focus_missing";
    return receipt;
  }
  if (!cr::creativeObjectIsHierarchyContainer(group->kind)) {
    receipt.status = CreativeEditorGroupFocusStatus::NotGroup;
    receipt.reasonCode = "creative_group_focus_requires_group";
    return receipt;
  }
  if (creativeEditorGroupFocusActive(state) &&
      resolveCreativeEditorGroupSelectionTarget(
          appState.facade.document(), state, groupObjectId) != groupObjectId) {
    receipt.status = CreativeEditorGroupFocusStatus::OutsideActiveGroup;
    receipt.reasonCode = "creative_group_focus_outside_active_group";
    return receipt;
  }
  if (state.depth >= state.groupIds.size()) {
    receipt.status = CreativeEditorGroupFocusStatus::DepthExceeded;
    receipt.reasonCode = "creative_group_focus_depth_exceeded";
    return receipt;
  }
  state.documentId = appState.facade.document().id();
  state.groupIds[state.depth++] = groupObjectId;
  selectOnly(appState.facade, cr::kInvalidObjectId);
  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = CreativeEditorGroupFocusStatus::Entered;
  receipt.depthAfter = state.depth;
  receipt.reasonCode = "creative_group_focus_entered";
  return receipt;
}

CreativeEditorGroupFocusReceipt exitCreativeEditorGroupFocus(
    cr::CreativeAppState& appState,
    CreativeEditorGroupFocusState& state) {
  static_cast<void>(syncCreativeEditorGroupFocus(
      state, appState.facade.document()));
  CreativeEditorGroupFocusReceipt receipt = focusReceipt(state);
  if (!creativeEditorGroupFocusActive(state)) {
    receipt.status = CreativeEditorGroupFocusStatus::NoFocus;
    receipt.reasonCode = "creative_group_focus_not_active";
    return receipt;
  }
  receipt.groupObjectId = state.groupIds[state.depth - 1U];
  state.groupIds[state.depth - 1U] = cr::kInvalidObjectId;
  --state.depth;
  selectOnly(appState.facade, receipt.groupObjectId);
  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = CreativeEditorGroupFocusStatus::Exited;
  receipt.depthAfter = state.depth;
  receipt.reasonCode = "creative_group_focus_exited";
  return receipt;
}

cr::CreativeGroupCommandReceipt applyCreativeEditorGroupCommandWithHistory(
    cr::CreativeAppState& appState,
    std::string_view source) {
  const cr::CreativeSelectionState& selection =
      appState.facade.selectionState();
  const cr::CreativeObject* primary =
      selection.selectedTarget.value == cr::kInvalidId
          ? nullptr
          : appState.facade.findObject(
                static_cast<cr::CreativeObjectId>(
                    selection.selectedTarget.value));
  cr::CreativeObjectId ungroupObjectId = cr::kInvalidObjectId;
  if (primary != nullptr && cr::selectedTargetCount(selection) == 1U) {
    if (cr::creativeObjectIsHierarchyContainer(primary->kind)) {
      ungroupObjectId = primary->id;
    } else if (primary->parentId.has_value()) {
      const cr::CreativeObject* parent =
          appState.facade.findObject(*primary->parentId);
      if (parent != nullptr &&
          cr::creativeObjectIsHierarchyContainer(parent->kind)) {
        ungroupObjectId = parent->id;
      }
    }
  }
  const std::vector<cr::CreativeObjectId> selectedIds =
      selectedObjectIds(selection);
  std::vector<cr::CreativeObjectId> semanticMutationIds = selectedIds;
  if (ungroupObjectId != cr::kInvalidObjectId) {
    semanticMutationIds.clear();
    semanticMutationIds.push_back(ungroupObjectId);
    for (const cr::CreativeObject& object :
         appState.facade.document().objects()) {
      if (object.parentId == ungroupObjectId) {
        semanticMutationIds.push_back(object.id);
      }
    }
  }
  const std::string_view semanticRejection =
      selectionStructuralMutationRejectionReason(
          appState.facade.document(), semanticMutationIds,
          ungroupObjectId != cr::kInvalidObjectId
              ? ungroupObjectId
              : primary != nullptr ? primary->id : cr::kInvalidObjectId);
  if (!semanticRejection.empty()) {
    cr::CreativeGroupCommandReceipt rejected;
    rejected.requested = true;
    rejected.kind = ungroupObjectId != cr::kInvalidObjectId
                        ? cr::CreativeGroupCommandKind::Ungroup
                        : cr::CreativeGroupCommandKind::Group;
    rejected.status = cr::CreativeGroupCommandStatus::MutationRejected;
    rejected.requestedObjectCount =
        ungroupObjectId != cr::kInvalidObjectId ? 1U : selectedIds.size();
    rejected.groupObjectId = ungroupObjectId;
    rejected.failedObjectId =
        ungroupObjectId != cr::kInvalidObjectId
            ? ungroupObjectId
            : primary != nullptr ? primary->id : cr::kInvalidObjectId;
    rejected.revisionBefore = appState.facade.document().revision();
    rejected.revisionAfter = rejected.revisionBefore;
    rejected.selectionObjectIds = selectedIds;
    rejected.reasonCode = semanticRejection;
    return rejected;
  }
  std::optional<cr::CreativeAuthoringOperationRecord> prefabOperation;
  const cr::CreativeObject* ungroupObject =
      appState.facade.findObject(ungroupObjectId);
  if (ungroupObject != nullptr &&
      ungroupObject->kind == cr::CreativeObjectKind::PrefabInstance) {
    const cr::CreativeAuthoredAssetFingerprint fingerprint =
        cr::fingerprintCreativeAuthoredAssetInstance(*ungroupObject);
    const cr::CreativeHierarchySelection hierarchy =
        cr::resolveCreativeObjectHierarchy(
            appState.facade.document(),
            std::span{&ungroupObjectId, 1U});
    prefabOperation =
        fingerprint.valid && hierarchy.accepted
            ? cr::makeCreativeAuthoringOperationRecord(
                  cr::CreativeAuthoringFamily::Prefab,
                  cr::CreativeAuthoringOperationKind::Destructive,
                  "Prefab.Detach",
                  fingerprint.value, hierarchy.objectIds.size())
            : std::nullopt;
    if (!prefabOperation.has_value()) {
      cr::CreativeGroupCommandReceipt rejected;
      rejected.requested = true;
      rejected.kind = cr::CreativeGroupCommandKind::Ungroup;
      rejected.status = cr::CreativeGroupCommandStatus::UnsupportedObject;
      rejected.groupObjectId = ungroupObjectId;
      rejected.reasonCode = "creative_prefab_detach_operation_invalid";
      return rejected;
    }
  }
  cr::CreativeDocumentHistoryTransaction transaction =
      prefabOperation.has_value()
          ? cr::beginCreativeHistoryTransaction(appState.facade, source,
                                                *prefabOperation)
          : beginEditTransaction(appState.facade, source);
  cr::CreativeGroupCommandReceipt receipt =
      ungroupObjectId != cr::kInvalidObjectId
          ? appState.facade.ungroupObject(ungroupObjectId)
          : appState.facade.groupSelectedObjects();
  static_cast<void>(completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      receipt.accepted && receipt.changed, receipt.reasonCode));
  SDL_Log("iggy3d_creative: %s accepted=%d changed=%d status='%s' "
          "requested=%llu affected=%llu groupId=%llu revision=%llu "
          "reasonCode='%s'",
          std::string(cr::toString(receipt.kind)).c_str(),
          receipt.accepted ? 1 : 0, receipt.changed ? 1 : 0,
          std::string(cr::toString(receipt.status)).c_str(),
          static_cast<unsigned long long>(receipt.requestedObjectCount),
          static_cast<unsigned long long>(receipt.affectedObjectCount),
          static_cast<unsigned long long>(receipt.groupObjectId),
          static_cast<unsigned long long>(receipt.revisionAfter),
          std::string(receipt.reasonCode).c_str());
  return receipt;
}

cr::CreativeGroupPivotReceipt setCreativeEditorGroupPivotWithHistory(
    cr::CreativeAppState& appState,
    cr::CreativeObjectId groupObjectId,
    cr::CreativeVec3 pivot,
    std::string_view source) {
  const std::string_view semanticRejection =
      structuralMutationRejectionReason(appState.facade.document(),
                                        groupObjectId);
  if (!semanticRejection.empty()) {
    cr::CreativeGroupPivotReceipt rejected;
    rejected.requested = true;
    rejected.status = cr::CreativeGroupPivotStatus::Rejected;
    rejected.groupObjectId = groupObjectId;
    rejected.revisionBefore = appState.facade.document().revision();
    rejected.revisionAfter = rejected.revisionBefore;
    rejected.reasonCode = semanticRejection;
    return rejected;
  }
  cr::CreativeDocumentHistoryTransaction transaction =
      beginEditTransaction(appState.facade, source);
  cr::CreativeGroupPivotReceipt receipt =
      appState.facade.setGroupPivot(groupObjectId, pivot);
  static_cast<void>(completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      receipt.accepted && receipt.changed, receipt.reasonCode));
  return receipt;
}

}  // namespace iggy3d_creative_app
