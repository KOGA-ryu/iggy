#include "EditorGizmo.hpp"

#include <SDL3/SDL_log.h>

#include <cstddef>
#include <string>

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "EditorFrame.hpp"
#include "EditorPathEditing.hpp"

namespace iggy3d_creative_app {

namespace creative = iggy3d::creative;

namespace {

bool isAppliedMoveCommit(
    const cr::CreativeFacadeToolDispatchReceipt& receipt) {
  return receipt.moveDrag.stage == cr::CreativeFacadeMoveDragStage::Commit &&
         receipt.moveDrag.outcome == cr::CreativeFacadeMoveDragOutcome::Applied &&
         receipt.moveDrag.committed && receipt.moveDrag.changed;
}

}  // namespace

cr::CreativeToolMoveHeldAxis heldAxisForGrabbedAxis(GizmoAxis grabbed) {
  switch (grabbed) {
    case GizmoAxis::X:
      return cr::CreativeToolMoveHeldAxis::Y;
    case GizmoAxis::Y:
      return cr::CreativeToolMoveHeldAxis::X;
    case GizmoAxis::Z:
      return cr::CreativeToolMoveHeldAxis::X;
    case GizmoAxis::None:
    default:
      return cr::CreativeToolMoveHeldAxis::Y;
  }
}

void logUndoMovePlacement(const char* phase,
                          cr::CreativeObjectId objectId,
                          const cr::CreativeObject* object) {
  if (object == nullptr) {
    SDL_Log("iggy3d_creative: UNDO move %s objectId=%llu object=<null>",
            phase, static_cast<unsigned long long>(objectId));
    return;
  }
  SDL_Log("iggy3d_creative: UNDO move %s objectId=%llu pos=(%.3f, %.3f, %.3f) "
          "bounds=[(%.3f, %.3f, %.3f)..(%.3f, %.3f, %.3f)]",
          phase, static_cast<unsigned long long>(objectId),
          object->transform.position.x, object->transform.position.y,
          object->transform.position.z, object->bounds.min.x,
          object->bounds.min.y, object->bounds.min.z, object->bounds.max.x,
          object->bounds.max.y, object->bounds.max.z);
}

void logMoveDispatch(const char* phase,
                     const cr::CreativeFacadeToolDispatchReceipt& receipt) {
  SDL_Log("iggy3d_creative: MOVE dispatch %s inputKind=%d accepted=%d changed=%d "
          "moveDragChanged=%d | drag.stage=%d drag.outcome=%d requested=%d "
          "accepted=%d committed=%d changed=%d snappedAnchor=(%.3f, %.3f, %.3f) "
          "documentStatus=%d msg='%s'",
          phase, static_cast<int>(receipt.inputKind),
          receipt.accepted ? 1 : 0, receipt.changed ? 1 : 0,
          receipt.moveDragChanged ? 1 : 0,
          static_cast<int>(receipt.moveDrag.stage),
          static_cast<int>(receipt.moveDrag.outcome),
          receipt.moveDrag.requested ? 1 : 0,
          receipt.moveDrag.accepted ? 1 : 0,
          receipt.moveDrag.committed ? 1 : 0,
          receipt.moveDrag.changed ? 1 : 0, receipt.moveDrag.snappedAnchor.x,
          receipt.moveDrag.snappedAnchor.y, receipt.moveDrag.snappedAnchor.z,
          static_cast<int>(receipt.moveDrag.documentStatus),
          receipt.moveDrag.message.c_str());
}

cr::CreativeFacadeToolDispatchReceipt dispatchMoveReleaseWithUndo(
    cr::CreativeAppState& appState,
    StandaloneEditHistory& history,
    const cr::CreativeToolInputPacket& release,
    cr::CreativeObjectId objectId,
    std::string_view source) {
  const std::uint64_t undoDepthBefore = cr::creativeUndoDepth(history);
  logUndoMovePlacement("before", objectId, appState.facade.findObject(objectId));
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  const cr::CreativeFacadeToolDispatchReceipt receipt =
      appState.facade.dispatchToolInput(release);
  const bool applied = isAppliedMoveCommit(receipt);
  (void)completeEditTransaction(history, std::move(transaction), appState.facade,
                                applied, receipt.moveDrag.message);
  logUndoMovePlacement("after", objectId, appState.facade.findObject(objectId));
  SDL_Log("iggy3d_creative: UNDO move commit source='%s' objectId=%llu "
          "applied=%d accepted=%d changed=%d moveDragChanged=%d "
          "depthBefore=%llu depthAfter=%llu stage=%d outcome=%d "
          "documentStatus=%d reasonCode='%s'",
          std::string(source).c_str(),
          static_cast<unsigned long long>(objectId), applied ? 1 : 0,
          receipt.accepted ? 1 : 0, receipt.changed ? 1 : 0,
          receipt.moveDrag.changed ? 1 : 0,
          static_cast<unsigned long long>(undoDepthBefore),
          static_cast<unsigned long long>(cr::creativeUndoDepth(history)),
          static_cast<int>(receipt.moveDrag.stage),
          static_cast<int>(receipt.moveDrag.outcome),
          static_cast<int>(receipt.moveDrag.documentStatus),
          receipt.moveDrag.message.c_str());
  return receipt;
}

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

  frame.selectedPathHandleObjectId =
      static_cast<iggy3d::creative::CreativeObjectId>(selection.selectedId);
  frame.selectedIsPathForHandles =
      selection.hasSelection && selection.selectionCount == 1U &&
      iggy3d::creative::objectStoresPathPoints(selection.selected->kind) &&
      validPathPoints(selection.selected->pathPoints);
  if (frame.selectedIsPathForHandles) {
    frame.pathPointHandleHits = buildPathPointHandleHits(
        *selection.selected, camera.clipFromWorld, drawableWidth, drawableHeight);
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
