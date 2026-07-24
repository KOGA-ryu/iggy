#include "EditorWorldLayoutCanvasPlanner.hpp"

#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
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

bool coordinatesRoundTripAcrossCanvasBounds() {
  const app::CreativeEditorWorldLayoutCanvasTransform transform{
      {320.0F, 180.0F}, 24.0F};
  const app::CreativeEditorWorldLayoutCanvasScreenPoint screen =
      app::planCreativeEditorWorldLayoutCanvasScreenPoint(
          transform, -3.5, 7.25);
  const app::CreativeEditorWorldLayoutPoint world =
      app::planCreativeEditorWorldLayoutCanvasWorldPoint(transform, screen);
  const app::CreativeEditorWorldLayoutPoint minimum =
      app::planCreativeEditorWorldLayoutCanvasWorldPoint(
          transform, {0.0F, 0.0F});
  const app::CreativeEditorWorldLayoutPoint maximum =
      app::planCreativeEditorWorldLayoutCanvasWorldPoint(
          transform, {640.0F, 360.0F});
  return expect(near(screen.x, 236.0) && near(screen.y, 354.0),
                "world coordinate maps to the expected screen point") &&
         expect(near(world.x, -3.5) && near(world.z, 7.25),
                "screen coordinate round-trips to world") &&
         expect(near(minimum.x, -13.333333) && near(minimum.z, -7.5) &&
                    near(maximum.x, 13.333333) && near(maximum.z, 7.5),
                "canvas edge coordinates remain deterministic");
}

bool hoverPriorityIsTotalAndStable() {
  using Kind = app::CreativeEditorWorldLayoutCanvasHoverTargetKind;
  app::CreativeEditorWorldLayoutCanvasTargetSelection targets;
  targets.opening.handle = app::CreativeEditorWorldLayoutOpeningHandle::Move;
  targets.wall.handle = app::CreativeEditorWorldLayoutWallHandle::Move;
  targets.objectIndex = 4U;
  targets.building = true;
  targets.verticalConnector.directionHandle = true;
  targets.verticalConnector.handle =
      app::CreativeEditorWorldLayoutRectHandle::Move;
  targets.room.handle = app::CreativeEditorWorldLayoutRoomHandle::Move;
  targets.roomCorner.topologyVertexIndex = 8U;
  targets.roomBoundary.topologyEdgeIndex = 9U;
  targets.box.handle = app::CreativeEditorWorldLayoutBoxHandle::Move;
  targets.roof.handle =
      app::CreativeEditorWorldLayoutRoofHandleKind::RidgeHeight;
  targets.roofAperture.handle =
      app::CreativeEditorWorldLayoutRoofApertureHandle::Move;

  bool exact = app::resolveCreativeEditorWorldLayoutCanvasHoverTarget(
                   targets, true) == Kind::Opening;
  targets.opening = {};
  exact = exact &&
          app::resolveCreativeEditorWorldLayoutCanvasHoverTarget(
              targets, true) == Kind::Wall;
  targets.wall = {};
  exact = exact &&
          app::resolveCreativeEditorWorldLayoutCanvasHoverTarget(
              targets, true) == Kind::Object;
  targets.objectIndex = cr::kInvalidCreativeWorldLayoutIndex;
  exact = exact &&
          app::resolveCreativeEditorWorldLayoutCanvasHoverTarget(
              targets, true) == Kind::Building;
  targets.building = false;
  exact = exact &&
          app::resolveCreativeEditorWorldLayoutCanvasHoverTarget(
              targets, true) == Kind::VerticalConnectorDirection;
  targets.verticalConnector.directionHandle = false;
  exact = exact &&
          app::resolveCreativeEditorWorldLayoutCanvasHoverTarget(
              targets, true) == Kind::VerticalConnector;
  targets.verticalConnector = {};
  exact = exact &&
          app::resolveCreativeEditorWorldLayoutCanvasHoverTarget(
              targets, true) == Kind::Room;
  targets.room = {};
  exact = exact &&
          app::resolveCreativeEditorWorldLayoutCanvasHoverTarget(
              targets, true) == Kind::RoomCorner;
  targets.roomCorner = {};
  exact = exact &&
          app::resolveCreativeEditorWorldLayoutCanvasHoverTarget(
              targets, true) == Kind::RoomBoundary;
  targets.roomBoundary = {};
  exact = exact &&
          app::resolveCreativeEditorWorldLayoutCanvasHoverTarget(
              targets, true) == Kind::Box;
  targets.box = {};
  exact = exact &&
          app::resolveCreativeEditorWorldLayoutCanvasHoverTarget(
              targets, true) == Kind::Roof;
  targets.roof = {};
  exact = exact &&
          app::resolveCreativeEditorWorldLayoutCanvasHoverTarget(
              targets, true) == Kind::RoofAperture;
  targets.roofAperture = {};
  exact = exact &&
          app::resolveCreativeEditorWorldLayoutCanvasHoverTarget(
              targets, true) == Kind::Source;
  exact = exact &&
          app::resolveCreativeEditorWorldLayoutCanvasHoverTarget(
              targets, false) == Kind::None;
  return expect(exact, "hover targets retain the complete legacy priority");
}

