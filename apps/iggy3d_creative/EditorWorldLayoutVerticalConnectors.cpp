#include "EditorWorldLayoutPlan.hpp"

#include "EditorWorldLayoutInternal.hpp"

#include "app/iggy3d/creative/world/WorldLayoutVerticalConnectors.hpp"

#include <cmath>
#include <string>
#include <string_view>
#include <utility>

namespace iggy3d_creative_app {
namespace {

struct ConnectorValidation {
  bool accepted = false;
  std::string_view reasonCode =
      "creative_editor_world_layout_vertical_connector_invalid";
  std::string_view message = "vertical connector settings are invalid";
};

bool sameRect(cr::CreativeWorldLayoutRect lhs,
              cr::CreativeWorldLayoutRect rhs) noexcept {
  return lhs.minimum == rhs.minimum && lhs.maximum == rhs.maximum;
}

CreativeEditorWorldLayoutVerticalConnectorSettings connectorSettings(
    const cr::CreativeWorldLayoutVerticalConnector& connector) noexcept {
  return {connector.footprint, connector.kind, connector.direction,
          connector.material};
}

bool sameSettings(
    const CreativeEditorWorldLayoutVerticalConnectorSettings& lhs,
    const CreativeEditorWorldLayoutVerticalConnectorSettings& rhs) noexcept {
  return sameRect(lhs.footprint, rhs.footprint) && lhs.kind == rhs.kind &&
         lhs.direction == rhs.direction && lhs.material == rhs.material;
}

std::string_view connectorValidationMessage(
    cr::CreativeWorldLayoutVerticalConnectorStatus status) noexcept {
  switch (status) {
    case cr::CreativeWorldLayoutVerticalConnectorStatus::NotRequested:
    case cr::CreativeWorldLayoutVerticalConnectorStatus::InvalidConnector:
      return "vertical connector settings are invalid";
    case cr::CreativeWorldLayoutVerticalConnectorStatus::InvalidOwner:
      return "vertical connector room ownership is unavailable";
    case cr::CreativeWorldLayoutVerticalConnectorStatus::InvalidLevels:
      return "vertical connector needs adjacent compatible levels";
    case cr::CreativeWorldLayoutVerticalConnectorStatus::InvalidFootprint:
      return "vertical connector must fit both owned rooms";
    case cr::CreativeWorldLayoutVerticalConnectorStatus::InvalidLanding:
      return "vertical connector needs a clear landing at both ends";
    case cr::CreativeWorldLayoutVerticalConnectorStatus::InvalidSlope:
      return "vertical connector run or slope exceeds its movement limit";
    case cr::CreativeWorldLayoutVerticalConnectorStatus::InvalidMaterial:
      return "vertical connector material is invalid";
    case cr::CreativeWorldLayoutVerticalConnectorStatus::InvalidHeadroom:
      return "vertical connector needs at least 1.84 m of headroom";
    case cr::CreativeWorldLayoutVerticalConnectorStatus::SurfaceAlreadyCut:
      return "these room surfaces already contain a connector";
    case cr::CreativeWorldLayoutVerticalConnectorStatus::
        UnrepresentableGeometry:
      return "vertical connector geometry is outside supported limits";
    case cr::CreativeWorldLayoutVerticalConnectorStatus::Ready:
      return "vertical connector settings ready";
  }
  return "vertical connector settings are invalid";
}

ConnectorValidation validateConnectorSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t connectorIndex,
    const CreativeEditorWorldLayoutVerticalConnectorSettings& settings,
    cr::CreativeGridSettings grid) {
  if (connectorIndex >= state.source.verticalConnectors.size()) {
    return {false,
            "creative_editor_world_layout_vertical_connector_index_invalid",
            "vertical connector is unavailable"};
  }
  cr::CreativeWorldLayoutVerticalConnector candidate =
      state.source.verticalConnectors[connectorIndex];
  candidate.footprint = settings.footprint;
  candidate.kind = settings.kind;
  candidate.direction = settings.direction;
  candidate.material = settings.material;
  const cr::CreativeWorldLayoutVerticalConnectorPlan plan =
      cr::planCreativeWorldLayoutVerticalConnector(
          grid, state.source, connectorIndex, candidate);
  return {plan.accepted, plan.reasonCode,
          connectorValidationMessage(plan.status)};
}

CreativeEditorWorldLayoutEditReceipt commitConnectorSettings(
    CreativeEditorWorldLayoutState& state, std::size_t connectorIndex,
    const CreativeEditorWorldLayoutVerticalConnectorSettings& settings,
    cr::CreativeGridSettings grid, std::string statusMessage) {
  const ConnectorValidation validation =
      validateConnectorSettings(state, connectorIndex, settings, grid);
  if (!validation.accepted) {
    state.statusMessage = validation.message;
    return {false, false, std::string(validation.reasonCode)};
  }
  cr::CreativeWorldLayoutVerticalConnector& connector =
      state.source.verticalConnectors[connectorIndex];
  if (sameSettings(connectorSettings(connector), settings)) {
    return {
        true, false,
        "creative_editor_world_layout_vertical_connector_settings_no_change"};
  }
  connector.footprint = settings.footprint;
  connector.kind = settings.kind;
  connector.direction = settings.direction;
  connector.material = settings.material;
  state.selection = {
      CreativeEditorWorldLayoutSelectionKind::VerticalConnector,
      connectorIndex};
  detail::noteWorldLayoutSourceChange(state, std::move(statusMessage));
  return {true, true,
          "creative_editor_world_layout_vertical_connector_settings_updated"};
}

double pointDistance(CreativeEditorWorldLayoutPoint lhs,
                     CreativeEditorWorldLayoutPoint rhs) noexcept {
  return std::hypot(lhs.x - rhs.x, lhs.z - rhs.z);
}

CreativeEditorWorldLayoutVerticalConnectorTarget connectorTargetAt(
    const CreativeEditorWorldLayoutState& state, std::size_t connectorIndex,
    CreativeEditorWorldLayoutPoint point, double toleranceCells,
    bool includeDirectionHandle) noexcept {
  if (!creativeEditorWorldLayoutVerticalConnectorOnActiveLevel(
          state, state.source, connectorIndex)) {
    return {};
  }
  const cr::CreativeWorldLayoutVerticalConnector& connector =
      state.source.verticalConnectors[connectorIndex];
  CreativeEditorWorldLayoutPoint directionHandle;
  if (includeDirectionHandle &&
      resolveCreativeEditorWorldLayoutVerticalConnectorDirectionHandle(
          connector.footprint, connector.direction, directionHandle) &&
      pointDistance(point, directionHandle) <= toleranceCells) {
    return {connectorIndex, CreativeEditorWorldLayoutRectHandle::None, true};
  }
  const CreativeEditorWorldLayoutRectHandle handle =
      detail::worldLayoutRectHandleAt(connector.footprint, point,
                                      toleranceCells);
  return handle == CreativeEditorWorldLayoutRectHandle::None
             ? CreativeEditorWorldLayoutVerticalConnectorTarget{}
             : CreativeEditorWorldLayoutVerticalConnectorTarget{
                   connectorIndex, handle, false};
}

cr::CreativeWorldLayoutVerticalDirection directionToward(
    cr::CreativeWorldLayoutRect footprint,
    CreativeEditorWorldLayoutPoint point,
    cr::CreativeWorldLayoutVerticalDirection fallback) noexcept {
  const double centerX =
      (static_cast<double>(footprint.minimum.x) + footprint.maximum.x) * 0.5;
  const double centerZ =
      (static_cast<double>(footprint.minimum.z) + footprint.maximum.z) * 0.5;
  const double deltaX = point.x - centerX;
  const double deltaZ = point.z - centerZ;
  if (!std::isfinite(deltaX) || !std::isfinite(deltaZ) ||
      (deltaX == 0.0 && deltaZ == 0.0)) {
    return fallback;
  }
  if (std::fabs(deltaX) >= std::fabs(deltaZ)) {
    return deltaX >= 0.0
               ? cr::CreativeWorldLayoutVerticalDirection::PositiveX
               : cr::CreativeWorldLayoutVerticalDirection::NegativeX;
  }
  return deltaZ >= 0.0
             ? cr::CreativeWorldLayoutVerticalDirection::PositiveZ
             : cr::CreativeWorldLayoutVerticalDirection::NegativeZ;
}

}  // namespace

