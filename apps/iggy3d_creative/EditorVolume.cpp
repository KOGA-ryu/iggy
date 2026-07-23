#include "EditorVolume.hpp"

#include <SDL3/SDL_log.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <string>
#include <utility>

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/history/History.hpp"
#include "app/iggy3d/creative/render/CreativeScreenProjection.hpp"

#include "EditorEdits.hpp"
#include "EditorTransform.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace creative = iggy3d::creative;
namespace {

[[nodiscard]] bool sameGrid(CreativeEditorVolumeState& state,
                            double cellSize,
                            creative::CreativeVec3 origin) noexcept {
  return std::isfinite(cellSize) &&
         state.selection.cellSize == cellSize &&
         state.selection.origin.x == origin.x &&
         state.selection.origin.y == origin.y &&
         state.selection.origin.z == origin.z;
}

[[nodiscard]] bool sameSelection(
    const creative::CreativeVolumeSelection& lhs,
    const creative::CreativeVolumeSelection& rhs) noexcept {
  return lhs.phase == rhs.phase && lhs.firstCell.x == rhs.firstCell.x &&
         lhs.firstCell.y == rhs.firstCell.y &&
         lhs.firstCell.z == rhs.firstCell.z &&
         lhs.secondCell.x == rhs.secondCell.x &&
         lhs.secondCell.y == rhs.secondCell.y &&
         lhs.secondCell.z == rhs.secondCell.z && lhs.origin.x == rhs.origin.x &&
         lhs.origin.y == rhs.origin.y && lhs.origin.z == rhs.origin.z &&
         lhs.cellSize == rhs.cellSize;
}

[[nodiscard]] bool sameRequest(
    const creative::CreativeVolumeOperationRequest& lhs,
    const creative::CreativeVolumeOperationRequest& rhs) noexcept {
  return lhs.operation == rhs.operation &&
         sameSelection(lhs.selection, rhs.selection) &&
         lhs.objectKind == rhs.objectKind && lhs.shapeKind == rhs.shapeKind &&
         lhs.shapeAxis == rhs.shapeAxis &&
         lhs.fillOverlapPolicy == rhs.fillOverlapPolicy &&
         lhs.hollowThickness == rhs.hollowThickness &&
         lhs.hollowAlignment == rhs.hollowAlignment &&
         lhs.hollowOpening == rhs.hollowOpening &&
         lhs.hollowCornerRule == rhs.hollowCornerRule &&
         lhs.hasReplaceKindFilter == rhs.hasReplaceKindFilter &&
         lhs.replaceKindFilter == rhs.replaceKindFilter &&
         lhs.replaceMemberMask == rhs.replaceMemberMask &&
         lhs.hasEraseKindFilter == rhs.hasEraseKindFilter &&
         lhs.eraseKindFilter == rhs.eraseKindFilter &&
         lhs.eraseMemberMask == rhs.eraseMemberMask &&
         lhs.hasCloneOffset == rhs.hasCloneOffset &&
         lhs.cloneOffset.x == rhs.cloneOffset.x &&
         lhs.cloneOffset.y == rhs.cloneOffset.y &&
         lhs.cloneOffset.z == rhs.cloneOffset.z &&
         lhs.cloneQuarterTurns == rhs.cloneQuarterTurns &&
         lhs.cloneMirrorX == rhs.cloneMirrorX &&
         lhs.cloneMirrorZ == rhs.cloneMirrorZ &&
         lhs.cloneMemberMask == rhs.cloneMemberMask &&
         lhs.cloneVoxelOverlapPolicy == rhs.cloneVoxelOverlapPolicy &&
         lhs.maxAffectedObjects == rhs.maxAffectedObjects;
}

[[nodiscard]] cr::CreativeAxis3 axisForFace(
    cr::CreativeVolumeFace face) noexcept {
  switch (face) {
    case cr::CreativeVolumeFace::NegativeX:
    case cr::CreativeVolumeFace::PositiveX: return cr::CreativeAxis3::X;
    case cr::CreativeVolumeFace::NegativeY:
    case cr::CreativeVolumeFace::PositiveY: return cr::CreativeAxis3::Y;
    case cr::CreativeVolumeFace::NegativeZ:
    case cr::CreativeVolumeFace::PositiveZ: return cr::CreativeAxis3::Z;
    case cr::CreativeVolumeFace::Count: break;
  }
  return cr::CreativeAxis3::Count;
}

[[nodiscard]] iggy3d::Vec3 axisVector(cr::CreativeAxis3 axis,
                                     float sign = 1.0F) noexcept {
  switch (axis) {
    case cr::CreativeAxis3::X: return {sign, 0.0F, 0.0F};
    case cr::CreativeAxis3::Y: return {0.0F, sign, 0.0F};
    case cr::CreativeAxis3::Z: return {0.0F, 0.0F, sign};
    case cr::CreativeAxis3::Count: break;
  }
  return {};
}

[[nodiscard]] bool negativeFace(cr::CreativeVolumeFace face) noexcept {
  return face == cr::CreativeVolumeFace::NegativeX ||
         face == cr::CreativeVolumeFace::NegativeY ||
         face == cr::CreativeVolumeFace::NegativeZ;
}

[[nodiscard]] bool sameGridBounds(const cr::CreativeVolumeSelection& lhs,
                                  const cr::CreativeVolumeSelection& rhs) noexcept {
  if (!cr::creativeVolumeSelectionComplete(lhs) ||
      !cr::creativeVolumeSelectionComplete(rhs)) {
    return lhs.phase == rhs.phase;
  }
  const cr::CreativeGridBounds3 left = cr::creativeVolumeGridBounds(lhs);
  const cr::CreativeGridBounds3 right = cr::creativeVolumeGridBounds(rhs);
  return left.min.x == right.min.x && left.min.y == right.min.y &&
         left.min.z == right.min.z && left.max.x == right.max.x &&
         left.max.y == right.max.y && left.max.z == right.max.z;
}

[[nodiscard]] bool projectVolumeHandle(
    CreativeEditorVolumeHandle& handle,
    const iggy3d::RenderCameraFrame& camera,
    iggy3d::RenderContentViewport content,
    const cr::CreativeScreenPoint& projectedCenter) noexcept {
  const cr::CreativeScreenPoint projected =
      cr::projectCreativeWorldPointToScreen(camera.clipFromWorld,
                                            handle.worldPosition,
                                            content.width, content.height);
  if (!projected.valid || !projected.insideViewport) {
    return false;
  }
  const float localDx = projected.x - projectedCenter.x;
  const float localDy = projected.y - projectedCenter.y;
  if (localDx * localDx + localDy * localDy < 16.0F) {
    return false;
  }
  handle.pixelX = static_cast<float>(content.x) + projected.x;
  handle.pixelY = static_cast<float>(content.y) + projected.y;
  handle.valid = std::isfinite(handle.pixelX) && std::isfinite(handle.pixelY);
  return handle.valid;
}

[[nodiscard]] float pointSegmentDistanceSquared(
    float pointX,
    float pointY,
    float startX,
    float startY,
    float endX,
    float endY) noexcept {
  const float dx = endX - startX;
  const float dy = endY - startY;
  const float lengthSquared = dx * dx + dy * dy;
  if (!std::isfinite(lengthSquared) || lengthSquared <= 1.0e-4F) {
    return std::numeric_limits<float>::max();
  }
  const float t = std::clamp(
      ((pointX - startX) * dx + (pointY - startY) * dy) / lengthSquared,
      0.0F, 1.0F);
  const float nearestX = startX + dx * t;
  const float nearestY = startY + dy * t;
  const float offsetX = pointX - nearestX;
  const float offsetY = pointY - nearestY;
  return offsetX * offsetX + offsetY * offsetY;
}

[[nodiscard]] double halfExtentForAxis(
    const cr::CreativeBoundsMetrics& metrics,
    cr::CreativeAxis3 axis) noexcept {
  switch (axis) {
    case cr::CreativeAxis3::X: return metrics.size.x * 0.5;
    case cr::CreativeAxis3::Y: return metrics.size.y * 0.5;
    case cr::CreativeAxis3::Z: return metrics.size.z * 0.5;
    case cr::CreativeAxis3::Count: break;
  }
  return 0.0;
}

[[nodiscard]] std::int32_t clampedResizeDelta(
    const cr::CreativeVolumeSelection& selection,
    cr::CreativeVolumeFace face,
    std::int32_t delta) noexcept {
  const cr::CreativeVolumeRegionFacts facts =
      cr::inspectCreativeVolumeRegion(selection);
  if (!facts.valid) {
    return 0;
  }
  std::int32_t dimension = 1;
  switch (axisForFace(face)) {
    case cr::CreativeAxis3::X: dimension = facts.dimensions.x; break;
    case cr::CreativeAxis3::Y: dimension = facts.dimensions.y; break;
    case cr::CreativeAxis3::Z: dimension = facts.dimensions.z; break;
    case cr::CreativeAxis3::Count: return 0;
  }
  return std::max(delta, 1 - dimension);
}

}  // namespace

