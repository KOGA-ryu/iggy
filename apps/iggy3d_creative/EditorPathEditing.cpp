#include "EditorPathEditing.hpp"

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/mutation/Mutation.hpp"

#include "EditorPlacement.hpp"
#include "app/iggy3d/creative/tools/Select.hpp"

#include <SDL3/SDL_log.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace iggy3d_creative_app {
namespace {

constexpr double kPathPointEpsilonMeters = 1.0e-5;

[[nodiscard]] bool finitePoint(cr::CreativeVec3 point) noexcept {
  return std::isfinite(point.x) && std::isfinite(point.y) &&
         std::isfinite(point.z);
}

[[nodiscard]] bool samePathPoint(cr::CreativeVec3 lhs,
                                 cr::CreativeVec3 rhs) noexcept {
  return std::hypot(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z) <=
         kPathPointEpsilonMeters;
}

[[nodiscard]] bool validPathEditCommand(
    CreativeMovingPlatformPathEditCommand command) noexcept {
  return command == CreativeMovingPlatformPathEditCommand::AppendAtTarget ||
         command == CreativeMovingPlatformPathEditCommand::RemoveLast;
}

[[nodiscard]] cr::CreativeObjectId selectedMovingPlatformId(
    const cr::CreativeAppState& appState) noexcept {
  const cr::CreativeSelectionState& selection =
      appState.facade.selectionState();
  if (cr::selectedTargetCount(selection) != 1U ||
      selection.selectedTarget.value == cr::kInvalidId) {
    return cr::kInvalidObjectId;
  }
  const auto objectId =
      static_cast<cr::CreativeObjectId>(selection.selectedTarget.value);
  const cr::CreativeObject* object = appState.facade.findObject(objectId);
  return object != nullptr &&
                 object->kind == cr::CreativeObjectKind::MovingPlatform
             ? objectId
             : cr::kInvalidObjectId;
}

[[nodiscard]] std::optional<cr::CreativeVec3>
movingPlatformPathPointAtPlacementAnchor(
    const cr::CreativeObject& object,
    cr::CreativeVec3 placementAnchor) noexcept {
  if (object.kind != cr::CreativeObjectKind::MovingPlatform ||
      !finitePoint(placementAnchor)) {
    return std::nullopt;
  }
  const cr::CreativeTransformedBounds resolved =
      cr::resolveCreativeObjectBounds(object);
  if (!resolved.valid) {
    return std::nullopt;
  }
  const cr::CreativeVec3 point{
      placementAnchor.x,
      placementAnchor.y + resolved.center.y - resolved.worldBounds.min.y,
      placementAnchor.z};
  return finitePoint(point) ? std::optional<cr::CreativeVec3>{point}
                            : std::nullopt;
}

[[nodiscard]] CreativeMovingPlatformPathEditReceipt applyPathEditWithUndo(
    cr::CreativeAppState& appState,
    cr::CreativeObjectId objectId,
    CreativeMovingPlatformPathEditCommand command,
    cr::CreativeVec3 targetPoint,
    std::string_view source) {
  CreativeMovingPlatformPathEditReceipt result;
  result.requested = true;
  result.objectId = objectId;
  const cr::CreativeObject* object = appState.facade.findObject(objectId);
  if (object == nullptr ||
      object->kind != cr::CreativeObjectKind::MovingPlatform) {
    result.status = CreativeMovingPlatformPathEditStatus::InvalidSelection;
    result.reasonCode = "creative_platform_path_edit_invalid_selection";
    return result;
  }

  result.pointCountBefore = object->pathPoints.size();
  CreativeMovingPlatformPathEditPlan plan =
      planCreativeMovingPlatformPathEdit(object->pathPoints, command,
                                         targetPoint);
  result.status = plan.status;
  result.reasonCode = plan.reasonCode;
  result.pointCountAfter = result.pointCountBefore;
  if (!plan.accepted) {
    return result;
  }

  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  result.mutation = cr::applyDocumentMutation(
      appState.facade.documentForPersistence(), objectId,
      cr::CreativeMutationKind::SetPatrolRoute,
      cr::makePathPointsPayload(std::move(plan.pathPoints)));
  result.changed = result.mutation.status ==
                       cr::CreativeDocumentMutationStatus::Applied &&
                   result.mutation.changed;
  result.accepted = result.changed;
  result.history = completeEditTransaction(
      appState.history, std::move(transaction), appState.facade, result.changed,
      result.mutation.message);
  const cr::CreativeObject* after = appState.facade.findObject(objectId);
  result.pointCountAfter =
      after != nullptr ? after->pathPoints.size() : result.pointCountBefore;
  if (result.changed) {
    result.status = CreativeMovingPlatformPathEditStatus::Applied;
    result.reasonCode =
        command == CreativeMovingPlatformPathEditCommand::AppendAtTarget
            ? "creative_platform_path_point_appended"
            : "creative_platform_path_point_removed";
  } else {
    result.status = CreativeMovingPlatformPathEditStatus::MutationRejected;
    result.reasonCode = "creative_platform_path_edit_mutation_rejected";
  }
  return result;
}

std::vector<cr::CreativePathPoint> translatePathPoints(
    const std::vector<cr::CreativePathPoint>& points,
    cr::CreativeVec3 delta) {
  std::vector<cr::CreativePathPoint> translated;
  translated.reserve(points.size());
  for (const cr::CreativePathPoint& point : points) {
    translated.push_back(cr::CreativePathPoint{
        {point.position.x + delta.x,
         point.position.y + delta.y,
         point.position.z + delta.z}});
  }
  return translated;
}

std::vector<cr::CreativePathPoint> movePathPoint(
    const std::vector<cr::CreativePathPoint>& points,
    std::size_t pointIndex,
    cr::CreativeVec3 delta) {
  std::vector<cr::CreativePathPoint> moved = points;
  if (pointIndex < moved.size()) {
    cr::CreativeVec3& position = moved[pointIndex].position;
    position.x += delta.x;
    position.y += delta.y;
    position.z += delta.z;
  }
  return moved;
}

}  // namespace

