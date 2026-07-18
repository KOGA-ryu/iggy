#include "EditorDesktopCommands.hpp"
#include "EditorState.hpp"
#include "EditorWorldLayoutDiagnostics.hpp"
#include "app/iggy3d/creative/document/DocumentMutation.hpp"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace app = iggy3d_creative_app;
namespace cr = iggy3d::creative;

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(float lhs, float rhs) {
  return std::fabs(lhs - rhs) <= 1.0e-4F;
}

cr::CreativeAppState makeApp(std::string name, cr::CreativeDocumentId id) {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create(std::move(name));
  static_cast<void>(document.assignId(id));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  return appState;
}

app::CreativeEditorWorldLayoutState roomLayout() {
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "diagnostic_layout");
  const app::CreativeEditorWorldLayoutEditReceipt created =
      app::createCreativeEditorWorldLayoutBuildingShell(
          state, {{{10, 20}, {18, 26}}, 0.0, 4U, 0.25, 1U});
  if (!created.accepted) {
    std::cerr << "FAIL: diagnostic fixture building shell\n";
  }
  return state;
}

bool preflightCacheTracksBothTruthRevisions() {
  cr::CreativeAppState first = makeApp("Diagnostic First", 9201U);
  cr::CreativeAppState second = makeApp("Diagnostic Second", 9202U);
  app::CreativeEditorWorldLayoutState state = roomLayout();

  const auto& initial = app::refreshCreativeEditorWorldLayoutDiagnostics(
      state.diagnosticCache, first.facade.document(), state.source,
      state.revision);
  const bool initialReady = initial.ready && initial.issueCount == 0U;
  static_cast<void>(app::refreshCreativeEditorWorldLayoutDiagnostics(
      state.diagnosticCache, first.facade.document(), state.source,
      state.revision));
  const std::uint64_t afterIdle = state.diagnosticCache.buildCount;

  state.source.rooms[0].name.clear();
  ++state.revision;
  const auto& invalid = app::refreshCreativeEditorWorldLayoutDiagnostics(
      state.diagnosticCache, first.facade.document(), state.source,
      state.revision);
  const bool exactFailure =
      !invalid.ready && invalid.issueCount == 1U &&
      invalid.issues[0].table == cr::CreativeWorldLayoutTable::Room &&
      invalid.issues[0].index == 0U &&
      invalid.issues[0].status == cr::CreativeWorldLayoutStatus::InvalidSymbol;
  const std::uint64_t afterLayoutEdit = state.diagnosticCache.buildCount;

  static_cast<void>(app::refreshCreativeEditorWorldLayoutDiagnostics(
      state.diagnosticCache, second.facade.document(), state.source,
      state.revision));
  const std::uint64_t afterDocumentChange = state.diagnosticCache.buildCount;

  return expect(initialReady, "valid layout preflight is ready") &&
         expect(afterIdle == 1U,
                "idle frames reuse one authoritative compile") &&
         expect(exactFailure,
                "compiler failure table and index survive projection") &&
         expect(afterLayoutEdit == 2U,
                "layout revision invalidates preflight once") &&
         expect(afterDocumentChange == 3U,
                "document identity invalidates preflight once");
}

