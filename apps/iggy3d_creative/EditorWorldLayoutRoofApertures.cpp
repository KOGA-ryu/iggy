#include "EditorWorldLayoutBuildings.hpp"

#include "EditorWorldLayoutInternal.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

namespace iggy3d_creative_app {
namespace {

constexpr double kRoofApertureSnapCells = 0.25;

CreativeEditorWorldLayoutRoofApertureHandle apertureHandleAt(
    const cr::CreativeWorldLayoutRoofAperture& aperture,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) noexcept {
  if (!detail::finiteWorldLayoutPoint(point) ||
      !std::isfinite(toleranceCells) || toleranceCells <= 0.0 ||
      point.x < aperture.minimumXCells - toleranceCells ||
      point.x > aperture.maximumXCells + toleranceCells ||
      point.z < aperture.minimumZCells - toleranceCells ||
      point.z > aperture.maximumZCells + toleranceCells) {
    return CreativeEditorWorldLayoutRoofApertureHandle::None;
  }
  const bool north =
      std::fabs(point.z - aperture.minimumZCells) <= toleranceCells;
  const bool east =
      std::fabs(point.x - aperture.maximumXCells) <= toleranceCells;
  const bool south =
      std::fabs(point.z - aperture.maximumZCells) <= toleranceCells;
  const bool west =
      std::fabs(point.x - aperture.minimumXCells) <= toleranceCells;
  if (north && west) {
    return CreativeEditorWorldLayoutRoofApertureHandle::NorthWest;
  }
  if (north && east) {
    return CreativeEditorWorldLayoutRoofApertureHandle::NorthEast;
  }
  if (south && east) {
    return CreativeEditorWorldLayoutRoofApertureHandle::SouthEast;
  }
  if (south && west) {
    return CreativeEditorWorldLayoutRoofApertureHandle::SouthWest;
  }
  if (north) {
    return CreativeEditorWorldLayoutRoofApertureHandle::North;
  }
  if (east) {
    return CreativeEditorWorldLayoutRoofApertureHandle::East;
  }
  if (south) {
    return CreativeEditorWorldLayoutRoofApertureHandle::South;
  }
  if (west) {
    return CreativeEditorWorldLayoutRoofApertureHandle::West;
  }
  return CreativeEditorWorldLayoutRoofApertureHandle::Move;
}

CreativeEditorWorldLayoutRoofApertureTarget apertureTargetAt(
    const CreativeEditorWorldLayoutState& state, std::size_t apertureIndex,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) noexcept {
  if (apertureIndex >= state.source.roofApertures.size()) {
    return {};
  }
  const cr::CreativeWorldLayoutRoofAperture& aperture =
      state.source.roofApertures[apertureIndex];
  if (state.activeLevelIndex < state.source.levels.size() &&
      aperture.levelIndex != state.activeLevelIndex) {
    return {};
  }
  const CreativeEditorWorldLayoutRoofApertureHandle handle =
      apertureHandleAt(aperture, point, toleranceCells);
  return handle == CreativeEditorWorldLayoutRoofApertureHandle::None
             ? CreativeEditorWorldLayoutRoofApertureTarget{}
             : CreativeEditorWorldLayoutRoofApertureTarget{apertureIndex,
                                                           handle};
}

bool snappedApertureDelta(double value, double start,
                          double& output) noexcept {
  if (!std::isfinite(value) || !std::isfinite(start)) {
    return false;
  }
  const double scaled = (value - start) / kRoofApertureSnapCells;
  constexpr double kMaximumStep =
      static_cast<double>(std::numeric_limits<std::int32_t>::max());
  if (!std::isfinite(scaled) || std::fabs(scaled) > kMaximumStep) {
    return false;
  }
  output = std::round(scaled) * kRoofApertureSnapCells;
  return std::isfinite(output);
}

bool offsetCoordinate(double value, double delta, double& output) noexcept {
  output = value + delta;
  return std::isfinite(output);
}

bool manipulatedApertureSettings(
    const CreativeEditorWorldLayoutRoofApertureManipulationState& manipulation,
    CreativeEditorWorldLayoutPoint point,
    CreativeEditorWorldLayoutRoofApertureSettings& output) noexcept {
  double deltaX = 0.0;
  double deltaZ = 0.0;
  if (!snappedApertureDelta(point.x, manipulation.startPoint.x, deltaX) ||
      !snappedApertureDelta(point.z, manipulation.startPoint.z, deltaZ)) {
    return false;
  }
  output = manipulation.originalSettings;
  const CreativeEditorWorldLayoutRoofApertureHandle handle =
      manipulation.target.handle;
  if (handle == CreativeEditorWorldLayoutRoofApertureHandle::Move) {
    return offsetCoordinate(output.minimumXCells, deltaX,
                            output.minimumXCells) &&
           offsetCoordinate(output.maximumXCells, deltaX,
                            output.maximumXCells) &&
           offsetCoordinate(output.minimumZCells, deltaZ,
                            output.minimumZCells) &&
           offsetCoordinate(output.maximumZCells, deltaZ,
                            output.maximumZCells);
  }
  if (detail::worldLayoutRectHandleMovesWest(handle) &&
      !offsetCoordinate(output.minimumXCells, deltaX,
                        output.minimumXCells)) {
    return false;
  }
  if (detail::worldLayoutRectHandleMovesEast(handle) &&
      !offsetCoordinate(output.maximumXCells, deltaX,
                        output.maximumXCells)) {
    return false;
  }
  if (detail::worldLayoutRectHandleMovesNorth(handle) &&
      !offsetCoordinate(output.minimumZCells, deltaZ,
                        output.minimumZCells)) {
    return false;
  }
  return !detail::worldLayoutRectHandleMovesSouth(handle) ||
         offsetCoordinate(output.maximumZCells, deltaZ,
                          output.maximumZCells);
}

bool previewSettingsAccepted(
    const CreativeEditorWorldLayoutState& state, std::size_t apertureIndex,
    const CreativeEditorWorldLayoutRoofApertureSettings& settings,
    cr::CreativeGridSettings grid, std::string& reasonCode,
    std::string& message) {
  CreativeEditorWorldLayoutState candidate;
  candidate.source = state.source;
  candidate.revision = state.revision;
  candidate.generatedRevision = state.generatedRevision;
  candidate.nextStableOrdinal = state.nextStableOrdinal;
  const CreativeEditorWorldLayoutEditReceipt receipt =
      setCreativeEditorWorldLayoutRoofApertureSettings(
          candidate, apertureIndex, settings, grid);
  reasonCode = receipt.reasonCode;
  message = candidate.statusMessage;
  return receipt.accepted;
}

}  // namespace

CreativeEditorWorldLayoutRoofApertureTarget
findCreativeEditorWorldLayoutRoofApertureTarget(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) noexcept {
  if (!detail::finiteWorldLayoutPoint(point) ||
      !std::isfinite(toleranceCells) || toleranceCells <= 0.0) {
    return {};
  }
  if (state.selection.kind ==
          CreativeEditorWorldLayoutSelectionKind::RoofAperture &&
      state.selection.index < state.source.roofApertures.size()) {
    const CreativeEditorWorldLayoutRoofApertureTarget selected =
        apertureTargetAt(state, state.selection.index, point, toleranceCells);
    if (selected.handle !=
        CreativeEditorWorldLayoutRoofApertureHandle::None) {
      return selected;
    }
  }
  for (std::size_t index = state.source.roofApertures.size(); index > 0U;
       --index) {
    const CreativeEditorWorldLayoutRoofApertureTarget target =
        apertureTargetAt(state, index - 1U, point, toleranceCells);
    if (target.handle != CreativeEditorWorldLayoutRoofApertureHandle::None) {
      return target;
    }
  }
  return {};
}

CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutRoofApertureManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutRoofApertureManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point, double toleranceCells,
    cr::CreativeGridSettings grid) {
  if (phase >= CreativeEditorWorldLayoutRoofApertureManipulationPhase::Count) {
    return {
        false, false,
        "creative_editor_world_layout_roof_aperture_manipulation_phase_invalid"};
  }
  if (phase ==
      CreativeEditorWorldLayoutRoofApertureManipulationPhase::Cancel) {
    const bool changed = state.roofApertureManipulation.active;
    state.roofApertureManipulation = {};
    state.statusMessage = "roof aperture manipulation cancelled";
    return {
        true, changed,
        "creative_editor_world_layout_roof_aperture_manipulation_cancelled"};
  }
  if (state.tool != CreativeEditorWorldLayoutTool::Select) {
    return {
        false, false,
        "creative_editor_world_layout_roof_aperture_manipulation_tool_invalid"};
  }
  if (phase == CreativeEditorWorldLayoutRoofApertureManipulationPhase::Begin) {
    const CreativeEditorWorldLayoutRoofApertureTarget target =
        findCreativeEditorWorldLayoutRoofApertureTarget(
            state, point, toleranceCells);
    CreativeEditorWorldLayoutRoofApertureSettings settings;
    if (target.handle == CreativeEditorWorldLayoutRoofApertureHandle::None ||
        !readCreativeEditorWorldLayoutRoofApertureSettings(
            state, target.apertureIndex, settings)) {
      return {
          false, false,
          "creative_editor_world_layout_roof_aperture_manipulation_target_missing"};
    }
    const std::size_t levelIndex =
        state.source.roofApertures[target.apertureIndex].levelIndex;
    detail::clearWorldLayoutInteraction(state);
    state.selection = {CreativeEditorWorldLayoutSelectionKind::RoofAperture,
                       target.apertureIndex};
    state.activeLevelIndex = levelIndex;
    state.anchorActive = false;
    state.roofApertureManipulation = {
        true,
        state.revision,
        target,
        point,
        settings,
        settings,
        true,
        "creative_editor_world_layout_roof_aperture_manipulation_ready",
    };
    state.statusMessage =
        target.handle == CreativeEditorWorldLayoutRoofApertureHandle::Move
            ? "drag to move roof aperture"
            : "drag to resize roof aperture";
    return {
        true, true,
        "creative_editor_world_layout_roof_aperture_manipulation_started"};
  }
  if (!state.roofApertureManipulation.active ||
      state.roofApertureManipulation.target.apertureIndex >=
          state.source.roofApertures.size()) {
    return {
        false, false,
        "creative_editor_world_layout_roof_aperture_manipulation_not_active"};
  }
  const std::size_t apertureIndex =
      state.roofApertureManipulation.target.apertureIndex;
  CreativeEditorWorldLayoutRoofApertureSettings current;
  if (state.revision != state.roofApertureManipulation.sourceRevision ||
      !readCreativeEditorWorldLayoutRoofApertureSettings(
          state, apertureIndex, current) ||
      current != state.roofApertureManipulation.originalSettings) {
    state.roofApertureManipulation = {};
    state.statusMessage = "roof aperture changed while drag was active";
    return {false, false,
            "creative_editor_world_layout_roof_aperture_manipulation_stale"};
  }
  if (phase == CreativeEditorWorldLayoutRoofApertureManipulationPhase::Update) {
    CreativeEditorWorldLayoutRoofApertureSettings settings;
    const bool coordinateValid = manipulatedApertureSettings(
        state.roofApertureManipulation, point, settings);
    if (coordinateValid &&
        settings == state.roofApertureManipulation.previewSettings) {
      return {true, false, state.roofApertureManipulation.reasonCode};
    }
    std::string reasonCode =
        "creative_editor_world_layout_roof_aperture_manipulation_out_of_range";
    std::string message =
        "roof aperture drag exceeds the layout coordinate range";
    const bool accepted = coordinateValid &&
                          previewSettingsAccepted(state, apertureIndex,
                                                  settings, grid, reasonCode,
                                                  message);
    state.roofApertureManipulation.previewSettings = settings;
    state.roofApertureManipulation.previewValid = accepted;
    state.roofApertureManipulation.reasonCode = reasonCode;
    state.statusMessage = accepted ? "roof aperture drag preview" : message;
    return {true, true, reasonCode};
  }

  const CreativeEditorWorldLayoutEditReceipt updated =
      applyCreativeEditorWorldLayoutRoofApertureManipulation(
          state,
          CreativeEditorWorldLayoutRoofApertureManipulationPhase::Update,
          point, toleranceCells, grid);
  if (!updated.accepted) {
    return updated;
  }
  if (!state.roofApertureManipulation.previewValid) {
    const std::string reasonCode =
        state.roofApertureManipulation.reasonCode;
    state.roofApertureManipulation = {};
    return {false, false, reasonCode};
  }
  const CreativeEditorWorldLayoutRoofApertureSettings settings =
      state.roofApertureManipulation.previewSettings;
  state.roofApertureManipulation = {};
  return setCreativeEditorWorldLayoutRoofApertureSettings(
      state, apertureIndex, settings, grid);
}

}  // namespace iggy3d_creative_app
