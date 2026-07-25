#include "EditorWorldLayoutElevationPlanner.hpp"

#include <algorithm>
#include <cmath>
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

bool near(double lhs, double rhs) {
  return std::fabs(lhs - rhs) <= 0.00001;
}

cr::CreativeWorldLayout elevationFixture() {
  cr::CreativeWorldLayout layout;
  layout.stableKey = "elevation_planner_fixture";
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
       cr::CreativeWorldLayoutVerticalDirection::PositiveZ,
       "elevation_stair",
       "Main Stair",
       {{1, 1}, {3, 5}}});

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

app::CreativeEditorWorldLayoutState plannerState() {
  app::CreativeEditorWorldLayoutState state;
  state.source = elevationFixture();
  state.revision = 7U;
  state.generatedRevision = 7U;
  state.activeLevelIndex = 0U;
  return state;
}

app::CreativeEditorWorldLayoutElevationProjection plannerProjection(
    const app::CreativeEditorWorldLayoutState& state) {
  const cr::CreativeGridSettings grid{{0.0, 0.0, 0.0}, 1.0, {32, 16, 32}};
  return app::planCreativeEditorWorldLayoutElevation(
      {&state.source, grid, 0U,
       app::CreativeEditorWorldLayoutElevationAxis::Z});
}

const app::CreativeEditorWorldLayoutElevationHandle* findHandle(
    const app::CreativeEditorWorldLayoutElevationProjection& projection,
    app::CreativeEditorWorldLayoutElevationHandleKind kind,
    app::CreativeEditorWorldLayoutElevationSourceKind sourceKind,
    std::size_t sourceIndex) {
  const auto iterator = std::find_if(
      projection.handles.begin(), projection.handles.end(),
      [&](const app::CreativeEditorWorldLayoutElevationHandle& handle) {
        return handle.kind == kind && handle.sourceKind == sourceKind &&
               handle.sourceIndex == sourceIndex;
      });
  return iterator == projection.handles.end() ? nullptr : &*iterator;
}

app::CreativeEditorWorldLayoutElevationInteractionPlan planAt(
    const app::CreativeEditorWorldLayoutState& state,
    const app::CreativeEditorWorldLayoutElevationProjection& projection,
    app::CreativeEditorWorldLayoutElevationPoint point,
    bool pressed = false, bool released = false, bool down = false,
    bool secondary = false, bool focusLost = false, bool cancel = false) {
  return app::planCreativeEditorWorldLayoutElevationInteraction(
      {&state,
       &projection,
       true,
       {true, pressed, released, down, secondary, focusLost, cancel, point,
        0.05}});
}

bool coordinatesRoundTripAcrossCanvasBounds() {
  const app::CreativeEditorWorldLayoutElevationCanvasTransform transform{
      {320.0F, 180.0F}, 24.0F};
  const auto screen = app::planCreativeEditorWorldLayoutElevationScreenPoint(
      transform, {-3.5, 7.25});
  const auto world = app::planCreativeEditorWorldLayoutElevationWorldPoint(
      transform, screen);
  const auto minimum = app::planCreativeEditorWorldLayoutElevationWorldPoint(
      transform, {0.0F, 0.0F});
  const auto maximum = app::planCreativeEditorWorldLayoutElevationWorldPoint(
      transform, {640.0F, 360.0F});
  return expect(near(screen.x, 236.0) && near(screen.y, 6.0),
                "elevation world point maps to expected screen point") &&
         expect(near(world.horizontal, -3.5) &&
                    near(world.vertical, 7.25),
                "elevation coordinate round-trips") &&
         expect(near(minimum.horizontal, -13.333333) &&
                    near(minimum.vertical, 7.5) &&
                    near(maximum.horizontal, 13.333333) &&
                    near(maximum.vertical, -7.5),
                "elevation canvas boundaries remain deterministic");
}

