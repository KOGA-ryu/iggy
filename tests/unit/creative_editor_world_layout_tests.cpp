
#include "EditorEdits.hpp"
#include "EditorPersistence.hpp"
#include "EditorWorldLayout.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <span>
#include <string>
#include <vector>

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/history/History.hpp"
#include "app/iggy3d/creative/world/MapTemplate.hpp"
#include "app/iggy3d/creative/world/WorldLayoutCodec.hpp"
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
  const std::span<const app::CreativeEditorWorldLayoutPaletteEntry> entries =
      app::creativeEditorWorldLayoutPaletteEntries();
  std::array<std::size_t, static_cast<std::size_t>(
                              app::CreativeEditorWorldLayoutPaletteCategory::Count)>
      categoryCounts{};
  std::array<bool, static_cast<std::size_t>(
                       app::CreativeEditorWorldLayoutTool::Count)>
      seenTools{};
  bool unique = true;
  for (const app::CreativeEditorWorldLayoutPaletteEntry& entry : entries) {
    const std::size_t category = static_cast<std::size_t>(entry.category);
    if (category >= categoryCounts.size() || entry.label.empty()) {
      unique = false;
      continue;
    }
    ++categoryCounts[category];
    if (entry.activation ==
        app::CreativeEditorWorldLayoutPaletteActivation::Tool) {
      const std::size_t tool = static_cast<std::size_t>(entry.tool);
      if (tool >= seenTools.size() || seenTools[tool]) {
        unique = false;
      } else {
        seenTools[tool] = true;
      }
    }
  }
  const auto estate = std::find_if(
      entries.begin(), entries.end(), [](const auto& entry) {
        return entry.activation ==
                   app::CreativeEditorWorldLayoutPaletteActivation::
                       BuildingTemplate &&
               entry.buildingTemplateId == cr::kBuilderEstateHouseTemplateId;
      });
  return expect(entries.size() == 17U && unique,
                "world layout palette is fixed and duplicate free") &&
         expect(std::all_of(categoryCounts.begin(), categoryCounts.end(),
                            [](std::size_t count) { return count > 0U; }),
                "every palette category owns an action") &&
         expect(estate != entries.end() &&
                    seenTools[static_cast<std::size_t>(
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
                        app::CreativeEditorWorldLayoutTool::Boulder)] &&
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
  const auto roadBegin = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {4.0, 4.0});
  const auto roadCommit = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {16.0, 4.0});

  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Ditch));
  const auto ditchBegin = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {4.0, 20.0});
  const auto ditchCommit = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {16.0, 20.0});

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
  const auto boulder =
      addPoint(app::CreativeEditorWorldLayoutTool::Boulder, {2.0, 12.0});
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
                    roadCommit.accepted && roadCommit.changed &&
                    ditchBegin.accepted && ditchCommit.accepted &&
                    ditchCommit.changed && bridgeBegin.accepted &&
                    bridgeCommit.accepted && bridgeCommit.changed &&
                    boulder.accepted && player.accepted && npc.accepted,
                "palette actions author semantic symbols") &&
         expect(state.source.terrainProfiles.size() == 1U &&
                    state.source.terrainPaths.size() == 2U &&
                    state.source.terrainPathPoints.size() == 4U &&
                    state.source.objects.size() == 4U,
                "palette actions retain compact flat source tables") &&
         expect(state.source.objects[0].kind == cr::CreativeObjectKind::Bridge &&
                    state.source.objects[1].assetId == "boulder_01" &&
                    state.source.objects[2].kind ==
                        cr::CreativeObjectKind::SpawnPoint &&
                    state.source.objects[3].kind ==
                        cr::CreativeObjectKind::NpcSpawn,
                "object palette entries preserve semantic identity") &&
         expect(compiled.receipt.accepted &&
                    compiled.receipt.objectRecipeCount == 1U &&
                    compiled.receipt.objectCount == 4U,
                "palette source compiles through one object recipe") &&
         expect(encoded.accepted && decoded.accepted &&
                    decoded.layout.objects.size() == 4U &&
                    decoded.layout.terrainPaths.size() == 2U,
                "palette source survives durable layout round trip");
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
  const auto door =
      app::applyCreativeEditorWorldLayoutPoint(state, {0.05, 0.1});
  const double doorCenter = state.source.openings[0].centerOffsetCells;
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Window));
  const auto overlap =
      app::applyCreativeEditorWorldLayoutPoint(state, {0.5, 0.1});
  const std::size_t countAfterOverlap = state.source.openings.size();
  const auto window =
      app::applyCreativeEditorWorldLayoutPoint(state, {5.8, 0.1});

  return expect(door.accepted && door.changed && doorCenter == 0.75,
                "door preserves a quarter-cell wall pier at the start") &&
         expect(
             !overlap.accepted && !overlap.changed && countAfterOverlap == 1U,
             "overlapping opening is rejected without a source edit") &&
         expect(window.accepted &&
                    state.source.openings[1].centerOffsetCells == 5.0,
                "window preserves a quarter-cell wall pier at the end");
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

  return expect(!rejected.accepted && !rejected.changed,
                "room shell rejects walls that consume the interior") &&
         expect(state.revision == revisionBefore &&
                    state.source.rooms[0].wallThicknessCells ==
                        cr::kDefaultCreativeWorldLayoutWallThicknessCells,
                "invalid room shell settings do not mutate source truth");
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
         expect(previewOnly,
                "building template movement and rotation remain candidate-only") &&
         expect(cancelRestoredSource,
                "building template cancel restores the exact source") &&
         expect(stampedOnce,
                "building template stamp remaps ownership in one revision") &&
         expect(resetPreservedLibrary && durableReload,
                "building template library survives reset and disk reload");
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
                    state.revision == revisionBeforeForce + 1U &&
                    secondAfterForce.state ==
                        cr::CreativeWorldLayoutBuildingTemplateSyncState::Current,
                "safe refresh skips local work and force refresh is explicit") &&
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
  doorSettings.pose =
      cr::CreativeBuildingOpeningPose::OpenFromStartNegativeNormal;
  windowSettings.widthCells = 1.25;
  windowSettings.sillHeightCells = 1.0;
  windowSettings.heightCells = 1.25;
  windowSettings.includeInsert = false;
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
  badWindow.pose =
      cr::CreativeBuildingOpeningPose::OpenFromEndPositiveNormal;
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
                    state.source.openings[0].pose ==
                        cr::CreativeBuildingOpeningPose::
                            OpenFromStartNegativeNormal,
                "door cutout and tracked insert dimensions stay aligned") &&
         expect(state.source.openings[1].widthCells == 1.25 &&
                    state.source.openings[1].cutoutBottomCells == 1.0 &&
                    state.source.openings[1].cutoutHeightCells == 1.25 &&
                    !state.source.openings[1].includeInsert,
                "window sill, height, width, and insert presence persist") &&
         expect(!badDoorResult.accepted && !badWindowResult.accepted &&
                    state.revision == revisionBeforeInvalid,
                "invalid door sill and window pose fail without mutation") &&
         expect(preview.accepted && doorCount == 1U && windowCount == 0U,
                "exact preview honors open door and omitted window insert");
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
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {2, 5});
  const auto commit = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {4, 1});

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
                        cr::CreativeWorldLayoutVerticalDirection::NegativeZ &&
                    state.source.verticalConnectors[0].stableKey.starts_with(
                        "ramp_") &&
                    state.source.verticalConnectors[0].name == "Ramp 1",
                "ramp tool owns kind direction identity and label") &&
         expect(rampGenerated,
                "ramp preview compiles through the shared layout recipe");
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
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {5, 5});
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
  settings.kind = cr::CreativeWorldLayoutVerticalConnectorKind::Ramp;
  settings.direction =
      cr::CreativeWorldLayoutVerticalDirection::NegativeZ;
  const std::uint64_t revisionBeforeConvert = state.revision;
  const auto converted =
      app::setCreativeEditorWorldLayoutVerticalConnectorSettings(
          state, 0U, settings);
  const bool convertedOnce =
      read && converted.accepted && converted.changed &&
      state.revision == revisionBeforeConvert + 1U &&
      state.source.verticalConnectors[0].kind ==
          cr::CreativeWorldLayoutVerticalConnectorKind::Ramp &&
      state.source.verticalConnectors[0].direction ==
          cr::CreativeWorldLayoutVerticalDirection::NegativeZ &&
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
              convertedSource.direction};
  const auto rejectedKind =
      app::setCreativeEditorWorldLayoutVerticalConnectorSettings(
          state, 0U, settings);
  const bool rejectedAtomically =
      !rejectedSlope.accepted && !rejectedSlope.changed &&
      rejectedSlope.reasonCode ==
          "creative_world_layout_vertical_connector_slope_invalid" &&
      !rejectedKind.accepted && !rejectedKind.changed &&
      state.revision == revisionBeforeRejected &&
      state.source.verticalConnectors[0].footprint.minimum ==
          convertedSource.footprint.minimum &&
      state.source.verticalConnectors[0].footprint.maximum ==
          convertedSource.footprint.maximum &&
      state.source.verticalConnectors[0].kind == convertedSource.kind &&
      state.source.verticalConnectors[0].direction ==
          convertedSource.direction;

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
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Select));
  if (!setupAccepted || !created.accepted) {
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
      state.source.verticalConnectors[0].stableKey == stableKey;

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

bool unsynchronizedDraftCannotBeLostAcrossLayoutHistory() {
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
  const bool undone = app::undoLastEdit(live, "layout-draft-undo", &state);
  return expect(generated.accepted && generated.changed && draft.accepted &&
                    draft.changed && state.revision != state.generatedRevision,
                "layout history draft guard prerequisites") &&
         expect(!undone && state.source.rooms.size() == 1U &&
                    state.source.rooms[0].footprint.maximum.x == 6 &&
                    live.facade.document().objectCount() == documentCount &&
                    cr::creativeUndoDepth(live.history) == 1U,
                "undo cannot discard an ungenerated semantic draft");
}

}  // namespace

int main() {
  const bool ok =
      floorAndWallGesturesProduceNormalizedSymbols() &&
      categorizedPaletteOwnsEveryBindableSemanticAction() &&
      terrainAndObjectPaletteToolsCreateCompilableSymbols() &&
      openingsSnapInsideWallsAndRejectOverlap() &&
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
      roomEdgesAndCornersResizeFromTheirOwnedSides() &&
      invalidRoomManipulationsRejectWithoutMutation() &&
      floorSettingsMoveAndResizeCommitOnce() &&
      partitionManipulationPreservesHostedOpeningWorldPositions() &&
      buildingGroupMoveDuplicateAndDeleteAreAtomic() &&
      buildingTransformPreviewsAndCommitsOneRevision() &&
      buildingTemplatesPersistPreviewAndStampOneRevision() &&
      builtInBuildingTemplateInstallIsDurableAndIdempotent() &&
      buildingTemplateUpdateAndRefreshLifecycleIsExplicit() &&
      openingSettingsApplyOnceAndMatchExactPreview() &&
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
      unsynchronizedLayoutCannotBeSaved() &&
      unsynchronizedDraftCannotBeLostAcrossLayoutHistory();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
