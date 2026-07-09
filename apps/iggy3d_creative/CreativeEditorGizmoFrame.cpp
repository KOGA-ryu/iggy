#include "CreativeEditorGizmoFrame.hpp"

#include <cstddef>

#include <SDL3/SDL.h>

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"

#include "EditorPreviewProxies.hpp"

namespace iggy3d_creative_app {

CreativeEditorGizmoFrame buildCreativeEditorGizmoFrame(
    const CreativeEditorSelectionFrame& selection,
    const iggy3d::RenderCameraFrame& camera,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    float axisLengthMeters) {
  CreativeEditorGizmoFrame frame;
  frame.center = {(selection.boxMin.x + selection.boxMax.x) * 0.5F,
                  (selection.boxMin.y + selection.boxMax.y) * 0.5F,
                  (selection.boxMin.z + selection.boxMax.z) * 0.5F};
  frame.shafts[0] = {GizmoAxis::X,
                     {frame.center.x + axisLengthMeters, frame.center.y,
                      frame.center.z},
                     {1.0F, 0.0F, 0.0F, 1.0F}};
  frame.shafts[1] = {GizmoAxis::Y,
                     {frame.center.x, frame.center.y + axisLengthMeters,
                      frame.center.z},
                     {0.0F, 1.0F, 0.0F, 1.0F}};
  frame.shafts[2] = {GizmoAxis::Z,
                     {frame.center.x, frame.center.y,
                      frame.center.z + axisLengthMeters},
                     {0.0F, 0.0F, 1.0F, 1.0F}};

  frame.centerScreen = projectPointToScreen(
      camera.clipFromWorld, frame.center, drawableWidth, drawableHeight);
  for (std::size_t i = 0; i < 3; ++i) {
    frame.tipScreens[i] =
        projectPointToScreen(camera.clipFromWorld,
                             frame.shafts[i].tip,
                             drawableWidth,
                             drawableHeight);
  }

  frame.selectedPathHandleObjectId =
      static_cast<iggy3d::creative::CreativeObjectId>(selection.selectedId);
  frame.selectedIsPathForHandles =
      selection.hasSelection &&
      iggy3d::creative::describeObject(selection.selected->kind).shapeKind ==
          iggy3d::creative::CreativeObjectShapeKind::Path &&
      validPathPoints(selection.selected->pathPoints);
  if (frame.selectedIsPathForHandles) {
    frame.pathPointHandleHits = buildPathPointHandleHits(
        *selection.selected, camera.clipFromWorld, drawableWidth, drawableHeight);
  }

  frame.anchorS = {frame.center.x, frame.center.y, frame.center.z};
  if (selection.hasSelection) {
    frame.anchorS = toVec3(selection.selected->transform.position);
  }
  return frame;
}

void logCreativeEditorPathHandleCaptureFrame(
    StandaloneCaptureScript& captureScript,
    bool captureMode,
    const CreativeEditorGizmoFrame& gizmoFrame) {
  if (!captureMode || captureScript.pathPointHandleLogged ||
      !gizmoFrame.selectedIsPathForHandles ||
      gizmoFrame.selectedPathHandleObjectId != captureScript.pathTargetId) {
    return;
  }

  for (const PathPointHandleHit& handle : gizmoFrame.pathPointHandleHits) {
    SDL_Log("iggy3d_creative: PATH_HANDLE hit proxy objectId=%llu "
            "pointIndex=%zu aabbValid=%d position=(%.3f, %.3f, %.3f) "
            "screen=[%.1f, %.1f..%.1f, %.1f]",
            static_cast<unsigned long long>(handle.objectId),
            handle.pointIndex, handle.aabb.valid ? 1 : 0,
            handle.position.x, handle.position.y, handle.position.z,
            handle.aabb.minX, handle.aabb.minY, handle.aabb.maxX,
            handle.aabb.maxY);
  }
  captureScript.pathPointHandleLogged = true;
}

}  // namespace iggy3d_creative_app
