#include "EditorWorldLayoutReviewModel.hpp"

#include "app/iggy3d/creative/input/Catalog.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace {

namespace app = iggy3d_creative_app;
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeCatalogEntry assetEntry(std::string label, std::string assetId,
                                    std::string categoryId,
                                    cr::CreativeObjectKind kind) {
  cr::CreativeCatalogEntry entry;
  entry.category = cr::CreativeCatalogEntryCategory::Asset;
  entry.label = std::move(label);
  entry.assetAuthoringMetadata.categoryId = std::move(categoryId);
  entry.hotbarEntry.objectKind = kind;
  static_cast<void>(cr::setCreativeHotbarAsset(
      entry.hotbarEntry, assetId,
      {{-0.5, 0.0, -0.25}, {0.5, 2.0, 0.25}}));
  return entry;
}

app::CreativeEditorWorldLayoutDiagnosticReport conflictDiagnostics() {
  app::CreativeEditorWorldLayoutDiagnosticReport diagnostics;
  diagnostics.compileReceipt.status =
      cr::CreativeWorldLayoutStatus::RefinementConflict;
  diagnostics.compileReceipt.objectRecipeConflictCount = 2U;

  cr::CreativeWorldLayoutRecipeChange groupConflict;
  groupConflict.kind = cr::CreativeWorldLayoutRecipeChangeKind::Conflict;
  groupConflict.instanceKey = "group_conflict";
  diagnostics.recipeChanges.push_back(std::move(groupConflict));

  cr::CreativeWorldLayoutRecipeChange memberConflict;
  memberConflict.kind = cr::CreativeWorldLayoutRecipeChangeKind::Conflict;
  memberConflict.instanceKey = "member_conflict";
  memberConflict.memberConflicts.push_back(
      {cr::CreativeWorldLayoutMemberConflictKind::ConcurrentEdit,
       "member_a", "Door A", 17U, 1U, 2U, 3U});
  diagnostics.recipeChanges.push_back(std::move(memberConflict));

  diagnostics.terrainReconciliation.blocked = true;
  diagnostics.terrainReconciliation.conflicts.push_back(
      {cr::CreativeWorldLayoutTable::TerrainPath, 4U, "terrain_path",
       cr::CreativeWorldLayoutTerrainImpactStatus::Drifted,
       true, cr::CreativeWorldLayoutTable::TerrainPath, 2U, true});
  return diagnostics;
}

