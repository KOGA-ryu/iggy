#include "EditorWorldLayoutVerticalConnectorHandles.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

#include "EditorTransform.hpp"
#include "EditorWorldLayoutLifecycle.hpp"
#include "EditorWorldLayoutPlan.hpp"

#include "app/iggy3d/creative/render/CreativeScreenProjection.hpp"
#include "app/iggy3d/creative/world/WorldLayoutVerticalConnectors.hpp"

namespace iggy3d_creative_app {
namespace {

[[nodiscard]] bool validGrid(cr::CreativeGridSettings grid) noexcept {
  return std::isfinite(grid.cellSizeMeters) && grid.cellSizeMeters > 0.0 &&
         cr::isFiniteCreativeVec3(grid.origin);
}

[[nodiscard]] std::size_t resolveConnectorIndex(
    const CreativeEditorWorldLayoutState& state,
    std::size_t requested) noexcept {
  if (requested < state.source.verticalConnectors.size()) {
    return requested;
  }
  if (state.verticalConnectorManipulation.active &&
      state.verticalConnectorManipulation.target.connectorIndex <
          state.source.verticalConnectors.size()) {
    return state.verticalConnectorManipulation.target.connectorIndex;
  }
  return state.selection.kind ==
                 CreativeEditorWorldLayoutSelectionKind::VerticalConnector &&
             state.selection.index < state.source.verticalConnectors.size()
         ? state.selection.index
         : cr::kInvalidCreativeWorldLayoutIndex;
}

[[nodiscard]] iggy3d::Vec3 toWorld(
    CreativeEditorWorldLayoutPoint point,
    double y,
    cr::CreativeGridSettings grid) noexcept {
  return {
      static_cast<float>(grid.origin.x + point.x * grid.cellSizeMeters),
      static_cast<float>(y),
      static_cast<float>(grid.origin.z + point.z * grid.cellSizeMeters),
  };
}

[[nodiscard]] double connectorSurfaceHeight(
    cr::CreativeWorldLayoutRect footprint,
    cr::CreativeWorldLayoutVerticalDirection direction,
    CreativeEditorWorldLayoutPoint point,
    const cr::CreativeWorldLayoutVerticalConnectorPlan& plan) noexcept {
  double progress = 0.0;
  switch (direction) {
    case cr::CreativeWorldLayoutVerticalDirection::PositiveX:
      progress = (point.x - footprint.minimum.x) /
                 static_cast<double>(footprint.maximum.x -
                                     footprint.minimum.x);
      break;
    case cr::CreativeWorldLayoutVerticalDirection::NegativeX:
      progress = (footprint.maximum.x - point.x) /
                 static_cast<double>(footprint.maximum.x -
                                     footprint.minimum.x);
      break;
    case cr::CreativeWorldLayoutVerticalDirection::PositiveZ:
      progress = (point.z - footprint.minimum.z) /
                 static_cast<double>(footprint.maximum.z -
                                     footprint.minimum.z);
      break;
    case cr::CreativeWorldLayoutVerticalDirection::NegativeZ:
      progress = (footprint.maximum.z - point.z) /
                 static_cast<double>(footprint.maximum.z -
                                     footprint.minimum.z);
      break;
    case cr::CreativeWorldLayoutVerticalDirection::Count:
      return plan.authoredBounds.min.y;
  }
  return plan.authoredBounds.min.y +
         std::clamp(progress, 0.0, 1.0) *
             (plan.authoredBounds.max.y - plan.authoredBounds.min.y);
}

void appendHandle(
    CreativeEditorWorldLayoutVerticalConnectorHandleFrame& frame,
    std::size_t connectorIndex,
    CreativeEditorWorldLayoutRectHandle handle,
    CreativeEditorWorldLayoutPoint planPosition,
    iggy3d::Vec3 worldAxis,
    bool directionHandle,
    bool planeSample,
    double worldY,
    cr::CreativeGridSettings grid) {
  if (frame.handleCount >= frame.handles.size()) {
    return;
  }
  CreativeEditorWorldLayoutVerticalConnectorHandle& output =
      frame.handles[frame.handleCount++];
  output.target = {connectorIndex, handle, directionHandle};
  output.planPosition = planPosition;
  output.worldPosition = toWorld(planPosition, worldY, grid);
  output.worldAxis = worldAxis;
  output.planeSample = planeSample;
  output.valid = std::isfinite(planPosition.x) &&
                 std::isfinite(planPosition.z) &&
                 std::isfinite(worldY);
}

[[nodiscard]] CreativeEditorWorldLayoutEditReceipt applyManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutVerticalConnectorManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point,
    CreativeEditorWorldLayoutVerticalConnectorTarget target,
    cr::CreativeGridSettings grid,
    double toleranceCells) {
  return applyCreativeEditorWorldLayoutVerticalConnectorManipulation(
      state, phase, point, toleranceCells, grid, target);
}

}  // namespace