bool creativeEditorWorldLayoutVerticalConnectorOnActiveLevel(
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeWorldLayout& source,
    std::size_t connectorIndex) noexcept {
  if (connectorIndex >= source.verticalConnectors.size()) {
    return false;
  }
  if (state.activeLevelIndex >= source.levels.size()) {
    return true;
  }
  const cr::CreativeWorldLayoutVerticalConnector& connector =
      source.verticalConnectors[connectorIndex];
  return (connector.lowerRoomIndex < source.rooms.size() &&
          source.rooms[connector.lowerRoomIndex].levelIndex ==
              state.activeLevelIndex) ||
         (connector.upperRoomIndex < source.rooms.size() &&
          source.rooms[connector.upperRoomIndex].levelIndex ==
              state.activeLevelIndex);
}

bool resolveCreativeEditorWorldLayoutVerticalConnectorAxis(
    cr::CreativeWorldLayoutRect footprint,
    cr::CreativeWorldLayoutVerticalDirection direction,
    CreativeEditorWorldLayoutPoint& low,
    CreativeEditorWorldLayoutPoint& high) noexcept {
  if (footprint.minimum.x >= footprint.maximum.x ||
      footprint.minimum.z >= footprint.maximum.z ||
      direction >= cr::CreativeWorldLayoutVerticalDirection::Count) {
    return false;
  }
  const double centerX =
      (static_cast<double>(footprint.minimum.x) + footprint.maximum.x) * 0.5;
  const double centerZ =
      (static_cast<double>(footprint.minimum.z) + footprint.maximum.z) * 0.5;
  low = {centerX, centerZ};
  high = low;
  switch (direction) {
    case cr::CreativeWorldLayoutVerticalDirection::PositiveX:
      low.x = footprint.minimum.x;
      high.x = footprint.maximum.x;
      return true;
    case cr::CreativeWorldLayoutVerticalDirection::NegativeX:
      low.x = footprint.maximum.x;
      high.x = footprint.minimum.x;
      return true;
    case cr::CreativeWorldLayoutVerticalDirection::PositiveZ:
      low.z = footprint.minimum.z;
      high.z = footprint.maximum.z;
      return true;
    case cr::CreativeWorldLayoutVerticalDirection::NegativeZ:
      low.z = footprint.maximum.z;
      high.z = footprint.minimum.z;
      return true;
    case cr::CreativeWorldLayoutVerticalDirection::Count:
      break;
  }
  return false;
}