bool assetRepairProjectionFiltersCompatibleReplacements() {
  app::CreativeEditorWorldLayoutState state;
  cr::CreativeWorldLayoutOpening opening;
  opening.kind = cr::CreativeBuildingOpeningKind::Door;
  state.source.openings.push_back(opening);
  cr::CreativeWorldLayoutObject object;
  object.kind = cr::CreativeObjectKind::Prop;
  object.stableKey = "prop";
  object.name = "Prop";
  state.source.objects.push_back(object);

  cr::CreativeCatalogState catalog;
  catalog.entries.push_back(
      assetEntry("Current Door", "door_current", "door",
                 cr::CreativeObjectKind::Prop));
  catalog.entries.push_back(
      assetEntry("Replacement Door", "door_replacement", "door",
                 cr::CreativeObjectKind::Prop));
  catalog.entries.push_back(
      assetEntry("Window", "window_replacement", "window",
                 cr::CreativeObjectKind::Prop));
  catalog.entries.push_back(
      assetEntry("Prop", "prop_replacement", "furniture",
                 cr::CreativeObjectKind::Prop));

  app::CreativeEditorWorldLayoutDiagnostic openingIssue;
  openingIssue.assetIssue =
      app::CreativeEditorWorldLayoutAssetIssue::StaleBounds;
  openingIssue.table = cr::CreativeWorldLayoutTable::Opening;
  openingIssue.index = 0U;
  openingIssue.assetId = "door_current";
  const auto openingProjection =
      app::projectCreativeEditorWorldLayoutAssetRepair(openingIssue, state,
                                                      catalog);

  app::CreativeEditorWorldLayoutDiagnostic objectIssue;
  objectIssue.assetIssue = app::CreativeEditorWorldLayoutAssetIssue::Missing;
  objectIssue.table = cr::CreativeWorldLayoutTable::Object;
  objectIssue.index = 0U;
  objectIssue.assetId = "missing_prop";
  const auto objectProjection =
      app::projectCreativeEditorWorldLayoutAssetRepair(objectIssue, state,
                                                      catalog);

  return expect(openingProjection.visible &&
                    openingProjection.refreshBoundsAvailable &&
                    openingProjection.proceduralInsertAvailable,
                "stale opening exposes refresh and procedural repair") &&
         expect(openingProjection.compatibleReplacementCount == 1U &&
                    app::creativeEditorWorldLayoutAssetRepairReplacementCompatible(
                        openingIssue, state, catalog.entries[1]) &&
                    !app::creativeEditorWorldLayoutAssetRepairReplacementCompatible(
                        openingIssue, state, catalog.entries[0]) &&
                    !app::creativeEditorWorldLayoutAssetRepairReplacementCompatible(
                        openingIssue, state, catalog.entries[2]),
                "opening replacement filters current and incompatible assets") &&
         expect(objectProjection.visible &&
                    !objectProjection.refreshBoundsAvailable &&
                    !objectProjection.proceduralInsertAvailable,
                "missing object exposes only replacement repair") &&
         expect(objectProjection.compatibleReplacementCount == 1U &&
                    app::creativeEditorWorldLayoutAssetRepairReplacementCompatible(
                        objectIssue, state, catalog.entries[3]),
                "object replacement preserves kind and hosted-opening policy");
}

bool recipeProjectionSuppressesKeepRows() {
  app::CreativeEditorWorldLayoutDiagnosticReport diagnostics;
  cr::CreativeWorldLayoutRecipeChange keep;
  keep.kind = cr::CreativeWorldLayoutRecipeChangeKind::Keep;
  diagnostics.recipeChanges.push_back(keep);
  cr::CreativeWorldLayoutRecipeChange patch;
  patch.kind = cr::CreativeWorldLayoutRecipeChangeKind::Patch;
  diagnostics.recipeChanges.push_back(patch);
  cr::CreativeWorldLayoutRecipeChange conflict;
  conflict.kind = cr::CreativeWorldLayoutRecipeChangeKind::Conflict;
  diagnostics.recipeChanges.push_back(conflict);
  const auto projection =
      app::projectCreativeEditorWorldLayoutRecipeChanges(diagnostics);
  return expect(projection.visibleChangeCount == 2U &&
                    !app::creativeEditorWorldLayoutRecipeChangeVisible(
                        cr::CreativeWorldLayoutRecipeChangeKind::Keep) &&
                    app::creativeEditorWorldLayoutRecipeChangeVisible(
                        cr::CreativeWorldLayoutRecipeChangeKind::Patch) &&
                    app::creativeEditorWorldLayoutRecipeChangeVisible(
                        cr::CreativeWorldLayoutRecipeChangeKind::Conflict),
                "recipe projection retains every non-Keep row in order");
}

