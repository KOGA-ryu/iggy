#include "EditorWorldLayout.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/history/History.hpp"
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
  append(layout.rooms);
  append(layout.boxes);
  append(layout.walls);
  append(layout.openings);
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
  const auto updated = app::setCreativeEditorWorldLayoutRoomSettings(
      state, 0U, {{{0, 0}, {8, 5}}, 2, 5U, 0.5, 2U});

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
                    state.source.rooms[0].baseLayer == 2 &&
                    state.source.rooms[0].wallHeightCells == 5U &&
                    state.source.rooms[0].wallThicknessCells == 0.5 &&
                    state.source.rooms[0].floorThicknessCells == 2U,
                "selected room shell settings change as one source edit");
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
  settings.baseLayer = 1;
  settings.heightCells = 2U;
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
                    state.source.boxes[0].baseLayer == 1 &&
                    state.source.boxes[0].heightCells == 2U,
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
      state, 0U, {{{0, 0}, {8, 6}}, 1, 4U, 0.5, 2U});

  const std::uint64_t liveCountBefore = live.facade.document().objectCount();
  const auto preview =
      app::previewCreativeEditorWorldLayout(state, live.facade.document());
  const cr::CreativeDocument& rendered =
      app::creativeEditorWorldLayoutRenderDocument(state,
                                                   live.facade.document());
  const std::uint64_t previewObjectCount = rendered.objectCount();
  std::uint64_t floorCount = 0U;
  std::uint64_t wallCount = 0U;
  bool linkedByLayout = true;
  const cr::CreativeObject* floor = nullptr;
  const cr::CreativeObject* wall = nullptr;
  const std::string layoutTag = cr::creativeWorldLayoutTag("world_layout");
  for (const cr::CreativeObject& object : rendered.objects()) {
    floorCount += object.kind == cr::CreativeObjectKind::Floor ? 1U : 0U;
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

  return expect(settings.accepted && settings.changed,
                "room shell settings are accepted before generation") &&
         expect(preview.accepted &&
                    app::creativeEditorWorldLayoutPreviewActive(state) == false,
                "confirm closes an accepted exact preview") &&
         expect(previewObjectCount == 5U && floorCount == 1U &&
                    wallCount == 4U && linkedByLayout &&
                    previewDidNotPublish,
                "preview renders one linked floor and four walls without publishing") &&
         expect(floorGeometry.valid && wallGeometry.valid &&
                    near(floorGeometry.size.y, 0.1) &&
                    near(wallGeometry.worldBounds.min.y, 2.0) &&
                    near(wallGeometry.worldBounds.max.y, 6.0) &&
                    near(std::min(wallGeometry.size.x, wallGeometry.size.z),
                         0.5),
                "preview geometry matches floor, elevation, height, and thickness settings") &&
         expect(applied.accepted && applied.changed &&
                    live.facade.document().objectCount() == 5U,
                "confirm publishes generated output") &&
         expect(cr::creativeUndoDepth(live.history) == 1U,
                "one layout confirm records exactly one undo entry");
}

}  // namespace

int main() {
  const bool ok = floorAndWallGesturesProduceNormalizedSymbols() &&
                  openingsSnapInsideWallsAndRejectOverlap() &&
                  deletingWallCascadesItsOpenings() &&
                  roomGestureHostsOpeningsAndSupportsResize() &&
                  invalidRoomShellSettingsFailWithoutMutation() &&
                  roomMovePreviewCommitsOnceAndKeepsOpeningHosted() &&
                  roomEdgesAndCornersResizeFromTheirOwnedSides() &&
                  invalidRoomManipulationsRejectWithoutMutation() &&
                  floorSettingsMoveAndResizeCommitOnce() &&
                  partitionManipulationPreservesHostedOpeningWorldPositions() &&
                  buildingGroupMoveDuplicateAndDeleteAreAtomic() &&
                  buildingTransformPreviewsAndCommitsOneRevision() &&
                  buildingTemplatesPersistPreviewAndStampOneRevision() &&
                  openingSettingsApplyOnceAndMatchExactPreview() &&
                  openingDragAndWidthHandlesAreQuarterCellTransactional() &&
                  openingDragPreservesSharedRoomWallOwnership() &&
                  minimumWidthOpeningRetainsMoveAndResizeTargets() &&
                  roomDeletionCascadesHostedOpeningsAndCancelIsEmpty() &&
                  exactPreviewAndConfirmUseOneHistoryEntry();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