bool missingOpeningAssetWarnsAndTracksCatalogMembership() {
  cr::CreativeAppState live = makeApp("Diagnostic Asset", 9204U);
  app::CreativeEditorWorldLayoutState state = roomLayout();
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Door));
  const app::CreativeEditorWorldLayoutEditReceipt placed =
      app::applyCreativeEditorWorldLayoutPoint(state, {14.0, 20.1});
  cr::CreativeWorldLayoutOpening& opening = state.source.openings[0];
  opening.includeInsert = true;
  opening.insertAssetId = "homestead/modular/door_leaf_1p1x2p2";
  opening.insertAssetSourceBoundsMeters =
      {{-0.2, 0.0, -0.05}, {0.9, 2.2, 0.15}};
  opening.hasInsertAssetSourceBounds = true;
  ++state.revision;

  cr::CreativeCatalogState catalog;
  const auto& missing = app::refreshCreativeEditorWorldLayoutDiagnostics(
      state.diagnosticCache, live.facade.document(), state.source,
      state.revision, &catalog);
  const bool warningProjected =
      placed.accepted && placed.changed && missing.ready &&
      missing.issueCount == 1U &&
      missing.issues[0].severity ==
          app::CreativeEditorWorldLayoutDiagnosticSeverity::Warning &&
      missing.issues[0].table == cr::CreativeWorldLayoutTable::Opening &&
      missing.issues[0].index == 0U &&
      missing.issues[0].assetIssue ==
          app::CreativeEditorWorldLayoutAssetIssue::Missing &&
      missing.issues[0].stableKey == opening.stableKey &&
      missing.issues[0].assetId == opening.insertAssetId &&
      missing.issues[0].reasonCode ==
          "creative_world_layout_opening_asset_missing";
  static_cast<void>(app::refreshCreativeEditorWorldLayoutDiagnostics(
      state.diagnosticCache, live.facade.document(), state.source,
      state.revision, &catalog));
  const std::uint64_t afterIdle = state.diagnosticCache.buildCount;

  cr::CreativeCatalogEntry entry;
  entry.category = cr::CreativeCatalogEntryCategory::Asset;
  entry.label = "Asymmetric Door";
  entry.assetAuthoringMetadata.categoryId = "door";
  entry.hotbarEntry.objectKind = cr::CreativeObjectKind::Door;
  static_cast<void>(cr::setCreativeHotbarAsset(
      entry.hotbarEntry, opening.insertAssetId,
      opening.insertAssetSourceBoundsMeters));
  catalog.entries.push_back(std::move(entry));
  const auto& resolved = app::refreshCreativeEditorWorldLayoutDiagnostics(
      state.diagnosticCache, live.facade.document(), state.source,
      state.revision, &catalog);
  const bool catalogResolved = resolved.ready && resolved.issueCount == 0U;
  const std::uint64_t afterCatalog = state.diagnosticCache.buildCount;

  catalog.entries[0].hotbarEntry.assetSourceBounds.max.x += 0.1;
  const auto& stale = app::refreshCreativeEditorWorldLayoutDiagnostics(
      state.diagnosticCache, live.facade.document(), state.source,
      state.revision, &catalog);
  const bool staleWarningProjected =
      stale.ready && stale.issueCount == 1U &&
      stale.issues[0].table == cr::CreativeWorldLayoutTable::Opening &&
      stale.issues[0].index == 0U &&
      stale.issues[0].assetIssue ==
          app::CreativeEditorWorldLayoutAssetIssue::StaleBounds &&
      stale.issues[0].reasonCode ==
          "creative_world_layout_opening_asset_bounds_stale";
  const std::uint64_t afterBoundsChange = state.diagnosticCache.buildCount;

  catalog.entries.clear();
  state.source.openings[0].includeInsert = false;
  ++state.revision;
  const auto& dormant = app::refreshCreativeEditorWorldLayoutDiagnostics(
      state.diagnosticCache, live.facade.document(), state.source,
      state.revision, &catalog);

  return expect(warningProjected,
                "missing opening asset is ready with a navigable warning") &&
         expect(afterIdle == 1U,
                "unchanged missing-asset diagnostics reuse the cache") &&
         expect(catalogResolved && afterCatalog == 2U,
                "catalog membership invalidates and clears the warning") &&
         expect(staleWarningProjected && afterBoundsChange == 3U,
                "same-id catalog bounds invalidate and warn once") &&
         expect(dormant.ready && dormant.issueCount == 0U &&
                    state.diagnosticCache.buildCount == 4U,
                "disabled inserts retain dormant identity without warning");
}

bool missingObjectAssetWarningIsNavigable() {
  cr::CreativeAppState live = makeApp("Diagnostic Object Asset", 9205U);
  app::CreativeEditorWorldLayoutState state = roomLayout();
  cr::CreativeWorldLayoutObject object;
  object.kind = cr::CreativeObjectKind::Prop;
  object.mode = cr::CreativeObjectLibraryPlacementMode::Point;
  object.stableKey = "prop.missing";
  object.name = "Missing Prop";
  object.assetId = "homestead/interior/missing_prop";
  object.pointCells = {12.0, 0.0, 22.0};
  object.assetSourceBoundsMeters =
      {{-0.5, 0.0, -0.5}, {0.5, 1.0, 0.5}};
  object.hasAssetSourceBounds = true;
  object.tags = {"world_layout:catalog_asset"};
  state.source.objects.push_back(std::move(object));
  ++state.revision;

  cr::CreativeCatalogState catalog;
  const auto& report = app::refreshCreativeEditorWorldLayoutDiagnostics(
      state.diagnosticCache, live.facade.document(), state.source,
      state.revision, &catalog);
  return expect(report.ready && report.issueCount == 1U,
                "missing object asset remains compile-ready with warning") &&
         expect(report.issues[0].severity ==
                        app::CreativeEditorWorldLayoutDiagnosticSeverity::Warning &&
                    report.issues[0].table ==
                        cr::CreativeWorldLayoutTable::Object &&
                    report.issues[0].index == 0U &&
                    report.issues[0].assetIssue ==
                        app::CreativeEditorWorldLayoutAssetIssue::Missing &&
                    report.issues[0].stableKey == "prop.missing" &&
                    report.issues[0].assetId ==
                        "homestead/interior/missing_prop" &&
                    report.issues[0].reasonCode ==
                        "creative_world_layout_object_asset_missing",
                "missing object asset warning identifies its source row");
}