app::CreativeEditorWorldLayoutPlanHitStack oneHit(
    cr::CreativeWorldLayoutTable table, std::size_t index = 0U) {
  app::CreativeEditorWorldLayoutPlanHitStack stack;
  stack.count = 1U;
  stack.items[0].hit = true;
  stack.items[0].table = table;
  stack.items[0].sourceIndex = index;
  stack.items[0].sourceLevelIndex = 0U;
  return stack;
}

bool pressPriorityAndSelectionCompositionRemainExact() {
  app::CreativeEditorWorldLayoutCanvasTargetSelection targets;
  targets.building = true;
  targets.roof = {
      2U, app::CreativeEditorWorldLayoutRoofHandleKind::EastEave};
  app::CreativeEditorWorldLayoutCanvasPressInput input;
  input.pressed = true;
  input.selectionToolActive = true;
  input.hoveredPoint = {6.0, 4.0};
  input.pointerPoint = input.hoveredPoint;
  input.toleranceCells = 0.25;

  const app::CreativeEditorWorldLayoutPlanHitStack stack =
      oneHit(cr::CreativeWorldLayoutTable::Wall, 3U);
  const app::CreativeEditorWorldLayoutCanvasPressPlan roof =
      app::planCreativeEditorWorldLayoutCanvasPress(
          input, targets, stack,
          {app::CreativeEditorWorldLayoutSelectionKind::Building, 1U});
  input.additive = true;
  const app::CreativeEditorWorldLayoutCanvasPressPlan additive =
      app::planCreativeEditorWorldLayoutCanvasPress(
          input, targets, stack,
          {app::CreativeEditorWorldLayoutSelectionKind::Building, 1U});
  input.additive = false;
  input.toggle = true;
  const app::CreativeEditorWorldLayoutCanvasPressPlan toggle =
      app::planCreativeEditorWorldLayoutCanvasPress(
          input, targets, stack,
          {app::CreativeEditorWorldLayoutSelectionKind::Building, 1U});
  const auto& roofPayload =
      std::get<app::CreativeDesktopWorldLayoutRoofManipulationPayload>(
          roof.commands.commands[0].payload);
  return expect(
             roof.kind ==
                     app::CreativeEditorWorldLayoutCanvasPressKind::BeginRoof &&
                 roof.commands.count == 1U &&
                 roof.commands.commands[0].id ==
                     app::CreativeDesktopCommandId::
                         WorldLayoutManipulateRoof &&
                 roofPayload.phase ==
                     app::CreativeEditorWorldLayoutRoofManipulationPhase::
                         Begin &&
                 near(roofPayload.coordinateCells, 6.0),
             "roof handle wins normal overlapping press priority") &&
         expect(
             additive.kind ==
                     app::CreativeEditorWorldLayoutCanvasPressKind::
                         SelectSource &&
                 additive.composition ==
                     app::CreativeEditorSelectionComposition::Add,
             "additive source selection wins before manipulation") &&
         expect(toggle.composition ==
                    app::CreativeEditorSelectionComposition::Toggle,
                "toggle composition wins over additive semantics");
}