void activateCreativeEditorVolumeMode(CreativeEditorVolumeState& state,
                                      double cellSize,
                                      creative::CreativeVec3 origin) {
  if (!sameGrid(state, cellSize, origin)) {
    static_cast<void>(finishCreativeEditorVolumeHandleGesture(state, false));
    creative::clearCreativeVolumeSelection(state.selection);
    state.selection.cellSize = cellSize;
    state.selection.origin = origin;
    state.cursorValid = false;
  }
  state.active = true;
  state.lastReceipt = {};
}

void deactivateCreativeEditorVolumeMode(
  CreativeEditorVolumeState& state) noexcept {
  static_cast<void>(finishCreativeEditorVolumeHandleGesture(state, false));
  state.active = false;
}

creative::CreativeVolumeSelection creativeEditorVolumePreviewSelection(
    const CreativeEditorVolumeState& state) noexcept {
  if (!state.active || !state.cursorValid) {
    return state.selection;
  }
  return creative::previewCreativeVolumeSelection(state.selection,
                                                   state.cursorCell);
}

creative::CreativeVolumeOperationRequest
makeCreativeEditorVolumeOperationRequest(
    const CreativeEditorVolumeState& state,
    const creative::CreativeVolumeSelection& selection,
    creative::CreativeObjectKind brushKind,
    const creative::CreativeToolSettings& toolSettings) noexcept {
  creative::CreativeVolumeOperationRequest request;
  request.operation = state.operation;
  request.selection = selection;
  request.objectKind = brushKind;
  request.maxAffectedObjects = kCreativeEditorVolumeCellLimit;
  if (!creative::applyCreativeToolSettingsToVolumeRequest(request,
                                                           toolSettings)) {
    request.maxAffectedObjects = 0U;
  }
  return request;
}

