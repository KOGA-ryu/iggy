#include "EditorWorldLayoutReviewPanel.hpp"

#include "EditorWorldLayoutReviewModel.hpp"

#include "app/iggy3d/creative/input/Catalog.hpp"

#include <cstddef>
#include <string>
#include <string_view>
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

void enqueuePlannedCommand(CreativeDesktopCommandFrame& commands,
                           CreativeDesktopCommand command) {
  commands.enqueue(command.id, std::move(command.payload));
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
    enqueuePlannedCommand(
        commands, planCreativeEditorWorldLayoutDiagnosticFocusCommand(issue));
  }
  if (hasRepair) {
    if (navigable) {
      ImGui::SameLine();
    }
    ImGui::BeginDisabled(repairDisabled || !issue.buildingRepairAvailable);
    if (ImGui::SmallButton(
            buildingRepairLabel(issue.buildingRepairOperation))) {
      enqueuePlannedCommand(
          commands, planCreativeEditorWorldLayoutBuildingRepairCommand(issue));
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
    const CreativeEditorWorldLayoutAssetRepairProjection projection =
        projectCreativeEditorWorldLayoutAssetRepair(issue, state, catalog);
    ImGui::TextDisabled("Current: %s", issue.assetId.c_str());
    if (projection.refreshBoundsAvailable) {
      if (ImGui::Button("Refresh Bounds")) {
        enqueuePlannedCommand(
            commands, planCreativeEditorWorldLayoutAssetRepairCommand(
                          issue,
                          CreativeDesktopWorldLayoutAssetRepairOperation::
                              RefreshBounds));
        ImGui::CloseCurrentPopup();
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "Adopt current catalog dimensions; preserve placement and fit");
      }
    }
    if (projection.proceduralInsertAvailable) {
      if (projection.refreshBoundsAvailable) {
        ImGui::SameLine();
      }
      if (ImGui::Button("Use Procedural")) {
        enqueuePlannedCommand(
            commands, planCreativeEditorWorldLayoutAssetRepairCommand(
                          issue,
                          CreativeDesktopWorldLayoutAssetRepairOperation::
                              UseProceduralInsert));
        ImGui::CloseCurrentPopup();
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Replace the catalog insert; preserve the opening");
      }
    }

    ImGui::BeginDisabled(projection.compatibleReplacementCount == 0U);
    if (ImGui::BeginCombo("Replace Asset", "Choose compatible asset")) {
      for (const cr::CreativeCatalogEntry& entry : catalog.entries) {
        if (!creativeEditorWorldLayoutAssetRepairReplacementCompatible(
                issue, state, entry)) {
          continue;
        }
        const std::string_view assetId =
            cr::creativeHotbarAssetId(entry.hotbarEntry);
        ImGui::PushID(entry.hotbarEntry.assetId.data());
        if (ImGui::Selectable(entry.label.c_str())) {
          enqueuePlannedCommand(
              commands, planCreativeEditorWorldLayoutAssetRepairCommand(
                            issue,
                            CreativeDesktopWorldLayoutAssetRepairOperation::
                                ReplaceAsset,
                            std::string(assetId)));
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
    if (projection.compatibleReplacementCount == 0U) {
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
  const CreativeEditorWorldLayoutRecipeChangeProjection projection =
      projectCreativeEditorWorldLayoutRecipeChanges(diagnostics);
  if (projection.visibleChangeCount == 0U ||
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
    if (!creativeEditorWorldLayoutRecipeChangeVisible(change.kind)) {
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
          static_cast<unsigned long long>(change.memberCounts.createCount),
          static_cast<unsigned long long>(change.memberCounts.preserveCount),
          static_cast<unsigned long long>(change.memberCounts.updateCount),
          static_cast<unsigned long long>(change.memberCounts.removeCount),
          static_cast<unsigned long long>(change.memberCounts.detachCount));
    }
  }
  ImGui::EndTable();
}

void drawRefinementConflictActions(
    CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutDiagnosticReport& diagnostics,
    CreativeDesktopCommandFrame& commands, bool editingDisabled) {
  const CreativeEditorWorldLayoutConflictReviewProjection reviewProjection =
      projectCreativeEditorWorldLayoutConflictReview(state.conflictReview,
                                                     diagnostics);
  if (!reviewProjection.objectConflicts &&
      !reviewProjection.terrainConflicts) {
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
  if (reviewProjection.objectConflicts &&
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
      const CreativeEditorWorldLayoutConflictDecisionProjection projection =
          projectCreativeEditorWorldLayoutConflictDecision(diagnostics,
                                                           decision);
      if (projection.groupDecision) {
        ImGui::TextWrapped("%s", decision.instanceKey.c_str());
        ImGui::TextDisabled("Group provenance requires recovery");
      } else {
        ImGui::TextWrapped(
            "%s",
            projection.conflict != nullptr &&
                    !projection.conflict->objectName.empty()
                ? projection.conflict->objectName.c_str()
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
      ImGui::BeginDisabled(!projection.sourceResolutionAvailable);
      if (ImGui::RadioButton(
              projection.groupDecision
                  ? "Regenerate"
                  : projection.concurrent ? "Use 2D" : "Remove",
              decision.resolution == projection.sourceResolution)) {
        decision.resolution = projection.sourceResolution;
      }
      if (ImGui::IsItemHovered()) {
        if (projection.groupDecision) {
          ImGui::SetTooltip(
              "Replace this managed group to match the layout");
        } else if (projection.concurrent) {
          ImGui::SetTooltip(
              "Apply the 2D source state at the existing object id");
        } else if (projection.sourceResolutionAvailable) {
          ImGui::SetTooltip(
              "Remove this object; incident logic links are removed too");
        }
      }
      ImGui::EndDisabled();
      ImGui::TableNextColumn();
      if (ImGui::RadioButton(
              projection.groupDecision
                  ? "Detach"
                  : projection.concurrent ? "Keep 3D" : "Detach",
              decision.resolution == projection.outputResolution)) {
        decision.resolution = projection.outputResolution;
      }
      if (ImGui::IsItemHovered()) {
        if (projection.groupDecision) {
          ImGui::SetTooltip(
              "Keep this group as authored output and create fresh managed output");
        } else if (projection.concurrent) {
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
  if (reviewProjection.terrainConflicts) {
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
            findCreativeEditorWorldLayoutTerrainConflict(diagnostics,
                                                         decision);
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
          enqueuePlannedCommand(
              commands,
              planCreativeEditorWorldLayoutTerrainDetachCommand(*conflict));
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
  ImGui::BeginDisabled(!reviewProjection.allResolved);
  if (ImGui::Button("Apply Resolutions")) {
    enqueuePlannedCommand(
        commands, planCreativeEditorWorldLayoutConflictConfirmCommand(
                      state.conflictReview));
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

void drawCreativeEditorWorldLayoutReview(
    CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutDiagnosticReport& diagnostics,
    const cr::CreativeCatalogState& catalog,
    CreativeDesktopCommandFrame& commands, bool editingDisabled,
    bool repairDisabled) {
  for (std::size_t issueIndex = 0U; issueIndex < diagnostics.issueCount;
       ++issueIndex) {
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
    drawWorldLayoutDiagnosticActions(issue, commands, repairDisabled,
                                     issueIndex);
    drawWorldLayoutAssetRepair(issue, state, catalog, commands,
                               editingDisabled, issueIndex);
  }
  drawRefinementConflictActions(state, diagnostics, commands,
                                editingDisabled);
  drawRecipeChanges(diagnostics);
}

}  // namespace iggy3d_creative_app
