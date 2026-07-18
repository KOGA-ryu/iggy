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
  bool conflict = false;
  bool requiresPatch = false;
  std::vector<CreativeObjectId> existingObjectIds;
  std::vector<CreativeWorldLayoutRecipeMemberAction> memberActions;
  std::vector<const CreativeObject*> removeObjects;
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
    std::size_t removeCount) noexcept {
  CreativeWorldLayoutRecipeMemberCounts counts;
  counts.removeCount = removeCount;
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
  CreativeWorldLayoutConflictResolution resolution =
      CreativeWorldLayoutConflictResolution::Block;
  std::size_t requestIndex = 0U;
};

struct ConflictDecisionLookup {
  std::map<std::string_view, ConflictDecisionEntry> entries;
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

[[nodiscard]] bool hasRemovalDependency(
    const CreativeDocument& document,
    CreativeObjectId objectId,
    const std::unordered_set<CreativeObjectId>& groupObjectIds) noexcept {
  const bool hasExternalChild = std::any_of(
      document.objects().begin(), document.objects().end(),
      [&](const CreativeObject& candidate) {
        return candidate.parentId == objectId &&
               !groupObjectIds.contains(candidate.id);
      });
  if (hasExternalChild) {
    return true;
  }
  return std::any_of(
      document.logicLinks().begin(), document.logicLinks().end(),
      [&](const CreativeLogicLink& link) {
        return link.sourceObjectId == objectId ||
               link.targetObjectId == objectId;
      });
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
      analysis.conflict = true;
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
        analysis.conflict = true;
      }
      continue;
    }

    const CreativeObject& object = *found->second;
    analysis.existingObjectIds[desiredIndex] = object.id;
    if (!creativeRecipeObjectHasInstanceProvenance(
            object, desired.kind, desired.instanceKey, desiredObject.role,
            desiredObject.stableKey) ||
        desiredFingerprint == 0U) {
      analysis.conflict = true;
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
        analysis.conflict = true;
      }
      analysis.requiresPatch = true;
    } else if (current != baseline) {
      if (desiredFingerprint != baseline) {
        analysis.conflict = true;
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
    if (baseline == 0U || !matchesStoredBaseline(document, *object) ||
        hasRemovalDependency(document, object->id, groupObjectIds)) {
      analysis.conflict = true;
      ++analysis.summary.refinedObjectCount;
    } else {
      analysis.removeObjects.push_back(object);
      analysis.requiresPatch = true;
    }
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
        decision.resolution ==
            CreativeWorldLayoutConflictResolution::Block ||
        decision.resolution >= CreativeWorldLayoutConflictResolution::Count ||
        !output.entries
             .emplace(std::string_view(decision.instanceKey),
                      ConflictDecisionEntry{decision.resolution, index})
             .second) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] CreativeWorldLayoutConflictResolution conflictResolutionFor(
    ConflictDecisionLookup& decisions,
    std::string_view instanceKey) noexcept {
  const auto found = decisions.entries.find(instanceKey);
  if (found == decisions.entries.end()) {
    return CreativeWorldLayoutConflictResolution::Block;
  }
  decisions.consumed[found->second.requestIndex] = true;
  return found->second.resolution;
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
    CreativeWorldLayoutRecipeMemberCounts counts = {}) {
  return {kind,
          recipeKind,
          std::move(instanceKey),
          desiredRecipeIndex,
          existingObjectCount,
          desiredObjectCount,
          counts,
          analysis.refinedObjectCount,
          analysis.missingBaselineCount};
}

void appendConflictResolution(
    CreativeWorldLayoutReconciliationResult& result,
    CreativeWorldLayoutConflictResolution resolution,
    std::span<const CreativeObject* const> existing,
    const CreativeRecipePlan* desired,
    std::size_t desiredIndex,
    ExistingGroupAnalysis analysis,
    std::string instanceKey) {
  ++result.conflictRecipeCount;
  if (resolution == CreativeWorldLayoutConflictResolution::Block) {
    result.blocked = true;
    result.changes.push_back(makeChange(
        CreativeWorldLayoutRecipeChangeKind::Conflict,
        desired != nullptr ? desired->kind : CreativeRecipeKind::Unknown,
        std::move(instanceKey), desiredIndex, existing.size(),
        desired != nullptr ? desired->objects.size() : 0U, analysis));
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
                       existing.size())));
    } else {
      ++result.removeRecipeCount;
      result.changes.push_back(makeChange(
          CreativeWorldLayoutRecipeChangeKind::Remove,
          CreativeRecipeKind::Unknown, std::move(instanceKey), desiredIndex,
          existing.size(), 0U, analysis,
          memberCounts(0U, 0U, 0U, existing.size())));
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
                     existing.size())));
  } else {
    result.changes.push_back(makeChange(
        CreativeWorldLayoutRecipeChangeKind::Detach,
        CreativeRecipeKind::Unknown, std::move(instanceKey), desiredIndex,
        existing.size(), 0U, analysis,
        memberCounts(0U, 0U, 0U, 0U, existing.size())));
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
    case CreativeWorldLayoutConflictResolution::Count: break;
  }
  return "Unknown";
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
    if (patch.conflict) {
      appendConflictResolution(
          result,
          conflictResolutionFor(decisions, desired.instanceKey),
          existing->second, &desired, desiredIndex, patch.summary,
          desired.instanceKey);
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
    ExistingGroupAnalysis analysis;
    analysis.allMatchStoredBaseline = !objects.empty();
    std::unordered_set<CreativeObjectId> groupObjectIds;
    groupObjectIds.reserve(objects.size());
    for (const CreativeObject* object : objects) {
      groupObjectIds.insert(object->id);
    }
    for (const CreativeObject* object : objects) {
      const std::uint64_t baseline =
          creativeRecipeObjectOutputFingerprint(*object);
      if (baseline == 0U) {
        ++analysis.missingBaselineCount;
      }
      if (baseline == 0U ||
          !matchesStoredBaseline(*request.document, *object) ||
          hasRemovalDependency(*request.document, object->id,
                               groupObjectIds)) {
        analysis.allMatchStoredBaseline = false;
        ++analysis.refinedObjectCount;
      }
    }
    if (analysis.allMatchStoredBaseline) {
      appendIds(result.removeObjectIds, objects);
      ++result.removeRecipeCount;
      result.changes.push_back(makeChange(
          CreativeWorldLayoutRecipeChangeKind::Remove,
          CreativeRecipeKind::Unknown, instanceKey,
          kInvalidCreativeWorldLayoutRecipeIndex, objects.size(), 0U,
          analysis, memberCounts(0U, 0U, 0U, objects.size())));
    } else {
      appendConflictResolution(
          result, conflictResolutionFor(decisions, instanceKey), objects,
          nullptr,
          kInvalidCreativeWorldLayoutRecipeIndex, analysis, instanceKey);
    }
  }

  if (!unidentified.empty()) {
    ExistingGroupAnalysis analysis;
    analysis.refinedObjectCount = unidentified.size();
    analysis.missingBaselineCount = unidentified.size();
    appendConflictResolution(
        result,
        conflictResolutionFor(decisions, "unidentified_managed_output"),
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
