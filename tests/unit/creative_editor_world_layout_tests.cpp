
#include "EditorEdits.hpp"
#include "EditorPersistence.hpp"
#include "EditorToolDescriptor.hpp"
#include "EditorWorldLayout.hpp"
#include "EditorWorldLayoutElevation.hpp"
#include "EditorWorldLayoutHistory.hpp"
#include "EditorWorldLayoutRoofs.hpp"
#include "EditorWorldLayoutVerticalConnectorHandles.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <numbers>
#include <span>
#include <string>
#include <vector>

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/history/History.hpp"
#include "app/iggy3d/creative/input/Catalog.hpp"
#include "app/iggy3d/creative/render/CreativeScreenProjection.hpp"
#include "app/iggy3d/creative/world/MapTemplate.hpp"
#include "app/iggy3d/creative/world/WorldLayoutCodec.hpp"
#include "app/iggy3d/creative/world/WorldLayoutDimensions.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRoofs.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"

namespace app = iggy3d_creative_app;
namespace cr = iggy3d::creative;

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(double lhs, double rhs) {
  return std::abs(lhs - rhs) <= 1.0e-9;
}

bool stableKeysUnique(const cr::CreativeWorldLayout& layout) {
  std::vector<std::string> keys;
  const auto append = [&](const auto& values) {
    for (const auto& value : values) {
      keys.push_back(value.stableKey);
    }
  };
  append(layout.buildings);
  append(layout.levels);
  append(layout.rooms);
  append(layout.verticalConnectors);
  append(layout.boxes);
  append(layout.walls);
  append(layout.openings);
  append(layout.objects);
  append(layout.terrainProfiles);
  append(layout.terrainPaths);
  std::sort(keys.begin(), keys.end());
  return std::adjacent_find(keys.begin(), keys.end()) == keys.end();
}

cr::CreativeAppState appState() {
  cr::CreativeAppState state;
  cr::CreativeDocument document = cr::CreativeDocument::create("Layout Editor");
  static_cast<void>(document.assignId(8101U));
  static_cast<void>(state.facade.installDocument(std::move(document)));
  return state;
}

bool floorAndWallGesturesProduceNormalizedSymbols() {
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "editor_layout");
  const auto floorTool = app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Floor);
  const auto floorStart =
      app::applyCreativeEditorWorldLayoutPoint(state, {5.1, 4.2});
  const auto floorEnd =
      app::applyCreativeEditorWorldLayoutPoint(state, {-2.2, -1.1});
  const auto wallTool = app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Wall);
  const auto wallStart =
      app::applyCreativeEditorWorldLayoutPoint(state, {0.0, 0.0});
  const auto wallEnd =
      app::applyCreativeEditorWorldLayoutPoint(state, {6.0, 2.0});

  return expect(floorTool.accepted && floorStart.accepted &&
                    !floorStart.changed && floorEnd.changed,
                "floor is one two-click source edit") &&
         expect(state.source.buildings.size() == 1U &&
                    state.source.boxes.size() == 1U,
                "floor creates one shared building and one slab") &&
         expect(state.source.boxes[0].footprint.minimum ==
                        cr::CreativeTerrainCoord2{-2, -1} &&
                    state.source.boxes[0].footprint.maximum ==
                        cr::CreativeTerrainCoord2{5, 4},
                "floor rectangle is normalized and grid snapped") &&
         expect(wallTool.accepted && wallStart.accepted && wallEnd.changed &&
                    state.source.walls.size() == 1U,
                "wall is one two-click source edit") &&
         expect(
             state.source.walls[0].start == cr::CreativeTerrainCoord2{0, 0} &&
                 state.source.walls[0].end == cr::CreativeTerrainCoord2{6, 0},
             "diagonal pointer input resolves to a cardinal wall");
}

bool categorizedPaletteOwnsEveryBindableSemanticAction() {
  std::array<std::size_t, static_cast<std::size_t>(
                              app::CreativeEditorWorldLayoutPaletteCategory::Count)>
      categoryCounts{};
  std::array<bool, static_cast<std::size_t>(
                       app::CreativeEditorWorldLayoutTool::Count)>
      seenTools{};
  bool unique = true;
  std::size_t boundToolCount = 0U;
  for (const app::CreativeEditorToolDescriptor& descriptor :
       app::creativeEditorToolDescriptors()) {
    if (descriptor.worldLayoutActivation !=
        app::CreativeEditorWorldLayoutToolActivation::Tool) {
      continue;
    }
    ++boundToolCount;
    const std::size_t category = static_cast<std::size_t>(
        app::creativeEditorWorldLayoutPaletteCategory(descriptor));
    if (category >= categoryCounts.size() || descriptor.name.empty()) {
      unique = false;
      continue;
    }
    ++categoryCounts[category];
    const std::size_t tool =
        static_cast<std::size_t>(descriptor.worldLayoutTool);
    if (tool >= seenTools.size() || seenTools[tool]) {
      unique = false;
    } else {
      seenTools[tool] = true;
    }
  }
  return expect(boundToolCount == seenTools.size() && unique &&
                    std::all_of(seenTools.begin(), seenTools.end(),
                                [](bool seen) { return seen; }),
                "every drafting tool has one descriptor binding") &&
         expect(std::all_of(categoryCounts.begin(), categoryCounts.end(),
                            [](std::size_t count) { return count > 0U; }),
                "every palette category owns an action") &&
         expect(seenTools[static_cast<std::size_t>(
                    app::CreativeEditorWorldLayoutTool::Select)] &&
                    seenTools[static_cast<std::size_t>(
                        app::CreativeEditorWorldLayoutTool::BuildingShell)] &&
                    seenTools[static_cast<std::size_t>(
                        app::CreativeEditorWorldLayoutTool::Stair)] &&
                    seenTools[static_cast<std::size_t>(
                        app::CreativeEditorWorldLayoutTool::Ramp)] &&
                    seenTools[static_cast<std::size_t>(
                        app::CreativeEditorWorldLayoutTool::Plateau)] &&
                    seenTools[static_cast<std::size_t>(
                        app::CreativeEditorWorldLayoutTool::NpcSpawn)],
                "palette binds structures terrain objects and gameplay") &&
         expect(app::creativeEditorWorldLayoutToolIsVerticalConnector(
                    app::CreativeEditorWorldLayoutTool::Stair) &&
                    app::creativeEditorWorldLayoutToolIsVerticalConnector(
                        app::CreativeEditorWorldLayoutTool::Ramp) &&
                    !app::creativeEditorWorldLayoutToolIsVerticalConnector(
                        app::CreativeEditorWorldLayoutTool::Floor),
                "vertical connector tool classification is centralized");
}

bool terrainAndObjectPaletteToolsCreateCompilableSymbols() {
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "palette_layout");

  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Plateau));
  const auto plateau =
      app::applyCreativeEditorWorldLayoutPoint(state, {12.0, 10.0});

  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Road));
  const auto roadBegin =
      app::applyCreativeEditorWorldLayoutPoint(state, {4.0, 4.0});
  const auto roadBend =
      app::applyCreativeEditorWorldLayoutPoint(state, {10.0, 8.0});
  const auto roadEnd =
      app::applyCreativeEditorWorldLayoutPoint(state, {16.0, 4.0});
  const auto roadCommit = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {});

  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Ditch));
  const auto ditchBegin =
      app::applyCreativeEditorWorldLayoutPoint(state, {4.0, 20.0});
  const auto ditchBend =
      app::applyCreativeEditorWorldLayoutPoint(state, {10.0, 24.0});
  const auto ditchEnd =
      app::applyCreativeEditorWorldLayoutPoint(state, {16.0, 20.0});
  const auto ditchCommit = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {});

  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Bridge));
  const auto bridgeBegin = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {8.0, 18.0});
  const auto bridgeCommit = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {12.0, 22.0});

  const auto addPoint = [&](app::CreativeEditorWorldLayoutTool tool,
                            app::CreativeEditorWorldLayoutPoint point) {
    static_cast<void>(app::setCreativeEditorWorldLayoutTool(state, tool));
    return app::applyCreativeEditorWorldLayoutPoint(state, point);
  };
  cr::CreativeCatalogEntry boulderEntry;
  boulderEntry.category = cr::CreativeCatalogEntryCategory::Asset;
  boulderEntry.label = "Boulder 01";
  boulderEntry.searchText = "boulder 01 rock nature";
  boulderEntry.assetAuthoringMetadata.categoryId = "boulder";
  boulderEntry.hotbarEntry.objectKind = cr::CreativeObjectKind::Rock;
  static_cast<void>(cr::setCreativeHotbarAsset(
      boulderEntry.hotbarEntry, "boulder_01",
      {{-1.25, 0.0, -1.25}, {1.25, 2.0, 1.25}}));
  const auto selectedBoulder =
      app::selectCreativeEditorWorldLayoutCatalogAsset(state, boulderEntry);
  const auto boulder = app::applyCreativeEditorWorldLayoutPoint(
      state, {2.0, 12.0}, cr::CreativeGridSettings{});
  const auto player =
      addPoint(app::CreativeEditorWorldLayoutTool::PlayerSpawn, {6.0, 8.0});
  const auto npc =
      addPoint(app::CreativeEditorWorldLayoutTool::NpcSpawn, {8.0, 8.0});

  cr::CreativeAppState app = appState();
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(app.facade.document(), state.source);
  const cr::CreativeWorldLayoutEncodeResult encoded =
      cr::encodeCreativeWorldLayout(state.source);
  const cr::CreativeWorldLayoutDecodeResult decoded =
      cr::decodeCreativeWorldLayout(encoded.encodedText);

  return expect(plateau.accepted && plateau.changed && roadBegin.accepted &&
                    roadBend.accepted && roadEnd.accepted &&
                    roadCommit.accepted && roadCommit.changed &&
                    ditchBegin.accepted && ditchBend.accepted &&
                    ditchEnd.accepted && ditchCommit.accepted &&
                    ditchCommit.changed && bridgeBegin.accepted &&
                    bridgeCommit.accepted && bridgeCommit.changed &&
                    selectedBoulder.accepted && boulder.accepted &&
                    player.accepted && npc.accepted,
                "palette actions author semantic symbols") &&
         expect(state.source.terrainProfiles.size() == 1U &&
                    state.source.terrainProfiles[0].usesLandformRecipe &&
                    state.source.terrainProfiles[0].landform.kind ==
                        cr::CreativeTerrainLandformKind::Plateau &&
                    state.source.terrainProfiles[0].landform.bounds ==
                        cr::CreativeTerrainHeightFieldBounds{{8, 6}, 8U, 8U} &&
                    state.source.terrainPaths.size() == 2U &&
                    state.source.terrainPaths[0].recipe.points.size() == 3U &&
                    state.source.terrainPaths[1].recipe.points.size() == 3U &&
                    state.source.terrainPaths[0].recipe.points[1].id == 2U &&
                    state.source.terrainPaths[1].recipe.points[2].id == 3U &&
                    state.source.objects.size() == 4U,
                "palette actions retain durable per-path source points") &&
         expect(state.source.objects[0].kind == cr::CreativeObjectKind::Bridge &&
                    !state.source.objects[0].usesBridgeRecipe &&
                    state.source.objects[1].assetId == "boulder_01" &&
                    state.source.objects[1].mode ==
                        cr::CreativeObjectLibraryPlacementMode::Point &&
                    state.source.objects[1].hasAssetSourceBounds &&
                    state.source.objects[2].kind ==
                        cr::CreativeObjectKind::SpawnPoint &&
                    state.source.objects[3].kind ==
                        cr::CreativeObjectKind::NpcSpawn,
                "object palette entries preserve semantic identity") &&
         expect(compiled.receipt.accepted &&
                    compiled.receipt.objectRecipeCount == 4U &&
                    compiled.receipt.objectRecipeCreateCount == 4U &&
                    compiled.receipt.objectCount == 4U &&
                    compiled.plan.terrainOperationMutations.size() == 3U &&
                    compiled.plan.terrainOperationMutations[0].operationKind ==
                        cr::CreativeTerrainOperationKind::Landform,
                "palette source compiles independently regenerable objects") &&
         expect(encoded.accepted && decoded.accepted &&
                    decoded.layout.objects.size() == 4U &&
                    decoded.layout.terrainPaths.size() == 2U &&
                    decoded.layout.terrainProfiles.size() == 1U &&
                    decoded.layout.terrainProfiles[0].usesLandformRecipe &&
                    decoded.layout.terrainProfiles[0].landform ==
                        state.source.terrainProfiles[0].landform,
                "palette source survives durable layout round trip");
}

bool bridgeToolInfersOneStableCrossingAndRejectsAmbiguity() {
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "bridge_attachment_layout");
  cr::CreativeWorldLayoutTerrainPath trench;
  trench.stableKey = "trench.main";
  trench.recipe.kind = cr::CreativeTerrainPathKind::Trench;
  trench.recipe.elevation = cr::CreativeTerrainPathElevation::Level;
  trench.recipe.crossSection = cr::CreativeTerrainPathCrossSection::Cut;
  trench.recipe.watercourse.bankSlopeCells = 1U;
  trench.recipe.watercourse.nextCrossingId = 8U;
  trench.recipe.watercourse.crossings = {{7U, 2U, 1U, 1U, 2U}};
  trench.recipe.nextPointId = 4U;
  trench.recipe.points = {
      {1U, {-4, 0}, 4U, 1U, 1U, 0},
      {2U, {0, 0}, 4U, 1U, 1U, 0},
      {3U, {4, 0}, 4U, 1U, 1U, 0},
  };
  state.source.terrainPaths.push_back(trench);

  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Bridge));
  const auto begun = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {-2.0, -2.0});
  const auto attached = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {2.0, 2.0});
  const cr::CreativeWorldLayoutObject* bridge =
      state.source.objects.size() == 1U ? &state.source.objects[0] : nullptr;

  const std::uint64_t revisionBeforeOccupied = state.revision;
  const auto occupiedBegin = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {-2.0, -2.0});
  const auto occupied = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {2.0, 2.0});

  state.source.terrainPaths[0].recipe.watercourse.nextCrossingId = 9U;
  state.source.terrainPaths[0].recipe.watercourse.crossings.push_back(
      {8U, 1U, 1U, 1U, 2U});
  const std::uint64_t revisionBeforeAmbiguous = state.revision;
  const auto ambiguousBegin = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {-5.0, -1.0});
  const auto ambiguous = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {1.0, 1.0});

  return expect(begun.accepted && attached.accepted && attached.changed &&
                    bridge != nullptr && bridge->usesBridgeRecipe,
                "bridge footprint binds one authored crossing") &&
         expect(bridge->bridge.watercoursePathKey == "trench.main" &&
                    bridge->bridge.crossingId == 7U &&
                    cr::isValidCreativeBridgeSourceRecipe(bridge->bridge),
                "bridge retains stable path and crossing identity") &&
         expect(occupiedBegin.accepted && !occupied.accepted &&
                    !occupied.changed &&
                    occupied.reasonCode ==
                        "creative_editor_world_layout_bridge_attachment_occupied" &&
                    state.source.objects.size() == 1U &&
                    revisionBeforeOccupied == revisionBeforeAmbiguous,
                "bridge tool keeps one owner per crossing") &&
         expect(ambiguousBegin.accepted && !ambiguous.accepted &&
                    !ambiguous.changed &&
                    ambiguous.reasonCode ==
                        "creative_editor_world_layout_bridge_attachment_ambiguous" &&
                    state.source.objects.size() == 1U &&
                    state.revision == revisionBeforeAmbiguous,
                "bridge tool rejects an ambiguous crossing footprint atomically");
}

bool terrainPathDraftCommitsAsOneSourceEdit() {
  cr::CreativeAppState live = appState();
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "path_draft_layout");
  const std::uint64_t revisionBefore = state.revision;
  const std::uint64_t undoDepthBefore =
      app::creativeEditorWorldLayoutSourceUndoDepth(state);

  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Road));
  const auto first =
      app::applyCreativeEditorWorldLayoutPoint(state, {1.0, 2.0});
  const auto incomplete = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {});
  const bool firstPointStayedTransient =
      first.accepted && !first.changed && !incomplete.accepted &&
      state.terrainPathDraft.active &&
      state.terrainPathDraft.path.recipe.points.size() == 1U &&
      state.source.terrainPaths.empty() && state.revision == revisionBefore &&
      app::creativeEditorWorldLayoutSourceUndoDepth(state) == undoDepthBefore;

  const auto cancelled = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Cancel, {});
  const bool cancelStayedTransient =
      cancelled.accepted && cancelled.changed &&
      !state.terrainPathDraft.active && state.source.terrainPaths.empty() &&
      state.revision == revisionBefore &&
      app::creativeEditorWorldLayoutSourceUndoDepth(state) == undoDepthBefore;

  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(state,
                                                              {1.0, 2.0}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(state,
                                                              {5.0, 4.0}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(state,
                                                              {9.0, 2.0}));
  const auto committed = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {});
  const bool oneCommit =
      committed.accepted && committed.changed &&
      !state.terrainPathDraft.active && state.source.terrainPaths.size() == 1U &&
      state.source.terrainPaths[0].recipe.points.size() == 3U &&
      state.revision == revisionBefore + 1U &&
      app::creativeEditorWorldLayoutSourceUndoDepth(state) ==
          undoDepthBefore + 1U;

  const cr::CreativeHistoryApplyReceipt undone =
      app::applyCreativeEditorWorldLayoutHistory(
          state, live, cr::CreativeHistoryDirection::Undo);
  const bool wholePathUndone =
      undone.accepted && undone.changed && state.source.terrainPaths.empty() &&
      state.revision == revisionBefore &&
      app::creativeEditorWorldLayoutSourceRedoDepth(state) == 1U;

  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(state,
                                                              {2.0, 2.0}));
  const auto switched = app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Wall);
  const bool toolSwitchCancelsTransientDraft =
      switched.accepted && switched.changed &&
      !state.terrainPathDraft.active && !state.anchorActive &&
      state.source.terrainPaths.empty() && state.revision == revisionBefore;

  return expect(firstPointStayedTransient,
                "incomplete path points do not mutate semantic source") &&
         expect(cancelStayedTransient,
                "cancelling a path draft leaves source history untouched") &&
         expect(oneCommit,
                "finishing a multi-point path records one source edit") &&
         expect(wholePathUndone,
                "one source undo removes the whole authored path") &&
         expect(toolSwitchCancelsTransientDraft,
                "switching tools discards an unfinished path draft");
}

bool terrainPathDraftBuildsExactTransient3dPreview() {
  cr::CreativeAppState live = appState();
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "path_preview_layout");
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Road));

  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(state,
                                                              {2.0, 3.0}));
  const auto onePoint = app::previewCreativeEditorWorldLayoutTerrainPathDraft(
      state, live.facade.document());
  const bool incompleteHasNo3dOutput =
      onePoint.accepted && !onePoint.changed &&
      !app::creativeEditorWorldLayoutPreviewActive(state) &&
      state.source.terrainPaths.empty();

  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(state,
                                                              {7.0, 6.0}));
  const auto twoPoint = app::previewCreativeEditorWorldLayoutTerrainPathDraft(
      state, live.facade.document());
  const std::uint64_t firstContentRevision = state.previewContentRevision;
  const bool twoPointsBuildExactCandidate =
      twoPoint.accepted && twoPoint.changed &&
      app::creativeEditorWorldLayoutPreviewActive(state) &&
      state.liveEditPreviewVisible && state.source.terrainPaths.empty() &&
      state.previewSource.terrainPaths.size() == 1U &&
      state.previewSource.terrainPaths[0].recipe ==
          state.terrainPathDraft.path.recipe &&
      state.preview.document.terrainOperationStack().operations.size() == 1U &&
      state.preview.document.terrainOperationStack().operations[0].kind ==
          cr::CreativeTerrainOperationKind::Path &&
      state.preview.document.terrainOperationStack().operations[0].path ==
          state.terrainPathDraft.path.recipe;

  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(state,
                                                              {11.0, 2.0}));
  const auto threePoint =
      app::previewCreativeEditorWorldLayoutTerrainPathDraft(
          state, live.facade.document());
  const bool bendRefreshesSameTransientCandidate =
      threePoint.accepted && threePoint.changed &&
      state.previewContentRevision > firstContentRevision &&
      state.source.terrainPaths.empty() &&
      state.previewSource.terrainPaths.size() == 1U &&
      state.previewSource.terrainPaths[0].recipe.points.size() == 3U &&
      state.revision == state.generatedRevision;

  const auto cancelled = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Cancel, {});
  const bool previewCleared =
      cancelled.accepted &&
      app::clearCreativeEditorWorldLayoutLiveEditPreview(state) &&
      !app::creativeEditorWorldLayoutPreviewActive(state) &&
      state.source.terrainPaths.empty();

  return expect(incompleteHasNo3dOutput,
                "one path point has no fabricated 3D segment") &&
         expect(twoPointsBuildExactCandidate,
                "two path points build the exact transient 3D recipe") &&
         expect(bendRefreshesSameTransientCandidate,
                "each added bend refreshes one transient path candidate") &&
         expect(previewCleared,
                "cancelling a path draft clears its transient 3D output");
}

bool catalogPlacementSharesOneExactTwoAndThreeDimensionalRecipe() {
  const auto catalogEntry = [](std::string label, std::string assetId,
                               std::string categoryId,
                               cr::CreativeObjectKind kind,
                               cr::CreativeBounds sourceBounds) {
    cr::CreativeCatalogEntry entry;
    entry.category = cr::CreativeCatalogEntryCategory::Asset;
    entry.label = std::move(label);
    entry.searchText = "dresser furnishing interior prop";
    entry.assetAuthoringMetadata.categoryId = std::move(categoryId);
    entry.hotbarEntry.objectKind = kind;
    static_cast<void>(cr::setCreativeHotbarAsset(
        entry.hotbarEntry, assetId, sourceBounds));
    return entry;
  };

  const cr::CreativeCatalogEntry architecture = catalogEntry(
      "Wall", "wall_asset", "wall", cr::CreativeObjectKind::Wall,
      {{-0.5, 0.0, -0.1}, {0.5, 2.5, 0.1}});
  const cr::CreativeCatalogEntry nature = catalogEntry(
      "Tree", "tree_asset", "tree", cr::CreativeObjectKind::Prop,
      {{-0.5, 0.0, -0.5}, {0.5, 3.0, 0.5}});
  const cr::CreativeCatalogEntry cover = catalogEntry(
      "Cover", "cover_asset", "stealth_blockout",
      cr::CreativeObjectKind::Prop, {{-1.0, 0.0, -0.2}, {1.0, 1.2, 0.2}});
  const cr::CreativeCatalogEntry gameplay = catalogEntry(
      "Objective", "objective_asset", "objective",
      cr::CreativeObjectKind::Prop, {{-0.4, 0.0, -0.4}, {0.4, 0.8, 0.4}});
  const cr::CreativeCatalogEntry dresser = catalogEntry(
      "Dresser", "homestead/interior/dresser_1p3", "furniture",
      cr::CreativeObjectKind::Prop,
      {{-0.5, 0.0, -0.25}, {1.5, 1.0, 0.75}});

  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "catalog_layout");
  const auto selected =
      app::selectCreativeEditorWorldLayoutCatalogAsset(state, dresser);
  state.catalogPlacement.elevationCells = 2.0;
  state.catalogPlacement.yawDegrees = 45.0;
  state.catalogPlacement.scale = {1.5, 0.75, 2.0};
  cr::CreativeGridSettings grid;
  grid.cellSizeMeters = 0.5;
  const app::CreativeEditorWorldLayoutCatalogPlacementPlan preview =
      app::planCreativeEditorWorldLayoutCatalogPlacement(state, {4.2, 5.7},
                                                         grid);
  const std::size_t undoBefore = state.sourceHistory.undoEntries.size();
  const auto placed = app::applyCreativeEditorWorldLayoutPoint(
      state, {4.2, 5.7}, grid);
  const cr::CreativeWorldLayoutObject& object = state.source.objects[0];
  const app::CreativeEditorWorldLayoutObjectFootprint footprint =
      app::planCreativeEditorWorldLayoutObjectFootprint(object, grid);

  cr::CreativeDocument document = cr::CreativeDocument::create("Catalog Plan");
  static_cast<void>(document.assignId(8102U));
  static_cast<void>(document.setGridSettings(grid));
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, state.source);
  const cr::CreativeDocumentCreateRequest* createRequest =
      compiled.receipt.accepted && compiled.plan.objectRecipes.size() == 1U &&
              compiled.plan.objectRecipes[0].objects.size() == 1U
          ? &compiled.plan.objectRecipes[0].objects[0].createRequest
          : nullptr;
  const cr::CreativeTransformedBounds worldBounds =
      createRequest == nullptr
          ? cr::CreativeTransformedBounds{}
          : cr::resolveCreativeTransformedBounds(createRequest->bounds,
                                                 createRequest->transform);
  const app::CreativeEditorWorldLayoutPoint aabbOnlyPoint{
      footprint.axisAlignedBoundsCells.min.x + 0.01,
      footprint.axisAlignedBoundsCells.min.z + 0.01};

  return expect(
             app::classifyCreativeEditorWorldLayoutAsset(architecture) ==
                     app::CreativeEditorWorldLayoutAssetCategory::Architecture &&
                 app::classifyCreativeEditorWorldLayoutAsset(nature) ==
                     app::CreativeEditorWorldLayoutAssetCategory::Nature &&
                 app::classifyCreativeEditorWorldLayoutAsset(cover) ==
                     app::CreativeEditorWorldLayoutAssetCategory::Cover &&
                 app::classifyCreativeEditorWorldLayoutAsset(dresser) ==
                     app::CreativeEditorWorldLayoutAssetCategory::Props &&
                 app::classifyCreativeEditorWorldLayoutAsset(gameplay) ==
                     app::CreativeEditorWorldLayoutAssetCategory::Gameplay &&
                 app::creativeEditorWorldLayoutAssetMatchesQuery(dresser,
                                                                 "DRESSER") &&
                 !app::creativeEditorWorldLayoutAssetMatchesQuery(dresser,
                                                                  "tree"),
             "catalog assets classify and search through one pure index") &&
         expect(selected.accepted && preview.accepted && placed.accepted &&
                    placed.changed && footprint.valid &&
                    state.source.objects.size() == 1U &&
                    state.sourceHistory.undoEntries.size() == undoBefore + 1U &&
                    object.stableKey.starts_with("asset_") &&
                    object.pointCells.x == 4.0 && object.pointCells.y == 2.0 &&
                    object.pointCells.z == 6.0 &&
                    object.assetId == "homestead/interior/dresser_1p3" &&
                    object.hasAssetSourceBounds &&
                    near(object.yawRadians, std::acos(-1.0) * 0.25) &&
                    object.scale.x == 1.5 && object.scale.y == 0.75 &&
                    object.scale.z == 2.0 && stableKeysUnique(state.source),
                "catalog confirm creates one durable source edit") &&
         expect(createRequest != nullptr && worldBounds.valid &&
                    createRequest->assetId == object.assetId &&
                    near(createRequest->transform.position.x, 2.0) &&
                    near(createRequest->transform.position.y, 1.0) &&
                    near(createRequest->transform.position.z, 3.0) &&
                    near(createRequest->transform.rotationEulerRadians.y,
                         object.yawRadians) &&
                    near(footprint.axisAlignedBoundsCells.min.x *
                             grid.cellSizeMeters,
                         worldBounds.worldBounds.min.x) &&
                    near(footprint.axisAlignedBoundsCells.min.z *
                             grid.cellSizeMeters,
                         worldBounds.worldBounds.min.z) &&
                    near(footprint.axisAlignedBoundsCells.max.x *
                             grid.cellSizeMeters,
                         worldBounds.worldBounds.max.x) &&
                    near(footprint.axisAlignedBoundsCells.max.z *
                             grid.cellSizeMeters,
                         worldBounds.worldBounds.max.z),
                "2D footprint and generated 3D bounds share pivot math") &&
         expect(app::findCreativeEditorWorldLayoutObjectAt(state, {4.0, 6.0},
                                                            grid) == 0U &&
                    app::findCreativeEditorWorldLayoutObjectAt(
                        state, aabbOnlyPoint, grid) ==
                        cr::kInvalidCreativeWorldLayoutIndex,
                "rotated hit testing uses the exact footprint, not its AABB");
}

bool catalogFloorSnapPlacesScaledSourceBottomOnTheFinishedFloor() {
  cr::CreativeCatalogEntry entry;
  entry.category = cr::CreativeCatalogEntryCategory::Asset;
  entry.label = "Asymmetric Cabinet";
  entry.assetAuthoringMetadata.categoryId = "structure";
  entry.hotbarEntry.objectKind = cr::CreativeObjectKind::Prop;
  static_cast<void>(cr::setCreativeHotbarAsset(
      entry.hotbarEntry, "architecture/asymmetric_cabinet",
      {{-0.75, -0.60, -0.25}, {1.25, 1.40, 0.75}}));

  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "floor_snap_layout");
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "building_1";
  building.name = "Building";
  state.source.buildings.push_back(building);
  cr::CreativeWorldLayoutLevel level;
  level.buildingIndex = 0U;
  level.stableKey = "level_1";
  level.name = "Upper Floor";
  level.floorTopLayer = 2.5;
  state.source.levels.push_back(level);
  cr::CreativeWorldLayoutRoom room;
  room.buildingIndex = 0U;
  room.levelIndex = 0U;
  room.stableKey = "room_1";
  room.name = "Room";
  room.footprint = {{0, -4}, {8, 4}};
  state.source.rooms.push_back(room);
  state.activeLevelIndex = 0U;

  const auto selected =
      app::selectCreativeEditorWorldLayoutCatalogAsset(state, entry);
  state.catalogPlacement.snapMode =
      app::CreativeEditorWorldLayoutCatalogSnapMode::Floor;
  state.catalogPlacement.elevationCells = -99.0;
  state.catalogPlacement.yawDegrees = 30.0;
  state.catalogPlacement.scale = {1.5, 1.75, 0.65};
  cr::CreativeGridSettings grid;
  grid.origin = {7.0, -3.0, 11.0};
  grid.cellSizeMeters = 0.4;
  const app::CreativeEditorWorldLayoutCatalogPlacementPlan preview =
      app::planCreativeEditorWorldLayoutCatalogPlacement(state, {3.2, -1.7},
                                                         grid);
  const auto placed = app::applyCreativeEditorWorldLayoutPoint(
      state, {3.2, -1.7}, grid);

  cr::CreativeDocument document = cr::CreativeDocument::create("Floor Snap");
  static_cast<void>(document.assignId(8103U));
  static_cast<void>(document.setGridSettings(grid));
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, state.source);
  const auto objectRecipe = std::find_if(
      compiled.plan.objectRecipes.begin(), compiled.plan.objectRecipes.end(),
      [](const cr::CreativeRecipePlan& recipe) {
        return recipe.kind == cr::CreativeRecipeKind::ObjectLibrary;
      });
  const cr::CreativeDocumentCreateRequest* request =
      compiled.receipt.accepted &&
              objectRecipe != compiled.plan.objectRecipes.end() &&
              objectRecipe->objects.size() == 1U
          ? &objectRecipe->objects[0].createRequest
          : nullptr;
  const cr::CreativeTransformedBounds generated =
      request == nullptr
          ? cr::CreativeTransformedBounds{}
          : cr::resolveCreativeTransformedBounds(request->bounds,
                                                 request->transform);
  const double expectedElevation =
      2.5 - (-0.60 * 1.75) / grid.cellSizeMeters;
  const double finishedFloorWorldY =
      grid.origin.y + 2.5 * grid.cellSizeMeters;

  app::CreativeEditorWorldLayoutState missingLevel = state;
  missingLevel.activeLevelIndex = cr::kInvalidCreativeWorldLayoutIndex;
  const app::CreativeEditorWorldLayoutCatalogPlacementPlan rejected =
      app::planCreativeEditorWorldLayoutCatalogPlacement(
          missingLevel, {3.2, -1.7}, grid);

  return expect(selected.accepted && preview.accepted && placed.accepted &&
                    preview.snapMode ==
                        app::CreativeEditorWorldLayoutCatalogSnapMode::Floor &&
                    preview.snapHostKind ==
                        app::CreativeEditorWorldLayoutCatalogSnapHostKind::
                            LevelFloor &&
                    preview.snapHostIndex == 0U &&
                    near(preview.object.pointCells.x, 3.0) &&
                    near(preview.object.pointCells.y, expectedElevation) &&
                    near(preview.object.pointCells.z, -2.0) &&
                    near(preview.object.yawRadians,
                         std::numbers::pi / 6.0),
                "floor snap derives one finite pose from active-level truth") &&
         expect(request != nullptr && generated.valid &&
                    near(generated.worldBounds.min.y, finishedFloorWorldY) &&
                    near(state.source.objects[0].pointCells.y,
                         preview.object.pointCells.y),
                "preview and generated geometry share the exact scaled bottom contact") &&
         expect(!rejected.accepted &&
                    rejected.reasonCode ==
                        "creative_editor_world_layout_catalog_floor_missing" &&
                    rejected.message == "Select a level for floor snap",
                "floor snap fails closed without an active level");
}