const creative::CreativeVolumeOperationReceipt&
refreshCreativeEditorVolumeOperationPreview(
    CreativeEditorVolumeState& state,
    const creative::CreativeDocument& document,
    const creative::CreativeVolumeSelection& selection,
    creative::CreativeObjectKind brushKind,
    const creative::CreativeToolSettings& toolSettings) {
  const creative::CreativeVolumeOperationRequest request =
      makeCreativeEditorVolumeOperationRequest(state, selection, brushKind,
                                               toolSettings);
  if (state.preview.valid && state.preview.documentId == document.id() &&
      state.preview.documentRevision == document.revision() &&
      sameRequest(state.preview.request, request)) {
    return state.preview.receipt;
  }

  state.preview.valid = true;
  state.preview.documentId = document.id();
  state.preview.documentRevision = document.revision();
  state.preview.request = request;
  state.preview.stagedDocument = document;
  state.preview.receipt = creative::executeCreativeVolumeOperation(
      state.preview.stagedDocument, request);
  state.preview.stagedDocumentValid =
      state.preview.receipt.accepted && state.preview.receipt.changed;
  ++state.preview.refreshCount;
  return state.preview.receipt;
}

void invalidateCreativeEditorVolumeOperationPreview(
    CreativeEditorVolumeState& state) noexcept {
  state.preview.valid = false;
  state.preview.stagedDocumentValid = false;
}

