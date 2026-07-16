#include "EditorWorldLayout.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/history/History.hpp"

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

  return expect(door.accepted && door.changed && doorCenter == 0.5,
                "door clamps fully inside the wall start") &&
         expect(
             !overlap.accepted && !overlap.changed && countAfterOverlap == 1U,
             "overlapping opening is rejected without a source edit") &&
         expect(window.accepted &&
                    state.source.openings[1].centerOffsetCells == 5.25,
                "window clamps fully inside the wall end");
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
                  roomDeletionCascadesHostedOpeningsAndCancelIsEmpty() &&
                  exactPreviewAndConfirmUseOneHistoryEntry();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
