#include "StandaloneGizmo.hpp"

#include <SDL3/SDL_log.h>

#include <string>

namespace iggy3d_creative_app {
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

const char* gizmoAxisName(GizmoAxis axis) {
  switch (axis) {
    case GizmoAxis::X:
      return "X";
    case GizmoAxis::Y:
      return "Y";
    case GizmoAxis::Z:
      return "Z";
    case GizmoAxis::None:
    default:
      return "None";
  }
}

void logObjectPlacement(const char* phase, const cr::CreativeObject* object) {
  if (object == nullptr) {
    SDL_Log("iggy3d_creative: MOVE %s object=<null>", phase);
    return;
  }
  SDL_Log("iggy3d_creative: MOVE %s pos=(%.3f, %.3f, %.3f) "
          "boundsMin=(%.3f, %.3f, %.3f) boundsMax=(%.3f, %.3f, %.3f)",
          phase, object->transform.position.x, object->transform.position.y,
          object->transform.position.z, object->bounds.min.x,
          object->bounds.min.y, object->bounds.min.z, object->bounds.max.x,
          object->bounds.max.y, object->bounds.max.z);
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
    StandaloneUndoStack& undoStack,
    const cr::CreativeToolInputPacket& release,
    cr::CreativeObjectId objectId,
    std::string_view source) {
  const std::size_t undoDepthBefore = undoStack.documents.size();
  logUndoMovePlacement("before", objectId, appState.facade.findObject(objectId));
  pushUndoSnapshot(undoStack, appState.facade, source);
  const cr::CreativeFacadeToolDispatchReceipt receipt =
      appState.facade.dispatchToolInput(release);
  const bool applied = isAppliedMoveCommit(receipt);
  if (!applied) {
    discardUndoSnapshot(undoStack, undoDepthBefore, source,
                        receipt.moveDrag.message);
  }
  logUndoMovePlacement("after", objectId, appState.facade.findObject(objectId));
  SDL_Log("iggy3d_creative: UNDO move commit source='%s' objectId=%llu "
          "applied=%d accepted=%d changed=%d moveDragChanged=%d "
          "depthBefore=%zu depthAfter=%zu stage=%d outcome=%d "
          "documentStatus=%d reasonCode='%s'",
          std::string(source).c_str(),
          static_cast<unsigned long long>(objectId), applied ? 1 : 0,
          receipt.accepted ? 1 : 0, receipt.changed ? 1 : 0,
          receipt.moveDrag.changed ? 1 : 0, undoDepthBefore,
          undoStack.documents.size(),
          static_cast<int>(receipt.moveDrag.stage),
          static_cast<int>(receipt.moveDrag.outcome),
          static_cast<int>(receipt.moveDrag.documentStatus),
          receipt.moveDrag.message.c_str());
  return receipt;
}

GizmoAxis pickGizmoAxisFromProjectedShafts(
    const std::array<GizmoAxisShaft, 3>& shafts,
    ScreenPoint centerScreen,
    const std::array<ScreenPoint, 3>& tipScreens,
    float px,
    float py,
    float thresholdPx) {
  if (!centerScreen.valid) {
    return GizmoAxis::None;
  }

  GizmoAxis best = GizmoAxis::None;
  float bestDist = thresholdPx;
  for (std::size_t i = 0; i < shafts.size(); ++i) {
    if (!tipScreens[i].valid) {
      continue;
    }
    const float d = pointToSegmentDistancePx(px,
                                             py,
                                             centerScreen.x,
                                             centerScreen.y,
                                             tipScreens[i].x,
                                             tipScreens[i].y);
    if (d < bestDist) {
      bestDist = d;
      best = shafts[i].axis;
    }
  }
  return best;
}

}  // namespace iggy3d_creative_app