CreativeEditorVolumeHandleFrame buildCreativeEditorVolumeHandleFrame(
    const CreativeEditorVolumeState& state,
    const iggy3d::RenderCameraFrame& camera,
    iggy3d::RenderContentViewport content) noexcept {
  CreativeEditorVolumeHandleFrame frame;
  if (!state.active || !cr::creativeVolumeSelectionValid(state.selection) ||
      content.width == 0U || content.height == 0U) {
    return frame;
  }
  const cr::CreativeBounds bounds =
      cr::creativeVolumeWorldBounds(state.selection);
  const cr::CreativeBoundsMetrics metrics = cr::measureCreativeBounds(bounds);
  const cr::CreativeCoreVec3Conversion center =
      cr::creativeVec3ToCoreChecked(metrics.center);
  if (!metrics.valid || !center.converted) {
    return frame;
  }
  frame.center = center.value;
  const cr::CreativeScreenPoint projectedCenter =
      cr::projectCreativeWorldPointToScreen(camera.clipFromWorld, frame.center,
                                            content.width, content.height);
  if (!projectedCenter.valid) {
    return frame;
  }
  frame.centerPixelX = static_cast<float>(content.x) + projectedCenter.x;
  frame.centerPixelY = static_cast<float>(content.y) + projectedCenter.y;

  constexpr std::array faces{
      cr::CreativeVolumeFace::NegativeX,
      cr::CreativeVolumeFace::PositiveX,
      cr::CreativeVolumeFace::NegativeY,
      cr::CreativeVolumeFace::PositiveY,
      cr::CreativeVolumeFace::NegativeZ,
      cr::CreativeVolumeFace::PositiveZ,
  };
  for (std::size_t index = 0U; index < faces.size(); ++index) {
    const cr::CreativeVolumeFace face = faces[index];
    const cr::CreativeAxis3 axis = axisForFace(face);
    const float sign = negativeFace(face) ? -1.0F : 1.0F;
    iggy3d::Vec3 position = frame.center;
    switch (face) {
      case cr::CreativeVolumeFace::NegativeX:
        position.x = static_cast<float>(bounds.min.x);
        break;
      case cr::CreativeVolumeFace::PositiveX:
        position.x = static_cast<float>(bounds.max.x);
        break;
      case cr::CreativeVolumeFace::NegativeY:
        position.y = static_cast<float>(bounds.min.y);
        break;
      case cr::CreativeVolumeFace::PositiveY:
        position.y = static_cast<float>(bounds.max.y);
        break;
      case cr::CreativeVolumeFace::NegativeZ:
        position.z = static_cast<float>(bounds.min.z);
        break;
      case cr::CreativeVolumeFace::PositiveZ:
        position.z = static_cast<float>(bounds.max.z);
        break;
      case cr::CreativeVolumeFace::Count: break;
    }
    frame.handles[index] = {CreativeEditorVolumeHandleKind::ResizeFace,
                            axis, face, position, axisVector(axis, sign)};
    static_cast<void>(projectVolumeHandle(frame.handles[index], camera, content,
                                          projectedCenter));
  }

  constexpr std::array axes{
      cr::CreativeAxis3::X,
      cr::CreativeAxis3::Y,
      cr::CreativeAxis3::Z,
  };
  for (std::size_t index = 0U; index < axes.size(); ++index) {
    const float handleLength = static_cast<float>(
        halfExtentForAxis(metrics, axes[index]) +
        std::max(0.5, state.selection.cellSize * 0.5));
    const iggy3d::Vec3 direction = axisVector(axes[index]);
    CreativeEditorVolumeHandle& handle = frame.handles[faces.size() + index];
    handle = {CreativeEditorVolumeHandleKind::MoveAxis,
              axes[index], cr::CreativeVolumeFace::Count,
              frame.center + direction * handleLength, direction};
    static_cast<void>(
        projectVolumeHandle(handle, camera, content, projectedCenter));
  }
  frame.valid = std::any_of(
      frame.handles.begin(), frame.handles.end(),
      [](const CreativeEditorVolumeHandle& handle) { return handle.valid; });
  return frame;
}

