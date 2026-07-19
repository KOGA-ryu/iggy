#include "EditorDesktopCommands.hpp"
#include "EditorState.hpp"
#include "EditorTerrain.hpp"
#include "EditorWorldLayout.hpp"
#include "EditorWorldLayoutDiagnostics.hpp"
#include "app/iggy3d/creative/document/DocumentMutation.hpp"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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
                        cr::CreativeWorldLayoutRecipeChangeKind::Conflict &&
                    report.recipeChanges[0].memberConflicts.size() == 1U &&
                    report.recipeChanges[0].memberConflicts[0].kind ==
                        cr::CreativeWorldLayoutMemberConflictKind::
                            ConcurrentEdit &&
                    report.recipeChanges[0].memberConflicts[0].objectId ==
                        refinedId,
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

bool terrainImpactCacheOverlayAndFramingShareOneSourcePlan() {
  cr::CreativeAppState live = makeApp("Diagnostic Terrain Impact", 9208U);
  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "terrain_impact_layout");
  cr::CreativeWorldLayoutTerrainProfile profile;
  profile.stableKey = "terrain.plateau";
  profile.kind = cr::CreativeTerrainRecipeKind::Plateau;
  profile.center = {4, 6};
  profile.baseHeightCells = 5U;
  profile.radiusCells = 2U;
  profile.spacingCells = 1U;
  profile.blend = cr::CreativeTerrainProfileBlend::Set;
  profile.rodPolicy = cr::CreativeTerrainProfileRodPolicy::Fill;
  editor.worldLayout.source.terrainProfiles.push_back(profile);
  ++editor.worldLayout.revision;

  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(live.facade.document(),
                                       editor.worldLayout.source);
  const cr::CreativeWorldLayoutApplyReceipt applied =
      cr::applyCreativeWorldLayoutPlan(live.facade, compiled.plan);
  if (!compiled.receipt.accepted || !applied.accepted) {
    return expect(false, "terrain impact editor fixture generates terrain");
  }
  editor.worldLayout.generatedRevision = editor.worldLayout.revision;
  editor.worldLayout.selection = {
      app::CreativeEditorWorldLayoutSelectionKind::TerrainProfile, 0U};
  editor.desktopUi.showWorldLayout = true;

  for (std::size_t frameIndex = 0U; frameIndex < 300U; ++frameIndex) {
    static_cast<void>(app::refreshCreativeEditorWorldLayoutDiagnostics(
        editor.worldLayout.diagnosticCache, live.facade.document(),
        editor.worldLayout.source, editor.worldLayout.revision, nullptr,
        editor.worldLayout.sourceEpoch));
  }
  const std::uint64_t afterIdle =
      editor.worldLayout.diagnosticCache.buildCount;
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> currentLines;
  const app::CreativeEditorTerrainSourceImpactOverlayFacts currentOverlay =
      app::appendCreativeEditorWorldLayoutTerrainImpactOverlay(
          live.facade.document(), editor, 0.08F, currentLines);
  const std::uint64_t documentRevisionAfterOverlay =
      live.facade.document().revision();
  --editor.worldLayout.generatedRevision;
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> pendingLines;
  const app::CreativeEditorTerrainSourceImpactOverlayFacts pendingOverlay =
      app::appendCreativeEditorWorldLayoutTerrainImpactOverlay(
          live.facade.document(), editor, 0.08F, pendingLines);
  ++editor.worldLayout.generatedRevision;
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> captureLines;
  const app::CreativeEditorTerrainSourceImpactOverlayFacts captureOverlay =
      app::appendCreativeEditorWorldLayoutTerrainImpactOverlay(
          live.facade.document(), editor, 0.08F, captureLines, true);

  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{
      live, editor, std::filesystem::path{}, &saveId};
  editor.flyPos = {40.0F, 24.0F, 40.0F};
  editor.yawDegrees = 31.0F;
  editor.pitchDegrees = -14.0F;
  editor.desktopUi.contentViewport = {0U, 0U, 1200U, 720U};
  const iggy3d::Vec3 cameraBefore = editor.flyPos;
  app::CreativeDesktopCommandFrame frame;
  frame.push(app::CreativeDesktopCommandId::WorldLayoutFrameSourceScope3D,
             app::CreativeDesktopWorldLayoutSourcePayload{
                 cr::CreativeWorldLayoutTable::TerrainProfile, 0U,
                 profile.stableKey});
  const app::CreativeDesktopCommandResult framed =
      app::dispatchCreativeDesktopCommands(frame, context);
  const bool cameraMoved = editor.flyPos.x != cameraBefore.x ||
                           editor.flyPos.y != cameraBefore.y ||
                           editor.flyPos.z != cameraBefore.z;

  ++editor.worldLayout.sourceEpoch;
  static_cast<void>(app::refreshCreativeEditorWorldLayoutDiagnostics(
      editor.worldLayout.diagnosticCache, live.facade.document(),
      editor.worldLayout.source, editor.worldLayout.revision, nullptr,
      editor.worldLayout.sourceEpoch));
  const std::uint64_t afterEpoch =
      editor.worldLayout.diagnosticCache.buildCount;
  const bool overlayAndFramePreservedDocument =
      live.facade.document().revision() == documentRevisionAfterOverlay;

  const auto& currentImpact =
      editor.worldLayout.diagnosticCache.report.terrainImpactPlan.sources[0];
  cr::CreativeTerrainControlPoint drifted = currentImpact.controls.front();
  ++drifted.heightCells;
  const cr::CreativeTerrainControlEdit driftEdit{
      cr::CreativeTerrainEditKind::Upsert, drifted};
  const cr::CreativeTerrainMutationReceipt driftReceipt =
      live.facade.documentForPersistence().applyTerrainControlEdits(
          {&driftEdit, 1U});
  static_cast<void>(app::refreshCreativeEditorWorldLayoutDiagnostics(
      editor.worldLayout.diagnosticCache, live.facade.document(),
      editor.worldLayout.source, editor.worldLayout.revision, nullptr,
      editor.worldLayout.sourceEpoch));
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> driftLines;
  const app::CreativeEditorTerrainSourceImpactOverlayFacts driftOverlay =
      app::appendCreativeEditorWorldLayoutTerrainImpactOverlay(
          live.facade.document(), editor, 0.08F, driftLines);

  return expect(afterIdle == 1U &&
                    editor.worldLayout.diagnosticCache.report
                        .terrainImpactPlan.accepted,
                "300 idle frames reuse one terrain impact compile") &&
         expect(currentOverlay.active &&
                    currentOverlay.status ==
                        cr::CreativeWorldLayoutTerrainImpactStatus::Current &&
                    currentOverlay.controlCount > 0U &&
                    currentOverlay.edgeCount == currentLines.size() &&
                    !currentLines.empty() &&
                    near(currentLines[0].color.r, 0.98F) &&
                    near(currentLines[0].color.g, 0.88F) &&
                    overlayAndFramePreservedDocument,
                "current terrain source projects yellow cached 3D impact") &&
         expect(pendingOverlay.active &&
                    pendingOverlay.status ==
                        cr::CreativeWorldLayoutTerrainImpactStatus::Drifted &&
                    !pendingLines.empty() &&
                    near(pendingLines[0].color.r, 1.0F) &&
                    near(pendingLines[0].color.g, 0.20F),
                "pending source edits project red without stale generation") &&
         expect(!captureOverlay.active && captureLines.empty(),
                "capture mode suppresses terrain source impact") &&
         expect(framed.accepted && framed.changed && cameraMoved &&
                    framed.message == "terrain source impact framed in 3D" &&
                    editor.yawDegrees == 31.0F &&
                    editor.pitchDegrees == -14.0F,
                "terrain source uses shared 3D framing command") &&
         expect(afterEpoch == 2U,
                "source replacement epoch invalidates equal revision cache") &&
         expect(driftReceipt.accepted && driftReceipt.changed &&
                    editor.worldLayout.diagnosticCache.buildCount == 3U &&
                    driftOverlay.active &&
                    driftOverlay.status ==
                        cr::CreativeWorldLayoutTerrainImpactStatus::Drifted &&
                    !driftLines.empty() &&
                    near(driftLines[0].color.r, 1.0F) &&
                    near(driftLines[0].color.g, 0.20F),
                "terrain revision invalidates once and projects drift red");
}