bool targetQueryMapsSemanticHitsAndHonorsAdmission() {
  app::CreativeEditorWorldLayoutState state;
  cr::CreativeGridSettings grid;
  const app::CreativeEditorWorldLayoutPlanHit objectHit =
      oneHit(cr::CreativeWorldLayoutTable::Object, 7U).items[0];
  const app::CreativeEditorWorldLayoutCanvasTargetSelection enabled =
      app::planCreativeEditorWorldLayoutCanvasTargets(
          state, grid, objectHit, {1.0, 2.0}, 0.25, true);
  const app::CreativeEditorWorldLayoutCanvasTargetSelection disabled =
      app::planCreativeEditorWorldLayoutCanvasTargets(
          state, grid, objectHit, {1.0, 2.0}, 0.25, false);
  return expect(
             enabled.objectIndex == 7U &&
                 enabled.hoverKind ==
                     app::CreativeEditorWorldLayoutCanvasHoverTargetKind::
                         Object,
             "target query carries semantic object identity") &&
         expect(disabled.objectIndex ==
                        cr::kInvalidCreativeWorldLayoutIndex &&
                    disabled.hoverKind ==
                        app::CreativeEditorWorldLayoutCanvasHoverTargetKind::
                            None,
                "disabled target query produces no interaction target");
}

template <typename Payload, typename Phase>
bool commandHasPhase(
    const app::CreativeEditorWorldLayoutCanvasCommandPlan& plan,
    app::CreativeDesktopCommandId id, Phase phase) {
  return plan.count == 1U && plan.commands[0].id == id &&
         std::get<Payload>(plan.commands[0].payload).phase == phase;
}

struct ManipulationCase {
  app::CreativeDesktopCommandId id;
  void (*activate)(
      app::CreativeEditorWorldLayoutCanvasManipulationSnapshot&);
  bool (*hasUpdate)(
      const app::CreativeEditorWorldLayoutCanvasCommandPlan&);
  bool (*hasCommit)(
      const app::CreativeEditorWorldLayoutCanvasCommandPlan&);
  bool (*hasCancel)(
      const app::CreativeEditorWorldLayoutCanvasCommandPlan&);
};

#define IGGY3D_CANVAS_CASE(NAME, FIELD, ID, PAYLOAD, PHASE)                 \
  void activate##NAME(                                                     \
      app::CreativeEditorWorldLayoutCanvasManipulationSnapshot& value) {    \
    value.FIELD = true;                                                     \
  }                                                                        \
  bool NAME##Update(                                                        \
      const app::CreativeEditorWorldLayoutCanvasCommandPlan& plan) {        \
    return commandHasPhase<PAYLOAD>(plan, ID, PHASE::Update);               \
  }                                                                        \
  bool NAME##Commit(                                                        \
      const app::CreativeEditorWorldLayoutCanvasCommandPlan& plan) {        \
    return commandHasPhase<PAYLOAD>(plan, ID, PHASE::Commit);               \
  }                                                                        \
  bool NAME##Cancel(                                                        \
      const app::CreativeEditorWorldLayoutCanvasCommandPlan& plan) {        \
    return commandHasPhase<PAYLOAD>(plan, ID, PHASE::Cancel);               \
  }

IGGY3D_CANVAS_CASE(
    Building, buildingActive,
    app::CreativeDesktopCommandId::WorldLayoutManipulateBuilding,
    app::CreativeDesktopWorldLayoutBuildingManipulationPayload,
    app::CreativeEditorWorldLayoutBuildingManipulationPhase)