bool conflictSynchronizationReplacesOnlyStaleReview() {
  const auto diagnostics = conflictDiagnostics();
  app::CreativeEditorWorldLayoutConflictReviewState current;
  current.diagnosticBuildCount = 7U;
  current.decisions.push_back(
      {"stale_group", cr::CreativeWorldLayoutConflictResolution::Detach});

  const auto unchanged =
      app::planCreativeEditorWorldLayoutConflictReviewSynchronization(
          current, 7U, diagnostics);
  const auto replaced =
      app::planCreativeEditorWorldLayoutConflictReviewSynchronization(
          current, 8U, diagnostics);

  return expect(!unchanged.replace,
                "matching diagnostic generation preserves user choices") &&
         expect(replaced.replace &&
                    replaced.review.diagnosticBuildCount == 8U &&
                    replaced.review.decisions.size() == 2U &&
                    replaced.review.decisions[0].instanceKey ==
                        "group_conflict" &&
                    replaced.review.decisions[1].memberStableKey ==
                        "member_a" &&
                    replaced.review.terrainDecisions.size() == 1U,
                "new diagnostics replace stale selection with exact conflicts") &&
         expect(replaced.review.decisions[0].resolution ==
                        cr::CreativeWorldLayoutConflictResolution::Block &&
                    replaced.review.terrainDecisions[0].resolution ==
                        cr::CreativeWorldLayoutTerrainConflictResolution::Block,
                "fresh conflict choices fail closed");
}

bool reviewProjectionPinsStaleAndResolvedDecisions() {
  const auto diagnostics = conflictDiagnostics();
  const auto synchronization =
      app::planCreativeEditorWorldLayoutConflictReviewSynchronization(
          {}, 1U, diagnostics);
  app::CreativeEditorWorldLayoutConflictReviewState review =
      synchronization.review;

  const auto blocked =
      app::projectCreativeEditorWorldLayoutConflictReview(review, diagnostics);
  const auto concurrent =
      app::projectCreativeEditorWorldLayoutConflictDecision(
          diagnostics, review.decisions[1]);

  cr::CreativeWorldLayoutConflictDecision stale = review.decisions[1];
  stale.memberStableKey = "missing_member";
  const auto staleProjection =
      app::projectCreativeEditorWorldLayoutConflictDecision(diagnostics, stale);

  review.decisions[0].resolution =
      cr::CreativeWorldLayoutConflictResolution::Regenerate;
  review.decisions[1].resolution =
      cr::CreativeWorldLayoutConflictResolution::KeepRefinement;
  review.terrainDecisions[0].resolution =
      cr::CreativeWorldLayoutTerrainConflictResolution::Regenerate;
  const auto resolved =
      app::projectCreativeEditorWorldLayoutConflictReview(review, diagnostics);

  return expect(blocked.objectConflicts && blocked.terrainConflicts &&
                    !blocked.allResolved,
                "blocked choices cannot apply") &&
         expect(concurrent.conflict != nullptr && concurrent.concurrent &&
                    concurrent.sourceResolutionAvailable &&
                    concurrent.sourceResolution ==
                        cr::CreativeWorldLayoutConflictResolution::UseSource &&
                    concurrent.outputResolution ==
                        cr::CreativeWorldLayoutConflictResolution::
                            KeepRefinement,
                "concurrent conflict exposes the exact two legal choices") &&
         expect(staleProjection.conflict == nullptr,
                "stale member identity never binds another conflict") &&
         expect(resolved.objectResolved && resolved.terrainResolved &&
                    resolved.allResolved,
                "accepted object and terrain choices enable apply");
}

