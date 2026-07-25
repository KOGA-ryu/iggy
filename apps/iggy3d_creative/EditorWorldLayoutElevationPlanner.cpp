#include "EditorWorldLayoutElevationPlanner.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <string>
#include <utility>

#include "EditorWorldLayoutBuildings.hpp"
#include "EditorWorldLayoutOpenings.hpp"
#include "EditorWorldLayoutPlan.hpp"
#include "EditorWorldLayoutRoofs.hpp"
#include "EditorWorldLayoutSources.hpp"
#include "EditorWorldLayoutVerticalConnectorHandles.hpp"

namespace iggy3d_creative_app {
namespace {

template <typename Payload>
void appendCommand(CreativeEditorWorldLayoutElevationCommandPlan& plan,
                   CreativeDesktopCommandId id, Payload payload) {
  assert(plan.count < plan.commands.size());
  if (plan.count >= plan.commands.size()) {
    return;
  }
  plan.commands[plan.count++] = {
      id, CreativeDesktopCommandPayload{std::move(payload)}};
}

void appendCommand(CreativeEditorWorldLayoutElevationCommandPlan& plan,
                   CreativeDesktopCommandId id) {
  appendCommand(plan, id, std::monostate{});
}

[[nodiscard]] bool selected(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutSelectionKind kind,
    std::size_t index) noexcept {
  return state.selection.kind == kind && state.selection.index == index;
}

[[nodiscard]] CreativeEditorWorldLayoutElevationSourceKind
selectionSourceKind(CreativeEditorWorldLayoutSelectionKind kind) noexcept {
  switch (kind) {
    case CreativeEditorWorldLayoutSelectionKind::Room:
      return CreativeEditorWorldLayoutElevationSourceKind::Room;
    case CreativeEditorWorldLayoutSelectionKind::Box:
      return CreativeEditorWorldLayoutElevationSourceKind::Box;
    case CreativeEditorWorldLayoutSelectionKind::Wall:
      return CreativeEditorWorldLayoutElevationSourceKind::Wall;
    case CreativeEditorWorldLayoutSelectionKind::Opening:
      return CreativeEditorWorldLayoutElevationSourceKind::Opening;
    case CreativeEditorWorldLayoutSelectionKind::RoofAperture:
      return CreativeEditorWorldLayoutElevationSourceKind::RoofAperture;
    case CreativeEditorWorldLayoutSelectionKind::VerticalConnector:
      return CreativeEditorWorldLayoutElevationSourceKind::VerticalConnector;
    case CreativeEditorWorldLayoutSelectionKind::None:
    case CreativeEditorWorldLayoutSelectionKind::Building:
    case CreativeEditorWorldLayoutSelectionKind::Level:
    case CreativeEditorWorldLayoutSelectionKind::TerrainProfile:
    case CreativeEditorWorldLayoutSelectionKind::TerrainPath:
    case CreativeEditorWorldLayoutSelectionKind::Object:
    case CreativeEditorWorldLayoutSelectionKind::TopologyEdge:
      break;
  }
  return CreativeEditorWorldLayoutElevationSourceKind::None;
}

[[nodiscard]] iggy3d::creative::CreativeWorldLayoutTable sourceTable(
    CreativeEditorWorldLayoutElevationSourceKind kind) noexcept {
  namespace cr = iggy3d::creative;
  switch (kind) {
    case CreativeEditorWorldLayoutElevationSourceKind::Room:
      return cr::CreativeWorldLayoutTable::Room;
    case CreativeEditorWorldLayoutElevationSourceKind::Box:
      return cr::CreativeWorldLayoutTable::Box;
    case CreativeEditorWorldLayoutElevationSourceKind::Wall:
      return cr::CreativeWorldLayoutTable::Wall;
    case CreativeEditorWorldLayoutElevationSourceKind::Opening:
      return cr::CreativeWorldLayoutTable::Opening;
    case CreativeEditorWorldLayoutElevationSourceKind::RoofAperture:
      return cr::CreativeWorldLayoutTable::RoofAperture;
    case CreativeEditorWorldLayoutElevationSourceKind::VerticalConnector:
      return cr::CreativeWorldLayoutTable::VerticalConnector;
    case CreativeEditorWorldLayoutElevationSourceKind::None:
    case CreativeEditorWorldLayoutElevationSourceKind::Count:
      return cr::CreativeWorldLayoutTable::None;
  }
  return cr::CreativeWorldLayoutTable::None;
}

void appendSourceSelection(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutElevationSourceKind sourceKind,
    std::size_t sourceIndex, std::size_t levelIndex,
    CreativeEditorWorldLayoutElevationCommandPlan& commands) {
  namespace cr = iggy3d::creative;
  const cr::CreativeWorldLayoutTable table = sourceTable(sourceKind);
  if (table == cr::CreativeWorldLayoutTable::None) {
    return;
  }
  appendCommand(
      commands, CreativeDesktopCommandId::WorldLayoutSelectSourceScope,
      CreativeDesktopWorldLayoutSourcePayload{
          table, sourceIndex,
          std::string(creativeEditorWorldLayoutSourceStableKey(
              state, table, sourceIndex)),
          levelIndex});
}

void appendRoofSelection(
    const CreativeEditorWorldLayoutState& state, std::size_t levelIndex,
    CreativeEditorWorldLayoutElevationCommandPlan& commands) {
  namespace cr = iggy3d::creative;
  if (levelIndex >= state.source.levels.size()) {
    return;
  }
  appendCommand(
      commands, CreativeDesktopCommandId::WorldLayoutSelectSourceScope,
      CreativeDesktopWorldLayoutSourcePayload{
          cr::CreativeWorldLayoutTable::Level, levelIndex,
          std::string(creativeEditorWorldLayoutSourceStableKey(
              state, cr::CreativeWorldLayoutTable::Level, levelIndex)),
          levelIndex});
}

void appendRoofManipulation(
    CreativeEditorWorldLayoutElevationCommandPlan& commands,
    CreativeEditorWorldLayoutRoofManipulationPhase phase,
    std::size_t levelIndex, double verticalCells) {
  appendCommand(
      commands, CreativeDesktopCommandId::WorldLayoutManipulateRoof,
      CreativeDesktopWorldLayoutRoofManipulationPayload{
          phase,
          {levelIndex, CreativeEditorWorldLayoutRoofHandleKind::RidgeHeight},
          verticalCells});
}

[[nodiscard]] CreativeEditorWorldLayoutPoint connectorPoint(
    iggy3d::creative::CreativeWorldLayoutRect footprint,
    CreativeEditorWorldLayoutRectHandle handle, double horizontal) {
  CreativeEditorWorldLayoutPoint point{
      (static_cast<double>(footprint.minimum.x) + footprint.maximum.x) * 0.5,
      (static_cast<double>(footprint.minimum.z) + footprint.maximum.z) * 0.5};
  if (handle == CreativeEditorWorldLayoutRectHandle::East ||
      handle == CreativeEditorWorldLayoutRectHandle::West) {
    point.x = horizontal;
  } else if (handle == CreativeEditorWorldLayoutRectHandle::North ||
             handle == CreativeEditorWorldLayoutRectHandle::South) {
    point.z = horizontal;
  }
  return point;
}

void appendConnectorManipulation(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutElevationCommandPlan& commands,
    CreativeEditorWorldLayoutVerticalConnectorManipulationPhase phase,
    CreativeEditorWorldLayoutVerticalConnectorTarget target,
    double horizontal) {
  if (target.connectorIndex >= state.source.verticalConnectors.size()) {
    return;
  }
  const iggy3d::creative::CreativeWorldLayoutRect footprint =
      state.source.verticalConnectors[target.connectorIndex].footprint;
  appendCommand(
      commands,
      CreativeDesktopCommandId::WorldLayoutManipulateVerticalConnector,
      CreativeDesktopWorldLayoutVerticalConnectorManipulationPayload{
          phase, connectorPoint(footprint, target.handle, horizontal), 0.25,
          target});
}

[[nodiscard]] bool appendElevationEdit(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutElevationEditResult& edit,
    CreativeEditorWorldLayoutElevationCommandPlan& commands) {
  if (!edit.accepted) {
    return false;
  }
  if (edit.handle.sourceKind ==
      CreativeEditorWorldLayoutElevationSourceKind::Room) {
    if (edit.handle.kind ==
        CreativeEditorWorldLayoutElevationHandleKind::LevelFloor) {
      if (edit.handle.levelIndex >= state.source.levels.size()) {
        return false;
      }
      appendCommand(
          commands, CreativeDesktopCommandId::WorldLayoutSetLevelDatum,
          CreativeDesktopWorldLayoutLevelDatumPayload{
              edit.handle.levelIndex,
              state.source.levels[edit.handle.levelIndex].stableKey,
              state.elevationLevelEditScope, edit.floorTopLayer});
      return true;
    }
    if (edit.handle.kind !=
            CreativeEditorWorldLayoutElevationHandleKind::WallTop ||
        edit.handle.levelIndex >= state.source.levels.size()) {
      return false;
    }
    CreativeEditorWorldLayoutLevelSettings settings;
    if (!readCreativeEditorWorldLayoutLevelSettings(
            state, edit.handle.levelIndex, settings)) {
      return false;
    }
    settings.wallHeightCells = edit.wallHeightCells;
    appendCommand(
        commands, CreativeDesktopCommandId::WorldLayoutSetLevelSettings,
        CreativeDesktopWorldLayoutLevelSettingsPayload{
            edit.handle.levelIndex,
            state.source.levels[edit.handle.levelIndex].stableKey, settings});
    return true;
  }
  if (edit.handle.sourceKind ==
      CreativeEditorWorldLayoutElevationSourceKind::Box) {
    CreativeEditorWorldLayoutBoxSettings settings;
    if (!readCreativeEditorWorldLayoutBoxSettings(
            state, edit.handle.sourceIndex, settings)) {
      return false;
    }
    settings.anchorLayer = edit.floorTopLayer;
    appendCommand(
        commands, CreativeDesktopCommandId::WorldLayoutSetBoxSettings,
        CreativeDesktopWorldLayoutBoxSettingsPayload{edit.handle.sourceIndex,
                                                     settings});
    return true;
  }
  if (edit.handle.sourceKind ==
      CreativeEditorWorldLayoutElevationSourceKind::Wall) {
    CreativeEditorWorldLayoutWallSettings settings;
    if (!readCreativeEditorWorldLayoutWallSettings(
            state, edit.handle.sourceIndex, settings)) {
      return false;
    }
    settings.heightCells = edit.wallHeightCells;
    appendCommand(
        commands, CreativeDesktopCommandId::WorldLayoutSetWallSettings,
        CreativeDesktopWorldLayoutWallSettingsPayload{edit.handle.sourceIndex,
                                                      settings});
    return true;
  }
  if (edit.handle.sourceKind ==
      CreativeEditorWorldLayoutElevationSourceKind::Opening) {
    CreativeEditorWorldLayoutOpeningSettings settings;
    if (!readCreativeEditorWorldLayoutOpeningSettings(
            state, edit.handle.sourceIndex, settings)) {
      return false;
    }
    settings.sillHeightCells = edit.openingSillCells;
    settings.heightCells = edit.openingHeightCells;
    appendCommand(
        commands, CreativeDesktopCommandId::WorldLayoutSetOpeningSettings,
        CreativeDesktopWorldLayoutOpeningSettingsPayload{
            edit.handle.sourceIndex, settings});
    return true;
  }
  return false;
}

void selectItem(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutElevationItem& item,
    CreativeEditorWorldLayoutElevationInteractionPlan& output) {
  CreativeEditorWorldLayoutSelectionKind selectionKind =
      CreativeEditorWorldLayoutSelectionKind::None;
  std::size_t sourceSize = 0U;
  switch (item.sourceKind) {
    case CreativeEditorWorldLayoutElevationSourceKind::Room:
      selectionKind = CreativeEditorWorldLayoutSelectionKind::Room;
      sourceSize = state.source.rooms.size();
      break;
    case CreativeEditorWorldLayoutElevationSourceKind::Box:
      selectionKind = CreativeEditorWorldLayoutSelectionKind::Box;
      sourceSize = state.source.boxes.size();
      break;
    case CreativeEditorWorldLayoutElevationSourceKind::Wall:
      selectionKind = CreativeEditorWorldLayoutSelectionKind::Wall;
      sourceSize = state.source.walls.size();
      break;
    case CreativeEditorWorldLayoutElevationSourceKind::Opening:
      selectionKind = CreativeEditorWorldLayoutSelectionKind::Opening;
      sourceSize = state.source.openings.size();
      break;
    case CreativeEditorWorldLayoutElevationSourceKind::RoofAperture:
      selectionKind = CreativeEditorWorldLayoutSelectionKind::RoofAperture;
      sourceSize = state.source.roofApertures.size();
      break;
    case CreativeEditorWorldLayoutElevationSourceKind::VerticalConnector:
      selectionKind = CreativeEditorWorldLayoutSelectionKind::VerticalConnector;
      sourceSize = state.source.verticalConnectors.size();
      break;
    case CreativeEditorWorldLayoutElevationSourceKind::None:
    case CreativeEditorWorldLayoutElevationSourceKind::Count:
      return;
  }
  if (item.sourceIndex >= sourceSize) {
    return;
  }
  output.selectionChanged = true;
  output.selection = {selectionKind, item.sourceIndex};
  output.activeLevelIndex = state.activeLevelIndex;
  if (item.sourceKind ==
      CreativeEditorWorldLayoutElevationSourceKind::Room) {
    output.activeLevelIndex = state.source.rooms[item.sourceIndex].levelIndex;
  } else if ((item.sourceKind ==
                  CreativeEditorWorldLayoutElevationSourceKind::Opening ||
              item.sourceKind ==
                  CreativeEditorWorldLayoutElevationSourceKind::RoofAperture ||
              item.sourceKind ==
                  CreativeEditorWorldLayoutElevationSourceKind::
                      VerticalConnector) &&
             item.levelIndex < state.source.levels.size()) {
    output.activeLevelIndex = item.levelIndex;
  }
}

void selectHandle(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutElevationHandle& handle,
    CreativeEditorWorldLayoutElevationInteractionPlan& output) {
  CreativeEditorWorldLayoutElevationItem item;
  item.sourceKind = handle.sourceKind;
  item.sourceIndex = handle.sourceIndex;
  item.levelIndex = handle.levelIndex;
  if (handle.kind == CreativeEditorWorldLayoutElevationHandleKind::RoofRidge &&
      handle.levelIndex < state.source.levels.size()) {
    output.selectionChanged = true;
    output.selection = {CreativeEditorWorldLayoutSelectionKind::Level,
                        handle.levelIndex};
    output.activeLevelIndex = handle.levelIndex;
    return;
  }
  selectItem(state, item, output);
}

[[nodiscard]] bool connectorHandle(
    CreativeEditorWorldLayoutElevationHandleKind kind) noexcept {
  return kind ==
             CreativeEditorWorldLayoutElevationHandleKind::ConnectorRunLow ||
         kind ==
             CreativeEditorWorldLayoutElevationHandleKind::ConnectorRunHigh;
}

}  // namespace

CreativeEditorWorldLayoutVerticalConnectorTarget
creativeEditorWorldLayoutElevationConnectorTarget(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutElevationHandle& handle) {
  namespace cr = iggy3d::creative;
  CreativeEditorWorldLayoutVerticalConnectorTarget target;
  if (handle.sourceKind !=
          CreativeEditorWorldLayoutElevationSourceKind::VerticalConnector ||
      handle.sourceIndex >= state.source.verticalConnectors.size()) {
    return target;
  }
  target.connectorIndex = handle.sourceIndex;
  const cr::CreativeWorldLayoutVerticalDirection direction =
      state.source.verticalConnectors[handle.sourceIndex].direction;
  const bool low =
      handle.kind ==
      CreativeEditorWorldLayoutElevationHandleKind::ConnectorRunLow;
  const bool high =
      handle.kind ==
      CreativeEditorWorldLayoutElevationHandleKind::ConnectorRunHigh;
  if (!low && !high) {
    return {};
  }
  switch (direction) {
    case cr::CreativeWorldLayoutVerticalDirection::PositiveX:
      target.handle = low ? CreativeEditorWorldLayoutRectHandle::West
                          : CreativeEditorWorldLayoutRectHandle::East;
      break;
    case cr::CreativeWorldLayoutVerticalDirection::NegativeX:
      target.handle = low ? CreativeEditorWorldLayoutRectHandle::East
                          : CreativeEditorWorldLayoutRectHandle::West;
      break;
    case cr::CreativeWorldLayoutVerticalDirection::PositiveZ:
      target.handle = low ? CreativeEditorWorldLayoutRectHandle::North
                          : CreativeEditorWorldLayoutRectHandle::South;
      break;
    case cr::CreativeWorldLayoutVerticalDirection::NegativeZ:
      target.handle = low ? CreativeEditorWorldLayoutRectHandle::South
                          : CreativeEditorWorldLayoutRectHandle::North;
      break;
    case cr::CreativeWorldLayoutVerticalDirection::Count:
      return {};
  }
  return target;
}

CreativeEditorWorldLayoutElevationScreenPoint
planCreativeEditorWorldLayoutElevationScreenPoint(
    const CreativeEditorWorldLayoutElevationCanvasTransform& transform,
    CreativeEditorWorldLayoutElevationPoint point) noexcept {
  return {
      transform.origin.x +
          static_cast<float>(point.horizontal) * transform.pixelsPerCell,
      transform.origin.y -
          static_cast<float>(point.vertical) * transform.pixelsPerCell};
}

CreativeEditorWorldLayoutElevationPoint
planCreativeEditorWorldLayoutElevationWorldPoint(
    const CreativeEditorWorldLayoutElevationCanvasTransform& transform,
    CreativeEditorWorldLayoutElevationScreenPoint point) noexcept {
  return {(point.x - transform.origin.x) / transform.pixelsPerCell,
          (transform.origin.y - point.y) / transform.pixelsPerCell};
}

bool creativeEditorWorldLayoutElevationHandleVisible(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutElevationHandle& handle) noexcept {
  if (state.elevationManipulation.active &&
      state.elevationManipulation.handle.kind == handle.kind &&
      state.elevationManipulation.handle.sourceKind == handle.sourceKind &&
      state.elevationManipulation.handle.sourceIndex == handle.sourceIndex) {
    return true;
  }
  switch (handle.sourceKind) {
    case CreativeEditorWorldLayoutElevationSourceKind::Room:
      if (selected(state, CreativeEditorWorldLayoutSelectionKind::Room,
                   handle.sourceIndex)) {
        return true;
      }
      return (state.selection.kind ==
                  CreativeEditorWorldLayoutSelectionKind::None ||
              state.selection.kind ==
                  CreativeEditorWorldLayoutSelectionKind::Building ||
              state.selection.kind ==
                  CreativeEditorWorldLayoutSelectionKind::Level) &&
             handle.levelIndex == state.activeLevelIndex;
    case CreativeEditorWorldLayoutElevationSourceKind::Box:
      return selected(state, CreativeEditorWorldLayoutSelectionKind::Box,
                      handle.sourceIndex);
    case CreativeEditorWorldLayoutElevationSourceKind::Wall:
      return selected(state, CreativeEditorWorldLayoutSelectionKind::Wall,
                      handle.sourceIndex);
    case CreativeEditorWorldLayoutElevationSourceKind::Opening:
      return selected(state, CreativeEditorWorldLayoutSelectionKind::Opening,
                      handle.sourceIndex);
    case CreativeEditorWorldLayoutElevationSourceKind::RoofAperture:
      return false;
    case CreativeEditorWorldLayoutElevationSourceKind::VerticalConnector:
      return (state.verticalConnectorManipulation.active &&
              state.verticalConnectorManipulation.target.connectorIndex ==
                  handle.sourceIndex) ||
             selected(
                 state,
                 CreativeEditorWorldLayoutSelectionKind::VerticalConnector,
                 handle.sourceIndex);
    case CreativeEditorWorldLayoutElevationSourceKind::None:
    case CreativeEditorWorldLayoutElevationSourceKind::Count:
      break;
  }
  return false;
}

CreativeEditorWorldLayoutElevationHandle
findVisibleCreativeEditorWorldLayoutElevationHandle(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutElevationProjection& projection,
    CreativeEditorWorldLayoutElevationPoint point,
    double toleranceCells) noexcept {
  if (!projection.accepted || !std::isfinite(point.horizontal) ||
      !std::isfinite(point.vertical) || !std::isfinite(toleranceCells) ||
      toleranceCells <= 0.0) {
    return {};
  }
  const double toleranceSquared = toleranceCells * toleranceCells;
  for (auto iterator = projection.handles.rbegin();
       iterator != projection.handles.rend(); ++iterator) {
    if (!creativeEditorWorldLayoutElevationHandleVisible(state, *iterator)) {
      continue;
    }
    const double horizontal = point.horizontal - iterator->position.horizontal;
    const double vertical = point.vertical - iterator->position.vertical;
    if (horizontal * horizontal + vertical * vertical <= toleranceSquared) {
      return *iterator;
    }
  }
  return {};
}

CreativeEditorWorldLayoutElevationInteractionPlan
planCreativeEditorWorldLayoutElevationInteraction(
    const CreativeEditorWorldLayoutElevationInteractionInput& input) {
  CreativeEditorWorldLayoutElevationInteractionPlan output;
  if (input.state == nullptr || input.projection == nullptr) {
    return output;
  }
  const CreativeEditorWorldLayoutState& state = *input.state;
  const CreativeEditorWorldLayoutElevationProjection& projection =
      *input.projection;
  const CreativeEditorWorldLayoutElevationPointerInput& pointer = input.pointer;
  CreativeEditorWorldLayoutElevationManipulationState elevationManipulation =
      state.elevationManipulation;

  if (!input.interactionEnabled || !projection.accepted) {
    if (state.roofManipulation.active) {
      appendRoofManipulation(
          output.commands,
          CreativeEditorWorldLayoutRoofManipulationPhase::Cancel,
          state.roofManipulation.target.levelIndex, 0.0);
    }
    if (state.verticalConnectorManipulation.active) {
      appendConnectorManipulation(
          state, output.commands,
          CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::Cancel,
          state.verticalConnectorManipulation.target, 0.0);
    }
    output.replaceElevationManipulation = true;
    return output;
  }

  output.hoveredHandle =
      pointer.hovered
          ? findVisibleCreativeEditorWorldLayoutElevationHandle(
                state, projection, pointer.point, pointer.handleToleranceCells)
          : CreativeEditorWorldLayoutElevationHandle{};
  const bool hoveredConnector = connectorHandle(output.hoveredHandle.kind);
  if (state.verticalConnectorManipulation.active || hoveredConnector) {
    output.cursor =
        CreativeEditorWorldLayoutElevationCursor::ResizeHorizontal;
  } else if (state.elevationManipulation.active ||
             state.roofManipulation.active ||
             output.hoveredHandle.kind !=
                 CreativeEditorWorldLayoutElevationHandleKind::None) {
    output.cursor = CreativeEditorWorldLayoutElevationCursor::ResizeVertical;
  }

  if (pointer.hovered && pointer.primaryPressed) {
    if (output.hoveredHandle.kind !=
        CreativeEditorWorldLayoutElevationHandleKind::None) {
      selectHandle(state, output.hoveredHandle, output);
      if (output.hoveredHandle.kind ==
          CreativeEditorWorldLayoutElevationHandleKind::RoofRidge) {
        appendRoofSelection(state, output.hoveredHandle.levelIndex,
                            output.commands);
        appendRoofManipulation(
            output.commands,
            CreativeEditorWorldLayoutRoofManipulationPhase::Begin,
            output.hoveredHandle.levelIndex, pointer.point.vertical);
      } else if (hoveredConnector) {
        const CreativeEditorWorldLayoutVerticalConnectorTarget target =
            creativeEditorWorldLayoutElevationConnectorTarget(
                state, output.hoveredHandle);
        appendSourceSelection(
            state, output.hoveredHandle.sourceKind,
            output.hoveredHandle.sourceIndex, output.hoveredHandle.levelIndex,
            output.commands);
        appendConnectorManipulation(
            state, output.commands,
            CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::Begin,
            target, pointer.point.horizontal);
      } else {
        output.replaceElevationManipulation = true;
        elevationManipulation = {
            true, state.revision, output.hoveredHandle,
            planCreativeEditorWorldLayoutElevationEdit(
                state.source, projection, output.hoveredHandle,
                pointer.point.vertical, state.elevationLevelEditScope)};
        output.elevationManipulation = elevationManipulation;
        appendSourceSelection(
            state, output.hoveredHandle.sourceKind,
            output.hoveredHandle.sourceIndex, output.hoveredHandle.levelIndex,
            output.commands);
      }
    } else if (const CreativeEditorWorldLayoutElevationItem* item =
                   cycleCreativeEditorWorldLayoutElevationItem(
                       findCreativeEditorWorldLayoutElevationItemStack(
                           projection, pointer.point,
                           pointer.handleToleranceCells),
                       selectionSourceKind(state.selection.kind),
                       state.selection.index);
               item != nullptr) {
      selectItem(state, *item, output);
      appendSourceSelection(state, item->sourceKind, item->sourceIndex,
                            item->levelIndex, output.commands);
    } else {
      appendCommand(output.commands,
                    CreativeDesktopCommandId::WorldLayoutClearSelection);
    }
  }

  if (state.roofManipulation.active) {
    const bool cancelRoof =
        pointer.focusLost ||
        state.roofManipulation.sourceRevision != state.revision ||
        (pointer.hovered && pointer.secondaryPressed) ||
        pointer.cancelPressed;
    if (cancelRoof) {
      appendRoofManipulation(
          output.commands,
          CreativeEditorWorldLayoutRoofManipulationPhase::Cancel,
          state.roofManipulation.target.levelIndex, pointer.point.vertical);
    } else if (pointer.primaryReleased) {
      appendRoofManipulation(
          output.commands,
          CreativeEditorWorldLayoutRoofManipulationPhase::Commit,
          state.roofManipulation.target.levelIndex, pointer.point.vertical);
    } else if (pointer.primaryDown) {
      appendRoofManipulation(
          output.commands,
          CreativeEditorWorldLayoutRoofManipulationPhase::Update,
          state.roofManipulation.target.levelIndex, pointer.point.vertical);
    }
    return output;
  }

  if (state.verticalConnectorManipulation.active) {
    const bool cancelConnector =
        pointer.focusLost ||
        state.verticalConnectorManipulation.sourceRevision != state.revision ||
        (pointer.hovered && pointer.secondaryPressed) ||
        pointer.cancelPressed;
    const CreativeEditorWorldLayoutVerticalConnectorTarget target =
        state.verticalConnectorManipulation.target;
    if (cancelConnector) {
      appendConnectorManipulation(
          state, output.commands,
          CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::Cancel,
          target, pointer.point.horizontal);
    } else if (pointer.primaryReleased) {
      appendConnectorManipulation(
          state, output.commands,
          CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::Commit,
          target, pointer.point.horizontal);
    } else if (pointer.primaryDown) {
      appendConnectorManipulation(
          state, output.commands,
          CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::Update,
          target, pointer.point.horizontal);
    }
    return output;
  }

  if (!elevationManipulation.active) {
    return output;
  }
  output.replaceElevationManipulation = true;
  const bool cancel =
      pointer.focusLost ||
      elevationManipulation.sourceRevision != state.revision ||
      (pointer.hovered && pointer.secondaryPressed) ||
      pointer.cancelPressed;
  if (cancel) {
    return output;
  }
  output.elevationManipulation = elevationManipulation;
  output.elevationManipulation.preview =
      planCreativeEditorWorldLayoutElevationEdit(
          state.source, projection, elevationManipulation.handle,
          pointer.point.vertical, state.elevationLevelEditScope);
  if (pointer.primaryReleased) {
    static_cast<void>(appendElevationEdit(
        state, output.elevationManipulation.preview, output.commands));
    output.elevationManipulation = {};
  }
  return output;
}

bool enqueueCreativeEditorWorldLayoutElevationCommandPlan(
    const CreativeEditorWorldLayoutElevationCommandPlan& plan,
    CreativeDesktopCommandFrame& commands) {
  bool accepted = true;
  for (std::size_t index = 0U; index < plan.count; ++index) {
    const CreativeDesktopCommand& command = plan.commands[index];
    accepted =
        commands.push(command.id, command.payload) ==
            CreativeDesktopCommandEnqueueResult::Enqueued &&
        accepted;
  }
  return accepted;
}

}  // namespace iggy3d_creative_app