bool visibleHandlePriorityUsesReverseDrawOrder() {
  app::CreativeEditorWorldLayoutState state = plannerState();
  app::CreativeEditorWorldLayoutElevationProjection projection;
  projection.accepted = true;
  projection.handles.push_back(
      {app::CreativeEditorWorldLayoutElevationHandleKind::LevelFloor,
       app::CreativeEditorWorldLayoutElevationSourceKind::Room, 0U, 0U,
       {4.0, 2.0}});
  projection.handles.push_back(
      {app::CreativeEditorWorldLayoutElevationHandleKind::WallTop,
       app::CreativeEditorWorldLayoutElevationSourceKind::Room, 0U, 0U,
       {4.0, 2.0}});
  const auto hit = app::findVisibleCreativeEditorWorldLayoutElevationHandle(
      state, projection, {4.0, 2.0}, 0.05);
  const auto invalid =
      app::findVisibleCreativeEditorWorldLayoutElevationHandle(
          state, projection, {4.0, 2.0}, -1.0);
  state.selection = {app::CreativeEditorWorldLayoutSelectionKind::Box, 0U};
  const bool hidden =
      !app::creativeEditorWorldLayoutElevationHandleVisible(
          state, projection.handles.front());
  return expect(
             hit.kind ==
                 app::CreativeEditorWorldLayoutElevationHandleKind::WallTop,
             "last drawn visible handle wins an overlapping hit") &&
         expect(
             invalid.kind ==
                 app::CreativeEditorWorldLayoutElevationHandleKind::None,
             "invalid handle tolerance fails closed") &&
         expect(hidden, "selection-specific handle visibility is preserved");
}

bool overlappingItemsCycleAndClearDeterministically() {
  app::CreativeEditorWorldLayoutState state = plannerState();
  app::CreativeEditorWorldLayoutElevationProjection projection;
  projection.accepted = true;
  projection.items.push_back(
      {app::CreativeEditorWorldLayoutElevationItemKind::FloorSlab,
       app::CreativeEditorWorldLayoutElevationSourceKind::Room, 0U, 0U,
       0.0, 2.0, 0.0, 2.0});
  projection.items.push_back(
      {app::CreativeEditorWorldLayoutElevationItemKind::Volume,
       app::CreativeEditorWorldLayoutElevationSourceKind::Box, 0U, 0U,
       0.0, 2.0, 0.0, 2.0});
  const auto first = planAt(state, projection, {1.0, 1.0}, true);
  state.selection = first.selection;
  const auto second = planAt(state, projection, {1.0, 1.0}, true);
  const auto clear = planAt(state, projection, {9.0, 9.0}, true);
  return expect(first.selectionChanged &&
                    first.selection.kind ==
                        app::CreativeEditorWorldLayoutSelectionKind::Box &&
                    first.commands.count == 1U,
                "topmost elevation item is selected first") &&
         expect(second.selectionChanged &&
                    second.selection.kind ==
                        app::CreativeEditorWorldLayoutSelectionKind::Room &&
                    second.commands.count == 1U,
                "repeated press cycles to the next semantic source") &&
         expect(clear.commands.count == 1U &&
                    clear.commands.commands[0].id ==
                        app::CreativeDesktopCommandId::
                            WorldLayoutClearSelection,
                "empty elevation press emits one clear command");
}