bool catalogWallSnapUsesCanonicalActiveLevelHosts() {
  const auto catalogAsset = [](std::string categoryId,
                               std::string assetId) {
    cr::CreativeCatalogEntry entry;
    entry.category = cr::CreativeCatalogEntryCategory::Asset;
    entry.label = "Wall Module";
    entry.assetAuthoringMetadata.categoryId = std::move(categoryId);
    entry.hotbarEntry.objectKind = cr::CreativeObjectKind::Prop;
    static_cast<void>(cr::setCreativeHotbarAsset(
        entry.hotbarEntry, assetId,
        {{-1.0, -0.5, -0.2}, {2.0, 1.0, 0.2}}));
    return entry;
  };
  const auto restsOnWallFace = [](const auto& plan) {
    if (!plan.accepted || !plan.footprint.valid) {
      return false;
    }
    const double normalLength =
        std::hypot(plan.snapNormal.x, plan.snapNormal.z);
    double minimumFaceDistance = std::numeric_limits<double>::infinity();
    for (const app::CreativeEditorWorldLayoutPoint corner :
         plan.footprint.corners) {
      const double distance =
          (corner.x - plan.snapSurfacePoint.x) * plan.snapNormal.x +
          (corner.z - plan.snapSurfacePoint.z) * plan.snapNormal.z;
      if (distance < -1.0e-9) {
        return false;
      }
      minimumFaceDistance = std::min(minimumFaceDistance, distance);
    }
    return near(normalLength, 1.0) && near(minimumFaceDistance, 0.0);
  };

  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "wall_snap_layout");
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "building_1";
  building.name = "Building";
  state.source.buildings.push_back(building);
  cr::CreativeWorldLayoutLevel ground;
  ground.buildingIndex = 0U;
  ground.stableKey = "level_ground";
  ground.name = "Ground";
  ground.floorTopLayer = 1.5;
  state.source.levels.push_back(ground);
  cr::CreativeWorldLayoutLevel upper = ground;
  upper.stableKey = "level_upper";
  upper.name = "Upper";
  upper.floorTopLayer = 4.5;
  state.source.levels.push_back(upper);
  state.activeLevelIndex = 0U;

  cr::CreativeWorldLayoutWall wrongLevel;
  wrongLevel.buildingIndex = 0U;
  wrongLevel.stableKey = "wall_upper";
  wrongLevel.name = "Upper Wall";
  wrongLevel.start = {0, 3};
  wrongLevel.end = {4, 3};
  wrongLevel.baseLayer = 4.5;
  state.source.walls.push_back(wrongLevel);
  cr::CreativeWorldLayoutWall diagonal = wrongLevel;
  diagonal.stableKey = "wall_ground";
  diagonal.name = "Ground Diagonal";
  diagonal.start = {4, 4};
  diagonal.end = {0, 0};
  diagonal.baseLayer = 1.5;
  diagonal.thicknessCells = 0.75;
  state.source.walls.push_back(diagonal);

  const cr::CreativeCatalogEntry wallAsset =
      catalogAsset("wall", "architecture/wall_module");
  static_cast<void>(
      app::selectCreativeEditorWorldLayoutCatalogAsset(state, wallAsset));
  state.catalogPlacement.snapMode =
      app::CreativeEditorWorldLayoutCatalogSnapMode::Wall;
  state.catalogPlacement.yawDegrees = 15.0;
  state.catalogPlacement.scale = {2.0, 2.0, 0.5};
  cr::CreativeGridSettings grid;
  grid.cellSizeMeters = 0.5;
  const app::CreativeEditorWorldLayoutCatalogPlacementPlan diagonalPlan =
      app::planCreativeEditorWorldLayoutCatalogPlacement(state, {2.0, 3.0},
                                                         grid);
  state.catalogPlacement.wallSideFlipped = true;
  const app::CreativeEditorWorldLayoutCatalogPlacementPlan flippedPlan =
      app::planCreativeEditorWorldLayoutCatalogPlacement(state, {2.0, 3.0},
                                                         grid);
  state.catalogPlacement.wallSideFlipped = false;
  const app::CreativeEditorWorldLayoutCatalogPlacementPlan noHost =
      app::planCreativeEditorWorldLayoutCatalogPlacement(state, {20.0, 20.0},
                                                         grid);

  app::CreativeEditorWorldLayoutState rooms;
  app::resetCreativeEditorWorldLayout(rooms, "room_wall_snap_layout");
  rooms.source.buildings.push_back(building);
  rooms.source.levels.push_back(ground);
  rooms.source.levels[0].wallHeightCells = 6U;
  rooms.activeLevelIndex = 0U;
  cr::CreativeWorldLayoutRoom firstRoom;
  firstRoom.buildingIndex = 0U;
  firstRoom.levelIndex = 0U;
  firstRoom.stableKey = "room_first";
  firstRoom.name = "First";
  firstRoom.footprint = {{0, 0}, {4, 4}};
  firstRoom.wallThicknessCells = 0.5;
  rooms.source.rooms.push_back(firstRoom);
  cr::CreativeWorldLayoutRoom secondRoom = firstRoom;
  secondRoom.stableKey = "room_second";
  secondRoom.name = "Second";
  secondRoom.footprint = {{4, 0}, {8, 4}};
  rooms.source.rooms.push_back(secondRoom);
  static_cast<void>(
      app::selectCreativeEditorWorldLayoutCatalogAsset(rooms, wallAsset));
  rooms.catalogPlacement.snapMode =
      app::CreativeEditorWorldLayoutCatalogSnapMode::Wall;
  const app::CreativeEditorWorldLayoutCatalogPlacementPlan sharedEdge =
      app::planCreativeEditorWorldLayoutCatalogPlacement(rooms, {4.0, 2.0},
                                                         grid);

  app::CreativeEditorWorldLayoutState openingAsset = rooms;
  cr::CreativeCatalogEntry doorAsset =
      catalogAsset("door", "architecture/door_leaf");
  static_cast<void>(cr::setCreativeHotbarAsset(
      doorAsset.hotbarEntry, "architecture/door_leaf",
      {{-0.55, 0.0, -0.1}, {0.55, 2.2, 0.1}}));
  static_cast<void>(app::selectCreativeEditorWorldLayoutCatalogAsset(
      openingAsset, doorAsset));
  openingAsset.catalogPlacement.snapMode =
      app::CreativeEditorWorldLayoutCatalogSnapMode::Wall;
  const app::CreativeEditorWorldLayoutCatalogPlacementPlan doorPlan =
      app::planCreativeEditorWorldLayoutCatalogPlacement(
          openingAsset, {4.0, 2.0}, grid);
  cr::CreativeCatalogEntry windowAsset =
      catalogAsset("window", "architecture/window_frame");
  static_cast<void>(cr::setCreativeHotbarAsset(
      windowAsset.hotbarEntry, "architecture/window_frame",
      {{-0.75, 0.0, -0.05}, {0.75, 1.2, 0.05}}));
  static_cast<void>(app::selectCreativeEditorWorldLayoutCatalogAsset(
      openingAsset, windowAsset));
  openingAsset.catalogPlacement.snapMode =
      app::CreativeEditorWorldLayoutCatalogSnapMode::Wall;
  const app::CreativeEditorWorldLayoutCatalogPlacementPlan windowPlan =
      app::planCreativeEditorWorldLayoutCatalogPlacement(
          openingAsset, {4.0, 2.0}, grid);

  const app::CreativeEditorWorldLayoutPoint diagonalCenterline{2.5, 2.5};
  const double diagonalFaceDistance =
      (diagonalPlan.snapSurfacePoint.x - diagonalCenterline.x) *
          diagonalPlan.snapNormal.x +
      (diagonalPlan.snapSurfacePoint.z - diagonalCenterline.z) *
          diagonalPlan.snapNormal.z;
  const double diagonalPointerSide =
      (2.0 - diagonalCenterline.x) * diagonalPlan.snapNormal.x +
      (3.0 - diagonalCenterline.z) * diagonalPlan.snapNormal.z;
  const double flippedPointerSide =
      (2.0 - diagonalCenterline.x) * flippedPlan.snapNormal.x +
      (3.0 - diagonalCenterline.z) * flippedPlan.snapNormal.z;
  const double sharedFaceDistance =
      (sharedEdge.snapSurfacePoint.x - 4.0) * sharedEdge.snapNormal.x +
      (sharedEdge.snapSurfacePoint.z - 2.0) * sharedEdge.snapNormal.z;
  return expect(diagonalPlan.accepted &&
                    diagonalPlan.snapHostKind ==
                        app::CreativeEditorWorldLayoutCatalogSnapHostKind::
                            ExplicitWall &&
                    diagonalPlan.snapHostIndex == 1U &&
                    near(diagonalPlan.object.pointCells.y, 3.5) &&
                    near(diagonalPlan.object.yawRadians,
                         -std::numbers::pi / 6.0) &&
                    near(diagonalPlan.snapDistanceCells,
                         std::sqrt(0.5)) &&
                    near(diagonalPlan.snapWallThicknessCells, 0.75) &&
                    near(diagonalFaceDistance, 0.375) &&
                    diagonalPointerSide > 0.0 &&
                    restsOnWallFace(diagonalPlan),
                "wall snap filters other levels and seats an asymmetric scaled asset on a reversed diagonal face") &&
         expect(flippedPlan.accepted && restsOnWallFace(flippedPlan) &&
                    near(flippedPlan.snapNormal.x,
                         -diagonalPlan.snapNormal.x) &&
                    near(flippedPlan.snapNormal.z,
                         -diagonalPlan.snapNormal.z) &&
                    near(flippedPlan.snapSurfacePoint.x +
                             diagonalPlan.snapSurfacePoint.x,
                         diagonalCenterline.x * 2.0) &&
                    near(flippedPlan.snapSurfacePoint.z +
                             diagonalPlan.snapSurfacePoint.z,
                         diagonalCenterline.z * 2.0) &&
                    flippedPointerSide < 0.0,
                "wall side flip selects the opposite face without breaking contact") &&
         expect(!noHost.accepted &&
                    noHost.reasonCode ==
                        "creative_editor_world_layout_catalog_wall_missing",
                "wall snap rejects pointers outside its bounded host radius") &&
         expect(sharedEdge.accepted &&
                    sharedEdge.snapHostKind ==
                        app::CreativeEditorWorldLayoutCatalogSnapHostKind::
                            RoomEdge &&
                    sharedEdge.snapHostIndex == 0U &&
                    sharedEdge.snapRoomEdge ==
                        cr::CreativeWorldLayoutRoomEdge::East &&
                    near(sharedEdge.object.pointCells.z, 2.0) &&
                    near(sharedEdge.object.yawRadians,
                         -std::numbers::pi * 0.5) &&
                    near(sharedEdge.snapWallThicknessCells, 0.5) &&
                    near(sharedFaceDistance, 0.25) &&
                    restsOnWallFace(sharedEdge),
                "shared room walls resolve once with their authored face thickness") &&
         expect(doorPlan.accepted && doorPlan.hostedOpening &&
                    doorPlan.openingPlacement.accepted &&
                    near(doorPlan.openingPlacement.opening.widthCells, 2.2) &&
                    doorPlan.reasonCode ==
                        "creative_editor_world_layout_opening_placement_ready",
                "catalog door assets resolve through the hosted opening planner") &&
         expect(!windowPlan.accepted && windowPlan.hostedOpening &&
                    windowPlan.reasonCode ==
                        "creative_editor_world_layout_window_requires_exterior" &&
                    windowPlan.message ==
                        "Place windows on an exterior room edge",
                "catalog windows retain the exterior-wall semantic law");
}

bool catalogOpeningAssetsCompileAsOwnedStructuralInserts() {
  const auto catalogAsset = [](std::string label, std::string assetId,
                               std::string category,
                               cr::CreativeObjectKind kind,
                               cr::CreativeBounds bounds) {
    cr::CreativeCatalogEntry entry;
    entry.category = cr::CreativeCatalogEntryCategory::Asset;
    entry.label = std::move(label);
    entry.assetAuthoringMetadata.categoryId = std::move(category);
    entry.hotbarEntry.objectKind = kind;
    static_cast<void>(
        cr::setCreativeHotbarAsset(entry.hotbarEntry, assetId, bounds));
    return entry;
  };

  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "asset_opening_layout");
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "building_1";
  building.name = "Building";
  state.source.buildings.push_back(building);
  cr::CreativeWorldLayoutLevel level;
  level.buildingIndex = 0U;
  level.stableKey = "level_1";
  level.name = "Ground";
  state.source.levels.push_back(level);
  cr::CreativeWorldLayoutWall wall;
  wall.buildingIndex = 0U;
  wall.stableKey = "wall_1";
  wall.name = "Exterior Wall";
  wall.start = {0, 0};
  wall.end = {8, 0};
  state.source.walls.push_back(wall);
  state.activeLevelIndex = 0U;

  const cr::CreativeCatalogEntry door = catalogAsset(
      "Asymmetric Door", "homestead/modular/door_leaf_1p1x2p2", "door",
      cr::CreativeObjectKind::Door,
      {{-0.2, 0.0, -0.05}, {0.9, 2.2, 0.15}});
  const auto doorSelected =
      app::selectCreativeEditorWorldLayoutCatalogAsset(state, door);
  cr::CreativeGridSettings grid;
  grid.cellSizeMeters = 1.0;
  const app::CreativeEditorWorldLayoutCatalogPlacementPlan doorPreview =
      app::planCreativeEditorWorldLayoutCatalogPlacement(state, {2.0, 0.1},
                                                         grid);
  const auto doorPlaced = app::applyCreativeEditorWorldLayoutPoint(
      state, {2.0, 0.1}, grid);

  const cr::CreativeCatalogEntry window = catalogAsset(
      "Wide Window", "homestead/modular/window_frame_1p5x1p2", "window",
      cr::CreativeObjectKind::Window,
      {{-0.75, 0.0, -0.05}, {0.75, 1.2, 0.05}});
  const auto windowSelected =
      app::selectCreativeEditorWorldLayoutCatalogAsset(state, window);
  state.catalogPlacement.scale = {1.0, 1.25, 2.0};
  const app::CreativeEditorWorldLayoutCatalogPlacementPlan windowPreview =
      app::planCreativeEditorWorldLayoutCatalogPlacement(state, {5.0, 0.1},
                                                         grid);
  const auto windowPlaced = app::applyCreativeEditorWorldLayoutPoint(
      state, {5.0, 0.1}, grid);

  cr::CreativeDocument document =
      cr::CreativeDocument::create("Asset Openings");
  static_cast<void>(document.assignId(8104U));
  static_cast<void>(document.setGridSettings(grid));
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, state.source);
  const cr::CreativeDocumentCreateRequest* doorRequest = nullptr;
  const cr::CreativeDocumentCreateRequest* windowRequest = nullptr;
  for (const cr::CreativeRecipePlan& recipe : compiled.plan.objectRecipes) {
    for (const cr::CreativeRecipeObjectPlan& object : recipe.objects) {
      if (object.createRequest.kind == cr::CreativeObjectKind::Door) {
        doorRequest = &object.createRequest;
      } else if (object.createRequest.kind == cr::CreativeObjectKind::Window) {
        windowRequest = &object.createRequest;
      }
    }
  }
  const cr::CreativeTransformedBounds compiledDoor =
      doorRequest == nullptr
          ? cr::CreativeTransformedBounds{}
          : cr::resolveCreativeTransformedBounds(doorRequest->bounds,
                                                 doorRequest->transform);
  const cr::CreativeTransformedBounds compiledWindow =
      windowRequest == nullptr
          ? cr::CreativeTransformedBounds{}
          : cr::resolveCreativeTransformedBounds(windowRequest->bounds,
                                                 windowRequest->transform);

  return expect(doorSelected.accepted && doorPreview.accepted &&
                    doorPreview.hostedOpening &&
                    doorPreview.openingPlacement.accepted &&
                    near(doorPreview.openingPlacement.opening.widthCells,
                         1.1) &&
                    near(doorPreview.openingPlacement.opening
                             .cutoutHeightCells,
                         2.2) &&
                    doorPlaced.accepted && doorPlaced.changed,
                "catalog door previews and commits through one opening plan") &&
         expect(windowSelected.accepted && windowPreview.accepted &&
                    windowPreview.hostedOpening &&
                    near(windowPreview.openingPlacement.opening.widthCells,
                         1.5) &&
                    near(windowPreview.openingPlacement.opening
                             .cutoutHeightCells,
                         1.5) &&
                    near(windowPreview.openingPlacement.opening
                             .insertThicknessCells,
                         0.2) &&
                    windowPlaced.accepted && windowPlaced.changed,
                "catalog scale deterministically sizes the window cutout") &&
         expect(state.source.objects.empty() &&
                    state.source.openings.size() == 2U &&
                    state.source.openings[0].insertAssetId ==
                        "homestead/modular/door_leaf_1p1x2p2" &&
                    state.source.openings[0].hasInsertAssetSourceBounds &&
                    state.source.openings[1].insertAssetId ==
                        "homestead/modular/window_frame_1p5x1p2",
                "hosted assets are opening-owned instead of duplicate props") &&
         expect(compiled.receipt.accepted && doorRequest != nullptr &&
                    windowRequest != nullptr && compiledDoor.valid &&
                    compiledWindow.valid &&
                    doorRequest->assetId ==
                        "homestead/modular/door_leaf_1p1x2p2" &&
                    doorRequest->hasDoorSettingsOverride &&
                    windowRequest->assetId ==
                        "homestead/modular/window_frame_1p5x1p2" &&
                    windowRequest->hasWindowSettingsOverride &&
                    windowRequest->window == cr::CreativeWindowSettings{} &&
                    near(compiledDoor.size.x, 0.95) &&
                    near(compiledDoor.size.y, 2.11) &&
                    near(compiledDoor.size.z, 0.05) &&
                    near(compiledWindow.size.x, 1.38) &&
                    near(compiledWindow.size.y, 1.38) &&
                    near(compiledWindow.size.z, 0.02),
                "compiled inserts retain identity and fit their physical frames");
}

bool openingInsertReplacementPreservesSemanticOwnership() {
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "opening_insert_replacement");
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Wall));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(state, {0, 0}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(state, {8, 0}));
  const auto wallConfigured = app::setCreativeEditorWorldLayoutWallSettings(
      state, 0U, {{0, 0}, {8, 0}, 0.0, 8U, 0.25});
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Door));
  const auto doorPlaced =
      app::applyCreativeEditorWorldLayoutPoint(state, {4.0, 0.1});

  const cr::CreativeWorldLayoutOpening original = state.source.openings[0];
  app::CreativeEditorWorldLayoutOpeningInsertRequest fit;
  fit.openingIndex = 0U;
  fit.operation =
      app::CreativeEditorWorldLayoutOpeningInsertOperation::FitAssetToOpening;
  fit.assetKind = cr::CreativeBuildingOpeningKind::Door;
  fit.assetId = "homestead/modular/door_leaf_1p1x2p2";
  fit.assetSourceBoundsMeters =
      {{-0.2, 0.0, -0.05}, {0.9, 2.2, 0.15}};
  fit.gridCellSizeMeters = 0.5;
  const std::uint64_t revisionBeforeFit = state.revision;
  const std::size_t undoBeforeFit = state.sourceHistory.undoEntries.size();
  const auto fitted =
      app::applyCreativeEditorWorldLayoutOpeningInsert(state, fit);
  const std::uint64_t revisionAfterFit = state.revision;
  const std::size_t undoAfterFit = state.sourceHistory.undoEntries.size();
  const cr::CreativeWorldLayoutOpening fittedOpening =
      state.source.openings[0];
  const auto fittedAgain =
      app::applyCreativeEditorWorldLayoutOpeningInsert(state, fit);

  app::CreativeEditorWorldLayoutOpeningInsertRequest mismatch = fit;
  mismatch.assetKind = cr::CreativeBuildingOpeningKind::Window;
  const std::uint64_t revisionBeforeMismatch = state.revision;
  const auto mismatched =
      app::applyCreativeEditorWorldLayoutOpeningInsert(state, mismatch);
  const std::uint64_t revisionAfterNoOps = state.revision;
  const std::size_t undoAfterNoOps = state.sourceHistory.undoEntries.size();

  app::CreativeEditorWorldLayoutOpeningInsertRequest resize = fit;
  resize.operation = app::CreativeEditorWorldLayoutOpeningInsertOperation::
      ResizeOpeningToAsset;
  resize.assetScale = {1.0, 1.0, 1.0};
  const std::size_t undoBeforeResize =
      state.sourceHistory.undoEntries.size();
  const auto resized =
      app::applyCreativeEditorWorldLayoutOpeningInsert(state, resize);
  const std::uint64_t revisionAfterResize = state.revision;
  const std::size_t undoAfterResize = state.sourceHistory.undoEntries.size();
  const cr::CreativeWorldLayoutOpening resizedOpening =
      state.source.openings[0];

  app::CreativeEditorWorldLayoutOpeningInsertRequest procedural;
  procedural.openingIndex = 0U;
  procedural.operation = app::CreativeEditorWorldLayoutOpeningInsertOperation::
      UseProceduralInsert;
  const std::size_t undoBeforeProcedural =
      state.sourceHistory.undoEntries.size();
  const auto proceduralApplied =
      app::applyCreativeEditorWorldLayoutOpeningInsert(state, procedural);
  const std::uint64_t revisionAfterProcedural = state.revision;
  const std::size_t undoAfterProcedural =
      state.sourceHistory.undoEntries.size();
  const cr::CreativeWorldLayoutOpening& finalOpening =
      state.source.openings[0];

  return expect(wallConfigured.accepted && doorPlaced.accepted &&
                    doorPlaced.changed,
                "opening insert test creates a valid hosted door") &&
         expect(fitted.accepted && fitted.changed &&
                    revisionAfterFit == revisionBeforeFit + 1U &&
                    undoAfterFit == undoBeforeFit + 1U &&
                    fittedOpening.stableKey == original.stableKey &&
                    fittedOpening.wallIndex == original.wallIndex &&
                    fittedOpening.centerOffsetCells ==
                        original.centerOffsetCells &&
                    fittedOpening.widthCells == original.widthCells &&
                    fittedOpening.insertAssetId == fit.assetId &&
                    fittedOpening.hasInsertAssetSourceBounds,
                "fit mode replaces only the insert and records one source edit") &&
         expect(fittedAgain.accepted && !fittedAgain.changed &&
                    !mismatched.accepted && !mismatched.changed &&
                    revisionAfterNoOps == revisionBeforeMismatch &&
                    undoAfterNoOps == undoAfterFit &&
                    resizedOpening.insertAssetId == fit.assetId,
                "repeat and mismatched replacement attempts do not add edits") &&
         expect(resized.accepted && resized.changed &&
                    revisionAfterResize == revisionAfterNoOps + 1U &&
                    undoAfterResize == undoBeforeResize + 1U &&
                    near(resizedOpening.widthCells, 2.2) &&
                    near(resizedOpening.cutoutHeightCells, 4.4) &&
                    near(resizedOpening.insertThicknessCells, 0.4) &&
                    resizedOpening.stableKey == original.stableKey &&
                    resizedOpening.wallIndex == original.wallIndex &&
                    resizedOpening.centerOffsetCells ==
                        original.centerOffsetCells &&
                    undoBeforeResize + 1U == undoBeforeProcedural,
                "resize mode derives cutout geometry without changing ownership") &&
         expect(proceduralApplied.accepted && proceduralApplied.changed &&
                    revisionAfterProcedural == revisionAfterResize + 1U &&
                    undoAfterProcedural == undoBeforeProcedural + 1U &&
                    finalOpening.includeInsert &&
                    finalOpening.insertAssetId.empty() &&
                    !finalOpening.hasInsertAssetSourceBounds,
                "procedural mode clears catalog identity in one source edit");
}

bool openingsSnapInsideWallsAndRejectOverlap() {
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state);
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Wall));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(state, {0, 0}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(state, {6, 0}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Door));
  const app::CreativeEditorWorldLayoutOpeningPlacementPlan doorPlan =
      app::planCreativeEditorWorldLayoutOpeningPlacement(
          state, {0.05, 0.1}, cr::CreativeBuildingOpeningKind::Door);
  const auto door =
      app::applyCreativeEditorWorldLayoutPoint(state, {0.05, 0.1});
  const double doorCenter = state.source.openings[0].centerOffsetCells;
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Window));
  const app::CreativeEditorWorldLayoutOpeningPlacementPlan overlapPlan =
      app::planCreativeEditorWorldLayoutOpeningPlacement(
          state, {0.5, 0.1}, cr::CreativeBuildingOpeningKind::Window);
  const auto overlap =
      app::applyCreativeEditorWorldLayoutPoint(state, {0.5, 0.1});
  const std::size_t countAfterOverlap = state.source.openings.size();
  const app::CreativeEditorWorldLayoutOpeningPlacementPlan windowPlan =
      app::planCreativeEditorWorldLayoutOpeningPlacement(
          state, {5.8, 0.1}, cr::CreativeBuildingOpeningKind::Window);
  const auto window =
      app::applyCreativeEditorWorldLayoutPoint(state, {5.8, 0.1});

  return expect(doorPlan.accepted && door.accepted && door.changed &&
                    doorCenter == doorPlan.opening.centerOffsetCells &&
                    near(doorPlan.centerPoint.x, 0.75) &&
                    near(doorPlan.startPoint.x, 0.25) &&
                    near(doorPlan.endPoint.x, 1.25) &&
                    near(doorPlan.pointerDistanceCells, 0.1),
                "door preview and mutation share the quarter-cell host plan") &&
         expect(
             !overlapPlan.accepted &&
                 overlapPlan.reasonCode ==
                     "creative_editor_world_layout_opening_overlap" &&
                 !overlap.accepted && !overlap.changed &&
                 countAfterOverlap == 1U,
             "overlap is rejected by preview and commit without a source edit") &&
         expect(windowPlan.accepted && window.accepted &&
                    state.source.openings[1].centerOffsetCells ==
                        windowPlan.opening.centerOffsetCells &&
                    near(windowPlan.centerPoint.x, 5.0) &&
                    near(windowPlan.startPoint.x, 4.25) &&
                    near(windowPlan.endPoint.x, 5.75),
                "window preview and mutation preserve the far wall pier");
}

bool openingPlacementPlansRespectActiveLevelsAndSharedEdges() {
  app::CreativeEditorWorldLayoutState levels;
  app::resetCreativeEditorWorldLayout(levels, "opening_level_plan");
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "building";
  building.name = "Building";
  levels.source.buildings.push_back(building);
  cr::CreativeWorldLayoutLevel ground;
  ground.buildingIndex = 0U;
  ground.stableKey = "ground";
  ground.name = "Ground";
  ground.floorTopLayer = 0.0;
  levels.source.levels.push_back(ground);
  cr::CreativeWorldLayoutLevel upper = ground;
  upper.stableKey = "upper";
  upper.name = "Upper";
  upper.floorTopLayer = 3.0;
  levels.source.levels.push_back(upper);
  levels.activeLevelIndex = 0U;
  cr::CreativeWorldLayoutWall upperWall;
  upperWall.buildingIndex = 0U;
  upperWall.stableKey = "upper_wall";
  upperWall.name = "Upper Wall";
  upperWall.start = {0, 0};
  upperWall.end = {6, 0};
  upperWall.baseLayer = 3.0;
  levels.source.walls.push_back(upperWall);
  cr::CreativeWorldLayoutWall groundWall = upperWall;
  groundWall.stableKey = "ground_wall";
  groundWall.name = "Ground Wall";
  groundWall.start = {0, 2};
  groundWall.end = {6, 2};
  groundWall.baseLayer = 0.0;
  levels.source.walls.push_back(groundWall);
  cr::CreativeWorldLayoutWall lowWall = groundWall;
  lowWall.stableKey = "low_wall";
  lowWall.name = "Low Wall";
  lowWall.start = {0, 4};
  lowWall.end = {6, 4};
  lowWall.heightCells = 2U;
  levels.source.walls.push_back(lowWall);
  cr::CreativeWorldLayoutWall diagonalWall = groundWall;
  diagonalWall.stableKey = "diagonal_wall";
  diagonalWall.name = "Diagonal Wall";
  diagonalWall.start = {0, 6};
  diagonalWall.end = {6, 12};
  levels.source.walls.push_back(diagonalWall);
  const auto wrongLevel = app::planCreativeEditorWorldLayoutOpeningPlacement(
      levels, {3.0, 0.0}, cr::CreativeBuildingOpeningKind::Door);
  const auto groundPlan = app::planCreativeEditorWorldLayoutOpeningPlacement(
      levels, {3.0, 2.1}, cr::CreativeBuildingOpeningKind::Door);
  const auto lowWallPlan = app::planCreativeEditorWorldLayoutOpeningPlacement(
      levels, {3.0, 4.0}, cr::CreativeBuildingOpeningKind::Door);
  const auto diagonalPlan = app::planCreativeEditorWorldLayoutOpeningPlacement(
      levels, {3.0, 9.0}, cr::CreativeBuildingOpeningKind::Door);

  app::CreativeEditorWorldLayoutState rooms;
  app::resetCreativeEditorWorldLayout(rooms, "shared_opening_plan");
  rooms.source.buildings.push_back(building);
  rooms.source.levels.push_back(ground);
  rooms.activeLevelIndex = 0U;
  cr::CreativeWorldLayoutRoom first;
  first.buildingIndex = 0U;
  first.levelIndex = 0U;
  first.stableKey = "first";
  first.name = "First";
  first.footprint = {{0, 0}, {4, 4}};
  rooms.source.rooms.push_back(first);
  cr::CreativeWorldLayoutRoom second = first;
  second.stableKey = "second";
  second.name = "Second";
  second.footprint = {{4, 0}, {8, 4}};
  rooms.source.rooms.push_back(second);
  const auto interiorWindow =
      app::planCreativeEditorWorldLayoutOpeningPlacement(
          rooms, {4.0, 2.0}, cr::CreativeBuildingOpeningKind::Window);
  const auto interiorDoor = app::planCreativeEditorWorldLayoutOpeningPlacement(
      rooms, {4.0, 2.0}, cr::CreativeBuildingOpeningKind::Door);
  cr::CreativeWorldLayoutOpening oppositeEdgeDoor;
  oppositeEdgeDoor.hostKind =
      cr::CreativeWorldLayoutOpeningHostKind::RoomEdge;
  oppositeEdgeDoor.roomIndex = 1U;
  oppositeEdgeDoor.roomEdge = cr::CreativeWorldLayoutRoomEdge::West;
  oppositeEdgeDoor.kind = cr::CreativeBuildingOpeningKind::Door;
  oppositeEdgeDoor.centerOffsetCells = 2.0;
  oppositeEdgeDoor.widthCells = 1.0;
  rooms.source.openings.push_back(oppositeEdgeDoor);
  const auto sharedOverlap =
      app::planCreativeEditorWorldLayoutOpeningPlacement(
          rooms, {4.0, 2.0}, cr::CreativeBuildingOpeningKind::Door);

  return expect(!wrongLevel.accepted &&
                    wrongLevel.reasonCode ==
                        "creative_editor_world_layout_wall_not_found" &&
                    groundPlan.accepted &&
                    groundPlan.opening.wallIndex == 1U,
                "opening planning ignores explicit walls from other levels") &&
         expect(!lowWallPlan.accepted &&
                    lowWallPlan.reasonCode ==
                        "creative_editor_world_layout_opening_height_invalid",
                "opening planning rejects cutouts taller than their host") &&
         expect(!diagonalPlan.accepted &&
                    diagonalPlan.reasonCode ==
                        "creative_editor_world_layout_opening_host_orientation_unsupported",
                "opening planning rejects walls the structural compiler cannot cut") &&
         expect(!interiorWindow.accepted &&
                    interiorWindow.reasonCode ==
                        "creative_editor_world_layout_window_requires_exterior" &&
                    interiorDoor.accepted &&
                    interiorDoor.opening.hostKind ==
                        cr::CreativeWorldLayoutOpeningHostKind::RoomEdge &&
                    interiorDoor.opening.roomIndex == 0U &&
                    interiorDoor.opening.roomEdge ==
                        cr::CreativeWorldLayoutRoomEdge::East &&
                    !sharedOverlap.accepted &&
                    sharedOverlap.reasonCode ==
                        "creative_editor_world_layout_opening_overlap",
                "shared room edges accept doors, reject windows, and detect opposite-edge overlap");
}

