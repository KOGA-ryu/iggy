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
         command == CreativeMovingPlatformPathEditCommand::RemoveLast ||
         command == CreativeMovingPlatformPathEditCommand::RemoveSelected ||
         command ==
             CreativeMovingPlatformPathEditCommand::MoveSelectedToTarget;
}

[[nodiscard]] bool commandUsesSelectedPoint(
    CreativeMovingPlatformPathEditCommand command) noexcept {
  return command == CreativeMovingPlatformPathEditCommand::RemoveSelected ||
         command ==
             CreativeMovingPlatformPathEditCommand::MoveSelectedToTarget;
}

void applyMoveConstraint(cr::CreativeVec3& target,
                         cr::CreativeVec3 source,
                         cr::CreativeMoveConstraint constraint) noexcept {
  switch (constraint) {
    case cr::CreativeMoveConstraint::Free:
      break;
    case cr::CreativeMoveConstraint::X:
      target.z = source.z;
      break;
    case cr::CreativeMoveConstraint::Z:
      target.x = source.x;
      break;
    case cr::CreativeMoveConstraint::Count:
      break;
  }
}

[[nodiscard]] bool targetKeepsPathValid(
    std::span<const cr::CreativePathPoint> points,
    std::size_t pointIndex,
    cr::CreativeVec3 target) noexcept {
  double totalLengthMeters = 0.0;
  for (std::size_t index = 1U; index < points.size(); ++index) {
    const cr::CreativeVec3 from =
        index - 1U == pointIndex ? target : points[index - 1U].position;
    const cr::CreativeVec3 to =
        index == pointIndex ? target : points[index].position;
    totalLengthMeters +=
        std::hypot(to.x - from.x, to.y - from.y, to.z - from.z);
  }
  return std::isfinite(totalLengthMeters) &&
         totalLengthMeters > kPathPointEpsilonMeters;
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
    std::size_t pointIndex,
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
                                         targetPoint, pointIndex);
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
    switch (command) {
      case CreativeMovingPlatformPathEditCommand::AppendAtTarget:
        result.reasonCode = "creative_platform_path_point_appended";
        break;
      case CreativeMovingPlatformPathEditCommand::RemoveLast:
      case CreativeMovingPlatformPathEditCommand::RemoveSelected:
        result.reasonCode = "creative_platform_path_point_removed";
        break;
      case CreativeMovingPlatformPathEditCommand::MoveSelectedToTarget:
        result.reasonCode = "creative_platform_path_point_moved";
        break;
      case CreativeMovingPlatformPathEditCommand::None:
      case CreativeMovingPlatformPathEditCommand::Count:
        break;
    }
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

CreativeMovingPlatformPathTargetPlan planCreativeMovingPlatformPathTarget(
    const cr::CreativeObject* object,
    bool targetAvailable,
    cr::CreativeVec3 placementAnchor) noexcept {
  CreativeMovingPlatformPathTargetPlan result;
  if (object == nullptr ||
      object->kind != cr::CreativeObjectKind::MovingPlatform) {
    result.status = CreativeMovingPlatformPathEditStatus::InvalidSelection;
    result.reasonCode = "creative_platform_path_edit_invalid_selection";
    return result;
  }
  result.objectId = object->id;
  if (!cr::isValidCreativeMovingPlatformPath(object->pathPoints)) {
    result.status = CreativeMovingPlatformPathEditStatus::InvalidPath;
    result.reasonCode = "creative_platform_path_edit_invalid_path";
    return result;
  }
  if (!targetAvailable) {
    return result;
  }
  const std::optional<cr::CreativeVec3> targetPoint =
      movingPlatformPathPointAtPlacementAnchor(*object, placementAnchor);
  if (!targetPoint.has_value()) {
    result.reasonCode = "creative_platform_path_edit_invalid_target";
    return result;
  }

  result.visible = true;
  result.fromPoint = object->pathPoints.back().position;
  result.targetPoint = *targetPoint;
  result.segmentVisible =
      !samePathPoint(result.fromPoint, result.targetPoint);
  if (object->pathPoints.size() >=
      cr::kCreativeMovingPlatformPathPointCapacity) {
    result.status = CreativeMovingPlatformPathEditStatus::CapacityReached;
    result.reasonCode = "creative_platform_path_edit_capacity_reached";
    return result;
  }
  if (!result.segmentVisible) {
    result.status = CreativeMovingPlatformPathEditStatus::DuplicateTarget;
    result.reasonCode = "creative_platform_path_edit_duplicate_target";
    return result;
  }
  result.appendAllowed = true;
  result.status = CreativeMovingPlatformPathEditStatus::Ready;
  result.reasonCode = "creative_platform_path_edit_planned";
  return result;
}