bool plannedCommandsPreserveIdsAndPayloads() {
  app::CreativeEditorWorldLayoutDiagnostic issue;
  issue.table = cr::CreativeWorldLayoutTable::Opening;
  issue.index = 3U;
  issue.stableKey = "opening_3";
  issue.assetId = "door_old";
  issue.buildingUsabilityIssue.table =
      cr::CreativeWorldLayoutTable::Opening;
  issue.buildingUsabilityIssue.index = 3U;

  const app::CreativeDesktopCommand focus =
      app::planCreativeEditorWorldLayoutDiagnosticFocusCommand(issue);
  const app::CreativeDesktopCommand repair =
      app::planCreativeEditorWorldLayoutAssetRepairCommand(
          issue,
          app::CreativeDesktopWorldLayoutAssetRepairOperation::ReplaceAsset,
          "door_new");
  const auto diagnostics = conflictDiagnostics();
  const auto synchronization =
      app::planCreativeEditorWorldLayoutConflictReviewSynchronization(
          {}, 1U, diagnostics);
  const app::CreativeDesktopCommand confirm =
      app::planCreativeEditorWorldLayoutConflictConfirmCommand(
          synchronization.review);
  const app::CreativeDesktopCommand detach =
      app::planCreativeEditorWorldLayoutTerrainDetachCommand(
          diagnostics.terrainReconciliation.conflicts[0]);

  const auto* focusPayload =
      std::get_if<app::CreativeDesktopWorldLayoutSourcePayload>(
          &focus.payload);
  const auto* repairPayload =
      std::get_if<app::CreativeDesktopWorldLayoutAssetRepairPayload>(
          &repair.payload);
  const auto* confirmPayload =
      std::get_if<app::CreativeDesktopWorldLayoutConfirmPayload>(
          &confirm.payload);
  const auto* detachPayload =
      std::get_if<app::CreativeDesktopWorldLayoutSourcePayload>(
          &detach.payload);
  return expect(
             focus.id ==
                     app::CreativeDesktopCommandId::WorldLayoutFocusSource &&
                 focusPayload != nullptr &&
                 focusPayload->table == cr::CreativeWorldLayoutTable::Opening &&
                 focusPayload->index == 3U &&
                 focusPayload->stableKey == "opening_3",
             "diagnostic focus command preserves stable source identity") &&
         expect(
             repair.id ==
                     app::CreativeDesktopCommandId::WorldLayoutRepairAsset &&
                 repairPayload != nullptr &&
                 repairPayload->operation ==
                     app::CreativeDesktopWorldLayoutAssetRepairOperation::
                         ReplaceAsset &&
                 repairPayload->expectedAssetId == "door_old" &&
                 repairPayload->replacementAssetId == "door_new",
             "asset repair command preserves operation and asset identities") &&
         expect(
             confirm.id == app::CreativeDesktopCommandId::WorldLayoutConfirm &&
                 confirmPayload != nullptr &&
                 confirmPayload->conflictDecisions.size() == 2U &&
                 confirmPayload->terrainConflictDecisions.size() == 1U,
             "confirm command carries the complete review") &&
         expect(
             detach.id ==
                     app::CreativeDesktopCommandId::WorldLayoutDeleteSource &&
                 detachPayload != nullptr &&
                 detachPayload->table ==
                     cr::CreativeWorldLayoutTable::TerrainPath &&
                 detachPayload->index == 2U &&
                 detachPayload->stableKey == "terrain_path",
             "terrain detach targets the desired source identity");
}

bool reviewModelHasNoUiOrBroadEditorDependency() {
  const std::filesystem::path source =
      "apps/iggy3d_creative/EditorWorldLayoutReviewModel.cpp";
  std::ifstream input(source);
  const std::string contents{std::istreambuf_iterator<char>(input),
                             std::istreambuf_iterator<char>()};
  return expect(input.good() || input.eof(),
                "review model source is readable") &&
         expect(contents.find("imgui") == std::string::npos &&
                    contents.find("EditorState.hpp") == std::string::npos &&
                    contents.find("CreativeEditorState") == std::string::npos,
                "review model excludes ImGui and broad editor state");
}

}  // namespace

int main() {
  bool ok = true;
  ok = assetRepairProjectionFiltersCompatibleReplacements() && ok;
  ok = recipeProjectionSuppressesKeepRows() && ok;
  ok = conflictSynchronizationReplacesOnlyStaleReview() && ok;
  ok = reviewProjectionPinsStaleAndResolvedDecisions() && ok;
  ok = plannedCommandsPreserveIdsAndPayloads() && ok;
  ok = reviewModelHasNoUiOrBroadEditorDependency() && ok;
  if (!ok) {
    return EXIT_FAILURE;
  }
  std::cout << "creative editor world layout review model tests passed\n";
  return EXIT_SUCCESS;
}
