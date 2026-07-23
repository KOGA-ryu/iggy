#include "EditorGizmo.hpp"

#include <SDL3/SDL_log.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <string>

#include "EditorTransform.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/render/CreativeScreenProjection.hpp"
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

[[nodiscard]] float pointSegmentDistanceSquared(
    float pointX,
    float pointY,
    const GizmoAxisScreenHandle& handle) noexcept {
  const float dx = handle.endX - handle.startX;
  const float dy = handle.endY - handle.startY;
  const float lengthSquared = dx * dx + dy * dy;
  if (!std::isfinite(lengthSquared) || lengthSquared <= 1.0e-4F) {
    return std::numeric_limits<float>::max();
  }
  const float t = std::clamp(
      ((pointX - handle.startX) * dx +
       (pointY - handle.startY) * dy) /
          lengthSquared,
      0.0F, 1.0F);
  const float nearestX = handle.startX + dx * t;
  const float nearestY = handle.startY + dy * t;
  const float offsetX = pointX - nearestX;
  const float offsetY = pointY - nearestY;
  return offsetX * offsetX + offsetY * offsetY;
}

[[nodiscard]] iggy3d::Vec3 normalizedAxis(iggy3d::Vec3 value,
                                           iggy3d::Vec3 fallback) noexcept {
  const float lengthSquared = iggy3d::lengthSquared(value);
  if (!std::isfinite(lengthSquared) || lengthSquared <= 1.0e-8F) {
    return fallback;
  }
  return value / std::sqrt(lengthSquared);
}

[[nodiscard]] iggy3d::Vec3 transformAxisDirection(
    const CreativeEditorSelectionTransformState* transform,
    cr::CreativeAxis3 axis) noexcept {
  cr::CreativeVec3 direction{};
  switch (axis) {
    case cr::CreativeAxis3::X: direction.x = 1.0; break;
    case cr::CreativeAxis3::Y: direction.y = 1.0; break;
    case cr::CreativeAxis3::Z: direction.z = 1.0; break;
    case cr::CreativeAxis3::Count: return {};
  }
  if (transform != nullptr && transform->active &&
      transform->request.coordinateSpace ==
          cr::CreativeSelectionPlacementCoordinateSpace::Local) {
    direction = cr::rotateCreativeVectorEulerXyz(
        direction, transform->request.coordinateBasisEulerRadians);
  }
  const cr::CreativeCoreVec3Conversion converted =
      cr::creativeVec3ToCoreChecked(direction);
  if (!converted.converted) {
    return {};
  }
  const iggy3d::Vec3 fallback =
      axis == cr::CreativeAxis3::X
          ? iggy3d::Vec3{1.0F, 0.0F, 0.0F}
          : axis == cr::CreativeAxis3::Y
                ? iggy3d::Vec3{0.0F, 1.0F, 0.0F}
                : iggy3d::Vec3{0.0F, 0.0F, 1.0F};
  return normalizedAxis(converted.value, fallback);
}

[[nodiscard]] iggy3d::RenderContentViewport resolvedContentViewport(
    iggy3d::RenderContentViewport content,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight) noexcept {
  if (content.width == 0U || content.height == 0U) {
    return {0, 0, drawableWidth, drawableHeight};
  }
  return content;
}

[[nodiscard]] GizmoAxisScreenHandle buildAxisScreenHandle(
    GizmoAxis axis,
    iggy3d::Vec3 center,
    iggy3d::Vec3 tip,
    const iggy3d::RenderCameraFrame& camera,
    iggy3d::RenderContentViewport content) noexcept {
  GizmoAxisScreenHandle handle;
  handle.axis = axis;
  const cr::CreativeScreenPoint projectedCenter =
      cr::projectCreativeWorldPointToScreen(camera.clipFromWorld, center,
                                            content.width, content.height);
  const cr::CreativeScreenPoint projectedTip =
      cr::projectCreativeWorldPointToScreen(camera.clipFromWorld, tip,
                                            content.width, content.height);
  if (!projectedCenter.valid || !projectedTip.valid) {
    return handle;
  }
  const float centerX = static_cast<float>(content.x) + projectedCenter.x;
  const float centerY = static_cast<float>(content.y) + projectedCenter.y;
  const float tipX = static_cast<float>(content.x) + projectedTip.x;
  const float tipY = static_cast<float>(content.y) + projectedTip.y;
  constexpr float kCenterExclusion = 0.22F;
  handle.startX = std::lerp(centerX, tipX, kCenterExclusion);
  handle.startY = std::lerp(centerY, tipY, kCenterExclusion);
  handle.endX = tipX;
  handle.endY = tipY;
  const float dx = handle.endX - handle.startX;
  const float dy = handle.endY - handle.startY;
  handle.valid = std::isfinite(handle.startX) &&
                 std::isfinite(handle.startY) &&
                 std::isfinite(handle.endX) && std::isfinite(handle.endY) &&
                 dx * dx + dy * dy >= 16.0F;
  return handle;
}

}  // namespace