IGGY3D_CANVAS_CASE(
    Opening, openingActive,
    app::CreativeDesktopCommandId::WorldLayoutManipulateOpening,
    app::CreativeDesktopWorldLayoutOpeningManipulationPayload,
    app::CreativeEditorWorldLayoutOpeningManipulationPhase)
IGGY3D_CANVAS_CASE(
    Wall, wallActive,
    app::CreativeDesktopCommandId::WorldLayoutManipulateWall,
    app::CreativeDesktopWorldLayoutWallManipulationPayload,
    app::CreativeEditorWorldLayoutWallManipulationPhase)
IGGY3D_CANVAS_CASE(
    Room, roomActive,
    app::CreativeDesktopCommandId::WorldLayoutManipulateRoom,
    app::CreativeDesktopWorldLayoutRoomManipulationPayload,
    app::CreativeEditorWorldLayoutRoomManipulationPhase)
IGGY3D_CANVAS_CASE(
    Boundary, roomBoundaryActive,
    app::CreativeDesktopCommandId::WorldLayoutManipulateRoomBoundary,
    app::CreativeDesktopWorldLayoutRoomBoundaryManipulationPayload,
    app::CreativeEditorWorldLayoutRoomBoundaryManipulationPhase)
IGGY3D_CANVAS_CASE(
    Corner, roomCornerActive,
    app::CreativeDesktopCommandId::WorldLayoutManipulateRoomCorner,
    app::CreativeDesktopWorldLayoutRoomCornerManipulationPayload,
    app::CreativeEditorWorldLayoutRoomCornerManipulationPhase)
IGGY3D_CANVAS_CASE(
    Connector, verticalConnectorActive,
    app::CreativeDesktopCommandId::WorldLayoutManipulateVerticalConnector,
    app::CreativeDesktopWorldLayoutVerticalConnectorManipulationPayload,
    app::CreativeEditorWorldLayoutVerticalConnectorManipulationPhase)
IGGY3D_CANVAS_CASE(
    Box, boxActive,
    app::CreativeDesktopCommandId::WorldLayoutManipulateBox,
    app::CreativeDesktopWorldLayoutBoxManipulationPayload,
    app::CreativeEditorWorldLayoutBoxManipulationPhase)
IGGY3D_CANVAS_CASE(
    Aperture, roofApertureActive,
    app::CreativeDesktopCommandId::WorldLayoutManipulateRoofAperture,
    app::CreativeDesktopWorldLayoutRoofApertureManipulationPayload,
    app::CreativeEditorWorldLayoutRoofApertureManipulationPhase)

#undef IGGY3D_CANVAS_CASE