bool deletingWallCascadesItsOpenings() {
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state);
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Wall));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(state, {0, 0}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(state, {8, 0}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Door));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(state, {4, 0}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Select));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(state, {1, 0}));
  const auto removed = app::deleteCreativeEditorWorldLayoutSelection(state);
  return expect(removed.accepted && removed.changed,
                "selected wall deletion accepted") &&
         expect(state.source.walls.empty() && state.source.openings.empty(),
                "wall deletion cascades dependent openings");
}

bool roomGestureHostsOpeningsAndSupportsResize() {
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state);
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Room));
  const auto begin = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {0, 0});
  const auto commit = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {6, 4});
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Door));
  const auto door =
      app::applyCreativeEditorWorldLayoutPoint(state, {3.0, 0.1});
  app::CreativeEditorWorldLayoutRoomSettings settings{
      {{0, 0}, {8, 5}}, 2.0, 5U, 0.5, 2U};
  settings.roofThicknessLayers = 2U;
  settings.roofStyle = cr::CreativeStructuralRoofStyle::Gable;
  settings.roofRidgeAxis = cr::CreativeStructuralRoofRidgeAxis::Z;
  settings.roofPitchDegrees = 35.0;
  settings.roofOverhangCells = 0.5;
  const auto updated = app::setCreativeEditorWorldLayoutRoomSettings(
      state, 0U, settings);

  return expect(begin.accepted && !begin.changed && commit.accepted &&
                    commit.changed,
                "room drag mutates only when committed") &&
         expect(state.source.rooms.size() == 1U &&
                    state.source.boxes.empty() && state.source.walls.empty(),
                "room remains semantic source instead of sprayed symbols") &&
         expect(door.accepted &&
                    state.source.openings[0].hostKind ==
                        cr::CreativeWorldLayoutOpeningHostKind::RoomEdge &&
                    state.source.openings[0].roomEdge ==
                        cr::CreativeWorldLayoutRoomEdge::North,
                "door slots into a semantic room edge") &&
         expect(updated.accepted && updated.changed &&
                    state.source.rooms[0].footprint.maximum ==
                        cr::CreativeTerrainCoord2{8, 5} &&
                    state.source.levels[state.source.rooms[0].levelIndex]
                            .floorTopLayer == 2.0 &&
                    state.source.levels[state.source.rooms[0].levelIndex]
                            .wallHeightCells == 5U &&
                    state.source.rooms[0].wallThicknessCells == 0.5 &&
                    state.source.levels[state.source.rooms[0].levelIndex]
                            .floorThicknessLayers == 2U &&
                    state.source.levels[state.source.rooms[0].levelIndex]
                            .roofThicknessLayers == 2U &&
                    state.source.levels[state.source.rooms[0].levelIndex]
                            .roofStyle ==
                        cr::CreativeStructuralRoofStyle::Gable &&
                    state.source.levels[state.source.rooms[0].levelIndex]
                            .roofRidgeAxis ==
                        cr::CreativeStructuralRoofRidgeAxis::Z &&
                    state.source.levels[state.source.rooms[0].levelIndex]
                            .roofPitchDegrees == 35.0 &&
                    state.source.levels[state.source.rooms[0].levelIndex]
                            .roofOverhangCells == 0.5,
                "selected room and shared roof settings change as one source edit");
}

bool buildingShellCreatesOwnedRoomAndGeneratesAsOneEdit() {
  cr::CreativeAppState live = appState();
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state);
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::BuildingShell));
  const std::uint64_t revisionBefore = state.revision;
  const std::uint64_t ordinalBefore = state.nextStableOrdinal;
  const auto begin = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {7, 5});
  const auto commit = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {1, 1});

  const bool ownedShell =
      begin.accepted && !begin.changed && commit.accepted && commit.changed &&
      state.revision == revisionBefore + 1U &&
      state.nextStableOrdinal == ordinalBefore + 3U &&
      state.source.buildings.size() == 1U && state.source.levels.size() == 1U &&
      state.source.rooms.size() == 1U &&
      state.source.levels[0].buildingIndex == 0U &&
      state.source.rooms[0].buildingIndex == 0U &&
      state.source.rooms[0].levelIndex == 0U &&
      state.source.rooms[0].footprint.minimum ==
          cr::CreativeTerrainCoord2{1, 1} &&
      state.source.rooms[0].footprint.maximum ==
          cr::CreativeTerrainCoord2{7, 5} &&
      state.source.buildings[0].stableKey != state.source.levels[0].stableKey &&
      state.source.levels[0].stableKey != state.source.rooms[0].stableKey;

  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Door));
  const auto door =
      app::applyCreativeEditorWorldLayoutPoint(state, {4.0, 1.1});
  const bool hostedDoor =
      door.accepted && door.changed && state.source.openings.size() == 1U &&
      state.source.openings[0].hostKind ==
          cr::CreativeWorldLayoutOpeningHostKind::RoomEdge &&
      state.source.openings[0].roomIndex == 0U;

  const std::uint64_t liveCountBefore = live.facade.document().objectCount();
  const auto preview =
      app::previewCreativeEditorWorldLayout(state, live.facade.document());
  std::uint64_t floorCount = 0U;
  std::uint64_t doorCount = 0U;
  for (const cr::CreativeObject& object : state.preview.document.objects()) {
    floorCount += object.kind == cr::CreativeObjectKind::Floor ? 1U : 0U;
    doorCount += object.kind == cr::CreativeObjectKind::Door ? 1U : 0U;
  }
  const bool exactPreview =
      preview.accepted && floorCount == 1U && doorCount == 1U &&
      live.facade.document().objectCount() == liveCountBefore;
  const std::uint64_t previewObjectCount =
      state.preview.document.objectCount();

  const auto generated = app::confirmCreativeEditorWorldLayout(state, live);
  const bool generatedOnce =
      generated.accepted && generated.changed &&
      cr::creativeUndoDepth(live.history) == 1U &&
      live.facade.document().objectCount() == previewObjectCount;
  const bool undone = app::undoLastEdit(live, "building-shell-undo", &state);
  const bool restored = undone && state.source.buildings.empty() &&
                        state.source.rooms.empty() &&
                        state.source.openings.empty() &&
                        live.facade.document().objectCount() == liveCountBefore;

  return expect(ownedShell,
                "building shell publishes one owned room in one revision") &&
         expect(hostedDoor,
                "door symbol remains hosted by the building shell room") &&
         expect(exactPreview,
                "building shell exact preview does not publish live objects") &&
         expect(generatedOnce,
                "building shell generation records exactly one undo edit") &&
         expect(restored,
                "building shell undo restores semantic and 3D state together");
}

bool rejectedBuildingShellIsTransactionallyEmpty() {
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state);
  const std::uint64_t revisionBefore = state.revision;
  const std::uint64_t ordinalBefore = state.nextStableOrdinal;
  const auto degenerate = app::createCreativeEditorWorldLayoutBuildingShell(
      state, {{{2, 2}, {2, 8}}, 0.0, 3U, 0.25, 1U});
  const auto consumed = app::createCreativeEditorWorldLayoutBuildingShell(
      state, {{{0, 0}, {2, 2}}, 0.0, 3U, 1.0, 1U});
  return expect(!degenerate.accepted && !degenerate.changed &&
                    !consumed.accepted && !consumed.changed,
                "invalid building shell candidates are rejected") &&
         expect(state.source.buildings.empty() && state.source.rooms.empty() &&
                    state.revision == revisionBefore &&
                    state.nextStableOrdinal == ordinalBefore,
                "rejected shells consume no source revision or stable key");
}

bool buildingShellsKeepIndependentBuildingOwnership() {
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state);
  const auto first = app::createCreativeEditorWorldLayoutBuildingShell(
      state, {{{0, 0}, {6, 4}}, 0.0, 3U, 0.25, 1U});
  const auto second = app::createCreativeEditorWorldLayoutBuildingShell(
      state, {{{10, 2}, {15, 7}}, 1.0, 4U, 0.5, 2U});
  return expect(first.accepted && first.changed && second.accepted &&
                    second.changed,
                "independent building shells are accepted") &&
         expect(state.source.buildings.size() == 2U &&
                    state.source.rooms.size() == 2U &&
                    state.source.rooms[0].buildingIndex == 0U &&
                    state.source.rooms[1].buildingIndex == 1U &&
                    state.source.levels[state.source.rooms[1].levelIndex]
                            .floorTopLayer == 1.0 &&
                    state.source.levels[state.source.rooms[1].levelIndex]
                            .wallHeightCells == 4U &&
                    state.source.rooms[1].wallThicknessCells == 0.5 &&
                    state.source.levels[state.source.rooms[1].levelIndex]
                            .floorThicknessLayers == 2U &&
                    stableKeysUnique(state.source),
                "each shell owns its room and authored dimensions");
}

bool addRoomTargetsSelectedBuildingAndRejectsAmbiguousOwnership() {
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state);
  static_cast<void>(app::createCreativeEditorWorldLayoutBuildingShell(
      state, {{{0, 0}, {4, 4}}, 0.0, 3U, 0.25, 1U}));
  static_cast<void>(app::createCreativeEditorWorldLayoutBuildingShell(
      state, {{{10, 0}, {14, 4}}, 0.0, 3U, 0.25, 1U}));
  const auto selected =
      app::selectCreativeEditorWorldLayoutBuilding(state, 1U);
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Room));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {14, 0}));
  const auto added = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {18, 4});

  static_cast<void>(app::clearCreativeEditorWorldLayoutSelection(state));
  const std::uint64_t revisionBeforeReject = state.revision;
  const std::uint64_t ordinalBeforeReject = state.nextStableOrdinal;
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {20, 0}));
  const auto ambiguous = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {24, 4});

  return expect(selected.accepted && added.accepted && added.changed &&
                    state.source.rooms.size() == 3U &&
                    state.source.rooms[2].buildingIndex == 1U,
                "Add Room attaches to the explicitly selected building") &&
         expect(!ambiguous.accepted && !ambiguous.changed &&
                    state.revision == revisionBeforeReject &&
                    state.nextStableOrdinal == ordinalBeforeReject,
                "Add Room rejects ambiguous multi-building ownership");
}

bool roomAdditionCannotInternalizeExistingWindow() {
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state);
  static_cast<void>(app::createCreativeEditorWorldLayoutBuildingShell(
      state, {{{0, 0}, {4, 4}}, 0.0, 3U, 0.25, 1U}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Window));
  const auto window =
      app::applyCreativeEditorWorldLayoutPoint(state, {4, 2});
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Room));
  const std::uint64_t revisionBefore = state.revision;
  const std::uint64_t ordinalBefore = state.nextStableOrdinal;
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {4, 0}));
  const auto room = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {8, 4});

  return expect(window.accepted && window.changed,
                "window begins on an exterior room edge") &&
         expect(!room.accepted && !room.changed &&
                    state.source.rooms.size() == 1U &&
                    state.source.openings.size() == 1U &&
                    state.revision == revisionBefore &&
                    state.nextStableOrdinal == ordinalBefore,
                "room addition cannot silently internalize a window");
}

bool fourRoomBuildingRoundTripsAsOneGeneratedEdit() {
  cr::CreativeAppState live = appState();
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state);
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::BuildingShell));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {0, 0}));
  const auto shell = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {4, 4});

  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Room));
  const auto addRoom = [&](app::CreativeEditorWorldLayoutPoint first,
                           app::CreativeEditorWorldLayoutPoint second) {
    const auto begin = app::applyCreativeEditorWorldLayoutGesture(
        state, app::CreativeEditorWorldLayoutGesturePhase::Begin, first);
    const auto commit = app::applyCreativeEditorWorldLayoutGesture(
        state, app::CreativeEditorWorldLayoutGesturePhase::Commit, second);
    return begin.accepted && !begin.changed && commit.accepted &&
           commit.changed;
  };
  const bool roomsAdded = addRoom({4, 0}, {8, 4}) &&
                          addRoom({0, 4}, {4, 8}) &&
                          addRoom({4, 4}, {8, 8});

  const auto sharedEdges =
      cr::inspectCreativeWorldLayoutSharedRoomEdges(state.source);
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Door));
  const auto interiorDoor =
      app::applyCreativeEditorWorldLayoutPoint(state, {4, 2});
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Window));
  const std::uint64_t revisionBeforeInteriorWindow = state.revision;
  const auto interiorWindow =
      app::applyCreativeEditorWorldLayoutPoint(state, {4, 6});
  const auto exteriorWindow =
      app::applyCreativeEditorWorldLayoutPoint(state, {2, 0});

  const cr::CreativeWorldLayoutRoomCompileResult expanded =
      cr::expandCreativeWorldLayoutRooms(state.source);
  const cr::CreativeWorldLayoutEncodeResult encoded =
      cr::encodeCreativeWorldLayout(state.source);
  const cr::CreativeWorldLayoutDecodeResult decoded =
      cr::decodeCreativeWorldLayout(encoded.encodedText);
  const std::uint64_t liveCountBefore = live.facade.document().objectCount();
  const auto preview =
      app::previewCreativeEditorWorldLayout(state, live.facade.document());
  std::uint64_t floorCount = 0U;
  std::uint64_t doorCount = 0U;
  std::uint64_t windowCount = 0U;
  for (const cr::CreativeObject& object : state.preview.document.objects()) {
    floorCount += object.kind == cr::CreativeObjectKind::Floor ? 1U : 0U;
    doorCount += object.kind == cr::CreativeObjectKind::Door ? 1U : 0U;
    windowCount += object.kind == cr::CreativeObjectKind::Window ? 1U : 0U;
  }
  const std::uint64_t previewObjectCount = state.preview.document.objectCount();
  const auto generated = app::confirmCreativeEditorWorldLayout(state, live);
  const bool undone = app::undoLastEdit(live, "four-room-undo", &state);
  const bool undoRestored =
      undone && state.source.buildings.empty() && state.source.rooms.empty() &&
      live.facade.document().objectCount() == liveCountBefore;
  const bool redone = app::redoLastEdit(live, "four-room-redo", &state);

  return expect(shell.accepted && shell.changed && roomsAdded &&
                    state.source.buildings.size() == 1U &&
                    state.source.rooms.size() == 4U &&
                    std::all_of(state.source.rooms.begin(),
                                state.source.rooms.end(),
                                [](const auto& room) {
                                  return room.buildingIndex == 0U;
                                }) &&
                    state.source.buildings[0].rootFootprint.minimum ==
                        cr::CreativeTerrainCoord2{0, 0} &&
                    state.source.buildings[0].rootFootprint.maximum ==
                        cr::CreativeTerrainCoord2{8, 8} &&
                    sharedEdges.size() == 4U,
                "four adjoining rooms remain one building with four shared "
                "spans") &&
         expect(
             interiorDoor.accepted && interiorDoor.changed &&
                 !interiorWindow.accepted && !interiorWindow.changed &&
                 state.revision == revisionBeforeInteriorWindow + 1U &&
                 exteriorWindow.accepted && exteriorWindow.changed &&
                 state.source.openings.size() == 2U,
             "doors connect rooms while windows default to exterior walls") &&
         expect(expanded.accepted && expanded.expanded.walls.size() == 6U,
                "four-room topology condenses to six canonical wall lanes") &&
         expect(encoded.accepted && decoded.accepted &&
                    decoded.layout.buildings.size() == 1U &&
                    decoded.layout.rooms.size() == 4U &&
                    decoded.layout.openings.size() == 2U,
                "four-room semantic source survives codec round trip") &&
         expect(preview.accepted && floorCount == 4U && doorCount == 1U &&
                    windowCount == 1U && liveCountBefore == 0U,
                "exact preview contains four floors and authored openings") &&
         expect(generated.accepted && generated.changed &&
                    cr::creativeUndoDepth(live.history) == 1U &&
                    live.facade.document().objectCount() == previewObjectCount,
                "four-room building generates as one history edit") &&
         expect(undoRestored && redone && state.source.rooms.size() == 4U &&
                    state.source.openings.size() == 2U &&
                    live.facade.document().objectCount() == previewObjectCount,
                "undo and redo restore semantic and rendered building state");
}

bool invalidRoomShellSettingsFailWithoutMutation() {
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state);
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Room));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {0, 0}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {4, 4}));
  const std::uint64_t revisionBefore = state.revision;
  const auto rejected = app::setCreativeEditorWorldLayoutRoomSettings(
      state, 0U, {{{0, 0}, {4, 4}}, 0, 3U, 2.0, 1U});
  const bool rejectedAtomic =
      state.revision == revisionBefore &&
      state.source.rooms[0].wallThicknessCells ==
          cr::kDefaultCreativeWorldLayoutWallThicknessCells;

  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Door));
  const auto door =
      app::applyCreativeEditorWorldLayoutPoint(state, {2.0, 0.1});
  app::CreativeEditorWorldLayoutRoomSettings clippedSettings;
  const bool read = app::readCreativeEditorWorldLayoutRoomSettings(
      state, 0U, clippedSettings);
  clippedSettings.wallHeightCells = 1U;
  const std::uint64_t heightRevisionBefore = state.revision;
  const auto clipped = app::setCreativeEditorWorldLayoutRoomSettings(
      state, 0U, clippedSettings);

  return expect(!rejected.accepted && !rejected.changed,
                "room shell rejects walls that consume the interior") &&
         expect(rejectedAtomic,
                "invalid room shell settings do not mutate source truth") &&
         expect(door.accepted && door.changed && read && !clipped.accepted &&
                    !clipped.changed && state.revision == heightRevisionBefore &&
                    state.source.levels[0].wallHeightCells == 3U,
                "room settings cannot clip a hosted opening vertically");
}

bool roomMovePreviewCommitsOnceAndKeepsOpeningHosted() {
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state);
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Room));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {0, 0}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {6, 4}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Door));
  static_cast<void>(
      app::applyCreativeEditorWorldLayoutPoint(state, {3.0, 0.1}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Select));

  const auto centerTarget = app::findCreativeEditorWorldLayoutRoomTarget(
      state, {2.0, 2.0}, 0.3);
  const auto cornerTarget = app::findCreativeEditorWorldLayoutRoomTarget(
      state, {0.0, 0.0}, 0.3);
  const auto openingTarget = app::findCreativeEditorWorldLayoutRoomTarget(
      state, {3.0, 0.1}, 0.3);
  const std::uint64_t revisionBefore = state.revision;
  const auto begin = app::applyCreativeEditorWorldLayoutRoomManipulation(
      state, app::CreativeEditorWorldLayoutRoomManipulationPhase::Begin,
      {2.49, 2.49}, 0.3);
  const auto smallUpdate = app::applyCreativeEditorWorldLayoutRoomManipulation(
      state, app::CreativeEditorWorldLayoutRoomManipulationPhase::Update,
      {2.51, 2.51}, 0.3);
  const auto update = app::applyCreativeEditorWorldLayoutRoomManipulation(
      state, app::CreativeEditorWorldLayoutRoomManipulationPhase::Update,
      {5.49, 4.49}, 0.3);
  const bool previewOnly =
      state.source.rooms[0].footprint.minimum ==
          cr::CreativeTerrainCoord2{0, 0} &&
      state.source.rooms[0].footprint.maximum ==
          cr::CreativeTerrainCoord2{6, 4} &&
      state.roomManipulation.previewFootprint.minimum ==
          cr::CreativeTerrainCoord2{3, 2} &&
      state.roomManipulation.previewFootprint.maximum ==
          cr::CreativeTerrainCoord2{9, 6} &&
      state.roomManipulation.previewValid && state.revision == revisionBefore;
  const auto commit = app::applyCreativeEditorWorldLayoutRoomManipulation(
      state, app::CreativeEditorWorldLayoutRoomManipulationPhase::Commit,
      {5.49, 4.49}, 0.3);

  return expect(centerTarget.handle ==
                        app::CreativeEditorWorldLayoutRoomHandle::Move &&
                    cornerTarget.handle ==
                        app::CreativeEditorWorldLayoutRoomHandle::NorthWest &&
                    openingTarget.handle ==
                        app::CreativeEditorWorldLayoutRoomHandle::None,
                "room handles do not steal a hosted opening") &&
         expect(begin.accepted && begin.changed && smallUpdate.accepted &&
                    !smallUpdate.changed && update.accepted && update.changed &&
                    previewOnly,
                "room move snaps press-relative deltas into preview only") &&
         expect(commit.accepted && commit.changed &&
                    state.revision == revisionBefore + 1U &&
                    !state.roomManipulation.active &&
                    state.source.rooms[0].footprint.minimum ==
                        cr::CreativeTerrainCoord2{3, 2} &&
                    state.source.rooms[0].footprint.maximum ==
                        cr::CreativeTerrainCoord2{9, 6},
                "room move commits one source revision") &&
         expect(state.source.openings.size() == 1U &&
                    state.source.openings[0].hostKind ==
                        cr::CreativeWorldLayoutOpeningHostKind::RoomEdge &&
                    state.source.openings[0].roomIndex == 0U &&
                    state.source.openings[0].roomEdge ==
                        cr::CreativeWorldLayoutRoomEdge::North &&
                    near(state.source.openings[0].centerOffsetCells, 3.0),
                "room move preserves the opening's semantic attachment");
}

bool sharedRoomBoundaryPreviewCommitsAsOneRelationalEdit() {
  cr::CreativeAppState live = appState();
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "relational_room_edit");
  static_cast<void>(app::createCreativeEditorWorldLayoutBuildingShell(
      state, {{{0, 0}, {4, 4}}, 0.0, 3U, 0.25, 1U}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Room));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {4, 0}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {8, 4}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Door));
  const auto door =
      app::applyCreativeEditorWorldLayoutPoint(state, {4.0, 3.0});
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Window));
  const auto window =
      app::applyCreativeEditorWorldLayoutPoint(state, {6.0, 0.1});
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Select));

  const std::uint64_t revisionBefore = state.revision;
  const auto began = app::applyCreativeEditorWorldLayoutRoomManipulation(
      state, app::CreativeEditorWorldLayoutRoomManipulationPhase::Begin,
      {3.9, 1.0}, 0.3);
  const auto updated = app::applyCreativeEditorWorldLayoutRoomManipulation(
      state, app::CreativeEditorWorldLayoutRoomManipulationPhase::Update,
      {4.9, 1.0}, 0.3);
  const cr::CreativeWorldLayout& display =
      app::creativeEditorWorldLayoutDisplaySource(state);
  const bool exactPreview =
      display.rooms[0].footprint.maximum.x == 5 &&
      display.rooms[1].footprint.minimum.x == 5 &&
      near(display.openings[1].centerOffsetCells, 1.0) &&
      state.source.rooms[0].footprint.maximum.x == 4 &&
      state.source.rooms[1].footprint.minimum.x == 4 &&
      near(state.source.openings[1].centerOffsetCells, 2.0) &&
      state.roomManipulation.previewEdit.roomChanges.size() == 2U &&
      state.revision == revisionBefore;
  const auto committed = app::applyCreativeEditorWorldLayoutRoomManipulation(
      state, app::CreativeEditorWorldLayoutRoomManipulationPhase::Commit,
      {4.9, 1.0}, 0.3);
  const bool undo = app::undoLastEdit(live, "shared-boundary-undo", &state);
  const bool undoRestored =
      undo && state.source.rooms[0].footprint.maximum.x == 4 &&
      state.source.rooms[1].footprint.minimum.x == 4 &&
      near(state.source.openings[1].centerOffsetCells, 2.0);
  const bool redo = app::redoLastEdit(live, "shared-boundary-redo", &state);
  app::CreativeEditorWorldLayoutState deletionState = state;
  deletionState.selection = {
      app::CreativeEditorWorldLayoutSelectionKind::Room, 1U};
  const auto deleted =
      app::deleteCreativeEditorWorldLayoutSelection(deletionState);
  const bool deletionRefreshesExtent =
      deleted.accepted && deleted.changed &&
      deletionState.source.rooms.size() == 1U &&
      deletionState.source.buildings[0].rootFootprint.maximum ==
          cr::CreativeTerrainCoord2{5, 4};
  const cr::CreativeWorldLayoutEncodeResult encoded =
      cr::encodeCreativeWorldLayout(state.source);
  const cr::CreativeWorldLayoutDecodeResult decoded =
      cr::decodeCreativeWorldLayout(encoded.encodedText);
  const auto preview =
      app::previewCreativeEditorWorldLayout(state, live.facade.document());

  return expect(door.accepted && door.changed && window.accepted &&
                    window.changed && state.source.openings.size() == 2U,
                "relational edit fixture owns an interior door and exterior window") &&
         expect(began.accepted && began.changed && updated.accepted &&
                    updated.changed && exactPreview,
                "shared boundary preview moves both owners without source mutation") &&
         expect(committed.accepted && committed.changed &&
                    state.revision == revisionBefore + 1U &&
                    state.source.rooms[0].footprint.maximum.x == 5 &&
                    state.source.rooms[1].footprint.minimum.x == 5 &&
                    near(state.source.openings[1].centerOffsetCells, 1.0) &&
                    state.source.buildings[0].rootFootprint.maximum ==
                        cr::CreativeTerrainCoord2{8, 4},
                "shared boundary and hosted openings commit as one source revision") &&
         expect(undoRestored && redo &&
                    state.source.rooms[0].footprint.maximum.x == 5 &&
                    state.source.rooms[1].footprint.minimum.x == 5 &&
                    deletionRefreshesExtent,
                "undo redo and deletion preserve complete derived topology") &&
         expect(encoded.accepted && decoded.accepted &&
                    decoded.layout.rooms[0].footprint.maximum.x == 5 &&
                    decoded.layout.rooms[1].footprint.minimum.x == 5 &&
                    near(decoded.layout.openings[1].centerOffsetCells, 1.0) &&
                    preview.accepted,
                "edited topology round-trips and generates exact 3D output");
}

bool roomSettingsUseTheRelationalTopologyOwner() {
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "relational_room_settings");
  static_cast<void>(app::createCreativeEditorWorldLayoutBuildingShell(
      state, {{{0, 0}, {4, 4}}, 0.0, 3U, 0.25, 1U}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Room));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {4, 0}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {8, 4}));

  app::CreativeEditorWorldLayoutRoomSettings settings;
  const bool read = app::readCreativeEditorWorldLayoutRoomSettings(
      state, 0U, settings);
  settings.footprint.maximum.x = 5;
  const std::uint64_t revisionBefore = state.revision;
  const auto edited = app::setCreativeEditorWorldLayoutRoomSettings(
      state, 0U, settings);

  return expect(read && edited.accepted && edited.changed &&
                    state.revision == revisionBefore + 1U,
                "room settings commit one relational source revision") &&
         expect(state.source.rooms[0].footprint.maximum.x == 5 &&
                    state.source.rooms[1].footprint.minimum.x == 5 &&
                    cr::inspectCreativeWorldLayoutSharedRoomEdges(state.source)
                            .size() == 1U,
                "numeric room settings retain the shared boundary");
}

bool roomEdgesAndCornersResizeFromTheirOwnedSides() {
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state);
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Room));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {0, 0}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {6, 4}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Select));

  const std::uint64_t revisionBeforeEdge = state.revision;
  const auto edgeBegin = app::applyCreativeEditorWorldLayoutRoomManipulation(
      state, app::CreativeEditorWorldLayoutRoomManipulationPhase::Begin,
      {6.0, 2.0}, 0.3);
  const auto edgeCommit = app::applyCreativeEditorWorldLayoutRoomManipulation(
      state, app::CreativeEditorWorldLayoutRoomManipulationPhase::Commit,
      {8.0, 2.0}, 0.3);
  const std::uint64_t revisionBeforeCorner = state.revision;
  const auto cornerBegin = app::applyCreativeEditorWorldLayoutRoomManipulation(
      state, app::CreativeEditorWorldLayoutRoomManipulationPhase::Begin,
      {0.0, 0.0}, 0.3);
  const auto cornerCommit = app::applyCreativeEditorWorldLayoutRoomManipulation(
      state, app::CreativeEditorWorldLayoutRoomManipulationPhase::Commit,
      {-2.0, -1.0}, 0.3);
  const std::uint64_t revisionBeforeCancel = state.revision;
  static_cast<void>(app::applyCreativeEditorWorldLayoutRoomManipulation(
      state, app::CreativeEditorWorldLayoutRoomManipulationPhase::Begin,
      {2.0, 2.0}, 0.3));
  static_cast<void>(app::applyCreativeEditorWorldLayoutRoomManipulation(
      state, app::CreativeEditorWorldLayoutRoomManipulationPhase::Update,
      {3.0, 3.0}, 0.3));
  const auto cancelled = app::applyCreativeEditorWorldLayoutRoomManipulation(
      state, app::CreativeEditorWorldLayoutRoomManipulationPhase::Cancel);

  return expect(edgeBegin.accepted && edgeCommit.accepted &&
                    edgeCommit.changed &&
                    state.source.rooms[0].footprint.maximum ==
                        cr::CreativeTerrainCoord2{8, 4} &&
                    revisionBeforeCorner == revisionBeforeEdge + 1U,
                "east handle changes only the east side once") &&
         expect(cornerBegin.accepted && cornerCommit.accepted &&
                    cornerCommit.changed &&
                    state.source.rooms[0].footprint.minimum ==
                        cr::CreativeTerrainCoord2{-2, -1} &&
                    state.source.rooms[0].footprint.maximum ==
                        cr::CreativeTerrainCoord2{8, 4} &&
                    state.revision == revisionBeforeCorner + 1U,
                "north-west handle changes exactly two owned sides once") &&
         expect(cancelled.accepted && cancelled.changed &&
                    !state.roomManipulation.active &&
                    state.revision == revisionBeforeCancel &&
                    state.source.rooms[0].footprint.minimum ==
                        cr::CreativeTerrainCoord2{-2, -1} &&
                    state.source.rooms[0].footprint.maximum ==
                        cr::CreativeTerrainCoord2{8, 4},
                "cancel discards a room preview without a source edit");
}

bool invalidRoomManipulationsRejectWithoutMutation() {
  app::CreativeEditorWorldLayoutState overlapState;
  app::resetCreativeEditorWorldLayout(overlapState);
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      overlapState, app::CreativeEditorWorldLayoutTool::Room));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      overlapState, app::CreativeEditorWorldLayoutGesturePhase::Begin,
      {0, 0}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      overlapState, app::CreativeEditorWorldLayoutGesturePhase::Commit,
      {4, 4}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      overlapState, app::CreativeEditorWorldLayoutGesturePhase::Begin,
      {4, 0}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      overlapState, app::CreativeEditorWorldLayoutGesturePhase::Commit,
      {8, 4}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      overlapState, app::CreativeEditorWorldLayoutTool::Select));
  const std::uint64_t overlapRevision = overlapState.revision;
  static_cast<void>(app::applyCreativeEditorWorldLayoutRoomManipulation(
      overlapState,
      app::CreativeEditorWorldLayoutRoomManipulationPhase::Begin, {2, 2},
      0.3));
  const auto overlapPreview =
      app::applyCreativeEditorWorldLayoutRoomManipulation(
          overlapState,
          app::CreativeEditorWorldLayoutRoomManipulationPhase::Update, {4, 2},
          0.3);
  const bool overlapShownInvalid =
      overlapPreview.accepted && !overlapState.roomManipulation.previewValid;
  const auto overlapCommit =
      app::applyCreativeEditorWorldLayoutRoomManipulation(
          overlapState,
          app::CreativeEditorWorldLayoutRoomManipulationPhase::Commit, {4, 2},
          0.3);

  app::CreativeEditorWorldLayoutState openingState;
  app::resetCreativeEditorWorldLayout(openingState);
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      openingState, app::CreativeEditorWorldLayoutTool::Room));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      openingState, app::CreativeEditorWorldLayoutGesturePhase::Begin,
      {0, 0}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      openingState, app::CreativeEditorWorldLayoutGesturePhase::Commit,
      {6, 4}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      openingState, app::CreativeEditorWorldLayoutTool::Door));
  static_cast<void>(
      app::applyCreativeEditorWorldLayoutPoint(openingState, {5.5, 0.1}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      openingState, app::CreativeEditorWorldLayoutTool::Select));
  const std::uint64_t openingRevision = openingState.revision;
  static_cast<void>(app::applyCreativeEditorWorldLayoutRoomManipulation(
      openingState,
      app::CreativeEditorWorldLayoutRoomManipulationPhase::Begin, {6, 2},
      0.3));
  const auto openingPreview =
      app::applyCreativeEditorWorldLayoutRoomManipulation(
          openingState,
          app::CreativeEditorWorldLayoutRoomManipulationPhase::Update, {4, 2},
          0.3);

  return expect(overlapShownInvalid && !overlapCommit.accepted &&
                    overlapState.revision == overlapRevision &&
                    overlapState.source.rooms[0].footprint.minimum ==
                        cr::CreativeTerrainCoord2{0, 0} &&
                    overlapState.source.rooms[0].footprint.maximum ==
                        cr::CreativeTerrainCoord2{4, 4},
                "overlapping room drag rejects before source mutation") &&
         expect(openingPreview.accepted &&
                    !openingState.roomManipulation.previewValid &&
                    openingState.revision == openingRevision &&
                    openingState.source.rooms[0].footprint.maximum ==
                        cr::CreativeTerrainCoord2{6, 4},
                "room shrink rejects before clipping a hosted opening");
}

