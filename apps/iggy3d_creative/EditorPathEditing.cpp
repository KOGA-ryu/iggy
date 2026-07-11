#include "EditorPathEditing.hpp"

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/mutation/Mutation.hpp"

#include "EditorPlacement.hpp"

#include <SDL3/SDL_log.h>

#include <string>
#include <vector>

namespace iggy3d_creative_app {
namespace {

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
  if (descriptor.shapeKind != cr::CreativeObjectShapeKind::Path) {
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
  if (descriptor.shapeKind != cr::CreativeObjectShapeKind::Path) {
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