CreativeEditorVolumeHandlePick pickCreativeEditorVolumeHandle(
    const CreativeEditorVolumeHandleFrame& frame,
    float pixelX,
    float pixelY,
    float hitRadiusPixels) noexcept {
  CreativeEditorVolumeHandlePick picked;
  picked.distanceSquared = std::numeric_limits<float>::max();
  if (!frame.valid || !std::isfinite(pixelX) || !std::isfinite(pixelY) ||
      !std::isfinite(hitRadiusPixels) || hitRadiusPixels < 0.0F) {
    return picked;
  }
  const float threshold = hitRadiusPixels * hitRadiusPixels;
  for (std::size_t index = 0U; index < frame.handles.size(); ++index) {
    const CreativeEditorVolumeHandle& handle = frame.handles[index];
    if (!handle.valid) {
      continue;
    }
    float distanceSquared = 0.0F;
    if (handle.kind == CreativeEditorVolumeHandleKind::MoveAxis) {
      constexpr float kCenterExclusion = 0.22F;
      const float startX =
          std::lerp(frame.centerPixelX, handle.pixelX, kCenterExclusion);
      const float startY =
          std::lerp(frame.centerPixelY, handle.pixelY, kCenterExclusion);
      distanceSquared = pointSegmentDistanceSquared(
          pixelX, pixelY, startX, startY, handle.pixelX, handle.pixelY);
    } else {
      const float dx = pixelX - handle.pixelX;
      const float dy = pixelY - handle.pixelY;
      distanceSquared = dx * dx + dy * dy;
    }
    if (distanceSquared <= threshold &&
        distanceSquared < picked.distanceSquared) {
      picked.hit = true;
      picked.index = index;
      picked.distanceSquared = distanceSquared;
    }
  }
  return picked;
}

bool beginCreativeEditorVolumeHandleGesture(
    CreativeEditorVolumeState& state,
    const CreativeEditorVolumeHandle& handle,
    cr::CreativeVec3 rayOrigin,
    cr::CreativeVec3 rayDirection) noexcept {
  if (!handle.valid || handle.kind == CreativeEditorVolumeHandleKind::None ||
      !cr::creativeVolumeSelectionValid(state.selection)) {
    return false;
  }
  const cr::CreativeVec3 axisOrigin = cr::creativeVec3FromCore(handle.worldPosition);
  const cr::CreativeVec3 axisDirection =
      cr::creativeVec3FromCore(handle.axisDirection);
  const CreativeEditorTransformAxisRaySample sample =
      sampleCreativeEditorTransformAxisRay(rayOrigin, rayDirection, axisOrigin,
                                           axisDirection);
  if (!sample.valid) {
    return false;
  }
  state.handleGesture = {};
  state.handleGesture.active = true;
  state.handleGesture.handle = handle;
  state.handleGesture.initialSelection = state.selection;
  state.handleGesture.axisOrigin = axisOrigin;
  state.handleGesture.axisDirection = axisDirection;
  state.handleGesture.initialAxisParameter = sample.axisParameter;
  return true;
}