bool floorSettingsMoveAndResizeCommitOnce() {
  cr::CreativeAppState live = appState();
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state);
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Floor));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {0, 0}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {4, 3}));

  app::CreativeEditorWorldLayoutBoxSettings settings;
  const bool read = app::readCreativeEditorWorldLayoutBoxSettings(
      state, 0U, settings);
  settings.anchorLayer = 1.0;
  settings.layerCount = 2U;
  const std::uint64_t revisionBeforeSettings = state.revision;
  const auto configured = app::setCreativeEditorWorldLayoutBoxSettings(
      state, 0U, settings);
  const std::uint64_t revisionAfterSettings = state.revision;
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Select));

  const auto moveTarget = app::findCreativeEditorWorldLayoutBoxTarget(
      state, {2.0, 1.5}, 0.2);
  const std::uint64_t revisionBeforeMove = state.revision;
  const auto moveBegin = app::applyCreativeEditorWorldLayoutBoxManipulation(
      state, app::CreativeEditorWorldLayoutBoxManipulationPhase::Begin,
      {2.0, 1.5}, 0.2);
  const auto moveUpdate = app::applyCreativeEditorWorldLayoutBoxManipulation(
      state, app::CreativeEditorWorldLayoutBoxManipulationPhase::Update,
      {4.2, 2.7}, 0.2);
  const bool movePreviewOnly =
      state.source.boxes[0].footprint.minimum ==
          cr::CreativeTerrainCoord2{0, 0} &&
      state.boxManipulation.previewFootprint.minimum ==
          cr::CreativeTerrainCoord2{2, 1} &&
      state.boxManipulation.previewFootprint.maximum ==
          cr::CreativeTerrainCoord2{6, 4} &&
      state.revision == revisionBeforeMove;
  const auto moveCommit = app::applyCreativeEditorWorldLayoutBoxManipulation(
      state, app::CreativeEditorWorldLayoutBoxManipulationPhase::Commit,
      {4.2, 2.7}, 0.2);
  const std::uint64_t revisionAfterMove = state.revision;

  const auto cornerTarget = app::findCreativeEditorWorldLayoutBoxTarget(
      state, {6.0, 4.0}, 0.2);
  const std::uint64_t revisionBeforeResize = state.revision;
  static_cast<void>(app::applyCreativeEditorWorldLayoutBoxManipulation(
      state, app::CreativeEditorWorldLayoutBoxManipulationPhase::Begin,
      {6.0, 4.0}, 0.2));
  const auto resizeCommit = app::applyCreativeEditorWorldLayoutBoxManipulation(
      state, app::CreativeEditorWorldLayoutBoxManipulationPhase::Commit,
      {8.2, 5.1}, 0.2);
  const std::uint64_t revisionAfterResize = state.revision;

  const std::uint64_t revisionBeforeInvalid = state.revision;
  static_cast<void>(app::applyCreativeEditorWorldLayoutBoxManipulation(
      state, app::CreativeEditorWorldLayoutBoxManipulationPhase::Begin,
      {2.0, 3.0}, 0.2));
  const auto invalidPreview =
      app::applyCreativeEditorWorldLayoutBoxManipulation(
          state, app::CreativeEditorWorldLayoutBoxManipulationPhase::Update,
          {8.0, 3.0}, 0.2);
  const bool invalidShown =
      invalidPreview.accepted && !state.boxManipulation.previewValid;
  const auto invalidCommit =
      app::applyCreativeEditorWorldLayoutBoxManipulation(
          state, app::CreativeEditorWorldLayoutBoxManipulationPhase::Commit,
          {8.0, 3.0}, 0.2);
  const auto preview =
      app::previewCreativeEditorWorldLayout(state, live.facade.document());

  return expect(read && configured.accepted && configured.changed &&
                    revisionAfterSettings == revisionBeforeSettings + 1U &&
                    state.source.boxes[0].anchorLayer == 1.0 &&
                    state.source.boxes[0].layerCount == 2U,
                "floor settings commit as one source edit") &&
         expect(moveTarget.handle ==
                        app::CreativeEditorWorldLayoutBoxHandle::Move &&
                    moveBegin.accepted && moveUpdate.accepted &&
                    movePreviewOnly && moveCommit.accepted &&
                    moveCommit.changed &&
                    revisionAfterMove == revisionBeforeMove + 1U,
                "floor move previews before one source commit") &&
         expect(cornerTarget.handle ==
                        app::CreativeEditorWorldLayoutBoxHandle::SouthEast &&
                    resizeCommit.accepted && resizeCommit.changed &&
                    revisionAfterResize == revisionBeforeResize + 1U &&
                    state.source.boxes[0].footprint.minimum ==
                        cr::CreativeTerrainCoord2{2, 1} &&
                    state.source.boxes[0].footprint.maximum ==
                        cr::CreativeTerrainCoord2{8, 5},
                "floor corner resizes its owned sides") &&
         expect(invalidShown && !invalidCommit.accepted &&
                    state.revision == revisionBeforeInvalid &&
                    state.source.boxes[0].footprint.minimum ==
                        cr::CreativeTerrainCoord2{2, 1},
                "degenerate floor resize rejects without source mutation") &&
         expect(preview.accepted,
                "edited floor remains exact 3D-previewable");
}

bool planStoreySelectionEditsOnlyItsExactFloorSource() {
  app::CreativeEditorWorldLayoutState state;
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "stacked_house";
  building.name = "Stacked House";
  state.source.buildings.push_back(building);

  cr::CreativeWorldLayoutLevel ground;
  ground.buildingIndex = 0U;
  ground.stableKey = "ground";
  ground.name = "Ground";
  state.source.levels.push_back(ground);
  cr::CreativeWorldLayoutLevel upper = ground;
  upper.stableKey = "upper";
  upper.name = "Upper";
  upper.floorTopLayer = 3.0;
  state.source.levels.push_back(upper);

  cr::CreativeWorldLayoutBox groundFloor;
  groundFloor.buildingIndex = 0U;
  groundFloor.stableKey = "ground_floor";
  groundFloor.name = "Ground Floor";
  groundFloor.footprint = {{0, 0}, {4, 4}};
  groundFloor.anchorLayer = 0.0;
  state.source.boxes.push_back(groundFloor);
  cr::CreativeWorldLayoutBox upperFloor = groundFloor;
  upperFloor.stableKey = "upper_floor";
  upperFloor.name = "Upper Floor";
  upperFloor.anchorLayer = 3.0;
  state.source.boxes.push_back(upperFloor);

  state.activeLevelIndex = 0U;
  state.tool = app::CreativeEditorWorldLayoutTool::Select;
  const auto selected = app::selectCreativeEditorWorldLayoutSource(
      state, cr::CreativeWorldLayoutTable::Box, 1U, 1U);
  const auto mismatched = app::selectCreativeEditorWorldLayoutSource(
      state, cr::CreativeWorldLayoutTable::Box, 0U, 1U);
  const std::uint64_t revisionBefore = state.revision;
  const auto begun = app::applyCreativeEditorWorldLayoutBoxManipulation(
      state, app::CreativeEditorWorldLayoutBoxManipulationPhase::Begin,
      {2.0, 2.0}, 0.2);
  const auto committed = app::applyCreativeEditorWorldLayoutBoxManipulation(
      state, app::CreativeEditorWorldLayoutBoxManipulationPhase::Commit,
      {3.0, 2.0}, 0.2);

  return expect(selected.accepted && selected.changed &&
                    state.activeLevelIndex == 1U &&
                    !mismatched.accepted && !mismatched.changed &&
                    mismatched.reasonCode ==
                        "creative_editor_world_layout_source_level_mismatch",
                "plan source selection accepts only its resolved storey") &&
         expect(begun.accepted && committed.accepted && committed.changed &&
                    state.revision == revisionBefore + 1U &&
                    state.source.boxes[0].footprint.minimum ==
                        cr::CreativeTerrainCoord2{0, 0} &&
                    state.source.boxes[0].footprint.maximum ==
                        cr::CreativeTerrainCoord2{4, 4} &&
                    state.source.boxes[1].footprint.minimum ==
                        cr::CreativeTerrainCoord2{1, 0} &&
                    state.source.boxes[1].footprint.maximum ==
                        cr::CreativeTerrainCoord2{5, 4},
                "overlapping storeys mutate only the clicked semantic floor");
}

bool partitionManipulationPreservesHostedOpeningWorldPositions() {
  cr::CreativeAppState live = appState();
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state);
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Wall));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {0, 0}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {10, 0}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Door));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(state, {3, 0}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Window));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(state, {7, 0}));

  app::CreativeEditorWorldLayoutWallSettings settings;
  const bool read = app::readCreativeEditorWorldLayoutWallSettings(
      state, 0U, settings);
  settings.heightCells = 4U;
  settings.thicknessCells = 0.5;
  const std::uint64_t revisionBeforeSettings = state.revision;
  const auto configured = app::setCreativeEditorWorldLayoutWallSettings(
      state, 0U, settings);
  const std::uint64_t revisionAfterSettings = state.revision;
  app::CreativeEditorWorldLayoutWallSettings tooShort = settings;
  tooShort.heightCells = 1U;
  const auto rejectedHeight = app::setCreativeEditorWorldLayoutWallSettings(
      state, 0U, tooShort);
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Select));

  const auto moveTarget = app::findCreativeEditorWorldLayoutWallTarget(
      state, {5.0, 0.0}, 0.2);
  const std::uint64_t revisionBeforeMove = state.revision;
  static_cast<void>(app::applyCreativeEditorWorldLayoutWallManipulation(
      state, app::CreativeEditorWorldLayoutWallManipulationPhase::Begin,
      {5.0, 0.0}, 0.2));
  const auto moveUpdate = app::applyCreativeEditorWorldLayoutWallManipulation(
      state, app::CreativeEditorWorldLayoutWallManipulationPhase::Update,
      {6.2, 2.1}, 0.2);
  const bool movePreviewOnly =
      state.source.walls[0].start == cr::CreativeTerrainCoord2{0, 0} &&
      state.wallManipulation.previewStart ==
          cr::CreativeTerrainCoord2{1, 2} &&
      state.wallManipulation.previewEnd == cr::CreativeTerrainCoord2{11, 2} &&
      state.revision == revisionBeforeMove;
  const auto moveCommit = app::applyCreativeEditorWorldLayoutWallManipulation(
      state, app::CreativeEditorWorldLayoutWallManipulationPhase::Commit,
      {6.2, 2.1}, 0.2);
  const std::uint64_t revisionAfterMove = state.revision;
  const cr::CreativeTerrainCoord2 startAfterMove = state.source.walls[0].start;
  const cr::CreativeTerrainCoord2 endAfterMove = state.source.walls[0].end;

  const auto startTarget = app::findCreativeEditorWorldLayoutWallTarget(
      state, {1.0, 2.0}, 0.2);
  const std::uint64_t revisionBeforeTrim = state.revision;
  static_cast<void>(app::applyCreativeEditorWorldLayoutWallManipulation(
      state, app::CreativeEditorWorldLayoutWallManipulationPhase::Begin,
      {1.0, 2.0}, 0.2));
  const auto trimCommit = app::applyCreativeEditorWorldLayoutWallManipulation(
      state, app::CreativeEditorWorldLayoutWallManipulationPhase::Commit,
      {2.1, 2.0}, 0.2);
  const std::uint64_t revisionAfterTrim = state.revision;
  const double doorWorldX = state.source.walls[0].start.x +
                            state.source.openings[0].centerOffsetCells;
  const double windowWorldX = state.source.walls[0].start.x +
                              state.source.openings[1].centerOffsetCells;

  const std::uint64_t revisionBeforeClip = state.revision;
  static_cast<void>(app::applyCreativeEditorWorldLayoutWallManipulation(
      state, app::CreativeEditorWorldLayoutWallManipulationPhase::Begin,
      {2.0, 2.0}, 0.2));
  const auto clipPreview = app::applyCreativeEditorWorldLayoutWallManipulation(
      state, app::CreativeEditorWorldLayoutWallManipulationPhase::Update,
      {6.1, 2.0}, 0.2);
  const bool clipShown =
      clipPreview.accepted && !state.wallManipulation.previewValid;
  const auto clipCommit = app::applyCreativeEditorWorldLayoutWallManipulation(
      state, app::CreativeEditorWorldLayoutWallManipulationPhase::Commit,
      {6.1, 2.0}, 0.2);
  const auto preview =
      app::previewCreativeEditorWorldLayout(state, live.facade.document());

  return expect(read && configured.accepted && configured.changed &&
                    revisionAfterSettings == revisionBeforeSettings + 1U &&
                    !rejectedHeight.accepted &&
                    state.source.walls[0].heightCells == 4U &&
                    state.source.walls[0].thicknessCells == 0.5,
                "partition settings validate hosted opening height") &&
         expect(moveTarget.handle ==
                        app::CreativeEditorWorldLayoutWallHandle::Move &&
                    moveUpdate.accepted && movePreviewOnly &&
                    moveCommit.accepted && moveCommit.changed &&
                    revisionAfterMove == revisionBeforeMove + 1U &&
                    startAfterMove == cr::CreativeTerrainCoord2{1, 2} &&
                    endAfterMove == cr::CreativeTerrainCoord2{11, 2},
                "partition move previews before one source commit") &&
         expect(startTarget.handle ==
                        app::CreativeEditorWorldLayoutWallHandle::Start &&
                    trimCommit.accepted && trimCommit.changed &&
                    revisionAfterTrim == revisionBeforeTrim + 1U &&
                    doorWorldX == 4.0 && windowWorldX == 8.0 &&
                    state.source.openings[0].centerOffsetCells == 2.0 &&
                    state.source.openings[1].centerOffsetCells == 6.0,
                "partition start trim preserves opening world positions") &&
         expect(clipShown && !clipCommit.accepted &&
                    state.revision == revisionBeforeClip &&
                    state.source.walls[0].start ==
                        cr::CreativeTerrainCoord2{2, 2},
                "partition trim cannot clip a hosted opening") &&
         expect(preview.accepted,
                "edited partition and openings remain exact 3D-previewable");
}

bool buildingGroupMoveDuplicateAndDeleteAreAtomic() {
  cr::CreativeAppState live = appState();
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state);
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Room));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {0, 0}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {6, 4}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Floor));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {-1, -1}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {7, 5}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Window));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(state, {3, 0}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Wall));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {0, 5}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {6, 5}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Door));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(state, {3, 5}));
  const std::size_t ownerFromOpening =
      app::creativeEditorWorldLayoutSelectedBuilding(state);
  const auto selected =
      app::selectCreativeEditorWorldLayoutBuilding(state, ownerFromOpening);
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Select));

  app::CreativeEditorWorldLayoutBuildingBounds bounds;
  const bool readBounds = app::readCreativeEditorWorldLayoutBuildingBounds(
      state, 0U, bounds);
  const std::uint64_t revisionBeforeCancel = state.revision;
  static_cast<void>(app::applyCreativeEditorWorldLayoutBuildingManipulation(
      state,
      app::CreativeEditorWorldLayoutBuildingManipulationPhase::Begin,
      {2.0, 2.0}, 0.25));
  static_cast<void>(app::applyCreativeEditorWorldLayoutBuildingManipulation(
      state,
      app::CreativeEditorWorldLayoutBuildingManipulationPhase::Update,
      {3.0, 3.0}, 0.25));
  const auto cancelled =
      app::applyCreativeEditorWorldLayoutBuildingManipulation(
          state,
          app::CreativeEditorWorldLayoutBuildingManipulationPhase::Cancel);
  const bool cancelDiscardedPreview =
      cancelled.accepted && cancelled.changed &&
      !state.buildingManipulation.active &&
      state.revision == revisionBeforeCancel &&
      state.source.rooms[0].footprint.minimum ==
          cr::CreativeTerrainCoord2{0, 0};
  const std::uint64_t revisionBeforeMove = state.revision;
  const auto moveBegin =
      app::applyCreativeEditorWorldLayoutBuildingManipulation(
          state,
          app::CreativeEditorWorldLayoutBuildingManipulationPhase::Begin,
          {2.0, 2.0}, 0.25);
  const auto moveUpdate =
      app::applyCreativeEditorWorldLayoutBuildingManipulation(
          state,
          app::CreativeEditorWorldLayoutBuildingManipulationPhase::Update,
          {5.2, 4.1}, 0.25);
  const app::CreativeEditorWorldLayoutOpeningHost movedOpeningPreview =
      app::resolveCreativeEditorWorldLayoutOpeningHost(state, 1U);
  const bool movePreviewOnly =
      state.revision == revisionBeforeMove &&
      state.source.rooms[0].footprint.minimum ==
          cr::CreativeTerrainCoord2{0, 0} &&
      state.source.boxes[0].footprint.minimum ==
          cr::CreativeTerrainCoord2{-1, -1} &&
      state.source.walls[0].start == cr::CreativeTerrainCoord2{0, 5} &&
      state.buildingManipulation.previewDeltaXCells == 3 &&
      state.buildingManipulation.previewDeltaZCells == 2 &&
      movedOpeningPreview.start.x == 3.0 &&
      movedOpeningPreview.start.z == 7.0;
  const auto moveCommit =
      app::applyCreativeEditorWorldLayoutBuildingManipulation(
          state,
          app::CreativeEditorWorldLayoutBuildingManipulationPhase::Commit,
          {5.2, 4.1}, 0.25);
  const std::uint64_t revisionAfterMove = state.revision;
  const auto movedPreview =
      app::previewCreativeEditorWorldLayout(state, live.facade.document());
  const bool movedGeometry =
      state.source.rooms[0].footprint.minimum ==
          cr::CreativeTerrainCoord2{3, 2} &&
      state.source.boxes[0].footprint.minimum ==
          cr::CreativeTerrainCoord2{2, 1} &&
      state.source.walls[0].start == cr::CreativeTerrainCoord2{3, 7} &&
      near(state.source.openings[0].centerOffsetCells, 3.0) &&
      near(state.source.openings[1].centerOffsetCells, 3.0);

  std::int64_t duplicateDeltaX = 0;
  std::int64_t duplicateDeltaZ = 0;
  const bool hasDefaultOffset =
      app::defaultCreativeEditorWorldLayoutBuildingDuplicateOffset(
          state, 0U, duplicateDeltaX, duplicateDeltaZ);
  const std::uint64_t revisionBeforeDuplicate = state.revision;
  const std::uint64_t ordinalBeforeDuplicate = state.nextStableOrdinal;
  const auto rejectedDuplicate =
      app::duplicateCreativeEditorWorldLayoutBuilding(
          state, 0U, std::numeric_limits<std::int64_t>::max(), 0);
  const bool rejectedWithoutMutation =
      !rejectedDuplicate.accepted &&
      state.revision == revisionBeforeDuplicate &&
      state.nextStableOrdinal == ordinalBeforeDuplicate &&
      state.source.buildings.size() == 1U;
  const auto duplicated = app::duplicateCreativeEditorWorldLayoutBuilding(
      state, 0U, duplicateDeltaX, duplicateDeltaZ);
  const std::uint64_t revisionAfterDuplicate = state.revision;
  const auto duplicatedPreview =
      app::previewCreativeEditorWorldLayout(state, live.facade.document());
  const bool duplicateRemapped =
      state.source.buildings.size() == 2U &&
      state.source.rooms.size() == 2U && state.source.boxes.size() == 2U &&
      state.source.walls.size() == 2U &&
      state.source.openings.size() == 4U &&
      state.source.rooms[1].buildingIndex == 1U &&
      state.source.boxes[1].buildingIndex == 1U &&
      state.source.walls[1].buildingIndex == 1U &&
      state.source.openings[2].roomIndex == 1U &&
      state.source.openings[3].wallIndex == 1U &&
      state.source.walls[1].start ==
          cr::CreativeTerrainCoord2{13, 7} &&
      state.source.walls[1].end == cr::CreativeTerrainCoord2{19, 7} &&
      near(state.source.openings[2].centerOffsetCells, 3.0) &&
      near(state.source.openings[3].centerOffsetCells, 3.0) &&
      stableKeysUnique(state.source);

  const auto originalSelectedForDelete =
      app::selectCreativeEditorWorldLayoutBuilding(state, 0U);
  const std::uint64_t revisionBeforeDelete = state.revision;
  const auto removed = app::deleteCreativeEditorWorldLayoutSelection(state);

  return expect(ownerFromOpening == 0U && selected.accepted &&
                    selected.changed && readBounds && bounds.valid &&
                    bounds.minimum == cr::CreativeTerrainCoord2{-1, -1} &&
                    bounds.maximum == cr::CreativeTerrainCoord2{7, 5},
                "a child selection resolves to one explicit building owner") &&
         expect(moveBegin.accepted && moveUpdate.accepted &&
                    cancelDiscardedPreview && movePreviewOnly &&
                    moveCommit.accepted &&
                    moveCommit.changed &&
                    revisionAfterMove == revisionBeforeMove + 1U &&
                    movedGeometry && movedPreview.accepted,
                "building movement previews all children and commits once") &&
         expect(hasDefaultOffset && duplicateDeltaX == 10 &&
                    duplicateDeltaZ == 0 && rejectedWithoutMutation &&
                    duplicated.accepted && duplicated.changed &&
                    revisionAfterDuplicate == revisionBeforeDuplicate + 1U &&
                    duplicateRemapped && duplicatedPreview.accepted,
                "building duplication remaps ownership, hosts, and stable keys") &&
         expect(originalSelectedForDelete.accepted &&
                    originalSelectedForDelete.changed && removed.accepted &&
                    removed.changed &&
                    state.revision == revisionBeforeDelete + 1U &&
                    state.source.buildings.size() == 1U &&
                    state.source.rooms.size() == 1U &&
                    state.source.boxes.size() == 1U &&
                    state.source.walls.size() == 1U &&
                    state.source.openings.size() == 2U &&
                    state.source.rooms[0].buildingIndex == 0U &&
                    state.source.walls[0].buildingIndex == 0U &&
                    state.source.openings[0].roomIndex == 0U &&
                    state.source.openings[1].wallIndex == 0U &&
                    state.source.walls[0].start ==
                        cr::CreativeTerrainCoord2{13, 7} &&
                    state.selection.kind ==
                        app::CreativeEditorWorldLayoutSelectionKind::None,
                "building deletion removes one owner and remaps survivors");
}

bool buildingTransformPreviewsAndCommitsOneRevision() {
  cr::CreativeAppState live = appState();
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state);
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Room));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {0, 0}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {6, 4}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Floor));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {-1, -1}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {7, 5}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Door));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(state, {2, 0}));
  static_cast<void>(app::selectCreativeEditorWorldLayoutBuilding(state, 0U));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Select));

  const std::uint64_t revisionBefore = state.revision;
  const auto preview = app::applyCreativeEditorWorldLayoutBuildingTransform(
      state, app::CreativeEditorWorldLayoutBuildingTransformPhase::Preview,
      cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90);
  const cr::CreativeWorldLayout &displayed =
      app::creativeEditorWorldLayoutDisplaySource(state);
  const bool candidateOnly =
      preview.accepted && preview.changed && state.buildingTransform.active &&
      state.revision == revisionBefore &&
      state.source.rooms[0].footprint.minimum ==
          cr::CreativeTerrainCoord2{0, 0} &&
      state.source.rooms[0].footprint.maximum ==
          cr::CreativeTerrainCoord2{6, 4} &&
      displayed.rooms[0].footprint.minimum == cr::CreativeTerrainCoord2{0, 0} &&
      displayed.rooms[0].footprint.maximum == cr::CreativeTerrainCoord2{4, 6} &&
      displayed.openings[0].roomEdge == cr::CreativeWorldLayoutRoomEdge::East;

  const auto cancelled = app::applyCreativeEditorWorldLayoutBuildingTransform(
      state, app::CreativeEditorWorldLayoutBuildingTransformPhase::Cancel);
  const bool cancelRestoredSource =
      cancelled.accepted && cancelled.changed &&
      !state.buildingTransform.active && state.revision == revisionBefore &&
      &app::creativeEditorWorldLayoutDisplaySource(state) == &state.source;

  static_cast<void>(app::applyCreativeEditorWorldLayoutBuildingTransform(
      state, app::CreativeEditorWorldLayoutBuildingTransformPhase::Preview,
      cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90));
  const auto committed = app::applyCreativeEditorWorldLayoutBuildingTransform(
      state, app::CreativeEditorWorldLayoutBuildingTransformPhase::Commit);
  const auto exactPreview =
      app::previewCreativeEditorWorldLayout(state, live.facade.document());
  const bool committedOnce =
      committed.accepted && committed.changed &&
      state.revision == revisionBefore + 1U &&
      !state.buildingTransform.active &&
      state.source.rooms[0].footprint.minimum ==
          cr::CreativeTerrainCoord2{0, 0} &&
      state.source.rooms[0].footprint.maximum ==
          cr::CreativeTerrainCoord2{4, 6} &&
      state.source.openings[0].roomEdge ==
          cr::CreativeWorldLayoutRoomEdge::East &&
      state.selection.kind ==
          app::CreativeEditorWorldLayoutSelectionKind::Building;

  const std::uint64_t revisionBeforeInvalid = state.revision;
  const auto invalid = app::applyCreativeEditorWorldLayoutBuildingTransform(
      state, app::CreativeEditorWorldLayoutBuildingTransformPhase::Preview,
      cr::CreativeWorldLayoutBuildingTransformOperation::Count);

  return expect(
             candidateOnly,
             "building transform candidate renders without source mutation") &&
         expect(cancelRestoredSource,
                "building transform cancel discards the candidate") &&
         expect(committedOnce && exactPreview.accepted,
                "building transform commits once and remains exact "
                "3D-previewable") &&
         expect(!invalid.accepted && !state.buildingTransform.active &&
                    state.revision == revisionBeforeInvalid,
                "invalid building transform cannot consume a revision");
}

bool buildingTemplatesPersistPreviewAndStampOneRevision() {
  cr::CreativeAppState live = appState();
  const std::filesystem::path saveRoot =
      std::filesystem::temp_directory_path() /
      "iggy3d_world_layout_building_template_tests";
  std::error_code error;
  std::filesystem::remove_all(saveRoot, error);

  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "template_source");
  const auto loaded =
      app::loadCreativeEditorWorldLayoutBuildingTemplateLibrary(
          state.buildingTemplates, saveRoot);
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Room));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {2, 3}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {8, 7}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Door));
  static_cast<void>(
      app::applyCreativeEditorWorldLayoutPoint(state, {5.0, 3.0}));
  static_cast<void>(app::selectCreativeEditorWorldLayoutBuilding(state, 0U));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Select));

  const std::uint64_t revisionBeforeCapture = state.revision;
  const auto captured = app::captureCreativeEditorWorldLayoutBuildingTemplate(
      state, 0U, "Reusable House");
  const std::filesystem::path templatePath =
      state.buildingTemplates.root / "building_template_0001.iwlt";
  const bool capturedWithoutSourceMutation =
      loaded.accepted && captured.accepted && captured.changed &&
      state.revision == revisionBeforeCapture &&
      state.source.buildings.size() == 1U &&
      state.buildingTemplates.templates.size() == 1U &&
      std::filesystem::is_regular_file(templatePath) &&
      state.buildingTemplates.templates[0].bounds.minimum ==
          cr::CreativeTerrainCoord2{0, 0};

  const auto blockedBegin =
      app::applyCreativeEditorWorldLayoutBuildingTemplatePlacement(
          state,
          app::CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Begin,
          {2.0, 3.0},
          cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90,
          &live.facade.document());
  const cr::CreativeWorldLayout& blockedCandidate =
      app::creativeEditorWorldLayoutDisplaySource(state);
  const auto blockedCommit =
      app::applyCreativeEditorWorldLayoutBuildingTemplatePlacement(
          state,
          app::CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Commit,
          {}, cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90,
          &live.facade.document());
  const bool blockedPreviewVisible =
      blockedBegin.accepted && state.buildingTemplatePlacement.active &&
      state.buildingTemplatePlacement.previewPositioned &&
      !state.buildingTemplatePlacement.previewValid &&
      blockedCandidate.buildings.size() == 2U && !blockedCommit.accepted &&
      state.revision == revisionBeforeCapture &&
      state.buildingTemplatePlacement.analysis.status ==
          cr::CreativeWorldLayoutBuildingTemplatePlacementStatus::
              BuildingOverlap;
  static_cast<void>(app::applyCreativeEditorWorldLayoutBuildingTemplatePlacement(
      state,
      app::CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Cancel));

  const auto begin =
      app::applyCreativeEditorWorldLayoutBuildingTemplatePlacement(
          state,
          app::CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Begin,
          {12.2, -2.6});
  const auto update =
      app::applyCreativeEditorWorldLayoutBuildingTemplatePlacement(
          state,
          app::CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Update,
          {18.4, 5.6});
  const auto transformed =
      app::applyCreativeEditorWorldLayoutBuildingTemplatePlacement(
          state,
          app::CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::
              Transform,
          {}, cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90);
  const cr::CreativeWorldLayout& candidate =
      app::creativeEditorWorldLayoutDisplaySource(state);
  const bool previewOnly =
      begin.accepted && update.accepted && transformed.accepted &&
      state.buildingTemplatePlacement.active &&
      state.buildingTemplatePlacement.anchor ==
          cr::CreativeTerrainCoord2{18, 6} &&
      state.revision == revisionBeforeCapture &&
      state.source.buildings.size() == 1U && candidate.buildings.size() == 2U &&
      candidate.rooms[1].footprint.minimum ==
          cr::CreativeTerrainCoord2{18, 6} &&
      candidate.rooms[1].footprint.maximum ==
          cr::CreativeTerrainCoord2{22, 12} &&
      candidate.openings[1].roomEdge ==
          cr::CreativeWorldLayoutRoomEdge::East;
  const auto cancelled =
      app::applyCreativeEditorWorldLayoutBuildingTemplatePlacement(
          state,
          app::CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Cancel);
  const bool cancelRestoredSource =
      cancelled.accepted && cancelled.changed &&
      !state.buildingTemplatePlacement.active &&
      state.revision == revisionBeforeCapture &&
      &app::creativeEditorWorldLayoutDisplaySource(state) == &state.source;

  static_cast<void>(
      app::applyCreativeEditorWorldLayoutBuildingTemplatePlacement(
          state,
          app::CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Begin,
          {20.0, 10.0}));
  const auto committed =
      app::applyCreativeEditorWorldLayoutBuildingTemplatePlacement(
          state,
          app::CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Commit);
  const auto exactPreview =
      app::previewCreativeEditorWorldLayout(state, live.facade.document());
  const bool stampedOnce =
      committed.accepted && committed.changed &&
      state.revision == revisionBeforeCapture + 1U &&
      state.source.buildings.size() == 2U && state.source.rooms.size() == 2U &&
      state.source.openings.size() == 2U &&
      state.source.rooms[1].buildingIndex == 1U &&
      state.source.openings[1].roomIndex == 1U &&
      state.source.rooms[1].footprint.minimum ==
          cr::CreativeTerrainCoord2{20, 10} &&
      state.selection.kind ==
          app::CreativeEditorWorldLayoutSelectionKind::Building &&
      state.selection.index == 1U && stableKeysUnique(state.source) &&
      exactPreview.accepted;

  app::resetCreativeEditorWorldLayout(state, "after_reset");
  const bool resetPreservedLibrary =
      state.source.buildings.empty() &&
      state.buildingTemplates.templates.size() == 1U &&
      state.buildingTemplates.selectedIndex == 0U;
  app::CreativeEditorWorldLayoutBuildingTemplateLibrary reloadedLibrary;
  const auto reloaded =
      app::loadCreativeEditorWorldLayoutBuildingTemplateLibrary(reloadedLibrary,
                                                                 saveRoot);
  const bool durableReload =
      reloaded.accepted && reloaded.loadedCount == 1U &&
      reloaded.rejectedCount == 0U && reloadedLibrary.templates.size() == 1U &&
      reloadedLibrary.templates[0].label == "Reusable House";

  std::filesystem::remove_all(saveRoot, error);
  return expect(capturedWithoutSourceMutation,
                "building template capture is durable and revision-neutral") &&
         expect(blockedPreviewVisible,
                "overlapping template stays positioned for red preview but cannot commit") &&
         expect(previewOnly,
                "building template movement and rotation remain candidate-only") &&
         expect(cancelRestoredSource,
                "building template cancel restores the exact source") &&
         expect(stampedOnce,
                "building template stamp remaps ownership in one revision") &&
         expect(resetPreservedLibrary && durableReload,
                "building template library survives reset and disk reload");
}

