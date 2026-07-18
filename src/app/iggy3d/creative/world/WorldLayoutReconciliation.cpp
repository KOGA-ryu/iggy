#include "app/iggy3d/creative/world/WorldLayoutReconciliation.hpp"

#include <algorithm>
#include <map>
#include <span>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace iggy3d::creative {
namespace {

constexpr std::string_view kWorldLayoutSourcePrefix =
    "creative_world_layout_source:";
constexpr std::string_view kWorldLayoutRoomEdgePrefix =
    "creative_world_layout_room_edge:";

struct ExistingGroupAnalysis {
  bool allMatchStoredBaseline = false;
  std::uint64_t refinedObjectCount = 0U;
  std::uint64_t missingBaselineCount = 0U;
};

struct RecipePatchAnalysis {
  ExistingGroupAnalysis summary;
  bool groupConflict = false;
  bool requiresPatch = false;
  std::vector<CreativeObjectId> existingObjectIds;
  std::vector<CreativeWorldLayoutRecipeMemberAction> memberActions;
  std::vector<const CreativeObject*> removeObjects;
  std::vector<CreativeWorldLayoutRecipeMemberConflict> memberConflicts;
  std::vector<std::size_t> conflictDesiredIndices;
  std::vector<const CreativeObject*> conflictObjects;
};

struct RecipeRemovalAnalysis {
  ExistingGroupAnalysis summary;
  bool groupConflict = false;
  std::vector<const CreativeObject*> removeObjects;
  std::vector<CreativeWorldLayoutRecipeMemberConflict> memberConflicts;
  std::vector<const CreativeObject*> conflictObjects;
};

[[nodiscard]] CreativeWorldLayoutRecipeMemberCounts memberCounts(
    std::uint64_t createCount = 0U,
    std::uint64_t preserveCount = 0U,
    std::uint64_t updateCount = 0U,
    std::uint64_t removeCount = 0U,
    std::uint64_t detachCount = 0U) noexcept {
  return {createCount, preserveCount, updateCount, removeCount, detachCount};
}

[[nodiscard]] CreativeWorldLayoutRecipeMemberCounts memberCountsForPatch(
    std::span<const CreativeWorldLayoutRecipeMemberAction> actions,
    std::size_t removeCount,
    std::size_t detachCount = 0U) noexcept {
  CreativeWorldLayoutRecipeMemberCounts counts;
  counts.removeCount = removeCount;
  counts.detachCount = detachCount;
  for (const CreativeWorldLayoutRecipeMemberAction action : actions) {
    switch (action) {
      case CreativeWorldLayoutRecipeMemberAction::Create:
        ++counts.createCount;
        break;
      case CreativeWorldLayoutRecipeMemberAction::Preserve:
        ++counts.preserveCount;
        break;
      case CreativeWorldLayoutRecipeMemberAction::Update:
        ++counts.updateCount;
        break;
      case CreativeWorldLayoutRecipeMemberAction::Count:
        break;
    }
  }
  return counts;
}

struct ConflictDecisionEntry {
  const CreativeWorldLayoutConflictDecision* decision = nullptr;
  std::size_t requestIndex = 0U;
};

struct ConflictDecisionLookup {
  std::map<std::string_view, ConflictDecisionEntry> groupEntries;
  std::map<std::pair<std::string_view, std::string_view>,
           ConflictDecisionEntry>
      memberEntries;
  std::vector<bool> consumed;
};

[[nodiscard]] bool hasTag(std::span<const std::string> tags,
                          std::string_view expected) noexcept {
  return std::any_of(tags.begin(), tags.end(),
                     [expected](const std::string& tag) {
                       return tag == expected;
                     });
}

[[nodiscard]] std::string_view parentStableKey(
    const CreativeDocument& document,
    const CreativeObject& object) noexcept {
  if (!object.parentId.has_value()) {
    return {};
  }
  const CreativeObject* parent = document.findObject(*object.parentId);
  return parent != nullptr ? creativeRecipeObjectStableKey(*parent)
                           : std::string_view{};
}

[[nodiscard]] bool matchesStoredBaseline(
    const CreativeDocument& document,
    const CreativeObject& object) noexcept {
  const std::uint64_t baseline =
      creativeRecipeObjectOutputFingerprint(object);
  return baseline != 0U &&
         fingerprintCreativeRecipeObjectState(
             object, parentStableKey(document, object)) == baseline;
}

[[nodiscard]] bool hasExternalChild(
    const CreativeDocument& document,
    CreativeObjectId objectId,
    const std::unordered_set<CreativeObjectId>& groupObjectIds) noexcept {
  return std::any_of(
      document.objects().begin(), document.objects().end(),
      [&](const CreativeObject& candidate) {
        return candidate.parentId == objectId &&
               !groupObjectIds.contains(candidate.id);
      });
}

[[nodiscard]] bool hasLogicLink(const CreativeDocument& document,
                                CreativeObjectId objectId) noexcept {
  return std::any_of(
      document.logicLinks().begin(), document.logicLinks().end(),
      [&](const CreativeLogicLink& link) {
        return link.sourceObjectId == objectId ||
               link.targetObjectId == objectId;
      });
}

[[nodiscard]] CreativeWorldLayoutRecipeMemberConflict makeMemberConflict(
    CreativeWorldLayoutMemberConflictKind kind,
    const CreativeObject& object,
    std::uint64_t baselineFingerprint,
    std::uint64_t currentFingerprint,
    std::uint64_t desiredFingerprint) {
  CreativeWorldLayoutRecipeMemberConflict conflict;
  conflict.kind = kind;
  conflict.stableKey = creativeRecipeObjectStableKey(object);
  conflict.objectName = object.name;
  conflict.objectId = object.id;
  conflict.baselineFingerprint = baselineFingerprint;
  conflict.currentFingerprint = currentFingerprint;
  conflict.desiredFingerprint = desiredFingerprint;
  return conflict;
}

[[nodiscard]] RecipePatchAnalysis analyzeRecipePatch(
    const CreativeDocument& document,
    std::span<const CreativeObject* const> existing,
    const CreativeRecipePlan& desired) {
  RecipePatchAnalysis analysis;
  analysis.summary.allMatchStoredBaseline = !existing.empty();
  analysis.existingObjectIds.assign(desired.objects.size(),
                                    kInvalidObjectId);
  analysis.memberActions.assign(
      desired.objects.size(), CreativeWorldLayoutRecipeMemberAction::Create);

  std::unordered_set<CreativeObjectId> groupObjectIds;
  groupObjectIds.reserve(existing.size());
  std::map<std::string_view, const CreativeObject*> byStableKey;
  for (const CreativeObject* object : existing) {
    groupObjectIds.insert(object->id);
    const std::uint64_t baseline =
        creativeRecipeObjectOutputFingerprint(*object);
    const bool matchesBaseline =
        baseline != 0U && matchesStoredBaseline(document, *object);
    if (baseline == 0U) {
      ++analysis.summary.missingBaselineCount;
    }
    if (!matchesBaseline) {
      analysis.summary.allMatchStoredBaseline = false;
      ++analysis.summary.refinedObjectCount;
    }

    const std::string_view stableKey =
        creativeRecipeObjectStableKey(*object);
    if (stableKey.empty() ||
        !byStableKey.emplace(stableKey, object).second) {
      analysis.groupConflict = true;
      ++analysis.summary.refinedObjectCount;
    }
  }

  for (std::size_t desiredIndex = 0U;
       desiredIndex < desired.objects.size(); ++desiredIndex) {
    const CreativeRecipeObjectPlan& desiredObject =
        desired.objects[desiredIndex];
    const std::uint64_t desiredFingerprint =
        fingerprintCreativeRecipeObjectPlan(desired, desiredIndex);
    const auto found = byStableKey.find(desiredObject.stableKey);
    if (found == byStableKey.end()) {
      analysis.requiresPatch = true;
      if (desiredFingerprint == 0U) {
        analysis.groupConflict = true;
      }
      continue;
    }

    const CreativeObject& object = *found->second;
    analysis.existingObjectIds[desiredIndex] = object.id;
    if (!creativeRecipeObjectHasInstanceProvenance(
            object, desired.kind, desired.instanceKey, desiredObject.role,
            desiredObject.stableKey) ||
        desiredFingerprint == 0U) {
      analysis.groupConflict = true;
      ++analysis.summary.refinedObjectCount;
      byStableKey.erase(found);
      continue;
    }

    const std::uint64_t baseline =
        creativeRecipeObjectOutputFingerprint(object);
    const std::uint64_t current = fingerprintCreativeRecipeObjectState(
        object, parentStableKey(document, object));
    CreativeWorldLayoutRecipeMemberAction action =
        CreativeWorldLayoutRecipeMemberAction::Preserve;
    if (baseline == 0U) {
      if (current != desiredFingerprint) {
        analysis.groupConflict = true;
      }
      analysis.requiresPatch = true;
    } else if (current != baseline) {
      if (desiredFingerprint != baseline) {
        analysis.memberConflicts.push_back(makeMemberConflict(
            CreativeWorldLayoutMemberConflictKind::ConcurrentEdit,
            object, baseline, current, desiredFingerprint));
        analysis.conflictDesiredIndices.push_back(desiredIndex);
        analysis.conflictObjects.push_back(&object);
        analysis.requiresPatch = true;
      }
    } else if (desiredFingerprint != current) {
      action = CreativeWorldLayoutRecipeMemberAction::Update;
      analysis.requiresPatch = true;
    }
    if (!creativeRecipeObjectHasDefinitionFingerprint(
            object, desired.definitionFingerprint)) {
      analysis.requiresPatch = true;
    }
    analysis.memberActions[desiredIndex] = action;
    byStableKey.erase(found);
  }

  for (const auto& [stableKey, object] : byStableKey) {
    static_cast<void>(stableKey);
    const std::uint64_t baseline =
        creativeRecipeObjectOutputFingerprint(*object);
    const std::uint64_t current = fingerprintCreativeRecipeObjectState(
        *object, parentStableKey(document, *object));
    const bool externalChild =
        hasExternalChild(document, object->id, groupObjectIds);
    const bool linked = hasLogicLink(document, object->id);
    if (baseline == 0U || current != baseline || externalChild || linked) {
      const CreativeWorldLayoutMemberConflictKind kind =
          externalChild
              ? CreativeWorldLayoutMemberConflictKind::SourceRemovedParent
              : linked
                    ? CreativeWorldLayoutMemberConflictKind::SourceRemovedLinked
                    : CreativeWorldLayoutMemberConflictKind::
                          SourceRemovedRefinement;
      analysis.memberConflicts.push_back(
          makeMemberConflict(kind, *object, baseline, current, 0U));
      analysis.conflictDesiredIndices.push_back(
          kInvalidCreativeWorldLayoutRecipeIndex);
      analysis.conflictObjects.push_back(object);
      analysis.requiresPatch = true;
      ++analysis.summary.refinedObjectCount;
    } else {
      analysis.removeObjects.push_back(object);
      analysis.requiresPatch = true;
    }
  }

  return analysis;
}

[[nodiscard]] RecipeRemovalAnalysis analyzeRecipeRemoval(
    const CreativeDocument& document,
    std::span<const CreativeObject* const> existing) {
  RecipeRemovalAnalysis analysis;
  analysis.summary.allMatchStoredBaseline = !existing.empty();
  std::unordered_set<CreativeObjectId> groupObjectIds;
  groupObjectIds.reserve(existing.size());
  std::map<std::string_view, const CreativeObject*> byStableKey;
  for (const CreativeObject* object : existing) {
    groupObjectIds.insert(object->id);
    const std::string_view stableKey = creativeRecipeObjectStableKey(*object);
    if (stableKey.empty() ||
        !byStableKey.emplace(stableKey, object).second) {
      analysis.groupConflict = true;
    }
  }

  for (const CreativeObject* object : existing) {
    const std::uint64_t baseline =
        creativeRecipeObjectOutputFingerprint(*object);
    const std::uint64_t current = fingerprintCreativeRecipeObjectState(
        *object, parentStableKey(document, *object));
    const bool externalChild =
        hasExternalChild(document, object->id, groupObjectIds);
    const bool linked = hasLogicLink(document, object->id);
    if (baseline == 0U) {
      ++analysis.summary.missingBaselineCount;
    }
    if (baseline != 0U && current == baseline && !externalChild && !linked) {
      analysis.removeObjects.push_back(object);
      continue;
    }

    analysis.summary.allMatchStoredBaseline = false;
    ++analysis.summary.refinedObjectCount;
    const CreativeWorldLayoutMemberConflictKind kind =
        externalChild
            ? CreativeWorldLayoutMemberConflictKind::SourceRemovedParent
            : linked
                  ? CreativeWorldLayoutMemberConflictKind::SourceRemovedLinked
                  : CreativeWorldLayoutMemberConflictKind::
                        SourceRemovedRefinement;
    analysis.memberConflicts.push_back(
        makeMemberConflict(kind, *object, baseline, current, 0U));
    analysis.conflictObjects.push_back(object);
  }
  return analysis;
}

void appendIds(std::vector<CreativeObjectId>& output,
               std::span<const CreativeObject* const> objects) {
  output.reserve(output.size() + objects.size());
  for (const CreativeObject* object : objects) {
    output.push_back(object->id);
  }
}

[[nodiscard]] bool buildConflictDecisionLookup(
    std::span<const CreativeWorldLayoutConflictDecision> decisions,
    ConflictDecisionLookup& output) {
  output.consumed.assign(decisions.size(), false);
  for (std::size_t index = 0U; index < decisions.size(); ++index) {
    const CreativeWorldLayoutConflictDecision& decision = decisions[index];
    if (decision.instanceKey.empty() ||
        decision.resolution == CreativeWorldLayoutConflictResolution::Block ||
        decision.resolution >= CreativeWorldLayoutConflictResolution::Count) {
      return false;
    }
    if (decision.memberStableKey.empty()) {
      if ((decision.resolution !=
               CreativeWorldLayoutConflictResolution::Regenerate &&
           decision.resolution !=
               CreativeWorldLayoutConflictResolution::Detach) ||
          decision.memberConflictKind !=
              CreativeWorldLayoutMemberConflictKind::Count ||
          decision.objectId != kInvalidObjectId ||
          decision.baselineFingerprint != 0U ||
          decision.currentFingerprint != 0U ||
          decision.desiredFingerprint != 0U ||
          !output.groupEntries
               .emplace(std::string_view(decision.instanceKey),
                        ConflictDecisionEntry{&decision, index})
               .second) {
        return false;
      }
      continue;
    }
    if (decision.memberConflictKind >=
            CreativeWorldLayoutMemberConflictKind::Count ||
        decision.objectId == kInvalidObjectId ||
        decision.currentFingerprint == 0U ||
        !creativeWorldLayoutMemberResolutionAllowed(
            decision.memberConflictKind, decision.resolution) ||
        !output.memberEntries
             .emplace(std::pair{std::string_view(decision.instanceKey),
                                std::string_view(decision.memberStableKey)},
                      ConflictDecisionEntry{&decision, index})
             .second) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] const CreativeWorldLayoutConflictDecision* groupDecisionFor(
    ConflictDecisionLookup& decisions,
    std::string_view instanceKey) noexcept {
  const auto found = decisions.groupEntries.find(instanceKey);
  if (found == decisions.groupEntries.end()) {
    return nullptr;
  }
  decisions.consumed[found->second.requestIndex] = true;
  return found->second.decision;
}

[[nodiscard]] const CreativeWorldLayoutConflictDecision* memberDecisionFor(
    ConflictDecisionLookup& decisions,
    std::string_view instanceKey,
    const CreativeWorldLayoutRecipeMemberConflict& conflict) noexcept {
  const auto found = decisions.memberEntries.find(
      std::pair{instanceKey, std::string_view(conflict.stableKey)});
  if (found == decisions.memberEntries.end()) {
    return nullptr;
  }
  const CreativeWorldLayoutConflictDecision& decision =
      *found->second.decision;
  if (decision.memberConflictKind != conflict.kind ||
      decision.objectId != conflict.objectId ||
      decision.baselineFingerprint != conflict.baselineFingerprint ||
      decision.currentFingerprint != conflict.currentFingerprint ||
      decision.desiredFingerprint != conflict.desiredFingerprint) {
    return nullptr;
  }
  decisions.consumed[found->second.requestIndex] = true;
  return &decision;
}

[[nodiscard]] bool allConflictDecisionsConsumed(
    const ConflictDecisionLookup& decisions) noexcept {
  return std::all_of(decisions.consumed.begin(), decisions.consumed.end(),
                     [](bool consumed) { return consumed; });
}

void clearExecutableOperations(
    CreativeWorldLayoutReconciliationResult& result) {
  result.removeObjectIds.clear();
  result.detachObjectIds.clear();
  result.applyRecipeIndices.clear();
  result.patchDecisions.clear();
}

CreativeWorldLayoutRecipeChange makeChange(
    CreativeWorldLayoutRecipeChangeKind kind,
    CreativeRecipeKind recipeKind,
    std::string instanceKey,
    std::size_t desiredRecipeIndex,
    std::size_t existingObjectCount,
    std::size_t desiredObjectCount,
    const ExistingGroupAnalysis& analysis = {},
    CreativeWorldLayoutRecipeMemberCounts counts = {},
    std::span<const CreativeWorldLayoutRecipeMemberConflict>
        memberConflicts = {}) {
  return {kind,
          recipeKind,
          std::move(instanceKey),
          desiredRecipeIndex,
          existingObjectCount,
          desiredObjectCount,
          counts,
          analysis.refinedObjectCount,
          analysis.missingBaselineCount,
          std::vector<CreativeWorldLayoutRecipeMemberConflict>(
              memberConflicts.begin(), memberConflicts.end())};
}

void appendConflictResolution(
    CreativeWorldLayoutReconciliationResult& result,
    CreativeWorldLayoutConflictResolution resolution,
    std::span<const CreativeObject* const> existing,
    const CreativeRecipePlan* desired,
    std::size_t desiredIndex,
    ExistingGroupAnalysis analysis,
    std::string instanceKey,
    std::span<const CreativeWorldLayoutRecipeMemberConflict>
        memberConflicts = {}) {
  ++result.conflictRecipeCount;
  if (resolution == CreativeWorldLayoutConflictResolution::Block) {
    result.blocked = true;
    result.changes.push_back(makeChange(
        CreativeWorldLayoutRecipeChangeKind::Conflict,
        desired != nullptr ? desired->kind : CreativeRecipeKind::Unknown,
        std::move(instanceKey), desiredIndex, existing.size(),
        desired != nullptr ? desired->objects.size() : 0U, analysis, {},
        memberConflicts));
    return;
  }
  if (resolution == CreativeWorldLayoutConflictResolution::Regenerate) {
    appendIds(result.removeObjectIds, existing);
    if (desired != nullptr) {
      result.applyRecipeIndices.push_back(desiredIndex);
      ++result.replaceRecipeCount;
      result.changes.push_back(makeChange(
          CreativeWorldLayoutRecipeChangeKind::Replace, desired->kind,
          std::move(instanceKey), desiredIndex, existing.size(),
          desired->objects.size(), analysis,
          memberCounts(desired->objects.size(), 0U, 0U,
                       existing.size()),
          memberConflicts));
    } else {
      ++result.removeRecipeCount;
      result.changes.push_back(makeChange(
          CreativeWorldLayoutRecipeChangeKind::Remove,
          CreativeRecipeKind::Unknown, std::move(instanceKey), desiredIndex,
          existing.size(), 0U, analysis,
          memberCounts(0U, 0U, 0U, existing.size()), memberConflicts));
    }
    return;
  }

  appendIds(result.detachObjectIds, existing);
  ++result.detachRecipeCount;
  if (desired != nullptr) {
    result.applyRecipeIndices.push_back(desiredIndex);
    ++result.createRecipeCount;
    result.changes.push_back(makeChange(
        CreativeWorldLayoutRecipeChangeKind::DetachAndReplace, desired->kind,
        std::move(instanceKey), desiredIndex, existing.size(),
        desired->objects.size(), analysis,
        memberCounts(desired->objects.size(), 0U, 0U, 0U,
                     existing.size()),
        memberConflicts));
  } else {
    result.changes.push_back(makeChange(
        CreativeWorldLayoutRecipeChangeKind::Detach,
        CreativeRecipeKind::Unknown, std::move(instanceKey), desiredIndex,
        existing.size(), 0U, analysis,
        memberCounts(0U, 0U, 0U, 0U, existing.size()), memberConflicts));
  }
}

}  // namespace

std::string_view toString(CreativeWorldLayoutRecipeChangeKind kind) noexcept {
  switch (kind) {
    case CreativeWorldLayoutRecipeChangeKind::Add: return "Add";
    case CreativeWorldLayoutRecipeChangeKind::Keep: return "Keep";
    case CreativeWorldLayoutRecipeChangeKind::Refined: return "Refined";
    case CreativeWorldLayoutRecipeChangeKind::Patch: return "Patch";
    case CreativeWorldLayoutRecipeChangeKind::Replace: return "Replace";
    case CreativeWorldLayoutRecipeChangeKind::Remove: return "Remove";
    case CreativeWorldLayoutRecipeChangeKind::Conflict: return "Conflict";
    case CreativeWorldLayoutRecipeChangeKind::DetachAndReplace:
      return "DetachAndReplace";
    case CreativeWorldLayoutRecipeChangeKind::Detach: return "Detach";
  }
  return "Unknown";
}

std::string_view toString(
    CreativeWorldLayoutConflictResolution resolution) noexcept {
  switch (resolution) {
    case CreativeWorldLayoutConflictResolution::Block: return "Block";
    case CreativeWorldLayoutConflictResolution::Regenerate:
      return "Regenerate";
    case CreativeWorldLayoutConflictResolution::Detach: return "Detach";
    case CreativeWorldLayoutConflictResolution::UseSource:
      return "UseSource";
    case CreativeWorldLayoutConflictResolution::KeepRefinement:
      return "KeepRefinement";
    case CreativeWorldLayoutConflictResolution::RemoveMember:
      return "RemoveMember";
    case CreativeWorldLayoutConflictResolution::DetachMember:
      return "DetachMember";
    case CreativeWorldLayoutConflictResolution::Count: break;
  }
  return "Unknown";
}

std::string_view toString(
    CreativeWorldLayoutMemberConflictKind kind) noexcept {
  switch (kind) {
    case CreativeWorldLayoutMemberConflictKind::ConcurrentEdit:
      return "ConcurrentEdit";
    case CreativeWorldLayoutMemberConflictKind::SourceRemovedRefinement:
      return "SourceRemovedRefinement";
    case CreativeWorldLayoutMemberConflictKind::SourceRemovedLinked:
      return "SourceRemovedLinked";
    case CreativeWorldLayoutMemberConflictKind::SourceRemovedParent:
      return "SourceRemovedParent";
    case CreativeWorldLayoutMemberConflictKind::Count: break;
  }
  return "Unknown";
}

bool creativeWorldLayoutMemberResolutionAllowed(
    CreativeWorldLayoutMemberConflictKind kind,
    CreativeWorldLayoutConflictResolution resolution) noexcept {
  switch (kind) {
    case CreativeWorldLayoutMemberConflictKind::ConcurrentEdit:
      return resolution == CreativeWorldLayoutConflictResolution::UseSource ||
             resolution ==
                 CreativeWorldLayoutConflictResolution::KeepRefinement;
    case CreativeWorldLayoutMemberConflictKind::SourceRemovedRefinement:
    case CreativeWorldLayoutMemberConflictKind::SourceRemovedLinked:
      return resolution ==
                 CreativeWorldLayoutConflictResolution::RemoveMember ||
             resolution ==
                 CreativeWorldLayoutConflictResolution::DetachMember;
    case CreativeWorldLayoutMemberConflictKind::SourceRemovedParent:
      return resolution ==
             CreativeWorldLayoutConflictResolution::DetachMember;
    case CreativeWorldLayoutMemberConflictKind::Count: break;
  }
  return false;
}

CreativeWorldLayoutConflictDecision
makeCreativeWorldLayoutMemberConflictDecision(
    std::string instanceKey,
    const CreativeWorldLayoutRecipeMemberConflict& conflict,
    CreativeWorldLayoutConflictResolution resolution) {
  CreativeWorldLayoutConflictDecision decision;
  decision.instanceKey = std::move(instanceKey);
  decision.resolution = resolution;
  decision.memberStableKey = conflict.stableKey;
  decision.memberConflictKind = conflict.kind;
  decision.objectId = conflict.objectId;
  decision.baselineFingerprint = conflict.baselineFingerprint;
  decision.currentFingerprint = conflict.currentFingerprint;
  decision.desiredFingerprint = conflict.desiredFingerprint;
  return decision;
}

CreativeWorldLayoutReconciliationResult reconcileCreativeWorldLayoutRecipes(
    const CreativeWorldLayoutReconciliationRequest& request) {
  CreativeWorldLayoutReconciliationResult result;
  if (request.document == nullptr || !request.document->isValid() ||
      request.layoutTag.empty()) {
    result.reasonCode = "creative_world_layout_reconciliation_invalid";
    return result;
  }
  ConflictDecisionLookup decisions;
  if (!buildConflictDecisionLookup(request.conflictDecisions, decisions)) {
    result.reasonCode =
        "creative_world_layout_conflict_decisions_invalid";
    return result;
  }

  std::map<std::string, std::vector<const CreativeObject*>> existingGroups;
  std::vector<const CreativeObject*> unidentified;
  for (const CreativeObject& object : request.document->objects()) {
    if (!hasTag(object.tags, request.layoutTag)) {
      continue;
    }
    const std::string_view instanceKey =
        creativeRecipeObjectInstanceKey(object);
    if (instanceKey.empty()) {
      unidentified.push_back(&object);
    } else {
      existingGroups[std::string(instanceKey)].push_back(&object);
    }
  }

  for (std::size_t desiredIndex = 0U;
       desiredIndex < request.desiredRecipes.size(); ++desiredIndex) {
    const CreativeRecipePlan& desired = request.desiredRecipes[desiredIndex];
    auto existing = existingGroups.find(desired.instanceKey);
    if (existing == existingGroups.end()) {
      result.applyRecipeIndices.push_back(desiredIndex);
      ++result.createRecipeCount;
      result.changes.push_back(makeChange(
          CreativeWorldLayoutRecipeChangeKind::Add, desired.kind,
          desired.instanceKey, desiredIndex, 0U, desired.objects.size(), {},
          memberCounts(desired.objects.size())));
      continue;
    }

    RecipePatchAnalysis patch = analyzeRecipePatch(
        *request.document, existing->second, desired);
    if (patch.groupConflict) {
      const CreativeWorldLayoutConflictDecision* groupDecision =
          groupDecisionFor(decisions, desired.instanceKey);
      appendConflictResolution(
          result,
          groupDecision != nullptr
              ? groupDecision->resolution
              : CreativeWorldLayoutConflictResolution::Block,
          existing->second, &desired, desiredIndex, patch.summary,
          desired.instanceKey);
    } else if (!patch.memberConflicts.empty()) {
      const CreativeWorldLayoutConflictDecision* groupDecision =
          groupDecisionFor(decisions, desired.instanceKey);
      if (groupDecision != nullptr) {
        appendConflictResolution(
            result, groupDecision->resolution, existing->second, &desired,
            desiredIndex, patch.summary, desired.instanceKey,
            patch.memberConflicts);
        existingGroups.erase(existing);
        continue;
      }

      ++result.conflictRecipeCount;
      bool unresolved = false;
      std::size_t explicitRemoveCount = 0U;
      std::size_t explicitDetachCount = 0U;
      appendIds(result.removeObjectIds, patch.removeObjects);
      for (std::size_t conflictIndex = 0U;
           conflictIndex < patch.memberConflicts.size(); ++conflictIndex) {
        const CreativeWorldLayoutRecipeMemberConflict& conflict =
            patch.memberConflicts[conflictIndex];
        const CreativeWorldLayoutConflictDecision* decision =
            memberDecisionFor(decisions, desired.instanceKey, conflict);
        if (decision == nullptr) {
          unresolved = true;
          continue;
        }
        switch (decision->resolution) {
          case CreativeWorldLayoutConflictResolution::UseSource:
            patch.memberActions[patch.conflictDesiredIndices[conflictIndex]] =
                CreativeWorldLayoutRecipeMemberAction::Update;
            break;
          case CreativeWorldLayoutConflictResolution::KeepRefinement:
            patch.memberActions[patch.conflictDesiredIndices[conflictIndex]] =
                CreativeWorldLayoutRecipeMemberAction::Preserve;
            break;
          case CreativeWorldLayoutConflictResolution::RemoveMember:
            result.removeObjectIds.push_back(
                patch.conflictObjects[conflictIndex]->id);
            ++explicitRemoveCount;
            break;
          case CreativeWorldLayoutConflictResolution::DetachMember:
            result.detachObjectIds.push_back(
                patch.conflictObjects[conflictIndex]->id);
            ++explicitDetachCount;
            break;
          case CreativeWorldLayoutConflictResolution::Block:
          case CreativeWorldLayoutConflictResolution::Regenerate:
          case CreativeWorldLayoutConflictResolution::Detach:
          case CreativeWorldLayoutConflictResolution::Count:
            unresolved = true;
            break;
        }
      }
      if (unresolved) {
        result.blocked = true;
        result.changes.push_back(makeChange(
            CreativeWorldLayoutRecipeChangeKind::Conflict, desired.kind,
            desired.instanceKey, desiredIndex, existing->second.size(),
            desired.objects.size(), patch.summary, {},
            patch.memberConflicts));
      } else {
        const CreativeWorldLayoutRecipeMemberCounts counts =
            memberCountsForPatch(
                patch.memberActions,
                patch.removeObjects.size() + explicitRemoveCount,
                explicitDetachCount);
        CreativeWorldLayoutRecipePatchDecision patchDecision;
        patchDecision.desiredRecipeIndex = desiredIndex;
        patchDecision.existingObjectIds =
            std::move(patch.existingObjectIds);
        patchDecision.memberActions = std::move(patch.memberActions);
        result.patchDecisions.push_back(std::move(patchDecision));
        ++result.patchRecipeCount;
        result.changes.push_back(makeChange(
            CreativeWorldLayoutRecipeChangeKind::Patch, desired.kind,
            desired.instanceKey, desiredIndex, existing->second.size(),
            desired.objects.size(), patch.summary, counts,
            patch.memberConflicts));
      }
    } else if (patch.requiresPatch) {
      const CreativeWorldLayoutRecipeMemberCounts counts =
          memberCountsForPatch(patch.memberActions,
                               patch.removeObjects.size());
      appendIds(result.removeObjectIds, patch.removeObjects);
      CreativeWorldLayoutRecipePatchDecision decision;
      decision.desiredRecipeIndex = desiredIndex;
      decision.existingObjectIds = std::move(patch.existingObjectIds);
      decision.memberActions = std::move(patch.memberActions);
      result.patchDecisions.push_back(std::move(decision));
      ++result.patchRecipeCount;
      result.changes.push_back(makeChange(
          CreativeWorldLayoutRecipeChangeKind::Patch, desired.kind,
          desired.instanceKey, desiredIndex, existing->second.size(),
          desired.objects.size(), patch.summary, counts));
    } else if (patch.summary.refinedObjectCount > 0U) {
      ++result.refinedRecipeCount;
      result.changes.push_back(makeChange(
          CreativeWorldLayoutRecipeChangeKind::Refined, desired.kind,
          desired.instanceKey, desiredIndex, existing->second.size(),
          desired.objects.size(), patch.summary,
          memberCounts(0U, desired.objects.size())));
    } else {
      ++result.keepRecipeCount;
      result.changes.push_back(makeChange(
          CreativeWorldLayoutRecipeChangeKind::Keep, desired.kind,
          desired.instanceKey, desiredIndex, existing->second.size(),
          desired.objects.size(), patch.summary,
          memberCounts(0U, desired.objects.size())));
    }
    existingGroups.erase(existing);
  }

  for (const auto& [instanceKey, objects] : existingGroups) {
    RecipeRemovalAnalysis removal =
        analyzeRecipeRemoval(*request.document, objects);
    if (removal.groupConflict) {
      const CreativeWorldLayoutConflictDecision* groupDecision =
          groupDecisionFor(decisions, instanceKey);
      appendConflictResolution(
          result,
          groupDecision != nullptr
              ? groupDecision->resolution
              : CreativeWorldLayoutConflictResolution::Block,
          objects, nullptr, kInvalidCreativeWorldLayoutRecipeIndex,
          removal.summary, instanceKey);
      continue;
    }
    if (removal.memberConflicts.empty()) {
      appendIds(result.removeObjectIds, removal.removeObjects);
      ++result.removeRecipeCount;
      result.changes.push_back(makeChange(
          CreativeWorldLayoutRecipeChangeKind::Remove,
          CreativeRecipeKind::Unknown, instanceKey,
          kInvalidCreativeWorldLayoutRecipeIndex, objects.size(), 0U,
          removal.summary,
          memberCounts(0U, 0U, 0U, removal.removeObjects.size())));
      continue;
    }

    const CreativeWorldLayoutConflictDecision* groupDecision =
        groupDecisionFor(decisions, instanceKey);
    if (groupDecision != nullptr) {
      appendConflictResolution(
          result, groupDecision->resolution, objects, nullptr,
          kInvalidCreativeWorldLayoutRecipeIndex, removal.summary,
          instanceKey, removal.memberConflicts);
      continue;
    }

    ++result.conflictRecipeCount;
    bool unresolved = false;
    std::size_t explicitRemoveCount = 0U;
    std::size_t explicitDetachCount = 0U;
    appendIds(result.removeObjectIds, removal.removeObjects);
    for (std::size_t conflictIndex = 0U;
         conflictIndex < removal.memberConflicts.size(); ++conflictIndex) {
      const CreativeWorldLayoutRecipeMemberConflict& conflict =
          removal.memberConflicts[conflictIndex];
      const CreativeWorldLayoutConflictDecision* decision =
          memberDecisionFor(decisions, instanceKey, conflict);
      if (decision == nullptr) {
        unresolved = true;
        continue;
      }
      if (decision->resolution ==
          CreativeWorldLayoutConflictResolution::RemoveMember) {
        result.removeObjectIds.push_back(
            removal.conflictObjects[conflictIndex]->id);
        ++explicitRemoveCount;
      } else if (decision->resolution ==
                 CreativeWorldLayoutConflictResolution::DetachMember) {
        result.detachObjectIds.push_back(
            removal.conflictObjects[conflictIndex]->id);
        ++explicitDetachCount;
      } else {
        unresolved = true;
      }
    }
    if (unresolved) {
      result.blocked = true;
      result.changes.push_back(makeChange(
          CreativeWorldLayoutRecipeChangeKind::Conflict,
          CreativeRecipeKind::Unknown, instanceKey,
          kInvalidCreativeWorldLayoutRecipeIndex, objects.size(), 0U,
          removal.summary, {}, removal.memberConflicts));
      continue;
    }

    const std::size_t removeCount =
        removal.removeObjects.size() + explicitRemoveCount;
    if (removeCount > 0U) {
      ++result.removeRecipeCount;
    }
    if (explicitDetachCount > 0U) {
      ++result.detachRecipeCount;
    }
    result.changes.push_back(makeChange(
        explicitDetachCount > 0U
            ? CreativeWorldLayoutRecipeChangeKind::Detach
            : CreativeWorldLayoutRecipeChangeKind::Remove,
        CreativeRecipeKind::Unknown, instanceKey,
        kInvalidCreativeWorldLayoutRecipeIndex, objects.size(), 0U,
        removal.summary,
        memberCounts(0U, 0U, 0U, removeCount, explicitDetachCount),
        removal.memberConflicts));
  }

  if (!unidentified.empty()) {
    ExistingGroupAnalysis analysis;
    analysis.refinedObjectCount = unidentified.size();
    analysis.missingBaselineCount = unidentified.size();
    const CreativeWorldLayoutConflictDecision* groupDecision =
        groupDecisionFor(decisions, "unidentified_managed_output");
    appendConflictResolution(
        result,
        groupDecision != nullptr
            ? groupDecision->resolution
            : CreativeWorldLayoutConflictResolution::Block,
        unidentified, nullptr,
        kInvalidCreativeWorldLayoutRecipeIndex, analysis,
        "unidentified_managed_output");
  }

  if (!allConflictDecisionsConsumed(decisions)) {
    clearExecutableOperations(result);
    result.blocked = false;
    result.reasonCode =
        "creative_world_layout_conflict_decision_stale";
    return result;
  }

  result.accepted = !result.blocked;
  if (result.blocked) {
    clearExecutableOperations(result);
  }
  result.reasonCode = result.blocked
                          ? "creative_world_layout_refinement_conflict"
                          : "creative_world_layout_reconciliation_ready";
  return result;
}

bool detachCreativeWorldLayoutObject(CreativeObject& object,
                                     std::string_view layoutTag) {
  const std::size_t before = object.tags.size();
  std::erase_if(object.tags, [&](const std::string& tag) {
    return tag == layoutTag || isCreativeRecipeManagementTag(tag) ||
           tag.starts_with(kWorldLayoutSourcePrefix) ||
           tag.starts_with(kWorldLayoutRoomEdgePrefix);
  });
  return object.tags.size() != before;
}

}  // namespace iggy3d::creative
