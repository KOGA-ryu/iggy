#include "EditorWorldLayoutPanel.hpp"

#include "EditorWorldLayoutPanelInternal.hpp"

#include "EditorDesktopModel.hpp"
#include "EditorDesktopWorldLayoutInspector.hpp"
#include "EditorMeasurement.hpp"
#include "EditorWorldLayoutElevationPanel.hpp"
#include "EditorWorldLayoutHierarchyPanel.hpp"
#include "EditorWorldLayoutTopography.hpp"

#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRoofs.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <string>
#include <utility>

#include "imgui.h"

namespace iggy3d_creative_app {
namespace {

const char* buildingRepairLabel(
    cr::CreativeWorldLayoutBuildingRepairOperation operation) noexcept {
  switch (operation) {
    case cr::CreativeWorldLayoutBuildingRepairOperation::AddExteriorEntrance:
      return "Add entrance";
    case cr::CreativeWorldLayoutBuildingRepairOperation::ConnectRoom:
      return "Connect room";
    case cr::CreativeWorldLayoutBuildingRepairOperation::
        ExpandOpeningClearance:
      return "Fit player clearance";
    case cr::CreativeWorldLayoutBuildingRepairOperation::None:
    case cr::CreativeWorldLayoutBuildingRepairOperation::Count:
      break;
  }
  return "Repair";
}

void drawWorldLayoutDiagnosticActions(
    const CreativeEditorWorldLayoutDiagnostic& issue,
    CreativeDesktopCommandFrame& commands, bool repairDisabled,
    std::size_t issueIndex) {
  const bool navigable =
      issue.table != cr::CreativeWorldLayoutTable::None &&
      issue.index != cr::kInvalidCreativeWorldLayoutIndex;
  const bool hasRepair =
      issue.buildingRepairOperation !=
      cr::CreativeWorldLayoutBuildingRepairOperation::None;
  if (!navigable && !hasRepair) {
    return;
  }

  ImGui::PushID(static_cast<int>(issueIndex));
  ImGui::Indent();
  if (navigable && ImGui::SmallButton("Focus")) {
    commands.enqueue(CreativeDesktopCommandId::WorldLayoutFocusSource,
                  CreativeDesktopWorldLayoutSourcePayload{
                      issue.table, issue.index, issue.stableKey});
  }
  if (hasRepair) {
    if (navigable) {
      ImGui::SameLine();
    }
    ImGui::BeginDisabled(repairDisabled || !issue.buildingRepairAvailable);
    if (ImGui::SmallButton(buildingRepairLabel(
            issue.buildingRepairOperation))) {
      commands.enqueue(
          CreativeDesktopCommandId::WorldLayoutRepairBuildingUsability,
          CreativeDesktopWorldLayoutBuildingRepairPayload{
              issue.buildingUsabilityIssue, issue.stableKey});
    }
    ImGui::EndDisabled();
    if (!issue.buildingRepairAvailable && ImGui::IsItemHovered()) {
      ImGui::SetTooltip(
          "No deterministic repair fits this source; focus and edit it "
          "directly");
    }
  }
  ImGui::Unindent();
  ImGui::PopID();
}

[[nodiscard]] bool worldLayoutRepairAssetCompatible(
    const CreativeEditorWorldLayoutDiagnostic& issue,
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeCatalogEntry& entry) noexcept {
  const cr::CreativeBoundsMetrics bounds =
      cr::measureCreativeBounds(entry.hotbarEntry.assetSourceBounds);
  if (entry.category != cr::CreativeCatalogEntryCategory::Asset ||
      !entry.hotbarEntry.hasAssetBounds || !bounds.valid ||
      !cr::isPositiveCreativeVec3(bounds.size)) {
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

void queueWorldLayoutAssetRepair(
    CreativeDesktopCommandFrame& commands,
    const CreativeEditorWorldLayoutDiagnostic& issue,
    CreativeDesktopWorldLayoutAssetRepairOperation operation,
    std::string replacementAssetId = {}) {
  commands.enqueue(
      CreativeDesktopCommandId::WorldLayoutRepairAsset,
      CreativeDesktopWorldLayoutAssetRepairPayload{
          operation, issue.table, issue.index, issue.stableKey,
          issue.assetId, std::move(replacementAssetId)});
}

void drawWorldLayoutAssetRepair(
    const CreativeEditorWorldLayoutDiagnostic& issue,
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeCatalogState& catalog,
    CreativeDesktopCommandFrame& commands, bool editingDisabled,
    std::size_t issueIndex) {
  if (issue.assetIssue == CreativeEditorWorldLayoutAssetIssue::None) {
    return;
  }

  ImGui::PushID(static_cast<int>(issueIndex));
  ImGui::Indent();
  ImGui::BeginDisabled(editingDisabled);
  if (ImGui::SmallButton("Repair...")) {
    ImGui::OpenPopup("Asset repair");
  }
  ImGui::EndDisabled();
  ImGui::Unindent();

  if (ImGui::BeginPopup("Asset repair")) {
    ImGui::TextDisabled("Current: %s", issue.assetId.c_str());
    if (issue.assetIssue ==
        CreativeEditorWorldLayoutAssetIssue::StaleBounds) {
      if (ImGui::Button("Refresh Bounds")) {
        queueWorldLayoutAssetRepair(
            commands, issue,
            CreativeDesktopWorldLayoutAssetRepairOperation::RefreshBounds);
        ImGui::CloseCurrentPopup();
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "Adopt current catalog dimensions; preserve placement and fit");
      }
    }
    if (issue.table == cr::CreativeWorldLayoutTable::Opening) {
      if (issue.assetIssue ==
          CreativeEditorWorldLayoutAssetIssue::StaleBounds) {
        ImGui::SameLine();
      }
      if (ImGui::Button("Use Procedural")) {
        queueWorldLayoutAssetRepair(
            commands, issue,
            CreativeDesktopWorldLayoutAssetRepairOperation::
                UseProceduralInsert);
        ImGui::CloseCurrentPopup();
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Replace the catalog insert; preserve the opening");
      }
    }

    std::size_t compatibleCount = 0U;
    for (const cr::CreativeCatalogEntry& entry : catalog.entries) {
      if (worldLayoutRepairAssetCompatible(issue, state, entry) &&
          cr::creativeHotbarAssetId(entry.hotbarEntry) != issue.assetId) {
        ++compatibleCount;
      }
    }
    ImGui::BeginDisabled(compatibleCount == 0U);
    if (ImGui::BeginCombo("Replace Asset", "Choose compatible asset")) {
      for (const cr::CreativeCatalogEntry& entry : catalog.entries) {
        const std::string_view assetId =
            cr::creativeHotbarAssetId(entry.hotbarEntry);
        if (!worldLayoutRepairAssetCompatible(issue, state, entry) ||
            assetId == issue.assetId) {
          continue;
        }
        ImGui::PushID(entry.hotbarEntry.assetId.data());
        if (ImGui::Selectable(entry.label.c_str())) {
          queueWorldLayoutAssetRepair(
              commands, issue,
              CreativeDesktopWorldLayoutAssetRepairOperation::ReplaceAsset,
              std::string(assetId));
          ImGui::CloseCurrentPopup();
        }
        if (ImGui::IsItemHovered()) {
          const cr::CreativeBoundsMetrics bounds =
              cr::measureCreativeBounds(entry.hotbarEntry.assetSourceBounds);
          ImGui::SetTooltip("%s\n%.2f x %.2f x %.2f m", assetId.data(),
                            bounds.size.x, bounds.size.y, bounds.size.z);
        }
        ImGui::PopID();
      }
      ImGui::EndCombo();
    }
    ImGui::EndDisabled();
    if (compatibleCount == 0U) {
      ImGui::TextDisabled("No compatible replacement assets");
    }
    ImGui::EndPopup();
  }
  ImGui::PopID();
}

ImVec4 recipeChangeColor(
    cr::CreativeWorldLayoutRecipeChangeKind kind) noexcept {
  switch (kind) {
    case cr::CreativeWorldLayoutRecipeChangeKind::Add:
    case cr::CreativeWorldLayoutRecipeChangeKind::Keep:
      return {0.28F, 0.92F, 0.40F, 1.0F};
    case cr::CreativeWorldLayoutRecipeChangeKind::Refined:
    case cr::CreativeWorldLayoutRecipeChangeKind::Patch:
    case cr::CreativeWorldLayoutRecipeChangeKind::Replace:
      return {1.0F, 0.72F, 0.20F, 1.0F};
    case cr::CreativeWorldLayoutRecipeChangeKind::Remove:
    case cr::CreativeWorldLayoutRecipeChangeKind::Conflict:
      return {1.0F, 0.34F, 0.30F, 1.0F};
    case cr::CreativeWorldLayoutRecipeChangeKind::DetachAndReplace:
    case cr::CreativeWorldLayoutRecipeChangeKind::Detach:
      return {0.38F, 0.72F, 1.0F, 1.0F};
  }
  return {0.75F, 0.75F, 0.75F, 1.0F};
}

void drawRecipeChanges(
    const CreativeEditorWorldLayoutDiagnosticReport& diagnostics) {
  const bool hasVisibleChanges = std::any_of(
      diagnostics.recipeChanges.begin(), diagnostics.recipeChanges.end(),
      [](const cr::CreativeWorldLayoutRecipeChange& change) {
        return change.kind != cr::CreativeWorldLayoutRecipeChangeKind::Keep;
      });
  if (!hasVisibleChanges ||
      !ImGui::CollapsingHeader("Generated output changes",
                               ImGuiTreeNodeFlags_DefaultOpen)) {
    return;
  }
  if (!ImGui::BeginTable("##world_layout_recipe_changes", 3,
                         ImGuiTableFlags_SizingStretchProp |
                             ImGuiTableFlags_BordersInnerH |
                             ImGuiTableFlags_RowBg)) {
    return;
  }
  ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 92.0F);
  ImGui::TableSetupColumn("Managed group", ImGuiTableColumnFlags_WidthStretch);
  ImGui::TableSetupColumn("Members", ImGuiTableColumnFlags_WidthStretch);
  ImGui::TableHeadersRow();
  for (const cr::CreativeWorldLayoutRecipeChange& change :
       diagnostics.recipeChanges) {
    if (change.kind == cr::CreativeWorldLayoutRecipeChangeKind::Keep) {
      continue;
    }
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextColored(recipeChangeColor(change.kind), "%s",
                       cr::toString(change.kind).data());
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(change.instanceKey.c_str());
    ImGui::TableNextColumn();
    if (change.kind == cr::CreativeWorldLayoutRecipeChangeKind::Conflict) {
      if (!change.memberConflicts.empty()) {
        ImGui::Text("blocked: %zu member conflict(s)",
                    change.memberConflicts.size());
      } else {
        ImGui::Text("blocked: %llu refined",
                    static_cast<unsigned long long>(
                        change.refinedObjectCount));
      }
    } else {
      ImGui::TextWrapped(
          "create %llu  keep %llu  update %llu  remove %llu  detach %llu",
          static_cast<unsigned long long>(
              change.memberCounts.createCount),
          static_cast<unsigned long long>(
              change.memberCounts.preserveCount),
          static_cast<unsigned long long>(
              change.memberCounts.updateCount),
          static_cast<unsigned long long>(
              change.memberCounts.removeCount),
          static_cast<unsigned long long>(
              change.memberCounts.detachCount));
    }
  }
  ImGui::EndTable();
}

void synchronizeConflictReview(
    CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutDiagnosticReport& diagnostics) {
  CreativeEditorWorldLayoutConflictReviewState& review =
      state.conflictReview;
  if (review.diagnosticBuildCount == state.diagnosticCache.buildCount) {
    return;
  }
  review.diagnosticBuildCount = state.diagnosticCache.buildCount;
  review.decisions.clear();
  review.terrainDecisions.clear();
  for (const cr::CreativeWorldLayoutRecipeChange& change :
       diagnostics.recipeChanges) {
    if (change.kind == cr::CreativeWorldLayoutRecipeChangeKind::Conflict) {
      if (change.memberConflicts.empty()) {
        review.decisions.push_back(
            {change.instanceKey,
             cr::CreativeWorldLayoutConflictResolution::Block});
        continue;
      }
      for (const cr::CreativeWorldLayoutRecipeMemberConflict& conflict :
           change.memberConflicts) {
        review.decisions.push_back(
            cr::makeCreativeWorldLayoutMemberConflictDecision(
                change.instanceKey, conflict));
      }
    }
  }
  for (const cr::CreativeWorldLayoutTerrainConflict& conflict :
       diagnostics.terrainReconciliation.conflicts) {
    review.terrainDecisions.push_back(
        {conflict.generatedTable, conflict.generatedIndex, conflict.stableKey,
         cr::CreativeWorldLayoutTerrainConflictResolution::Block});
  }
}

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

[[nodiscard]] const cr::CreativeWorldLayoutTerrainConflict*
findReviewedTerrainConflict(
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

void drawRefinementConflictActions(
    CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutDiagnosticReport& diagnostics,
    CreativeDesktopCommandFrame& commands, bool editingDisabled) {
  const bool objectConflicts =
      diagnostics.compileReceipt.status ==
      cr::CreativeWorldLayoutStatus::RefinementConflict;
  const bool terrainConflicts = diagnostics.terrainReconciliation.blocked;
  if (!objectConflicts && !terrainConflicts) {
    return;
  }
  ImGui::BeginDisabled(editingDisabled);
  if (ImGui::Button("Resolve generation conflicts...")) {
    ImGui::OpenPopup("Resolve generation conflicts");
  }
  ImGui::EndDisabled();
  if (!ImGui::BeginPopupModal("Resolve generation conflicts", nullptr,
                              ImGuiWindowFlags_AlwaysAutoResize)) {
    return;
  }
  ImGui::Text("Review conflicts between the 2D layout and refined 3D output.");
  ImGui::TextDisabled(
      "%llu object group(s), %zu terrain source(s) require a decision.",
      static_cast<unsigned long long>(
          diagnostics.compileReceipt.objectRecipeConflictCount),
      diagnostics.terrainReconciliation.conflicts.size());
  ImGui::Separator();
  ImGui::BeginDisabled(editingDisabled);
  if (objectConflicts &&
      ImGui::BeginTable("##world_layout_conflict_review", 3,
                        ImGuiTableFlags_SizingStretchProp |
                            ImGuiTableFlags_BordersInnerH |
                            ImGuiTableFlags_RowBg)) {
    ImGui::TableSetupColumn("Managed output",
                            ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("2D source", ImGuiTableColumnFlags_WidthFixed,
                            104.0F);
    ImGui::TableSetupColumn("3D output", ImGuiTableColumnFlags_WidthFixed,
                            104.0F);
    ImGui::TableHeadersRow();
    for (cr::CreativeWorldLayoutConflictDecision& decision :
         state.conflictReview.decisions) {
      ImGui::PushID(decision.instanceKey.c_str());
      ImGui::PushID(decision.memberStableKey.empty()
                        ? "whole_group"
                        : decision.memberStableKey.c_str());
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      const cr::CreativeWorldLayoutRecipeMemberConflict* conflict =
          findReviewedMemberConflict(diagnostics, decision);
      if (decision.memberStableKey.empty()) {
        ImGui::TextWrapped("%s", decision.instanceKey.c_str());
        ImGui::TextDisabled("Group provenance requires recovery");
      } else {
        ImGui::TextWrapped("%s",
                           conflict != nullptr && !conflict->objectName.empty()
                               ? conflict->objectName.c_str()
                               : decision.memberStableKey.c_str());
        ImGui::TextDisabled("%s / %s", decision.instanceKey.c_str(),
                            decision.memberStableKey.c_str());
        switch (decision.memberConflictKind) {
          case cr::CreativeWorldLayoutMemberConflictKind::ConcurrentEdit:
            ImGui::TextDisabled("Changed in both 2D and 3D");
            break;
          case cr::CreativeWorldLayoutMemberConflictKind::
              SourceRemovedRefinement:
            ImGui::TextDisabled("Removed in 2D, refined in 3D");
            break;
          case cr::CreativeWorldLayoutMemberConflictKind::SourceRemovedLinked:
            ImGui::TextDisabled("Removed in 2D, has logic links");
            break;
          case cr::CreativeWorldLayoutMemberConflictKind::SourceRemovedParent:
            ImGui::TextDisabled("Removed in 2D, has authored children");
            break;
          case cr::CreativeWorldLayoutMemberConflictKind::Count:
            break;
        }
      }
      ImGui::TableNextColumn();
      const bool groupDecision = decision.memberStableKey.empty();
      const bool concurrent =
          decision.memberConflictKind ==
          cr::CreativeWorldLayoutMemberConflictKind::ConcurrentEdit;
      const bool sourceRemoveAllowed =
          decision.memberConflictKind !=
          cr::CreativeWorldLayoutMemberConflictKind::SourceRemovedParent;
      const cr::CreativeWorldLayoutConflictResolution sourceResolution =
          groupDecision
              ? cr::CreativeWorldLayoutConflictResolution::Regenerate
              : concurrent
                    ? cr::CreativeWorldLayoutConflictResolution::UseSource
                    : cr::CreativeWorldLayoutConflictResolution::RemoveMember;
      ImGui::BeginDisabled(!groupDecision && !concurrent &&
                           !sourceRemoveAllowed);
      if (ImGui::RadioButton(
              groupDecision ? "Regenerate"
                            : concurrent ? "Use 2D" : "Remove",
              decision.resolution == sourceResolution)) {
        decision.resolution = sourceResolution;
      }
      if (ImGui::IsItemHovered()) {
        if (groupDecision) {
          ImGui::SetTooltip(
              "Replace this managed group to match the layout");
        } else if (concurrent) {
          ImGui::SetTooltip(
              "Apply the 2D source state at the existing object id");
        } else if (sourceRemoveAllowed) {
          ImGui::SetTooltip(
              "Remove this object; incident logic links are removed too");
        }
      }
      ImGui::EndDisabled();
      ImGui::TableNextColumn();
      const cr::CreativeWorldLayoutConflictResolution outputResolution =
          groupDecision
              ? cr::CreativeWorldLayoutConflictResolution::Detach
              : concurrent
                    ? cr::CreativeWorldLayoutConflictResolution::
                          KeepRefinement
                    : cr::CreativeWorldLayoutConflictResolution::DetachMember;
      if (ImGui::RadioButton(
              groupDecision ? "Detach"
                            : concurrent ? "Keep 3D" : "Detach",
              decision.resolution == outputResolution)) {
        decision.resolution = outputResolution;
      }
      if (ImGui::IsItemHovered()) {
        if (groupDecision) {
          ImGui::SetTooltip(
              "Keep this group as authored output and create fresh managed output");
        } else if (concurrent) {
          ImGui::SetTooltip(
              "Keep the 3D geometry and rebase its generator metadata to the new 2D source");
        } else {
          ImGui::SetTooltip(
              "Keep this object and release generator ownership");
        }
      }
      ImGui::PopID();
      ImGui::PopID();
    }
    ImGui::EndTable();
  }
  if (terrainConflicts) {
    ImGui::SeparatorText("Terrain sources");
    if (ImGui::BeginTable("##world_layout_terrain_conflict_review", 3,
                          ImGuiTableFlags_SizingStretchProp |
                              ImGuiTableFlags_BordersInnerH |
                              ImGuiTableFlags_RowBg)) {
      ImGui::TableSetupColumn("Terrain source",
                              ImGuiTableColumnFlags_WidthStretch);
      ImGui::TableSetupColumn("2D source", ImGuiTableColumnFlags_WidthFixed,
                              112.0F);
      ImGui::TableSetupColumn("3D terrain", ImGuiTableColumnFlags_WidthFixed,
                              152.0F);
      ImGui::TableHeadersRow();
      for (cr::CreativeWorldLayoutTerrainConflictDecision& decision :
           state.conflictReview.terrainDecisions) {
        const cr::CreativeWorldLayoutTerrainConflict* conflict =
            findReviewedTerrainConflict(diagnostics, decision);
        if (conflict == nullptr) {
          continue;
        }
        ImGui::PushID("terrain_conflict");
        ImGui::PushID(decision.stableKey.c_str());
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextWrapped("%s", decision.stableKey.c_str());
        ImGui::TextDisabled("Changed after its last generation");
        ImGui::TableNextColumn();
        if (ImGui::RadioButton(
                "Regenerate",
                decision.resolution ==
                    cr::CreativeWorldLayoutTerrainConflictResolution::
                        Regenerate)) {
          decision.resolution =
              cr::CreativeWorldLayoutTerrainConflictResolution::Regenerate;
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("Reapply the 2D terrain source over the 3D edit");
        }
        ImGui::TableNextColumn();
        ImGui::BeginDisabled(!conflict->canDetachAndKeep3D);
        if (ImGui::Button("Keep 3D & remove source")) {
          commands.enqueue(
              CreativeDesktopCommandId::WorldLayoutDeleteSource,
              CreativeDesktopWorldLayoutSourcePayload{
                  conflict->desiredTable, conflict->desiredIndex,
                  conflict->stableKey});
          ImGui::CloseCurrentPopup();
        }
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
          ImGui::SetTooltip(
              conflict->canDetachAndKeep3D
                  ? "Preserve the current 3D terrain and remove its 2D owner"
                  : "Unavailable: Replace All ownership would clear the 3D terrain");
        }
        ImGui::PopID();
        ImGui::PopID();
      }
      ImGui::EndTable();
    }
  }
  const bool objectResolved =
      !objectConflicts ||
      (!state.conflictReview.decisions.empty() &&
       std::all_of(
           state.conflictReview.decisions.begin(),
           state.conflictReview.decisions.end(),
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
  const bool terrainResolved =
      !terrainConflicts ||
      (!state.conflictReview.terrainDecisions.empty() &&
       std::all_of(
           state.conflictReview.terrainDecisions.begin(),
           state.conflictReview.terrainDecisions.end(),
           [](const cr::CreativeWorldLayoutTerrainConflictDecision& decision) {
             return decision.resolution ==
                    cr::CreativeWorldLayoutTerrainConflictResolution::
                        Regenerate;
           }));
  const bool allResolved = objectResolved && terrainResolved;
  ImGui::BeginDisabled(!allResolved);
  if (ImGui::Button("Apply Resolutions")) {
    commands.enqueue(CreativeDesktopCommandId::WorldLayoutConfirm,
                  CreativeDesktopWorldLayoutConfirmPayload{
                      state.conflictReview.decisions,
                      state.conflictReview.terrainDecisions});
    ImGui::CloseCurrentPopup();
  }
  ImGui::EndDisabled();
  ImGui::EndDisabled();
  ImGui::SameLine();
  if (ImGui::Button("Cancel")) {
    ImGui::CloseCurrentPopup();
  }
  ImGui::EndPopup();
}


}  // namespace

void buildCreativeEditorWorldLayoutPanel(
    CreativeEditorDesktopUiState& desktopUi, CreativeEditorState& editor,
    const cr::CreativeDocument& document,
    const cr::CreativeSelectionState& selection,
    const cr::CreativeMeasurementState& measurement, bool playModeActive,
    const CreativeEditorUiInputFrame& input,
    CreativeDesktopCommandFrame& commands) {
  CreativeEditorWorldLayoutState& state = editor.worldLayout;
  CreativeEditorWorldLayoutTopographyState& topography =
      editor.worldLayoutTopography;
  if (synchronizeCreativeEditorWorldLayoutTerrainRegion(
          topography.region, editor.terrainGeneration, document)) {
    topography.cacheValid = false;
  }
  if (!desktopUi.showWorldLayout) {
    cancelCreativeEditorWorldLayoutPanelManipulation(state, commands);
    if (topography.region.editingEnabled || topography.region.ownsPreview) {
      topography.region.editingEnabled = false;
      commands.enqueue(
          CreativeDesktopCommandId::WorldLayoutTerrainRegionCancel);
    }
    return;
  }

  const bool editingDisabled = playModeActive || editor.assetEdit.active;
  const bool exactPreviewActive =
      creativeEditorWorldLayoutPreviewActive(state);
  const CreativeEditorWorldLayoutDiagnosticReport& diagnostics =
      refreshCreativeEditorWorldLayoutDiagnostics(
          state.diagnosticCache, document, state.source, state.revision,
          &editor.catalog.model, state.sourceEpoch, state.generatedRevision,
          &state.generatedBaseline.source);
  synchronizeConflictReview(state, diagnostics);
  if (editingDisabled) {
    cancelCreativeEditorWorldLayoutPanelManipulation(state, commands);
    if (topography.region.editingEnabled || topography.region.ownsPreview) {
      topography.region.editingEnabled = false;
      commands.enqueue(
          CreativeDesktopCommandId::WorldLayoutTerrainRegionCancel);
    }
  }
  if (state.buildingTransform.active && input.cancelPressed) {
    commands.enqueue(CreativeDesktopCommandId::WorldLayoutTransformBuilding,
                  CreativeDesktopWorldLayoutBuildingTransformPayload{
                      CreativeEditorWorldLayoutBuildingTransformPhase::Cancel,
                      state.buildingTransform.operation});
  }

  // The World Layout workspace reuses the shell's existing dock identities.
  // The ### suffix keeps the persisted ImGui IDs stable while presenting
  // task-specific titles instead of leaving unrelated panels beside a
  // crowded center canvas.
  if (ImGui::Begin("World Layout Tools###Project", nullptr,
                   ImGuiWindowFlags_NoCollapse)) {
    if (ImGui::BeginTabBar("##world_layout_left_tabs")) {
      if (ImGui::BeginTabItem("Source")) {
        drawCreativeEditorWorldLayoutHierarchy(desktopUi, state, commands,
                                               editingDisabled ||
                                                   topography.region
                                                       .editingEnabled);
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Create")) {
        drawCreativeEditorWorldLayoutCreateTools(
            desktopUi, state, editor.catalog.model, document.gridSettings(),
            commands,
            editingDisabled || state.buildingTransform.active ||
                state.buildingTemplatePlacement.active ||
                topography.region.editingEnabled);
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Terrain")) {
        drawCreativeEditorWorldLayoutTerrainTab(
            topography, editor.terrainGeneration, commands,
            editingDisabled || exactPreviewActive ||
                state.buildingTransform.active ||
                state.buildingTemplatePlacement.active);
        ImGui::EndTabItem();
      }
      ImGui::EndTabBar();
    }
  }
  ImGui::End();

  if (ImGui::Begin("World Layout Properties###Inspector", nullptr,
                   ImGuiWindowFlags_NoCollapse)) {
    if (topography.region.editingEnabled) {
      drawCreativeEditorWorldLayoutTerrainRegionProperties(
          topography, editor.terrainGeneration, commands,
          editingDisabled || exactPreviewActive ||
              state.buildingTransform.active ||
              state.buildingTemplatePlacement.active);
    } else {
      ImGui::BeginDisabled(editingDisabled);
      drawCreativeEditorWorldLayoutSourceInspector(state, document, commands);
      drawCreativeEditorWorldLayoutStructureInspector(
          desktopUi, state, document, commands);
      drawCreativeEditorWorldLayoutSelectionProperties(
          state, editor.catalog.model, commands);
      if (state.selection.kind ==
          CreativeEditorWorldLayoutSelectionKind::None) {
        ImGui::TextDisabled("Select a World Layout source to edit it.");
      }
      ImGui::EndDisabled();
    }
  }
  ImGui::End();

  const bool hasSelection =
      state.selection.kind != CreativeEditorWorldLayoutSelectionKind::None;
  const bool buildingSelected =
      state.selection.kind == CreativeEditorWorldLayoutSelectionKind::Building;
  if (ImGui::Begin("World Layout Build###Diagnostics##bottom", nullptr,
                   ImGuiWindowFlags_NoCollapse)) {
    if (topography.region.editingEnabled) {
      drawCreativeEditorWorldLayoutTerrainRegionBuild(
          topography, editor.terrainGeneration, commands, editingDisabled);
    } else {
      ImGui::BeginDisabled(editingDisabled ||
                           state.buildingTransform.active ||
                           state.buildingTemplatePlacement.active);
      ImGui::BeginDisabled(!diagnostics.ready);
      if (ImGui::Button(exactPreviewActive ? "Refresh 3D Preview"
                                           : "Preview 3D")) {
        commands.enqueue(CreativeDesktopCommandId::WorldLayoutPreview);
      }
      ImGui::EndDisabled();
      ImGui::SameLine();
      ImGui::BeginDisabled(!diagnostics.canGenerate);
      if (ImGui::Button(exactPreviewActive ? "Confirm Preview"
                                           : "Confirm & Generate")) {
        commands.enqueue(CreativeDesktopCommandId::WorldLayoutConfirm);
      }
      ImGui::EndDisabled();
      ImGui::SameLine();
      ImGui::BeginDisabled(!hasSelection);
      if (ImGui::Button(buildingSelected ? "Delete building" : "Delete")) {
        if (buildingSelected) {
          ImGui::OpenPopup("Delete building group");
        } else {
          commands.enqueue(CreativeDesktopCommandId::WorldLayoutDeleteSelection);
        }
      }
      ImGui::EndDisabled();
      ImGui::EndDisabled();

      if (ImGui::BeginPopupModal("Delete building group", nullptr,
                                 ImGuiWindowFlags_AlwaysAutoResize)) {
        const char* buildingName =
            buildingSelected &&
                    state.selection.index < state.source.buildings.size()
                ? state.source.buildings[state.selection.index].name.c_str()
                : "selected building";
        ImGui::Text("Delete %s and all owned layout symbols?", buildingName);
        if (ImGui::Button("Delete building")) {
          commands.enqueue(CreativeDesktopCommandId::WorldLayoutDeleteSelection);
          ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
          ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
      }

      ImGui::Separator();
      ImGui::TextColored(
          diagnostics.canGenerate ? ImVec4{0.20F, 1.0F, 0.35F, 1.0F}
                                  : ImVec4{1.0F, 0.34F, 0.30F, 1.0F},
          "%s", diagnostics.canGenerate ? "READY" : "BLOCKED");
      ImGui::SameLine();
      if (diagnostics.ready) {
        ImGui::TextDisabled(
            "%s", !diagnostics.canGenerate
                      ? "Refined terrain requires an explicit decision"
                      : diagnostics.hasChanges
                            ? "Changes are ready to generate"
                            : "Generated output already matches");
        const cr::CreativeWorldLayoutReceipt& generation =
            diagnostics.compileReceipt;
        ImGui::TextDisabled(
            "Recipe groups: +%llu  patch %llu  replace %llu  keep %llu  "
            "refined %llu  |  detach %llu  remove %llu objects",
            static_cast<unsigned long long>(
                generation.objectRecipeCreateCount),
            static_cast<unsigned long long>(
                generation.objectRecipePatchCount),
            static_cast<unsigned long long>(
                generation.objectRecipeReplaceCount),
            static_cast<unsigned long long>(generation.objectRecipeKeepCount),
            static_cast<unsigned long long>(generation.objectRecipeRefinedCount),
            static_cast<unsigned long long>(generation.objectDetachCount),
            static_cast<unsigned long long>(generation.objectRemoveCount));
      }
      for (std::size_t issueIndex = 0U;
           issueIndex < diagnostics.issueCount; ++issueIndex) {
        const CreativeEditorWorldLayoutDiagnostic& issue =
            diagnostics.issues[issueIndex];
        const ImVec4 issueColor =
            issue.severity == CreativeEditorWorldLayoutDiagnosticSeverity::Warning
                ? ImVec4{1.0F, 0.72F, 0.20F, 1.0F}
                : issue.severity ==
                          CreativeEditorWorldLayoutDiagnosticSeverity::Info
                      ? ImVec4{0.38F, 0.72F, 1.0F, 1.0F}
                      : ImVec4{1.0F, 0.34F, 0.30F, 1.0F};
        ImGui::PushStyleColor(ImGuiCol_Text, issueColor);
        ImGui::TextWrapped("%s", issue.message.c_str());
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) {
          if (!issue.kernelReasonCode.empty() &&
              issue.kernelReasonCode !=
                  "creative_world_layout_kernel_not_requested") {
            ImGui::SetTooltip("%s\n%s", issue.reasonCode.c_str(),
                              issue.kernelReasonCode.c_str());
          } else {
            ImGui::SetTooltip("%s", issue.reasonCode.c_str());
          }
        }
        drawWorldLayoutDiagnosticActions(
            issue, commands,
            editingDisabled || state.buildingTransform.active ||
                state.buildingTemplatePlacement.active ||
                topography.region.editingEnabled,
            issueIndex);
        drawWorldLayoutAssetRepair(issue, state, editor.catalog.model, commands,
                                   editingDisabled, issueIndex);
      }
      drawRefinementConflictActions(state, diagnostics, commands,
                                    editingDisabled);
      drawRecipeChanges(diagnostics);

      ImGui::Separator();
      if (ImGui::BeginTable("##world_layout_counts", 4,
                            ImGuiTableFlags_SizingStretchSame |
                                ImGuiTableFlags_BordersInnerV)) {
        ImGui::TableNextColumn();
        ImGui::Text("Buildings  %llu", static_cast<unsigned long long>(
                                          state.source.buildings.size()));
        ImGui::TableNextColumn();
        ImGui::Text("Levels  %llu", static_cast<unsigned long long>(
                                       state.source.levels.size()));
        ImGui::TableNextColumn();
        ImGui::Text("Rooms  %llu", static_cast<unsigned long long>(
                                      state.source.rooms.size()));
        ImGui::TableNextColumn();
        ImGui::Text("Floors  %llu", static_cast<unsigned long long>(
                                       state.source.boxes.size()));
        ImGui::TableNextColumn();
        ImGui::Text("Partitions  %llu", static_cast<unsigned long long>(
                                           state.source.walls.size()));
        ImGui::TableNextColumn();
        ImGui::Text("Openings  %llu", static_cast<unsigned long long>(
                                         state.source.openings.size()));
        ImGui::TableNextColumn();
        ImGui::Text(
            "Terrain  %llu",
            static_cast<unsigned long long>(
                state.source.terrainProfiles.size() +
                state.source.terrainPaths.size()));
        ImGui::TableNextColumn();
        ImGui::Text("Objects  %llu", static_cast<unsigned long long>(
                                        state.source.objects.size()));
        ImGui::EndTable();
      }

      ImGui::TextDisabled("Revision %llu%s",
                          static_cast<unsigned long long>(state.revision),
                          creativeEditorWorldLayoutDirty(state) ? " *" : "");
      ImGui::SameLine();
      ImGui::TextColored(ImVec4{0.32F, 0.95F, 0.43F, 1.0F}, "%s",
                         state.statusMessage.c_str());
    }
  }
  ImGui::End();

  if (ImGui::Begin("World Layout", &desktopUi.showWorldLayout,
                   ImGuiWindowFlags_NoCollapse |
                       ImGuiWindowFlags_NoScrollbar |
                       ImGuiWindowFlags_NoScrollWithMouse)) {
    drawCreativeEditorWorldLayoutViewControls(
        state, editor.worldLayoutTopography, commands);
    ImGui::Separator();
    ImGui::BeginDisabled(editingDisabled);
    const bool canvasInteractionEnabled =
        !editingDisabled && !state.buildingTransform.active;
    drawCreativeEditorWorldLayoutToolOptions(editor, commands);
    ImGui::Separator();
    CreativeEditorWorldLayoutCanvasHoverStatus canvasHover;
    const float statusBarHeight = ImGui::GetFrameHeight();
    if (ImGui::BeginChild("##world_layout_canvas_host",
                          ImVec2{0.0F, -statusBarHeight}, ImGuiChildFlags_None,
                          ImGuiWindowFlags_NoScrollbar |
                              ImGuiWindowFlags_NoScrollWithMouse)) {
      drawCreativeEditorWorldLayoutToolboxStrip(editor, commands);
      ImGui::SameLine();
      if (state.viewMode == CreativeEditorWorldLayoutViewMode::Elevation) {
        drawCreativeEditorWorldLayoutElevationCanvas(
            editor, document.gridSettings(),
            document.measurementAnnotationStore(), measurement, commands,
            canvasInteractionEnabled, input);
      } else {
        drawCreativeEditorWorldLayoutCanvas(
            editor, document, selection, measurement, commands,
            canvasInteractionEnabled, input, &canvasHover);
      }
    }
    ImGui::EndChild();
    ImGui::EndDisabled();
    drawCreativeEditorWorldLayoutStatusBar(editor, canvasHover);
  } else {
    cancelCreativeEditorWorldLayoutPanelManipulation(state, commands);
    if (topography.region.editingEnabled || topography.region.ownsPreview) {
      topography.region.editingEnabled = false;
      commands.enqueue(
          CreativeDesktopCommandId::WorldLayoutTerrainRegionCancel);
    }
  }
  ImGui::End();
}

}  // namespace iggy3d_creative_app