bool buildingTemplateFoundationPreviewUsesStagedLayoutTerrain() {
  cr::CreativeAppState live = appState();
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "template_staged_terrain");
  const auto shell = app::createCreativeEditorWorldLayoutBuildingShell(
      state, {{{0, 0}, {4, 4}}, 0.0, 3U, 0.25, 1U});
  const cr::CreativeWorldLayoutBuildingTemplateResult captured =
      cr::captureCreativeWorldLayoutBuildingTemplate(
          state.source, {0U, "foundation_template", "Foundation Template"});
  cr::CreativeWorldLayoutBuildingTemplateResult foundation;
  if (captured.accepted) {
    cr::CreativeWorldLayout source = captured.value.normalizedLayout;
    source.buildings[0].groundingMode =
        cr::CreativeWorldLayoutGroundingMode::Foundation;
    source.buildings[0].maximumGroundReliefCells = 0U;
    foundation = cr::loadCreativeWorldLayoutBuildingTemplate(std::move(source));
  }
  if (!shell.accepted || !foundation.accepted) {
    return expect(false, "foundation template staged-terrain fixture accepted");
  }

  state.buildingTemplates.templates.push_back(foundation.value);
  state.buildingTemplates.selectedIndex = 0U;
  cr::CreativeWorldLayoutTerrainProfile plateau;
  plateau.stableKey = "template_staged_plateau";
  plateau.kind = cr::CreativeTerrainRecipeKind::Plateau;
  plateau.center = {12, 12};
  plateau.baseHeightCells = 3U;
  plateau.radiusCells = 8U;
  plateau.spacingCells = 1U;
  state.source.terrainProfiles.push_back(plateau);
  ++state.revision;
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Select));

  const auto began =
      app::applyCreativeEditorWorldLayoutBuildingTemplatePlacement(
          state,
          app::CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Begin,
          {10.0, 10.0},
          cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90,
          &live.facade.document());
  const auto analysis = state.buildingTemplatePlacement.analysis;
  const auto committed =
      app::applyCreativeEditorWorldLayoutBuildingTemplatePlacement(
          state,
          app::CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Commit,
          {}, cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90,
          &live.facade.document());
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(live.facade.document(), state.source);

  return expect(began.accepted &&
                    state.source.terrainProfiles.size() == 1U &&
                    analysis.accepted &&
                    analysis.terrainImpact ==
                        cr::CreativeWorldLayoutBuildingTemplateTerrainImpact::
                            Grounded &&
                    analysis.grounding.minimumHeightCells == 3U &&
                    analysis.grounding.maximumHeightCells == 3U,
                "template preview uses the staged World Layout plateau") &&
         expect(committed.accepted && committed.changed &&
                    compiled.receipt.accepted &&
                    compiled.receipt.groundedBuildingCount == 1U,
                "template preview terrain outcome matches final compilation");
}

bool builtInBuildingTemplateInstallIsDurableAndIdempotent() {
  const std::filesystem::path saveRoot =
      std::filesystem::temp_directory_path() /
      "iggy3d_builtin_building_template_install_tests";
  std::error_code error;
  std::filesystem::remove_all(saveRoot, error);

  app::CreativeEditorWorldLayoutBuildingTemplateLibrary library;
  const auto loaded =
      app::loadCreativeEditorWorldLayoutBuildingTemplateLibrary(library,
                                                                 saveRoot);
  const cr::CreativeWorldLayoutBuildingTemplateResult source =
      cr::buildCreativeBuiltInBuildingTemplate(
          cr::kBuilderEstateHouseTemplateId);
  if (!loaded.accepted || !source.accepted) {
    std::filesystem::remove_all(saveRoot, error);
    return expect(false, "built-in template install setup accepted");
  }
  const auto installed =
      app::installCreativeEditorWorldLayoutBuildingTemplate(library,
                                                             source.value);
  const auto repeated =
      app::installCreativeEditorWorldLayoutBuildingTemplate(library,
                                                             source.value);

  cr::CreativeWorldLayout changedLayout = source.value.normalizedLayout;
  changedLayout.levels[0].wallHeightCells += 1U;
  const cr::CreativeWorldLayoutBuildingTemplateResult changedSource =
      cr::loadCreativeWorldLayoutBuildingTemplate(std::move(changedLayout));
  const auto conflict = changedSource.accepted
                            ? app::installCreativeEditorWorldLayoutBuildingTemplate(
                                  library, changedSource.value)
                            : app::CreativeEditorWorldLayoutBuildingTemplateInstallReceipt{};
  const auto installedStaleBuiltIn =
      changedSource.accepted
          ? app::installCreativeEditorBuiltInWorldLayoutBuildingTemplate(
                library, changedSource.value)
          : app::CreativeEditorWorldLayoutBuildingTemplateInstallReceipt{};
  const auto upgradedBuiltIn =
      app::installCreativeEditorBuiltInWorldLayoutBuildingTemplate(library,
                                                                   source.value);
  const auto repeatedUpgrade =
      app::installCreativeEditorBuiltInWorldLayoutBuildingTemplate(library,
                                                                   source.value);

  app::CreativeEditorWorldLayoutBuildingTemplateLibrary reloadedLibrary;
  const auto reloaded =
      app::loadCreativeEditorWorldLayoutBuildingTemplateLibrary(
          reloadedLibrary, saveRoot);
  const bool filePresent = std::filesystem::is_regular_file(
      saveRoot / "world_layout_templates" / "builder_estate.house.iwlt",
      error);

  const bool ok =
      expect(loaded.accepted && source.accepted,
             "built-in template install setup accepted") &&
      expect(installed.accepted && installed.changed &&
                 installed.templateIndex == 0U &&
                 library.templates.size() == 1U,
             "built-in template installs once") &&
      expect(repeated.accepted && !repeated.changed &&
                 repeated.templateIndex == 0U && library.templates.size() == 1U,
             "matching built-in template install is idempotent") &&
      expect(changedSource.accepted && !conflict.accepted &&
                 conflict.reasonCode == "creative_editor_world_layout_building_"
                                        "template_install_conflict",
             "conflicting built-in template id fails closed") &&
      expect(installedStaleBuiltIn.accepted && installedStaleBuiltIn.changed &&
                 upgradedBuiltIn.accepted && upgradedBuiltIn.changed &&
                 repeatedUpgrade.accepted && !repeatedUpgrade.changed &&
                 library.templates.size() == 1U &&
                 library.templates[0].sourceFingerprint ==
                     source.value.sourceFingerprint,
             "authoritative built-in install upgrades a stale cached definition") &&
      expect(filePresent && reloaded.accepted && reloaded.loadedCount == 1U &&
                 reloadedLibrary.templates.size() == 1U &&
                 reloadedLibrary.templates[0].sourceFingerprint ==
                     source.value.sourceFingerprint,
             "built-in template survives disk reload");
  std::filesystem::remove_all(saveRoot, error);
  return ok;
}

bool buildingTemplateUpdateAndRefreshLifecycleIsExplicit() {
  const std::filesystem::path saveRoot =
      std::filesystem::temp_directory_path() /
      "iggy3d_world_layout_building_template_sync_tests";
  std::error_code error;
  std::filesystem::remove_all(saveRoot, error);

  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "template_sync_source");
  const auto loaded =
      app::loadCreativeEditorWorldLayoutBuildingTemplateLibrary(
          state.buildingTemplates, saveRoot);
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Room));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {0, 0}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {6, 4}));
  static_cast<void>(app::selectCreativeEditorWorldLayoutBuilding(state, 0U));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Select));
  const auto captured = app::captureCreativeEditorWorldLayoutBuildingTemplate(
      state, 0U, "Linked House");

  const auto stampAt = [&](app::CreativeEditorWorldLayoutPoint point) {
    const auto began =
        app::applyCreativeEditorWorldLayoutBuildingTemplatePlacement(
            state,
            app::CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::
                Begin,
            point);
    const auto committed =
        app::applyCreativeEditorWorldLayoutBuildingTemplatePlacement(
            state,
            app::CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::
                Commit);
    return began.accepted && committed.accepted && committed.changed;
  };
  const bool twoInstancesStamped = stampAt({10, 0}) && stampAt({20, 0});
  if (!loaded.accepted || !captured.accepted || !twoInstancesStamped) {
    std::filesystem::remove_all(saveRoot, error);
    return expect(false, "linked editor instance setup accepted");
  }

  const auto roomIndexForBuilding = [&](std::size_t buildingIndex) {
    for (std::size_t index = 0U; index < state.source.rooms.size(); ++index) {
      if (state.source.rooms[index].buildingIndex == buildingIndex) {
        return index;
      }
    }
    return cr::kInvalidCreativeWorldLayoutIndex;
  };

  const std::size_t firstRoomIndex = roomIndexForBuilding(1U);
  const cr::CreativeWorldLayoutRoom& firstRoom =
      state.source.rooms[firstRoomIndex];
  const cr::CreativeWorldLayoutLevel& firstLevel =
      state.source.levels[firstRoom.levelIndex];
  app::CreativeEditorWorldLayoutRoomSettings firstSettings{
      firstRoom.footprint, firstLevel.floorTopLayer,
      firstLevel.wallHeightCells, firstRoom.wallThicknessCells,
      firstLevel.floorThicknessLayers};
  firstSettings.wallHeightCells += 2U;
  static_cast<void>(app::setCreativeEditorWorldLayoutRoomSettings(
      state, firstRoomIndex, firstSettings));
  const auto localBeforeUpdate =
      app::inspectCreativeEditorWorldLayoutBuildingTemplateSync(state, 1U);
  const std::uint64_t revisionBeforeUpdate = state.revision;
  const auto updated =
      app::updateCreativeEditorWorldLayoutBuildingTemplateFromInstance(state,
                                                                      1U);
  const auto firstAfterUpdate =
      app::inspectCreativeEditorWorldLayoutBuildingTemplateSync(state, 1U);
  const auto secondAfterUpdate =
      app::inspectCreativeEditorWorldLayoutBuildingTemplateSync(state, 2U);

  const std::uint64_t revisionBeforeSafe = state.revision;
  const auto safe =
      app::refreshCreativeEditorWorldLayoutBuildingTemplateInstances(
          state, 1U,
          cr::CreativeWorldLayoutBuildingTemplateRefreshMode::SafeInstances);
  const std::uint64_t revisionAfterSafe = state.revision;
  const auto firstAfterSafe =
      app::inspectCreativeEditorWorldLayoutBuildingTemplateSync(state, 1U);
  const auto secondAfterSafe =
      app::inspectCreativeEditorWorldLayoutBuildingTemplateSync(state, 2U);

  const std::size_t secondRoomIndex = roomIndexForBuilding(2U);
  const cr::CreativeWorldLayoutRoom& secondRoom =
      state.source.rooms[secondRoomIndex];
  const cr::CreativeWorldLayoutLevel& secondLevel =
      state.source.levels[secondRoom.levelIndex];
  app::CreativeEditorWorldLayoutRoomSettings secondSettings{
      secondRoom.footprint, secondLevel.floorTopLayer,
      secondLevel.wallHeightCells, secondRoom.wallThicknessCells,
      secondLevel.floorThicknessLayers};
  secondSettings.wallHeightCells += 3U;
  static_cast<void>(app::setCreativeEditorWorldLayoutRoomSettings(
      state, secondRoomIndex, secondSettings));
  const auto secondLocal =
      app::inspectCreativeEditorWorldLayoutBuildingTemplateSync(state, 2U);
  const std::uint64_t revisionBeforeSkippedSafe = state.revision;
  const auto skippedSafe =
      app::refreshCreativeEditorWorldLayoutBuildingTemplateInstances(
          state, 1U,
          cr::CreativeWorldLayoutBuildingTemplateRefreshMode::SafeInstances);
  const std::uint64_t revisionBeforeForce = state.revision;
  const auto forced =
      app::refreshCreativeEditorWorldLayoutBuildingTemplateInstances(
          state, 1U,
          cr::CreativeWorldLayoutBuildingTemplateRefreshMode::ForceAll);
  const auto secondAfterForce =
      app::inspectCreativeEditorWorldLayoutBuildingTemplateSync(state, 2U);
  const std::uint64_t revisionAfterForce = state.revision;
  const cr::CreativeWorldLayoutBuildingTemplateFingerprint beforeDetach =
      cr::fingerprintCreativeWorldLayoutBuilding(state.source, 2U);
  const std::string detachedBuildingKey = state.source.buildings[2U].stableKey;
  const auto detached =
      app::detachCreativeEditorWorldLayoutBuildingTemplateInstance(state, 2U);
  const auto secondAfterDetach =
      app::inspectCreativeEditorWorldLayoutBuildingTemplateSync(state, 2U);
  const cr::CreativeWorldLayoutBuildingTemplateFingerprint afterDetach =
      cr::fingerprintCreativeWorldLayoutBuilding(state.source, 2U);

  app::CreativeEditorWorldLayoutBuildingTemplateLibrary reloadedLibrary;
  const auto reloaded =
      app::loadCreativeEditorWorldLayoutBuildingTemplateLibrary(reloadedLibrary,
                                                                 saveRoot);
  const bool durableUpdate =
      reloaded.accepted && reloadedLibrary.templates.size() == 1U &&
      reloadedLibrary.templates[0].sourceFingerprint ==
          state.buildingTemplates.templates[0].sourceFingerprint;

  std::filesystem::remove_all(saveRoot, error);
  return expect(localBeforeUpdate.state ==
                        cr::CreativeWorldLayoutBuildingTemplateSyncState::
                            LocallyModified &&
                    updated.accepted && updated.changed &&
                    revisionBeforeSafe == revisionBeforeUpdate + 1U &&
                    firstAfterUpdate.state ==
                        cr::CreativeWorldLayoutBuildingTemplateSyncState::Current &&
                    secondAfterUpdate.state ==
                        cr::CreativeWorldLayoutBuildingTemplateSyncState::
                            SourceChanged,
                "updating from one instance advances its source once") &&
         expect(safe.accepted && safe.changed &&
                    revisionAfterSafe == revisionBeforeSafe + 1U &&
                    firstAfterSafe.state ==
                        cr::CreativeWorldLayoutBuildingTemplateSyncState::Current &&
                    secondAfterSafe.state ==
                        cr::CreativeWorldLayoutBuildingTemplateSyncState::Current,
                "safe refresh updates untouched sibling instances") &&
         expect(secondLocal.state ==
                        cr::CreativeWorldLayoutBuildingTemplateSyncState::
                            LocallyModified &&
                    !skippedSafe.accepted && !skippedSafe.changed &&
                    revisionBeforeSkippedSafe == revisionBeforeForce &&
                    forced.accepted && forced.changed &&
                    revisionAfterForce == revisionBeforeForce + 1U &&
                    secondAfterForce.state ==
                        cr::CreativeWorldLayoutBuildingTemplateSyncState::Current,
                "safe refresh skips local work and force refresh is explicit") &&
         expect(detached.accepted && detached.changed &&
                    state.revision == revisionAfterForce + 1U &&
                    state.source.buildings[2U].stableKey == detachedBuildingKey &&
                    beforeDetach == afterDetach &&
                    secondAfterDetach.state ==
                        cr::CreativeWorldLayoutBuildingTemplateSyncState::Unlinked,
                "editor detach preserves refinements and removes only the template link") &&
         expect(durableUpdate,
                "updated template fingerprint is durable on disk");
}

bool openingSettingsApplyOnceAndMatchExactPreview() {
  cr::CreativeAppState live = appState();
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state);
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Room));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {0, 0}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {8, 4}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Door));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(state, {2, 0}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Window));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(state, {6, 0}));

  app::CreativeEditorWorldLayoutOpeningSettings doorSettings;
  app::CreativeEditorWorldLayoutOpeningSettings windowSettings;
  const bool readDoor = app::readCreativeEditorWorldLayoutOpeningSettings(
      state, 0U, doorSettings);
  const bool readWindow = app::readCreativeEditorWorldLayoutOpeningSettings(
      state, 1U, windowSettings);
  doorSettings.widthCells = 1.5;
  doorSettings.heightCells = 2.5;
  doorSettings.door.leafArrangement =
      cr::CreativeDoorLeafArrangement::Double;
  doorSettings.door.hingeSide = cr::CreativeDoorHingeSide::MinimumEdge;
  doorSettings.door.swingSide = cr::CreativeDoorSwingSide::NegativeNormal;
  doorSettings.door.initialState = cr::CreativeDoorInitialState::Open;
  doorSettings.door.gameplayLocked = true;
  doorSettings.door.transitionSeconds = 0.75;
  doorSettings.facing = cr::CreativeBuildingOpeningFacing::NegativeNormal;
  windowSettings.widthCells = 1.25;
  windowSettings.sillHeightCells = 1.0;
  windowSettings.heightCells = 1.25;
  windowSettings.includeInsert = false;
  windowSettings.window.insertKind =
      cr::CreativeWindowInsertKind::PairedShutters;
  windowSettings.facing = cr::CreativeBuildingOpeningFacing::NegativeNormal;
  const std::uint64_t revisionBefore = state.revision;
  const auto doorUpdated = app::setCreativeEditorWorldLayoutOpeningSettings(
      state, 0U, doorSettings);
  const auto windowUpdated = app::setCreativeEditorWorldLayoutOpeningSettings(
      state, 1U, windowSettings);

  app::CreativeEditorWorldLayoutOpeningSettings badDoor = doorSettings;
  badDoor.sillHeightCells = 0.25;
  const std::uint64_t revisionBeforeInvalid = state.revision;
  const auto badDoorResult = app::setCreativeEditorWorldLayoutOpeningSettings(
      state, 0U, badDoor);
  app::CreativeEditorWorldLayoutOpeningSettings badWindow = windowSettings;
  badWindow.heightCells = 0.0;
  const auto badWindowResult =
      app::setCreativeEditorWorldLayoutOpeningSettings(state, 1U, badWindow);
  const auto preview =
      app::previewCreativeEditorWorldLayout(state, live.facade.document());
  const cr::CreativeDocument& rendered =
      app::creativeEditorWorldLayoutRenderDocument(state,
                                                   live.facade.document());
  std::uint64_t doorCount = 0U;
  std::uint64_t windowCount = 0U;
  for (const cr::CreativeObject& object : rendered.objects()) {
    doorCount += object.kind == cr::CreativeObjectKind::Door ? 1U : 0U;
    windowCount += object.kind == cr::CreativeObjectKind::Window ? 1U : 0U;
  }

  return expect(readDoor && readWindow && doorUpdated.accepted &&
                    doorUpdated.changed && windowUpdated.accepted &&
                    windowUpdated.changed &&
                    state.revision == revisionBefore + 2U,
                "each opening inspector apply records one source revision") &&
         expect(state.source.openings[0].widthCells == 1.5 &&
                    state.source.openings[0].cutoutHeightCells == 2.5 &&
                    state.source.openings[0].insertWidthCells == 1.5 &&
                    state.source.openings[0].insertHeightCells == 2.5 &&
                    state.source.openings[0].door == doorSettings.door &&
                    state.source.openings[0].facing ==
                        cr::CreativeBuildingOpeningFacing::NegativeNormal,
                "door cutout and tracked insert dimensions stay aligned") &&
         expect(state.source.openings[1].widthCells == 1.25 &&
                    state.source.openings[1].cutoutBottomCells == 1.0 &&
                    state.source.openings[1].cutoutHeightCells == 1.25 &&
                    state.source.openings[1].window == windowSettings.window &&
                    !state.source.openings[1].includeInsert &&
                    state.source.openings[1].facing ==
                        cr::CreativeBuildingOpeningFacing::NegativeNormal,
                "window sill, height, width, and insert presence persist") &&
         expect(!badDoorResult.accepted && !badWindowResult.accepted &&
                    state.revision == revisionBeforeInvalid,
                "invalid door sill and window dimensions fail without mutation") &&
         expect(preview.accepted && doorCount == 1U && windowCount == 0U,
                "exact preview honors open door and omitted window insert");
}

bool concaveRoomOpeningPlacementKeepsTheExactTopologyHost() {
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "concave_opening_editor");
  state.source.buildings.push_back(
      {"building", "L Building", cr::CreativeBuildingRootMode::CreateRoom,
       {{0, 0}, {6, 5}}, 0, 4U, true, {}});
  state.source.levels.push_back(
      {0U, "level", "Ground", 0.0, 4U, 1U, 1U, 1U});
  state.source.rooms.push_back(
      {0U, 0U, "room", "L Room", {{0, 0}, {6, 5}}, 0.25});
  constexpr std::array<cr::CreativeTerrainCoord2, 6U> kVertices = {
      cr::CreativeTerrainCoord2{0, 0}, cr::CreativeTerrainCoord2{6, 0},
      cr::CreativeTerrainCoord2{6, 2}, cr::CreativeTerrainCoord2{2, 2},
      cr::CreativeTerrainCoord2{2, 5}, cr::CreativeTerrainCoord2{0, 5}};
  for (std::size_t index = 0U; index < kVertices.size(); ++index) {
    state.source.topologyVertices.push_back(
        {0U, "vertex_" + std::to_string(index), kVertices[index]});
  }
  constexpr std::array<std::array<std::size_t, 2U>, 6U> kEdges = {
      std::array<std::size_t, 2U>{0U, 1U}, {1U, 2U}, {3U, 2U},
      {3U, 4U}, {5U, 4U}, {0U, 5U}};
  constexpr std::array<bool, 6U> kReversed = {false, false, true,
                                               false, true, true};
  for (std::size_t index = 0U; index < kEdges.size(); ++index) {
    state.source.topologyEdges.push_back(
        {0U, "edge_" + std::to_string(index), kEdges[index][0],
         kEdges[index][1], 0.25, 4U});
    state.source.roomBoundaries.push_back({0U, index, index, kReversed[index]});
  }

  const auto tool = app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Door);
  const auto planned = app::planCreativeEditorWorldLayoutOpeningPlacement(
      state, {4.0, 2.1}, cr::CreativeBuildingOpeningKind::Door);
  const std::uint64_t revisionBefore = state.revision;
  const auto placed =
      app::applyCreativeEditorWorldLayoutPoint(state, {4.0, 2.1});

  return expect(tool.accepted && planned.accepted &&
                    planned.opening.roomTopologyEdgeIndex == 2U &&
                    planned.opening.roomEdge ==
                        cr::CreativeWorldLayoutRoomEdge::Count,
                "concave placement resolves the exact inner topology edge") &&
         expect(placed.accepted && placed.changed &&
                    state.revision == revisionBefore + 1U &&
                    state.source.openings.size() == 1U &&
                    state.source.openings[0].roomTopologyEdgeIndex == 2U &&
                    state.source.openings[0].roomEdge ==
                        cr::CreativeWorldLayoutRoomEdge::Count &&
                    state.source.openings[0].centerOffsetCells == 2.0,
                "editor commit retains direct host identity and snapped offset");
}

bool openingDragAndWidthHandlesAreQuarterCellTransactional() {
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state);
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Room));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {0, 0}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {10, 4}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Door));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(state, {2, 0}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Window));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(state, {7, 0}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Select));

  const auto moveTarget = app::findCreativeEditorWorldLayoutOpeningTarget(
      state, {2.0, 0.0}, 0.2);
  const std::uint64_t revisionBeforeMove = state.revision;
  const auto moveBegin = app::applyCreativeEditorWorldLayoutOpeningManipulation(
      state, app::CreativeEditorWorldLayoutOpeningManipulationPhase::Begin,
      {2.0, 0.0}, 0.2);
  const auto moveUpdate =
      app::applyCreativeEditorWorldLayoutOpeningManipulation(
          state, app::CreativeEditorWorldLayoutOpeningManipulationPhase::Update,
          {3.12, 0.0}, 0.2);
  const bool movePreviewOnly =
      state.source.openings[0].centerOffsetCells == 2.0 &&
      state.openingManipulation.previewCenterOffsetCells == 3.0 &&
      state.revision == revisionBeforeMove;
  const auto moveCommit =
      app::applyCreativeEditorWorldLayoutOpeningManipulation(
          state, app::CreativeEditorWorldLayoutOpeningManipulationPhase::Commit,
          {3.12, 0.0}, 0.2);
  const double centerAfterMove =
      state.source.openings[0].centerOffsetCells;
  const std::uint64_t revisionAfterMove = state.revision;

  const auto startTarget = app::findCreativeEditorWorldLayoutOpeningTarget(
      state, {2.5, 0.0}, 0.2);
  const std::uint64_t revisionBeforeResize = state.revision;
  static_cast<void>(app::applyCreativeEditorWorldLayoutOpeningManipulation(
      state, app::CreativeEditorWorldLayoutOpeningManipulationPhase::Begin,
      {2.5, 0.0}, 0.2));
  const auto resizeCommit =
      app::applyCreativeEditorWorldLayoutOpeningManipulation(
          state, app::CreativeEditorWorldLayoutOpeningManipulationPhase::Commit,
          {2.0, 0.0}, 0.2);
  const double centerAfterResize =
      state.source.openings[0].centerOffsetCells;
  const double widthAfterResize = state.source.openings[0].widthCells;
  const double insertWidthAfterResize =
      state.source.openings[0].insertWidthCells;
  const std::uint64_t revisionAfterResize = state.revision;

  const std::uint64_t revisionBeforeOverlap = state.revision;
  static_cast<void>(app::applyCreativeEditorWorldLayoutOpeningManipulation(
      state, app::CreativeEditorWorldLayoutOpeningManipulationPhase::Begin,
      {2.75, 0.0}, 0.2));
  const auto overlapPreview =
      app::applyCreativeEditorWorldLayoutOpeningManipulation(
          state, app::CreativeEditorWorldLayoutOpeningManipulationPhase::Update,
          {6.0, 0.0}, 0.2);
  const bool overlapShownInvalid =
      overlapPreview.accepted && !state.openingManipulation.previewValid;
  const auto overlapCommit =
      app::applyCreativeEditorWorldLayoutOpeningManipulation(
          state, app::CreativeEditorWorldLayoutOpeningManipulationPhase::Commit,
          {6.0, 0.0}, 0.2);

  return expect(moveTarget.handle ==
                        app::CreativeEditorWorldLayoutOpeningHandle::Move &&
                    moveBegin.accepted && moveUpdate.accepted &&
                    movePreviewOnly && moveCommit.accepted &&
                    moveCommit.changed &&
                    centerAfterMove == 3.0 &&
                    revisionAfterMove == revisionBeforeMove + 1U,
                "opening center drag previews and snaps to quarter cells") &&
         expect(startTarget.handle ==
                        app::CreativeEditorWorldLayoutOpeningHandle::Start &&
                    resizeCommit.accepted && resizeCommit.changed &&
                    revisionAfterResize == revisionBeforeResize + 1U &&
                    centerAfterResize == 2.75 && widthAfterResize == 1.5 &&
                    insertWidthAfterResize == 1.5,
                "opening start handle resizes cutout and tracked insert once") &&
         expect(overlapShownInvalid && !overlapCommit.accepted &&
                    state.revision == revisionBeforeOverlap &&
                    state.source.openings[0].centerOffsetCells == 2.75 &&
                    state.source.openings[0].widthCells == 1.5,
                "overlapping opening drag rejects before source mutation");
}

bool openingDragPreservesSharedRoomWallOwnership() {
  cr::CreativeAppState live = appState();
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state);
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Room));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {0, 0}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {4, 4}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {4, 0}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {8, 4}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Door));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(state, {4, 2}));
  const auto originalHostKind = state.source.openings[0].hostKind;
  const std::size_t originalRoomIndex = state.source.openings[0].roomIndex;
  const auto originalRoomEdge = state.source.openings[0].roomEdge;
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Select));
  static_cast<void>(app::applyCreativeEditorWorldLayoutOpeningManipulation(
      state, app::CreativeEditorWorldLayoutOpeningManipulationPhase::Begin,
      {4, 2}, 0.2));
  const auto moved = app::applyCreativeEditorWorldLayoutOpeningManipulation(
      state, app::CreativeEditorWorldLayoutOpeningManipulationPhase::Commit,
      {4, 2.8}, 0.2);
  const cr::CreativeWorldLayoutRoomCompileResult expanded =
      cr::expandCreativeWorldLayoutRooms(state.source);
  const auto preview =
      app::previewCreativeEditorWorldLayout(state, live.facade.document());

  return expect(moved.accepted && moved.changed &&
                    state.source.openings[0].hostKind == originalHostKind &&
                    state.source.openings[0].roomIndex == originalRoomIndex &&
                    state.source.openings[0].roomEdge == originalRoomEdge &&
                    state.source.openings[0].centerOffsetCells == 2.75,
                "shared-wall drag preserves authored room-edge ownership") &&
         expect(expanded.accepted &&
                    expanded.expanded.openings[0].hostKind ==
                        cr::CreativeWorldLayoutOpeningHostKind::Wall &&
                    expanded.expanded.walls
                            [expanded.expanded.openings[0].wallIndex]
                                .start == cr::CreativeTerrainCoord2{4, 0} &&
                    expanded.expanded.walls
                            [expanded.expanded.openings[0].wallIndex]
                                .end == cr::CreativeTerrainCoord2{4, 4},
                "shared-wall opening resolves onto one canonical wall") &&
         expect(preview.accepted,
                "shared-wall opening manipulation remains 3D-previewable");
}

bool minimumWidthOpeningRetainsMoveAndResizeTargets() {
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state);
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Wall));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(state, {0, 0}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(state, {4, 0}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Door));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(state, {2, 0}));
  app::CreativeEditorWorldLayoutOpeningSettings settings;
  static_cast<void>(app::readCreativeEditorWorldLayoutOpeningSettings(
      state, 0U, settings));
  settings.widthCells = 0.25;
  const auto narrowed = app::setCreativeEditorWorldLayoutOpeningSettings(
      state, 0U, settings);
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Select));
  const auto centerTarget = app::findCreativeEditorWorldLayoutOpeningTarget(
      state, {2.0, 0.0}, 0.2);
  const auto startTarget = app::findCreativeEditorWorldLayoutOpeningTarget(
      state, {1.875, 0.0}, 0.2);

  return expect(narrowed.accepted && narrowed.changed &&
                    centerTarget.handle ==
                        app::CreativeEditorWorldLayoutOpeningHandle::Move,
                "minimum-width opening center remains a move target") &&
         expect(startTarget.handle ==
                    app::CreativeEditorWorldLayoutOpeningHandle::Start,
                "minimum-width opening endpoint remains a resize target");
}

bool roomDeletionCascadesHostedOpeningsAndCancelIsEmpty() {
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state);
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Room));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {0, 0}));
  const auto restarted = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {2, 2});
  const bool restartReplacedAnchor =
      restarted.accepted && !restarted.changed && state.source.rooms.empty() &&
      state.anchor == cr::CreativeTerrainCoord2{2, 2};
  const auto cancelled = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Cancel);
  const std::uint64_t revisionAfterCancel = state.revision;
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {0, 0}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {5, 4}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Window));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(state, {5, 2}));
  state.selection = {app::CreativeEditorWorldLayoutSelectionKind::Room, 0U};
  const auto removed = app::deleteCreativeEditorWorldLayoutSelection(state);

  return expect(restartReplacedAnchor,
                "repeated begin replaces a stale anchor without committing") &&
         expect(cancelled.accepted && !cancelled.changed &&
                    revisionAfterCancel == 1U,
                "cancelled room draft records no source revision") &&
         expect(removed.accepted && removed.changed &&
                    state.source.rooms.empty() &&
                    state.source.openings.empty(),
                "room deletion removes its hosted openings");
}