GizmoAxisPickResult pickCreativeEditorGizmoAxisAtPixel(
    std::span<const GizmoAxisScreenHandle> handles,
    float pixelX,
    float pixelY,
    float paddingPixels) noexcept {
  GizmoAxisPickResult result;
  result.distanceSquared = std::numeric_limits<float>::max();
  if (!std::isfinite(pixelX) || !std::isfinite(pixelY) ||
      !std::isfinite(paddingPixels) || paddingPixels < 0.0F) {
    return result;
  }
  const float thresholdSquared = paddingPixels * paddingPixels;
  for (const GizmoAxisScreenHandle& handle : handles) {
    if (!handle.valid || handle.axis == GizmoAxis::None) {
      continue;
    }
    const float distanceSquared =
        pointSegmentDistanceSquared(pixelX, pixelY, handle);
    if (distanceSquared <= thresholdSquared &&
        distanceSquared < result.distanceSquared) {
      result.hit = true;
      result.axis = handle.axis;
      result.distanceSquared = distanceSquared;
    }
  }
  return result;
}

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
    const CreativeMovingPlatformPathEditState& pathEdit,
    const iggy3d::RenderCameraFrame& camera,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    float axisLengthMeters,
    iggy3d::RenderContentViewport contentViewport,
    const CreativeEditorSelectionTransformState* transform) {
  CreativeEditorGizmoFrame frame;
  frame.center = {(selection.boxMin.x + selection.boxMax.x) * 0.5F,
                  (selection.boxMin.y + selection.boxMax.y) * 0.5F,
                  (selection.boxMin.z + selection.boxMax.z) * 0.5F};
  if (transform != nullptr && transform->active &&
      transform->targetPositionable) {
    const cr::CreativeCoreVec3Conversion converted =
        cr::creativeVec3ToCoreChecked(transform->request.targetAnchor);
    if (converted.converted) {
      frame.center = converted.value;
    }
  } else if (selection.selectionCount == 1U && selection.selected != nullptr &&
             selection.selected->kind == cr::CreativeObjectKind::Group) {
    const cr::CreativeCoreVec3Conversion converted =
        cr::creativeVec3ToCoreChecked(selection.selected->transform.position);
    if (converted.converted) {
      frame.center = converted.value;
    }
  }
  if (pathEdit.pointSelected && selection.selected != nullptr &&
      pathEdit.objectId == selection.selected->id &&
      pathEdit.selectedPointIndex < selection.selected->pathPoints.size()) {
    const creative::CreativeVec3 point =
        selection.selected->pathPoints[pathEdit.selectedPointIndex].position;
    frame.center = {static_cast<float>(point.x), static_cast<float>(point.y),
                    static_cast<float>(point.z)};
  }
  const std::array directions{
      transformAxisDirection(transform, cr::CreativeAxis3::X),
      transformAxisDirection(transform, cr::CreativeAxis3::Y),
      transformAxisDirection(transform, cr::CreativeAxis3::Z),
  };
  frame.shafts[0] = {GizmoAxis::X,
                     frame.center + directions[0] * axisLengthMeters,
                     {1.0F, 0.0F, 0.0F, 1.0F}};
  frame.shafts[1] = {GizmoAxis::Y,
                     frame.center + directions[1] * axisLengthMeters,
                     {0.0F, 1.0F, 0.0F, 1.0F}};
  frame.shafts[2] = {GizmoAxis::Z,
                     frame.center + directions[2] * axisLengthMeters,
                     {0.0F, 0.0F, 1.0F, 1.0F}};

  const iggy3d::RenderContentViewport content = resolvedContentViewport(
      contentViewport, drawableWidth, drawableHeight);
  if (selection.hasSelection && std::isfinite(axisLengthMeters) &&
      axisLengthMeters > 0.0F && content.width > 0U && content.height > 0U) {
    for (std::size_t index = 0U; index < frame.shafts.size(); ++index) {
      frame.axisHandles[index] = buildAxisScreenHandle(
          frame.shafts[index].axis, frame.center, frame.shafts[index].tip,
          camera, content);
    }
  }

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
