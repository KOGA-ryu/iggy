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
#include "app/iggy3d/creative/tools/Group.hpp"
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
                       return isValidCreativePathPoint(point);
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
  create.assetId = object.assetId;
  create.assetContentHash = object.assetContentHash;
  create.assetMaterialVariant = object.assetMaterialVariant;
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
      create.attachmentSocket = object.attachmentSocket;
    } else if (request.externalParentPolicy ==
                   CreativeClipboardExternalParentPolicy::PreserveIfPresent &&
               targetDocument.containsObject(*object.parentId)) {
      create.parentId = object.parentId;
      create.attachmentSocket = object.attachmentSocket;
    }
  }
  create.pathPoints = object.pathPoints;
  create.hasPathOverride = !object.pathPoints.empty();
  if (object.kind == CreativeObjectKind::MovingPlatform) {
    create.movingPlatform = object.movingPlatform;
    create.hasMovingPlatformSettingsOverride = true;
  }
  if (object.kind == CreativeObjectKind::Door) {
    create.door = object.door;
    create.hasDoorSettingsOverride = true;
  }
  if (object.kind == CreativeObjectKind::Window) {
    create.window = object.window;
    create.hasWindowSettingsOverride = true;
  }
  if (object.kind == CreativeObjectKind::SpawnPoint) {
    create.playerSpawn = object.playerSpawn;
    create.hasPlayerSpawnSettingsOverride = true;
  }
  if (object.kind == CreativeObjectKind::NpcSpawn ||
      object.kind == CreativeObjectKind::EnemySpawn) {
    create.npcSpawn = object.npcSpawn;
    create.hasNpcSpawnSettingsOverride = true;
  }
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
                       return isValidCreativePathPoint(point);
                     });
}

[[nodiscard]] bool clipboardPatternRecipesValid(
    const CreativeClipboard& clipboard) {
  CreativePatternRecipeStore store;
  store.recipes = clipboard.patternRecipes;
  CreativePatternRecipeId maximumId = kInvalidCreativePatternRecipeId;
  for (const CreativePatternRecipe& recipe : store.recipes) {
    maximumId = std::max(maximumId, recipe.id);
  }
  if (!store.recipes.empty()) {
    if (maximumId == std::numeric_limits<CreativePatternRecipeId>::max()) {
      return false;
    }
    store.nextRecipeId = maximumId + 1U;
  }
  return validateCreativePatternRecipeReferences(store, clipboard.objects);
}

[[nodiscard]] bool patternPasteTransformSupported(
    const CreativeClipboardPasteRequest& request) noexcept {
  return creativeVec3ExactlyEqual(request.scaleFactor, {1.0, 1.0, 1.0}) &&
         request.quarterTurns == 0U && !request.mirrorX && !request.mirrorZ &&
         !request.hasAxisAngleRotation;
}

void clearPublishedPasteOutputs(
    CreativeClipboardBatchPasteReceipt& receipt) noexcept {
  receipt.pastedPasteCount = 0U;
  receipt.pastedObjectCount = 0U;
  receipt.pastedLogicLinkCount = 0U;
  receipt.pastedPatternRecipeCount = 0U;
  receipt.idRemaps.clear();
  receipt.patternRecipeIdRemaps.clear();
  receipt.pastedObjectIds.clear();
}

[[nodiscard]] bool remapPatternRecipe(
    const CreativePatternRecipe& source,
    const std::unordered_map<CreativeObjectId, CreativeObjectId>& remaps,
    CreativeVec3 translation,
    CreativePatternRecipe& output) {
  output = source;
  output.id = kInvalidCreativePatternRecipeId;
  const auto remapIds = [&remaps](std::vector<CreativeObjectId>& objectIds) {
    for (CreativeObjectId& objectId : objectIds) {
      const auto remap = remaps.find(objectId);
      if (remap == remaps.end()) {
        return false;
      }
      objectId = remap->second;
    }
    return true;
  };
  if (!remapIds(output.sourceObjectIds) ||
      !remapIds(output.generatedObjectIds)) {
    return false;
  }
  if (output.kind == CreativePatternRecipeKind::RadialArray) {
    output.radial.pivot = add(output.radial.pivot, translation);
  } else if (output.kind == CreativePatternRecipeKind::AssetScatter) {
    for (CreativeVec3& center : output.scatter.paintCenters) {
      center = add(center, translation);
    }
    for (CreativeAssetScatterExclusion& exclusion :
         output.scatter.exclusions) {
      exclusion.center = add(exclusion.center, translation);
    }
  }
  return true;
}

