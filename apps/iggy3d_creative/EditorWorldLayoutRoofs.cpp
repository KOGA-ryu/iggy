#include "EditorWorldLayoutRoofs.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <string>
#include <utility>

#include "app/iggy3d/creative/render/CreativeScreenProjection.hpp"
#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"

namespace iggy3d_creative_app {
namespace {

[[nodiscard]] bool finitePoint(CreativeEditorWorldLayoutPoint point) noexcept {
  return std::isfinite(point.x) && std::isfinite(point.z);
}

[[nodiscard]] bool validGrid(cr::CreativeGridSettings grid) noexcept {
  return std::isfinite(grid.cellSizeMeters) && grid.cellSizeMeters > 0.0 &&
         cr::isFiniteCreativeVec3(grid.origin);
}

[[nodiscard]] std::size_t resolveLevelIndex(
    const CreativeEditorWorldLayoutState& state,
    std::size_t requested) noexcept {
  if (requested < state.source.levels.size()) {
    return requested;
  }
  if (state.selection.kind == CreativeEditorWorldLayoutSelectionKind::Level &&
      state.selection.index < state.source.levels.size()) {
    return state.selection.index;
  }
  return state.activeLevelIndex < state.source.levels.size()
             ? state.activeLevelIndex
             : cr::kInvalidCreativeWorldLayoutIndex;
}

void assignLevelSettings(
    cr::CreativeWorldLayoutLevel& level,
    const CreativeEditorWorldLayoutLevelSettings& settings) {
  level.name = settings.name;
  level.floorTopLayer = settings.floorTopLayer;
  level.wallHeightCells = settings.wallHeightCells;
  level.floorThicknessLayers = settings.floorThicknessLayers;
  level.ceilingThicknessLayers = settings.ceilingThicknessLayers;
  level.roofThicknessLayers = settings.roofThicknessLayers;
  level.roofStyle = settings.roofStyle;
  level.roofRidgeAxis = settings.roofRidgeAxis;
  level.roofPitchDegrees = settings.roofPitchDegrees;
  level.roofOverhangCells = settings.roofOverhangCells;
  level.roofSlopeDirection = settings.roofSlopeDirection;
  level.roofMaterial = settings.roofMaterial;
}

[[nodiscard]] iggy3d::Vec3 toCore(cr::CreativeVec3 value) noexcept {
  return {static_cast<float>(value.x), static_cast<float>(value.y),
          static_cast<float>(value.z)};
}

[[nodiscard]] CreativeEditorWorldLayoutPoint toPlan(
    cr::CreativeVec3 world,
    cr::CreativeGridSettings grid) noexcept {
  return {(world.x - grid.origin.x) / grid.cellSizeMeters,
          (world.z - grid.origin.z) / grid.cellSizeMeters};
}

[[nodiscard]] CreativeEditorWorldLayoutRoofHandleKind handleKind(
    cr::CreativeStructuralRoofPerimeterEdge edge) noexcept {
  switch (edge) {
    case cr::CreativeStructuralRoofPerimeterEdge::North:
      return CreativeEditorWorldLayoutRoofHandleKind::NorthEave;
    case cr::CreativeStructuralRoofPerimeterEdge::East:
      return CreativeEditorWorldLayoutRoofHandleKind::EastEave;
    case cr::CreativeStructuralRoofPerimeterEdge::South:
      return CreativeEditorWorldLayoutRoofHandleKind::SouthEave;
    case cr::CreativeStructuralRoofPerimeterEdge::West:
      return CreativeEditorWorldLayoutRoofHandleKind::WestEave;
    case cr::CreativeStructuralRoofPerimeterEdge::Count:
      break;
  }
  return CreativeEditorWorldLayoutRoofHandleKind::None;
}

[[nodiscard]] bool validTarget(
    const cr::CreativeWorldLayout& layout,
    CreativeEditorWorldLayoutRoofTarget target) noexcept {
  return target.levelIndex < layout.levels.size() &&
         target.handle > CreativeEditorWorldLayoutRoofHandleKind::None &&
         target.handle < CreativeEditorWorldLayoutRoofHandleKind::Count;
}

[[nodiscard]] bool validTarget(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutRoofTarget target) noexcept {
  return validTarget(state.source, target);
}

[[nodiscard]] CreativeEditorWorldLayoutLevelSettings levelSettings(
    const cr::CreativeWorldLayoutLevel& level) {
  return {level.name,
          level.floorTopLayer,
          level.wallHeightCells,
          level.floorThicknessLayers,
          level.ceilingThicknessLayers,
          level.roofThicknessLayers,
          level.roofStyle,
          level.roofRidgeAxis,
          level.roofPitchDegrees,
          level.roofOverhangCells,
          level.roofSlopeDirection,
          level.roofMaterial};
}

[[nodiscard]] double snapDelta(double delta) noexcept {
  if (!std::isfinite(delta)) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return std::round(delta / kCreativeEditorWorldLayoutRoofHandleSnapCells) *
         kCreativeEditorWorldLayoutRoofHandleSnapCells;
}

[[nodiscard]] double roofRunMeters(
    const cr::CreativeWorldLayoutRoofPlan& roof,
    double pitchDegrees) noexcept {
  const double tangent = std::tan(pitchDegrees * std::numbers::pi / 180.0);
  if (!std::isfinite(tangent) || tangent <= 0.0 ||
      !std::isfinite(roof.geometry.riseMeters) ||
      roof.geometry.riseMeters <= 0.0) {
    return 0.0;
  }
  return roof.geometry.riseMeters / tangent;
}

void resetRoofManipulation(CreativeEditorWorldLayoutState& state) {
  state.roofManipulation = {};
}

[[nodiscard]] CreativeEditorWorldLayoutEditReceipt rejectEdit(
    CreativeEditorWorldLayoutState& state,
    std::string_view message,
    std::string_view reasonCode) {
  state.statusMessage = std::string(message);
  return {false, false, std::string(reasonCode)};
}

}  // namespace

CreativeEditorWorldLayoutRoofHandleFrame
buildCreativeEditorWorldLayoutRoofHandleFrame(
    const CreativeEditorWorldLayoutState& state,
    cr::CreativeGridSettings grid,
    std::size_t levelIndex,
    bool includeManipulationPreview) {
  CreativeEditorWorldLayoutRoofHandleFrame frame;
  const std::size_t resolved = resolveLevelIndex(state, levelIndex);
  if (!validGrid(grid) || resolved >= state.source.levels.size() ||
      state.generatedRevision != state.revision ||
      !cr::creativeWorldLayoutLevelIsTopmostOccupied(state.source, resolved)) {
    frame.reasonCode = "creative_editor_world_layout_roof_handles_invalid";
    return frame;
  }

  const cr::CreativeWorldLayout* layout = &state.source;
  cr::CreativeWorldLayout previewLayout;
  if (includeManipulationPreview && state.roofManipulation.active &&
      state.roofManipulation.target.levelIndex == resolved &&
      state.roofManipulation.previewValid) {
    previewLayout = state.source;
    assignLevelSettings(previewLayout.levels[resolved],
                        state.roofManipulation.previewSettings);
    layout = &previewLayout;
  }
  frame.roof = cr::planCreativeWorldLayoutRoof(grid, *layout, resolved);
  if (!frame.roof.accepted) {
    frame.reasonCode = frame.roof.reasonCode;
    return frame;
  }

  for (std::size_t index = 0U;
       index < frame.roof.geometry.edgeCount &&
       frame.handleCount < frame.handles.size();
       ++index) {
    const cr::CreativeStructuralRoofEdgePlan& edge =
        frame.roof.geometry.edges[index];
    const CreativeEditorWorldLayoutRoofHandleKind kind =
        handleKind(edge.edge);
    if (kind == CreativeEditorWorldLayoutRoofHandleKind::None) {
      continue;
    }
    const cr::CreativeVec3 midpoint{
        (edge.startMeters.x + edge.endMeters.x) * 0.5,
        (edge.startMeters.y + edge.endMeters.y) * 0.5,
        (edge.startMeters.z + edge.endMeters.z) * 0.5};
    CreativeEditorWorldLayoutRoofHandle& handle =
        frame.handles[frame.handleCount++];
    handle.target = {resolved, kind};
    handle.planPosition = toPlan(midpoint, grid);
    handle.worldPosition = toCore(midpoint);
    handle.worldAxis = toCore(edge.outward);
    handle.valid = finitePoint(handle.planPosition);
  }

  if (layout->levels[resolved].roofStyle !=
          cr::CreativeStructuralRoofStyle::Flat &&
      frame.handleCount < frame.handles.size()) {
    const cr::CreativeVec3 midpoint{
        (frame.roof.geometry.ridgeStart.x +
         frame.roof.geometry.ridgeEnd.x) * 0.5,
        (frame.roof.geometry.ridgeStart.y +
         frame.roof.geometry.ridgeEnd.y) * 0.5,
        (frame.roof.geometry.ridgeStart.z +
         frame.roof.geometry.ridgeEnd.z) * 0.5};
    CreativeEditorWorldLayoutRoofHandle& handle =
        frame.handles[frame.handleCount++];
    handle.target = {resolved,
                     CreativeEditorWorldLayoutRoofHandleKind::RidgeHeight};
    handle.planPosition = toPlan(midpoint, grid);
    handle.worldPosition = toCore(midpoint);
    handle.worldAxis = {0.0F, 1.0F, 0.0F};
    handle.valid = finitePoint(handle.planPosition);
  }

  frame.accepted = frame.handleCount >= 4U;
  frame.reasonCode = frame.accepted
                         ? "creative_editor_world_layout_roof_handles_ready"
                         : "creative_editor_world_layout_roof_handles_incomplete";
  return frame;
}

CreativeEditorWorldLayoutRoofTarget
findCreativeEditorWorldLayoutPlanRoofHandle(
    const CreativeEditorWorldLayoutRoofHandleFrame& frame,
    CreativeEditorWorldLayoutPoint point,
    double toleranceCells) noexcept {
  CreativeEditorWorldLayoutRoofTarget result;
  if (!frame.accepted || !finitePoint(point) ||
      !std::isfinite(toleranceCells) || toleranceCells < 0.0) {
    return result;
  }
  const double maximumDistanceSquared = toleranceCells * toleranceCells;
  double nearestDistanceSquared = maximumDistanceSquared;
  for (std::size_t index = 0U; index < frame.handleCount; ++index) {
    const CreativeEditorWorldLayoutRoofHandle& handle = frame.handles[index];
    if (!handle.valid ||
        handle.target.handle ==
            CreativeEditorWorldLayoutRoofHandleKind::RidgeHeight) {
      continue;
    }
    const double dx = point.x - handle.planPosition.x;
    const double dz = point.z - handle.planPosition.z;
    const double distanceSquared = dx * dx + dz * dz;
    if (distanceSquared <= nearestDistanceSquared) {
      nearestDistanceSquared = distanceSquared;
      result = handle.target;
    }
  }
  return result;
}

CreativeEditorWorldLayoutRoofHandlePick
pickCreativeEditorWorldLayoutRoofHandleAtPixel(
    const CreativeEditorWorldLayoutRoofHandleFrame& frame,
    const iggy3d::RenderCameraFrame& camera,
    iggy3d::RenderContentViewport viewport,
    float pixelX,
    float pixelY,
    float tolerancePixels) noexcept {
  CreativeEditorWorldLayoutRoofHandlePick result;
  if (!frame.accepted || viewport.width == 0U || viewport.height == 0U ||
      !std::isfinite(pixelX) || !std::isfinite(pixelY) ||
      !std::isfinite(tolerancePixels) || tolerancePixels < 0.0F) {
    return result;
  }
  const float maximumDistanceSquared = tolerancePixels * tolerancePixels;
  float nearestDistanceSquared = maximumDistanceSquared;
  for (std::size_t index = 0U; index < frame.handleCount; ++index) {
    const CreativeEditorWorldLayoutRoofHandle& handle = frame.handles[index];
    if (!handle.valid) {
      continue;
    }
    const cr::CreativeScreenPoint projected =
        cr::projectCreativeWorldPointToScreen(
            camera.clipFromWorld, handle.worldPosition, viewport.width,
            viewport.height);
    if (!projected.valid || !projected.insideViewport) {
      continue;
    }
    const float screenX = static_cast<float>(viewport.x) + projected.x;
    const float screenY = static_cast<float>(viewport.y) + projected.y;
    const float dx = pixelX - screenX;
    const float dy = pixelY - screenY;
    const float distanceSquared = dx * dx + dy * dy;
    if (distanceSquared <= nearestDistanceSquared) {
      nearestDistanceSquared = distanceSquared;
      result.hit = true;
      result.handleIndex = index;
      result.distanceSquaredPixels = distanceSquared;
    }
  }
  return result;
}

CreativeEditorWorldLayoutRoofEditPlan
planCreativeEditorWorldLayoutRoofEdit(
    const cr::CreativeWorldLayout& layout,
    cr::CreativeGridSettings grid,
    CreativeEditorWorldLayoutRoofTarget target,
    double deltaCells) {
  CreativeEditorWorldLayoutRoofEditPlan result;
  result.target = target;
  if (!validGrid(grid) || !validTarget(layout, target)) {
    result.reasonCode = "creative_editor_world_layout_roof_edit_target_invalid";
    return result;
  }
  result.snappedDeltaCells = snapDelta(deltaCells);
  if (!std::isfinite(result.snappedDeltaCells)) {
    result.reasonCode = "creative_editor_world_layout_roof_edit_input_invalid";
    return result;
  }
  result.settings = levelSettings(layout.levels[target.levelIndex]);

  const cr::CreativeWorldLayoutRoofPlan original =
      cr::planCreativeWorldLayoutRoof(grid, layout, target.levelIndex);
  if (!original.accepted) {
    result.reasonCode = original.reasonCode;
    return result;
  }

  if (target.handle ==
      CreativeEditorWorldLayoutRoofHandleKind::RidgeHeight) {
    if (result.settings.roofStyle == cr::CreativeStructuralRoofStyle::Flat) {
      result.reasonCode =
          "creative_editor_world_layout_roof_ridge_flat_unsupported";
      return result;
    }
    const double runMeters =
        roofRunMeters(original, result.settings.roofPitchDegrees);
    const double riseMeters =
        original.geometry.riseMeters +
        result.snappedDeltaCells * grid.cellSizeMeters;
    if (!std::isfinite(runMeters) || runMeters <= 0.0 ||
        !std::isfinite(riseMeters)) {
      result.reasonCode =
          "creative_editor_world_layout_roof_ridge_unrepresentable";
      return result;
    }
    const double pitchDegrees =
        std::atan2(riseMeters, runMeters) * 180.0 / std::numbers::pi;
    result.settings.roofPitchDegrees =
        std::clamp(pitchDegrees, 5.0, 75.0);
  } else {
    result.settings.roofOverhangCells = std::clamp(
        result.settings.roofOverhangCells + result.snappedDeltaCells, 0.0,
        cr::kMaximumCreativeWorldLayoutRoofOverhangCells);
  }

  cr::CreativeWorldLayout candidate = layout;
  assignLevelSettings(candidate.levels[target.levelIndex], result.settings);
  result.roof =
      cr::planCreativeWorldLayoutRoof(grid, candidate, target.levelIndex);
  if (!result.roof.accepted) {
    result.reasonCode = result.roof.reasonCode;
    return result;
  }
  result.accepted = true;
  result.reasonCode = "creative_editor_world_layout_roof_edit_ready";
  return result;
}

CreativeEditorWorldLayoutRoofEditPlan
planCreativeEditorWorldLayoutRoofEdit(
    const CreativeEditorWorldLayoutState& state,
    cr::CreativeGridSettings grid,
    CreativeEditorWorldLayoutRoofTarget target,
    double deltaCells) {
  return planCreativeEditorWorldLayoutRoofEdit(state.source, grid, target,
                                               deltaCells);
}

CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutRoofManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutRoofManipulationPhase phase,
    CreativeEditorWorldLayoutRoofTarget target,
    double coordinateCells,
    cr::CreativeGridSettings grid) {
  if (phase >= CreativeEditorWorldLayoutRoofManipulationPhase::Count ||
      !validGrid(grid)) {
    return rejectEdit(
        state, "roof manipulation input is invalid",
        "creative_editor_world_layout_roof_manipulation_input_invalid");
  }

  if (phase == CreativeEditorWorldLayoutRoofManipulationPhase::Begin) {
    if (state.roofManipulation.active ||
        state.tool != CreativeEditorWorldLayoutTool::Select ||
        state.generatedRevision != state.revision ||
        !validTarget(state, target) || !std::isfinite(coordinateCells) ||
        !cr::creativeWorldLayoutLevelIsTopmostOccupied(state.source,
                                                       target.levelIndex)) {
      return rejectEdit(
          state, "roof handle is not available",
          "creative_editor_world_layout_roof_manipulation_begin_rejected");
    }
    CreativeEditorWorldLayoutLevelSettings settings;
    const cr::CreativeWorldLayoutRoofPlan roof =
        cr::planCreativeWorldLayoutRoof(grid, state.source, target.levelIndex);
    if (!roof.accepted ||
        !readCreativeEditorWorldLayoutLevelSettings(
            state, target.levelIndex, settings) ||
        (target.handle ==
             CreativeEditorWorldLayoutRoofHandleKind::RidgeHeight &&
         settings.roofStyle == cr::CreativeStructuralRoofStyle::Flat)) {
      return rejectEdit(
          state, "roof handle is not available",
          "creative_editor_world_layout_roof_manipulation_target_rejected");
    }
    state.roofManipulation.active = true;
    state.roofManipulation.sourceRevision = state.revision;
    state.roofManipulation.target = target;
    state.roofManipulation.startCoordinateCells = coordinateCells;
    state.roofManipulation.previewDeltaCells = 0.0;
    state.roofManipulation.originalSettings = settings;
    state.roofManipulation.previewSettings = settings;
    state.roofManipulation.previewValid = true;
    state.roofManipulation.reasonCode =
        "creative_editor_world_layout_roof_manipulation_started";
    state.activeLevelIndex = target.levelIndex;
    state.selection = {CreativeEditorWorldLayoutSelectionKind::Level,
                       target.levelIndex};
    state.statusMessage = "drag roof handle";
    return {true, false,
            "creative_editor_world_layout_roof_manipulation_started"};
  }

  if (phase == CreativeEditorWorldLayoutRoofManipulationPhase::Cancel) {
    const bool wasActive = state.roofManipulation.active;
    resetRoofManipulation(state);
    state.statusMessage = wasActive ? "roof edit cancelled"
                                    : "no roof edit is active";
    return {true, wasActive,
            wasActive
                ? "creative_editor_world_layout_roof_manipulation_cancelled"
                : "creative_editor_world_layout_roof_manipulation_inactive"};
  }

  if (!state.roofManipulation.active ||
      state.roofManipulation.sourceRevision != state.revision ||
      !std::isfinite(coordinateCells)) {
    resetRoofManipulation(state);
    return rejectEdit(
        state, "roof edit became stale",
        "creative_editor_world_layout_roof_manipulation_stale");
  }

  if (phase == CreativeEditorWorldLayoutRoofManipulationPhase::Update) {
    const double deltaCells =
        coordinateCells - state.roofManipulation.startCoordinateCells;
    const CreativeEditorWorldLayoutRoofEditPlan preview =
        planCreativeEditorWorldLayoutRoofEdit(
            state, grid, state.roofManipulation.target, deltaCells);
    const bool changed =
        state.roofManipulation.previewValid != preview.accepted ||
        state.roofManipulation.previewDeltaCells != preview.snappedDeltaCells ||
        state.roofManipulation.previewSettings != preview.settings;
    state.roofManipulation.previewDeltaCells = preview.snappedDeltaCells;
    state.roofManipulation.previewSettings = preview.settings;
    state.roofManipulation.previewValid = preview.accepted;
    state.roofManipulation.reasonCode = std::string(preview.reasonCode);
    state.statusMessage = preview.accepted ? "roof edit preview ready"
                                           : std::string(preview.reasonCode);
    return {preview.accepted, changed, std::string(preview.reasonCode)};
  }

  if (!state.roofManipulation.previewValid) {
    const std::string reason = state.roofManipulation.reasonCode;
    resetRoofManipulation(state);
    return rejectEdit(state, "roof edit preview is invalid", reason);
  }
  const std::size_t levelIndex = state.roofManipulation.target.levelIndex;
  const CreativeEditorWorldLayoutLevelSettings settings =
      state.roofManipulation.previewSettings;
  resetRoofManipulation(state);
  return setCreativeEditorWorldLayoutLevelSettings(state, levelIndex,
                                                   settings);
}