bool resolveCreativeEditorWorldLayoutVerticalConnectorDirectionHandle(
    cr::CreativeWorldLayoutRect footprint,
    cr::CreativeWorldLayoutVerticalDirection direction,
    CreativeEditorWorldLayoutPoint& output) noexcept {
  CreativeEditorWorldLayoutPoint low;
  CreativeEditorWorldLayoutPoint high;
  if (!resolveCreativeEditorWorldLayoutVerticalConnectorAxis(
          footprint, direction, low, high)) {
    return false;
  }
  const double length = std::hypot(high.x - low.x, high.z - low.z);
  if (!std::isfinite(length) || length <= 0.0) {
    return false;
  }
  output = {
      high.x + (high.x - low.x) / length *
                   kCreativeEditorWorldLayoutVerticalConnectorDirectionHandleOffsetCells,
      high.z + (high.z - low.z) / length *
                   kCreativeEditorWorldLayoutVerticalConnectorDirectionHandleOffsetCells,
  };
  return true;
}

bool readCreativeEditorWorldLayoutVerticalConnectorSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t connectorIndex,
    CreativeEditorWorldLayoutVerticalConnectorSettings& output) noexcept {
  if (connectorIndex >= state.source.verticalConnectors.size()) {
    return false;
  }
  output = connectorSettings(state.source.verticalConnectors[connectorIndex]);
  return true;
}

CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutVerticalConnectorSettings(
    CreativeEditorWorldLayoutState& state, std::size_t connectorIndex,
    CreativeEditorWorldLayoutVerticalConnectorSettings settings,
    cr::CreativeGridSettings grid) {
  return commitConnectorSettings(state, connectorIndex, settings, grid,
                                 "vertical connector settings updated");
}

CreativeEditorWorldLayoutVerticalConnectorTarget
findCreativeEditorWorldLayoutVerticalConnectorTarget(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) noexcept {
  if (!detail::finiteWorldLayoutPoint(point) ||
      !std::isfinite(toleranceCells) || toleranceCells <= 0.0) {
    return {};
  }
  if (state.selection.kind ==
          CreativeEditorWorldLayoutSelectionKind::VerticalConnector &&
      state.selection.index < state.source.verticalConnectors.size()) {
    const CreativeEditorWorldLayoutVerticalConnectorTarget selected =
        connectorTargetAt(state, state.selection.index, point, toleranceCells,
                          true);
    if (selected.directionHandle ||
        selected.handle != CreativeEditorWorldLayoutRectHandle::None) {
      return selected;
    }
  }
  for (std::size_t index = state.source.verticalConnectors.size(); index > 0U;
       --index) {
    const CreativeEditorWorldLayoutVerticalConnectorTarget target =
        connectorTargetAt(state, index - 1U, point, toleranceCells, false);
    if (target.handle != CreativeEditorWorldLayoutRectHandle::None) {
      return target;
    }
  }
  return {};
}

CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutVerticalConnectorManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutVerticalConnectorManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point, double toleranceCells,
    cr::CreativeGridSettings grid,
    CreativeEditorWorldLayoutVerticalConnectorTarget requestedTarget) {
  if (phase >=
      CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::Count) {
    return {
        false, false,
        "creative_editor_world_layout_vertical_connector_manipulation_phase_invalid"};
  }
  if (phase ==
      CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::Cancel) {
    const bool changed = state.verticalConnectorManipulation.active;
    state.verticalConnectorManipulation = {};
    state.statusMessage = "vertical connector manipulation cancelled";
    return {
        true, changed,
        "creative_editor_world_layout_vertical_connector_manipulation_cancelled"};
  }
  if (state.tool != CreativeEditorWorldLayoutTool::Select) {
    return {
        false, false,
        "creative_editor_world_layout_vertical_connector_manipulation_tool_invalid"};
  }
  if (phase ==
      CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::Begin) {
    const bool explicitTarget =
        requestedTarget.directionHandle ||
        requestedTarget.handle != CreativeEditorWorldLayoutRectHandle::None;
    const CreativeEditorWorldLayoutVerticalConnectorTarget target =
        explicitTarget
            ? requestedTarget
            : findCreativeEditorWorldLayoutVerticalConnectorTarget(
                  state, point, toleranceCells);
    if ((!target.directionHandle &&
         target.handle == CreativeEditorWorldLayoutRectHandle::None) ||
        target.connectorIndex >= state.source.verticalConnectors.size()) {
      return {
          false, false,
          "creative_editor_world_layout_vertical_connector_manipulation_target_missing"};
    }
    const cr::CreativeWorldLayoutVerticalConnector& connector =
        state.source.verticalConnectors[target.connectorIndex];
    detail::clearWorldLayoutInteraction(state);
    state.selection = {
        CreativeEditorWorldLayoutSelectionKind::VerticalConnector,
        target.connectorIndex};
    state.anchorActive = false;
    state.verticalConnectorManipulation = {
        true,
        state.revision,
        target,
        point,
        connector.footprint,
        connector.kind,
        connector.direction,
        connector.footprint,
        connector.kind,
        connector.direction,
        true,
        "creative_editor_world_layout_vertical_connector_manipulation_ready",
    };
    state.statusMessage = target.directionHandle
                              ? "drag to set connector rise direction"
                          : target.handle ==
                                    CreativeEditorWorldLayoutRectHandle::Move
                              ? "drag to move vertical connector"
                              : "drag to resize vertical connector";
    return {
        true, true,
        "creative_editor_world_layout_vertical_connector_manipulation_started"};
  }
  if (!state.verticalConnectorManipulation.active ||
      state.verticalConnectorManipulation.target.connectorIndex >=
          state.source.verticalConnectors.size()) {
    return {
        false, false,
        "creative_editor_world_layout_vertical_connector_manipulation_not_active"};
  }
  const std::size_t connectorIndex =
      state.verticalConnectorManipulation.target.connectorIndex;
  const cr::CreativeWorldLayoutVerticalConnector& connector =
      state.source.verticalConnectors[connectorIndex];
  if (state.revision !=
          state.verticalConnectorManipulation.sourceRevision ||
      !sameRect(connector.footprint,
                state.verticalConnectorManipulation.originalFootprint) ||
      connector.kind != state.verticalConnectorManipulation.originalKind ||
      connector.direction !=
          state.verticalConnectorManipulation.originalDirection) {
    state.verticalConnectorManipulation = {};
    state.statusMessage = "vertical connector changed while drag was active";
    return {
        false, false,
        "creative_editor_world_layout_vertical_connector_manipulation_stale"};
  }
  if (phase ==
      CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::Update) {
    CreativeEditorWorldLayoutVerticalConnectorSettings settings =
        connectorSettings(connector);
    bool coordinateValid = detail::finiteWorldLayoutPoint(point);
    if (coordinateValid &&
        state.verticalConnectorManipulation.target.directionHandle) {
      settings.direction = directionToward(
          settings.footprint, point, settings.direction);
    } else if (coordinateValid) {
      coordinateValid = detail::worldLayoutManipulatedRect(
          state.verticalConnectorManipulation, point, settings.footprint);
    }
    if (coordinateValid &&
        sameSettings(
            settings,
            {state.verticalConnectorManipulation.previewFootprint,
             state.verticalConnectorManipulation.previewKind,
             state.verticalConnectorManipulation.previewDirection,
             connector.material})) {
      return {true, false,
              state.verticalConnectorManipulation.reasonCode};
    }
    ConnectorValidation validation;
    if (coordinateValid) {
      validation =
          validateConnectorSettings(state, connectorIndex, settings, grid);
    } else {
      validation.reasonCode =
          "creative_editor_world_layout_vertical_connector_manipulation_out_of_range";
      validation.message =
          "vertical connector drag exceeds the layout coordinate range";
    }
    state.verticalConnectorManipulation.previewFootprint =
        settings.footprint;
    state.verticalConnectorManipulation.previewKind = settings.kind;
    state.verticalConnectorManipulation.previewDirection =
        settings.direction;
    state.verticalConnectorManipulation.previewValid = validation.accepted;
    state.verticalConnectorManipulation.reasonCode = validation.reasonCode;
    state.statusMessage = validation.accepted
                              ? "vertical connector drag preview"
                              : std::string(validation.message);
    return {true, true, std::string(validation.reasonCode)};
  }

  const CreativeEditorWorldLayoutEditReceipt updated =
      applyCreativeEditorWorldLayoutVerticalConnectorManipulation(
          state,
          CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::Update,
          point, toleranceCells, grid, {});
  if (!updated.accepted) {
    return updated;
  }
  if (!state.verticalConnectorManipulation.previewValid) {
    const std::string reasonCode =
        state.verticalConnectorManipulation.reasonCode;
    state.verticalConnectorManipulation = {};
    return {false, false, reasonCode};
  }
  const CreativeEditorWorldLayoutVerticalConnectorSettings settings = {
      state.verticalConnectorManipulation.previewFootprint,
      state.verticalConnectorManipulation.previewKind,
      state.verticalConnectorManipulation.previewDirection,
      connector.material,
  };
  state.verticalConnectorManipulation = {};
  return commitConnectorSettings(state, connectorIndex, settings, grid,
                                 "vertical connector updated");
}

}  // namespace iggy3d_creative_app