bool activeManipulationFamiliesPlanDeterministically() {
  const std::array cases{
      ManipulationCase{
          app::CreativeDesktopCommandId::WorldLayoutManipulateBuilding,
          activateBuilding, BuildingUpdate, BuildingCommit, BuildingCancel},
      ManipulationCase{
          app::CreativeDesktopCommandId::WorldLayoutManipulateOpening,
          activateOpening, OpeningUpdate, OpeningCommit, OpeningCancel},
      ManipulationCase{
          app::CreativeDesktopCommandId::WorldLayoutManipulateWall,
          activateWall, WallUpdate, WallCommit, WallCancel},
      ManipulationCase{
          app::CreativeDesktopCommandId::WorldLayoutManipulateRoom,
          activateRoom, RoomUpdate, RoomCommit, RoomCancel},
      ManipulationCase{
          app::CreativeDesktopCommandId::WorldLayoutManipulateRoomBoundary,
          activateBoundary, BoundaryUpdate, BoundaryCommit, BoundaryCancel},
      ManipulationCase{
          app::CreativeDesktopCommandId::WorldLayoutManipulateRoomCorner,
          activateCorner, CornerUpdate, CornerCommit, CornerCancel},
      ManipulationCase{
          app::CreativeDesktopCommandId::
              WorldLayoutManipulateVerticalConnector,
          activateConnector, ConnectorUpdate, ConnectorCommit,
          ConnectorCancel},
      ManipulationCase{
          app::CreativeDesktopCommandId::WorldLayoutManipulateBox,
          activateBox, BoxUpdate, BoxCommit, BoxCancel},
      ManipulationCase{
          app::CreativeDesktopCommandId::WorldLayoutManipulateRoofAperture,
          activateAperture, ApertureUpdate, ApertureCommit, ApertureCancel},
  };

  bool exact = true;
  for (const ManipulationCase& row : cases) {
    app::CreativeEditorWorldLayoutCanvasManipulationSnapshot active;
    row.activate(active);
    app::CreativeEditorWorldLayoutCanvasPointerSnapshot pointer;
    pointer.hovered = true;
    pointer.pointerPoint = {3.0, 5.0};
    pointer.hoveredPoint = pointer.pointerPoint;
    pointer.toleranceCells = 0.3;
    pointer.primaryDown = true;
    exact = row.hasUpdate(
                app::planCreativeEditorWorldLayoutCanvasManipulations(
                    active, pointer)) &&
            exact;
    pointer.primaryReleased = true;
    exact = row.hasCommit(
                app::planCreativeEditorWorldLayoutCanvasManipulations(
                    active, pointer)) &&
            exact;
    pointer.focusLost = true;
    exact = row.hasCancel(
                app::planCreativeEditorWorldLayoutCanvasManipulations(
                    active, pointer)) &&
            exact;
  }

  app::CreativeEditorWorldLayoutCanvasManipulationSnapshot roof;
  roof.roofActive = true;
  roof.roofTarget = {
      2U, app::CreativeEditorWorldLayoutRoofHandleKind::WestEave};
  app::CreativeEditorWorldLayoutCanvasPointerSnapshot roofPointer;
  roofPointer.pointerPoint = {7.0, 9.0};
  roofPointer.primaryDown = true;
  const auto roofUpdate =
      app::planCreativeEditorWorldLayoutCanvasManipulations(roof, roofPointer);
  const auto& updatePayload =
      std::get<app::CreativeDesktopWorldLayoutRoofManipulationPayload>(
          roofUpdate.commands[0].payload);
  roofPointer.primaryReleased = true;
  roofPointer.focusLost = true;
  const auto roofCancel =
      app::planCreativeEditorWorldLayoutCanvasManipulations(roof, roofPointer);
  const auto& cancelPayload =
      std::get<app::CreativeDesktopWorldLayoutRoofManipulationPayload>(
          roofCancel.commands[0].payload);
  exact = exact && updatePayload.phase ==
                       app::CreativeEditorWorldLayoutRoofManipulationPhase::
                           Update &&
          near(updatePayload.coordinateCells, -7.0) &&
          cancelPayload.phase ==
              app::CreativeEditorWorldLayoutRoofManipulationPhase::Cancel &&
          near(cancelPayload.coordinateCells, 0.0);
  return expect(
      exact,
      "every manipulation family preserves update, commit, and cancel phases");
}