bool exactPreviewAndConfirmUseOneHistoryEntry() {
  cr::CreativeAppState live = appState();
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state);
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Room));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {0, 0}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {6, 5}));
  const auto settings = app::setCreativeEditorWorldLayoutRoomSettings(
      state, 0U, {{{0, 0}, {8, 6}}, 1.0, 4U, 0.5, 2U});

  const std::uint64_t liveCountBefore = live.facade.document().objectCount();
  const auto preview =
      app::previewCreativeEditorWorldLayout(state, live.facade.document());
  const cr::CreativeDocument& rendered =
      app::creativeEditorWorldLayoutRenderDocument(state,
                                                   live.facade.document());
  const std::uint64_t previewObjectCount = rendered.objectCount();
  std::uint64_t floorCount = 0U;
  std::uint64_t roofCount = 0U;
  std::uint64_t wallCount = 0U;
  bool linkedByLayout = true;
  const cr::CreativeObject* floor = nullptr;
  const cr::CreativeObject* wall = nullptr;
  const std::string layoutTag = cr::creativeWorldLayoutTag("world_layout");
  for (const cr::CreativeObject& object : rendered.objects()) {
    floorCount += object.kind == cr::CreativeObjectKind::Floor ? 1U : 0U;
    roofCount += object.kind == cr::CreativeObjectKind::Roof ? 1U : 0U;
    wallCount += object.kind == cr::CreativeObjectKind::Wall ? 1U : 0U;
    if (floor == nullptr && object.kind == cr::CreativeObjectKind::Floor) {
      floor = &object;
    }
    if (wall == nullptr && object.kind == cr::CreativeObjectKind::Wall) {
      wall = &object;
    }
    linkedByLayout =
        linkedByLayout &&
        std::find(object.tags.begin(), object.tags.end(), layoutTag) !=
            object.tags.end();
  }
  const cr::CreativeTransformedBounds floorGeometry =
      floor == nullptr ? cr::CreativeTransformedBounds{}
                       : cr::resolveCreativeObjectBounds(*floor);
  const cr::CreativeTransformedBounds wallGeometry =
      wall == nullptr ? cr::CreativeTransformedBounds{}
                      : cr::resolveCreativeObjectBounds(*wall);
  const bool previewDidNotPublish =
      live.facade.document().objectCount() == liveCountBefore;
  const auto applied = app::confirmCreativeEditorWorldLayout(state, live);
  const bool appliedOnce =
      applied.accepted && applied.changed &&
      live.facade.document().objectCount() == 6U &&
      cr::creativeUndoDepth(live.history) == 1U;
  const bool undone = app::undoLastEdit(live, "layout-undo", &state);
  const bool undoRestoredBoth =
      undone && live.facade.document().objectCount() == liveCountBefore &&
      state.source.rooms.empty() && state.generatedRevision == state.revision;
  const bool redone = app::redoLastEdit(live, "layout-redo", &state);
  const bool redoRestoredBoth =
      redone && live.facade.document().objectCount() == 6U &&
      state.source.rooms.size() == 1U &&
      state.generatedRevision == state.revision;

  return expect(settings.accepted && settings.changed,
                "room shell settings are accepted before generation") &&
         expect(preview.accepted &&
                    app::creativeEditorWorldLayoutPreviewActive(state) == false,
                "confirm closes an accepted exact preview") &&
         expect(previewObjectCount == 6U && floorCount == 1U &&
                    roofCount == 1U && wallCount == 4U && linkedByLayout &&
                    previewDidNotPublish,
                "preview renders one floor, one roof, and four walls without "
                "publishing") &&
         expect(
             floorGeometry.valid && wallGeometry.valid &&
                 near(floorGeometry.size.y, 0.1) &&
                 near(wallGeometry.worldBounds.min.y, 1.0) &&
                 near(wallGeometry.worldBounds.max.y, 5.0) &&
                 near(std::min(wallGeometry.size.x, wallGeometry.size.z), 0.5),
             "preview geometry matches floor, elevation, height, and thickness "
             "settings") &&
         expect(appliedOnce, "confirm publishes generated output") &&
         expect(
             undoRestoredBoth,
             "layout undo restores semantic source and generated document") &&
         expect(redoRestoredBoth,
                "layout redo restores semantic source and generated document");
}

bool buildingLevelLifecycleIsAtomicAndRemapsHostedSymbols() {
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state);
  const auto shell = app::createCreativeEditorWorldLayoutBuildingShell(
      state, {{{0, 0}, {6, 4}}, 0.0, 3U, 0.25, 1U});
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Door));
  const auto door =
      app::applyCreativeEditorWorldLayoutPoint(state, {3.0, 0.1});
  const std::uint64_t revisionBeforeLevels = state.revision;

  const auto selected = app::applyCreativeEditorWorldLayoutLevelOperation(
      state, app::CreativeEditorWorldLayoutLevelOperation::Select, 0U, 0U);
  const bool selectionIsViewOnly =
      selected.accepted && state.revision == revisionBeforeLevels &&
      state.activeLevelIndex == 0U;
  const auto added = app::applyCreativeEditorWorldLayoutLevelOperation(
      state, app::CreativeEditorWorldLayoutLevelOperation::Add, 0U);
  const bool emptyLevelAdded =
      added.accepted && added.changed && state.source.levels.size() == 2U &&
      state.source.rooms.size() == 1U && state.activeLevelIndex == 1U &&
      state.source.levels[1].floorTopLayer == 3.0;

  const auto duplicated = app::applyCreativeEditorWorldLayoutLevelOperation(
      state, app::CreativeEditorWorldLayoutLevelOperation::Duplicate, 0U, 0U);
  const bool duplicateRemapped =
      duplicated.accepted && duplicated.changed &&
      state.source.levels.size() == 3U && state.source.rooms.size() == 2U &&
      state.source.openings.size() == 2U && state.activeLevelIndex == 2U &&
      state.source.levels[2].floorTopLayer == 6.0 &&
      state.source.rooms[1].levelIndex == 2U &&
      state.source.openings[1].roomIndex == 1U && stableKeysUnique(state.source);

  const auto reordered = app::applyCreativeEditorWorldLayoutLevelOperation(
      state, app::CreativeEditorWorldLayoutLevelOperation::MoveEarlier, 0U,
      2U);
  const bool reorderRemapped =
      reordered.accepted && reordered.changed && state.activeLevelIndex == 1U &&
      state.source.rooms[1].levelIndex == 1U &&
      state.source.levels[1].floorTopLayer == 6.0 &&
      state.source.levels[2].floorTopLayer == 3.0;

  const auto deletedCopy = app::applyCreativeEditorWorldLayoutLevelOperation(
      state, app::CreativeEditorWorldLayoutLevelOperation::Delete, 0U, 1U);
  const bool copyDeletedAtomically =
      deletedCopy.accepted && deletedCopy.changed &&
      state.source.levels.size() == 2U && state.source.rooms.size() == 1U &&
      state.source.openings.size() == 1U && state.activeLevelIndex == 0U;
  const auto deletedEmpty = app::applyCreativeEditorWorldLayoutLevelOperation(
      state, app::CreativeEditorWorldLayoutLevelOperation::Delete, 0U, 1U);
  const std::uint64_t revisionBeforeLastDelete = state.revision;
  const auto rejectedLast = app::applyCreativeEditorWorldLayoutLevelOperation(
      state, app::CreativeEditorWorldLayoutLevelOperation::Delete, 0U, 0U);
  const bool lastLevelProtected =
      deletedEmpty.accepted && deletedEmpty.changed &&
      !rejectedLast.accepted && !rejectedLast.changed &&
      state.source.levels.size() == 1U && state.source.rooms.size() == 1U &&
      state.revision == revisionBeforeLastDelete;

  app::CreativeEditorWorldLayoutState legacyState;
  app::resetCreativeEditorWorldLayout(legacyState, "legacy_building");
  cr::CreativeWorldLayoutBuilding legacyBuilding;
  legacyBuilding.stableKey = "legacy_building";
  legacyBuilding.name = "Legacy Building";
  legacyBuilding.rootBaseLayer = 2;
  legacyBuilding.rootHeightCells = 4U;
  legacyState.source.buildings.push_back(std::move(legacyBuilding));
  const auto firstLevel = app::applyCreativeEditorWorldLayoutLevelOperation(
      legacyState, app::CreativeEditorWorldLayoutLevelOperation::Add, 0U);
  const bool firstLevelCreated =
      firstLevel.accepted && firstLevel.changed &&
      legacyState.source.levels.size() == 1U &&
      legacyState.source.levels[0].buildingIndex == 0U &&
      legacyState.source.levels[0].floorTopLayer == 2.0 &&
      legacyState.source.levels[0].wallHeightCells == 4U &&
      legacyState.activeLevelIndex == 0U;

  return expect(shell.accepted && door.accepted && selectionIsViewOnly,
                "level selection changes the editor view without editing "
                "source") &&
         expect(emptyLevelAdded, "add level derives the next story plane "
                                 "without fabricating rooms") &&
         expect(duplicateRemapped, "duplicate level copies rooms and hosted "
                                   "openings with fresh keys") &&
         expect(reorderRemapped,
                "level reordering remaps room ownership atomically") &&
         expect(copyDeletedAtomically,
                "deleting a level removes its rooms and hosted openings") &&
         expect(lastLevelProtected,
                "a building cannot lose its final level recipe") &&
         expect(firstLevelCreated,
                "a level-less legacy building can acquire its first level");
}

bool prepareTwoStoreyEditorLayout(app::CreativeEditorWorldLayoutState& state,
                                  std::string layoutKey) {
  app::resetCreativeEditorWorldLayout(state, std::move(layoutKey));
  const auto shell = app::createCreativeEditorWorldLayoutBuildingShell(
      state, {{{0, 0}, {8, 6}}, 0.0, 4U, 0.25, 1U});
  const auto upperLevel = app::applyCreativeEditorWorldLayoutLevelOperation(
      state, app::CreativeEditorWorldLayoutLevelOperation::Add, 0U);
  if (!shell.accepted || !upperLevel.accepted || state.source.rooms.empty()) {
    return false;
  }
  cr::CreativeWorldLayoutRoom upperRoom = state.source.rooms.front();
  upperRoom.levelIndex = 1U;
  upperRoom.stableKey = "room_upper";
  upperRoom.name = "Upper Room";
  state.source.rooms.push_back(std::move(upperRoom));
  state.activeLevelIndex = 0U;
  return true;
}

bool stairGestureOwnsConnectorLifecycleAcrossLevels() {
  app::CreativeEditorWorldLayoutState state;
  const bool setupAccepted =
      prepareTwoStoreyEditorLayout(state, "stair_layout");
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Stair));
  const std::uint64_t revisionBeforeStair = state.revision;
  const auto begin = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {1, 2});
  const auto commit = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {5, 4});
  const bool stairAuthoredOnce =
      setupAccepted && begin.accepted && !begin.changed && commit.accepted &&
      commit.changed && state.revision == revisionBeforeStair + 1U &&
      state.source.verticalConnectors.size() == 1U &&
      state.source.verticalConnectors[0].buildingIndex == 0U &&
      state.source.verticalConnectors[0].lowerRoomIndex == 0U &&
      state.source.verticalConnectors[0].upperRoomIndex == 1U &&
      state.source.verticalConnectors[0].direction ==
          cr::CreativeWorldLayoutVerticalDirection::PositiveX &&
      state.source.verticalConnectors[0].footprint.minimum ==
          cr::CreativeTerrainCoord2{1, 2} &&
      state.source.verticalConnectors[0].footprint.maximum ==
          cr::CreativeTerrainCoord2{5, 4} &&
      state.selection.kind ==
          app::CreativeEditorWorldLayoutSelectionKind::VerticalConnector &&
      stableKeysUnique(state.source);

  cr::CreativeAppState live = appState();
  const auto preview =
      app::previewCreativeEditorWorldLayout(state, live.facade.document());
  const auto upperSelected = app::applyCreativeEditorWorldLayoutLevelOperation(
      state, app::CreativeEditorWorldLayoutLevelOperation::Select, 0U, 1U);
  const bool visibleFromEitherLevel =
      preview.accepted && state.preview.document.isValid() &&
      upperSelected.accepted && upperSelected.changed &&
      state.activeLevelIndex == 1U &&
      state.selection.kind ==
          app::CreativeEditorWorldLayoutSelectionKind::VerticalConnector;

  app::CreativeEditorWorldLayoutState roomDeleteState = state;
  roomDeleteState.selection = {
      app::CreativeEditorWorldLayoutSelectionKind::Room, 0U};
  const auto roomDeleted =
      app::deleteCreativeEditorWorldLayoutSelection(roomDeleteState);
  const bool roomCascade = roomDeleted.accepted && roomDeleted.changed &&
                           roomDeleteState.source.rooms.size() == 1U &&
                           roomDeleteState.source.verticalConnectors.empty();

  state.selection = {
      app::CreativeEditorWorldLayoutSelectionKind::VerticalConnector, 0U};
  const auto connectorDeleted =
      app::deleteCreativeEditorWorldLayoutSelection(state);
  const bool connectorDelete = connectorDeleted.accepted &&
                               connectorDeleted.changed &&
                               state.source.verticalConnectors.empty();

  state.activeLevelIndex = 0U;
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Stair));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {1, 2}));
  const auto recreated = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {5, 4});
  const auto levelDeleted = app::applyCreativeEditorWorldLayoutLevelOperation(
      state, app::CreativeEditorWorldLayoutLevelOperation::Delete, 0U, 1U);
  const bool levelCascade = recreated.accepted && recreated.changed &&
                            levelDeleted.accepted && levelDeleted.changed &&
                            state.source.levels.size() == 1U &&
                            state.source.rooms.size() == 1U &&
                            state.source.verticalConnectors.empty();

  const std::uint64_t revisionBeforeRejected = state.revision;
  state.activeLevelIndex = 0U;
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {1, 2}));
  const auto noUpper = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {5, 4});
  const bool missingUpperFailsClosed =
      !noUpper.accepted && !noUpper.changed &&
      state.source.verticalConnectors.empty() &&
      state.revision == revisionBeforeRejected;

  return expect(stairAuthoredOnce,
                "one stair drag authors one directional connector revision") &&
         expect(
             visibleFromEitherLevel,
             "stair compiles and remains selected from either owned level") &&
         expect(roomCascade,
                "deleting an owned room removes its vertical connector") &&
         expect(connectorDelete,
                "vertical connector selection deletes without other symbols") &&
         expect(
             levelCascade,
             "deleting a level removes connectors that reference its rooms") &&
         expect(
             missingUpperFailsClosed,
             "stair authoring rejects a missing upper level transactionally");
}

bool rampGestureUsesTheSharedVerticalConnectorLifecycle() {
  app::CreativeEditorWorldLayoutState state;
  const bool setupAccepted = prepareTwoStoreyEditorLayout(state, "ramp_layout");
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Ramp));
  const std::uint64_t revisionBeforeRamp = state.revision;
  const auto begin = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {1, 2});
  const auto commit = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {7, 4});

  cr::CreativeAppState live = appState();
  const auto preview =
      app::previewCreativeEditorWorldLayout(state, live.facade.document());
  const bool rampGenerated =
      preview.accepted &&
      std::any_of(state.preview.document.objects().begin(),
                  state.preview.document.objects().end(),
                  [](const cr::CreativeObject& object) {
                    return object.kind == cr::CreativeObjectKind::Ramp;
                  });

  app::CreativeEditorWorldLayoutState lowHeadroomState;
  const bool lowHeadroomSetup = prepareTwoStoreyEditorLayout(
      lowHeadroomState, "ramp_low_headroom_layout");
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      lowHeadroomState, app::CreativeEditorWorldLayoutTool::Ramp));
  const cr::CreativeGridSettings lowHeadroomGrid{
      {}, 0.4, {32U, 16U, 32U}};
  const std::uint64_t lowHeadroomRevision = lowHeadroomState.revision;
  const auto lowHeadroomBegin = app::applyCreativeEditorWorldLayoutGesture(
      lowHeadroomState, app::CreativeEditorWorldLayoutGesturePhase::Begin,
      {1, 2}, lowHeadroomGrid);
  const auto lowHeadroomCommit = app::applyCreativeEditorWorldLayoutGesture(
      lowHeadroomState, app::CreativeEditorWorldLayoutGesturePhase::Commit,
      {7, 4}, lowHeadroomGrid);

  return expect(setupAccepted && begin.accepted && !begin.changed &&
                    commit.accepted && commit.changed &&
                    commit.reasonCode ==
                        "creative_editor_world_layout_ramp_added" &&
                    state.revision == revisionBeforeRamp + 1U &&
                    state.source.verticalConnectors.size() == 1U,
                "one ramp drag authors one connector revision") &&
         expect(state.source.verticalConnectors[0].kind ==
                        cr::CreativeWorldLayoutVerticalConnectorKind::Ramp &&
                    state.source.verticalConnectors[0].direction ==
                        cr::CreativeWorldLayoutVerticalDirection::PositiveX &&
                    state.source.verticalConnectors[0].stableKey.starts_with(
                        "ramp_") &&
                    state.source.verticalConnectors[0].name == "Ramp 1",
                "ramp tool owns kind direction identity and label") &&
         expect(rampGenerated,
                "ramp preview compiles through the shared layout recipe") &&
         expect(lowHeadroomSetup && lowHeadroomBegin.accepted &&
                    !lowHeadroomCommit.accepted &&
                    !lowHeadroomCommit.changed &&
                    lowHeadroomCommit.reasonCode ==
                        "creative_ramp_headroom_insufficient" &&
                    lowHeadroomState.source.verticalConnectors.empty() &&
                    lowHeadroomState.revision == lowHeadroomRevision,
                "document-grid headroom rejects ramp creation atomically");
}

bool verticalConnectorSettingsConvertAndRejectAtomically() {
  app::CreativeEditorWorldLayoutState state;
  const bool setupAccepted =
      prepareTwoStoreyEditorLayout(state, "connector_settings_layout");
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Stair));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {1, 1}));
  const auto created = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {7, 3});
  if (!setupAccepted || !created.accepted ||
      state.source.verticalConnectors.size() != 1U) {
    return expect(false, "vertical connector settings test setup");
  }
  const cr::CreativeWorldLayoutVerticalConnector original =
      state.source.verticalConnectors[0];
  app::CreativeEditorWorldLayoutVerticalConnectorSettings settings;
  const bool read =
      app::readCreativeEditorWorldLayoutVerticalConnectorSettings(
          state, 0U, settings);
  const bool readDefaultMaterial =
      read && settings.material == cr::CreativeStructuralMaterial::Blockout;
  settings.kind = cr::CreativeWorldLayoutVerticalConnectorKind::Ramp;
  settings.direction =
      cr::CreativeWorldLayoutVerticalDirection::PositiveX;
  settings.material = cr::CreativeStructuralMaterial::Stone;
  const std::uint64_t revisionBeforeConvert = state.revision;
  const auto converted =
      app::setCreativeEditorWorldLayoutVerticalConnectorSettings(
          state, 0U, settings);
  const bool convertedOnce =
      readDefaultMaterial && converted.accepted && converted.changed &&
      state.revision == revisionBeforeConvert + 1U &&
      state.source.verticalConnectors[0].kind ==
          cr::CreativeWorldLayoutVerticalConnectorKind::Ramp &&
      state.source.verticalConnectors[0].direction ==
          cr::CreativeWorldLayoutVerticalDirection::PositiveX &&
      state.source.verticalConnectors[0].material ==
          cr::CreativeStructuralMaterial::Stone &&
      state.source.verticalConnectors[0].stableKey == original.stableKey &&
      state.source.verticalConnectors[0].name == original.name &&
      state.source.verticalConnectors[0].buildingIndex ==
          original.buildingIndex &&
      state.source.verticalConnectors[0].lowerRoomIndex ==
          original.lowerRoomIndex &&
      state.source.verticalConnectors[0].upperRoomIndex ==
          original.upperRoomIndex;

  const cr::CreativeWorldLayoutVerticalConnector convertedSource =
      state.source.verticalConnectors[0];
  const std::uint64_t revisionBeforeRejected = state.revision;
  settings.footprint = {{1, 1}, {2, 5}};
  settings.direction =
      cr::CreativeWorldLayoutVerticalDirection::PositiveX;
  const auto rejectedSlope =
      app::setCreativeEditorWorldLayoutVerticalConnectorSettings(
          state, 0U, settings);
  settings = {convertedSource.footprint,
              cr::CreativeWorldLayoutVerticalConnectorKind::Count,
              convertedSource.direction,
              convertedSource.material};
  const auto rejectedKind =
      app::setCreativeEditorWorldLayoutVerticalConnectorSettings(
          state, 0U, settings);
  settings = {convertedSource.footprint,
              convertedSource.kind,
              convertedSource.direction,
              cr::CreativeStructuralMaterial::Count};
  const auto rejectedMaterial =
      app::setCreativeEditorWorldLayoutVerticalConnectorSettings(
          state, 0U, settings);
  const bool rejectedAtomically =
      !rejectedSlope.accepted && !rejectedSlope.changed &&
      rejectedSlope.reasonCode ==
          "creative_ramp_slope_exceeds_movement_limit" &&
      !rejectedKind.accepted && !rejectedKind.changed &&
      !rejectedMaterial.accepted && !rejectedMaterial.changed &&
      rejectedMaterial.reasonCode ==
          "creative_world_layout_vertical_connector_material_invalid" &&
      state.revision == revisionBeforeRejected &&
      state.source.verticalConnectors[0].footprint.minimum ==
          convertedSource.footprint.minimum &&
      state.source.verticalConnectors[0].footprint.maximum ==
          convertedSource.footprint.maximum &&
      state.source.verticalConnectors[0].kind == convertedSource.kind &&
      state.source.verticalConnectors[0].direction ==
          convertedSource.direction &&
      state.source.verticalConnectors[0].material == convertedSource.material;

  return expect(convertedOnce,
                "connector conversion preserves identity and commits once") &&
         expect(rejectedAtomically,
                "invalid connector edits reject without partial mutation");
}

bool verticalConnectorManipulationIsTransactional() {
  app::CreativeEditorWorldLayoutState state;
  const bool setupAccepted =
      prepareTwoStoreyEditorLayout(state, "connector_manipulation_layout");
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Stair));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {1, 1}));
  const auto created = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {5, 5});
  app::CreativeEditorWorldLayoutVerticalConnectorSettings settings;
  const bool settingsRead =
      app::readCreativeEditorWorldLayoutVerticalConnectorSettings(
          state, 0U, settings);
  settings.material = cr::CreativeStructuralMaterial::Stone;
  const auto materialSet =
      app::setCreativeEditorWorldLayoutVerticalConnectorSettings(
          state, 0U, settings);
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Select));
  if (!setupAccepted || !created.accepted || !settingsRead ||
      !materialSet.accepted) {
    return expect(false, "vertical connector manipulation test setup");
  }

  const std::string stableKey =
      state.source.verticalConnectors[0].stableKey;
  const std::uint64_t revisionBeforeMove = state.revision;
  const auto moveBegin =
      app::applyCreativeEditorWorldLayoutVerticalConnectorManipulation(
          state,
          app::CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
              Begin,
          {3.0, 3.0}, 0.2);
  const auto moveUpdate =
      app::applyCreativeEditorWorldLayoutVerticalConnectorManipulation(
          state,
          app::CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
              Update,
          {4.0, 3.0}, 0.2);
  const bool movePreviewOnly =
      moveBegin.accepted && moveUpdate.accepted &&
      state.verticalConnectorManipulation.previewValid &&
      state.verticalConnectorManipulation.previewFootprint.minimum ==
          cr::CreativeTerrainCoord2{2, 1} &&
      state.source.verticalConnectors[0].footprint.minimum ==
          cr::CreativeTerrainCoord2{1, 1} &&
      state.revision == revisionBeforeMove;
  const auto moveCommit =
      app::applyCreativeEditorWorldLayoutVerticalConnectorManipulation(
          state,
          app::CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
              Commit,
          {4.0, 3.0}, 0.2);
  const bool moveCommittedOnce =
      moveCommit.accepted && moveCommit.changed &&
      state.revision == revisionBeforeMove + 1U &&
      state.source.verticalConnectors[0].footprint.minimum ==
          cr::CreativeTerrainCoord2{2, 1} &&
      state.source.verticalConnectors[0].footprint.maximum ==
          cr::CreativeTerrainCoord2{6, 5} &&
      state.source.verticalConnectors[0].stableKey == stableKey &&
      state.source.verticalConnectors[0].material ==
          cr::CreativeStructuralMaterial::Stone;

  const cr::CreativeWorldLayoutRect movedFootprint =
      state.source.verticalConnectors[0].footprint;
  const std::uint64_t revisionBeforeInvalid = state.revision;
  const auto resizeBegin =
      app::applyCreativeEditorWorldLayoutVerticalConnectorManipulation(
          state,
          app::CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
              Begin,
          {6.0, 3.0}, 0.2);
  const auto resizeUpdate =
      app::applyCreativeEditorWorldLayoutVerticalConnectorManipulation(
          state,
          app::CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
              Update,
          {3.0, 3.0}, 0.2);
  const bool invalidPreviewVisible =
      resizeBegin.accepted && resizeUpdate.accepted &&
      !state.verticalConnectorManipulation.previewValid &&
      state.verticalConnectorManipulation.reasonCode ==
          "creative_world_layout_vertical_connector_slope_invalid";
  const auto resizeCommit =
      app::applyCreativeEditorWorldLayoutVerticalConnectorManipulation(
          state,
          app::CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
              Commit,
          {3.0, 3.0}, 0.2);
  const bool invalidCommitRolledBack =
      !resizeCommit.accepted && !resizeCommit.changed &&
      state.revision == revisionBeforeInvalid &&
      state.source.verticalConnectors[0].footprint.minimum ==
          movedFootprint.minimum &&
      state.source.verticalConnectors[0].footprint.maximum ==
          movedFootprint.maximum;

  const auto cancelBegin =
      app::applyCreativeEditorWorldLayoutVerticalConnectorManipulation(
          state,
          app::CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
              Begin,
          {4.0, 3.0}, 0.2);
  static_cast<void>(
      app::applyCreativeEditorWorldLayoutVerticalConnectorManipulation(
          state,
          app::CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
              Update,
          {3.0, 3.0}, 0.2));
  const auto cancelled =
      app::applyCreativeEditorWorldLayoutVerticalConnectorManipulation(
          state,
          app::CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
              Cancel);
  const bool cancelIsEmpty =
      cancelBegin.accepted && cancelled.accepted && cancelled.changed &&
      state.revision == revisionBeforeInvalid &&
      state.source.verticalConnectors[0].footprint.minimum ==
          movedFootprint.minimum;

  static_cast<void>(
      app::applyCreativeEditorWorldLayoutVerticalConnectorManipulation(
          state,
          app::CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
              Begin,
          {4.0, 3.0}, 0.2));
  const auto interrupted = app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Ramp);
  const bool toolChangeFinalizesInteraction =
      interrupted.accepted && interrupted.changed &&
      !state.verticalConnectorManipulation.active &&
      state.revision == revisionBeforeInvalid &&
      state.source.verticalConnectors[0].footprint.minimum ==
          movedFootprint.minimum;
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Select));

  static_cast<void>(
      app::applyCreativeEditorWorldLayoutVerticalConnectorManipulation(
          state,
          app::CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
              Begin,
          {4.0, 3.0}, 0.2));
  ++state.revision;
  const auto stale =
      app::applyCreativeEditorWorldLayoutVerticalConnectorManipulation(
          state,
          app::CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
              Update,
          {5.0, 3.0}, 0.2);
  const bool staleFailsClosed =
      !stale.accepted && !stale.changed &&
      stale.reasonCode ==
          "creative_editor_world_layout_vertical_connector_manipulation_stale" &&
      !state.verticalConnectorManipulation.active &&
      state.source.verticalConnectors[0].footprint.minimum ==
          movedFootprint.minimum;

  return expect(movePreviewOnly && moveCommittedOnce,
                "connector move previews source-free and commits once") &&
         expect(invalidPreviewVisible && invalidCommitRolledBack,
                "invalid connector resize stays visible then rolls back") &&
         expect(cancelIsEmpty && toolChangeFinalizesInteraction &&
                    staleFailsClosed,
                "connector cancel, interruption, and stale revision mutate no source");
}

bool verticalConnectorDirectionHandleOwnsCardinalRise() {
  app::CreativeEditorWorldLayoutState state;
  const bool setupAccepted =
      prepareTwoStoreyEditorLayout(state, "connector_direction_layout");
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Stair));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {1, 1}));
  const auto created = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {5, 5});
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Select));
  if (!setupAccepted || !created.accepted) {
    return expect(false, "vertical connector direction test setup");
  }

  app::CreativeEditorWorldLayoutPoint low;
  app::CreativeEditorWorldLayoutPoint high;
  app::CreativeEditorWorldLayoutPoint handlePoint;
  const bool axisResolved =
      app::resolveCreativeEditorWorldLayoutVerticalConnectorAxis(
          state.source.verticalConnectors[0].footprint,
          state.source.verticalConnectors[0].direction, low, high) &&
      app::resolveCreativeEditorWorldLayoutVerticalConnectorDirectionHandle(
          state.source.verticalConnectors[0].footprint,
          state.source.verticalConnectors[0].direction, handlePoint);
  const auto target =
      app::findCreativeEditorWorldLayoutVerticalConnectorTarget(
          state, handlePoint, 0.2);
  const std::uint64_t revisionBefore = state.revision;
  const auto began =
      app::applyCreativeEditorWorldLayoutVerticalConnectorManipulation(
          state,
          app::CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
              Begin,
          handlePoint, 0.2);
  const auto updated =
      app::applyCreativeEditorWorldLayoutVerticalConnectorManipulation(
          state,
          app::CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
              Update,
          {3.0, 0.0}, 0.2);
  const bool directionPreviewOnly =
      axisResolved && low.x == 1.0 && low.z == 3.0 && high.x == 5.0 &&
      high.z == 3.0 &&
      std::fabs(handlePoint.x - 5.65) < 1.0e-9 && handlePoint.z == 3.0 &&
      target.directionHandle && target.connectorIndex == 0U &&
      began.accepted && updated.accepted &&
      state.verticalConnectorManipulation.previewValid &&
      state.verticalConnectorManipulation.previewDirection ==
          cr::CreativeWorldLayoutVerticalDirection::NegativeZ &&
      state.source.verticalConnectors[0].direction ==
          cr::CreativeWorldLayoutVerticalDirection::PositiveX &&
      state.revision == revisionBefore;
  const auto committed =
      app::applyCreativeEditorWorldLayoutVerticalConnectorManipulation(
          state,
          app::CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
              Commit,
          {3.0, 0.0}, 0.2);
  return expect(directionPreviewOnly && committed.accepted &&
                    committed.changed &&
                    state.revision == revisionBefore + 1U &&
                    state.source.verticalConnectors[0].direction ==
                        cr::CreativeWorldLayoutVerticalDirection::NegativeZ,
                "external direction handle previews and commits cardinal rise");
}

cr::CreativeWorldLayout elevationFixture() {
  cr::CreativeWorldLayout layout;
  layout.stableKey = "elevation_fixture";
  layout.buildings.push_back({"elevation_house",
                              "Elevation House",
                              cr::CreativeBuildingRootMode::None,
                              {},
                              0,
                              3U,
                              true,
                              {}});
  layout.levels.push_back(
      {0U, "elevation_ground", "Ground", 0.0, 4U, 1U, 1U, 1U});
  layout.levels.push_back(
      {0U, "elevation_upper", "Upper", 4.0, 4U, 1U, 1U, 1U,
       cr::CreativeStructuralRoofStyle::Gable,
       cr::CreativeStructuralRoofRidgeAxis::X, 45.0, 0.0});
  layout.rooms.push_back({0U, 0U, "elevation_ground_room", "Ground Room",
                          {{0, 0}, {8, 6}}, 0.25});
  layout.rooms.push_back({0U, 1U, "elevation_upper_room", "Upper Room",
                          {{0, 0}, {8, 6}}, 0.25});
  layout.boxes.push_back({0U, cr::CreativeObjectKind::Floor,
                          "elevation_terrace", "Terrace",
                          {{10, 0}, {12, 2}}, 0.0, 1U});
  layout.walls.push_back({0U, "elevation_partition", "Partition",
                          {10, 0}, {10, 4}, 0.0, 3U, 0.25});
  layout.verticalConnectors.push_back(
      {0U,
       0U,
       1U,
       cr::CreativeWorldLayoutVerticalConnectorKind::Stair,
       cr::CreativeWorldLayoutVerticalDirection::PositiveX,
       "elevation_stair",
       "Main Stair",
       {{1, 2}, {5, 4}}});
  cr::CreativeWorldLayoutOpening door;
  door.hostKind = cr::CreativeWorldLayoutOpeningHostKind::RoomEdge;
  door.roomIndex = 0U;
  door.roomEdge = cr::CreativeWorldLayoutRoomEdge::North;
  door.kind = cr::CreativeBuildingOpeningKind::Door;
  door.stableKey = "elevation_door";
  door.name = "Front Door";
  door.centerOffsetCells = 3.0;
  door.widthCells = 1.0;
  door.cutoutHeightCells = 2.0;
  layout.openings.push_back(std::move(door));

  cr::CreativeWorldLayoutOpening window;
  window.hostKind = cr::CreativeWorldLayoutOpeningHostKind::RoomEdge;
  window.roomIndex = 0U;
  window.roomEdge = cr::CreativeWorldLayoutRoomEdge::East;
  window.kind = cr::CreativeBuildingOpeningKind::Window;
  window.stableKey = "elevation_window";
  window.name = "East Window";
  window.centerOffsetCells = 3.0;
  window.widthCells = 1.0;
  window.cutoutBottomCells = 1.0;
  window.cutoutHeightCells = 1.5;
  layout.openings.push_back(std::move(window));
  return layout;
}

