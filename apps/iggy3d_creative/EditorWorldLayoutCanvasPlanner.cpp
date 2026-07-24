#include "EditorWorldLayoutCanvasPlanner.hpp"

#include "EditorWorldLayoutBuildings.hpp"
#include "EditorWorldLayoutOpenings.hpp"
#include "EditorWorldLayoutPlan.hpp"
#include "EditorWorldLayoutRoofs.hpp"
#include "EditorWorldLayoutSources.hpp"

#include <cassert>
#include <cmath>
#include <utility>

namespace iggy3d_creative_app {
namespace {

template <typename Payload>
void appendCommand(CreativeEditorWorldLayoutCanvasCommandPlan& plan,
                   CreativeDesktopCommandId id, Payload payload) {
  if (plan.count >= plan.commands.size()) {
    plan.overflowed = true;
    return;
  }
  plan.commands[plan.count++] = {
      id, CreativeEditorWorldLayoutCanvasCommandPayload{std::move(payload)}};
}

bool selectedBuildingContains(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) noexcept {
  if (state.selection.kind !=
      CreativeEditorWorldLayoutSelectionKind::Building) {
    return false;
  }
  CreativeEditorWorldLayoutBuildingBounds bounds;
  if (!readCreativeEditorWorldLayoutBuildingBounds(
          state, state.selection.index, bounds) ||
      !std::isfinite(point.x) || !std::isfinite(point.z)) {
    return false;
  }
  return point.x >= bounds.minimum.x - toleranceCells &&
         point.x <= bounds.maximum.x + toleranceCells &&
         point.z >= bounds.minimum.z - toleranceCells &&
         point.z <= bounds.maximum.z + toleranceCells;
}

double roofHandleCoordinate(
    CreativeEditorWorldLayoutRoofHandleKind handle,
    CreativeEditorWorldLayoutPoint point) noexcept {
  switch (handle) {
    case CreativeEditorWorldLayoutRoofHandleKind::NorthEave:
      return -point.z;
    case CreativeEditorWorldLayoutRoofHandleKind::EastEave:
      return point.x;
    case CreativeEditorWorldLayoutRoofHandleKind::SouthEave:
      return point.z;
    case CreativeEditorWorldLayoutRoofHandleKind::WestEave:
      return -point.x;
    case CreativeEditorWorldLayoutRoofHandleKind::RidgeHeight:
      return point.z;
    case CreativeEditorWorldLayoutRoofHandleKind::None:
    case CreativeEditorWorldLayoutRoofHandleKind::Count:
      break;
  }
  return 0.0;
}

CreativeDesktopWorldLayoutSourcePayload sourcePayload(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutPlanHit& hit) {
  return {
      hit.table,
      hit.sourceIndex,
      std::string(creativeEditorWorldLayoutSourceStableKey(
          state, hit.table, hit.sourceIndex)),
      hit.sourceLevelIndex,
  };
}

template <typename Phase>
Phase pointerPhase(bool cancel, bool released, bool down) noexcept {
  if (cancel) {
    return Phase::Cancel;
  }
  if (released) {
    return Phase::Commit;
  }
  if (down) {
    return Phase::Update;
  }
  return Phase::Count;
}

template <typename Phase, typename Payload>
void appendPointerCommand(
    CreativeEditorWorldLayoutCanvasCommandPlan& plan, bool active,
    bool cancel, bool released, bool down, CreativeDesktopCommandId id,
    Payload payload) {
  if (!active) {
    return;
  }
  const Phase phase = pointerPhase<Phase>(cancel, released, down);
  if (phase == Phase::Count) {
    return;
  }
  payload.phase = phase;
  appendCommand(plan, id, std::move(payload));
}

}  // namespace

CreativeEditorWorldLayoutCanvasScreenPoint
planCreativeEditorWorldLayoutCanvasScreenPoint(
    const CreativeEditorWorldLayoutCanvasTransform& transform,
    double x, double z) noexcept {
  return {
      transform.origin.x + static_cast<float>(x) * transform.pixelsPerCell,
      transform.origin.y + static_cast<float>(z) * transform.pixelsPerCell,
  };
}

CreativeEditorWorldLayoutPoint planCreativeEditorWorldLayoutCanvasWorldPoint(
    const CreativeEditorWorldLayoutCanvasTransform& transform,
    CreativeEditorWorldLayoutCanvasScreenPoint screen) noexcept {
  return {
      (screen.x - transform.origin.x) / transform.pixelsPerCell,
      (screen.y - transform.origin.y) / transform.pixelsPerCell,
  };
}

CreativeEditorWorldLayoutPlanHitStack
planCreativeEditorWorldLayoutCanvasHitStack(
    const CreativeEditorWorldLayoutPlanViewCache& planView,
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeWorldLayout& displaySource,
    CreativeEditorWorldLayoutPoint point, double toleranceCells,
    bool enabled) {
  if (!enabled) {
    return {};
  }
  return hitCreativeEditorWorldLayoutPlanStack(
      planView, state, displaySource, {point.x, point.z}, toleranceCells);
}

CreativeEditorWorldLayoutCanvasTargetSelection
planCreativeEditorWorldLayoutCanvasTargets(
    const CreativeEditorWorldLayoutState& state,
    cr::CreativeGridSettings grid,
    const CreativeEditorWorldLayoutPlanHit& hit,
    CreativeEditorWorldLayoutPoint point, double toleranceCells,
    bool enabled) {
  CreativeEditorWorldLayoutCanvasTargetSelection targets;
  if (!enabled) {
    return targets;
  }

  switch (hit.table) {
    case cr::CreativeWorldLayoutTable::Opening:
      targets.opening =
          findCreativeEditorWorldLayoutOpeningTarget(state, point,
                                                     toleranceCells);
      break;
    case cr::CreativeWorldLayoutTable::Wall:
      targets.wall =
          findCreativeEditorWorldLayoutWallTarget(state, point, toleranceCells);
      break;
    case cr::CreativeWorldLayoutTable::VerticalConnector:
      targets.verticalConnector =
          findCreativeEditorWorldLayoutVerticalConnectorTarget(
              state, point, toleranceCells);
      break;
    case cr::CreativeWorldLayoutTable::Room:
      targets.room =
          findCreativeEditorWorldLayoutRoomTarget(state, point, toleranceCells);
      targets.roomBoundary =
          findCreativeEditorWorldLayoutRoomBoundaryTarget(
              state, point, toleranceCells);
      targets.roomCorner = findCreativeEditorWorldLayoutRoomCornerTarget(
          state, hit.sourceIndex, point, toleranceCells);
      break;
    case cr::CreativeWorldLayoutTable::Box:
      targets.box =
          findCreativeEditorWorldLayoutBoxTarget(state, point, toleranceCells);
      break;
    case cr::CreativeWorldLayoutTable::RoofAperture:
      targets.roofAperture =
          findCreativeEditorWorldLayoutRoofApertureTarget(
              state, point, toleranceCells);
      break;
    case cr::CreativeWorldLayoutTable::Object:
      targets.objectIndex = hit.sourceIndex;
      break;
    case cr::CreativeWorldLayoutTable::None:
    case cr::CreativeWorldLayoutTable::Building:
    case cr::CreativeWorldLayoutTable::Level:
    case cr::CreativeWorldLayoutTable::TerrainProfile:
    case cr::CreativeWorldLayoutTable::TerrainPath:
    case cr::CreativeWorldLayoutTable::TerrainPathPoint:
    case cr::CreativeWorldLayoutTable::TopologyEdge:
      break;
  }

  const std::size_t roofLevelIndex =
      state.roofManipulation.active
          ? state.roofManipulation.target.levelIndex
          : state.selection.kind ==
                    CreativeEditorWorldLayoutSelectionKind::Level
                ? state.selection.index
                : cr::kInvalidCreativeWorldLayoutIndex;
  targets.roof = findCreativeEditorWorldLayoutPlanRoofHandle(
      buildCreativeEditorWorldLayoutRoofHandleFrame(state, grid,
                                                    roofLevelIndex),
      point, toleranceCells);
  targets.building = selectedBuildingContains(state, point, toleranceCells);
  targets.hoverKind =
      resolveCreativeEditorWorldLayoutCanvasHoverTarget(targets, hit.hit);
  return targets;
}

CreativeEditorWorldLayoutCanvasHoverTargetKind
resolveCreativeEditorWorldLayoutCanvasHoverTarget(
    const CreativeEditorWorldLayoutCanvasTargetSelection& targets,
    bool sourceHit) noexcept {
  if (targets.opening.handle != CreativeEditorWorldLayoutOpeningHandle::None) {
    return CreativeEditorWorldLayoutCanvasHoverTargetKind::Opening;
  }
  if (targets.wall.handle != CreativeEditorWorldLayoutWallHandle::None) {
    return CreativeEditorWorldLayoutCanvasHoverTargetKind::Wall;
  }
  if (targets.objectIndex != cr::kInvalidCreativeWorldLayoutIndex) {
    return CreativeEditorWorldLayoutCanvasHoverTargetKind::Object;
  }
  if (targets.building) {
    return CreativeEditorWorldLayoutCanvasHoverTargetKind::Building;
  }
  if (targets.verticalConnector.directionHandle) {
    return CreativeEditorWorldLayoutCanvasHoverTargetKind::
        VerticalConnectorDirection;
  }
  if (targets.verticalConnector.handle !=
      CreativeEditorWorldLayoutRectHandle::None) {
    return CreativeEditorWorldLayoutCanvasHoverTargetKind::VerticalConnector;
  }
  if (targets.room.handle != CreativeEditorWorldLayoutRoomHandle::None) {
    return CreativeEditorWorldLayoutCanvasHoverTargetKind::Room;
  }
  if (targets.roomCorner.topologyVertexIndex !=
      cr::kInvalidCreativeWorldLayoutIndex) {
    return CreativeEditorWorldLayoutCanvasHoverTargetKind::RoomCorner;
  }
  if (targets.roomBoundary.topologyEdgeIndex !=
      cr::kInvalidCreativeWorldLayoutIndex) {
    return CreativeEditorWorldLayoutCanvasHoverTargetKind::RoomBoundary;
  }
  if (targets.box.handle != CreativeEditorWorldLayoutBoxHandle::None) {
    return CreativeEditorWorldLayoutCanvasHoverTargetKind::Box;
  }
  if (targets.roof.handle !=
      CreativeEditorWorldLayoutRoofHandleKind::None) {
    return CreativeEditorWorldLayoutCanvasHoverTargetKind::Roof;
  }
  if (targets.roofAperture.handle !=
      CreativeEditorWorldLayoutRoofApertureHandle::None) {
    return CreativeEditorWorldLayoutCanvasHoverTargetKind::RoofAperture;
  }
  return sourceHit ? CreativeEditorWorldLayoutCanvasHoverTargetKind::Source
                   : CreativeEditorWorldLayoutCanvasHoverTargetKind::None;
}

CreativeEditorSelectionComposition
creativeEditorWorldLayoutCanvasSelectionComposition(
    bool additive, bool toggle) noexcept {
  if (toggle) {
    return CreativeEditorSelectionComposition::Toggle;
  }
  return additive ? CreativeEditorSelectionComposition::Add
                  : CreativeEditorSelectionComposition::Replace;
}

CreativeEditorWorldLayoutCanvasPressPlan
planCreativeEditorWorldLayoutCanvasPress(
    const CreativeEditorWorldLayoutCanvasPressInput& input,
    const CreativeEditorWorldLayoutCanvasTargetSelection& targets,
    const CreativeEditorWorldLayoutPlanHitStack& hitStack,
    CreativeEditorWorldLayoutSelection selection) {
  CreativeEditorWorldLayoutCanvasPressPlan plan;
  if (!input.pressed) {
    return plan;
  }

  const CreativeEditorWorldLayoutPlanHit hit =
      hitStack.count > 0U ? hitStack.items.front()
                          : CreativeEditorWorldLayoutPlanHit{};
  plan.composition = creativeEditorWorldLayoutCanvasSelectionComposition(
      input.additive, input.toggle);

  if (input.selectionToolActive) {
    if ((input.additive || input.toggle) && hit.hit) {
      plan.kind = CreativeEditorWorldLayoutCanvasPressKind::SelectSource;
      plan.source = {hit.table, hit.sourceIndex};
      return plan;
    }
    if ((input.additive || input.toggle) && targets.building &&
        selection.kind == CreativeEditorWorldLayoutSelectionKind::Building) {
      plan.kind = CreativeEditorWorldLayoutCanvasPressKind::SelectBuilding;
      plan.source = {cr::CreativeWorldLayoutTable::Building, selection.index};
      return plan;
    }
    if (targets.roof.handle !=
        CreativeEditorWorldLayoutRoofHandleKind::None) {
      plan.kind = CreativeEditorWorldLayoutCanvasPressKind::BeginRoof;
      appendCommand(
          plan.commands,
          CreativeDesktopCommandId::WorldLayoutManipulateRoof,
          CreativeDesktopWorldLayoutRoofManipulationPayload{
              CreativeEditorWorldLayoutRoofManipulationPhase::Begin,
              targets.roof,
              roofHandleCoordinate(targets.roof.handle, input.hoveredPoint)});
      return plan;
    }
    if (targets.building) {
      plan.kind = CreativeEditorWorldLayoutCanvasPressKind::BeginBuilding;
      appendCommand(
          plan.commands,
          CreativeDesktopCommandId::WorldLayoutManipulateBuilding,
          CreativeDesktopWorldLayoutBuildingManipulationPayload{
              CreativeEditorWorldLayoutBuildingManipulationPhase::Begin,
              input.hoveredPoint, input.toleranceCells});
      return plan;
    }
    if (hit.hit) {
      plan.kind = CreativeEditorWorldLayoutCanvasPressKind::InteractSource;
      plan.hit = cycleCreativeEditorWorldLayoutPlanHit(hitStack, selection);
      return plan;
    }
    plan.kind =
        CreativeEditorWorldLayoutCanvasPressKind::BeginRegionSelection;
    plan.regionSelection = {true, input.pointerPoint, input.pointerPoint,
                            input.additive, input.toggle};
    return plan;
  }

  if (input.dragToolActive) {
    plan.kind = CreativeEditorWorldLayoutCanvasPressKind::BeginGesture;
    appendCommand(
        plan.commands, CreativeDesktopCommandId::WorldLayoutCanvasGesture,
        CreativeDesktopWorldLayoutGesturePayload{
            CreativeEditorWorldLayoutGesturePhase::Begin,
            input.hoveredPoint});
    return plan;
  }

  plan.kind = CreativeEditorWorldLayoutCanvasPressKind::ApplyPoint;
  appendCommand(plan.commands,
                CreativeDesktopCommandId::WorldLayoutCanvasPoint,
                CreativeDesktopWorldLayoutPointPayload{input.hoveredPoint});
  return plan;
}

CreativeEditorWorldLayoutCanvasSourceInteractionPlan
planCreativeEditorWorldLayoutCanvasSourceInteraction(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutPlanHit& hit,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) {
  CreativeEditorWorldLayoutCanvasSourceInteractionPlan plan;
  plan.requested = true;
  plan.hit = hit;
  if (!hit.hit) {
    return plan;
  }

  plan.accepted = true;
  plan.beginObjectManipulation =
      hit.table == cr::CreativeWorldLayoutTable::Object;
  appendCommand(
      plan.commands, CreativeDesktopCommandId::WorldLayoutSelectSourceScope,
      sourcePayload(state, hit));
  if (plan.beginObjectManipulation) {
    return plan;
  }

  switch (hit.table) {
    case cr::CreativeWorldLayoutTable::Room:
      if (state.source.roomBoundaries.empty()) {
        appendCommand(
            plan.commands,
            CreativeDesktopCommandId::WorldLayoutManipulateRoom,
            CreativeDesktopWorldLayoutRoomManipulationPayload{
                CreativeEditorWorldLayoutRoomManipulationPhase::Begin,
                point, toleranceCells});
      } else {
        const CreativeEditorWorldLayoutRoomCornerTarget corner =
            findCreativeEditorWorldLayoutRoomCornerTarget(
                state, hit.sourceIndex, point, toleranceCells);
        if (corner.topologyVertexIndex !=
            cr::kInvalidCreativeWorldLayoutIndex) {
          appendCommand(
              plan.commands,
              CreativeDesktopCommandId::WorldLayoutManipulateRoomCorner,
              CreativeDesktopWorldLayoutRoomCornerManipulationPayload{
                  CreativeEditorWorldLayoutRoomCornerManipulationPhase::Begin,
                  point, toleranceCells});
        } else {
          appendCommand(
              plan.commands,
              CreativeDesktopCommandId::WorldLayoutManipulateRoomBoundary,
              CreativeDesktopWorldLayoutRoomBoundaryManipulationPayload{
                  CreativeEditorWorldLayoutRoomBoundaryManipulationPhase::Begin,
                  point, toleranceCells});
        }
      }
      break;
    case cr::CreativeWorldLayoutTable::TopologyEdge:
      appendCommand(
          plan.commands,
          CreativeDesktopCommandId::WorldLayoutManipulateRoomBoundary,
          CreativeDesktopWorldLayoutRoomBoundaryManipulationPayload{
              CreativeEditorWorldLayoutRoomBoundaryManipulationPhase::Begin,
              point, toleranceCells});
      break;
    case cr::CreativeWorldLayoutTable::VerticalConnector:
      appendCommand(
          plan.commands,
          CreativeDesktopCommandId::WorldLayoutManipulateVerticalConnector,
          CreativeDesktopWorldLayoutVerticalConnectorManipulationPayload{
              CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
                  Begin,
              point, toleranceCells, {}});
      break;
    case cr::CreativeWorldLayoutTable::Box:
      appendCommand(
          plan.commands,
          CreativeDesktopCommandId::WorldLayoutManipulateBox,
          CreativeDesktopWorldLayoutBoxManipulationPayload{
              CreativeEditorWorldLayoutBoxManipulationPhase::Begin,
              point, toleranceCells});
      break;
    case cr::CreativeWorldLayoutTable::Wall:
      appendCommand(
          plan.commands,
          CreativeDesktopCommandId::WorldLayoutManipulateWall,
          CreativeDesktopWorldLayoutWallManipulationPayload{
              CreativeEditorWorldLayoutWallManipulationPhase::Begin,
              point, toleranceCells});
      break;
    case cr::CreativeWorldLayoutTable::Opening:
      appendCommand(
          plan.commands,
          CreativeDesktopCommandId::WorldLayoutManipulateOpening,
          CreativeDesktopWorldLayoutOpeningManipulationPayload{
              CreativeEditorWorldLayoutOpeningManipulationPhase::Begin,
              point, toleranceCells});
      break;
    case cr::CreativeWorldLayoutTable::RoofAperture:
      appendCommand(
          plan.commands,
          CreativeDesktopCommandId::WorldLayoutManipulateRoofAperture,
          CreativeDesktopWorldLayoutRoofApertureManipulationPayload{
              CreativeEditorWorldLayoutRoofApertureManipulationPhase::Begin,
              point, toleranceCells});
      break;
    case cr::CreativeWorldLayoutTable::None:
    case cr::CreativeWorldLayoutTable::Building:
    case cr::CreativeWorldLayoutTable::Level:
    case cr::CreativeWorldLayoutTable::Object:
    case cr::CreativeWorldLayoutTable::TerrainProfile:
    case cr::CreativeWorldLayoutTable::TerrainPath:
    case cr::CreativeWorldLayoutTable::TerrainPathPoint:
      break;
  }
  return plan;
}

CreativeEditorWorldLayoutCanvasCommandPlan
planCreativeEditorWorldLayoutCanvasManipulations(
    const CreativeEditorWorldLayoutCanvasManipulationSnapshot& manipulations,
    const CreativeEditorWorldLayoutCanvasPointerSnapshot& pointer) {
  CreativeEditorWorldLayoutCanvasCommandPlan plan;
  const bool cancel =
      pointer.focusLost ||
      (pointer.hovered && pointer.secondaryPressed) ||
      pointer.cancelPressed;

  if (manipulations.roofActive) {
    const CreativeEditorWorldLayoutRoofManipulationPhase phase =
        pointerPhase<CreativeEditorWorldLayoutRoofManipulationPhase>(
            cancel, pointer.primaryReleased, pointer.primaryDown);
    if (phase != CreativeEditorWorldLayoutRoofManipulationPhase::Count) {
      appendCommand(
          plan, CreativeDesktopCommandId::WorldLayoutManipulateRoof,
          CreativeDesktopWorldLayoutRoofManipulationPayload{
              phase, manipulations.roofTarget,
              phase == CreativeEditorWorldLayoutRoofManipulationPhase::Cancel
                  ? 0.0
                  : roofHandleCoordinate(manipulations.roofTarget.handle,
                                         pointer.pointerPoint)});
    }
  }

  appendPointerCommand<CreativeEditorWorldLayoutBuildingManipulationPhase>(
      plan, manipulations.buildingActive, cancel, pointer.primaryReleased,
      pointer.primaryDown,
      CreativeDesktopCommandId::WorldLayoutManipulateBuilding,
      CreativeDesktopWorldLayoutBuildingManipulationPayload{
          CreativeEditorWorldLayoutBuildingManipulationPhase::Begin,
          pointer.pointerPoint, pointer.toleranceCells});
  appendPointerCommand<CreativeEditorWorldLayoutOpeningManipulationPhase>(
      plan, manipulations.openingActive, cancel, pointer.primaryReleased,
      pointer.primaryDown,
      CreativeDesktopCommandId::WorldLayoutManipulateOpening,
      CreativeDesktopWorldLayoutOpeningManipulationPayload{
          CreativeEditorWorldLayoutOpeningManipulationPhase::Begin,
          pointer.pointerPoint, pointer.toleranceCells});
  appendPointerCommand<CreativeEditorWorldLayoutWallManipulationPhase>(
      plan, manipulations.wallActive, cancel, pointer.primaryReleased,
      pointer.primaryDown,
      CreativeDesktopCommandId::WorldLayoutManipulateWall,
      CreativeDesktopWorldLayoutWallManipulationPayload{
          CreativeEditorWorldLayoutWallManipulationPhase::Begin,
          pointer.pointerPoint, pointer.toleranceCells});
  appendPointerCommand<CreativeEditorWorldLayoutRoomManipulationPhase>(
      plan, manipulations.roomActive, cancel, pointer.primaryReleased,
      pointer.primaryDown,
      CreativeDesktopCommandId::WorldLayoutManipulateRoom,
      CreativeDesktopWorldLayoutRoomManipulationPayload{
          CreativeEditorWorldLayoutRoomManipulationPhase::Begin,
          pointer.pointerPoint, pointer.toleranceCells});
  appendPointerCommand<
      CreativeEditorWorldLayoutRoomBoundaryManipulationPhase>(
      plan, manipulations.roomBoundaryActive, cancel, pointer.primaryReleased,
      pointer.primaryDown,
      CreativeDesktopCommandId::WorldLayoutManipulateRoomBoundary,
      CreativeDesktopWorldLayoutRoomBoundaryManipulationPayload{
          CreativeEditorWorldLayoutRoomBoundaryManipulationPhase::Begin,
          pointer.pointerPoint, pointer.toleranceCells});
  appendPointerCommand<CreativeEditorWorldLayoutRoomCornerManipulationPhase>(
      plan, manipulations.roomCornerActive, cancel, pointer.primaryReleased,
      pointer.primaryDown,
      CreativeDesktopCommandId::WorldLayoutManipulateRoomCorner,
      CreativeDesktopWorldLayoutRoomCornerManipulationPayload{
          CreativeEditorWorldLayoutRoomCornerManipulationPhase::Begin,
          pointer.pointerPoint, pointer.toleranceCells});
  appendPointerCommand<
      CreativeEditorWorldLayoutVerticalConnectorManipulationPhase>(
      plan, manipulations.verticalConnectorActive, cancel,
      pointer.primaryReleased, pointer.primaryDown,
      CreativeDesktopCommandId::WorldLayoutManipulateVerticalConnector,
      CreativeDesktopWorldLayoutVerticalConnectorManipulationPayload{
          CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::Begin,
          pointer.pointerPoint, pointer.toleranceCells, {}});
  appendPointerCommand<CreativeEditorWorldLayoutBoxManipulationPhase>(
      plan, manipulations.boxActive, cancel, pointer.primaryReleased,
      pointer.primaryDown,
      CreativeDesktopCommandId::WorldLayoutManipulateBox,
      CreativeDesktopWorldLayoutBoxManipulationPayload{
          CreativeEditorWorldLayoutBoxManipulationPhase::Begin,
          pointer.pointerPoint, pointer.toleranceCells});
  appendPointerCommand<
      CreativeEditorWorldLayoutRoofApertureManipulationPhase>(
      plan, manipulations.roofApertureActive, cancel, pointer.primaryReleased,
      pointer.primaryDown,
      CreativeDesktopCommandId::WorldLayoutManipulateRoofAperture,
      CreativeDesktopWorldLayoutRoofApertureManipulationPayload{
          CreativeEditorWorldLayoutRoofApertureManipulationPhase::Begin,
          pointer.pointerPoint, pointer.toleranceCells});

  const bool geometryManipulationActive =
      manipulations.objectActive || manipulations.roofActive ||
      manipulations.buildingActive || manipulations.openingActive ||
      manipulations.wallActive || manipulations.roomActive ||
      manipulations.roomBoundaryActive || manipulations.roomCornerActive ||
      manipulations.verticalConnectorActive || manipulations.boxActive ||
      manipulations.roofApertureActive;
  const bool cancelGesture =
      !geometryManipulationActive && manipulations.anchorActive && cancel;
  const bool finishTerrainPathDraft =
      manipulations.terrainPathDraftActive &&
      (pointer.confirmPressed ||
       (pointer.hovered && pointer.primaryDoubleClicked));
  if (cancelGesture) {
    appendCommand(
        plan, CreativeDesktopCommandId::WorldLayoutCanvasGesture,
        CreativeDesktopWorldLayoutGesturePayload{
            CreativeEditorWorldLayoutGesturePhase::Cancel, {}});
  } else if (finishTerrainPathDraft) {
    appendCommand(
        plan, CreativeDesktopCommandId::WorldLayoutCanvasGesture,
        CreativeDesktopWorldLayoutGesturePayload{
            CreativeEditorWorldLayoutGesturePhase::Commit, {}});
  } else if (manipulations.dragToolActive &&
             pointer.primaryReleased && pointer.itemDeactivated) {
    appendCommand(
        plan, CreativeDesktopCommandId::WorldLayoutCanvasGesture,
        CreativeDesktopWorldLayoutGesturePayload{
            CreativeEditorWorldLayoutGesturePhase::Commit,
            pointer.hoveredPoint});
  } else if (manipulations.anchorActive &&
             manipulations.dragToolActive && pointer.primaryDown) {
    appendCommand(
        plan, CreativeDesktopCommandId::WorldLayoutCanvasGesture,
        CreativeDesktopWorldLayoutGesturePayload{
            CreativeEditorWorldLayoutGesturePhase::Update,
            pointer.hoveredPoint});
  }
  return plan;
}

void enqueueCreativeEditorWorldLayoutCanvasCommandPlan(
    CreativeDesktopCommandFrame& frame,
    CreativeEditorWorldLayoutCanvasCommandPlan plan) {
  assert(!plan.overflowed &&
         "World Layout Canvas planner exceeded its fixed command capacity");
  for (std::size_t index = 0U; index < plan.count; ++index) {
    CreativeEditorWorldLayoutCanvasPlannedCommand& command =
        plan.commands[index];
    std::visit(
        [&](auto payload) {
          frame.enqueue(
              command.id,
              CreativeDesktopCommandPayload{std::move(payload)});
        },
        std::move(command.payload));
  }
}

}  // namespace iggy3d_creative_app
