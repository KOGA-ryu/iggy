#include "app/iggy3d/creative/tools/SelectionResolution.hpp"

#include "app/iggy3d/creative/document/Hierarchy.hpp"

#include <algorithm>

namespace iggy3d::creative {
namespace {

[[nodiscard]] CreativeSemanticSelectionResolution resolveSelection(
    const CreativeDocument& document,
    CreativeObjectId objectId,
    const CreativeWorldLayout* worldLayout,
    const CreativeVec3* sourcePointCells) noexcept {
  CreativeSemanticSelectionResolution result;
  result.requested = true;
  result.objectId = objectId;
  if (!document.isValid()) {
    result.status = CreativeSemanticSelectionStatus::InvalidDocument;
    result.reasonCode = "creative_selection_document_invalid";
    return result;
  }

  const CreativeObject* object = document.findObject(objectId);
  if (object == nullptr) {
    result.status = CreativeSemanticSelectionStatus::MissingObject;
    result.reasonCode = "creative_selection_object_missing";
    return result;
  }

  result.accepted = true;
  result.status = CreativeSemanticSelectionStatus::Ready;
  result.primaryOwner = CreativeSemanticSelectionOwner::AuthoredObject;
  result.objectKind = object->kind;
  const CreativeObjectHierarchyState hierarchyState =
      resolveCreativeObjectHierarchyState(document, objectId);
  result.objectVisible =
      hierarchyState.resolved && hierarchyState.effectivelyVisible;
  result.objectLocked =
      !hierarchyState.resolved || hierarchyState.effectivelyLocked;
  if (object->parentId.has_value()) {
    result.hasParent = true;
    result.parentObjectId = *object->parentId;
  }

  if (worldLayout != nullptr) {
    result.worldLayoutSource =
        sourcePointCells != nullptr
            ? resolveCreativeWorldLayoutObjectProvenance(
                  *worldLayout, *object, *sourcePointCells)
            : resolveCreativeWorldLayoutObjectProvenance(*worldLayout, *object);
    if (result.worldLayoutSource.owned) {
      result.primaryOwner = CreativeSemanticSelectionOwner::WorldLayoutSource;
    }
  }

  const CreativePatternRecipe* pattern =
      findCreativePatternRecipeByGeneratedObject(
          document.patternRecipeStore(), objectId);
  if (pattern != nullptr) {
    result.patternRecipeId = pattern->id;
    result.patternRecipeKind = pattern->kind;
    result.primaryOwner = CreativeSemanticSelectionOwner::PatternRecipe;
  }

  result.reasonCode = "creative_selection_ready";
  return result;
}

}  // namespace

std::string_view toString(CreativeSemanticSelectionOwner owner) noexcept {
  switch (owner) {
    case CreativeSemanticSelectionOwner::None: return "none";
    case CreativeSemanticSelectionOwner::AuthoredObject:
      return "authored_object";
    case CreativeSemanticSelectionOwner::PatternRecipe:
      return "pattern_recipe";
    case CreativeSemanticSelectionOwner::WorldLayoutSource:
      return "world_layout_source";
  }
  return "none";
}

std::string_view toString(CreativeSemanticSelectionStatus status) noexcept {
  switch (status) {
    case CreativeSemanticSelectionStatus::NotRequested: return "not_requested";
    case CreativeSemanticSelectionStatus::InvalidDocument:
      return "invalid_document";
    case CreativeSemanticSelectionStatus::MissingObject:
      return "missing_object";
    case CreativeSemanticSelectionStatus::Ready: return "ready";
  }
  return "not_requested";
}

CreativeSemanticSelectionResolution resolveCreativeSemanticSelection(
    const CreativeDocument& document,
    CreativeObjectId objectId,
    const CreativeWorldLayout* worldLayout) noexcept {
  return resolveSelection(document, objectId, worldLayout, nullptr);
}

CreativeSemanticSelectionResolution resolveCreativeSemanticSelection(
    const CreativeDocument& document,
    CreativeObjectId objectId,
    const CreativeWorldLayout& worldLayout,
    CreativeVec3 sourcePointCells) noexcept {
  return resolveSelection(document, objectId, &worldLayout, &sourcePointCells);
}

CreativeSemanticSelectionSetResolution resolveCreativeSemanticSelectionSet(
    const CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds,
    CreativeObjectId primaryObjectId,
    const CreativeWorldLayout* worldLayout) noexcept {
  CreativeSemanticSelectionSetResolution result;
  result.requested = true;
  result.selectedCount = objectIds.size();
  if (!document.isValid()) {
    result.status = CreativeSemanticSelectionStatus::InvalidDocument;
    result.reasonCode = "creative_selection_set_document_invalid";
    return result;
  }
  if (objectIds.empty()) {
    result.accepted = true;
    result.status = CreativeSemanticSelectionStatus::Ready;
    result.reasonCode = "creative_selection_set_empty";
    return result;
  }

  const bool requestedPrimaryPresent =
      primaryObjectId != kInvalidObjectId &&
      std::find(objectIds.begin(), objectIds.end(), primaryObjectId) !=
          objectIds.end() &&
      document.findObject(primaryObjectId) != nullptr;
  result.primaryObjectId = requestedPrimaryPresent ? primaryObjectId
                                                   : objectIds.front();

  bool commonPatternInitialized = false;
  bool commonPattern = true;
  for (CreativeObjectId objectId : objectIds) {
    const CreativeObject* object = document.findObject(objectId);
    if (object == nullptr) {
      ++result.missingCount;
      continue;
    }
    ++result.resolvedCount;
    const CreativePatternRecipe* recipe =
        findCreativePatternRecipeByGeneratedObject(document.patternRecipeStore(),
                                                   objectId);
    if (!commonPatternInitialized) {
      commonPatternInitialized = true;
      if (recipe != nullptr) {
        result.patternRecipeId = recipe->id;
        result.patternRecipeKind = recipe->kind;
      } else {
        commonPattern = false;
      }
    } else if (recipe == nullptr ||
               recipe->id != result.patternRecipeId) {
      commonPattern = false;
    }
  }
  if (result.missingCount > 0U || result.resolvedCount != objectIds.size()) {
    result.status = CreativeSemanticSelectionStatus::MissingObject;
    result.patternRecipeId = kInvalidCreativePatternRecipeId;
    result.patternRecipeKind = CreativePatternRecipeKind::Count;
    result.reasonCode = "creative_selection_set_object_missing";
    return result;
  }
  if (!commonPattern) {
    result.patternRecipeId = kInvalidCreativePatternRecipeId;
    result.patternRecipeKind = CreativePatternRecipeKind::Count;
  }

  if (worldLayout != nullptr) {
    const CreativeObject* primary =
        document.findObject(result.primaryObjectId);
    if (primary != nullptr) {
      const CreativeWorldLayoutObjectProvenance provenance =
          resolveCreativeWorldLayoutObjectProvenance(*worldLayout, *primary);
      const CreativeWorldLayoutSourceAncestry ancestry =
          buildCreativeWorldLayoutSourceAncestry(*worldLayout, provenance);
      for (std::size_t ancestryIndex = ancestry.count;
           ancestryIndex > 0U; --ancestryIndex) {
        const CreativeWorldLayoutSourceRef candidate =
            ancestry.entries[ancestryIndex - 1U];
        const bool shared = std::all_of(
            objectIds.begin(), objectIds.end(),
            [&](CreativeObjectId objectId) {
              const CreativeObject* object = document.findObject(objectId);
              return object != nullptr &&
                     creativeWorldLayoutObjectBelongsToSource(
                         *worldLayout, *object, candidate.table,
                         candidate.index);
            });
        if (shared) {
          result.commonWorldLayoutSource = candidate;
          break;
        }
      }
    }
  }

  result.accepted = true;
  result.status = CreativeSemanticSelectionStatus::Ready;
  if (result.patternRecipeId != kInvalidCreativePatternRecipeId) {
    result.primaryOwner = CreativeSemanticSelectionOwner::PatternRecipe;
  } else if (result.commonWorldLayoutSource.table !=
             CreativeWorldLayoutTable::None) {
    result.primaryOwner = CreativeSemanticSelectionOwner::WorldLayoutSource;
  } else {
    result.primaryOwner = CreativeSemanticSelectionOwner::AuthoredObject;
  }
  result.reasonCode = "creative_selection_set_ready";
  return result;
}

}  // namespace iggy3d::creative