bool diagnosticFocusSelectsFramesAndPreservesSource() {
  app::CreativeEditorWorldLayoutState state = roomLayout();
  state.canvasPixelsPerCell = 10.0F;
  state.elevationPixelsPerCell = 12.0F;
  state.elevationAxis = app::CreativeEditorWorldLayoutElevationAxis::X;
  const std::uint64_t sourceRevision = state.revision;

  const app::CreativeEditorWorldLayoutEditReceipt room =
      app::focusCreativeEditorWorldLayoutSource(
          state, cr::CreativeWorldLayoutTable::Room, 0U);
  const bool roomFocused =
      room.accepted &&
      state.selection.kind ==
          app::CreativeEditorWorldLayoutSelectionKind::Room &&
      state.selection.index == 0U && state.activeLevelIndex == 0U &&
      near(state.canvasPanX, -140.0F) && near(state.canvasPanZ, -230.0F) &&
      near(state.elevationPanHorizontal, -168.0F);

  const app::CreativeEditorWorldLayoutEditReceipt level =
      app::focusCreativeEditorWorldLayoutSource(
          state, cr::CreativeWorldLayoutTable::Level, 0U);
  const bool levelFocused =
      level.accepted &&
      state.selection.kind ==
          app::CreativeEditorWorldLayoutSelectionKind::Level &&
      state.selection.index == 0U && state.activeLevelIndex == 0U;

  const app::CreativeEditorWorldLayoutEditReceipt invalid =
      app::focusCreativeEditorWorldLayoutSource(
          state, cr::CreativeWorldLayoutTable::Room, 99U);
  return expect(roomFocused,
                "room issue selects its source and centers both views") &&
         expect(levelFocused,
                "level issue selects the editable level source") &&
         expect(!invalid.accepted,
                "out-of-range diagnostic target is rejected") &&
         expect(state.revision == sourceRevision,
                "diagnostic navigation never mutates source truth");
}

bool diagnosticFocusRoutesThroughTypedDispatcher() {
  cr::CreativeAppState live = makeApp("Diagnostic Command", 9203U);
  app::CreativeEditorState editor;
  editor.worldLayout = roomLayout();
  editor.worldLayout.tool = app::CreativeEditorWorldLayoutTool::Room;
  editor.worldLayout.selection = {};
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{
      live, editor, std::filesystem::path{}, &saveId};
  app::CreativeDesktopCommandFrame frame;
  frame.push(app::CreativeDesktopCommandId::WorldLayoutFocusSource,
             app::CreativeDesktopWorldLayoutSourcePayload{
                 cr::CreativeWorldLayoutTable::Room, 0U, {}});
  const app::CreativeDesktopCommandResult result =
      app::dispatchCreativeDesktopCommands(frame, context);

  return expect(result.accepted && result.changed,
                "typed diagnostic command is accepted") &&
         expect(!result.worldLayoutChanged && !result.sceneChanged,
                "diagnostic command changes only editor view state") &&
         expect(editor.worldLayout.selection.kind ==
                        app::CreativeEditorWorldLayoutSelectionKind::Room &&
                    editor.worldLayout.selection.index == 0U,
                "dispatcher focuses the requested source symbol");
}

