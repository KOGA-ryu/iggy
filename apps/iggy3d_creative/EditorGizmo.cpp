#include "EditorGizmo.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_log.h>

#include <cmath>
#include <cstddef>
#include <string>

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/platform/SdlWindow.hpp"
#include "EditorPlacement.hpp"
#include "EditorFrame.hpp"
#include "EditorPathEditing.hpp"
#include "EditorPreviewProxies.hpp"
#include "EditorState.hpp"

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

void processCreativeEditorMoveFrame(
    const CreativeEditorMoveFrameRequest& request) {
  iggy3d::SdlWindow& window = request.window;
  cr::CreativeAppState& appState = request.appState;
  CreativeEditorState& editor = request.editor;
  const iggy3d::RenderCameraFrame& camera = request.camera;
  const std::uint32_t drawableWidth = request.drawableWidth;
  const std::uint32_t drawableHeight = request.drawableHeight;
  const float axisLengthMeters = request.axisLengthMeters;
  const float handleThresholdPx = request.handleThresholdPx;
  const bool captureMode = request.captureMode;

  const creative::Id selectedId = request.selection.selectedId;
  const creative::CreativeObject* selected = request.selection.selected;
  const bool hasSelection = request.selection.hasSelection;
  const std::array<GizmoAxisShaft, 3>& gizmoShafts =
      request.gizmoFrame.shafts;
  const ScreenPoint gizmoCenterScreen = request.gizmoFrame.centerScreen;
  const std::array<ScreenPoint, 3>& gizmoTipScreen =
      request.gizmoFrame.tipScreens;
  const std::vector<PathPointHandleHit>& pathPointHandleHits =
      request.gizmoFrame.pathPointHandleHits;

  // Start anchor S for an axis-constrained move = the object's corner anchor,
  // exactly as the facade captures it on BeginMove (objectCornerAnchor): for a
  // Crate (hasTransform=true) that is transform.position, NOT bounds.min. Using
  // the SAME anchor the facade holds keeps the pinned axes grid-aligned so they
  // snap to themselves; reading bounds.min instead would desync the pinned axes
  // and let the snap drag a "held" axis off S. Constrained-move worldDestination
  // is built FROM S: grabbed axis carries the dragged value, other two pinned.
  const iggy3d::Vec3 gizmoAnchorS = request.gizmoFrame.anchorS;

  // ---- MOVE --------------------------------------------------------------
  // Everything below drives the kernel's GENERIC Move: setActiveTool(Move) +
  // the PRESS/MOVE/RELEASE pointer lifecycle through dispatchToolInput. The
  // facade picks the object, snaps the world destination to the grid, and
  // commits ONE Move mutation. NO per-object position math lives here, and the
  // target is ALWAYS the currently selected id — for the capture that is
  // the FLOOR, which rides the identical path the crate did in the earlier proof.
  const creative::CreativeObjectId selectedObjectId =
      static_cast<creative::CreativeObjectId>(selectedId);
  if (captureMode && !editor.placeMode) {
    // --capture: after the FLOOR is selected (frame 3), grab the X
    // gizmo handle and run an AXIS-CONSTRAINED Move along +X by 2 m.
    //   frame 5: hit-test the X shaft (grab X) + switch to Move + PRESS
    //   frame 6: PointerMove carrying worldDestination = {S.x+2, S.y, S.z},
    //            moveHeldAxis=Y (PreviewMove) — X follows, Y held, Z pinned
    //   frame 7: RELEASE with the same worldDestination (CommitMove) -> snap
    // The single-axis motion is entirely a product of the destination + held
    // axis; there is NO per-object move math. worldDestination pins Y,Z to the
    // start anchor S so only X (= S.x + 2) can change after the facade snaps.
    const creative::CreativeToolWorldPoint xAxisDestination{
        static_cast<double>(gizmoAnchorS.x) + 2.0,
        static_cast<double>(gizmoAnchorS.y),
        static_cast<double>(gizmoAnchorS.z)};
    if (editor.frameIndex == 5U && hasSelection) {
      // Synthesize a grab of the X handle: click the projected midpoint of the
      // X shaft [screen(C), screen(Xtip)] and confirm the hit-test picks X.
      GizmoAxis grabbed = GizmoAxis::None;
      if (gizmoCenterScreen.valid && gizmoTipScreen[0].valid) {
        const float hx = (gizmoCenterScreen.x + gizmoTipScreen[0].x) * 0.5F;
        const float hy = (gizmoCenterScreen.y + gizmoTipScreen[0].y) * 0.5F;
        grabbed = pickGizmoAxisFromProjectedShafts(
            gizmoShafts,
            gizmoCenterScreen,
            gizmoTipScreen,
            hx,
            hy,
            handleThresholdPx);
      }
      if (!editor.loggedGizmoGrab) {
        SDL_Log("iggy3d_creative: GIZMO grabbed axis=%s (expected X) on "
                "selected id=%u kind='%s'",
                gizmoAxisName(grabbed), selectedId,
                std::string(creative::toString(selected->kind)).c_str());
        editor.loggedGizmoGrab = true;
      }
      const bool ok = appState.facade.setActiveTool(creative::Tool::Move);
      SDL_Log("iggy3d_creative: setActiveTool(Move) accepted=%d", ok ? 1 : 0);
      if (!editor.loggedMoveBefore) {
        logObjectPlacement("BEFORE",
                           appState.facade.findObject(selectedObjectId));
        editor.loggedMoveBefore = true;
      }
      creative::CreativeToolInputPacket press;
      press.kind = creative::CreativeToolInputKind::PointerPress;
      press.pointer.button = creative::CreativeToolPointerButton::Primary;
      press.pointer.target =
          creative::TargetRef{static_cast<creative::Id>(selectedObjectId)};
      const creative::CreativeFacadeToolDispatchReceipt r =
          appState.facade.dispatchToolInput(press);
      logMoveDispatch("PRESS", r);
    } else if (editor.frameIndex == 6U) {
      creative::CreativeToolInputPacket move;
      move.kind = creative::CreativeToolInputKind::PointerMove;
      move.pointer.button = creative::CreativeToolPointerButton::Primary;
      move.pointer.hasWorldDestination = true;
      move.pointer.worldDestination = xAxisDestination;
      move.pointer.moveHeldAxis = heldAxisForGrabbedAxis(GizmoAxis::X);
      const creative::CreativeFacadeToolDispatchReceipt r =
          appState.facade.dispatchToolInput(move);
      logMoveDispatch("MOVE", r);
    } else if (editor.frameIndex == 7U) {
      creative::CreativeToolInputPacket release;
      release.kind = creative::CreativeToolInputKind::PointerRelease;
      release.pointer.button = creative::CreativeToolPointerButton::Primary;
      release.pointer.hasWorldDestination = true;
      release.pointer.worldDestination = xAxisDestination;
      release.pointer.moveHeldAxis = heldAxisForGrabbedAxis(GizmoAxis::X);
      const creative::CreativeFacadeToolDispatchReceipt r =
          dispatchMoveReleaseWithUndo(appState, editor.undoStack, release,
                                      selectedObjectId,
                                      "capture_move_release");
      logMoveDispatch("RELEASE", r);
      if (!editor.loggedMoveAfter) {
        logObjectPlacement("AFTER",
                           appState.facade.findObject(selectedObjectId));
        editor.loggedMoveAfter = true;
      }
    }
  } else if (!editor.placeMode &&
             appState.facade.toolState().activeTool == creative::Tool::Move &&
             hasSelection) {
    // Interactive Move: while the Move tool is active and ANY object is
    // selected, hold Left-Alt (releases fly-look) and left-press. A press NEAR
    // a gizmo handle grabs that axis and maps cursor motion ALONG THAT AXIS
    // ONLY; a press away from every handle falls back to the ground-plane move
    // (camera-forward ray -> Y=0 plane). Either way the facade still owns the
    // pick + snap + commit — no per-object move math here.
    const bool* mvKeys = SDL_GetKeyboardState(nullptr);
    const bool altHeld = mvKeys != nullptr && (mvKeys[SDL_SCANCODE_LALT] != 0);
    if (altHeld) {
      float mx = 0.0F;
      float my = 0.0F;
      const SDL_MouseButtonFlags buttons = SDL_GetMouseState(&mx, &my);
      const bool lDown = (buttons & SDL_BUTTON_LMASK) != 0U;
      // High-DPI: scale logical cursor coords to drawable pixels (as select).
      const std::uint32_t logicalW = window.eventState().windowWidth;
      const std::uint32_t logicalH = window.eventState().windowHeight;
      const float scaleX =
          logicalW > 0 ? static_cast<float>(drawableWidth) /
                             static_cast<float>(logicalW)
                       : 1.0F;
      const float scaleY =
          logicalH > 0 ? static_cast<float>(drawableHeight) /
                             static_cast<float>(logicalH)
                       : 1.0F;
      const float cursorPx = mx * scaleX;
      const float cursorPy = my * scaleY;

      const creative::CreativeToolWorldPoint ground =
          resolveCreativeEditorGroundPoint(camera);

      const bool selectedIsPath =
          selected != nullptr &&
          creative::describeObject(selected->kind).shapeKind ==
              creative::CreativeObjectShapeKind::Path;
      if (selectedIsPath) {
        if (lDown && !editor.interactivePathMoveActive &&
            !editor.interactivePathPointMoveActive) {
          PathPointHandleHit handle;
          if (pickPathPointHandle(pathPointHandleHits,
                                  cursorPx,
                                  cursorPy,
                                  handle)) {
            editor.interactivePathPointMoveActive = true;
            editor.interactivePathPointMoveObjectId = handle.objectId;
            editor.interactivePathPointMoveIndex = handle.pointIndex;
            editor.interactivePathPointMoveStartGround = ground;
            SDL_Log("iggy3d_creative: PATH_HANDLE interactive move begin "
                    "objectId=%llu pointIndex=%zu ground=(%.3f, %.3f, %.3f) "
                    "position=(%.3f, %.3f, %.3f) pathPoints='%s'",
                    static_cast<unsigned long long>(handle.objectId),
                    handle.pointIndex, ground.x, ground.y, ground.z,
                    handle.position.x, handle.position.y, handle.position.z,
                    pathPointsSummary(selected->pathPoints).c_str());
          } else {
            editor.interactivePathMoveActive = true;
            editor.interactivePathMoveObjectId = selectedObjectId;
            editor.interactivePathMoveStartGround = ground;
            SDL_Log("iggy3d_creative: PATH interactive move begin objectId=%llu "
                    "ground=(%.3f, %.3f, %.3f) pathPoints='%s'",
                    static_cast<unsigned long long>(selectedObjectId),
                    ground.x, ground.y, ground.z,
                    pathPointsSummary(selected->pathPoints).c_str());
          }
        } else if (!lDown && editor.interactivePathPointMoveActive) {
          const creative::CreativeVec3 delta{
              ground.x - editor.interactivePathPointMoveStartGround.x,
              0.0,
              ground.z - editor.interactivePathPointMoveStartGround.z};
          const creative::CreativeDocumentMutationReceipt receipt =
              movePathPointWithUndo(appState,
                                    editor.undoStack,
                                    editor.interactivePathPointMoveObjectId,
                                    editor.interactivePathPointMoveIndex,
                                    delta,
                                    "path_point_move_interactive_release");
          SDL_Log("iggy3d_creative: PATH_HANDLE interactive move release "
                  "objectId=%llu pointIndex=%zu status='%s' changed=%d "
                  "delta=(%.3f, %.3f, %.3f)",
                  static_cast<unsigned long long>(
                      editor.interactivePathPointMoveObjectId),
                  editor.interactivePathPointMoveIndex,
                  std::string(creative::toString(receipt.status)).c_str(),
                  receipt.changed ? 1 : 0, delta.x, delta.y, delta.z);
          editor.interactivePathPointMoveActive = false;
          editor.interactivePathPointMoveObjectId = creative::kInvalidObjectId;
          editor.interactivePathPointMoveIndex = 0U;
        } else if (!lDown && editor.interactivePathMoveActive) {
          const creative::CreativeVec3 delta{
              ground.x - editor.interactivePathMoveStartGround.x,
              0.0,
              ground.z - editor.interactivePathMoveStartGround.z};
          const creative::CreativeDocumentMutationReceipt receipt =
              movePathObjectWithUndo(appState,
                                     editor.undoStack,
                                     editor.interactivePathMoveObjectId,
                                     delta,
                                     "path_move_interactive_release");
          SDL_Log("iggy3d_creative: PATH interactive move release objectId=%llu "
                  "status='%s' changed=%d delta=(%.3f, %.3f, %.3f)",
                  static_cast<unsigned long long>(
                      editor.interactivePathMoveObjectId),
                  std::string(creative::toString(receipt.status)).c_str(),
                  receipt.changed ? 1 : 0, delta.x, delta.y, delta.z);
          editor.interactivePathMoveActive = false;
          editor.interactivePathMoveObjectId = creative::kInvalidObjectId;
        }
      } else {
      // Build the pointer packet's worldDestination + moveHeldAxis. When an
      // axis handle is grabbed, project the cursor delta since grab onto the
      // shaft's screen direction, scale it to world length along the axis, and
      // set worldDestination = S with only the grabbed axis advanced (the other
      // two pinned to S), moveHeldAxis = one of the two non-grabbed axes.
      const auto buildConstrainedDestination =
          [&](creative::CreativeToolWorldPoint& dest,
              creative::CreativeToolMoveHeldAxis& held) {
            if (editor.interactiveGrabbedAxis == GizmoAxis::None) {
              dest = ground;  // Free ground-plane move.
              held = creative::CreativeToolMoveHeldAxis::Y;
              return;
            }
            // Screen-space shaft direction at grab time (center -> tip).
            float sdx = editor.interactiveGrabTipScreen.x -
                        editor.interactiveGrabCenterScreen.x;
            float sdy = editor.interactiveGrabTipScreen.y -
                        editor.interactiveGrabCenterScreen.y;
            const float slen = std::sqrt(sdx * sdx + sdy * sdy);
            float along = 0.0F;
            if (slen > 1.0e-3F) {
              sdx /= slen;
              sdy /= slen;
              const float cdx = cursorPx - editor.interactiveGrabCursorX;
              const float cdy = cursorPy - editor.interactiveGrabCursorY;
              // pixels moved along the shaft / pixels per shaft * world length.
              const float alongPx = cdx * sdx + cdy * sdy;
              along = (alongPx / slen) * axisLengthMeters;
            }
            dest.x = static_cast<double>(editor.interactiveGrabAnchorS.x);
            dest.y = static_cast<double>(editor.interactiveGrabAnchorS.y);
            dest.z = static_cast<double>(editor.interactiveGrabAnchorS.z);
            switch (editor.interactiveGrabbedAxis) {
              case GizmoAxis::X:
                dest.x += static_cast<double>(along);
                break;
              case GizmoAxis::Y:
                dest.y += static_cast<double>(along);
                break;
              case GizmoAxis::Z:
                dest.z += static_cast<double>(along);
                break;
              case GizmoAxis::None:
              default:
                break;
            }
            held = heldAxisForGrabbedAxis(editor.interactiveGrabbedAxis);
          };

      if (lDown && !editor.moveDragButtonDown) {
        editor.moveDragButtonDown = true;
        // Grab an axis handle if the press landed near one; else free move.
        editor.interactiveGrabbedAxis = pickGizmoAxisFromProjectedShafts(
            gizmoShafts,
            gizmoCenterScreen,
            gizmoTipScreen,
            cursorPx,
            cursorPy,
            handleThresholdPx);
        editor.interactiveGrabAnchorS = gizmoAnchorS;
        editor.interactiveGrabCursorX = cursorPx;
        editor.interactiveGrabCursorY = cursorPy;
        editor.interactiveGrabCenterScreen = gizmoCenterScreen;
        if (editor.interactiveGrabbedAxis == GizmoAxis::X) {
          editor.interactiveGrabTipScreen = gizmoTipScreen[0];
        } else if (editor.interactiveGrabbedAxis == GizmoAxis::Y) {
          editor.interactiveGrabTipScreen = gizmoTipScreen[1];
        } else if (editor.interactiveGrabbedAxis == GizmoAxis::Z) {
          editor.interactiveGrabTipScreen = gizmoTipScreen[2];
        }
        SDL_Log("iggy3d_creative: GIZMO grabbed axis=%s",
                gizmoAxisName(editor.interactiveGrabbedAxis));
        creative::CreativeToolInputPacket press;
        press.kind = creative::CreativeToolInputKind::PointerPress;
        press.pointer.button = creative::CreativeToolPointerButton::Primary;
        press.pointer.target =
            creative::TargetRef{static_cast<creative::Id>(selectedObjectId)};
        (void)appState.facade.dispatchToolInput(press);
      } else if (lDown && editor.moveDragButtonDown) {
        creative::CreativeToolInputPacket move;
        move.kind = creative::CreativeToolInputKind::PointerMove;
        move.pointer.button = creative::CreativeToolPointerButton::Primary;
        move.pointer.hasWorldDestination = true;
        buildConstrainedDestination(move.pointer.worldDestination,
                                    move.pointer.moveHeldAxis);
        (void)appState.facade.dispatchToolInput(move);
      } else if (!lDown && editor.moveDragButtonDown) {
        editor.moveDragButtonDown = false;
        creative::CreativeToolInputPacket release;
        release.kind = creative::CreativeToolInputKind::PointerRelease;
        release.pointer.button = creative::CreativeToolPointerButton::Primary;
        release.pointer.hasWorldDestination = true;
        buildConstrainedDestination(release.pointer.worldDestination,
                                    release.pointer.moveHeldAxis);
        const creative::CreativeFacadeToolDispatchReceipt r =
            dispatchMoveReleaseWithUndo(appState, editor.undoStack, release,
                                        selectedObjectId,
                                        "move_interactive_release");
        logMoveDispatch("RELEASE", r);
        editor.interactiveGrabbedAxis = GizmoAxis::None;
      }
      }
    } else if (editor.moveDragButtonDown) {
      editor.moveDragButtonDown = false;  // Alt released mid-drag: drop the latch.
      editor.interactiveGrabbedAxis = GizmoAxis::None;
    } else if (editor.interactivePathMoveActive) {
      SDL_Log("iggy3d_creative: PATH interactive move cancelled objectId=%llu",
              static_cast<unsigned long long>(
                  editor.interactivePathMoveObjectId));
      editor.interactivePathMoveActive = false;
      editor.interactivePathMoveObjectId = creative::kInvalidObjectId;
    } else if (editor.interactivePathPointMoveActive) {
      SDL_Log("iggy3d_creative: PATH_HANDLE interactive move cancelled "
              "objectId=%llu pointIndex=%zu",
              static_cast<unsigned long long>(
                  editor.interactivePathPointMoveObjectId),
              editor.interactivePathPointMoveIndex);
      editor.interactivePathPointMoveActive = false;
      editor.interactivePathPointMoveObjectId = creative::kInvalidObjectId;
      editor.interactivePathPointMoveIndex = 0U;
    }
  }
}

}  // namespace iggy3d_creative_app