bool everyHandleFamilyPlansTheEstablishedBeginRoute() {
  app::CreativeEditorWorldLayoutState state = plannerState();
  const auto projection = plannerProjection(state);
  struct Case {
    std::string_view name;
    app::CreativeEditorWorldLayoutElevationHandleKind kind;
    app::CreativeEditorWorldLayoutElevationSourceKind sourceKind;
    std::size_t sourceIndex;
    app::CreativeEditorWorldLayoutSelection selection;
    app::CreativeDesktopCommandId terminalCommand;
    std::size_t commandCount;
  };
  const Case cases[] = {
      {"room floor",
       app::CreativeEditorWorldLayoutElevationHandleKind::LevelFloor,
       app::CreativeEditorWorldLayoutElevationSourceKind::Room, 0U, {},
       app::CreativeDesktopCommandId::WorldLayoutSelectSourceScope, 1U},
      {"room wall",
       app::CreativeEditorWorldLayoutElevationHandleKind::WallTop,
       app::CreativeEditorWorldLayoutElevationSourceKind::Room, 0U, {},
       app::CreativeDesktopCommandId::WorldLayoutSelectSourceScope, 1U},
      {"roof ridge",
       app::CreativeEditorWorldLayoutElevationHandleKind::RoofRidge,
       app::CreativeEditorWorldLayoutElevationSourceKind::Room, 1U,
       {app::CreativeEditorWorldLayoutSelectionKind::Level, 1U},
       app::CreativeDesktopCommandId::WorldLayoutManipulateRoof, 2U},
      {"opening bottom",
       app::CreativeEditorWorldLayoutElevationHandleKind::OpeningBottom,
       app::CreativeEditorWorldLayoutElevationSourceKind::Opening, 1U,
       {app::CreativeEditorWorldLayoutSelectionKind::Opening, 1U},
       app::CreativeDesktopCommandId::WorldLayoutSelectSourceScope, 1U},
      {"opening top",
       app::CreativeEditorWorldLayoutElevationHandleKind::OpeningTop,
       app::CreativeEditorWorldLayoutElevationSourceKind::Opening, 1U,
       {app::CreativeEditorWorldLayoutSelectionKind::Opening, 1U},
       app::CreativeDesktopCommandId::WorldLayoutSelectSourceScope, 1U},
      {"connector low",
       app::CreativeEditorWorldLayoutElevationHandleKind::ConnectorRunLow,
       app::CreativeEditorWorldLayoutElevationSourceKind::VerticalConnector,
       0U,
       {app::CreativeEditorWorldLayoutSelectionKind::VerticalConnector, 0U},
       app::CreativeDesktopCommandId::
           WorldLayoutManipulateVerticalConnector,
       2U},
      {"connector high",
       app::CreativeEditorWorldLayoutElevationHandleKind::ConnectorRunHigh,
       app::CreativeEditorWorldLayoutElevationSourceKind::VerticalConnector,
       0U,
       {app::CreativeEditorWorldLayoutSelectionKind::VerticalConnector, 0U},
       app::CreativeDesktopCommandId::
           WorldLayoutManipulateVerticalConnector,
       2U},
      {"box floor",
       app::CreativeEditorWorldLayoutElevationHandleKind::LevelFloor,
       app::CreativeEditorWorldLayoutElevationSourceKind::Box, 0U,
       {app::CreativeEditorWorldLayoutSelectionKind::Box, 0U},
       app::CreativeDesktopCommandId::WorldLayoutSelectSourceScope, 1U},
      {"explicit wall",
       app::CreativeEditorWorldLayoutElevationHandleKind::WallTop,
       app::CreativeEditorWorldLayoutElevationSourceKind::Wall, 0U,
       {app::CreativeEditorWorldLayoutSelectionKind::Wall, 0U},
       app::CreativeDesktopCommandId::WorldLayoutSelectSourceScope, 1U},
  };
  bool exact = projection.accepted;
  for (const Case& test : cases) {
    const auto* handle =
        findHandle(projection, test.kind, test.sourceKind, test.sourceIndex);
    if (handle == nullptr) {
      std::cerr << "missing elevation handle: " << test.name << '\n';
      exact = false;
      continue;
    }
    app::CreativeEditorWorldLayoutState local = state;
    local.selection = test.selection;
    local.activeLevelIndex = handle->levelIndex;
    const auto plan = planAt(local, projection, handle->position, true);
    const bool caseExact =
        plan.selectionChanged && plan.commands.count == test.commandCount &&
        plan.commands.commands[plan.commands.count - 1U].id ==
            test.terminalCommand;
    if (!caseExact) {
      std::cerr << "wrong elevation begin route: " << test.name
                << " commands=" << plan.commands.count
                << " selected=" << plan.selectionChanged
                << " hovered="
                << static_cast<int>(plan.hoveredHandle.kind)
                << " visible="
                << app::creativeEditorWorldLayoutElevationHandleVisible(
                       local, *handle)
                << " accepted=" << projection.accepted
                << " reason=" << projection.reasonCode << '\n';
    }
    exact = exact && caseExact;
    if (test.commandCount == 1U) {
      exact = exact && plan.replaceElevationManipulation &&
              plan.elevationManipulation.active;
    }
  }
  return expect(exact,
                "all editable elevation handle families preserve begin routes");
}

