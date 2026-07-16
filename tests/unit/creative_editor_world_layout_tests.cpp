#include "EditorWorldLayout.hpp"

#include <cstdlib>
#include <iostream>

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

bool exactPreviewAndConfirmUseOneHistoryEntry() {
  cr::CreativeAppState live = appState();
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state);
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Floor));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(state, {0, 0}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(state, {6, 5}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Wall));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(state, {0, 0}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(state, {6, 0}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Door));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(state, {3, 0}));

  const std::uint64_t liveCountBefore = live.facade.document().objectCount();
  const auto preview =
      app::previewCreativeEditorWorldLayout(state, live.facade.document());
  const cr::CreativeDocument& rendered =
      app::creativeEditorWorldLayoutRenderDocument(state,
                                                   live.facade.document());
  const std::uint64_t previewObjectCount = rendered.objectCount();
  const bool previewDidNotPublish =
      live.facade.document().objectCount() == liveCountBefore;
  const auto applied = app::confirmCreativeEditorWorldLayout(state, live);

  return expect(preview.accepted &&
                    app::creativeEditorWorldLayoutPreviewActive(state) == false,
                "confirm closes an accepted exact preview") &&
         expect(previewObjectCount > liveCountBefore && previewDidNotPublish,
                "preview renders generated output without publishing") &&
         expect(applied.accepted && applied.changed &&
                    live.facade.document().objectCount() > liveCountBefore,
                "confirm publishes generated output") &&
         expect(cr::creativeUndoDepth(live.history) == 1U,
                "one layout confirm records exactly one undo entry");
}

}  // namespace

int main() {
  const bool ok = floorAndWallGesturesProduceNormalizedSymbols() &&
                  openingsSnapInsideWallsAndRejectOverlap() &&
                  deletingWallCascadesItsOpenings() &&
                  exactPreviewAndConfirmUseOneHistoryEntry();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