bool activeManipulationCommandOrderIsStable() {
  app::CreativeEditorWorldLayoutCanvasManipulationSnapshot active;
  active.roofActive = true;
  active.roofTarget = {
      0U, app::CreativeEditorWorldLayoutRoofHandleKind::SouthEave};
  active.buildingActive = true;
  active.openingActive = true;
  active.wallActive = true;
  active.roomActive = true;
  active.roomBoundaryActive = true;
  active.roomCornerActive = true;
  active.verticalConnectorActive = true;
  active.boxActive = true;
  active.roofApertureActive = true;
  active.anchorActive = true;
  active.dragToolActive = true;

  app::CreativeEditorWorldLayoutCanvasPointerSnapshot pointer;
  pointer.pointerPoint = {2.0, 4.0};
  pointer.hoveredPoint = pointer.pointerPoint;
  pointer.primaryDown = true;
  const auto plan =
      app::planCreativeEditorWorldLayoutCanvasManipulations(active, pointer);
  constexpr std::array expected{
      app::CreativeDesktopCommandId::WorldLayoutManipulateRoof,
      app::CreativeDesktopCommandId::WorldLayoutManipulateBuilding,
      app::CreativeDesktopCommandId::WorldLayoutManipulateOpening,
      app::CreativeDesktopCommandId::WorldLayoutManipulateWall,
      app::CreativeDesktopCommandId::WorldLayoutManipulateRoom,
      app::CreativeDesktopCommandId::WorldLayoutManipulateRoomBoundary,
      app::CreativeDesktopCommandId::WorldLayoutManipulateRoomCorner,
      app::CreativeDesktopCommandId::WorldLayoutManipulateVerticalConnector,
      app::CreativeDesktopCommandId::WorldLayoutManipulateBox,
      app::CreativeDesktopCommandId::WorldLayoutManipulateRoofAperture,
      app::CreativeDesktopCommandId::WorldLayoutCanvasGesture,
  };
  bool exact = plan.count == expected.size() && !plan.overflowed;
  for (std::size_t index = 0U;
       exact && index < expected.size(); ++index) {
    exact = plan.commands[index].id == expected[index];
  }
  if (!exact) {
    std::cerr << "planned count=" << plan.count
              << " overflowed=" << plan.overflowed << " ids:";
    for (std::size_t index = 0U; index < plan.count; ++index) {
      std::cerr << ' ' << static_cast<int>(plan.commands[index].id);
    }
    std::cerr << '\n';
  }
  return expect(exact,
                "simultaneous state preserves legacy command order and bound");
}

bool sourceInteractionPlansSelectionBeforeManipulation() {
  app::CreativeEditorWorldLayoutState state;
  const std::array cases{
      std::pair{cr::CreativeWorldLayoutTable::Room,
                app::CreativeDesktopCommandId::WorldLayoutManipulateRoom},
      std::pair{
          cr::CreativeWorldLayoutTable::TopologyEdge,
          app::CreativeDesktopCommandId::WorldLayoutManipulateRoomBoundary},
      std::pair{
          cr::CreativeWorldLayoutTable::VerticalConnector,
          app::CreativeDesktopCommandId::
              WorldLayoutManipulateVerticalConnector},
      std::pair{cr::CreativeWorldLayoutTable::Box,
                app::CreativeDesktopCommandId::WorldLayoutManipulateBox},
      std::pair{cr::CreativeWorldLayoutTable::Wall,
                app::CreativeDesktopCommandId::WorldLayoutManipulateWall},
      std::pair{cr::CreativeWorldLayoutTable::Opening,
                app::CreativeDesktopCommandId::WorldLayoutManipulateOpening},
      std::pair{
          cr::CreativeWorldLayoutTable::RoofAperture,
          app::CreativeDesktopCommandId::WorldLayoutManipulateRoofAperture},
  };
  bool exact = true;
  for (const auto& [table, command] : cases) {
    const auto plan =
        app::planCreativeEditorWorldLayoutCanvasSourceInteraction(
            state, oneHit(table).items[0], {2.0, 3.0}, 0.25);
    exact = plan.accepted && !plan.beginObjectManipulation &&
            plan.commands.count == 2U &&
            plan.commands.commands[0].id ==
                app::CreativeDesktopCommandId::WorldLayoutSelectSourceScope &&
            plan.commands.commands[1].id == command && exact;
  }
  const auto object =
      app::planCreativeEditorWorldLayoutCanvasSourceInteraction(
          state, oneHit(cr::CreativeWorldLayoutTable::Object).items[0],
          {2.0, 3.0}, 0.25);
  return expect(exact && object.accepted &&
                    object.beginObjectManipulation &&
                    object.commands.count == 1U &&
                    object.commands.commands[0].id ==
                        app::CreativeDesktopCommandId::
                            WorldLayoutSelectSourceScope,
                "source interaction preserves selection and begin ordering");
}