bool editCommitPayloadsRemainExact() {
  const app::CreativeEditorWorldLayoutState base = plannerState();
  const auto projection = plannerProjection(base);
  struct Case {
    app::CreativeEditorWorldLayoutElevationHandleKind kind;
    app::CreativeEditorWorldLayoutElevationSourceKind sourceKind;
    std::size_t sourceIndex;
    double requestedVertical;
    app::CreativeDesktopCommandId command;
  };
  const Case cases[] = {
      {app::CreativeEditorWorldLayoutElevationHandleKind::LevelFloor,
       app::CreativeEditorWorldLayoutElevationSourceKind::Room, 0U, 1.0,
       app::CreativeDesktopCommandId::WorldLayoutSetLevelDatum},
      {app::CreativeEditorWorldLayoutElevationHandleKind::WallTop,
       app::CreativeEditorWorldLayoutElevationSourceKind::Room, 0U, 3.0,
       app::CreativeDesktopCommandId::WorldLayoutSetLevelSettings},
      {app::CreativeEditorWorldLayoutElevationHandleKind::LevelFloor,
       app::CreativeEditorWorldLayoutElevationSourceKind::Box, 0U, 1.25,
       app::CreativeDesktopCommandId::WorldLayoutSetBoxSettings},
      {app::CreativeEditorWorldLayoutElevationHandleKind::WallTop,
       app::CreativeEditorWorldLayoutElevationSourceKind::Wall, 0U, 5.2,
       app::CreativeDesktopCommandId::WorldLayoutSetWallSettings},
      {app::CreativeEditorWorldLayoutElevationHandleKind::OpeningBottom,
       app::CreativeEditorWorldLayoutElevationSourceKind::Opening, 1U, 1.5,
       app::CreativeDesktopCommandId::WorldLayoutSetOpeningSettings},
      {app::CreativeEditorWorldLayoutElevationHandleKind::OpeningTop,
       app::CreativeEditorWorldLayoutElevationSourceKind::Opening, 1U, 3.0,
       app::CreativeDesktopCommandId::WorldLayoutSetOpeningSettings},
  };
  bool exact = projection.accepted;
  for (const Case& test : cases) {
    const auto* handle =
        findHandle(projection, test.kind, test.sourceKind, test.sourceIndex);
    if (handle == nullptr) {
      exact = false;
      continue;
    }
    app::CreativeEditorWorldLayoutState state = base;
    state.elevationManipulation = {true, state.revision, *handle, {}};
    const auto plan =
        planAt(state, projection,
               {handle->position.horizontal, test.requestedVertical}, false,
               true);
    exact = exact && plan.replaceElevationManipulation &&
            !plan.elevationManipulation.active &&
            plan.commands.count == 1U &&
            plan.commands.commands[0].id == test.command;
  }
  if (!exact) {
    return expect(false, "all elevation edits emit their canonical command");
  }
  const auto* floor = findHandle(
      projection, app::CreativeEditorWorldLayoutElevationHandleKind::LevelFloor,
      app::CreativeEditorWorldLayoutElevationSourceKind::Room, 0U);
  app::CreativeEditorWorldLayoutState state = base;
  state.elevationManipulation = {true, state.revision, *floor, {}};
  const auto plan =
      planAt(state, projection, {floor->position.horizontal, 1.0}, false, true);
  const auto& payload =
      std::get<app::CreativeDesktopWorldLayoutLevelDatumPayload>(
          plan.commands.commands[0].payload);
  return expect(payload.levelIndex == 0U &&
                    payload.stableKey == "elevation_ground" &&
                    near(payload.floorTopLayer, 1.0),
                "level datum commit preserves stable key and snapped value");
}