CreativeMovingPlatformPathPointTargetPlan
planCreativeMovingPlatformPathPointTarget(
    const cr::CreativeObject* object,
    std::size_t pointIndex,
    bool targetAvailable,
    cr::CreativeVec3 placementAnchor,
    cr::CreativeMoveConstraint constraint) noexcept {
  CreativeMovingPlatformPathPointTargetPlan result;
  if (object == nullptr ||
      object->kind != cr::CreativeObjectKind::MovingPlatform) {
    result.status = CreativeMovingPlatformPathEditStatus::InvalidSelection;
    result.reasonCode = "creative_platform_path_edit_invalid_selection";
    return result;
  }
  result.objectId = object->id;
  result.pointIndex = pointIndex;
  if (!cr::isValidCreativeMovingPlatformPath(object->pathPoints)) {
    result.status = CreativeMovingPlatformPathEditStatus::InvalidPath;
    result.reasonCode = "creative_platform_path_edit_invalid_path";
    return result;
  }
  if (pointIndex >= object->pathPoints.size()) {
    result.status = CreativeMovingPlatformPathEditStatus::InvalidPointIndex;
    result.reasonCode = "creative_platform_path_edit_invalid_point_index";
    return result;
  }
  if (!targetAvailable) {
    return result;
  }
  if (constraint == cr::CreativeMoveConstraint::Count) {
    result.reasonCode = "creative_platform_path_edit_invalid_constraint";
    return result;
  }
  const std::optional<cr::CreativeVec3> targetPoint =
      movingPlatformPathPointAtPlacementAnchor(*object, placementAnchor);
  if (!targetPoint.has_value()) {
    result.reasonCode = "creative_platform_path_edit_invalid_target";
    return result;
  }

  result.visible = true;
  result.fromPoint = object->pathPoints[pointIndex].position;
  result.targetPoint = *targetPoint;
  applyMoveConstraint(result.targetPoint, result.fromPoint, constraint);
  result.segmentVisible =
      !samePathPoint(result.fromPoint, result.targetPoint);
  const bool duplicatesPrevious =
      pointIndex > 0U &&
      samePathPoint(object->pathPoints[pointIndex - 1U].position,
                    result.targetPoint);
  const bool duplicatesNext =
      pointIndex + 1U < object->pathPoints.size() &&
      samePathPoint(object->pathPoints[pointIndex + 1U].position,
                    result.targetPoint);
  if (!result.segmentVisible || duplicatesPrevious || duplicatesNext) {
    result.status = CreativeMovingPlatformPathEditStatus::DuplicateTarget;
    result.reasonCode = "creative_platform_path_edit_duplicate_target";
    return result;
  }
  if (!targetKeepsPathValid(object->pathPoints, pointIndex,
                            result.targetPoint)) {
    result.status = CreativeMovingPlatformPathEditStatus::InvalidPath;
    result.reasonCode = "creative_platform_path_edit_result_invalid";
    return result;
  }
  result.moveAllowed = true;
  result.status = CreativeMovingPlatformPathEditStatus::Ready;
  result.reasonCode = "creative_platform_path_point_move_planned";
  return result;
}

