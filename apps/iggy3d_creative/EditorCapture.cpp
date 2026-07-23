#include "EditorCapture.hpp"

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/mutation/Mutation.hpp"

#include "EditorState.hpp"
#include "EditorPersistence.hpp"
#include "EditorPlacement.hpp"
#include "EditorPreviewProxies.hpp"
#include "EditorGizmo.hpp"
#include "EditorEdits.hpp"
#include "EditorPathEditing.hpp"

#include <SDL3/SDL_log.h>

#include <string>
#include <vector>

namespace iggy3d_creative_app {
namespace {

bool selectObjectForCapture(cr::Facade& facade,
                            cr::CreativeObjectId objectId,
                            std::string_view source) {
  if (objectId == cr::kInvalidObjectId) {
    SDL_Log("iggy3d_creative: CAPTURE select skipped source='%s' "
            "objectId=0",
            std::string(source).c_str());
    return false;
  }
  cr::CreativeToolInputPacket packet;
  packet.kind = cr::CreativeToolInputKind::PointerPress;
  packet.pointer.button = cr::CreativeToolPointerButton::Primary;
  packet.pointer.target = cr::TargetRef{static_cast<cr::Id>(objectId)};
  const cr::CreativeFacadeToolDispatchReceipt receipt =
      facade.dispatchToolInput(packet);
  SDL_Log("iggy3d_creative: CAPTURE selected source='%s' objectId=%llu "
          "accepted=%d changed=%d selectedTarget=%u",
          std::string(source).c_str(), static_cast<unsigned long long>(objectId),
          receipt.accepted ? 1 : 0, receipt.changed ? 1 : 0,
          facade.selectionState().selectedTarget.value);
  return receipt.accepted;
}

void runScriptedPlacements(const StandaloneCaptureScenarioStepRequest& request) {
  cr::CreativeAppState& appState = *request.appState;
  StandaloneCaptureScript& captureScript = *request.captureScript;
  StandaloneEditHistory& history = *request.history;
  cr::CreativeObjectKind& placeBrush = *request.placeBrush;
  std::uint64_t& placedCount = *request.placedCount;

  for (const StandaloneCapturePlacement& p : captureScript.placements) {
    if (request.frameIndex != p.frame) {
      continue;
    }

    const bool createUndoProofTarget =
        capturePlacementHasProofRole(p, StandaloneCaptureProofRole::CreateUndo);
    const bool deleteProofTarget =
        capturePlacementHasProofRole(p, StandaloneCaptureProofRole::Delete);
    const bool moveProofTarget =
        capturePlacementHasProofRole(p, StandaloneCaptureProofRole::Move);
    const bool pointProofTarget =
        capturePlacementHasProofRole(p, StandaloneCaptureProofRole::Point);
    const bool lineProofTarget =
        capturePlacementHasProofRole(p, StandaloneCaptureProofRole::Line);
    const bool pathProofTarget =
        capturePlacementHasProofRole(p, StandaloneCaptureProofRole::Path);
    placeBrush = p.kind;
    const iggy3d::Vec3 cell =
        snapGroundToCellCenter(p.worldX, p.worldZ, request.placeCellSize);
    const cr::CreativeDocumentCreateReceipt receipt = placeBrushObjectWithUndo(
        appState.facade,
        history,
        placeBrush,
        cell,
        ++placedCount,
        createUndoProofTarget ? "capture_place_create_target"
                              : "capture_place");
    if (createUndoProofTarget && receipt.accepted) {
      captureScript.createUndoTargetId = receipt.objectId;
      SDL_Log("iggy3d_creative: CREATE_UNDO capture target objectId=%llu "
              "kind='%s'",
              static_cast<unsigned long long>(
                  captureScript.createUndoTargetId),
              std::string(cr::toString(receipt.objectKind)).c_str());
    }
    if (deleteProofTarget && receipt.accepted) {
      captureScript.deleteTargetId = receipt.objectId;
      SDL_Log("iggy3d_creative: DELETE capture target objectId=%llu "
              "kind='%s'",
              static_cast<unsigned long long>(captureScript.deleteTargetId),
              std::string(cr::toString(receipt.objectKind)).c_str());
    }
    if (moveProofTarget && receipt.accepted) {
      captureScript.moveTargetId = receipt.objectId;
      SDL_Log("iggy3d_creative: MOVE_UNDO capture target objectId=%llu "
              "kind='%s'",
              static_cast<unsigned long long>(captureScript.moveTargetId),
              std::string(cr::toString(receipt.objectKind)).c_str());
    }
    if (pointProofTarget && receipt.accepted) {
      captureScript.pointTargetId = receipt.objectId;
      SDL_Log("iggy3d_creative: POINT capture target objectId=%llu "
              "kind='%s' shape='%s' boundsOverride=%d",
              static_cast<unsigned long long>(captureScript.pointTargetId),
              std::string(cr::toString(receipt.objectKind)).c_str(),
              std::string(cr::toString(
                              cr::describeObject(receipt.objectKind).shapeKind))
                  .c_str(),
              0);
    }
    if (lineProofTarget && receipt.accepted) {
      captureScript.lineTargetId = receipt.objectId;
      const cr::CreativeObject* lineTarget =
          appState.facade.findObject(captureScript.lineTargetId);
      const VisualBounds lineVisual =
          lineTarget != nullptr ? visualBoundsForObject(*lineTarget)
                                : VisualBounds{};
      SDL_Log("iggy3d_creative: LINE capture target objectId=%llu "
              "kind='%s' shape='%s' projection='%s' occupancy='%s' "
              "boundsOverride=%d visual=[(%.3f, %.3f, %.3f).."
              "(%.3f, %.3f, %.3f)]",
              static_cast<unsigned long long>(captureScript.lineTargetId),
              std::string(cr::toString(receipt.objectKind)).c_str(),
              std::string(cr::toString(
                              cr::describeObject(receipt.objectKind).shapeKind))
                  .c_str(),
              std::string(cr::toString(cr::describeObject(receipt.objectKind)
                                           .projectionProfile))
                  .c_str(),
              std::string(cr::toString(cr::describeObject(receipt.objectKind)
                                           .occupancyKind))
                  .c_str(),
              lineTarget != nullptr ? 1 : 0, lineVisual.min.x,
              lineVisual.min.y, lineVisual.min.z, lineVisual.max.x,
              lineVisual.max.y, lineVisual.max.z);
    }
    if (pathProofTarget && receipt.accepted) {
      captureScript.pathTargetId = receipt.objectId;
      const cr::CreativeObject* pathTarget =
          appState.facade.findObject(captureScript.pathTargetId);
      const VisualBounds pathVisual =
          pathTarget != nullptr ? visualBoundsForObject(*pathTarget)
                                : VisualBounds{};
      SDL_Log("iggy3d_creative: PATH capture target objectId=%llu "
              "kind='%s' shape='%s' projection='%s' occupancy='%s' "
              "pathOverride=1 pathPointCount=%zu pathPoints='%s' "
              "visual=[(%.3f, %.3f, %.3f)..(%.3f, %.3f, %.3f)]",
              static_cast<unsigned long long>(captureScript.pathTargetId),
              std::string(cr::toString(receipt.objectKind)).c_str(),
              std::string(cr::toString(
                              cr::describeObject(receipt.objectKind).shapeKind))
                  .c_str(),
              std::string(cr::toString(cr::describeObject(receipt.objectKind)
                                           .projectionProfile))
                  .c_str(),
              std::string(cr::toString(cr::describeObject(receipt.objectKind)
                                           .occupancyKind))
                  .c_str(),
              pathTarget != nullptr ? pathTarget->pathPoints.size() : 0U,
              pathTarget != nullptr
                  ? pathPointsSummary(pathTarget->pathPoints).c_str()
                  : "",
              pathVisual.min.x, pathVisual.min.y, pathVisual.min.z,
              pathVisual.max.x, pathVisual.max.y, pathVisual.max.z);
    }
  }
}

void runDeleteAndUndoProof(
    const StandaloneCaptureScenarioStepRequest& request) {
  cr::CreativeAppState& appState = *request.appState;
  StandaloneCaptureScript& captureScript = *request.captureScript;

  if (request.frameIndex == StandaloneCaptureScript::kDeleteNoSelectionFrame &&
      !captureScript.deleteNoSelectionAttempted) {
    (void)request.deleteSelected("capture_no_selection");
    captureScript.deleteNoSelectionAttempted = true;
  } else if (request.frameIndex == StandaloneCaptureScript::kCreateUndoFrame &&
             !captureScript.createUndoAttempted) {
    SDL_Log("iggy3d_creative: CREATE_UNDO attempting target objectId=%llu "
            "objectCountBefore=%llu",
            static_cast<unsigned long long>(captureScript.createUndoTargetId),
            static_cast<unsigned long long>(
                appState.facade.document().objectCount()));
    (void)undoLastEdit(appState, "capture_create_undo");
    SDL_Log("iggy3d_creative: CREATE_UNDO objectCountAfter=%llu "
            "targetPresentAfter=%d",
            static_cast<unsigned long long>(
                appState.facade.document().objectCount()),
            appState.facade.findObject(captureScript.createUndoTargetId) !=
                    nullptr
                ? 1
                : 0);
    captureScript.createUndoAttempted = true;
  } else if (request.frameIndex == StandaloneCaptureScript::kDeleteFrame &&
             !captureScript.deleteAttempted) {
    (void)selectObjectForCapture(appState.facade, captureScript.deleteTargetId,
                                 "capture_delete");
    (void)request.deleteSelected("capture_delete");
    captureScript.deleteAttempted = true;
  } else if (request.frameIndex == StandaloneCaptureScript::kUndoFrame &&
             !captureScript.undoAttempted) {
    (void)undoLastEdit(appState, "capture_undo");
    captureScript.undoAttempted = true;
  }
}

void runMoveUndoProof(const StandaloneCaptureScenarioStepRequest& request) {
  cr::CreativeAppState& appState = *request.appState;
  StandaloneCaptureScript& captureScript = *request.captureScript;
  StandaloneEditHistory& history = *request.history;
  bool& placeMode = *request.placeMode;
  cr::CreativeObjectKind& placeBrush = *request.placeBrush;

  if (request.frameIndex == StandaloneCaptureScript::kMoveBeginFrame &&
      !captureScript.moveBeginAttempted) {
    placeMode = false;
    (void)appState.facade.setActiveTool(cr::Tool::Move);
    (void)selectObjectForCapture(appState.facade, captureScript.moveTargetId,
                                 "capture_move");
    const cr::CreativeObject* moveTarget =
        appState.facade.findObject(captureScript.moveTargetId);
    if (moveTarget != nullptr) {
      captureScript.moveDestination = {moveTarget->transform.position.x + 2.0,
                                       moveTarget->transform.position.y,
                                       moveTarget->transform.position.z};
      logUndoMovePlacement("capture_begin",
                           captureScript.moveTargetId,
                           moveTarget);
      cr::CreativeToolInputPacket press;
      press.kind = cr::CreativeToolInputKind::PointerPress;
      press.pointer.button = cr::CreativeToolPointerButton::Primary;
      press.pointer.target =
          cr::TargetRef{static_cast<cr::Id>(captureScript.moveTargetId)};
      const cr::CreativeFacadeToolDispatchReceipt receipt =
          appState.facade.dispatchToolInput(press);
      logMoveDispatch("CAPTURE_MOVE_PRESS", receipt);
    }
    captureScript.moveBeginAttempted = true;
  } else if (request.frameIndex == StandaloneCaptureScript::kMovePreviewFrame &&
             !captureScript.movePreviewAttempted) {
    cr::CreativeToolInputPacket move;
    move.kind = cr::CreativeToolInputKind::PointerMove;
    move.pointer.button = cr::CreativeToolPointerButton::Primary;
    move.pointer.hasWorldDestination = true;
    move.pointer.worldDestination = captureScript.moveDestination;
    move.pointer.moveHeldAxis = request.moveHeldAxisForX;
    const cr::CreativeFacadeToolDispatchReceipt receipt =
        appState.facade.dispatchToolInput(move);
    logMoveDispatch("CAPTURE_MOVE", receipt);
    captureScript.movePreviewAttempted = true;
  } else if (request.frameIndex == StandaloneCaptureScript::kMoveCommitFrame &&
             !captureScript.moveCommitAttempted) {
    cr::CreativeToolInputPacket release;
    release.kind = cr::CreativeToolInputKind::PointerRelease;
    release.pointer.button = cr::CreativeToolPointerButton::Primary;
    release.pointer.hasWorldDestination = true;
    release.pointer.worldDestination = captureScript.moveDestination;
    release.pointer.moveHeldAxis = request.moveHeldAxisForX;
    const cr::CreativeFacadeToolDispatchReceipt receipt =
        dispatchMoveReleaseWithUndo(appState,
                                    history,
                                    release,
                                    captureScript.moveTargetId,
                                    "capture_move_commit");
    logMoveDispatch("CAPTURE_MOVE_RELEASE", receipt);
    captureScript.moveCommitAttempted = true;
  } else if (request.frameIndex == StandaloneCaptureScript::kMoveUndoFrame &&
             !captureScript.moveUndoAttempted) {
    (void)undoLastEdit(appState, "capture_move_undo");
    placeMode = true;
    placeBrush = cr::CreativeObjectKind::Wall;
    logUndoMovePlacement("capture_undo_after",
                         captureScript.moveTargetId,
                         appState.facade.findObject(
                             captureScript.moveTargetId));
    captureScript.moveUndoAttempted = true;
  }
}

void runPointProof(const StandaloneCaptureScenarioStepRequest& request) {
  cr::CreativeAppState& appState = *request.appState;
  StandaloneCaptureScript& captureScript = *request.captureScript;
  StandaloneEditHistory& history = *request.history;
  bool& placeMode = *request.placeMode;
  cr::CreativeObjectKind& placeBrush = *request.placeBrush;

  if (request.frameIndex == StandaloneCaptureScript::kPointMoveBeginFrame &&
      !captureScript.pointMoveBeginAttempted) {
    placeMode = false;
    (void)appState.facade.setActiveTool(cr::Tool::Move);
    (void)selectObjectForCapture(appState.facade, captureScript.pointTargetId,
                                 "capture_point_move");
    const cr::CreativeObject* pointTarget =
        appState.facade.findObject(captureScript.pointTargetId);
    if (pointTarget != nullptr) {
      const VisualBounds markerBounds = visualBoundsForObject(*pointTarget);
      captureScript.pointMoveDestination = {
          pointTarget->transform.position.x + 1.0,
          pointTarget->transform.position.y,
          pointTarget->transform.position.z};
      SDL_Log("iggy3d_creative: POINT before move objectId=%llu "
              "pos=(%.3f, %.3f, %.3f) marker=[(%.3f, %.3f, %.3f).."
              "(%.3f, %.3f, %.3f)]",
              static_cast<unsigned long long>(captureScript.pointTargetId),
              pointTarget->transform.position.x,
              pointTarget->transform.position.y,
              pointTarget->transform.position.z, markerBounds.min.x,
              markerBounds.min.y, markerBounds.min.z, markerBounds.max.x,
              markerBounds.max.y, markerBounds.max.z);
      cr::CreativeToolInputPacket press;
      press.kind = cr::CreativeToolInputKind::PointerPress;
      press.pointer.button = cr::CreativeToolPointerButton::Primary;
      press.pointer.target =
          cr::TargetRef{static_cast<cr::Id>(captureScript.pointTargetId)};
      const cr::CreativeFacadeToolDispatchReceipt receipt =
          appState.facade.dispatchToolInput(press);
      logMoveDispatch("POINT_MOVE_PRESS", receipt);
    }
    captureScript.pointMoveBeginAttempted = true;
  } else if (request.frameIndex ==
                 StandaloneCaptureScript::kPointMoveCommitFrame &&
             !captureScript.pointMoveCommitAttempted) {
    cr::CreativeToolInputPacket release;
    release.kind = cr::CreativeToolInputKind::PointerRelease;
    release.pointer.button = cr::CreativeToolPointerButton::Primary;
    release.pointer.hasWorldDestination = true;
    release.pointer.worldDestination = captureScript.pointMoveDestination;
    release.pointer.moveHeldAxis = request.moveHeldAxisForX;
    const cr::CreativeFacadeToolDispatchReceipt receipt =
        dispatchMoveReleaseWithUndo(appState,
                                    history,
                                    release,
                                    captureScript.pointTargetId,
                                    "capture_point_move_commit");
    logMoveDispatch("POINT_MOVE_RELEASE", receipt);
    const cr::CreativeObject* pointTarget =
        appState.facade.findObject(captureScript.pointTargetId);
    if (pointTarget != nullptr) {
      const VisualBounds markerBounds = visualBoundsForObject(*pointTarget);
      SDL_Log("iggy3d_creative: POINT after move objectId=%llu "
              "pos=(%.3f, %.3f, %.3f) marker=[(%.3f, %.3f, %.3f).."
              "(%.3f, %.3f, %.3f)]",
              static_cast<unsigned long long>(captureScript.pointTargetId),
              pointTarget->transform.position.x,
              pointTarget->transform.position.y,
              pointTarget->transform.position.z, markerBounds.min.x,
              markerBounds.min.y, markerBounds.min.z, markerBounds.max.x,
              markerBounds.max.y, markerBounds.max.z);
    }
    captureScript.pointMoveCommitAttempted = true;
  } else if (request.frameIndex ==
                 StandaloneCaptureScript::kPointMoveUndoFrame &&
             !captureScript.pointMoveUndoAttempted) {
    (void)undoLastEdit(appState, "capture_point_move_undo");
    placeMode = true;
    placeBrush = cr::CreativeObjectKind::Wall;
    const cr::CreativeObject* pointTarget =
        appState.facade.findObject(captureScript.pointTargetId);
    if (pointTarget != nullptr) {
      const VisualBounds markerBounds = visualBoundsForObject(*pointTarget);
      SDL_Log("iggy3d_creative: POINT after undo objectId=%llu "
              "pos=(%.3f, %.3f, %.3f) marker=[(%.3f, %.3f, %.3f).."
              "(%.3f, %.3f, %.3f)]",
              static_cast<unsigned long long>(captureScript.pointTargetId),
              pointTarget->transform.position.x,
              pointTarget->transform.position.y,
              pointTarget->transform.position.z, markerBounds.min.x,
              markerBounds.min.y, markerBounds.min.z, markerBounds.max.x,
              markerBounds.max.y, markerBounds.max.z);
    }
    captureScript.pointMoveUndoAttempted = true;
  }
}

void runLineProof(const StandaloneCaptureScenarioStepRequest& request) {
  cr::CreativeAppState& appState = *request.appState;
  StandaloneCaptureScript& captureScript = *request.captureScript;
  StandaloneEditHistory& history = *request.history;
  bool& placeMode = *request.placeMode;
  cr::CreativeObjectKind& placeBrush = *request.placeBrush;

  if (request.frameIndex == StandaloneCaptureScript::kLineMoveBeginFrame &&
      !captureScript.lineMoveBeginAttempted) {
    placeMode = false;
    (void)appState.facade.setActiveTool(cr::Tool::Move);
    (void)selectObjectForCapture(appState.facade, captureScript.lineTargetId,
                                 "capture_line_move");
    const cr::CreativeObject* lineTarget =
        appState.facade.findObject(captureScript.lineTargetId);
    if (lineTarget != nullptr) {
      const VisualBounds authoredBounds{
          cr::creativeVec3ToCoreChecked(lineTarget->bounds.min).value,
          cr::creativeVec3ToCoreChecked(lineTarget->bounds.max).value};
      const VisualBounds lineVisual = visualBoundsForObject(*lineTarget);
      captureScript.lineMoveDestination = {
          lineTarget->transform.position.x,
          lineTarget->transform.position.y,
          lineTarget->transform.position.z + 1.0};
      SDL_Log("iggy3d_creative: LINE before move objectId=%llu "
              "pos=(%.3f, %.3f, %.3f) authored=[(%.3f, %.3f, %.3f).."
              "(%.3f, %.3f, %.3f)] visual=[(%.3f, %.3f, %.3f).."
              "(%.3f, %.3f, %.3f)]",
              static_cast<unsigned long long>(captureScript.lineTargetId),
              lineTarget->transform.position.x,
              lineTarget->transform.position.y,
              lineTarget->transform.position.z, authoredBounds.min.x,
              authoredBounds.min.y, authoredBounds.min.z, authoredBounds.max.x,
              authoredBounds.max.y, authoredBounds.max.z, lineVisual.min.x,
              lineVisual.min.y, lineVisual.min.z, lineVisual.max.x,
              lineVisual.max.y, lineVisual.max.z);
      cr::CreativeToolInputPacket press;
      press.kind = cr::CreativeToolInputKind::PointerPress;
      press.pointer.button = cr::CreativeToolPointerButton::Primary;
      press.pointer.target =
          cr::TargetRef{static_cast<cr::Id>(captureScript.lineTargetId)};
      const cr::CreativeFacadeToolDispatchReceipt receipt =
          appState.facade.dispatchToolInput(press);
      logMoveDispatch("LINE_MOVE_PRESS", receipt);
    }
    captureScript.lineMoveBeginAttempted = true;
  } else if (request.frameIndex ==
                 StandaloneCaptureScript::kLineMoveCommitFrame &&
             !captureScript.lineMoveCommitAttempted) {
    cr::CreativeToolInputPacket release;
    release.kind = cr::CreativeToolInputKind::PointerRelease;
    release.pointer.button = cr::CreativeToolPointerButton::Primary;
    release.pointer.hasWorldDestination = true;
    release.pointer.worldDestination = captureScript.lineMoveDestination;
    release.pointer.moveHeldAxis = request.moveHeldAxisForZ;
    const cr::CreativeFacadeToolDispatchReceipt receipt =
        dispatchMoveReleaseWithUndo(appState,
                                    history,
                                    release,
                                    captureScript.lineTargetId,
                                    "capture_line_move_commit");
    logMoveDispatch("LINE_MOVE_RELEASE", receipt);
    const cr::CreativeObject* lineTarget =
        appState.facade.findObject(captureScript.lineTargetId);
    if (lineTarget != nullptr) {
      const VisualBounds authoredBounds{
          cr::creativeVec3ToCoreChecked(lineTarget->bounds.min).value,
          cr::creativeVec3ToCoreChecked(lineTarget->bounds.max).value};
      const VisualBounds lineVisual = visualBoundsForObject(*lineTarget);
      SDL_Log("iggy3d_creative: LINE after move objectId=%llu "
              "pos=(%.3f, %.3f, %.3f) authored=[(%.3f, %.3f, %.3f).."
              "(%.3f, %.3f, %.3f)] visual=[(%.3f, %.3f, %.3f).."
              "(%.3f, %.3f, %.3f)]",
              static_cast<unsigned long long>(captureScript.lineTargetId),
              lineTarget->transform.position.x,
              lineTarget->transform.position.y,
              lineTarget->transform.position.z, authoredBounds.min.x,
              authoredBounds.min.y, authoredBounds.min.z, authoredBounds.max.x,
              authoredBounds.max.y, authoredBounds.max.z, lineVisual.min.x,
              lineVisual.min.y, lineVisual.min.z, lineVisual.max.x,
              lineVisual.max.y, lineVisual.max.z);
    }
    captureScript.lineMoveCommitAttempted = true;
  } else if (request.frameIndex ==
                 StandaloneCaptureScript::kLineMoveUndoFrame &&
             !captureScript.lineMoveUndoAttempted) {
    (void)undoLastEdit(appState, "capture_line_move_undo");
    placeMode = true;
    placeBrush = cr::CreativeObjectKind::Wall;
    const cr::CreativeObject* lineTarget =
        appState.facade.findObject(captureScript.lineTargetId);
    if (lineTarget != nullptr) {
      const VisualBounds authoredBounds{
          cr::creativeVec3ToCoreChecked(lineTarget->bounds.min).value,
          cr::creativeVec3ToCoreChecked(lineTarget->bounds.max).value};
      const VisualBounds lineVisual = visualBoundsForObject(*lineTarget);
      SDL_Log("iggy3d_creative: LINE after undo objectId=%llu "
              "pos=(%.3f, %.3f, %.3f) authored=[(%.3f, %.3f, %.3f).."
              "(%.3f, %.3f, %.3f)] visual=[(%.3f, %.3f, %.3f).."
              "(%.3f, %.3f, %.3f)]",
              static_cast<unsigned long long>(captureScript.lineTargetId),
              lineTarget->transform.position.x,
              lineTarget->transform.position.y,
              lineTarget->transform.position.z, authoredBounds.min.x,
              authoredBounds.min.y, authoredBounds.min.z, authoredBounds.max.x,
              authoredBounds.max.y, authoredBounds.max.z, lineVisual.min.x,
              lineVisual.min.y, lineVisual.min.z, lineVisual.max.x,
              lineVisual.max.y, lineVisual.max.z);
    }
    captureScript.lineMoveUndoAttempted = true;
  }
}

void runPathProof(const StandaloneCaptureScenarioStepRequest& request) {
  cr::CreativeAppState& appState = *request.appState;
  StandaloneCaptureScript& captureScript = *request.captureScript;
  StandaloneEditHistory& history = *request.history;
  bool& placeMode = *request.placeMode;
  cr::CreativeObjectKind& placeBrush = *request.placeBrush;

  if (request.frameIndex == StandaloneCaptureScript::kPathMoveFrame &&
      !captureScript.pathMoveAttempted) {
    placeMode = false;
    (void)selectObjectForCapture(appState.facade, captureScript.pathTargetId,
                                 "capture_path_move");
    const cr::CreativeDocumentMutationReceipt receipt =
        movePathObjectWithUndo(appState,
                               history,
                               captureScript.pathTargetId,
                               {1.0, 0.0, 1.0},
                               "capture_path_move_commit");
    SDL_Log("iggy3d_creative: PATH mutation receipt status='%s' changed=%d "
            "revisionBefore=%llu revisionAfter=%llu dirtyFlags=%llu",
            std::string(cr::toString(receipt.status)).c_str(),
            receipt.changed ? 1 : 0,
            static_cast<unsigned long long>(receipt.revisionBefore),
            static_cast<unsigned long long>(receipt.revisionAfter),
            static_cast<unsigned long long>(receipt.dirtyFlags));
    captureScript.pathMoveAttempted = true;
  } else if (request.frameIndex ==
                 StandaloneCaptureScript::kPathMoveUndoFrame &&
             !captureScript.pathMoveUndoAttempted) {
    (void)undoLastEdit(appState, "capture_path_move_undo");
    placeMode = true;
    placeBrush = cr::CreativeObjectKind::Wall;
    const cr::CreativeObject* pathTarget =
        appState.facade.findObject(captureScript.pathTargetId);
    if (pathTarget != nullptr) {
      SDL_Log("iggy3d_creative: PATH after undo objectId=%llu "
              "pathPointCount=%zu pathPoints='%s'",
              static_cast<unsigned long long>(captureScript.pathTargetId),
              pathTarget->pathPoints.size(),
              pathPointsSummary(pathTarget->pathPoints).c_str());
    }
    captureScript.pathMoveUndoAttempted = true;
  } else if (request.frameIndex ==
                 StandaloneCaptureScript::kPathPointMoveFrame &&
             !captureScript.pathPointMoveAttempted) {
    placeMode = false;
    (void)selectObjectForCapture(appState.facade, captureScript.pathTargetId,
                                 "capture_path_point_move");
    constexpr std::size_t kCapturePathPointIndex = 2U;
    const cr::CreativeDocumentMutationReceipt receipt =
        movePathPointWithUndo(appState,
                              history,
                              captureScript.pathTargetId,
                              kCapturePathPointIndex,
                              {0.0, 0.0, 1.0},
                              "capture_path_point_move_commit");
    SDL_Log("iggy3d_creative: PATH_HANDLE mutation receipt status='%s' "
            "changed=%d revisionBefore=%llu revisionAfter=%llu "
            "dirtyFlags=%llu pointIndex=%zu",
            std::string(cr::toString(receipt.status)).c_str(),
            receipt.changed ? 1 : 0,
            static_cast<unsigned long long>(receipt.revisionBefore),
            static_cast<unsigned long long>(receipt.revisionAfter),
            static_cast<unsigned long long>(receipt.dirtyFlags),
            kCapturePathPointIndex);
    captureScript.pathPointMoveAttempted = true;
  } else if (request.frameIndex ==
                 StandaloneCaptureScript::kPathPointMoveUndoFrame &&
             !captureScript.pathPointMoveUndoAttempted) {
    (void)undoLastEdit(appState, "capture_path_point_move_undo");
    placeMode = true;
    placeBrush = cr::CreativeObjectKind::Wall;
    const cr::CreativeObject* pathTarget =
        appState.facade.findObject(captureScript.pathTargetId);
    if (pathTarget != nullptr) {
      SDL_Log("iggy3d_creative: PATH_HANDLE after undo objectId=%llu "
              "pathPointCount=%zu pathPoints='%s'",
              static_cast<unsigned long long>(captureScript.pathTargetId),
              pathTarget->pathPoints.size(),
              pathPointsSummary(pathTarget->pathPoints).c_str());
    }
    captureScript.pathPointMoveUndoAttempted = true;
  }
}

void runPersistenceProof(const StandaloneCaptureScenarioStepRequest& request) {
  cr::CreativeAppState& appState = *request.appState;
  StandaloneCaptureScript& captureScript = *request.captureScript;
  StandaloneEditHistory& history = *request.history;

  if (request.frameIndex == StandaloneCaptureScript::kSaveFrame &&
      !captureScript.roundtripSaved) {
    captureScript.roundtripBefore = snapshotDocument(appState.facade.document());
    logDocumentSnapshot("BEFORE", captureScript.roundtripBefore);
    static_cast<void>(
        saveStandaloneScene(appState.facade, *request.saveRoot, *request.saveId));
    captureScript.roundtripSaved = true;
  } else if (request.frameIndex == StandaloneCaptureScript::kClearFrame &&
             !captureScript.roundtripCleared) {
    const cr::CreativeFacadeDocumentInstallReceipt cleared =
        clearToBlankScene(appState);
    if (cleared.accepted) {
      clearEditHistory(history, "capture_clear");
    }
    captureScript.roundtripCountAfterClear =
        appState.facade.document().objectCount();
    captureScript.roundtripCleared = true;
  } else if (request.frameIndex == StandaloneCaptureScript::kLoadFrame &&
             !captureScript.roundtripLoaded) {
    const bool loaded =
        loadStandaloneScene(appState, *request.saveRoot, *request.saveId);
    if (loaded) {
      clearEditHistory(history, "capture_load_success");
    }
    const std::vector<ObjectSnapshotEntry> roundtripAfter =
        snapshotDocument(appState.facade.document());
    logDocumentSnapshot("AFTER", roundtripAfter);
    const bool match =
        snapshotsMatch(captureScript.roundtripBefore, roundtripAfter);
    SDL_Log("iggy3d_creative: ROUNDTRIP objectCount before=%zu afterClear=%zu "
            "afterLoad=%zu match=%d",
            captureScript.roundtripBefore.size(),
            captureScript.roundtripCountAfterClear,
            roundtripAfter.size(),
            match ? 1 : 0);
    captureScript.roundtripLoaded = true;
  }
}

bool requestIsReady(const StandaloneCaptureScenarioStepRequest& request) {
  return request.enabled && request.appState != nullptr &&
         request.history != nullptr && request.captureScript != nullptr &&
         request.placeBrush != nullptr && request.placeMode != nullptr &&
         request.placedCount != nullptr && request.saveRoot != nullptr &&
         request.saveId != nullptr && request.deleteSelected;
}

}  // namespace

void runStandaloneCaptureScenarioStep(
    const StandaloneCaptureScenarioStepRequest& request) {
  if (!requestIsReady(request)) {
    return;
  }

  if (*request.placeMode) {
    runScriptedPlacements(request);
  }
  runDeleteAndUndoProof(request);
  runMoveUndoProof(request);
  runPointProof(request);
  runLineProof(request);
  runPathProof(request);
  runPersistenceProof(request);
}

void runCreativeEditorCaptureScenarioFrame(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    const std::filesystem::path& saveRoot,
    const std::string& saveId,
    bool captureMode) {
  if (!captureMode) {
    return;
  }

  StandaloneCaptureScenarioStepRequest captureStep;
  captureStep.enabled = true;
  captureStep.frameIndex = editor.frameIndex;
  captureStep.appState = &appState;
  captureStep.history = &appState.history;
  captureStep.captureScript = &editor.captureScript;
  captureStep.placeBrush = &editor.placeBrush;
  captureStep.placeMode = &editor.placeMode;
  captureStep.placedCount = &editor.placedCount;
  captureStep.placeCellSize = editor.placeCellSize;
  captureStep.saveRoot = &saveRoot;
  captureStep.saveId = &saveId;
  captureStep.moveHeldAxisForX = heldAxisForGrabbedAxis(GizmoAxis::X);
  captureStep.moveHeldAxisForZ = heldAxisForGrabbedAxis(GizmoAxis::Z);
  captureStep.deleteSelected = [&](std::string_view source) {
    return deleteSelectedObjectsWithUndo(appState, source, &appState.history);
  };
  runStandaloneCaptureScenarioStep(captureStep);
}

}  // namespace iggy3d_creative_app