CreativeMovingPlatformPathEditPlan planCreativeMovingPlatformPathEdit(
    std::span<const cr::CreativePathPoint> currentPath,
    CreativeMovingPlatformPathEditCommand command,
    cr::CreativeVec3 targetPoint) {
  CreativeMovingPlatformPathEditPlan result;
  if (!validPathEditCommand(command)) {
    return result;
  }
  if (!cr::isValidCreativeMovingPlatformPath(currentPath)) {
    result.status = CreativeMovingPlatformPathEditStatus::InvalidPath;
    result.reasonCode = "creative_platform_path_edit_invalid_path";
    return result;
  }

  if (command == CreativeMovingPlatformPathEditCommand::AppendAtTarget) {
    if (!finitePoint(targetPoint)) {
      result.status = CreativeMovingPlatformPathEditStatus::InvalidTarget;
      result.reasonCode = "creative_platform_path_edit_invalid_target";
      return result;
    }
    if (currentPath.size() >= cr::kCreativeMovingPlatformPathPointCapacity) {
      result.status = CreativeMovingPlatformPathEditStatus::CapacityReached;
      result.reasonCode = "creative_platform_path_edit_capacity_reached";
      return result;
    }
    if (samePathPoint(currentPath.back().position, targetPoint)) {
      result.status = CreativeMovingPlatformPathEditStatus::DuplicateTarget;
      result.reasonCode = "creative_platform_path_edit_duplicate_target";
      return result;
    }
  } else {
    if (currentPath.size() <= 2U) {
      result.status = CreativeMovingPlatformPathEditStatus::MinimumPointCount;
      result.reasonCode = "creative_platform_path_edit_minimum_point_count";
      return result;
    }
  }

  result.pathPoints.assign(currentPath.begin(), currentPath.end());
  if (command == CreativeMovingPlatformPathEditCommand::AppendAtTarget) {
    result.pathPoints.push_back({targetPoint});
  } else {
    result.pathPoints.pop_back();
  }

  if (!cr::isValidCreativeMovingPlatformPath(result.pathPoints)) {
    result.status = CreativeMovingPlatformPathEditStatus::InvalidPath;
    result.reasonCode = "creative_platform_path_edit_result_invalid";
    result.pathPoints.clear();
    return result;
  }
  result.accepted = true;
  result.changed = true;
  result.status = CreativeMovingPlatformPathEditStatus::Ready;
  result.reasonCode = "creative_platform_path_edit_planned";
  return result;
}