bool verticalConnectorHandlesShareElevationAnd3dGeometry() {
  const cr::CreativeGridSettings grid{{0.0, 0.0, 0.0}, 1.0,
                                      {32, 16, 32}};
  app::CreativeEditorWorldLayoutState state;
  state.source = elevationFixture();
  state.revision = 7U;
  state.generatedRevision = 7U;
  state.tool = app::CreativeEditorWorldLayoutTool::Select;
  state.activeLevelIndex = 0U;
  state.selection = {
      app::CreativeEditorWorldLayoutSelectionKind::VerticalConnector, 0U};

  const auto elevationX = app::planCreativeEditorWorldLayoutElevation(
      {&state.source, grid, 0U,
       app::CreativeEditorWorldLayoutElevationAxis::X});
  const auto elevationZ = app::planCreativeEditorWorldLayoutElevation(
      {&state.source, grid, 0U,
       app::CreativeEditorWorldLayoutElevationAxis::Z});
  const auto elevationHandle =
      [&](app::CreativeEditorWorldLayoutElevationHandleKind kind) {
        return std::find_if(
            elevationX.handles.begin(), elevationX.handles.end(),
            [kind](const auto& handle) {
              return handle.kind == kind &&
                     handle.sourceKind ==
                         app::CreativeEditorWorldLayoutElevationSourceKind::
                             VerticalConnector &&
                     handle.sourceIndex == 0U;
            });
      };
  const auto low = elevationHandle(
      app::CreativeEditorWorldLayoutElevationHandleKind::ConnectorRunLow);
  const auto high = elevationHandle(
      app::CreativeEditorWorldLayoutElevationHandleKind::ConnectorRunHigh);
  const std::size_t perpendicularHandleCount =
      static_cast<std::size_t>(std::count_if(
          elevationZ.handles.begin(), elevationZ.handles.end(),
          [](const auto& handle) {
            return handle.sourceKind ==
                   app::CreativeEditorWorldLayoutElevationSourceKind::
                       VerticalConnector;
          }));

  const auto frame =
      app::buildCreativeEditorWorldLayoutVerticalConnectorHandleFrame(
          state, grid, 0U);
  const app::CreativeEditorWorldLayoutVerticalConnectorTarget eastTarget{
      0U, app::CreativeEditorWorldLayoutRectHandle::East, false};
  const app::CreativeEditorWorldLayoutVerticalConnectorTarget moveTarget{
      0U, app::CreativeEditorWorldLayoutRectHandle::Move, false};
  const app::CreativeEditorWorldLayoutVerticalConnectorTarget directionTarget{
      0U, app::CreativeEditorWorldLayoutRectHandle::None, true};
  const auto* east =
      app::findCreativeEditorWorldLayoutVerticalConnectorHandle(frame,
                                                                 eastTarget);
  const auto* move =
      app::findCreativeEditorWorldLayoutVerticalConnectorHandle(frame,
                                                                 moveTarget);
  const auto* direction =
      app::findCreativeEditorWorldLayoutVerticalConnectorHandle(
          frame, directionTarget);
  app::CreativeEditorWorldLayoutPoint sampled;
  const bool sampledEast =
      east != nullptr &&
      app::sampleCreativeEditorWorldLayoutVerticalConnectorHandlePoint(
          *east, {6.0F, 10.0F, 3.0F}, {0.0F, -1.0F, 0.0F}, grid, sampled);

  const auto pickFrame =
      app::buildCreativeEditorWorldLayoutVerticalConnectorHandleFrame(
          state, grid, 0U);
  const auto* pickEast =
      app::findCreativeEditorWorldLayoutVerticalConnectorHandle(
          pickFrame, eastTarget);
  iggy3d::RenderCameraFrame camera;
  camera.clipFromWorld = iggy3d::identityMat4();
  camera.clipFromWorld.m[0] = 0.1F;
  camera.clipFromWorld.m[5] = 0.1F;
  camera.clipFromWorld.m[10] = 0.1F;
  const iggy3d::RenderContentViewport viewport{0U, 0U, 800U, 600U};
  const cr::CreativeScreenPoint projectedEast =
      pickEast == nullptr
          ? cr::CreativeScreenPoint{}
          : cr::projectCreativeWorldPointToScreen(
                camera.clipFromWorld, pickEast->worldPosition, viewport.width,
                viewport.height);
  const auto pickedEast =
      app::pickCreativeEditorWorldLayoutVerticalConnectorHandleAtPixel(
          pickFrame, camera, viewport, projectedEast.x, projectedEast.y,
          2.0F);

  app::CreativeEditorWorldLayoutState previewState = state;
  const auto previewBegin =
      app::applyCreativeEditorWorldLayoutVerticalConnectorManipulation(
          previewState,
          app::CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
              Begin,
          {50.0, 50.0}, 0.01, grid, eastTarget);
  const auto previewUpdate =
      app::applyCreativeEditorWorldLayoutVerticalConnectorManipulation(
          previewState,
          app::CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
              Update,
          {51.0, 50.0}, 0.01, grid);
  const auto previewFrame =
      app::buildCreativeEditorWorldLayoutVerticalConnectorHandleFrame(
          previewState, grid, 0U);
  const auto* previewEast =
      app::findCreativeEditorWorldLayoutVerticalConnectorHandle(
          previewFrame, eastTarget);
  static_cast<void>(
      app::applyCreativeEditorWorldLayoutVerticalConnectorManipulation(
          previewState,
          app::CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
              Cancel));

  app::CreativeEditorWorldLayoutState rampState = state;
  rampState.source.verticalConnectors[0].kind =
      cr::CreativeWorldLayoutVerticalConnectorKind::Ramp;
  rampState.source.verticalConnectors[0].footprint.maximum.x = 7;
  rampState.revision = 8U;
  rampState.generatedRevision = 8U;
  const auto rampFrame =
      app::buildCreativeEditorWorldLayoutVerticalConnectorHandleFrame(
          rampState, grid, 0U);

  app::CreativeEditorWorldLayoutState staleState = state;
  staleState.generatedRevision = state.revision - 1U;
  const auto staleFrame =
      app::buildCreativeEditorWorldLayoutVerticalConnectorHandleFrame(
          staleState, grid, 0U);

  return expect(elevationX.accepted &&
                    low != elevationX.handles.end() &&
                    high != elevationX.handles.end() &&
                    near(low->position.horizontal, 1.0) &&
                    near(low->position.vertical, 0.0) &&
                    near(high->position.horizontal, 5.0) &&
                    near(high->position.vertical, 4.0) &&
                    perpendicularHandleCount == 0U,
                "elevation exposes connector run ends only on its run axis") &&
         expect(frame.accepted && frame.handleCount == 6U &&
                    frame.connector.accepted &&
                    frame.connector.objectKind ==
                        cr::CreativeObjectKind::Stair &&
                    frame.connector.stair.accepted && east != nullptr &&
                    move != nullptr && direction != nullptr,
                "3D stair handles wrap one accepted canonical recipe") &&
         expect(sampledEast && near(sampled.x, 6.0) &&
                    near(sampled.z, 3.0),
                "3D edge sampling converts the axis ray into plan cells") &&
         expect(projectedEast.valid && pickedEast.hit &&
                    pickedEast.handleIndex < pickFrame.handleCount &&
                    pickFrame.handles[pickedEast.handleIndex].target ==
                        eastTarget,
                "3D reticle picking resolves the exact connector handle") &&
         expect(previewBegin.accepted && previewUpdate.accepted &&
                    previewFrame.accepted && previewEast != nullptr &&
                    near(previewEast->planPosition.x, 6.0) &&
                    state.source.verticalConnectors[0].footprint.maximum.x ==
                        5,
                "explicit handle target drives source-free preview geometry") &&
         expect(rampFrame.accepted &&
                    rampFrame.connector.objectKind ==
                        cr::CreativeObjectKind::Ramp &&
                    rampFrame.connector.ramp.accepted &&
                    rampFrame.handleCount == 6U,
                "ramps reuse the same six-handle manipulation surface") &&
         expect(!staleFrame.accepted && staleFrame.handleCount == 0U,
                "stale generated geometry exposes no connector handles");
}

bool elevationProjectionUsesExactRecipeGeometry() {
  const cr::CreativeGridSettings grid{{10.0, 2.0, -5.0}, 2.0, {32, 16, 32}};
  const cr::CreativeWorldLayout layout = elevationFixture();
  const app::CreativeEditorWorldLayoutElevationProjection projection =
      app::planCreativeEditorWorldLayoutElevation(
          {&layout, grid, 0U,
           app::CreativeEditorWorldLayoutElevationAxis::Z});
  const auto item = [&](app::CreativeEditorWorldLayoutElevationItemKind kind,
                        std::size_t sourceIndex) {
    return std::find_if(
        projection.items.begin(), projection.items.end(),
        [&](const app::CreativeEditorWorldLayoutElevationItem& candidate) {
          return candidate.kind == kind &&
                 candidate.sourceIndex == sourceIndex;
        });
  };
  const auto lowerFloor = item(
      app::CreativeEditorWorldLayoutElevationItemKind::FloorSlab, 0U);
  const auto window = item(
      app::CreativeEditorWorldLayoutElevationItemKind::Window, 1U);
  const auto stair = item(
      app::CreativeEditorWorldLayoutElevationItemKind::Stair, 0U);
  const auto roofBase = item(
      app::CreativeEditorWorldLayoutElevationItemKind::RoofBase, 1U);
  const auto lowerWall = std::find_if(
      projection.items.begin(), projection.items.end(), [](const auto& value) {
        return value.kind ==
                   app::CreativeEditorWorldLayoutElevationItemKind::
                       WallEnvelope &&
               value.sourceKind ==
                   app::CreativeEditorWorldLayoutElevationSourceKind::Room &&
               value.sourceIndex == 0U;
      });
  const auto lowerCeiling = std::find_if(
      projection.items.begin(), projection.items.end(), [](const auto& value) {
        return value.kind ==
                   app::CreativeEditorWorldLayoutElevationItemKind::
                       CeilingSlab &&
               value.sourceKind ==
                   app::CreativeEditorWorldLayoutElevationSourceKind::Room &&
               value.sourceIndex == 0U;
      });
  const auto explicitFloor = std::find_if(
      projection.items.begin(), projection.items.end(), [](const auto& value) {
        return value.kind ==
                   app::CreativeEditorWorldLayoutElevationItemKind::FloorSlab &&
               value.sourceKind ==
                   app::CreativeEditorWorldLayoutElevationSourceKind::Box;
      });
  const auto explicitWall = std::find_if(
      projection.items.begin(), projection.items.end(), [](const auto& value) {
        return value.kind == app::CreativeEditorWorldLayoutElevationItemKind::
                                 WallEnvelope &&
               value.sourceKind ==
                   app::CreativeEditorWorldLayoutElevationSourceKind::Wall;
      });
  const std::size_t roofSlopeCount = static_cast<std::size_t>(std::count_if(
      projection.lines.begin(), projection.lines.end(), [](const auto& line) {
        return line.kind ==
               app::CreativeEditorWorldLayoutElevationLineKind::RoofSlope;
      }));
  const auto ridgeHandle = std::find_if(
      projection.handles.begin(), projection.handles.end(),
      [](const auto& handle) {
        return handle.kind ==
               app::CreativeEditorWorldLayoutElevationHandleKind::RoofRidge;
      });
  const double floorThicknessCells =
      cr::defaultCreativeStructuralLayerThicknessMeters(
          cr::CreativeObjectKind::Floor) /
      grid.cellSizeMeters;
  const cr::CreativeWorldLayoutRoofPlan roofPlan =
      cr::planCreativeWorldLayoutRoof(grid, layout, 1U);
  const double expectedRoofMinimum =
      (roofPlan.geometry.worldBounds.min.y - grid.origin.y) /
      grid.cellSizeMeters;
  const double expectedRoofMaximum =
      (roofPlan.geometry.worldBounds.max.y - grid.origin.y) /
      grid.cellSizeMeters;
  const double expectedRidgeHorizontal =
      (roofPlan.geometry.ridgeStart.z - grid.origin.z) /
      grid.cellSizeMeters;
  const double expectedRidgeVertical =
      (roofPlan.geometry.ridgeStart.y - grid.origin.y) /
      grid.cellSizeMeters;
  const cr::CreativeWorldLayoutLevelDimensions lowerDimensions =
      cr::measureCreativeWorldLayoutLevelDimensions(grid, layout, 0U);
  const cr::CreativeWorldLayoutLevelDimensions upperDimensions =
      cr::measureCreativeWorldLayoutLevelDimensions(grid, layout, 1U);
  const auto lowerSection = std::find_if(
      projection.sectionLevels.begin(), projection.sectionLevels.end(),
      [](const app::CreativeEditorWorldLayoutSectionLevel& level) {
        return level.levelIndex == 0U;
      });
  const auto upperSection = std::find_if(
      projection.sectionLevels.begin(), projection.sectionLevels.end(),
      [](const app::CreativeEditorWorldLayoutSectionLevel& level) {
        return level.levelIndex == 1U;
      });
  const double expectedPartitionTop =
      (std::min(lowerDimensions.wallTopMeters,
                lowerDimensions.interiorPartitionTopMeters) -
       grid.origin.y) /
      grid.cellSizeMeters;
  const double expectedCeilingMinimum =
      (lowerDimensions.upperSurfaceSupportMeters - grid.origin.y) /
      grid.cellSizeMeters;
  const double expectedCeilingMaximum =
      (lowerDimensions.upperSurfaceTopMeters - grid.origin.y) /
      grid.cellSizeMeters;

  return expect(projection.accepted && projection.bounds.valid,
                "elevation projection accepts one owned building") &&
         expect(projection.sectionLevels.size() == 2U &&
                    lowerSection != projection.sectionLevels.end() &&
                    upperSection != projection.sectionLevels.end() &&
                    lowerSection->name == "Ground" &&
                    near(lowerSection->floorDatumCells, 0.0) &&
                    near(lowerSection->floorDatumMeters,
                         lowerDimensions.floorTopMeters) &&
                    near(lowerSection->floorToFloorMeters,
                         lowerDimensions.floorToFloorMeters) &&
                    near(lowerSection->clearHeightMeters,
                         lowerDimensions.clearHeightMeters) &&
                    near(lowerSection->partitionTopCells,
                         expectedPartitionTop) &&
                    lowerSection->partitionConstraint ==
                        app::CreativeEditorWorldLayoutSectionWallConstraint::
                            TopLinked &&
                    upperDimensions.accepted &&
                    upperSection->topmostOccupied &&
                    upperSection->partitionConstraint ==
                        app::CreativeEditorWorldLayoutSectionWallConstraint::
                            FixedHeight &&
                    app::creativeEditorWorldLayoutLevelEditScopeLabel(
                        app::CreativeEditorWorldLayoutLevelEditScope::
                            SelectedAndAbove) == "Selected + above",
                "section rows expose canonical datums, clear heights, and constraints") &&
         expect(lowerFloor != projection.items.end() &&
                    near(lowerFloor->minimumVertical, -floorThicknessCells) &&
                    near(lowerFloor->maximumVertical, 0.0),
                "elevation floor uses descriptor-sized structural thickness") &&
         expect(lowerDimensions.accepted &&
                    lowerWall != projection.items.end() &&
                    near(lowerWall->maximumVertical,
                         expectedPartitionTop) &&
                    lowerCeiling != projection.items.end() &&
                    near(lowerCeiling->minimumVertical,
                         expectedCeilingMinimum) &&
                    near(lowerCeiling->maximumVertical,
                         expectedCeilingMaximum),
                "elevation partitions and ceilings share the compiled storey envelope") &&
         expect(roofPlan.accepted && roofPlan.geometry.accepted &&
                    roofBase != projection.items.end() &&
                    near(roofBase->minimumVertical, expectedRoofMinimum) &&
                    near(roofBase->maximumVertical, expectedRoofMaximum) &&
                    roofSlopeCount == 2U &&
                    ridgeHandle != projection.handles.end() &&
                    near(ridgeHandle->position.horizontal,
                         expectedRidgeHorizontal) &&
                    near(ridgeHandle->position.vertical,
                         expectedRidgeVertical),
                "gable elevation uses the canonical thin-panel bounds and ridge") &&
         expect(window != projection.items.end() &&
                    near(window->minimumHorizontal, 2.5) &&
                    near(window->maximumHorizontal, 3.5) &&
                    near(window->minimumVertical, 1.0) &&
                    near(window->maximumVertical, 2.5),
                "opening elevation projects its host offset and cutout") &&
         expect(stair != projection.items.end() &&
                    near(stair->minimumVertical, 0.0) &&
                    near(stair->maximumVertical, 4.0),
                "elevation reuses exact vertical connector authored bounds") &&
         expect(explicitFloor != projection.items.end() &&
                    explicitWall != projection.items.end(),
                "elevation includes explicit floor and partition symbols");
}

bool scopedSectionDatumEditsPreserveLevelOrderAndOneHistoryStep() {
  cr::CreativeWorldLayout layout = elevationFixture();

  cr::CreativeWorldLayoutLevel attic = layout.levels[1U];
  attic.stableKey = "elevation_attic";
  attic.name = "Attic";
  attic.floorTopLayer = 8.0;
  attic.roofStyle = cr::CreativeStructuralRoofStyle::Flat;
  layout.levels.push_back(attic);
  cr::CreativeWorldLayoutRoom atticRoom = layout.rooms[1U];
  atticRoom.levelIndex = 2U;
  atticRoom.stableKey = "elevation_attic_room";
  atticRoom.name = "Attic Room";
  layout.rooms.push_back(atticRoom);

  cr::CreativeWorldLayoutBuilding annex = layout.buildings[0U];
  annex.stableKey = "elevation_annex";
  annex.name = "Annex";
  layout.buildings.push_back(annex);
  cr::CreativeWorldLayoutLevel annexLevel = layout.levels[0U];
  annexLevel.buildingIndex = 1U;
  annexLevel.stableKey = "elevation_annex_ground";
  annexLevel.name = "Annex Ground";
  layout.levels.push_back(annexLevel);
  cr::CreativeWorldLayoutRoom annexRoom = layout.rooms[0U];
  annexRoom.buildingIndex = 1U;
  annexRoom.levelIndex = 3U;
  annexRoom.stableKey = "elevation_annex_room";
  annexRoom.name = "Annex Room";
  layout.rooms.push_back(annexRoom);

  const auto selected = app::planCreativeEditorWorldLayoutLevelDatumEdit(
      layout,
      {1U, app::CreativeEditorWorldLayoutLevelEditScope::Selected, 6.0});
  const auto above = app::planCreativeEditorWorldLayoutLevelDatumEdit(
      layout,
      {1U, app::CreativeEditorWorldLayoutLevelEditScope::SelectedAndAbove,
       6.0});
  const auto below = app::planCreativeEditorWorldLayoutLevelDatumEdit(
      layout,
      {1U, app::CreativeEditorWorldLayoutLevelEditScope::SelectedAndBelow,
       2.0});
  const auto all = app::planCreativeEditorWorldLayoutLevelDatumEdit(
      layout, {1U, app::CreativeEditorWorldLayoutLevelEditScope::All, 5.0});
  const auto crossing = app::planCreativeEditorWorldLayoutLevelDatumEdit(
      layout,
      {1U, app::CreativeEditorWorldLayoutLevelEditScope::Selected, 8.0});
  cr::CreativeWorldLayout selectedLayout = layout;
  cr::CreativeWorldLayout aboveLayout = layout;
  cr::CreativeWorldLayout belowLayout = layout;
  cr::CreativeWorldLayout allLayout = layout;
  cr::CreativeWorldLayout malformedLayout = layout;
  auto malformed = above;
  ++malformed.affectedLevelCount;
  const bool selectedApplied =
      app::applyCreativeEditorWorldLayoutLevelDatumEditPlan(
          selectedLayout, selected);
  const bool aboveApplied =
      app::applyCreativeEditorWorldLayoutLevelDatumEditPlan(aboveLayout,
                                                            above);
  const bool belowApplied =
      app::applyCreativeEditorWorldLayoutLevelDatumEditPlan(belowLayout,
                                                            below);
  const bool allApplied =
      app::applyCreativeEditorWorldLayoutLevelDatumEditPlan(allLayout, all);
  const bool malformedApplied =
      app::applyCreativeEditorWorldLayoutLevelDatumEditPlan(
          malformedLayout, malformed);

  app::CreativeEditorWorldLayoutState state;
  state.source = layout;
  state.revision = 40U;
  state.sourceHistory.maxDepth = 16U;
  const std::size_t undoBefore = state.sourceHistory.undoEntries.size();
  const auto applied = app::setCreativeEditorWorldLayoutLevelDatum(
      state,
      {1U, app::CreativeEditorWorldLayoutLevelEditScope::SelectedAndAbove,
       6.0});

  return expect(selected.accepted && selected.changed &&
                    selected.affectedLevelCount == 1U &&
                    selectedApplied &&
                    near(selectedLayout.levels[1U].floorTopLayer, 6.0) &&
                    near(selectedLayout.levels[2U].floorTopLayer, 8.0),
                "selected section scope moves exactly one level") &&
         expect(above.accepted && above.changed &&
                    above.affectedLevelCount == 2U && aboveApplied &&
                    near(aboveLayout.levels[1U].floorTopLayer, 6.0) &&
                    near(aboveLayout.levels[2U].floorTopLayer, 10.0),
                "above section scope preserves the upper level stack") &&
         expect(below.accepted && below.changed &&
                    below.affectedLevelCount == 2U && belowApplied &&
                    near(belowLayout.levels[0U].floorTopLayer, -2.0) &&
                    near(belowLayout.levels[1U].floorTopLayer, 2.0),
                "below section scope preserves the lower level stack") &&
         expect(all.accepted && all.changed &&
                    all.affectedLevelCount == 3U && allApplied &&
                    near(allLayout.levels[0U].floorTopLayer, 1.0) &&
                    near(allLayout.levels[1U].floorTopLayer, 5.0) &&
                    near(allLayout.levels[2U].floorTopLayer, 9.0) &&
                    near(allLayout.levels[3U].floorTopLayer, 0.0),
                "all section scope translates one building only") &&
         expect(!crossing.accepted && !crossing.changed &&
                    crossing.reasonCode ==
                        "creative_editor_world_layout_elevation_floor_crosses_level",
                "selected section scope cannot cross an adjacent datum") &&
         expect(!malformedApplied &&
                    near(malformedLayout.levels[0U].floorTopLayer, 0.0) &&
                    near(malformedLayout.levels[1U].floorTopLayer, 4.0) &&
                    near(malformedLayout.levels[2U].floorTopLayer, 8.0),
                "malformed section plans cannot partially mutate datums") &&
         expect(applied.accepted && applied.changed &&
                    state.revision == 41U &&
                    state.sourceHistory.undoEntries.size() ==
                        undoBefore + 1U &&
                    near(state.source.levels[1U].floorTopLayer, 6.0) &&
                    near(state.source.levels[2U].floorTopLayer, 10.0) &&
                    near(state.source.levels[3U].floorTopLayer, 0.0),
                "scoped datum commit is one history step and stays building-local");
}

bool roofAperturesProjectIntoElevationFromExactClosureGeometry() {
  const cr::CreativeGridSettings grid{{10.0, 2.0, -5.0}, 2.0,
                                      {32, 16, 32}};
  cr::CreativeWorldLayout layout = elevationFixture();
  cr::CreativeWorldLayoutRoofAperture skylight;
  skylight.levelIndex = 1U;
  skylight.kind = cr::CreativeStructuralRoofApertureKind::Skylight;
  skylight.stableKey = "elevation_skylight";
  skylight.name = "Elevation Skylight";
  skylight.minimumXCells = 1.0;
  skylight.maximumXCells = 2.0;
  skylight.minimumZCells = 0.5;
  skylight.maximumZCells = 1.5;
  layout.roofApertures.push_back(skylight);
  cr::CreativeWorldLayoutRoofAperture clearance = skylight;
  clearance.kind =
      cr::CreativeStructuralRoofApertureKind::ChimneyClearance;
  clearance.stableKey = "elevation_chimney_clearance";
  clearance.name = "Elevation Chimney Clearance";
  clearance.minimumXCells = 4.0;
  clearance.maximumXCells = 5.0;
  layout.roofApertures.push_back(clearance);

  const cr::CreativeWorldLayoutRoofPlan roof =
      cr::planCreativeWorldLayoutRoof(grid, layout, 1U);
  const app::CreativeEditorWorldLayoutElevationProjection projection =
      app::planCreativeEditorWorldLayoutElevation(
          {&layout, grid, 0U,
           app::CreativeEditorWorldLayoutElevationAxis::X});
  const auto findItem = [&](
                            app::CreativeEditorWorldLayoutElevationItemKind kind,
                            std::size_t index) {
    return std::find_if(
        projection.items.begin(), projection.items.end(),
        [kind, index](const auto& item) {
          return item.kind == kind &&
                 item.sourceKind ==
                     app::CreativeEditorWorldLayoutElevationSourceKind::
                         RoofAperture &&
                 item.sourceIndex == index;
        });
  };
  const auto skylightItem = findItem(
      app::CreativeEditorWorldLayoutElevationItemKind::RoofSkylight, 0U);
  const auto clearanceItem = findItem(
      app::CreativeEditorWorldLayoutElevationItemKind::RoofClearance, 1U);
  const cr::CreativeStructuralRoofApertureInsertPlan* insert = nullptr;
  if (roof.accepted && roof.closure.insertCount == 1U) {
    insert = &roof.closure.inserts[0];
  }
  const double expectedMinimum =
      insert == nullptr
          ? 0.0
          : (insert->bounds.min.y - grid.origin.y) / grid.cellSizeMeters;
  const double expectedMaximum =
      insert == nullptr
          ? 0.0
          : (insert->bounds.max.y - grid.origin.y) / grid.cellSizeMeters;
  return expect(roof.accepted && projection.accepted && insert != nullptr,
                "elevation aperture fixture uses the accepted roof closure") &&
         expect(skylightItem != projection.items.end() &&
                    skylightItem->levelIndex == 1U &&
                    near(skylightItem->minimumHorizontal, 1.0) &&
                    near(skylightItem->maximumHorizontal, 2.0) &&
                    near(skylightItem->minimumVertical, expectedMinimum) &&
                    near(skylightItem->maximumVertical, expectedMaximum),
                "skylight elevation uses the exact generated insert bounds") &&
         expect(clearanceItem != projection.items.end() &&
                    clearanceItem->levelIndex == 1U &&
                    near(clearanceItem->minimumHorizontal, 4.0) &&
                    near(clearanceItem->maximumHorizontal, 5.0) &&
                    clearanceItem->minimumVertical <
                        clearanceItem->maximumVertical,
                "chimney clearance elevation preserves its authored span and roof plane");
}

bool shedAndHipElevationsUseCanonicalProfilesAndPitchHandles() {
  const cr::CreativeGridSettings grid{{0.0, 0.0, 0.0}, 1.0, {32, 16, 32}};
  cr::CreativeWorldLayout layout = elevationFixture();
  cr::CreativeWorldLayoutLevel& roof = layout.levels[1];
  roof.roofPitchDegrees = 30.0;

  roof.roofStyle = cr::CreativeStructuralRoofStyle::Shed;
  roof.roofSlopeDirection =
      cr::CreativeStructuralRoofSlopeDirection::PositiveX;
  const auto shedProfile = app::planCreativeEditorWorldLayoutElevation(
      {&layout, grid, 0U, app::CreativeEditorWorldLayoutElevationAxis::X});
  const auto shedAlongEave = app::planCreativeEditorWorldLayoutElevation(
      {&layout, grid, 0U, app::CreativeEditorWorldLayoutElevationAxis::Z});

  roof.roofStyle = cr::CreativeStructuralRoofStyle::Hip;
  roof.roofRidgeAxis = cr::CreativeStructuralRoofRidgeAxis::X;
  const auto hipAlongRidge = app::planCreativeEditorWorldLayoutElevation(
      {&layout, grid, 0U, app::CreativeEditorWorldLayoutElevationAxis::X});
  const auto hipCrossSection = app::planCreativeEditorWorldLayoutElevation(
      {&layout, grid, 0U, app::CreativeEditorWorldLayoutElevationAxis::Z});

  const auto lineCount = [](const auto& projection,
                            app::CreativeEditorWorldLayoutElevationLineKind kind) {
    return static_cast<std::size_t>(std::count_if(
        projection.lines.begin(), projection.lines.end(),
        [kind](const auto& line) { return line.kind == kind; }));
  };
  const auto ridgeHandle = [](const auto& projection) {
    return std::find_if(
        projection.handles.begin(), projection.handles.end(),
        [](const auto& handle) {
          return handle.kind ==
                 app::CreativeEditorWorldLayoutElevationHandleKind::RoofRidge;
        });
  };
  const auto shedHandle = ridgeHandle(shedProfile);
  const auto hipAlongHandle = ridgeHandle(hipAlongRidge);
  const auto hipCrossHandle = ridgeHandle(hipCrossSection);

  const double support = 8.0;
  const double shedRise = std::tan(std::numbers::pi / 6.0) * 8.0;
  const double hipRise = std::tan(std::numbers::pi / 6.0) * 3.0;
  return expect(shedProfile.accepted &&
                    lineCount(shedProfile,
                              app::CreativeEditorWorldLayoutElevationLineKind::
                                  RoofSlope) == 1U &&
                    shedHandle != shedProfile.handles.end() &&
                    near(shedHandle->position.horizontal, 0.0) &&
                    near(shedHandle->position.vertical, support + shedRise),
                "shed profile exposes one exact slope and draggable high edge") &&
         expect(shedAlongEave.accepted &&
                    lineCount(shedAlongEave,
                              app::CreativeEditorWorldLayoutElevationLineKind::
                                  RoofRidge) == 1U &&
                    ridgeHandle(shedAlongEave) ==
                        shedAlongEave.handles.end(),
                "shed eave elevation shows height without a false pitch handle") &&
         expect(hipAlongRidge.accepted &&
                    lineCount(hipAlongRidge,
                              app::CreativeEditorWorldLayoutElevationLineKind::
                                  RoofSlope) == 2U &&
                    lineCount(hipAlongRidge,
                              app::CreativeEditorWorldLayoutElevationLineKind::
                                  RoofRidge) == 1U &&
                    hipAlongHandle != hipAlongRidge.handles.end() &&
                    near(hipAlongHandle->position.horizontal, 3.0) &&
                    near(hipAlongHandle->position.vertical, support + hipRise),
                "hip ridge elevation exposes two hips and shortened ridge") &&
         expect(hipCrossSection.accepted &&
                    lineCount(hipCrossSection,
                              app::CreativeEditorWorldLayoutElevationLineKind::
                                  RoofSlope) == 2U &&
                    lineCount(hipCrossSection,
                              app::CreativeEditorWorldLayoutElevationLineKind::
                                  RoofRidge) == 0U &&
                    hipCrossHandle != hipCrossSection.handles.end() &&
                    near(hipCrossHandle->position.horizontal, 3.0) &&
                    near(hipCrossHandle->position.vertical, support + hipRise),
                "hip cross-section exposes the same canonical pitch apex");
}