CreativeEditorWorldLayoutVerticalConnectorHandleFrame
buildCreativeEditorWorldLayoutVerticalConnectorHandleFrame(
    const CreativeEditorWorldLayoutState& state,
    cr::CreativeGridSettings grid,
    std::size_t connectorIndex,
    bool includeManipulationPreview) {
  CreativeEditorWorldLayoutVerticalConnectorHandleFrame frame;
  const std::size_t resolved = resolveConnectorIndex(state, connectorIndex);
  if (!validGrid(grid) ||
      resolved >= state.source.verticalConnectors.size() ||
      state.generatedRevision != state.revision) {
    frame.reasonCode =
        "creative_editor_world_layout_vertical_connector_handles_invalid";
    return frame;
  }

  cr::CreativeWorldLayoutVerticalConnector connector =
      state.source.verticalConnectors[resolved];
  if (includeManipulationPreview &&
      state.verticalConnectorManipulation.active &&
      state.verticalConnectorManipulation.target.connectorIndex == resolved &&
      state.verticalConnectorManipulation.previewValid) {
    connector.footprint =
        state.verticalConnectorManipulation.previewFootprint;
    connector.kind = state.verticalConnectorManipulation.previewKind;
    connector.direction =
        state.verticalConnectorManipulation.previewDirection;
  }
  frame.connector = cr::planCreativeWorldLayoutVerticalConnector(
      grid, state.source, resolved, connector);
  if (!frame.connector.accepted) {
    frame.reasonCode = frame.connector.reasonCode;
    return frame;
  }

  const cr::CreativeWorldLayoutRect footprint = connector.footprint;
  const double centerX =
      (static_cast<double>(footprint.minimum.x) + footprint.maximum.x) * 0.5;
  const double centerZ =
      (static_cast<double>(footprint.minimum.z) + footprint.maximum.z) * 0.5;
  const CreativeEditorWorldLayoutPoint center{centerX, centerZ};
  const CreativeEditorWorldLayoutPoint north{centerX,
                                              double(footprint.minimum.z)};
  const CreativeEditorWorldLayoutPoint east{double(footprint.maximum.x),
                                             centerZ};
  const CreativeEditorWorldLayoutPoint south{centerX,
                                              double(footprint.maximum.z)};
  const CreativeEditorWorldLayoutPoint west{double(footprint.minimum.x),
                                             centerZ};

  appendHandle(frame, resolved, CreativeEditorWorldLayoutRectHandle::Move,
               center, {}, false, true,
               connectorSurfaceHeight(footprint, connector.direction, center,
                                      frame.connector),
               grid);
  appendHandle(frame, resolved, CreativeEditorWorldLayoutRectHandle::North,
               north, {0.0F, 0.0F, 1.0F}, false, false,
               connectorSurfaceHeight(footprint, connector.direction, north,
                                      frame.connector),
               grid);
  appendHandle(frame, resolved, CreativeEditorWorldLayoutRectHandle::East,
               east, {1.0F, 0.0F, 0.0F}, false, false,
               connectorSurfaceHeight(footprint, connector.direction, east,
                                      frame.connector),
               grid);
  appendHandle(frame, resolved, CreativeEditorWorldLayoutRectHandle::South,
               south, {0.0F, 0.0F, 1.0F}, false, false,
               connectorSurfaceHeight(footprint, connector.direction, south,
                                      frame.connector),
               grid);
  appendHandle(frame, resolved, CreativeEditorWorldLayoutRectHandle::West,
               west, {1.0F, 0.0F, 0.0F}, false, false,
               connectorSurfaceHeight(footprint, connector.direction, west,
                                      frame.connector),
               grid);

  CreativeEditorWorldLayoutPoint direction;
  if (!resolveCreativeEditorWorldLayoutVerticalConnectorDirectionHandle(
          footprint, connector.direction, direction)) {
    frame.reasonCode =
        "creative_editor_world_layout_vertical_connector_direction_handle_invalid";
    return frame;
  }
  appendHandle(frame, resolved, CreativeEditorWorldLayoutRectHandle::None,
               direction, {}, true, true, frame.connector.authoredBounds.max.y,
               grid);

  frame.accepted = frame.handleCount == frame.handles.size();
  frame.reasonCode =
      frame.accepted
          ? "creative_editor_world_layout_vertical_connector_handles_ready"
          : "creative_editor_world_layout_vertical_connector_handles_incomplete";
  return frame;
}