bool roofConnectorAndDisabledPhasesKeepOrdering() {
  app::CreativeEditorWorldLayoutState state = plannerState();
  const auto projection = plannerProjection(state);
  state.roofManipulation.active = true;
  state.roofManipulation.sourceRevision = state.revision;
  state.roofManipulation.target = {
      1U, app::CreativeEditorWorldLayoutRoofHandleKind::RidgeHeight};
  const auto roof = planAt(state, projection, {2.0, 8.0}, false, false, true);
  state.roofManipulation = {};
  state.verticalConnectorManipulation.active = true;
  state.verticalConnectorManipulation.sourceRevision = state.revision;
  state.verticalConnectorManipulation.target = {
      0U, app::CreativeEditorWorldLayoutRectHandle::West, false};
  const auto connector =
      planAt(state, projection, {2.0, 3.0}, false, true);

  state.roofManipulation.active = true;
  state.roofManipulation.sourceRevision = state.revision;
  state.roofManipulation.target = {
      1U, app::CreativeEditorWorldLayoutRoofHandleKind::RidgeHeight};
  state.elevationManipulation.active = true;
  const auto disabled =
      app::planCreativeEditorWorldLayoutElevationInteraction(
          {&state, &projection, false, {}});
  const auto& roofPayload =
      std::get<app::CreativeDesktopWorldLayoutRoofManipulationPayload>(
          roof.commands.commands[0].payload);
  const auto& connectorPayload = std::get<
      app::CreativeDesktopWorldLayoutVerticalConnectorManipulationPayload>(
      connector.commands.commands[0].payload);
  return expect(
             roof.commands.count == 1U &&
                 roofPayload.phase ==
                     app::CreativeEditorWorldLayoutRoofManipulationPhase::
                         Update,
             "active roof emits one deterministic update") &&
         expect(
             connector.commands.count == 1U &&
                 connectorPayload.phase ==
                     app::
                         CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
                             Commit,
             "active connector emits one deterministic commit") &&
         expect(
             disabled.commands.count == 2U &&
                 disabled.commands.commands[0].id ==
                     app::CreativeDesktopCommandId::WorldLayoutManipulateRoof &&
                 disabled.commands.commands[1].id ==
                     app::CreativeDesktopCommandId::
                         WorldLayoutManipulateVerticalConnector &&
                 disabled.replaceElevationManipulation &&
                 !disabled.elevationManipulation.active,
             "disabled interaction cancels roof then connector and clears edit");
}

bool checkedAdapterPreservesCommandOrder() {
  app::CreativeEditorWorldLayoutElevationCommandPlan plan;
  plan.commands[0] = {
      app::CreativeDesktopCommandId::WorldLayoutClearSelection,
      std::monostate{}};
  plan.commands[1] = {
      app::CreativeDesktopCommandId::WorldLayoutSelectSourceScope,
      app::CreativeDesktopWorldLayoutSourcePayload{
          cr::CreativeWorldLayoutTable::Room, 0U, "room", 0U}};
  plan.count = 2U;
  app::CreativeDesktopCommandFrame frame;
  const bool accepted =
      app::enqueueCreativeEditorWorldLayoutElevationCommandPlan(plan, frame);
  return expect(accepted && !frame.overflowed && frame.count == 2U &&
                    frame.commands[0].id ==
                        app::CreativeDesktopCommandId::
                            WorldLayoutClearSelection &&
                    frame.commands[1].id ==
                        app::CreativeDesktopCommandId::
                            WorldLayoutSelectSourceScope,
                "checked adapter preserves planner order without overflow");
}

bool plannerSourceHasNoUiOrBroadEditorDependency() {
  const std::filesystem::path source =
      "apps/iggy3d_creative/EditorWorldLayoutElevationPlanner.cpp";
  std::ifstream input(source);
  const std::string text{std::istreambuf_iterator<char>{input},
                         std::istreambuf_iterator<char>{}};
  return expect(input.good() || input.eof(), "planner source is readable") &&
         expect(text.find("imgui") == std::string::npos &&
                    text.find("EditorState.hpp") == std::string::npos &&
                    text.find("CreativeEditorState") == std::string::npos,
                "planner is independent of ImGui and broad editor state");
}

}  // namespace

int main() {
  const bool passed =
      coordinatesRoundTripAcrossCanvasBounds() &&
      visibleHandlePriorityUsesReverseDrawOrder() &&
      overlappingItemsCycleAndClearDeterministically() &&
      everyHandleFamilyPlansTheEstablishedBeginRoute() &&
      editCommitPayloadsRemainExact() &&
      roofConnectorAndDisabledPhasesKeepOrdering() &&
      checkedAdapterPreservesCommandOrder() &&
      plannerSourceHasNoUiOrBroadEditorDependency();
  return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