bool stableIdPatchProjectsHonestMemberCounts() {
  cr::CreativeAppState live = makeApp("Diagnostic Patch", 9207U);
  app::CreativeEditorWorldLayoutState state = roomLayout();
  const cr::CreativeWorldLayoutCompileResult initial =
      cr::buildCreativeWorldLayoutPlan(live.facade.document(), state.source);
  const cr::CreativeWorldLayoutApplyReceipt applied =
      cr::applyCreativeWorldLayoutPlan(live.facade, initial.plan);
  state.source.rooms[0].footprint.maximum.x += 1;
  ++state.revision;

  const app::CreativeEditorWorldLayoutDiagnosticReport report =
      app::buildCreativeEditorWorldLayoutDiagnosticReport(
          live.facade.document(), state.source);
  if (report.recipeChanges.size() != 1U) {
    return expect(false, "stable-id patch diagnostic has one managed group");
  }
  const cr::CreativeWorldLayoutRecipeChange& change =
      report.recipeChanges.front();
  const std::uint64_t accountedMembers =
      change.memberCounts.createCount + change.memberCounts.preserveCount +
      change.memberCounts.updateCount + change.memberCounts.removeCount;

  return expect(initial.receipt.accepted && applied.accepted,
                "stable-id patch diagnostic fixture generated") &&
         expect(report.ready && report.hasChanges &&
                    report.compileReceipt.objectRecipePatchCount == 1U &&
                    report.compileReceipt.objectRecipeReplaceCount == 0U,
                "preflight separates safe patch from destructive replace") &&
         expect(change.kind ==
                        cr::CreativeWorldLayoutRecipeChangeKind::Patch &&
                    change.memberCounts.createCount == 0U &&
                    change.memberCounts.updateCount > 0U &&
                    change.memberCounts.removeCount == 0U &&
                    accountedMembers == change.desiredObjectCount,
                "preflight accounts for every patched recipe member");
}

bool refinementConflictProjectsExactManagedGroupAndSource() {
  cr::CreativeAppState live = makeApp("Diagnostic Refinement", 9206U);
  app::CreativeEditorWorldLayoutState state = roomLayout();
  const cr::CreativeWorldLayoutCompileResult initial =
      cr::buildCreativeWorldLayoutPlan(live.facade.document(), state.source);
  const cr::CreativeWorldLayoutApplyReceipt applied =
      cr::applyCreativeWorldLayoutPlan(live.facade, initial.plan);
  if (!applied.accepted || live.facade.document().objects().empty()) {
    return expect(false, "refinement diagnostic fixture generated");
  }
  const cr::CreativeObjectId refinedId =
      live.facade.document().objects().front().id;
  const cr::CreativeDocumentMutationReceipt refined = cr::moveDocumentObject(
      live.facade.documentForPersistence(), refinedId, {4.0, 2.0, 3.0});
  ++state.source.rooms[0].footprint.maximum.x;
  ++state.revision;

  const app::CreativeEditorWorldLayoutDiagnosticReport report =
      app::buildCreativeEditorWorldLayoutDiagnosticReport(
          live.facade.document(), state.source);
  return expect(refined.changed && !report.ready && !report.hasChanges,
                "refined output blocks ordinary generation preflight") &&
         expect(report.compileReceipt.status ==
                        cr::CreativeWorldLayoutStatus::RefinementConflict &&
                    report.compileReceipt.objectRecipeConflictCount == 1U &&
                    report.recipeChanges.size() == 1U &&
                    report.recipeChanges[0].kind ==
                        cr::CreativeWorldLayoutRecipeChangeKind::Conflict,
                "diagnostic report retains the three-way change record") &&
         expect(report.issueCount == 1U &&
                    report.issues[0].severity ==
                        app::CreativeEditorWorldLayoutDiagnosticSeverity::Error &&
                    report.issues[0].table ==
                        cr::CreativeWorldLayoutTable::Building &&
                    report.issues[0].index == 0U &&
                    report.issues[0].reasonCode ==
                        "creative_world_layout_refinement_conflict",
                "conflict points back to its building source row");
}

}  // namespace

int main() {
  const bool ok = preflightCacheTracksBothTruthRevisions() &&
                  missingOpeningAssetWarnsAndTracksCatalogMembership() &&
                  missingObjectAssetWarningIsNavigable() &&
                  diagnosticFocusSelectsFramesAndPreservesSource() &&
                  diagnosticFocusRoutesThroughTypedDispatcher() &&
                  stableIdPatchProjectsHonestMemberCounts() &&
                  refinementConflictProjectsExactManagedGroupAndSource();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