void syncCreativeMovingPlatformPathEditState(
    const cr::CreativeAppState& appState,
    CreativeMovingPlatformPathEditState& state) noexcept {
  const cr::CreativeDocumentId documentId = appState.facade.document().id();
  const cr::CreativeObjectId objectId = selectedMovingPlatformId(appState);
  const bool selectionChanged = state.documentId != documentId ||
                                state.objectId != objectId;
  state.documentId = documentId;
  state.objectId = objectId;
  state.available = objectId != cr::kInvalidObjectId;
  const cr::CreativeObject* object =
      state.available ? appState.facade.findObject(objectId) : nullptr;
  state.pointCount = object != nullptr
                         ? static_cast<std::uint8_t>(std::min<std::size_t>(
                               object->pathPoints.size(),
                               std::numeric_limits<std::uint8_t>::max()))
                         : 0U;
  if (selectionChanged) {
    state.pending = CreativeMovingPlatformPathEditCommand::None;
    state.status = state.available
                       ? CreativeMovingPlatformPathEditStatus::Ready
                       : CreativeMovingPlatformPathEditStatus::Idle;
    state.reasonCode = state.available
                           ? "creative_platform_path_edit_ready"
                           : "creative_platform_path_edit_idle";
  }
}

bool queueCreativeMovingPlatformPathEdit(
    const cr::CreativeAppState& appState,
    CreativeMovingPlatformPathEditState& state,
    CreativeMovingPlatformPathEditCommand command) noexcept {
  syncCreativeMovingPlatformPathEditState(appState, state);
  if (!state.available || !validPathEditCommand(command)) {
    state.pending = CreativeMovingPlatformPathEditCommand::None;
    state.status = CreativeMovingPlatformPathEditStatus::InvalidSelection;
    state.reasonCode = "creative_platform_path_edit_invalid_selection";
    return false;
  }
  state.pending = command;
  state.status = CreativeMovingPlatformPathEditStatus::Queued;
  state.reasonCode = "creative_platform_path_edit_queued";
  return true;
}

CreativeMovingPlatformPathEditReceipt consumeCreativeMovingPlatformPathEdit(
    cr::CreativeAppState& appState,
    CreativeMovingPlatformPathEditState& state,
    bool targetAvailable,
    cr::CreativeVec3 targetAnchor,
    std::string_view source) {
  const CreativeMovingPlatformPathEditCommand command = state.pending;
  state.pending = CreativeMovingPlatformPathEditCommand::None;
  CreativeMovingPlatformPathEditReceipt result;
  if (!validPathEditCommand(command)) {
    return result;
  }
  syncCreativeMovingPlatformPathEditState(appState, state);
  std::optional<cr::CreativeVec3> targetPoint;
  if (command == CreativeMovingPlatformPathEditCommand::AppendAtTarget &&
      targetAvailable) {
    const cr::CreativeObject* object = appState.facade.findObject(state.objectId);
    if (object != nullptr) {
      targetPoint =
          movingPlatformPathPointAtPlacementAnchor(*object, targetAnchor);
    }
  }
  if (command == CreativeMovingPlatformPathEditCommand::AppendAtTarget &&
      !targetPoint.has_value()) {
    result.requested = true;
    result.objectId = state.objectId;
    result.status = CreativeMovingPlatformPathEditStatus::InvalidTarget;
    result.reasonCode = "creative_platform_path_edit_target_unavailable";
  } else {
    result = applyPathEditWithUndo(appState, state.objectId, command,
                                   targetPoint.value_or(targetAnchor), source);
  }
  syncCreativeMovingPlatformPathEditState(appState, state);
  state.status = result.status;
  state.reasonCode = result.reasonCode;
  return result;
}