CreativeMovingPlatformPathEditPlan planCreativeMovingPlatformPathEdit(
    std::span<const cr::CreativePathPoint> currentPath,
    CreativeMovingPlatformPathEditCommand command,
    cr::CreativeVec3 targetPoint,
    std::size_t pointIndex) {
  CreativeMovingPlatformPathEditPlan result;
  if (!validPathEditCommand(command)) {
    return result;
  }
  if (!cr::isValidCreativeMovingPlatformPath(currentPath)) {
    result.status = CreativeMovingPlatformPathEditStatus::InvalidPath;
    result.reasonCode = "creative_platform_path_edit_invalid_path";
    return result;
  }

  const bool movingSelected =
      command ==
      CreativeMovingPlatformPathEditCommand::MoveSelectedToTarget;
  const bool removingSelected =
      command == CreativeMovingPlatformPathEditCommand::RemoveSelected;
  if ((movingSelected || removingSelected) &&
      pointIndex >= currentPath.size()) {
    result.status = CreativeMovingPlatformPathEditStatus::InvalidPointIndex;
    result.reasonCode = "creative_platform_path_edit_invalid_point_index";
    return result;
  }

  if (command == CreativeMovingPlatformPathEditCommand::AppendAtTarget ||
      movingSelected) {
    if (!finitePoint(targetPoint)) {
      result.status = CreativeMovingPlatformPathEditStatus::InvalidTarget;
      result.reasonCode = "creative_platform_path_edit_invalid_target";
      return result;
    }
    if (command == CreativeMovingPlatformPathEditCommand::AppendAtTarget &&
        currentPath.size() >= cr::kCreativeMovingPlatformPathPointCapacity) {
      result.status = CreativeMovingPlatformPathEditStatus::CapacityReached;
      result.reasonCode = "creative_platform_path_edit_capacity_reached";
      return result;
    }
    const cr::CreativeVec3 sourcePoint =
        movingSelected ? currentPath[pointIndex].position
                       : currentPath.back().position;
    if (samePathPoint(sourcePoint, targetPoint)) {
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
  switch (command) {
    case CreativeMovingPlatformPathEditCommand::AppendAtTarget:
      result.pathPoints.push_back({targetPoint});
      break;
    case CreativeMovingPlatformPathEditCommand::RemoveLast:
      result.pathPoints.pop_back();
      break;
    case CreativeMovingPlatformPathEditCommand::RemoveSelected:
      result.pathPoints.erase(result.pathPoints.begin() +
                              static_cast<std::ptrdiff_t>(pointIndex));
      break;
    case CreativeMovingPlatformPathEditCommand::MoveSelectedToTarget:
      result.pathPoints[pointIndex].position = targetPoint;
      break;
    case CreativeMovingPlatformPathEditCommand::None:
    case CreativeMovingPlatformPathEditCommand::Count:
      break;
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
  if (selectionChanged || !state.available) {
    state.pending = CreativeMovingPlatformPathEditCommand::None;
    state.selectedPointIndex = 0U;
    state.pointSelected = false;
    state.status = state.available
                       ? CreativeMovingPlatformPathEditStatus::Ready
                       : CreativeMovingPlatformPathEditStatus::Idle;
    state.reasonCode = state.available
                           ? "creative_platform_path_edit_ready"
                           : "creative_platform_path_edit_idle";
  } else if (state.pointSelected &&
             state.selectedPointIndex >= state.pointCount) {
    if (state.pointCount == 0U) {
      state.selectedPointIndex = 0U;
      state.pointSelected = false;
    } else {
      state.selectedPointIndex =
          static_cast<std::uint8_t>(state.pointCount - 1U);
    }
  }
}

bool selectCreativeMovingPlatformPathPoint(
    CreativeMovingPlatformPathEditState& state,
    std::size_t pointIndex) noexcept {
  if (!state.available || pointIndex >= state.pointCount) {
    return false;
  }
  state.selectedPointIndex = static_cast<std::uint8_t>(pointIndex);
  state.pointSelected = true;
  state.status = CreativeMovingPlatformPathEditStatus::Ready;
  state.reasonCode = "creative_platform_path_point_selected";
  return true;
}

bool cycleCreativeMovingPlatformPathPoint(
    CreativeMovingPlatformPathEditState& state,
    int direction) noexcept {
  if (!state.available || state.pointCount == 0U || direction == 0) {
    return false;
  }
  const int pointCount = static_cast<int>(state.pointCount);
  int selected = state.pointSelected
                     ? static_cast<int>(state.selectedPointIndex)
                     : (direction > 0 ? -1 : 0);
  selected = (selected + (direction > 0 ? 1 : -1) + pointCount) % pointCount;
  return selectCreativeMovingPlatformPathPoint(
      state, static_cast<std::size_t>(selected));
}

bool clearCreativeMovingPlatformPathPointSelection(
    CreativeMovingPlatformPathEditState& state) noexcept {
  if (!state.pointSelected) {
    return false;
  }
  state.pointSelected = false;
  state.selectedPointIndex = 0U;
  state.pending = CreativeMovingPlatformPathEditCommand::None;
  state.status = state.available
                     ? CreativeMovingPlatformPathEditStatus::Ready
                     : CreativeMovingPlatformPathEditStatus::Idle;
  state.reasonCode = state.available ? "creative_platform_path_edit_ready"
                                     : "creative_platform_path_edit_idle";
  return true;
}

bool queueCreativeMovingPlatformPathEdit(
    const cr::CreativeAppState& appState,
    CreativeMovingPlatformPathEditState& state,
    CreativeMovingPlatformPathEditCommand command) noexcept {
  syncCreativeMovingPlatformPathEditState(appState, state);
  if (!state.available || !validPathEditCommand(command) ||
      (commandUsesSelectedPoint(command) && !state.pointSelected)) {
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
    std::string_view source,
    cr::CreativeMoveConstraint constraint) {
  const CreativeMovingPlatformPathEditCommand command = state.pending;
  state.pending = CreativeMovingPlatformPathEditCommand::None;
  CreativeMovingPlatformPathEditReceipt result;
  if (!validPathEditCommand(command)) {
    return result;
  }
  syncCreativeMovingPlatformPathEditState(appState, state);
  if (command == CreativeMovingPlatformPathEditCommand::AppendAtTarget) {
    const cr::CreativeObject* object = appState.facade.findObject(state.objectId);
    const CreativeMovingPlatformPathTargetPlan target =
        planCreativeMovingPlatformPathTarget(object, targetAvailable,
                                             targetAnchor);
    if (!target.appendAllowed) {
      result.requested = true;
      result.objectId = target.objectId;
      result.status = target.status;
      result.reasonCode = target.reasonCode;
      result.pointCountBefore = object != nullptr ? object->pathPoints.size() : 0U;
      result.pointCountAfter = result.pointCountBefore;
    } else {
      result = applyPathEditWithUndo(appState, state.objectId, command,
                                     target.targetPoint,
                                     kInvalidCreativeMovingPlatformPathPointIndex,
                                     source);
    }
  } else if (command ==
             CreativeMovingPlatformPathEditCommand::MoveSelectedToTarget) {
    const cr::CreativeObject* object = appState.facade.findObject(state.objectId);
    const CreativeMovingPlatformPathPointTargetPlan target =
        planCreativeMovingPlatformPathPointTarget(
            object, state.selectedPointIndex, targetAvailable, targetAnchor,
            constraint);
    if (!target.moveAllowed) {
      result.requested = true;
      result.objectId = target.objectId;
      result.status = target.status;
      result.reasonCode = target.reasonCode;
      result.pointCountBefore =
          object != nullptr ? object->pathPoints.size() : 0U;
      result.pointCountAfter = result.pointCountBefore;
    } else {
      result = applyPathEditWithUndo(
          appState, state.objectId, command, target.targetPoint,
          state.selectedPointIndex, source);
    }
  } else {
    const std::size_t pointIndex =
        command == CreativeMovingPlatformPathEditCommand::RemoveSelected
            ? state.selectedPointIndex
            : kInvalidCreativeMovingPlatformPathPointIndex;
    result = applyPathEditWithUndo(appState, state.objectId, command,
                                   targetAnchor, pointIndex, source);
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
