#include "EditorWorldLayoutReviewModel.hpp"

#include "EditorWorldLayoutSources.hpp"

#include "app/iggy3d/creative/input/Catalog.hpp"

#include <algorithm>
#include <string_view>
#include <utility>

namespace iggy3d_creative_app {
namespace {

[[nodiscard]] const cr::CreativeWorldLayoutRecipeMemberConflict*
findReviewedMemberConflict(
    const CreativeEditorWorldLayoutDiagnosticReport& diagnostics,
    const cr::CreativeWorldLayoutConflictDecision& decision) noexcept {
  for (const cr::CreativeWorldLayoutRecipeChange& change :
       diagnostics.recipeChanges) {
    if (change.instanceKey != decision.instanceKey) {
      continue;
    }
    const auto found = std::find_if(
        change.memberConflicts.begin(), change.memberConflicts.end(),
        [&](const cr::CreativeWorldLayoutRecipeMemberConflict& conflict) {
          return conflict.stableKey == decision.memberStableKey &&
                 conflict.objectId == decision.objectId;
        });
    if (found != change.memberConflicts.end()) {
      return &*found;
    }
  }
  return nullptr;
}

}  // namespace

bool creativeEditorWorldLayoutAssetRepairReplacementCompatible(
    const CreativeEditorWorldLayoutDiagnostic& issue,
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeCatalogEntry& entry) noexcept {
  const cr::CreativeBoundsMetrics bounds =
      cr::measureCreativeBounds(entry.hotbarEntry.assetSourceBounds);
  if (entry.category != cr::CreativeCatalogEntryCategory::Asset ||
      !entry.hotbarEntry.hasAssetBounds || !bounds.valid ||
      !cr::isPositiveCreativeVec3(bounds.size) ||
      cr::creativeHotbarAssetId(entry.hotbarEntry) == issue.assetId) {
    return false;
  }
  if (issue.table == cr::CreativeWorldLayoutTable::Opening) {
    return issue.index < state.source.openings.size() &&
           creativeEditorWorldLayoutCatalogAssetMatchesOpening(
               entry.assetAuthoringMetadata.categoryId,
               state.source.openings[issue.index].kind);
  }
  if (issue.table == cr::CreativeWorldLayoutTable::Object) {
    return issue.index < state.source.objects.size() &&
           !creativeEditorWorldLayoutCatalogAssetIsHostedOpening(
               entry.assetAuthoringMetadata.categoryId) &&
           entry.hotbarEntry.objectKind ==
               state.source.objects[issue.index].kind;
  }
  return false;
}

CreativeEditorWorldLayoutAssetRepairProjection
projectCreativeEditorWorldLayoutAssetRepair(
    const CreativeEditorWorldLayoutDiagnostic& issue,
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeCatalogState& catalog) {
  CreativeEditorWorldLayoutAssetRepairProjection projection;
  projection.visible =
      issue.assetIssue != CreativeEditorWorldLayoutAssetIssue::None;
  if (!projection.visible) {
    return projection;
  }
  projection.refreshBoundsAvailable =
      issue.assetIssue == CreativeEditorWorldLayoutAssetIssue::StaleBounds;
  projection.proceduralInsertAvailable =
      issue.table == cr::CreativeWorldLayoutTable::Opening;
  for (const cr::CreativeCatalogEntry& entry : catalog.entries) {
    if (creativeEditorWorldLayoutAssetRepairReplacementCompatible(
            issue, state, entry)) {
      ++projection.compatibleReplacementCount;
    }
  }
  return projection;
}

bool creativeEditorWorldLayoutRecipeChangeVisible(
    cr::CreativeWorldLayoutRecipeChangeKind kind) noexcept {
  return kind != cr::CreativeWorldLayoutRecipeChangeKind::Keep;
}

CreativeEditorWorldLayoutRecipeChangeProjection
projectCreativeEditorWorldLayoutRecipeChanges(
    const CreativeEditorWorldLayoutDiagnosticReport& diagnostics) {
  CreativeEditorWorldLayoutRecipeChangeProjection projection;
  for (const cr::CreativeWorldLayoutRecipeChange& change :
       diagnostics.recipeChanges) {
    if (creativeEditorWorldLayoutRecipeChangeVisible(change.kind)) {
      ++projection.visibleChangeCount;
    }
  }
  return projection;
}

CreativeEditorWorldLayoutConflictReviewSynchronization
planCreativeEditorWorldLayoutConflictReviewSynchronization(
    const CreativeEditorWorldLayoutConflictReviewState& current,
    std::uint64_t diagnosticBuildCount,
    const CreativeEditorWorldLayoutDiagnosticReport& diagnostics) {
  CreativeEditorWorldLayoutConflictReviewSynchronization synchronization;
  if (current.diagnosticBuildCount == diagnosticBuildCount) {
    return synchronization;
  }

  synchronization.replace = true;
  synchronization.review.diagnosticBuildCount = diagnosticBuildCount;
  for (const cr::CreativeWorldLayoutRecipeChange& change :
       diagnostics.recipeChanges) {
    if (change.kind != cr::CreativeWorldLayoutRecipeChangeKind::Conflict) {
      continue;
    }
    if (change.memberConflicts.empty()) {
      synchronization.review.decisions.push_back(
          {change.instanceKey,
           cr::CreativeWorldLayoutConflictResolution::Block});
      continue;
    }
    for (const cr::CreativeWorldLayoutRecipeMemberConflict& conflict :
         change.memberConflicts) {
      synchronization.review.decisions.push_back(
          cr::makeCreativeWorldLayoutMemberConflictDecision(change.instanceKey,
                                                            conflict));
    }
  }
  for (const cr::CreativeWorldLayoutTerrainConflict& conflict :
       diagnostics.terrainReconciliation.conflicts) {
    synchronization.review.terrainDecisions.push_back(
        {conflict.generatedTable, conflict.generatedIndex, conflict.stableKey,
         cr::CreativeWorldLayoutTerrainConflictResolution::Block});
  }
  return synchronization;
}

CreativeEditorWorldLayoutConflictDecisionProjection
projectCreativeEditorWorldLayoutConflictDecision(
    const CreativeEditorWorldLayoutDiagnosticReport& diagnostics,
    const cr::CreativeWorldLayoutConflictDecision& decision) noexcept {
  CreativeEditorWorldLayoutConflictDecisionProjection projection;
  projection.conflict = findReviewedMemberConflict(diagnostics, decision);
  projection.groupDecision = decision.memberStableKey.empty();
  projection.concurrent =
      decision.memberConflictKind ==
      cr::CreativeWorldLayoutMemberConflictKind::ConcurrentEdit;
  projection.sourceResolutionAvailable =
      projection.groupDecision || projection.concurrent ||
      decision.memberConflictKind !=
          cr::CreativeWorldLayoutMemberConflictKind::SourceRemovedParent;
  projection.sourceResolution =
      projection.groupDecision
          ? cr::CreativeWorldLayoutConflictResolution::Regenerate
          : projection.concurrent
                ? cr::CreativeWorldLayoutConflictResolution::UseSource
                : cr::CreativeWorldLayoutConflictResolution::RemoveMember;
  projection.outputResolution =
      projection.groupDecision
          ? cr::CreativeWorldLayoutConflictResolution::Detach
          : projection.concurrent
                ? cr::CreativeWorldLayoutConflictResolution::KeepRefinement
                : cr::CreativeWorldLayoutConflictResolution::DetachMember;
  return projection;
}

const cr::CreativeWorldLayoutTerrainConflict*
findCreativeEditorWorldLayoutTerrainConflict(
    const CreativeEditorWorldLayoutDiagnosticReport& diagnostics,
    const cr::CreativeWorldLayoutTerrainConflictDecision& decision) noexcept {
  const auto found = std::find_if(
      diagnostics.terrainReconciliation.conflicts.begin(),
      diagnostics.terrainReconciliation.conflicts.end(),
      [&](const cr::CreativeWorldLayoutTerrainConflict& conflict) {
        return conflict.generatedTable == decision.table &&
               conflict.generatedIndex == decision.generatedIndex &&
               conflict.stableKey == decision.stableKey;
      });
  return found == diagnostics.terrainReconciliation.conflicts.end()
             ? nullptr
             : &*found;
}

CreativeEditorWorldLayoutConflictReviewProjection
projectCreativeEditorWorldLayoutConflictReview(
    const CreativeEditorWorldLayoutConflictReviewState& review,
    const CreativeEditorWorldLayoutDiagnosticReport& diagnostics) noexcept {
  CreativeEditorWorldLayoutConflictReviewProjection projection;
  projection.objectConflicts =
      diagnostics.compileReceipt.status ==
      cr::CreativeWorldLayoutStatus::RefinementConflict;
  projection.terrainConflicts = diagnostics.terrainReconciliation.blocked;
  projection.objectResolved =
      !projection.objectConflicts ||
      (!review.decisions.empty() &&
       std::all_of(
           review.decisions.begin(), review.decisions.end(),
           [](const cr::CreativeWorldLayoutConflictDecision& decision) {
             if (decision.memberStableKey.empty()) {
               return decision.resolution ==
                          cr::CreativeWorldLayoutConflictResolution::Regenerate ||
                      decision.resolution ==
                          cr::CreativeWorldLayoutConflictResolution::Detach;
             }
             return cr::creativeWorldLayoutMemberResolutionAllowed(
                 decision.memberConflictKind, decision.resolution);
           }));
  projection.terrainResolved =
      !projection.terrainConflicts ||
      (!review.terrainDecisions.empty() &&
       std::all_of(
           review.terrainDecisions.begin(), review.terrainDecisions.end(),
           [](const cr::CreativeWorldLayoutTerrainConflictDecision& decision) {
             return decision.resolution ==
                    cr::CreativeWorldLayoutTerrainConflictResolution::
                        Regenerate;
           }));
  projection.allResolved =
      projection.objectResolved && projection.terrainResolved;
  return projection;
}

CreativeDesktopCommand planCreativeEditorWorldLayoutDiagnosticFocusCommand(
    const CreativeEditorWorldLayoutDiagnostic& issue) {
  return {CreativeDesktopCommandId::WorldLayoutFocusSource,
          CreativeDesktopWorldLayoutSourcePayload{
              issue.table, issue.index, issue.stableKey}};
}

CreativeDesktopCommand planCreativeEditorWorldLayoutBuildingRepairCommand(
    const CreativeEditorWorldLayoutDiagnostic& issue) {
  return {CreativeDesktopCommandId::WorldLayoutRepairBuildingUsability,
          CreativeDesktopWorldLayoutBuildingRepairPayload{
              issue.buildingUsabilityIssue, issue.stableKey}};
}

CreativeDesktopCommand planCreativeEditorWorldLayoutAssetRepairCommand(
    const CreativeEditorWorldLayoutDiagnostic& issue,
    CreativeDesktopWorldLayoutAssetRepairOperation operation,
    std::string replacementAssetId) {
  return {CreativeDesktopCommandId::WorldLayoutRepairAsset,
          CreativeDesktopWorldLayoutAssetRepairPayload{
              operation, issue.table, issue.index, issue.stableKey,
              issue.assetId, std::move(replacementAssetId)}};
}

CreativeDesktopCommand planCreativeEditorWorldLayoutTerrainDetachCommand(
    const cr::CreativeWorldLayoutTerrainConflict& conflict) {
  return {CreativeDesktopCommandId::WorldLayoutDeleteSource,
          CreativeDesktopWorldLayoutSourcePayload{
              conflict.desiredTable, conflict.desiredIndex,
              conflict.stableKey}};
}

CreativeDesktopCommand planCreativeEditorWorldLayoutConflictConfirmCommand(
    const CreativeEditorWorldLayoutConflictReviewState& review) {
  return {CreativeDesktopCommandId::WorldLayoutConfirm,
          CreativeDesktopWorldLayoutConfirmPayload{review.decisions,
                                                   review.terrainDecisions}};
}

}  // namespace iggy3d_creative_app
