#include "app/iggy3d/creative/tools/SelectionResolution.hpp"

#include "app/iggy3d/creative/document/Hierarchy.hpp"
#include "app/iggy3d/creative/tools/Group.hpp"
#include "app/iggy3d/creative/world/WorldLayoutSourceDuplication.hpp"

#include <algorithm>
#include <unordered_set>

namespace iggy3d::creative {
namespace {

[[nodiscard]] constexpr bool actionNeedsSynchronizedWorldLayout(
    CreativeSemanticObjectActionRoute route) noexcept {
  return route == CreativeSemanticObjectActionRoute::WorldLayoutSource ||
         route == CreativeSemanticObjectActionRoute::RefineThenAdopt;
}

[[nodiscard]] CreativeWorldLayoutSourceRef
resolveCompleteWorldLayoutBuildingSource(
    const CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds,
    const CreativeWorldLayout& worldLayout) {
  CreativeWorldLayoutSourceRef source;
  if (objectIds.empty()) {
    return source;
  }
  const CreativeObject* primary = document.findObject(objectIds.front());
  if (primary == nullptr) {
    return source;
  }
  const CreativeWorldLayoutObjectProvenance provenance =
      resolveCreativeWorldLayoutObjectProvenance(worldLayout, *primary);
  const CreativeWorldLayoutSourceAncestry ancestry =
      buildCreativeWorldLayoutSourceAncestry(worldLayout, provenance);
  const auto building = std::find_if(
      ancestry.entries.begin(), ancestry.entries.begin() + ancestry.count,
      [](CreativeWorldLayoutSourceRef value) {
        return value.table == CreativeWorldLayoutTable::Building;
      });
  if (building == ancestry.entries.begin() + ancestry.count ||
      building->index >= worldLayout.buildings.size()) {
    return source;
  }

  std::unordered_set<CreativeObjectId> selected;
  selected.reserve(objectIds.size());
  for (CreativeObjectId objectId : objectIds) {
    const CreativeObject* object = document.findObject(objectId);
    if (object == nullptr ||
        !creativeWorldLayoutObjectBelongsToSource(
            worldLayout, *object, CreativeWorldLayoutTable::Building,
            building->index) ||
        !selected.insert(objectId).second) {
      return source;
    }
  }

  std::size_t completeMemberCount = 0U;
  for (const CreativeObject& object : document.objects()) {
    if (!creativeWorldLayoutObjectBelongsToSource(
            worldLayout, object, CreativeWorldLayoutTable::Building,
            building->index)) {
      continue;
    }
    ++completeMemberCount;
    if (!selected.contains(object.id)) {
      return source;
    }
  }
  if (completeMemberCount == 0U ||
      completeMemberCount != selected.size()) {
    return source;
  }
  return *building;
}

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
  }
  if (result.worldLayoutSource.owned ||
      creativeObjectHasWorldLayoutProvenanceTag(*object)) {
    result.primaryOwner = CreativeSemanticSelectionOwner::WorldLayoutSource;
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

bool creativeObjectHasWorldLayoutProvenanceTag(
    const CreativeObject& object) noexcept {
  return std::any_of(object.tags.begin(), object.tags.end(),
                     [](const std::string& tag) {
                       return tag.starts_with("creative_world_layout:");
                     });
}

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

  for (CreativeObjectId objectId : objectIds) {
    const CreativeSemanticSelectionResolution selection =
        resolveSelection(document, objectId, worldLayout, nullptr);
    switch (selection.primaryOwner) {
      case CreativeSemanticSelectionOwner::AuthoredObject:
        ++result.authoredOwnerCount;
        break;
      case CreativeSemanticSelectionOwner::PatternRecipe:
        ++result.patternOwnerCount;
        break;
      case CreativeSemanticSelectionOwner::WorldLayoutSource:
        ++result.worldLayoutOwnerCount;
        break;
      case CreativeSemanticSelectionOwner::None:
        break;
    }
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
  const bool onlyPatternOwned =
      result.patternOwnerCount > 0U && result.authoredOwnerCount == 0U &&
      result.worldLayoutOwnerCount == 0U;
  const bool onlyWorldLayoutOwned =
      result.worldLayoutOwnerCount > 0U && result.authoredOwnerCount == 0U &&
      result.patternOwnerCount == 0U;
  if (result.patternRecipeId != kInvalidCreativePatternRecipeId ||
      onlyPatternOwned) {
    result.primaryOwner = CreativeSemanticSelectionOwner::PatternRecipe;
  } else if (result.commonWorldLayoutSource.table !=
                 CreativeWorldLayoutTable::None ||
             onlyWorldLayoutOwned) {
    result.primaryOwner = CreativeSemanticSelectionOwner::WorldLayoutSource;
  } else {
    result.primaryOwner = CreativeSemanticSelectionOwner::AuthoredObject;
  }
  result.reasonCode = "creative_selection_set_ready";
  return result;
}

CreativeWorldLayoutSourceRef
resolveCompleteCreativeWorldLayoutBuildingSelectionSource(
    const CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds,
    const CreativeWorldLayout& worldLayout) {
  return resolveCompleteWorldLayoutBuildingSource(
      document, objectIds, worldLayout);
}

CreativeSemanticObjectActionPolicy resolveCreativeSemanticObjectAction(
    CreativeSemanticSelectionOwner owner,
    CreativeSemanticObjectAction action,
    CreativeWorldLayoutTable worldLayoutTable,
    std::size_t worldLayoutContributorCount) noexcept {
  if (action >= CreativeSemanticObjectAction::Count) {
    return {false, CreativeSemanticObjectActionRoute::Reject,
            "creative_semantic_action_invalid"};
  }
  if (action == CreativeSemanticObjectAction::Inspect ||
      action == CreativeSemanticObjectAction::Copy) {
    return owner == CreativeSemanticSelectionOwner::None
               ? CreativeSemanticObjectActionPolicy{
                     false, CreativeSemanticObjectActionRoute::Reject,
                     "creative_semantic_action_owner_missing"}
               : CreativeSemanticObjectActionPolicy{
                     true, CreativeSemanticObjectActionRoute::ReadOnly,
                     "creative_semantic_action_read_only"};
  }

  switch (owner) {
    case CreativeSemanticSelectionOwner::AuthoredObject:
      return {true, CreativeSemanticObjectActionRoute::Document,
              "creative_semantic_action_document"};
    case CreativeSemanticSelectionOwner::PatternRecipe:
      switch (action) {
        case CreativeSemanticObjectAction::Duplicate:
        case CreativeSemanticObjectAction::Delete:
        case CreativeSemanticObjectAction::Cut:
          return {true,
                  CreativeSemanticObjectActionRoute::SemanticDocument,
                  "creative_semantic_action_pattern_document"};
        case CreativeSemanticObjectAction::TransformSelection:
          return {true, CreativeSemanticObjectActionRoute::PatternRecipe,
                  "creative_semantic_action_pattern_recipe"};
        case CreativeSemanticObjectAction::Inspect:
        case CreativeSemanticObjectAction::Copy:
        case CreativeSemanticObjectAction::Rename:
        case CreativeSemanticObjectAction::SetVisible:
        case CreativeSemanticObjectAction::SetLocked:
        case CreativeSemanticObjectAction::SetTransform:
        case CreativeSemanticObjectAction::StructuralMutation:
        case CreativeSemanticObjectAction::Count:
          return {false, CreativeSemanticObjectActionRoute::Reject,
                  "creative_semantic_action_pattern_owned"};
      }
      break;
    case CreativeSemanticSelectionOwner::WorldLayoutSource:
      switch (action) {
        case CreativeSemanticObjectAction::Duplicate:
          if (creativeWorldLayoutSourceDuplicatePolicy(worldLayoutTable) !=
              CreativeWorldLayoutSourceDuplicatePolicy::Unsupported) {
            return {true,
                    CreativeSemanticObjectActionRoute::WorldLayoutSource,
                    "creative_semantic_action_world_layout_source"};
          }
          break;
        case CreativeSemanticObjectAction::Delete:
          if (worldLayoutTable != CreativeWorldLayoutTable::None &&
              worldLayoutTable !=
                  CreativeWorldLayoutTable::TerrainPathPoint &&
              worldLayoutTable != CreativeWorldLayoutTable::TopologyEdge) {
            return {true,
                    CreativeSemanticObjectActionRoute::WorldLayoutSource,
                    "creative_semantic_action_world_layout_source"};
          }
          break;
        case CreativeSemanticObjectAction::Rename:
          switch (worldLayoutTable) {
            case CreativeWorldLayoutTable::Building:
            case CreativeWorldLayoutTable::Level:
            case CreativeWorldLayoutTable::Room:
            case CreativeWorldLayoutTable::VerticalConnector:
            case CreativeWorldLayoutTable::Box:
            case CreativeWorldLayoutTable::Wall:
            case CreativeWorldLayoutTable::Opening:
            case CreativeWorldLayoutTable::RoofAperture:
            case CreativeWorldLayoutTable::Object:
              return {true,
                      CreativeSemanticObjectActionRoute::WorldLayoutSource,
                      "creative_semantic_action_world_layout_source"};
            case CreativeWorldLayoutTable::None:
            case CreativeWorldLayoutTable::TerrainProfile:
            case CreativeWorldLayoutTable::TerrainPath:
            case CreativeWorldLayoutTable::TerrainPathPoint:
            case CreativeWorldLayoutTable::TopologyEdge:
              break;
          }
          break;
        case CreativeSemanticObjectAction::SetVisible:
          if (worldLayoutTable == CreativeWorldLayoutTable::Building ||
              worldLayoutTable == CreativeWorldLayoutTable::Object) {
            return {true,
                    CreativeSemanticObjectActionRoute::WorldLayoutSource,
                    "creative_semantic_action_world_layout_source"};
          }
          break;
        case CreativeSemanticObjectAction::TransformSelection:
          if (worldLayoutTable == CreativeWorldLayoutTable::Building) {
            return {true,
                    CreativeSemanticObjectActionRoute::WorldLayoutSource,
                    "creative_semantic_action_world_layout_source"};
          }
          break;
        case CreativeSemanticObjectAction::SetTransform:
          if (worldLayoutContributorCount == 1U &&
              (worldLayoutTable == CreativeWorldLayoutTable::Object ||
               worldLayoutTable == CreativeWorldLayoutTable::Box)) {
            return {true,
                    CreativeSemanticObjectActionRoute::RefineThenAdopt,
                    "creative_semantic_action_refine_then_adopt"};
          }
          break;
        case CreativeSemanticObjectAction::Inspect:
        case CreativeSemanticObjectAction::Copy:
        case CreativeSemanticObjectAction::Cut:
        case CreativeSemanticObjectAction::SetLocked:
        case CreativeSemanticObjectAction::StructuralMutation:
        case CreativeSemanticObjectAction::Count:
          break;
      }
      return {false, CreativeSemanticObjectActionRoute::Reject,
              "creative_semantic_action_world_layout_owned"};
    case CreativeSemanticSelectionOwner::None:
      return {false, CreativeSemanticObjectActionRoute::Reject,
              "creative_semantic_action_owner_missing"};
  }
  return {false, CreativeSemanticObjectActionRoute::Reject,
          "creative_semantic_action_unsupported"};
}

CreativeSemanticObjectActionPolicy resolveCreativeSemanticObjectAction(
    const CreativeSemanticSelectionResolution& selection,
    CreativeSemanticObjectAction action) noexcept {
  if (!selection.accepted) {
    return {false, CreativeSemanticObjectActionRoute::Reject,
            selection.reasonCode};
  }
  return resolveCreativeSemanticObjectAction(
      selection.primaryOwner, action, selection.worldLayoutSource.table,
      selection.worldLayoutSource.contributorCount);
}

CreativeSemanticObjectActionPolicy resolveCreativeSemanticObjectAction(
    const CreativeSemanticSelectionSetResolution& selection,
    CreativeSemanticObjectAction action) noexcept {
  if (!selection.accepted) {
    return {false, CreativeSemanticObjectActionRoute::Reject,
            selection.reasonCode};
  }
  const std::size_t ownerKindCount =
      static_cast<std::size_t>(selection.authoredOwnerCount > 0U) +
      static_cast<std::size_t>(selection.patternOwnerCount > 0U) +
      static_cast<std::size_t>(selection.worldLayoutOwnerCount > 0U);
  if (ownerKindCount != 1U) {
    return {false, CreativeSemanticObjectActionRoute::Reject,
            ownerKindCount == 0U
                ? "creative_semantic_action_selection_empty"
                : "creative_semantic_action_mixed_ownership"};
  }
  CreativeSemanticSelectionOwner owner =
      CreativeSemanticSelectionOwner::WorldLayoutSource;
  if (selection.authoredOwnerCount > 0U) {
    owner = CreativeSemanticSelectionOwner::AuthoredObject;
  } else if (selection.patternOwnerCount > 0U) {
    owner = CreativeSemanticSelectionOwner::PatternRecipe;
  }
  return resolveCreativeSemanticObjectAction(
      owner, action, selection.commonWorldLayoutSource.table);
}

CreativeSemanticObjectActionFacts resolveCreativeSemanticObjectActionFacts(
    const CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds,
    CreativeObjectId primaryObjectId,
    const CreativeWorldLayout* worldLayout,
    bool worldLayoutSynchronized) {
  CreativeSemanticObjectActionFacts facts;
  facts.requested = true;
  facts.worldLayoutSynchronized =
      worldLayout != nullptr && worldLayoutSynchronized;
  facts.selection = resolveCreativeSemanticSelectionSet(
      document, objectIds, primaryObjectId, worldLayout);
  if (!facts.selection.accepted) {
    facts.failedObjectId = facts.selection.primaryObjectId;
    facts.reasonCode = facts.selection.reasonCode;
    return facts;
  }
  facts.selectionResolved = true;

  if (objectIds.size() == 1U) {
    facts.singleSelection =
        resolveCreativeSemanticSelection(document, objectIds.front(), worldLayout);
    facts.hasSingleSelection = facts.singleSelection.accepted;
    if (!facts.hasSingleSelection) {
      facts.selectionResolved = false;
      facts.failedObjectId = objectIds.front();
      facts.reasonCode = facts.singleSelection.reasonCode;
      return facts;
    }
  }

  const bool onlyWorldLayoutOwned =
      facts.selection.worldLayoutOwnerCount > 0U &&
      facts.selection.authoredOwnerCount == 0U &&
      facts.selection.patternOwnerCount == 0U;
  if (worldLayout != nullptr && onlyWorldLayoutOwned) {
    facts.completeWorldLayoutBuildingSource =
        resolveCompleteWorldLayoutBuildingSource(
            document, objectIds, *worldLayout);
  }

  const CreativeHierarchySelection hierarchy =
      resolveCreativeObjectHierarchy(document, objectIds);
  if (!hierarchy.accepted) {
    facts.failedObjectId = hierarchy.missingObjectId;
    facts.reasonCode = hierarchy.reasonCode;
    return facts;
  }
  facts.hierarchyResolved = true;
  facts.hierarchyObjectIds = hierarchy.objectIds;
  facts.allUnlocked = true;
  for (CreativeObjectId objectId : facts.hierarchyObjectIds) {
    if (creativeObjectEffectivelyLocked(document, objectId)) {
      facts.allUnlocked = false;
      facts.failedObjectId = objectId;
      break;
    }
  }
  facts.reasonCode = "creative_semantic_action_facts_ready";
  return facts;
}

CreativeSemanticObjectActionFacts resolveCreativeSemanticObjectActionFacts(
    const CreativeDocument& document,
    CreativeObjectId objectId,
    const CreativeWorldLayout* worldLayout,
    bool worldLayoutSynchronized) {
  const std::array objectIds{objectId};
  return resolveCreativeSemanticObjectActionFacts(
      document, objectIds, objectId, worldLayout, worldLayoutSynchronized);
}

CreativeSemanticObjectActionAdmission
resolveCreativeSemanticObjectActionAdmission(
    const CreativeSemanticObjectActionFacts& facts,
    CreativeSemanticObjectAction action) noexcept {
  CreativeSemanticObjectActionAdmission admission;
  admission.requested = true;
  admission.action = action;
  admission.failedObjectId = facts.failedObjectId;
  if (action >= CreativeSemanticObjectAction::Count) {
    admission.status =
        CreativeSemanticObjectActionAdmissionStatus::OwnershipRejected;
    admission.reasonCode = "creative_semantic_action_invalid";
    return admission;
  }
  if (!facts.selectionResolved) {
    admission.status =
        CreativeSemanticObjectActionAdmissionStatus::InvalidSelection;
    admission.reasonCode = facts.reasonCode;
    return admission;
  }

  CreativeSemanticObjectActionPolicy policy;
  if (action == CreativeSemanticObjectAction::TransformSelection &&
      facts.completeWorldLayoutBuildingSource.table ==
          CreativeWorldLayoutTable::Building) {
    policy = resolveCreativeSemanticObjectAction(
        CreativeSemanticSelectionOwner::WorldLayoutSource, action,
        facts.completeWorldLayoutBuildingSource.table);
  } else {
    policy = facts.hasSingleSelection
                 ? resolveCreativeSemanticObjectAction(facts.singleSelection,
                                                      action)
                 : resolveCreativeSemanticObjectAction(facts.selection, action);
  }
  admission.route = policy.route;
  admission.reasonCode = policy.reasonCode;
  if (!policy.allowed) {
    admission.status =
        CreativeSemanticObjectActionAdmissionStatus::OwnershipRejected;
    return admission;
  }
  if (actionNeedsSynchronizedWorldLayout(policy.route) &&
      !facts.worldLayoutSynchronized) {
    admission.status =
        CreativeSemanticObjectActionAdmissionStatus::
            WorldLayoutUnsynchronized;
    admission.reasonCode =
        "creative_editor_object_action_world_layout_unsynchronized";
    return admission;
  }
  if (creativeSemanticObjectActionRequiresUnlockedSelection(action)) {
    if (!facts.hierarchyResolved) {
      admission.status =
          CreativeSemanticObjectActionAdmissionStatus::InvalidSelection;
      admission.reasonCode = facts.reasonCode;
      return admission;
    }
    if (!facts.allUnlocked) {
      admission.status =
          CreativeSemanticObjectActionAdmissionStatus::SelectionLocked;
      admission.reasonCode =
          "creative_editor_object_action_selection_locked";
      return admission;
    }
  }

  admission.allowed = true;
  admission.status = CreativeSemanticObjectActionAdmissionStatus::Ready;
  return admission;
}

CreativeSemanticObjectActionAdmission
resolveCreativeSemanticObjectActionAdmission(
    const CreativeDocument& document,
    CreativeObjectId objectId,
    CreativeSemanticObjectAction action,
    const CreativeWorldLayout* worldLayout,
    bool worldLayoutSynchronized) {
  return resolveCreativeSemanticObjectActionAdmission(
      resolveCreativeSemanticObjectActionFacts(
          document, objectId, worldLayout, worldLayoutSynchronized),
      action);
}

CreativeSemanticObjectActionAdmissions
resolveCreativeSemanticObjectActionAdmissions(
    const CreativeSemanticObjectActionFacts& facts) noexcept {
  CreativeSemanticObjectActionAdmissions admissions;
  for (std::size_t index = 0U;
       index < kCreativeSemanticObjectActionAdmissionCount; ++index) {
    admissions.actions[index] =
        resolveCreativeSemanticObjectActionAdmission(
            facts, static_cast<CreativeSemanticObjectAction>(index));
  }
  return admissions;
}

CreativeSemanticObjectActionAdmission
creativeSemanticObjectActionAdmission(
    const CreativeSemanticObjectActionAdmissions& admissions,
    CreativeSemanticObjectAction action) noexcept {
  const std::size_t index = static_cast<std::size_t>(action);
  return index < admissions.actions.size()
             ? admissions.actions[index]
             : CreativeSemanticObjectActionAdmission{};
}

CreativeStructuralMutationAdmission
resolveCreativeStructuralMutationAdmission(
    const CreativeDocument& document,
    CreativeObjectId objectId,
    const CreativeWorldLayout* worldLayout) noexcept {
  CreativeStructuralMutationAdmission admission;
  admission.requested = true;
  admission.requestedObjectCount = 1U;
  admission.primaryObjectId = objectId;
  const CreativeSemanticSelectionResolution selection =
      resolveCreativeSemanticSelection(document, objectId, worldLayout);
  admission.selectionStatus = selection.status;
  if (!selection.accepted) {
    admission.status =
        CreativeStructuralMutationAdmissionStatus::InvalidSelection;
    admission.failedObjectId = objectId;
    admission.reasonCode = selection.reasonCode;
    return admission;
  }

  admission.selectionResolved = true;
  admission.resolvedObjectCount = 1U;
  const CreativeSemanticObjectActionPolicy policy =
      resolveCreativeSemanticObjectAction(
          selection, CreativeSemanticObjectAction::StructuralMutation);
  admission.route = policy.route;
  admission.reasonCode = policy.reasonCode;
  admission.allowed =
      policy.allowed &&
      policy.route == CreativeSemanticObjectActionRoute::Document;
  admission.status =
      admission.allowed
          ? CreativeStructuralMutationAdmissionStatus::Ready
          : CreativeStructuralMutationAdmissionStatus::OwnershipRejected;
  admission.failedObjectId =
      admission.allowed ? kInvalidObjectId : objectId;
  return admission;
}

CreativeStructuralMutationAdmission
resolveCreativeStructuralMutationAdmission(
    const CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds,
    CreativeObjectId primaryObjectId,
    const CreativeWorldLayout* worldLayout) noexcept {
  CreativeStructuralMutationAdmission admission;
  admission.requested = true;
  admission.requestedObjectCount = objectIds.size();
  const CreativeSemanticSelectionSetResolution selection =
      resolveCreativeSemanticSelectionSet(
          document, objectIds, primaryObjectId, worldLayout);
  admission.selectionStatus = selection.status;
  admission.resolvedObjectCount = selection.resolvedCount;
  admission.primaryObjectId = selection.primaryObjectId;
  if (!selection.accepted || objectIds.empty()) {
    admission.status =
        CreativeStructuralMutationAdmissionStatus::InvalidSelection;
    const auto missing = std::find_if(
        objectIds.begin(), objectIds.end(),
        [&](CreativeObjectId objectId) {
          return document.findObject(objectId) == nullptr;
        });
    if (missing != objectIds.end()) {
      admission.failedObjectId = *missing;
    } else if (!objectIds.empty()) {
      admission.failedObjectId = objectIds.front();
    }
    admission.reasonCode = selection.reasonCode;
    return admission;
  }

  admission.selectionResolved = true;
  const CreativeSemanticObjectActionPolicy policy =
      resolveCreativeSemanticObjectAction(
          selection, CreativeSemanticObjectAction::StructuralMutation);
  admission.route = policy.route;
  admission.reasonCode = policy.reasonCode;
  admission.allowed =
      policy.allowed &&
      policy.route == CreativeSemanticObjectActionRoute::Document;
  admission.status =
      admission.allowed
          ? CreativeStructuralMutationAdmissionStatus::Ready
          : CreativeStructuralMutationAdmissionStatus::OwnershipRejected;
  if (admission.allowed) {
    return admission;
  }

  admission.failedObjectId = admission.primaryObjectId;
  for (CreativeObjectId objectId : objectIds) {
    const CreativeStructuralMutationAdmission objectAdmission =
        resolveCreativeStructuralMutationAdmission(
            document, objectId, worldLayout);
    if (!objectAdmission.allowed) {
      admission.failedObjectId = objectId;
      break;
    }
  }
  return admission;
}

}  // namespace iggy3d::creative