CreativeEditorWorldLayoutRoofLiveEditReceipt
applyCreativeEditorWorldLayoutRoofManipulationToDocument(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeAppState& appState,
    CreativeEditorWorldLayoutRoofManipulationPhase phase,
    CreativeEditorWorldLayoutRoofTarget target,
    double coordinateCells) {
  CreativeEditorWorldLayoutRoofLiveEditReceipt result;
  const cr::CreativeGridSettings grid =
      appState.facade.document().gridSettings();

  if (phase == CreativeEditorWorldLayoutRoofManipulationPhase::Begin) {
    result.sceneChanged =
        clearCreativeEditorWorldLayoutLiveEditPreview(state);
    const CreativeEditorWorldLayoutEditReceipt edit =
        applyCreativeEditorWorldLayoutRoofManipulation(
            state, phase, target, coordinateCells, grid);
    result.accepted = edit.accepted;
    result.changed = edit.changed;
    result.reasonCode = edit.reasonCode;
    return result;
  }

  if (phase == CreativeEditorWorldLayoutRoofManipulationPhase::Cancel) {
    const CreativeEditorWorldLayoutEditReceipt edit =
        applyCreativeEditorWorldLayoutRoofManipulation(
            state, phase, target, coordinateCells, grid);
    result.accepted = edit.accepted;
    result.changed = edit.changed;
    result.sceneChanged =
        clearCreativeEditorWorldLayoutLiveEditPreview(state);
    result.reasonCode = edit.reasonCode;
    return result;
  }

  if (phase == CreativeEditorWorldLayoutRoofManipulationPhase::Update) {
    const CreativeEditorWorldLayoutEditReceipt edit =
        applyCreativeEditorWorldLayoutRoofManipulation(
            state, phase, target, coordinateCells, grid);
    result.accepted = edit.accepted;
    result.changed = edit.changed;
    result.reasonCode = edit.reasonCode;
    if (!edit.accepted || !state.roofManipulation.previewValid) {
      result.sceneChanged =
          clearCreativeEditorWorldLayoutLiveEditPreview(state);
      return result;
    }
    const CreativeEditorWorldLayoutPreviewReceipt preview =
        previewCreativeEditorWorldLayoutLevelSettings(
            state, appState.facade.document(),
            state.roofManipulation.target.levelIndex,
            state.roofManipulation.previewSettings);
    result.accepted = preview.accepted;
    result.sceneChanged = preview.changed;
    result.reasonCode = preview.reasonCode;
    return result;
  }

  if (phase != CreativeEditorWorldLayoutRoofManipulationPhase::Commit ||
      !state.roofManipulation.active ||
      state.roofManipulation.sourceRevision != state.revision ||
      !state.roofManipulation.previewValid) {
    const CreativeEditorWorldLayoutEditReceipt stale =
        applyCreativeEditorWorldLayoutRoofManipulation(
            state, phase, target, coordinateCells, grid);
    result.accepted = stale.accepted;
    result.changed = stale.changed;
    result.sceneChanged =
        clearCreativeEditorWorldLayoutLiveEditPreview(state);
    result.reasonCode = stale.reasonCode;
    return result;
  }

  const bool previewWasVisible = creativeEditorWorldLayoutPreviewActive(state);
  const std::size_t levelIndex = state.roofManipulation.target.levelIndex;
  const CreativeEditorWorldLayoutLevelSettings settings =
      state.roofManipulation.previewSettings;
  resetRoofManipulation(state);
  const CreativeEditorWorldLayoutApplyReceipt applied =
      applyCreativeEditorWorldLayoutLevelSettingsToDocument(
          state, appState, levelIndex, settings);
  result.accepted = applied.accepted;
  result.changed = applied.changed;
  result.worldLayoutChanged = applied.changed;
  result.sceneChanged = previewWasVisible || applied.apply.changed;
  result.reasonCode = applied.reasonCode;
  return result;
}

}  // namespace iggy3d_creative_app