bool gesturePhasesAndFrameAdapterPreservePayloads() {
  app::CreativeEditorWorldLayoutCanvasManipulationSnapshot manipulation;
  manipulation.anchorActive = true;
  manipulation.dragToolActive = true;
  app::CreativeEditorWorldLayoutCanvasPointerSnapshot pointer;
  pointer.hovered = true;
  pointer.hoveredPoint = {4.0, 6.0};
  pointer.primaryDown = true;
  const auto update =
      app::planCreativeEditorWorldLayoutCanvasManipulations(
          manipulation, pointer);
  pointer.primaryReleased = true;
  pointer.itemDeactivated = true;
  const auto commit =
      app::planCreativeEditorWorldLayoutCanvasManipulations(
          manipulation, pointer);
  pointer.cancelPressed = true;
  const auto cancel =
      app::planCreativeEditorWorldLayoutCanvasManipulations(
          manipulation, pointer);

  app::CreativeDesktopCommandFrame frame;
  app::enqueueCreativeEditorWorldLayoutCanvasCommandPlan(frame, commit);
  const auto* adapted =
      std::get_if<app::CreativeDesktopWorldLayoutGesturePayload>(
          &frame.commands[0].payload);
  return expect(
             commandHasPhase<
                 app::CreativeDesktopWorldLayoutGesturePayload>(
                 update,
                 app::CreativeDesktopCommandId::WorldLayoutCanvasGesture,
                 app::CreativeEditorWorldLayoutGesturePhase::Update),
             "dragging plans one gesture update") &&
         expect(
             commandHasPhase<
                 app::CreativeDesktopWorldLayoutGesturePayload>(
                 commit,
                 app::CreativeDesktopCommandId::WorldLayoutCanvasGesture,
                 app::CreativeEditorWorldLayoutGesturePhase::Commit),
             "release plans one gesture commit") &&
         expect(
             commandHasPhase<
                 app::CreativeDesktopWorldLayoutGesturePayload>(
                 cancel,
                 app::CreativeDesktopCommandId::WorldLayoutCanvasGesture,
                 app::CreativeEditorWorldLayoutGesturePhase::Cancel),
             "cancel takes precedence over release and update") &&
         expect(frame.count == 1U && adapted != nullptr &&
                    adapted->phase ==
                        app::CreativeEditorWorldLayoutGesturePhase::Commit &&
                    near(adapted->point.x, 4.0) &&
                    near(adapted->point.z, 6.0),
                "adapter emits the planned id and payload through frame enqueue");
}

std::string readFile(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary);
  return {std::istreambuf_iterator<char>{input},
          std::istreambuf_iterator<char>{}};
}

bool plannerBoundaryStaysHeadlessAndNarrow() {
  const std::string header = readFile(
      "apps/iggy3d_creative/EditorWorldLayoutCanvasPlanner.hpp");
  const std::string implementation = readFile(
      "apps/iggy3d_creative/EditorWorldLayoutCanvasPlanner.cpp");
  const std::string source = header + implementation;
  return expect(source.find("imgui") == std::string::npos &&
                    source.find("ImGui") == std::string::npos &&
                    source.find("CreativeEditorState") == std::string::npos,
                "planner has no ImGui, window, or broad editor-state dependency");
}

}  // namespace

int main() {
  const bool coordinates = coordinatesRoundTripAcrossCanvasBounds();
  const bool hover = hoverPriorityIsTotalAndStable();
  const bool press = pressPriorityAndSelectionCompositionRemainExact();
  const bool targets = targetQueryMapsSemanticHitsAndHonorsAdmission();
  const bool manipulation = activeManipulationFamiliesPlanDeterministically();
  const bool order = activeManipulationCommandOrderIsStable();
  const bool source = sourceInteractionPlansSelectionBeforeManipulation();
  const bool adapter = gesturePhasesAndFrameAdapterPreservePayloads();
  const bool boundary = plannerBoundaryStaysHeadlessAndNarrow();
  return coordinates && hover && press && targets && manipulation && order &&
                 source && adapter && boundary
             ? EXIT_SUCCESS
             : EXIT_FAILURE;
}