cr::CreativeDocumentMutationReceipt movePathObjectWithUndo(
    cr::CreativeAppState& appState,
    StandaloneEditHistory& history,
    cr::CreativeObjectId objectId,
    cr::CreativeVec3 delta,
    std::string_view source) {
  const cr::CreativeObject* beforeObject = appState.facade.findObject(objectId);
  const std::uint64_t undoDepthBefore = cr::creativeUndoDepth(history);
  if (beforeObject == nullptr) {
    SDL_Log("iggy3d_creative: PATH move skipped source='%s' objectId=%llu "
            "reason='missing_object'",
            std::string(source).c_str(),
            static_cast<unsigned long long>(objectId));
    return {};
  }
  const cr::CreativeObjectDescriptor& descriptor =
      cr::describeObject(beforeObject->kind);
  if (!cr::objectStoresPathPoints(beforeObject->kind)) {
    SDL_Log("iggy3d_creative: PATH move skipped source='%s' objectId=%llu "
            "kind='%s' shape='%s'",
            std::string(source).c_str(),
            static_cast<unsigned long long>(objectId),
            std::string(cr::toString(beforeObject->kind)).c_str(),
            std::string(cr::toString(descriptor.shapeKind)).c_str());
    return {};
  }

  const std::vector<cr::CreativePathPoint> beforePoints =
      beforeObject->pathPoints;
  const std::vector<cr::CreativePathPoint> afterPoints =
      translatePathPoints(beforePoints, delta);
  SDL_Log("iggy3d_creative: PATH before move objectId=%llu kind='%s' "
          "shape='%s' projection='%s' pathPointCount=%zu pathPoints='%s' "
          "delta=(%.3f, %.3f, %.3f)",
          static_cast<unsigned long long>(objectId),
          std::string(cr::toString(beforeObject->kind)).c_str(),
          std::string(cr::toString(descriptor.shapeKind)).c_str(),
          std::string(cr::toString(descriptor.projectionProfile)).c_str(),
          beforePoints.size(), pathPointsSummary(beforePoints).c_str(), delta.x,
          delta.y, delta.z);

  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  const cr::CreativeDocumentMutationReceipt receipt =
      cr::applyDocumentMutation(appState.facade.documentForPersistence(),
                                objectId,
                                cr::CreativeMutationKind::SetPatrolRoute,
                                cr::makePathPointsPayload(afterPoints));
  (void)completeEditTransaction(
      history, std::move(transaction), appState.facade,
      receipt.status == cr::CreativeDocumentMutationStatus::Applied &&
          receipt.changed,
      receipt.message);

  const cr::CreativeObject* afterObject = appState.facade.findObject(objectId);
  SDL_Log("iggy3d_creative: PATH move commit source='%s' objectId=%llu "
          "status='%s' allowed=%d changed=%d revisionBefore=%llu "
          "revisionAfter=%llu dirtyFlags=%llu depthBefore=%llu depthAfter=%llu "
          "before='%s' after='%s'",
          std::string(source).c_str(),
          static_cast<unsigned long long>(objectId),
          std::string(cr::toString(receipt.status)).c_str(),
          receipt.allowed ? 1 : 0, receipt.changed ? 1 : 0,
          static_cast<unsigned long long>(receipt.revisionBefore),
          static_cast<unsigned long long>(receipt.revisionAfter),
          static_cast<unsigned long long>(receipt.dirtyFlags),
          static_cast<unsigned long long>(undoDepthBefore),
          static_cast<unsigned long long>(cr::creativeUndoDepth(history)),
          pathPointsSummary(beforePoints).c_str(),
          afterObject != nullptr
              ? pathPointsSummary(afterObject->pathPoints).c_str()
              : "<missing>");
  return receipt;
}