const CreativeEditorWorldLayoutVerticalConnectorHandle*
findCreativeEditorWorldLayoutVerticalConnectorHandle(
    const CreativeEditorWorldLayoutVerticalConnectorHandleFrame& frame,
    CreativeEditorWorldLayoutVerticalConnectorTarget target) noexcept {
  for (std::size_t index = 0U; index < frame.handleCount; ++index) {
    if (frame.handles[index].valid &&
        frame.handles[index].target == target) {
      return &frame.handles[index];
    }
  }
  return nullptr;
}

CreativeEditorWorldLayoutVerticalConnectorHandlePick
pickCreativeEditorWorldLayoutVerticalConnectorHandleAtPixel(
    const CreativeEditorWorldLayoutVerticalConnectorHandleFrame& frame,
    const iggy3d::RenderCameraFrame& camera,
    iggy3d::RenderContentViewport viewport,
    float pixelX,
    float pixelY,
    float tolerancePixels) noexcept {
  CreativeEditorWorldLayoutVerticalConnectorHandlePick result;
  if (!frame.accepted || viewport.width == 0U || viewport.height == 0U ||
      !std::isfinite(pixelX) || !std::isfinite(pixelY) ||
      !std::isfinite(tolerancePixels) || tolerancePixels < 0.0F) {
    return result;
  }
  const float maximumDistanceSquared = tolerancePixels * tolerancePixels;
  float nearestDistanceSquared = maximumDistanceSquared;
  for (std::size_t index = 0U; index < frame.handleCount; ++index) {
    const CreativeEditorWorldLayoutVerticalConnectorHandle& handle =
        frame.handles[index];
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

bool sampleCreativeEditorWorldLayoutVerticalConnectorHandlePoint(
    const CreativeEditorWorldLayoutVerticalConnectorHandle& handle,
    iggy3d::Vec3 rayOrigin,
    iggy3d::Vec3 rayDirection,
    cr::CreativeGridSettings grid,
    CreativeEditorWorldLayoutPoint& output) noexcept {
  if (!handle.valid || !validGrid(grid) || !std::isfinite(rayOrigin.x) ||
      !std::isfinite(rayOrigin.y) || !std::isfinite(rayOrigin.z) ||
      !std::isfinite(rayDirection.x) || !std::isfinite(rayDirection.y) ||
      !std::isfinite(rayDirection.z)) {
    return false;
  }
  if (handle.planeSample) {
    if (std::fabs(rayDirection.y) <= 1.0e-6F) {
      return false;
    }
    const double rayParameter =
        (static_cast<double>(handle.worldPosition.y) - rayOrigin.y) /
        rayDirection.y;
    if (!std::isfinite(rayParameter) || rayParameter < 0.0) {
      return false;
    }
    const double worldX = rayOrigin.x + rayDirection.x * rayParameter;
    const double worldZ = rayOrigin.z + rayDirection.z * rayParameter;
    output = {(worldX - grid.origin.x) / grid.cellSizeMeters,
              (worldZ - grid.origin.z) / grid.cellSizeMeters};
    return std::isfinite(output.x) && std::isfinite(output.z);
  }

  const CreativeEditorTransformAxisRaySample sample =
      sampleCreativeEditorTransformAxisRay(
          cr::creativeVec3FromCore(rayOrigin),
          cr::creativeVec3FromCore(rayDirection),
          cr::creativeVec3FromCore(handle.worldPosition),
          cr::creativeVec3FromCore(handle.worldAxis));
  if (!sample.valid) {
    return false;
  }
  output = handle.planPosition;
  output.x += static_cast<double>(handle.worldAxis.x) *
              sample.axisParameter / grid.cellSizeMeters;
  output.z += static_cast<double>(handle.worldAxis.z) *
              sample.axisParameter / grid.cellSizeMeters;
  return std::isfinite(output.x) && std::isfinite(output.z);
}

CreativeEditorWorldLayoutVerticalConnectorLiveEditReceipt
applyCreativeEditorWorldLayoutVerticalConnectorManipulationToDocument(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeAppState& appState,
    CreativeEditorWorldLayoutVerticalConnectorManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point,
    CreativeEditorWorldLayoutVerticalConnectorTarget target,
    double toleranceCells) {
  CreativeEditorWorldLayoutVerticalConnectorLiveEditReceipt result;
  const cr::CreativeGridSettings grid =
      appState.facade.document().gridSettings();
  const bool previewWasActive =
      creativeEditorWorldLayoutPreviewActive(state);

  if (phase ==
      CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::Begin) {
    result.sceneChanged =
        clearCreativeEditorWorldLayoutLiveEditPreview(state);
    const CreativeEditorWorldLayoutEditReceipt edit = applyManipulation(
        state, phase, point, target, grid, toleranceCells);
    result.accepted = edit.accepted;
    result.changed = edit.changed;
    result.reasonCode = edit.reasonCode;
    return result;
  }
  if (phase ==
      CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::Cancel) {
    const CreativeEditorWorldLayoutEditReceipt edit = applyManipulation(
        state, phase, point, target, grid, toleranceCells);
    result.accepted = edit.accepted;
    result.changed = edit.changed;
    result.sceneChanged =
        clearCreativeEditorWorldLayoutLiveEditPreview(state);
    result.reasonCode = edit.reasonCode;
    return result;
  }

  const bool sourceSynchronized = state.generatedRevision == state.revision;
  if (!sourceSynchronized) {
    const CreativeEditorWorldLayoutEditReceipt edit = applyManipulation(
        state, phase, point, target, grid, toleranceCells);
    result.accepted = edit.accepted;
    result.changed = edit.changed;
    result.worldLayoutChanged =
        phase ==
            CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
                Commit &&
        edit.changed;
    result.sceneChanged = previewWasActive && result.worldLayoutChanged;
    result.reasonCode = edit.reasonCode;
    return result;
  }

  if (phase ==
      CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::Update) {
    const CreativeEditorWorldLayoutEditReceipt edit = applyManipulation(
        state, phase, point, target, grid, toleranceCells);
    result.accepted = edit.accepted;
    result.changed = edit.changed;
    result.reasonCode = edit.reasonCode;
    if (!edit.accepted || !edit.changed) {
      return result;
    }
    CreativeEditorWorldLayoutState candidate =
        makeCreativeEditorWorldLayoutLiveEditCandidate(state);
    const CreativeEditorWorldLayoutEditReceipt candidateEdit =
        applyManipulation(
            candidate,
            CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
                Commit,
            point, {}, grid, toleranceCells);
    if (!candidateEdit.accepted || !candidateEdit.changed) {
      const std::string candidateMessage = candidate.statusMessage;
      result.sceneChanged =
          clearCreativeEditorWorldLayoutLiveEditPreview(state);
      state.statusMessage = candidateMessage;
      return result;
    }
    const CreativeEditorWorldLayoutPreviewReceipt preview =
        previewCreativeEditorWorldLayoutLiveEditCandidate(
            state, appState.facade.document(), std::move(candidate),
            candidateEdit, "vertical connector drag preview ready in 3D");
    result.sceneChanged = preview.changed;
    result.reasonCode = preview.reasonCode;
    return result;
  }

  if (phase !=
      CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::Commit) {
    const CreativeEditorWorldLayoutEditReceipt edit = applyManipulation(
        state, phase, point, target, grid, toleranceCells);
    result.accepted = edit.accepted;
    result.changed = edit.changed;
    result.reasonCode = edit.reasonCode;
    return result;
  }

  CreativeEditorWorldLayoutState candidate =
      makeCreativeEditorWorldLayoutLiveEditCandidate(state);
  const CreativeEditorWorldLayoutEditReceipt candidateEdit =
      applyManipulation(
          candidate,
          CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::Commit,
          point, {}, grid, toleranceCells);
  if (!candidateEdit.accepted || !candidateEdit.changed) {
    const std::string candidateMessage = candidate.statusMessage;
    static_cast<void>(applyManipulation(
        state,
        CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::Cancel,
        point, {}, grid, toleranceCells));
    result.sceneChanged =
        clearCreativeEditorWorldLayoutLiveEditPreview(state);
    state.statusMessage = candidateMessage;
    result.accepted = candidateEdit.accepted;
    result.reasonCode = candidateEdit.reasonCode;
    return result;
  }

  const CreativeEditorWorldLayoutApplyReceipt applied =
      applyCreativeEditorWorldLayoutLiveEditCandidate(
          state, appState, std::move(candidate), candidateEdit,
          "direct_world_layout_vertical_connector_drag",
          "vertical connector updated in 3D");
  result.accepted = applied.accepted;
  result.changed = applied.changed;
  result.worldLayoutChanged = applied.changed;
  result.sceneChanged = previewWasActive || applied.apply.changed;
  result.reasonCode = applied.reasonCode;
  if (!applied.accepted) {
    const std::string failureMessage = state.statusMessage;
    static_cast<void>(applyManipulation(
        state,
        CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::Cancel,
        point, {}, grid, toleranceCells));
    static_cast<void>(
        clearCreativeEditorWorldLayoutLiveEditPreview(state));
    state.statusMessage = failureMessage;
  }
  return result;
}

}  // namespace iggy3d_creative_app