bool terrainReconciliationBlocksGenerationButKeepsPreviewAvailable() {
  cr::CreativeAppState live = makeApp("Diagnostic Terrain Reconciliation",
                                      9209U);
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "terrain_reconciliation_layout");
  cr::CreativeWorldLayoutTerrainProfile profile;
  profile.stableKey = "terrain.plateau";
  profile.kind = cr::CreativeTerrainRecipeKind::Plateau;
  profile.center = {3, 5};
  profile.baseHeightCells = 4U;
  profile.radiusCells = 2U;
  profile.spacingCells = 1U;
  profile.blend = cr::CreativeTerrainProfileBlend::Set;
  profile.rodPolicy = cr::CreativeTerrainProfileRodPolicy::Fill;
  state.source.terrainProfiles.push_back(profile);
  ++state.revision;

  const app::CreativeEditorWorldLayoutApplyReceipt generated =
      app::confirmCreativeEditorWorldLayout(state, live);
  const cr::CreativeWorldLayoutTerrainImpactPlan generatedImpact =
      cr::buildCreativeWorldLayoutTerrainImpactPlan(
          live.facade.document(), state.generatedBaseline.source);
  if (!generated.accepted || generatedImpact.sources.size() != 1U ||
      generatedImpact.sources[0].controls.empty()) {
    return expect(false, "terrain reconciliation diagnostic fixture generated");
  }
  cr::CreativeTerrainControlPoint drifted =
      generatedImpact.sources[0].controls.front();
  ++drifted.heightCells;
  const cr::CreativeTerrainControlEdit edit{
      cr::CreativeTerrainEditKind::Upsert, drifted};
  const cr::CreativeTerrainMutationReceipt driftReceipt =
      live.facade.documentForPersistence().applyTerrainControlEdits(
          {&edit, 1U});
  const std::uint64_t documentRevisionBeforePreview =
      live.facade.document().revision();
  const std::uint64_t terrainRevisionBeforePreview =
      live.facade.document().terrainField().revision();

  const auto& blocked = app::refreshCreativeEditorWorldLayoutDiagnostics(
      state.diagnosticCache, live.facade.document(), state.source,
      state.revision, nullptr, state.sourceEpoch, state.generatedRevision,
      &state.generatedBaseline.source);
  const std::uint64_t buildCount = state.diagnosticCache.buildCount;
  static_cast<void>(app::refreshCreativeEditorWorldLayoutDiagnostics(
      state.diagnosticCache, live.facade.document(), state.source,
      state.revision, nullptr, state.sourceEpoch, state.generatedRevision,
      &state.generatedBaseline.source));
  const app::CreativeEditorWorldLayoutPreviewReceipt preview =
      app::previewCreativeEditorWorldLayout(state, live.facade.document());

  return expect(driftReceipt.accepted && driftReceipt.changed &&
                    blocked.ready && !blocked.canGenerate &&
                    blocked.terrainReconciliation.blocked &&
                    blocked.terrainReconciliation.conflicts.size() == 1U &&
                    blocked.issueCount == 1U &&
                    blocked.issues[0].table ==
                        cr::CreativeWorldLayoutTable::TerrainProfile &&
                    blocked.issues[0].index == 0U &&
                    blocked.issues[0].stableKey == "terrain.plateau",
                "terrain drift is a navigable generation conflict") &&
         expect(state.diagnosticCache.buildCount == buildCount,
                "unchanged terrain reconciliation reuses the diagnostic cache") &&
         expect(preview.accepted &&
                    app::creativeEditorWorldLayoutPreviewActive(state) &&
                    live.facade.document().revision() ==
                        documentRevisionBeforePreview &&
                    live.facade.document().terrainField().revision() ==
                        terrainRevisionBeforePreview,
                "read-only exact preview remains available during conflict");
}

}  // namespace

int main() {
  const bool ok = preflightCacheTracksBothTruthRevisions() &&
                  missingOpeningAssetWarnsAndTracksCatalogMembership() &&
                  missingObjectAssetWarningIsNavigable() &&
                  diagnosticFocusSelectsFramesAndPreservesSource() &&
                  diagnosticFocusRoutesThroughTypedDispatcher() &&
                  stableIdPatchProjectsHonestMemberCounts() &&
                  refinementConflictProjectsExactManagedGroupAndSource() &&
                  terrainImpactCacheOverlayAndFramingShareOneSourcePlan() &&
                  terrainReconciliationBlocksGenerationButKeepsPreviewAvailable();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