bool updateCreativeEditorVolumeHandleGesture(
    CreativeEditorVolumeState& state,
    cr::CreativeVec3 rayOrigin,
    cr::CreativeVec3 rayDirection) noexcept {
  CreativeEditorVolumeHandleGesture& gesture = state.handleGesture;
  if (!gesture.active || gesture.initialSelection.cellSize <= 0.0) {
    return false;
  }
  const CreativeEditorTransformAxisRaySample sample =
      sampleCreativeEditorTransformAxisRay(
          rayOrigin, rayDirection, gesture.axisOrigin, gesture.axisDirection);
  if (!sample.valid) {
    return false;
  }
  const double rawDelta =
      (sample.axisParameter - gesture.initialAxisParameter) /
      gesture.initialSelection.cellSize;
  if (!std::isfinite(rawDelta) ||
      rawDelta < static_cast<double>(std::numeric_limits<std::int32_t>::min()) ||
      rawDelta > static_cast<double>(std::numeric_limits<std::int32_t>::max())) {
    return false;
  }
  std::int32_t delta = static_cast<std::int32_t>(std::llround(rawDelta));
  if (gesture.handle.kind == CreativeEditorVolumeHandleKind::ResizeFace) {
    delta = clampedResizeDelta(gesture.initialSelection, gesture.handle.face,
                               delta);
  }
  if (delta == gesture.appliedDeltaCells) {
    return false;
  }

  cr::CreativeVolumeSelection candidate = gesture.initialSelection;
  bool applied = delta == 0;
  if (gesture.handle.kind == CreativeEditorVolumeHandleKind::MoveAxis) {
    cr::CreativeGridCoord3 move{};
    switch (gesture.handle.axis) {
      case cr::CreativeAxis3::X: move.x = delta; break;
      case cr::CreativeAxis3::Y: move.y = delta; break;
      case cr::CreativeAxis3::Z: move.z = delta; break;
      case cr::CreativeAxis3::Count: return false;
    }
    applied = delta == 0 || cr::moveCreativeVolumeSelection(candidate, move);
  } else if (gesture.handle.kind ==
             CreativeEditorVolumeHandleKind::ResizeFace) {
    applied = delta == 0 || cr::resizeCreativeVolumeSelectionFace(
                                candidate, gesture.handle.face, delta);
  }
  if (!applied || sameGridBounds(state.selection, candidate)) {
    gesture.appliedDeltaCells = delta;
    return false;
  }
  state.selection = candidate;
  state.lastReceipt = {};
  invalidateCreativeEditorVolumeOperationPreview(state);
  gesture.appliedDeltaCells = delta;
  return true;
}

bool finishCreativeEditorVolumeHandleGesture(CreativeEditorVolumeState& state,
                                             bool commit) noexcept {
  if (!state.handleGesture.active) {
    return false;
  }
  const cr::CreativeVolumeSelection initial =
      state.handleGesture.initialSelection;
  const bool changed = !sameGridBounds(state.selection, initial);
  state.handleGesture = {};
  if (!commit && changed) {
    state.selection = initial;
    state.lastReceipt = {};
    invalidateCreativeEditorVolumeOperationPreview(state);
  }
  return changed;
}

CreativeEditorVolumeGestureReceipt stepCreativeEditorVolumeGesture(
    CreativeEditorVolumeState& state,
    CreativeEditorVolumeGestureAction action,
    bool targetValid,
    creative::CreativeGridCoord3 targetCell) noexcept {
  CreativeEditorVolumeGestureReceipt receipt;
  receipt.requested = true;
  receipt.action = action;
  receipt.phaseBefore = state.selection.phase;
  receipt.phaseAfter = state.selection.phase;
  if (!targetValid) {
    return receipt;
  }

  switch (action) {
    case CreativeEditorVolumeGestureAction::Begin:
      creative::clearCreativeVolumeSelection(state.selection);
      static_cast<void>(creative::setCreativeVolumeSelectionCorner(
          state.selection, creative::CreativeVolumeCorner::First, targetCell));
      state.lastReceipt = {};
      receipt.accepted = true;
      receipt.status = CreativeEditorVolumeGestureStatus::Began;
      receipt.reasonCode = "creative_volume_gesture_began";
      break;
    case CreativeEditorVolumeGestureAction::Commit:
      if (state.selection.phase !=
          creative::CreativeVolumeSelectionPhase::FirstCorner) {
        receipt.status = CreativeEditorVolumeGestureStatus::NotArmed;
        receipt.reasonCode = "creative_volume_gesture_not_armed";
        break;
      }
      static_cast<void>(creative::setCreativeVolumeSelectionCorner(
          state.selection, creative::CreativeVolumeCorner::Second, targetCell));
      state.lastReceipt = {};
      receipt.accepted = true;
      receipt.status = CreativeEditorVolumeGestureStatus::Completed;
      receipt.reasonCode = "creative_volume_gesture_completed";
      break;
  }
  receipt.phaseAfter = state.selection.phase;
  return receipt;
}