cr::CreativeDocumentMutationReceipt movePathPointWithUndo(
    cr::CreativeAppState& appState,
    StandaloneEditHistory& history,
    cr::CreativeObjectId objectId,
    std::size_t pointIndex,
    cr::CreativeVec3 delta,
    std::string_view source) {
  const cr::CreativeObject* beforeObject = appState.facade.findObject(objectId);
  const std::uint64_t undoDepthBefore = cr::creativeUndoDepth(history);
  if (beforeObject == nullptr) {
    SDL_Log("iggy3d_creative: PATH_HANDLE move skipped source='%s' objectId=%llu "
            "reason='missing_object'",
            std::string(source).c_str(),
            static_cast<unsigned long long>(objectId));
    return {};
  }

  const cr::CreativeObjectDescriptor& descriptor =
      cr::describeObject(beforeObject->kind);
  if (!cr::objectStoresPathPoints(beforeObject->kind)) {
    SDL_Log("iggy3d_creative: PATH_HANDLE move skipped source='%s' objectId=%llu "
            "kind='%s' shape='%s'",
            std::string(source).c_str(),
            static_cast<unsigned long long>(objectId),
            std::string(cr::toString(beforeObject->kind)).c_str(),
            std::string(cr::toString(descriptor.shapeKind)).c_str());
    return {};
  }
  if (pointIndex >= beforeObject->pathPoints.size()) {
    SDL_Log("iggy3d_creative: PATH_HANDLE move skipped source='%s' objectId=%llu "
            "pointIndex=%zu pointCount=%zu reason='invalid_point_index'",
            std::string(source).c_str(),
            static_cast<unsigned long long>(objectId), pointIndex,
            beforeObject->pathPoints.size());
    return {};
  }

  const std::vector<cr::CreativePathPoint> beforePoints =
      beforeObject->pathPoints;
  const std::vector<cr::CreativePathPoint> afterPoints =
      movePathPoint(beforePoints, pointIndex, delta);
  SDL_Log("iggy3d_creative: PATH_HANDLE before move objectId=%llu kind='%s' "
          "shape='%s' pointIndex=%zu delta=(%.3f, %.3f, %.3f) before='%s' "
          "requested='%s'",
          static_cast<unsigned long long>(objectId),
          std::string(cr::toString(beforeObject->kind)).c_str(),
          std::string(cr::toString(descriptor.shapeKind)).c_str(),
          pointIndex, delta.x, delta.y, delta.z,
          pathPointsSummary(beforePoints).c_str(),
          pathPointsSummary(afterPoints).c_str());

  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  const cr::CreativeDocumentMutationReceipt receipt =
      cr::applyDocumentMutation(appState.facade.documentForPersistence(),
                                objectId,
                                cr::CreativeMutationKind::SetPatrolRoute,
                                cr::makePathPointsPayload(afterPoints));
  (void)completeEditTransaction(
      history, std::move(transaction), appState.facade,
      receipt.status == cr::CreativeDocumentMutationStatus::Applied &&
          receipt.changed,
      receipt.message);

  const cr::CreativeObject* afterObject = appState.facade.findObject(objectId);
  SDL_Log("iggy3d_creative: PATH_HANDLE move commit source='%s' objectId=%llu "
          "pointIndex=%zu status='%s' allowed=%d changed=%d "
          "revisionBefore=%llu revisionAfter=%llu dirtyFlags=%llu "
          "depthBefore=%llu depthAfter=%llu before='%s' after='%s'",
          std::string(source).c_str(),
          static_cast<unsigned long long>(objectId), pointIndex,
          std::string(cr::toString(receipt.status)).c_str(),
          receipt.allowed ? 1 : 0, receipt.changed ? 1 : 0,
          static_cast<unsigned long long>(receipt.revisionBefore),
          static_cast<unsigned long long>(receipt.revisionAfter),
          static_cast<unsigned long long>(receipt.dirtyFlags),
          static_cast<unsigned long long>(undoDepthBefore),
          static_cast<unsigned long long>(cr::creativeUndoDepth(history)),
          pathPointsSummary(beforePoints).c_str(),
          afterObject != nullptr
              ? pathPointsSummary(afterObject->pathPoints).c_str()
              : "<missing>");
  return receipt;
}

}  // namespace iggy3d_creative_app