[[nodiscard]] bool clipboardCutHasExternalReferences(
    const CreativeDocument& document,
    const CreativeClipboard& clipboard,
    CreativeObjectId& failedObjectId) {
  std::unordered_set<CreativeObjectId> copiedIds;
  copiedIds.reserve(clipboard.objects.size());
  for (const CreativeObject& object : clipboard.objects) {
    copiedIds.insert(object.id);
  }
  std::unordered_set<CreativePatternRecipeId> copiedRecipeIds;
  copiedRecipeIds.reserve(clipboard.patternRecipes.size());
  for (const CreativePatternRecipe& recipe : clipboard.patternRecipes) {
    copiedRecipeIds.insert(recipe.id);
  }
  for (const CreativeLogicLink& link : document.logicLinks()) {
    const bool sourceCopied = copiedIds.contains(link.sourceObjectId);
    const bool targetCopied = copiedIds.contains(link.targetObjectId);
    if (sourceCopied != targetCopied) {
      failedObjectId = sourceCopied ? link.sourceObjectId : link.targetObjectId;
      return true;
    }
  }
  for (const CreativePatternRecipe& recipe :
       document.patternRecipeStore().recipes) {
    if (copiedRecipeIds.contains(recipe.id)) {
      continue;
    }
    const auto copiedReference = [&copiedIds](CreativeObjectId objectId) {
      return copiedIds.contains(objectId);
    };
    const auto source = std::find_if(recipe.sourceObjectIds.begin(),
                                     recipe.sourceObjectIds.end(),
                                     copiedReference);
    if (source != recipe.sourceObjectIds.end()) {
      failedObjectId = *source;
      return true;
    }
    const auto generated = std::find_if(recipe.generatedObjectIds.begin(),
                                        recipe.generatedObjectIds.end(),
                                        copiedReference);
    if (generated != recipe.generatedObjectIds.end()) {
      failedObjectId = *generated;
      return true;
    }
  }
  return false;
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

std::string_view toString(CreativeDuplicateCommandStatus status) noexcept {
  switch (status) {
    case CreativeDuplicateCommandStatus::NotRequested: return "NotRequested";
    case CreativeDuplicateCommandStatus::EmptySelection:
      return "EmptySelection";
    case CreativeDuplicateCommandStatus::InvalidRequest:
      return "InvalidRequest";
    case CreativeDuplicateCommandStatus::MissingObject: return "MissingObject";
    case CreativeDuplicateCommandStatus::Applied: return "Applied";
    case CreativeDuplicateCommandStatus::Rejected: return "Rejected";
  }
  return "Unknown";
}

std::string_view toString(CreativeSemanticDeleteStatus status) noexcept {
  switch (status) {
    case CreativeSemanticDeleteStatus::NotRequested: return "NotRequested";
    case CreativeSemanticDeleteStatus::EmptySelection: return "EmptySelection";
    case CreativeSemanticDeleteStatus::InvalidDocument: return "InvalidDocument";
    case CreativeSemanticDeleteStatus::InvalidHierarchy:
      return "InvalidHierarchy";
    case CreativeSemanticDeleteStatus::MissingObject: return "MissingObject";
    case CreativeSemanticDeleteStatus::ExternalReference:
      return "ExternalReference";
    case CreativeSemanticDeleteStatus::RemoveRejected: return "RemoveRejected";
    case CreativeSemanticDeleteStatus::Deleted: return "Deleted";
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
    CreativeClipboard& outClipboard,
    CreativeClipboardCopyMode mode) {
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
  if (mode != CreativeClipboardCopyMode::SemanticClosure &&
      mode != CreativeClipboardCopyMode::ExactObjects) {
    receipt.status = CreativeClipboardStatus::InvalidRequest;
    receipt.reasonCode = "creative_clipboard_copy_mode_invalid";
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

  if (!validateCreativePatternRecipeReferences(document.patternRecipeStore(),
                                                document.objects())) {
    receipt.status = CreativeClipboardStatus::InvalidClipboard;
    receipt.reasonCode = "creative_clipboard_pattern_store_invalid";
    return receipt;
  }

  std::vector<bool> copiedRecipes(
      document.patternRecipeStore().recipes.size(), false);
  if (mode == CreativeClipboardCopyMode::SemanticClosure) {
    bool closureChanged = true;
    while (closureChanged) {
      closureChanged = false;
      for (std::size_t index = 0U;
           index < document.patternRecipeStore().recipes.size(); ++index) {
        if (copiedRecipes[index]) {
          continue;
        }
        const CreativePatternRecipe& recipe =
            document.patternRecipeStore().recipes[index];
        const bool generatedMemberSelected = std::any_of(
            recipe.generatedObjectIds.begin(), recipe.generatedObjectIds.end(),
            [&requested](CreativeObjectId objectId) {
              return requested.contains(objectId);
            });
        if (!generatedMemberSelected) {
          continue;
        }
        copiedRecipes[index] = true;
        for (CreativeObjectId objectId : recipe.sourceObjectIds) {
          closureChanged = requested.insert(objectId).second || closureChanged;
        }
        for (CreativeObjectId objectId : recipe.generatedObjectIds) {
          closureChanged = requested.insert(objectId).second || closureChanged;
        }
      }
    }
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
  staged.logicLinks.reserve(document.logicLinks().size());
  for (const CreativeLogicLink& link : document.logicLinks()) {
    if (requested.contains(link.sourceObjectId) &&
        requested.contains(link.targetObjectId)) {
      staged.logicLinks.push_back(link);
    }
  }
  for (std::size_t index = 0U; index < copiedRecipes.size(); ++index) {
    if (copiedRecipes[index]) {
      staged.patternRecipes.push_back(
          document.patternRecipeStore().recipes[index]);
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
  if (!clipboardPatternRecipesValid(staged)) {
    receipt.status = CreativeClipboardStatus::InvalidClipboard;
    receipt.reasonCode = "creative_clipboard_pattern_references_invalid";
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
  receipt.copiedLogicLinkCount = staged.logicLinks.size();
  receipt.copiedPatternRecipeCount = staged.patternRecipes.size();
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
  receipt.pastedLogicLinkCount = batch.pastedLogicLinkCount;
  receipt.pastedPatternRecipeCount = batch.pastedPatternRecipeCount;
  receipt.failedObjectId = batch.failedObjectId;
  receipt.revisionBefore = batch.revisionBefore;
  receipt.revisionAfter = batch.revisionAfter;
  receipt.idRemaps = std::move(batch.idRemaps);
  receipt.patternRecipeIdRemaps =
      std::move(batch.patternRecipeIdRemaps);
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
  if (clipboard.logicLinks.size() >
      std::numeric_limits<std::uint64_t>::max() / requests.size()) {
    receipt.status = CreativeClipboardStatus::InvalidRequest;
    receipt.reasonCode = "creative_clipboard_link_batch_size_overflow";
    return receipt;
  }
  receipt.requestedLogicLinkCount =
      static_cast<std::uint64_t>(clipboard.logicLinks.size()) *
      requests.size();
  if (clipboard.patternRecipes.size() >
      std::numeric_limits<std::uint64_t>::max() / requests.size()) {
    receipt.status = CreativeClipboardStatus::InvalidRequest;
    receipt.reasonCode = "creative_clipboard_pattern_batch_size_overflow";
    return receipt;
  }
  receipt.requestedPatternRecipeCount =
      static_cast<std::uint64_t>(clipboard.patternRecipes.size()) *
      requests.size();

  const CreativeLogicLinkValidationReceipt linkValidation =
      validateCreativeLogicLinks(clipboard.logicLinks, clipboard.objects);
  if (!linkValidation.valid) {
    receipt.status = CreativeClipboardStatus::InvalidClipboard;
    receipt.failedObjectId = linkValidation.sourceObjectId;
    receipt.reasonCode = std::string(linkValidation.reasonCode);
    return receipt;
  }
  if (!clipboardPatternRecipesValid(clipboard)) {
    receipt.status = CreativeClipboardStatus::InvalidClipboard;
    receipt.reasonCode = "creative_clipboard_pattern_references_invalid";
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
    if (!isPositiveCreativeVec3(request.scaleFactor)) {
      receipt.failedPasteIndex = requestIndex;
      receipt.status = CreativeClipboardStatus::InvalidRequest;
      receipt.reasonCode = "creative_clipboard_scale_invalid";
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
        (request.hasAxisAngleRotation && request.quarterTurns != 0U)) {
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
    if (!clipboard.patternRecipes.empty() &&
        !patternPasteTransformSupported(request)) {
      receipt.failedPasteIndex = requestIndex;
      receipt.status = CreativeClipboardStatus::InvalidRequest;
      receipt.reasonCode = "creative_clipboard_pattern_transform_unsupported";
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
  receipt.patternRecipeIdRemaps.reserve(
      static_cast<std::size_t>(receipt.requestedPatternRecipeCount));
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
    placementRequest.pivotMode = request.pivotMode;
    placementRequest.coordinateSpace = request.coordinateSpace;
    placementRequest.coordinateBasisEulerRadians =
        request.coordinateBasisEulerRadians;
    placementRequest.scaleFactor = request.scaleFactor;
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
      clearPublishedPasteOutputs(receipt);
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
        clearPublishedPasteOutputs(receipt);
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
        clearPublishedPasteOutputs(receipt);
        return receipt;
      }
      receipt.pastedObjectIds.push_back(createReceipt.objectId);
    }
    for (const CreativeLogicLink& link : clipboard.logicLinks) {
      const auto source = remaps.find(link.sourceObjectId);
      const auto target = remaps.find(link.targetObjectId);
      if (source == remaps.end() || target == remaps.end()) {
        receipt.failedPasteIndex = requestIndex;
        receipt.failedObjectId = link.sourceObjectId;
        receipt.status = CreativeClipboardStatus::InvalidClipboard;
        receipt.reasonCode = "creative_clipboard_link_endpoint_missing";
        clearPublishedPasteOutputs(receipt);
        return receipt;
      }
      const CreativeLogicLinkMutationReceipt linkReceipt = staged.setLogicLink(
          {source->second, target->second, link.action});
      if (!linkReceipt.accepted || !linkReceipt.changed) {
        receipt.failedPasteIndex = requestIndex;
        receipt.failedObjectId = link.sourceObjectId;
        receipt.status = CreativeClipboardStatus::CreateRejected;
        receipt.reasonCode = std::string(linkReceipt.reasonCode);
        clearPublishedPasteOutputs(receipt);
        return receipt;
      }
      ++receipt.pastedLogicLinkCount;
    }
    for (const CreativePatternRecipe& recipe : clipboard.patternRecipes) {
      CreativePatternRecipe remappedRecipe;
      if (!remapPatternRecipe(recipe, remaps, request.offset,
                              remappedRecipe)) {
        receipt.failedPasteIndex = requestIndex;
        receipt.status = CreativeClipboardStatus::InvalidClipboard;
        receipt.reasonCode = "creative_clipboard_pattern_endpoint_missing";
        clearPublishedPasteOutputs(receipt);
        return receipt;
      }
      CreativePatternRecipeMutationRequest mutation;
      mutation.kind = CreativePatternRecipeMutationKind::Add;
      mutation.recipe = std::move(remappedRecipe);
      const CreativePatternRecipeMutationReceipt recipeReceipt =
          staged.applyPatternRecipeMutation(mutation);
      if (!recipeReceipt.accepted || !recipeReceipt.changed) {
        receipt.failedPasteIndex = requestIndex;
        receipt.status = CreativeClipboardStatus::CreateRejected;
        receipt.reasonCode = std::string(recipeReceipt.reasonCode);
        clearPublishedPasteOutputs(receipt);
        return receipt;
      }
      receipt.patternRecipeIdRemaps.push_back(
          {recipe.id, recipeReceipt.recipeId});
      ++receipt.pastedPatternRecipeCount;
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

CreativeDuplicateCommandReceipt duplicateDocumentObjectsAtomically(
    CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds,
    const CreativeDuplicateCommandRequest& request) {
  CreativeDuplicateCommandReceipt receipt;
  receipt.requested = true;
  receipt.requestedObjectCount = objectIds.size();
  receipt.revisionBefore = document.revision();
  receipt.revisionAfter = receipt.revisionBefore;

  if (objectIds.empty()) {
    receipt.status = CreativeDuplicateCommandStatus::EmptySelection;
    receipt.message = "duplicate_selection_empty";
    return receipt;
  }
  if (!isFiniteCreativeVec3(request.offset)) {
    receipt.status = CreativeDuplicateCommandStatus::InvalidRequest;
    receipt.message = "duplicate_request_invalid";
    return receipt;
  }

  const CreativeHierarchySelection hierarchy =
      resolveCreativeObjectHierarchy(document, objectIds);
  if (!hierarchy.accepted) {
    receipt.failedObjectId = hierarchy.missingObjectId;
    receipt.status =
        hierarchy.status == CreativeHierarchySelectionStatus::MissingObject
            ? CreativeDuplicateCommandStatus::MissingObject
            : CreativeDuplicateCommandStatus::InvalidRequest;
    receipt.message = hierarchy.reasonCode;
    return receipt;
  }

  CreativeClipboard clipboard;
  const CreativeClipboardCopyReceipt copyReceipt =
      copyDocumentObjectsToClipboard(document, hierarchy.objectIds, clipboard);
  if (!copyReceipt.accepted) {
    receipt.failedObjectId = copyReceipt.failedObjectId;
    receipt.status = CreativeDuplicateCommandStatus::MissingObject;
    receipt.message = "duplicate_object_missing";
    return receipt;
  }

  CreativeClipboardPasteRequest pasteRequest;
  pasteRequest.offset = request.offset;
  pasteRequest.appendCopySuffix = request.appendCopySuffix;
  pasteRequest.externalParentPolicy =
      CreativeClipboardExternalParentPolicy::PreserveIfPresent;
  CreativeClipboardPasteReceipt pasteReceipt =
      pasteCreativeClipboardAtomically(document, clipboard, pasteRequest);
  if (!pasteReceipt.accepted) {
    receipt.failedObjectId = pasteReceipt.failedObjectId;
    receipt.status =
        pasteReceipt.status == CreativeClipboardStatus::ObjectIdExhausted
            ? CreativeDuplicateCommandStatus::InvalidRequest
            : CreativeDuplicateCommandStatus::Rejected;
    receipt.message =
        pasteReceipt.status == CreativeClipboardStatus::ObjectIdExhausted
            ? "duplicate_object_id_exhausted"
            : pasteReceipt.reasonCode;
    return receipt;
  }

  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = CreativeDuplicateCommandStatus::Applied;
  receipt.duplicatedObjectIds = std::move(pasteReceipt.pastedObjectIds);
  receipt.duplicatedObjectCount = receipt.duplicatedObjectIds.size();
  receipt.duplicatedPatternRecipeCount =
      pasteReceipt.pastedPatternRecipeCount;
  receipt.duplicatedSelectionObjectIds.reserve(hierarchy.rootObjectIds.size());
  for (CreativeObjectId rootObjectId : hierarchy.rootObjectIds) {
    const auto remap = std::find_if(
        pasteReceipt.idRemaps.begin(), pasteReceipt.idRemaps.end(),
        [rootObjectId](const CreativeClipboardIdRemap& item) {
          return item.sourceObjectId == rootObjectId;
        });
    if (remap != pasteReceipt.idRemaps.end()) {
      receipt.duplicatedSelectionObjectIds.push_back(remap->pastedObjectId);
    }
  }
  receipt.revisionAfter = document.revision();
  receipt.message = "duplicate_applied";
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
  if (clipboardCutHasExternalReferences(document, stagedClipboard,
                                        receipt.failedObjectId)) {
    receipt.status = CreativeClipboardStatus::RemoveRejected;
    receipt.reasonCode = "creative_clipboard_cut_external_reference";
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
  receipt.cutPatternRecipeCount = outClipboard.patternRecipes.size();
  receipt.revisionAfter = document.revision();
  receipt.reasonCode = "creative_clipboard_cut";
  return receipt;
}

CreativeSemanticDeleteReceipt deleteDocumentObjectsSemanticallyAtomically(
    CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds) {
  CreativeSemanticDeleteReceipt receipt;
  receipt.requested = true;
  receipt.requestedObjectCount = objectIds.size();
  receipt.revisionBefore = document.revision();
  receipt.revisionAfter = receipt.revisionBefore;

  const CreativeHierarchySelection hierarchy =
      resolveCreativeObjectHierarchy(document, objectIds);
  if (!hierarchy.accepted) {
    receipt.failedObjectId = hierarchy.missingObjectId;
    switch (hierarchy.status) {
      case CreativeHierarchySelectionStatus::EmptySelection:
        receipt.status = CreativeSemanticDeleteStatus::EmptySelection;
        break;
      case CreativeHierarchySelectionStatus::InvalidDocument:
        receipt.status = CreativeSemanticDeleteStatus::InvalidDocument;
        break;
      case CreativeHierarchySelectionStatus::MissingObject:
        receipt.status = CreativeSemanticDeleteStatus::MissingObject;
        break;
      case CreativeHierarchySelectionStatus::InvalidHierarchy:
      case CreativeHierarchySelectionStatus::NotRequested:
      case CreativeHierarchySelectionStatus::Ready:
        receipt.status = CreativeSemanticDeleteStatus::InvalidHierarchy;
        break;
    }
    receipt.reasonCode = std::string(hierarchy.reasonCode);
    return receipt;
  }

  CreativeClipboard discarded;
  const CreativeClipboardCutReceipt cut =
      cutDocumentObjectsAtomically(document, hierarchy.objectIds, discarded);
  if (!cut.accepted) {
    receipt.failedObjectId = cut.failedObjectId;
    receipt.revisionAfter = cut.revisionAfter;
    if (cut.reasonCode == "creative_clipboard_cut_external_reference") {
      receipt.status = CreativeSemanticDeleteStatus::ExternalReference;
    } else if (cut.status == CreativeClipboardStatus::MissingObject) {
      receipt.status = CreativeSemanticDeleteStatus::MissingObject;
    } else if (cut.status == CreativeClipboardStatus::InvalidClipboard ||
               cut.status == CreativeClipboardStatus::InvalidRequest) {
      receipt.status = CreativeSemanticDeleteStatus::InvalidHierarchy;
    } else {
      receipt.status = CreativeSemanticDeleteStatus::RemoveRejected;
    }
    receipt.reasonCode = cut.reasonCode;
    return receipt;
  }

  receipt.removedObjectIds.reserve(discarded.objects.size());
  for (const CreativeObject& object : discarded.objects) {
    receipt.removedObjectIds.push_back(object.id);
  }
  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = CreativeSemanticDeleteStatus::Deleted;
  receipt.removedObjectCount = receipt.removedObjectIds.size();
  receipt.removedPatternRecipeCount = discarded.patternRecipes.size();
  receipt.revisionAfter = document.revision();
  receipt.reasonCode = "creative_semantic_delete_applied";
  return receipt;
}

}  // namespace iggy3d::creative