creative::CreativeVolumeOperationReceipt
applyCreativeEditorVolumeOperationWithHistory(
    creative::CreativeAppState& appState,
    CreativeEditorVolumeState& state,
    creative::CreativeObjectKind brushKind,
    creative::CreativeVolumeOperationKind operation,
    const creative::CreativeToolSettings& toolSettings,
    std::string_view source) {
  state.operation = operation;
  const creative::CreativeVolumeOperationRequest request =
      makeCreativeEditorVolumeOperationRequest(state, state.selection,
                                               brushKind, toolSettings);

  creative::CreativeHistoryRecordReceipt historyReceipt;
  state.lastReceipt = creative::previewCreativeVolumeOperation(
      appState.facade.document(), request);
  if (state.lastReceipt.accepted && state.lastReceipt.changed) {
    std::optional<creative::CreativeAuthoringOperationRecord> operationRecord =
        creative::makeCreativeAuthoringOperationRecord(
            creative::CreativeAuthoringFamily::Volume,
            creative::CreativeAuthoringOperationKind::Apply,
            creative::toString(request.operation),
            creative::fingerprintCreativeVolumeOperationRequest(request),
            creative::creativeVolumeChangedMemberCount(state.lastReceipt));
    if (!operationRecord.has_value()) {
      state.lastReceipt.accepted = false;
      state.lastReceipt.changed = false;
      state.lastReceipt.status =
          creative::CreativeVolumeOperationStatus::InvalidRequest;
      state.lastReceipt.reasonCode =
          "creative_volume_operation_record_invalid";
      return state.lastReceipt;
    }
    creative::CreativeDocumentHistoryTransaction transaction =
        creative::beginCreativeHistoryTransaction(
            appState.facade, source, std::move(*operationRecord));
    state.lastReceipt = appState.facade.applyVolumeOperation(request);
    historyReceipt = completeEditTransaction(
        appState.history, std::move(transaction), appState.facade,
        state.lastReceipt.accepted && state.lastReceipt.changed,
        state.lastReceipt.reasonCode);
    invalidateCreativeEditorVolumeOperationPreview(state);
  }

  SDL_Log("iggy3d_creative: VOLUME operation='%s' shape='%s' axis='%s' "
          "material='%s' overlap='%s' shell='%s' alignment='%s' "
          "opening='%s' corners='%s' source='%s' members='%s' "
          "cloneOffset=(%.3f,%.3f,%.3f) cloneTurns=%u cloneMirrorX=%d "
          "cloneMirrorZ=%d cloneVoxelOverlap='%s' "
          "status='%s' accepted=%d changed=%d candidates=%llu planned=%llu "
          "matchedObjects=%llu matchedVoxels=%llu unchangedObjects=%llu "
          "unchangedVoxels=%llu excludedObjects=%llu excludedVoxels=%llu "
          "protectedObjects=%llu dependentSources=%llu blockedObjects=%llu "
          "blockedVoxels=%llu clonedLinks=%llu clonedRecipes=%llu "
          "createdObjects=%llu removedObjects=%llu replacedObjects=%llu "
          "createdVoxels=%llu removedVoxels=%llu replacedVoxels=%llu "
          "dirtyChunks=%llu undoRecorded=%d "
          "reasonCode='%s'",
          std::string(creative::toString(operation)).c_str(),
          std::string(creative::toString(state.lastReceipt.shapeKind)).c_str(),
          std::string(creative::toString(state.lastReceipt.shapeAxis)).c_str(),
          std::string(creative::toString(state.lastReceipt.objectKind)).c_str(),
          std::string(creative::toString(
                          state.lastReceipt.fillOverlapPolicy))
              .c_str(),
          std::string(creative::toString(state.lastReceipt.hollowThickness))
              .c_str(),
          std::string(creative::toString(state.lastReceipt.hollowAlignment))
              .c_str(),
          std::string(creative::toString(state.lastReceipt.hollowOpening))
              .c_str(),
          std::string(creative::toString(state.lastReceipt.hollowCornerRule))
              .c_str(),
          std::string(state.lastReceipt.operation ==
                                  creative::CreativeVolumeOperationKind::Replace &&
                              request.hasReplaceKindFilter
                          ? creative::toString(request.replaceKindFilter)
                          : state.lastReceipt.operation ==
                                        creative::CreativeVolumeOperationKind::Erase &&
                                    request.hasEraseKindFilter
                                ? creative::toString(request.eraseKindFilter)
                                : std::string_view{"ANY"})
              .c_str(),
          std::string(creative::toString(
                          state.lastReceipt.operation ==
                                  creative::CreativeVolumeOperationKind::Erase
                              ? state.lastReceipt.eraseMemberMask
                              : state.lastReceipt.operation ==
                                        creative::CreativeVolumeOperationKind::Clone
                                    ? state.lastReceipt.cloneMemberMask
                              : state.lastReceipt.replaceMemberMask))
              .c_str(),
          state.lastReceipt.cloneOffset.x, state.lastReceipt.cloneOffset.y,
          state.lastReceipt.cloneOffset.z,
          static_cast<unsigned>(state.lastReceipt.cloneQuarterTurns),
          state.lastReceipt.cloneMirrorX ? 1 : 0,
          state.lastReceipt.cloneMirrorZ ? 1 : 0,
          std::string(creative::toString(
                          state.lastReceipt.cloneVoxelOverlapPolicy))
              .c_str(),
          std::string(creative::toString(state.lastReceipt.status)).c_str(),
          state.lastReceipt.accepted ? 1 : 0,
          state.lastReceipt.changed ? 1 : 0,
          static_cast<unsigned long long>(
              state.lastReceipt.shapeCandidateCellCount),
          static_cast<unsigned long long>(state.lastReceipt.plannedCellCount),
          static_cast<unsigned long long>(state.lastReceipt.matchedObjectCount),
          static_cast<unsigned long long>(
              state.lastReceipt.matchedVoxelCellCount),
          static_cast<unsigned long long>(
              state.lastReceipt.unchangedObjectCount),
          static_cast<unsigned long long>(
              state.lastReceipt.unchangedMaterialCellCount),
          static_cast<unsigned long long>(
              state.lastReceipt.excludedObjectCount),
          static_cast<unsigned long long>(
              state.lastReceipt.excludedVoxelCellCount),
          static_cast<unsigned long long>(
              state.lastReceipt.protectedObjectCount),
          static_cast<unsigned long long>(
              state.lastReceipt.dependentSourceObjectCount),
          static_cast<unsigned long long>(
              state.lastReceipt.blockedObjectIds.size()),
          static_cast<unsigned long long>(
              state.lastReceipt.blockedVoxelCells.size()),
          static_cast<unsigned long long>(
              state.lastReceipt.clonedLogicLinkCount),
          static_cast<unsigned long long>(
              state.lastReceipt.clonedPatternRecipeCount),
          static_cast<unsigned long long>(state.lastReceipt.createdObjectCount),
          static_cast<unsigned long long>(state.lastReceipt.removedObjectCount),
          static_cast<unsigned long long>(
              state.lastReceipt.replacedObjectCount),
          static_cast<unsigned long long>(
              state.lastReceipt.createdVoxelCellCount),
          static_cast<unsigned long long>(
              state.lastReceipt.removedVoxelCellCount),
          static_cast<unsigned long long>(
              state.lastReceipt.replacedVoxelCellCount),
          static_cast<unsigned long long>(
              state.lastReceipt.dirtyVoxelChunkCount),
          historyReceipt.recorded ? 1 : 0,
          std::string(state.lastReceipt.reasonCode).c_str());
  return state.lastReceipt;
}

}  // namespace iggy3d_creative_app