bool roofHandlesAndGesturesShareExactClosureGeometry() {
  const cr::CreativeGridSettings grid{{0.0, 0.0, 0.0}, 1.0, {32, 16, 32}};
  app::CreativeEditorWorldLayoutState state;
  state.source = elevationFixture();
  state.revision = 7U;
  state.generatedRevision = 7U;
  state.tool = app::CreativeEditorWorldLayoutTool::Select;
  state.activeLevelIndex = 1U;
  state.selection = {app::CreativeEditorWorldLayoutSelectionKind::Level, 1U};

  const app::CreativeEditorWorldLayoutRoofHandleFrame frame =
      app::buildCreativeEditorWorldLayoutRoofHandleFrame(state, grid, 1U);
  const auto handle = [&](app::CreativeEditorWorldLayoutRoofHandleKind kind) {
    return std::find_if(
        frame.handles.begin(), frame.handles.begin() + frame.handleCount,
        [kind](const auto& candidate) {
          return candidate.target.handle == kind;
        });
  };
  const auto north =
      handle(app::CreativeEditorWorldLayoutRoofHandleKind::NorthEave);
  const auto east =
      handle(app::CreativeEditorWorldLayoutRoofHandleKind::EastEave);
  const auto south =
      handle(app::CreativeEditorWorldLayoutRoofHandleKind::SouthEave);
  const auto west =
      handle(app::CreativeEditorWorldLayoutRoofHandleKind::WestEave);
  const auto ridge =
      handle(app::CreativeEditorWorldLayoutRoofHandleKind::RidgeHeight);
  const bool exactFootprint =
      frame.accepted && frame.handleCount == 5U &&
      north != frame.handles.begin() + frame.handleCount &&
      east != frame.handles.begin() + frame.handleCount &&
      south != frame.handles.begin() + frame.handleCount &&
      west != frame.handles.begin() + frame.handleCount &&
      ridge != frame.handles.begin() + frame.handleCount &&
      near(north->planPosition.x, 4.0) && near(north->planPosition.z, 0.0) &&
      near(east->planPosition.x, 8.0) && near(east->planPosition.z, 3.0) &&
      near(south->planPosition.x, 4.0) && near(south->planPosition.z, 6.0) &&
      near(west->planPosition.x, 0.0) && near(west->planPosition.z, 3.0) &&
      near(ridge->planPosition.x, 4.0) && near(ridge->planPosition.z, 3.0);

  const auto northHit = app::findCreativeEditorWorldLayoutPlanRoofHandle(
      frame, north->planPosition, 0.2);
  const auto ridgePlanHit = app::findCreativeEditorWorldLayoutPlanRoofHandle(
      frame, ridge->planPosition, 0.2);
  const app::CreativeEditorWorldLayoutRoofTarget eastTarget{
      1U, app::CreativeEditorWorldLayoutRoofHandleKind::EastEave};
  const app::CreativeEditorWorldLayoutRoofTarget ridgeTarget{
      1U, app::CreativeEditorWorldLayoutRoofHandleKind::RidgeHeight};
  const auto expanded = app::planCreativeEditorWorldLayoutRoofEdit(
      state, grid, eastTarget, 0.37);
  const auto contracted = app::planCreativeEditorWorldLayoutRoofEdit(
      state, grid, eastTarget, -100.0);
  const auto raised = app::planCreativeEditorWorldLayoutRoofEdit(
      state, grid, ridgeTarget, 0.37);
  cr::CreativeWorldLayout flat = state.source;
  flat.levels[1].roofStyle = cr::CreativeStructuralRoofStyle::Flat;
  const auto flatRidge = app::planCreativeEditorWorldLayoutRoofEdit(
      flat, grid, ridgeTarget, 1.0);

  const cr::CreativeGridSettings pickGrid{
      {-0.4, -0.8, -0.3}, 0.1, {32, 16, 32}};
  const auto pickFrame = app::buildCreativeEditorWorldLayoutRoofHandleFrame(
      state, pickGrid, 1U);
  const auto pickEast = std::find_if(
      pickFrame.handles.begin(),
      pickFrame.handles.begin() + pickFrame.handleCount,
      [](const auto& candidate) {
        return candidate.target.handle ==
               app::CreativeEditorWorldLayoutRoofHandleKind::EastEave;
      });
  iggy3d::RenderCameraFrame camera;
  camera.clipFromWorld = iggy3d::identityMat4();
  const iggy3d::RenderContentViewport viewport{0U, 0U, 800U, 600U};
  const cr::CreativeScreenPoint projectedEast =
      pickEast == pickFrame.handles.begin() + pickFrame.handleCount
          ? cr::CreativeScreenPoint{}
          : cr::projectCreativeWorldPointToScreen(
                camera.clipFromWorld, pickEast->worldPosition, viewport.width,
                viewport.height);
  const auto pickedEast = app::pickCreativeEditorWorldLayoutRoofHandleAtPixel(
      pickFrame, camera, viewport, projectedEast.x, projectedEast.y, 2.0F);

  app::CreativeEditorWorldLayoutState committedState = state;
  const auto begin = app::applyCreativeEditorWorldLayoutRoofManipulation(
      committedState,
      app::CreativeEditorWorldLayoutRoofManipulationPhase::Begin, eastTarget,
      8.0, grid);
  const auto update = app::applyCreativeEditorWorldLayoutRoofManipulation(
      committedState,
      app::CreativeEditorWorldLayoutRoofManipulationPhase::Update, {}, 8.37,
      grid);
  const bool previewOnly =
      begin.accepted && !begin.changed && update.accepted && update.changed &&
      committedState.roofManipulation.active &&
      near(committedState.roofManipulation.previewSettings.roofOverhangCells,
           0.25) &&
      near(committedState.source.levels[1].roofOverhangCells, 0.0) &&
      committedState.revision == 7U;
  const auto commit = app::applyCreativeEditorWorldLayoutRoofManipulation(
      committedState,
      app::CreativeEditorWorldLayoutRoofManipulationPhase::Commit, {}, 8.37,
      grid);

  app::CreativeEditorWorldLayoutState cancelledState = state;
  static_cast<void>(app::applyCreativeEditorWorldLayoutRoofManipulation(
      cancelledState,
      app::CreativeEditorWorldLayoutRoofManipulationPhase::Begin, ridgeTarget,
      0.0, grid));
  static_cast<void>(app::applyCreativeEditorWorldLayoutRoofManipulation(
      cancelledState,
      app::CreativeEditorWorldLayoutRoofManipulationPhase::Update, {}, 0.5,
      grid));
  const auto cancelled = app::applyCreativeEditorWorldLayoutRoofManipulation(
      cancelledState,
      app::CreativeEditorWorldLayoutRoofManipulationPhase::Cancel, {}, 0.0,
      grid);

  app::CreativeEditorWorldLayoutState staleState = state;
  static_cast<void>(app::applyCreativeEditorWorldLayoutRoofManipulation(
      staleState, app::CreativeEditorWorldLayoutRoofManipulationPhase::Begin,
      eastTarget, 8.0, grid));
  ++staleState.revision;
  const auto stale = app::applyCreativeEditorWorldLayoutRoofManipulation(
      staleState, app::CreativeEditorWorldLayoutRoofManipulationPhase::Update,
      {}, 8.5, grid);

  return expect(exactFootprint,
                "roof handles attach to the exact canonical footprint") &&
         expect(northHit == north->target &&
                    ridgePlanHit.handle ==
                        app::CreativeEditorWorldLayoutRoofHandleKind::None,
                "plan view picks eaves without inventing a pitch gesture") &&
         expect(expanded.accepted && near(expanded.snappedDeltaCells, 0.25) &&
                    near(expanded.settings.roofOverhangCells, 0.25) &&
                    contracted.accepted &&
                    near(contracted.settings.roofOverhangCells, 0.0),
                "all eaves share quarter-cell symmetric overhang math") &&
         expect(raised.accepted && raised.settings.roofPitchDegrees > 45.0 &&
                    !flatRidge.accepted,
                "ridge height derives pitch and flat roofs expose no ridge edit") &&
         expect(projectedEast.valid && pickedEast.hit &&
                    pickedEast.handleIndex < pickFrame.handleCount &&
                    pickFrame.handles[pickedEast.handleIndex].target ==
                        eastTarget,
                "3D reticle picking resolves the exact canonical roof handle") &&
         expect(previewOnly && commit.accepted && commit.changed &&
                    !committedState.roofManipulation.active &&
                    committedState.revision == 8U &&
                    near(committedState.source.levels[1].roofOverhangCells,
                         0.25),
                "roof gesture previews transiently and commits one revision") &&
         expect(cancelled.accepted && cancelled.changed &&
                    !cancelledState.roofManipulation.active &&
                    cancelledState.revision == 7U &&
                    near(cancelledState.source.levels[1].roofPitchDegrees,
                         45.0),
                "roof cancel restores the exact source") &&
         expect(!stale.accepted && !stale.changed &&
                    !staleState.roofManipulation.active &&
                    near(staleState.source.levels[1].roofOverhangCells, 0.0),
                "stale roof gestures fail closed without source mutation");
}

bool directRoofManipulationPreviewsAndCommitsOneDocumentEdit() {
  cr::CreativeAppState live = appState();
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "direct_roof_edit");
  app::CreativeEditorWorldLayoutBuildingBlockoutSettings settings;
  settings.shell.footprint = {{0, 0}, {8, 6}};
  settings.shell.roofStyle = cr::CreativeStructuralRoofStyle::Gable;
  settings.shell.roofRidgeAxis = cr::CreativeStructuralRoofRidgeAxis::X;
  settings.shell.roofPitchDegrees = 35.0;
  settings.facade.includeExteriorWindows = false;
  const auto created =
      app::createCreativeEditorWorldLayoutBuildingBlockout(state, settings);
  const auto generated = app::confirmCreativeEditorWorldLayout(state, live);
  if (!created.accepted || !generated.accepted || state.source.levels.empty()) {
    return expect(false, "direct roof live-edit fixture generates");
  }

  const std::size_t levelIndex = state.source.levels.size() - 1U;
  const app::CreativeEditorWorldLayoutRoofTarget target{
      levelIndex, app::CreativeEditorWorldLayoutRoofHandleKind::EastEave};
  state.tool = app::CreativeEditorWorldLayoutTool::Select;
  state.activeLevelIndex = levelIndex;
  state.selection = {app::CreativeEditorWorldLayoutSelectionKind::Level,
                     levelIndex};
  const double originalOverhang =
      state.source.levels[levelIndex].roofOverhangCells;
  const std::uint64_t sourceRevisionBefore = state.revision;
  const std::uint64_t documentRevisionBefore = live.facade.document().revision();
  const std::uint64_t undoDepthBefore = cr::creativeUndoDepth(live.history);
  const auto begin = app::applyCreativeEditorWorldLayoutRoofManipulationToDocument(
      state, live, app::CreativeEditorWorldLayoutRoofManipulationPhase::Begin,
      target, 8.0);
  const auto update = app::applyCreativeEditorWorldLayoutRoofManipulationToDocument(
      state, live, app::CreativeEditorWorldLayoutRoofManipulationPhase::Update,
      {}, 8.37);
  const bool previewOnly =
      begin.accepted && !begin.changed && update.accepted && update.changed &&
      update.sceneChanged && !update.worldLayoutChanged &&
      app::creativeEditorWorldLayoutPreviewActive(state) &&
      state.source.levels[levelIndex].roofOverhangCells == originalOverhang &&
      state.previewSource.levels[levelIndex].roofOverhangCells ==
          originalOverhang + 0.25 &&
      state.revision == sourceRevisionBefore &&
      live.facade.document().revision() == documentRevisionBefore &&
      cr::creativeUndoDepth(live.history) == undoDepthBefore;
  const auto committed =
      app::applyCreativeEditorWorldLayoutRoofManipulationToDocument(
          state, live,
          app::CreativeEditorWorldLayoutRoofManipulationPhase::Commit, {},
          8.37);
  const bool committedOnce =
      committed.accepted && committed.changed &&
      committed.worldLayoutChanged && committed.sceneChanged &&
      !state.roofManipulation.active &&
      !app::creativeEditorWorldLayoutPreviewActive(state) &&
      state.source.levels[levelIndex].roofOverhangCells ==
          originalOverhang + 0.25 &&
      state.revision == sourceRevisionBefore + 1U &&
      state.generatedRevision == state.revision &&
      live.facade.document().revision() != documentRevisionBefore &&
      cr::creativeUndoDepth(live.history) == undoDepthBefore + 1U;

  return expect(previewOnly,
                "direct 3D roof drag previews without publishing source") &&
         expect(committedOnce,
                "direct 3D roof release commits source scene and history once");
}

bool directVerticalConnectorManipulationCommitsOneDocumentEdit() {
  cr::CreativeAppState live = appState();
  app::CreativeEditorWorldLayoutState state;
  const bool setupAccepted =
      prepareTwoStoreyEditorLayout(state, "direct_connector_edit");
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Stair));
  const auto gestureBegin = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {1, 2});
  const auto gestureCommit = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {5, 4});
  const auto generated = app::confirmCreativeEditorWorldLayout(state, live);
  if (!setupAccepted || !gestureBegin.accepted || !gestureCommit.accepted ||
      !generated.accepted || state.source.verticalConnectors.size() != 1U) {
    return expect(false, "direct connector live-edit fixture generates");
  }

  state.tool = app::CreativeEditorWorldLayoutTool::Select;
  state.selection = {
      app::CreativeEditorWorldLayoutSelectionKind::VerticalConnector, 0U};
  const app::CreativeEditorWorldLayoutVerticalConnectorTarget eastTarget{
      0U, app::CreativeEditorWorldLayoutRectHandle::East, false};
  const std::int32_t originalMaximumX =
      state.source.verticalConnectors[0].footprint.maximum.x;
  const std::uint64_t sourceRevisionBefore = state.revision;
  const std::uint64_t documentRevisionBefore =
      live.facade.document().revision();
  const std::uint64_t undoDepthBefore = cr::creativeUndoDepth(live.history);
  const auto begin =
      app::applyCreativeEditorWorldLayoutVerticalConnectorManipulationToDocument(
          state, live,
          app::CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
              Begin,
          {50.0, 50.0}, eastTarget);
  const auto update =
      app::applyCreativeEditorWorldLayoutVerticalConnectorManipulationToDocument(
          state, live,
          app::CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
              Update,
          {51.0, 50.0});
  const bool previewOnly =
      begin.accepted && begin.changed && update.accepted && update.changed &&
      update.sceneChanged && !update.worldLayoutChanged &&
      app::creativeEditorWorldLayoutPreviewActive(state) &&
      state.source.verticalConnectors[0].footprint.maximum.x ==
          originalMaximumX &&
      state.previewSource.verticalConnectors[0].footprint.maximum.x ==
          originalMaximumX + 1 &&
      state.revision == sourceRevisionBefore &&
      live.facade.document().revision() == documentRevisionBefore &&
      cr::creativeUndoDepth(live.history) == undoDepthBefore;
  const auto committed =
      app::applyCreativeEditorWorldLayoutVerticalConnectorManipulationToDocument(
          state, live,
          app::CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
              Commit,
          {51.0, 50.0});
  const bool committedOnce =
      committed.accepted && committed.changed &&
      committed.worldLayoutChanged && committed.sceneChanged &&
      !state.verticalConnectorManipulation.active &&
      !app::creativeEditorWorldLayoutPreviewActive(state) &&
      state.source.verticalConnectors[0].footprint.maximum.x ==
          originalMaximumX + 1 &&
      state.revision == sourceRevisionBefore + 1U &&
      state.generatedRevision == state.revision &&
      live.facade.document().revision() != documentRevisionBefore &&
      cr::creativeUndoDepth(live.history) == undoDepthBefore + 1U &&
      std::count_if(
          live.facade.document().objects().begin(),
          live.facade.document().objects().end(), [](const auto& object) {
            return object.kind == cr::CreativeObjectKind::Stair;
          }) == 1;

  const cr::CreativeWorldLayoutRect committedFootprint =
      state.source.verticalConnectors[0].footprint;
  const std::uint64_t committedDocumentRevision =
      live.facade.document().revision();
  const std::uint64_t committedUndoDepth =
      cr::creativeUndoDepth(live.history);
  const app::CreativeEditorWorldLayoutVerticalConnectorTarget moveTarget{
      0U, app::CreativeEditorWorldLayoutRectHandle::Move, false};
  const auto cancelBegin =
      app::applyCreativeEditorWorldLayoutVerticalConnectorManipulationToDocument(
          state, live,
          app::CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
              Begin,
          {70.0, 70.0}, moveTarget);
  const auto cancelUpdate =
      app::applyCreativeEditorWorldLayoutVerticalConnectorManipulationToDocument(
          state, live,
          app::CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
              Update,
          {71.0, 70.0});
  const auto cancelled =
      app::applyCreativeEditorWorldLayoutVerticalConnectorManipulationToDocument(
          state, live,
          app::CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
              Cancel);
  const bool cancelledCleanly =
      cancelBegin.accepted && cancelUpdate.accepted &&
      cancelUpdate.sceneChanged && cancelled.accepted && cancelled.changed &&
      cancelled.sceneChanged &&
      !state.verticalConnectorManipulation.active &&
      !app::creativeEditorWorldLayoutPreviewActive(state) &&
      state.source.verticalConnectors[0].footprint.minimum ==
          committedFootprint.minimum &&
      state.source.verticalConnectors[0].footprint.maximum ==
          committedFootprint.maximum &&
      live.facade.document().revision() == committedDocumentRevision &&
      cr::creativeUndoDepth(live.history) == committedUndoDepth;

  return expect(previewOnly,
                "direct 3D connector drag previews without publishing source") &&
         expect(committedOnce,
                "direct 3D connector release commits scene and history once") &&
         expect(cancelledCleanly,
                "cancelled 3D connector drag leaves no source or history edit");
}

bool elevationHitTestingAndEditMathAreTransactionalInputs() {
  const cr::CreativeGridSettings grid{{0.0, 0.0, 0.0}, 1.0, {32, 16, 32}};
  const cr::CreativeWorldLayout layout = elevationFixture();
  const app::CreativeEditorWorldLayoutElevationProjection projection =
      app::planCreativeEditorWorldLayoutElevation(
          {&layout, grid, 0U,
           app::CreativeEditorWorldLayoutElevationAxis::Z});
  const auto handle = [&](app::CreativeEditorWorldLayoutElevationHandleKind kind,
                          std::size_t sourceIndex) {
    return std::find_if(
        projection.handles.begin(), projection.handles.end(),
        [&](const app::CreativeEditorWorldLayoutElevationHandle& candidate) {
          return candidate.kind == kind &&
                 candidate.sourceIndex == sourceIndex;
        });
  };
  const auto groundFloor = handle(
      app::CreativeEditorWorldLayoutElevationHandleKind::LevelFloor, 0U);
  const auto groundWall = handle(
      app::CreativeEditorWorldLayoutElevationHandleKind::WallTop, 0U);
  const auto ridge = handle(
      app::CreativeEditorWorldLayoutElevationHandleKind::RoofRidge, 1U);
  const auto windowBottom = handle(
      app::CreativeEditorWorldLayoutElevationHandleKind::OpeningBottom, 1U);
  const auto windowTop = handle(
      app::CreativeEditorWorldLayoutElevationHandleKind::OpeningTop, 1U);
  const auto explicitFloor = std::find_if(
      projection.handles.begin(), projection.handles.end(),
      [](const auto& candidate) {
        return candidate.kind == app::CreativeEditorWorldLayoutElevationHandleKind::
                                     LevelFloor &&
               candidate.sourceKind ==
                   app::CreativeEditorWorldLayoutElevationSourceKind::Box;
      });
  const auto explicitWall = std::find_if(
      projection.handles.begin(), projection.handles.end(),
      [](const auto& candidate) {
        return candidate.kind == app::CreativeEditorWorldLayoutElevationHandleKind::
                                     WallTop &&
               candidate.sourceKind ==
                   app::CreativeEditorWorldLayoutElevationSourceKind::Wall;
      });
  if (groundFloor == projection.handles.end() ||
      groundWall == projection.handles.end() ||
      ridge == projection.handles.end() ||
      windowBottom == projection.handles.end() ||
      windowTop == projection.handles.end() ||
      explicitFloor == projection.handles.end() ||
      explicitWall == projection.handles.end()) {
    return expect(false, "elevation edit fixture exposes expected handles");
  }

  const auto raisedFloor = app::planCreativeEditorWorldLayoutElevationEdit(
      layout, projection, *groundFloor, 0.5);
  const auto crossedFloor = app::planCreativeEditorWorldLayoutElevationEdit(
      layout, projection, *groundFloor, 4.0);
  const auto shorterWall = app::planCreativeEditorWorldLayoutElevationEdit(
      layout, projection, *groundWall, 2.2);
  const auto roofSlope = std::find_if(
      projection.lines.begin(), projection.lines.end(), [](const auto& line) {
        return line.kind ==
               app::CreativeEditorWorldLayoutElevationLineKind::RoofSlope;
      });
  const auto roof45 = app::planCreativeEditorWorldLayoutElevationEdit(
      layout, projection, *ridge,
      roofSlope->start.vertical +
          std::abs(roofSlope->end.horizontal - roofSlope->start.horizontal));
  const auto sharedRoof45 = app::planCreativeEditorWorldLayoutRoofEdit(
      layout, grid,
      {1U, app::CreativeEditorWorldLayoutRoofHandleKind::RidgeHeight},
      roofSlope->start.vertical +
          std::abs(roofSlope->end.horizontal - roofSlope->start.horizontal) -
          ridge->position.vertical);
  const auto raisedSill = app::planCreativeEditorWorldLayoutElevationEdit(
      layout, projection, *windowBottom, 1.5);
  const auto raisedTop = app::planCreativeEditorWorldLayoutElevationEdit(
      layout, projection, *windowTop, 3.0);
  const auto movedExplicitFloor =
      app::planCreativeEditorWorldLayoutElevationEdit(
          layout, projection, *explicitFloor, 1.25);
  const auto raisedExplicitWall =
      app::planCreativeEditorWorldLayoutElevationEdit(
          layout, projection, *explicitWall, 5.2);
  const auto hit = app::findCreativeEditorWorldLayoutElevationHandle(
      projection, windowTop->position, 0.1);
  const auto* windowItem = app::findCreativeEditorWorldLayoutElevationItem(
      projection, {3.0, 2.0}, 0.01);

  return expect(raisedFloor.accepted &&
                    near(raisedFloor.floorTopLayer, 1.0),
                "floor datum edits use the building lattice instead of authored wall height") &&
         expect(!crossedFloor.accepted &&
                    crossedFloor.reasonCode ==
                        "creative_editor_world_layout_elevation_floor_crosses_level",
                "floor edit cannot cross the next occupied datum") &&
         expect(shorterWall.accepted && shorterWall.wallHeightCells == 2U,
                "wall top snaps to whole-cell height") &&
         expect(roof45.accepted && sharedRoof45.accepted &&
                    near(roof45.roofPitchDegrees,
                         sharedRoof45.settings.roofPitchDegrees) &&
                    near(roof45.roofPitchDegrees, 45.0),
                "elevation ridge uses the shared exact roof edit math") &&
         expect(raisedSill.accepted &&
                    near(raisedSill.openingSillCells, 1.5) &&
                    near(raisedSill.openingHeightCells, 1.0) &&
                    raisedTop.accepted &&
                    near(raisedTop.openingHeightCells, 2.0),
                "window sill preserves top while top handle preserves sill") &&
         expect(hit.kind ==
                    app::CreativeEditorWorldLayoutElevationHandleKind::OpeningTop &&
                    windowItem != nullptr &&
                    windowItem->kind ==
                        app::CreativeEditorWorldLayoutElevationItemKind::Window,
                "elevation hit testing prioritizes exact handles and openings") &&
         expect(movedExplicitFloor.accepted &&
                    near(movedExplicitFloor.floorTopLayer, 1.25) &&
                    raisedExplicitWall.accepted &&
                    raisedExplicitWall.wallHeightCells == 5U,
                "explicit floor and wall handles share snapped elevation math");
}

bool elevationHitStackCyclesDistinctSemanticSources() {
  app::CreativeEditorWorldLayoutElevationProjection projection;
  projection.accepted = true;
  const auto item = [](
                        app::CreativeEditorWorldLayoutElevationItemKind kind,
                        app::CreativeEditorWorldLayoutElevationSourceKind
                            sourceKind,
                        std::size_t sourceIndex) {
    app::CreativeEditorWorldLayoutElevationItem result;
    result.kind = kind;
    result.sourceKind = sourceKind;
    result.sourceIndex = sourceIndex;
    result.minimumHorizontal = 0.0;
    result.maximumHorizontal = 2.0;
    result.minimumVertical = 0.0;
    result.maximumVertical = 2.0;
    return result;
  };
  projection.items = {
      item(app::CreativeEditorWorldLayoutElevationItemKind::Volume,
           app::CreativeEditorWorldLayoutElevationSourceKind::Room, 0U),
      item(app::CreativeEditorWorldLayoutElevationItemKind::WallEnvelope,
           app::CreativeEditorWorldLayoutElevationSourceKind::Wall, 1U),
      item(app::CreativeEditorWorldLayoutElevationItemKind::CeilingSlab,
           app::CreativeEditorWorldLayoutElevationSourceKind::Wall, 1U),
  };

  const auto stack = app::findCreativeEditorWorldLayoutElevationItemStack(
      projection, {1.0, 1.0}, 0.0);
  const auto* first = app::cycleCreativeEditorWorldLayoutElevationItem(
      stack, app::CreativeEditorWorldLayoutElevationSourceKind::None,
      cr::kInvalidCreativeWorldLayoutIndex);
  const auto* second = app::cycleCreativeEditorWorldLayoutElevationItem(
      stack, app::CreativeEditorWorldLayoutElevationSourceKind::Wall, 1U);
  const auto* wrapped = app::cycleCreativeEditorWorldLayoutElevationItem(
      stack, app::CreativeEditorWorldLayoutElevationSourceKind::Room, 0U);
  const auto* singular = app::findCreativeEditorWorldLayoutElevationItem(
      projection, {1.0, 1.0}, 0.0);

  app::CreativeEditorWorldLayoutElevationProjection crowded;
  crowded.accepted = true;
  for (std::size_t index = 0U;
       index < app::kCreativeEditorWorldLayoutElevationHitCapacity + 2U;
       ++index) {
    crowded.items.push_back(
        item(app::CreativeEditorWorldLayoutElevationItemKind::Volume,
             app::CreativeEditorWorldLayoutElevationSourceKind::Box, index));
  }
  const auto bounded = app::findCreativeEditorWorldLayoutElevationItemStack(
      crowded, {1.0, 1.0}, 0.0);
  const auto invalid = app::findCreativeEditorWorldLayoutElevationItemStack(
      crowded,
      {std::numeric_limits<double>::quiet_NaN(), 1.0}, 0.0);

  return expect(stack.count == 2U && stack.testedItemCount == 3U &&
                    stack.totalHitItemCount == 3U && !stack.truncated,
                "elevation hit stack deduplicates one semantic source") &&
         expect(first != nullptr &&
                    first->sourceKind ==
                        app::CreativeEditorWorldLayoutElevationSourceKind::Wall &&
                    first->sourceIndex == 1U && second != nullptr &&
                    second->sourceKind ==
                        app::CreativeEditorWorldLayoutElevationSourceKind::Room &&
                    second->sourceIndex == 0U && wrapped == first &&
                    singular == first,
                "elevation overlap selection cycles in visual order and wraps") &&
         expect(bounded.count ==
                        app::kCreativeEditorWorldLayoutElevationHitCapacity &&
                    bounded.testedItemCount == crowded.items.size() &&
                    bounded.totalHitItemCount == crowded.items.size() &&
                    bounded.truncated && bounded.items.front() != nullptr &&
                    bounded.items.front()->sourceIndex ==
                        crowded.items.size() - 1U &&
                    bounded.items[bounded.count - 1U]->sourceIndex == 2U,
                "elevation overlap stack reports its exact fixed bound") &&
         expect(invalid.count == 0U && invalid.testedItemCount == 0U &&
                    invalid.totalHitItemCount == 0U && !invalid.truncated,
                "invalid elevation hit requests fail closed");
}

bool unsynchronizedLayoutCannotBeSaved() {
  cr::CreativeAppState live = appState();
  cr::CreativeWorldLayout layout;
  layout.stableKey = "unsynchronized_layout";
  const iggy3d::CreativeWorldSaveResult saved = app::saveStandaloneScene(
      live.facade, std::filesystem::temp_directory_path(),
      "iggy3d_unsynchronized_layout_test", &layout, false);
  return expect(!saved.accepted && !saved.saved && saved.path.empty() &&
                    saved.reasonCode ==
                        "creative_world_layout_not_generated",
                "save rejects semantic source newer than generated document");
}

bool unsynchronizedDraftUsesSourceHistoryBeforeDocumentHistory() {
  cr::CreativeAppState live = appState();
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state);
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Room));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {0, 0}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {4, 4}));
  const auto generated = app::confirmCreativeEditorWorldLayout(state, live);
  const auto draft = app::setCreativeEditorWorldLayoutRoomSettings(
      state, 0U, {{{0, 0}, {6, 4}}, 0.0, 3U, 0.25, 1U});
  const std::uint64_t documentCount = live.facade.document().objectCount();
  const std::uint64_t documentUndoDepth =
      cr::creativeUndoDepth(live.history);
  const bool undone = app::undoLastEdit(live, "layout-draft-undo", &state);
  const bool restoredGeneratedSource =
      undone && state.source.rooms.size() == 1U &&
      state.source.rooms[0].footprint.maximum.x == 4 &&
      state.revision == state.generatedRevision &&
      app::creativeEditorWorldLayoutSourceRedoAvailable(state);
  const bool redone = app::redoLastEdit(live, "layout-draft-redo", &state);
  return expect(generated.accepted && generated.changed && draft.accepted &&
                    draft.changed && restoredGeneratedSource,
                "source undo restores the generated semantic baseline") &&
         expect(redone && state.source.rooms.size() == 1U &&
                    state.source.rooms[0].footprint.maximum.x == 6 &&
                    state.revision != state.generatedRevision &&
                    live.facade.document().objectCount() == documentCount &&
                    cr::creativeUndoDepth(live.history) == documentUndoDepth,
                "source redo reapplies the draft without consuming document history");
}

}  // namespace

int main() {
  const bool ok =
      floorAndWallGesturesProduceNormalizedSymbols() &&
      categorizedPaletteOwnsEveryBindableSemanticAction() &&
      terrainAndObjectPaletteToolsCreateCompilableSymbols() &&
      bridgeToolInfersOneStableCrossingAndRejectsAmbiguity() &&
      terrainPathDraftCommitsAsOneSourceEdit() &&
      terrainPathDraftBuildsExactTransient3dPreview() &&
      catalogPlacementSharesOneExactTwoAndThreeDimensionalRecipe() &&
      catalogFloorSnapPlacesScaledSourceBottomOnTheFinishedFloor() &&
      catalogWallSnapUsesCanonicalActiveLevelHosts() &&
      catalogOpeningAssetsCompileAsOwnedStructuralInserts() &&
      openingInsertReplacementPreservesSemanticOwnership() &&
      openingsSnapInsideWallsAndRejectOverlap() &&
      openingPlacementPlansRespectActiveLevelsAndSharedEdges() &&
      deletingWallCascadesItsOpenings() &&
      roomGestureHostsOpeningsAndSupportsResize() &&
      buildingShellCreatesOwnedRoomAndGeneratesAsOneEdit() &&
      rejectedBuildingShellIsTransactionallyEmpty() &&
      buildingShellsKeepIndependentBuildingOwnership() &&
      addRoomTargetsSelectedBuildingAndRejectsAmbiguousOwnership() &&
      roomAdditionCannotInternalizeExistingWindow() &&
      fourRoomBuildingRoundTripsAsOneGeneratedEdit() &&
      invalidRoomShellSettingsFailWithoutMutation() &&
      roomMovePreviewCommitsOnceAndKeepsOpeningHosted() &&
      sharedRoomBoundaryPreviewCommitsAsOneRelationalEdit() &&
      roomSettingsUseTheRelationalTopologyOwner() &&
      roomEdgesAndCornersResizeFromTheirOwnedSides() &&
      invalidRoomManipulationsRejectWithoutMutation() &&
      floorSettingsMoveAndResizeCommitOnce() &&
      planStoreySelectionEditsOnlyItsExactFloorSource() &&
      partitionManipulationPreservesHostedOpeningWorldPositions() &&
      buildingGroupMoveDuplicateAndDeleteAreAtomic() &&
      buildingTransformPreviewsAndCommitsOneRevision() &&
      buildingTemplatesPersistPreviewAndStampOneRevision() &&
      buildingTemplateFoundationPreviewUsesStagedLayoutTerrain() &&
      builtInBuildingTemplateInstallIsDurableAndIdempotent() &&
      buildingTemplateUpdateAndRefreshLifecycleIsExplicit() &&
      openingSettingsApplyOnceAndMatchExactPreview() &&
      concaveRoomOpeningPlacementKeepsTheExactTopologyHost() &&
      openingDragAndWidthHandlesAreQuarterCellTransactional() &&
      openingDragPreservesSharedRoomWallOwnership() &&
      minimumWidthOpeningRetainsMoveAndResizeTargets() &&
      roomDeletionCascadesHostedOpeningsAndCancelIsEmpty() &&
      exactPreviewAndConfirmUseOneHistoryEntry() &&
      buildingLevelLifecycleIsAtomicAndRemapsHostedSymbols() &&
      stairGestureOwnsConnectorLifecycleAcrossLevels() &&
      rampGestureUsesTheSharedVerticalConnectorLifecycle() &&
      verticalConnectorSettingsConvertAndRejectAtomically() &&
      verticalConnectorManipulationIsTransactional() &&
      verticalConnectorDirectionHandleOwnsCardinalRise() &&
      verticalConnectorHandlesShareElevationAnd3dGeometry() &&
      elevationProjectionUsesExactRecipeGeometry() &&
      scopedSectionDatumEditsPreserveLevelOrderAndOneHistoryStep() &&
      roofAperturesProjectIntoElevationFromExactClosureGeometry() &&
      shedAndHipElevationsUseCanonicalProfilesAndPitchHandles() &&
      roofHandlesAndGesturesShareExactClosureGeometry() &&
      directRoofManipulationPreviewsAndCommitsOneDocumentEdit() &&
      directVerticalConnectorManipulationCommitsOneDocumentEdit() &&
      elevationHitTestingAndEditMathAreTransactionalInputs() &&
      elevationHitStackCyclesDistinctSemanticSources() &&
      unsynchronizedLayoutCannotBeSaved() &&
      unsynchronizedDraftUsesSourceHistoryBeforeDocumentHistory();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
