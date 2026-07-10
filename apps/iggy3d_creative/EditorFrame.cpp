#include "EditorFrame.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>
#include <thread>

#include "app/iggy3d/creative/camera/Fly.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"
#include "render/vulkan/VulkanBackend.hpp"

#include "EditorPlacement.hpp"
#include "EditorEdits.hpp"
#include "EditorPersistence.hpp"
#include "EditorPicking.hpp"
#include "EditorFrustumCull.hpp"
#include "EditorPreviewFrame.hpp"
#include "EditorPreviewProxies.hpp"

namespace iggy3d_creative_app {

[[nodiscard]] iggy3d::creative::CreativeToolWorldPoint
resolveCreativeEditorGroundPoint(const iggy3d::RenderCameraFrame& camera) {
  const iggy3d::Vec3 eye = camera.worldEye;
  const iggy3d::Vec3 fwd = camera.worldForward;
  iggy3d::creative::CreativeToolWorldPoint ground{eye.x, 0.0, eye.z};
  if (std::fabs(fwd.y) > 1.0e-4F) {
    const float t = -eye.y / fwd.y;  // eye.y + t*fwd.y == 0
    if (t > 0.0F) {
      ground.x = static_cast<double>(eye.x + fwd.x * t);
      ground.z = static_cast<double>(eye.z + fwd.z * t);
    }
  }
  return ground;
}

[[nodiscard]] iggy3d::Vec3 resolveCreativeEditorAimCell(
    const iggy3d::RenderCameraFrame& camera,
    double placeCellSize) {
  const iggy3d::creative::CreativeToolWorldPoint ground =
      resolveCreativeEditorGroundPoint(camera);
  return snapGroundToCellCenter(ground.x, ground.z, placeCellSize);
}

void applyCreativeEditorClickSelection(
    iggy3d::SdlWindow& window,
    iggy3d::creative::CreativeAppState& appState,
    const iggy3d::RenderCameraFrame& camera,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    const CreativeEditorPickFrame& pickFrame,
    CreativeEditorState& editor,
    bool captureMode) {
  bool clickRequested = false;
  float clickX = 0.0F;
  float clickY = 0.0F;
  if (captureMode && editor.placeMode) {
    // Place-mode capture drops objects directly in the capture script.
  } else if (captureMode) {
    if (editor.frameIndex == 3U && pickFrame.haveFloorBounds) {
      const iggy3d::Vec3 floorTopCorner{
          pickFrame.floorBoxMin.x +
              (pickFrame.floorBoxMax.x - pickFrame.floorBoxMin.x) * 0.85F,
          pickFrame.floorBoxMax.y,
          pickFrame.floorBoxMin.z +
              (pickFrame.floorBoxMax.z - pickFrame.floorBoxMin.z) * 0.85F};
      const ScreenPoint p = projectPointToScreen(
          camera.clipFromWorld, floorTopCorner, drawableWidth, drawableHeight);
      if (p.valid) {
        clickRequested = true;
        clickX = p.x;
        clickY = p.y;
      }
    }
  } else if (!editor.placeMode) {
    const bool* selKeys = SDL_GetKeyboardState(nullptr);
    const bool altHeld =
        selKeys != nullptr && (selKeys[SDL_SCANCODE_LALT] != 0);
    if (altHeld) {
      window.setRelativeMouseMode(false);
      float mx = 0.0F;
      float my = 0.0F;
      const SDL_MouseButtonFlags buttons = SDL_GetMouseState(&mx, &my);
      const bool leftDown = (buttons & SDL_BUTTON_LMASK) != 0U;
      if (leftDown && !editor.selectionButtonDown) {
        editor.selectionButtonDown = true;
        clickRequested = true;
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
        clickX = mx * scaleX;
        clickY = my * scaleY;
      } else if (!leftDown) {
        editor.selectionButtonDown = false;
      }
    } else {
      window.setRelativeMouseMode(true);
      editor.selectionButtonDown = false;
    }
  } else {
    editor.selectionButtonDown = false;
  }

  if (clickRequested) {
    const WorldRay ray =
        worldRayFromPixel(camera, clickX, clickY, drawableWidth, drawableHeight);
    const ObjectVisualPickResult pick = pickNearestVisualBoundsObject(
        pickFrame.objectPickCandidates, ray);
    const iggy3d::creative::CreativeObjectId pickedId = pick.objectId;
    SDL_Log("iggy3d_creative: WORLD_PICK click=(%.1f, %.1f) rayValid=%d "
            "tested=%llu hits=%llu pickedObjectId=%llu entryDistance=%.3f",
            clickX, clickY, pick.rayValid ? 1 : 0,
            static_cast<unsigned long long>(pick.testedCount),
            static_cast<unsigned long long>(pick.hitCount),
            static_cast<unsigned long long>(pickedId), pick.entryDistance);
    iggy3d::creative::CreativeToolInputPacket packet;
    packet.kind = iggy3d::creative::CreativeToolInputKind::PointerPress;
    packet.pointer.button = iggy3d::creative::CreativeToolPointerButton::Primary;
    if ((SDL_GetModState() & SDL_KMOD_SHIFT) != 0U) {
      packet.pointer.modifiers |=
          iggy3d::creative::kCreativeToolModifierShift;
    }
    if (pickedId != iggy3d::creative::kInvalidObjectId) {
      packet.pointer.target =
          iggy3d::creative::TargetRef{
              static_cast<iggy3d::creative::Id>(pickedId)};
    }
    (void)appState.facade.dispatchToolInput(packet);
  }
}

namespace creative = iggy3d::creative;
namespace {

void setSdlKey(creative::CreativeInputFrame& frame,
               creative::CreativeInputKey key,
               const bool* keys,
               SDL_Scancode scancode) {
  creative::setCreativeInputKey(frame, key,
                                keys != nullptr && keys[scancode] != 0);
}

[[nodiscard]] creative::CreativeInputFrame makeCreativeInputFrame(
    const bool* keys,
    SDL_Keymod modifiers,
    creative::CreativeInputContext context) {
  creative::CreativeInputFrame frame;
  frame.context = context;
  if ((modifiers & SDL_KMOD_SHIFT) != 0U) {
    frame.modifiers |= creative::kCreativeInputModifierShift;
  }
  if ((modifiers & SDL_KMOD_CTRL) != 0U) {
    frame.modifiers |= creative::kCreativeInputModifierControl;
  }
  if ((modifiers & SDL_KMOD_ALT) != 0U) {
    frame.modifiers |= creative::kCreativeInputModifierAlt;
  }
  if ((modifiers & SDL_KMOD_GUI) != 0U) {
    frame.modifiers |= creative::kCreativeInputModifierCommand;
  }

  setSdlKey(frame, creative::CreativeInputKey::Digit1, keys, SDL_SCANCODE_1);
  setSdlKey(frame, creative::CreativeInputKey::Digit2, keys, SDL_SCANCODE_2);
  setSdlKey(frame, creative::CreativeInputKey::Digit3, keys, SDL_SCANCODE_3);
  setSdlKey(frame, creative::CreativeInputKey::B, keys, SDL_SCANCODE_B);
  setSdlKey(frame, creative::CreativeInputKey::D, keys, SDL_SCANCODE_D);
  setSdlKey(frame, creative::CreativeInputKey::Z, keys, SDL_SCANCODE_Z);
  setSdlKey(frame, creative::CreativeInputKey::LeftBracket, keys,
            SDL_SCANCODE_LEFTBRACKET);
  setSdlKey(frame, creative::CreativeInputKey::RightBracket, keys,
            SDL_SCANCODE_RIGHTBRACKET);
  setSdlKey(frame, creative::CreativeInputKey::Minus, keys,
            SDL_SCANCODE_MINUS);
  setSdlKey(frame, creative::CreativeInputKey::Equals, keys,
            SDL_SCANCODE_EQUALS);
  setSdlKey(frame, creative::CreativeInputKey::Delete, keys,
            SDL_SCANCODE_DELETE);
  setSdlKey(frame, creative::CreativeInputKey::Backspace, keys,
            SDL_SCANCODE_BACKSPACE);
  setSdlKey(frame, creative::CreativeInputKey::F5, keys, SDL_SCANCODE_F5);
  setSdlKey(frame, creative::CreativeInputKey::F6, keys, SDL_SCANCODE_F6);
  setSdlKey(frame, creative::CreativeInputKey::F9, keys, SDL_SCANCODE_F9);
  setSdlKey(frame, creative::CreativeInputKey::W, keys, SDL_SCANCODE_W);
  setSdlKey(frame, creative::CreativeInputKey::A, keys, SDL_SCANCODE_A);
  setSdlKey(frame, creative::CreativeInputKey::S, keys, SDL_SCANCODE_S);
  setSdlKey(frame, creative::CreativeInputKey::Space, keys,
            SDL_SCANCODE_SPACE);
  setSdlKey(frame, creative::CreativeInputKey::LeftControl, keys,
            SDL_SCANCODE_LCTRL);
  setSdlKey(frame, creative::CreativeInputKey::RightControl, keys,
            SDL_SCANCODE_RCTRL);
  setSdlKey(frame, creative::CreativeInputKey::LeftShift, keys,
            SDL_SCANCODE_LSHIFT);
  setSdlKey(frame, creative::CreativeInputKey::RightShift, keys,
            SDL_SCANCODE_RSHIFT);
  setSdlKey(frame, creative::CreativeInputKey::LeftAlt, keys,
            SDL_SCANCODE_LALT);
  setSdlKey(frame, creative::CreativeInputKey::RightAlt, keys,
            SDL_SCANCODE_RALT);
  setSdlKey(frame, creative::CreativeInputKey::LeftCommand, keys,
            SDL_SCANCODE_LGUI);
  setSdlKey(frame, creative::CreativeInputKey::RightCommand, keys,
            SDL_SCANCODE_RGUI);
  return frame;
}

[[nodiscard]] bool navigationKeyDown(
    const creative::CreativeInputFrame& frame,
    const creative::CreativeInputRouteResult& routedInput,
    creative::CreativeInputKey key) noexcept {
  return creative::creativeInputKeyDown(frame, key) &&
         !creative::creativeInputKeyConsumed(routedInput, key);
}

}  // namespace

void applyCreativeEditorCommandInput(
    const creative::CreativeInputRouteResult& routedInput,
    creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    const std::filesystem::path& saveRoot,
    const std::string& saveId) {
  for (const creative::CreativeInputActionEvent& event :
       routedInput.actionEvents()) {
    switch (event.action) {
      case creative::CreativeInputActionId::SelectTool: {
        editor.placeMode = false;
        const bool ok = appState.facade.setActiveTool(creative::Tool::Select);
        SDL_Log("iggy3d_creative: setActiveTool(Select) accepted=%d placeMode=0",
                ok ? 1 : 0);
        break;
      }
      case creative::CreativeInputActionId::MoveTool: {
        editor.placeMode = false;
        const bool ok = appState.facade.setActiveTool(creative::Tool::Move);
        SDL_Log("iggy3d_creative: setActiveTool(Move) accepted=%d placeMode=0",
                ok ? 1 : 0);
        break;
      }
      case creative::CreativeInputActionId::EnterPlaceMode:
        editor.placeMode = true;
        SDL_Log("iggy3d_creative: placeMode=1 brush='%s'",
                std::string(creative::toString(editor.placeBrush)).c_str());
        break;
      case creative::CreativeInputActionId::CycleBrush:
        editor.placeBrush = nextBrushKind(editor.brushPalette, editor.placeBrush);
        SDL_Log("iggy3d_creative: brush cycled -> '%s'",
                std::string(creative::toString(editor.placeBrush)).c_str());
        break;
      case creative::CreativeInputActionId::DeleteSelection:
        (void)deleteSelectedObject(
            appState,
            event.trigger == creative::CreativeInputKey::Backspace
                ? "backspace_key"
                : "delete_key",
            &editor.undoStack);
        break;
      case creative::CreativeInputActionId::Undo:
        (void)undoLastSnapshot(appState, editor.undoStack, "keyboard_undo");
        break;
      case creative::CreativeInputActionId::DuplicateSelection:
        (void)duplicateSelectedObjectsWithUndo(
            appState, editor.undoStack,
            creative::CreativeDuplicateCommandRequest{}, "keyboard_duplicate");
        break;
      case creative::CreativeInputActionId::RotateYawNegative:
      case creative::CreativeInputActionId::RotateYawPositive: {
        creative::CreativeTransformCommandRequest request;
        request.kind = creative::CreativeTransformCommandKind::RotateYaw;
        request.yawDegrees =
            event.action == creative::CreativeInputActionId::RotateYawNegative
                ? -15.0
                : 15.0;
        (void)transformSelectedObjectsWithUndo(
            appState, editor.undoStack, request,
            request.yawDegrees < 0.0 ? "keyboard_rotate_yaw_negative"
                                     : "keyboard_rotate_yaw_positive");
        break;
      }
      case creative::CreativeInputActionId::ScaleDown:
      case creative::CreativeInputActionId::ScaleUp: {
        creative::CreativeTransformCommandRequest request;
        request.kind = creative::CreativeTransformCommandKind::Scale;
        const double factor =
            event.action == creative::CreativeInputActionId::ScaleDown ? 0.9
                                                                        : 1.1;
        request.scaleFactor = {factor, factor, factor};
        (void)transformSelectedObjectsWithUndo(
            appState, editor.undoStack, request,
            factor < 1.0 ? "keyboard_scale_down" : "keyboard_scale_up");
        break;
      }
      case creative::CreativeInputActionId::Save: {
        const iggy3d::CreativeWorldSaveResult saveResult =
            saveStandaloneScene(appState.facade, saveRoot, saveId);
        if (saveResult.accepted && saveResult.saved) {
          clearUndoStack(editor.undoStack, "save_success");
        }
        break;
      }
      case creative::CreativeInputActionId::NewDocument:
        clearToBlankScene(appState);
        clearUndoStack(editor.undoStack, "new_clear");
        break;
      case creative::CreativeInputActionId::Load: {
        const bool loaded = loadStandaloneScene(appState, saveRoot, saveId);
        if (loaded) {
          clearUndoStack(editor.undoStack, "load_success");
        }
        break;
      }
    }
  }
}

namespace {

constexpr float kMouseSensitivity = 0.12F;

}  // namespace

CreativeEditorFrameInputResult beginCreativeEditorFrameInput(
    iggy3d::SdlWindow& window,
    iggy3d::VulkanBackend& backend,
    CreativeEditorState& editor,
    bool captureMode) {
  CreativeEditorFrameInputResult result;

  window.pollEvents();
  if (window.eventState().quitRequested) {
    result.keepRunning = false;
    return result;
  }
  if (!window.isDrawable()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
    result.skipFrame = true;
    return result;
  }

  const iggy3d::SdlDrawableExtent extent = window.drawableExtent();
  if (extent.width == 0U || extent.height == 0U) {
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
    result.skipFrame = true;
    return result;
  }
  result.extent = extent;

  if (extent.width != editor.lastWidth || extent.height != editor.lastHeight) {
    iggy3d::RenderViewport viewport;
    viewport.width = extent.width;
    viewport.height = extent.height;
    viewport.aspectRatio =
        static_cast<float>(extent.width) / static_cast<float>(extent.height);
    backend.resize(viewport);
    editor.lastWidth = extent.width;
    editor.lastHeight = extent.height;
  }

  const bool* keys = SDL_GetKeyboardState(nullptr);
  const creative::CreativeInputFrame inputFrame = makeCreativeInputFrame(
      keys, SDL_GetModState(),
      captureMode ? creative::CreativeInputContext::Capture
                  : creative::CreativeInputContext::EditorViewport);
  result.routedInput =
      creative::routeCreativeInput(editor.inputRouterState, inputFrame);
  iggy3d::ProductCreativeFlyInput flyInput;
  if (!captureMode) {
    const float moveX =
        (navigationKeyDown(inputFrame, result.routedInput,
                           creative::CreativeInputKey::D)
             ? 1.0F
             : 0.0F) -
        (navigationKeyDown(inputFrame, result.routedInput,
                           creative::CreativeInputKey::A)
             ? 1.0F
             : 0.0F);
    const float moveY =
        (navigationKeyDown(inputFrame, result.routedInput,
                           creative::CreativeInputKey::W)
             ? 1.0F
             : 0.0F) -
        (navigationKeyDown(inputFrame, result.routedInput,
                           creative::CreativeInputKey::S)
             ? 1.0F
             : 0.0F);
    const float moveZ =
        (navigationKeyDown(inputFrame, result.routedInput,
                           creative::CreativeInputKey::Space)
             ? 1.0F
             : 0.0F) -
        (navigationKeyDown(inputFrame, result.routedInput,
                           creative::CreativeInputKey::LeftControl)
             ? 1.0F
             : 0.0F);
    flyInput.moveX = moveX;
    flyInput.moveY = moveY;
    flyInput.moveZ = moveZ;
    flyInput.sprinting = navigationKeyDown(
        inputFrame, result.routedInput, creative::CreativeInputKey::LeftShift);
  }

  float mouseDx = 0.0F;
  float mouseDy = 0.0F;
  SDL_GetRelativeMouseState(&mouseDx, &mouseDy);
  if (!captureMode) {
    editor.yawDegrees += mouseDx * kMouseSensitivity;
    editor.pitchDegrees = std::clamp(
        editor.pitchDegrees - mouseDy * kMouseSensitivity, -80.0F, 80.0F);
  }
  flyInput.cameraYawDegrees = editor.yawDegrees;
  flyInput.cameraPitchDegrees = editor.pitchDegrees;

  const iggy3d::ProductCreativeFlyResult flyResult =
      applyProductCreativeFlyInput(editor.flyConfig, flyInput, editor.flyPos);
  if (flyResult.applied) {
    editor.flyPos = flyResult.finalPositionMeters;
  }

  return result;
}

CreativeEditorPickFrame buildCreativeEditorPickFrame(
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::RenderCameraFrame& camera,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    iggy3d::creative::CreativeObjectId floorObjectId,
    StandaloneCaptureScript& captureScript,
    bool captureMode) {
  CreativeEditorPickFrame frame;
  for (const iggy3d::creative::CreativeObject& obj : document.objects()) {
    if (!obj.visible) {
      continue;
    }
    const ObjectVisualPickBounds hit = buildObjectVisualPickBounds(
        obj, camera.clipFromWorld, drawableWidth, drawableHeight);
    const iggy3d::Vec3 boxMin = hit.bounds.min;
    const iggy3d::Vec3 boxMax = hit.bounds.max;
    frame.objectPickCandidates.push_back(hit);
    if (obj.id == floorObjectId) {
      frame.haveFloorBounds = true;
      frame.floorBoxMin = boxMin;
      frame.floorBoxMax = boxMax;
    }
    if (captureMode && !captureScript.pointHitProxyLogged &&
        obj.id == captureScript.pointTargetId) {
      SDL_Log("iggy3d_creative: POINT hit proxy objectId=%llu "
              "aabbValid=%d marker=[(%.3f, %.3f, %.3f).."
              "(%.3f, %.3f, %.3f)] screen=[%.1f, %.1f..%.1f, %.1f]",
              static_cast<unsigned long long>(captureScript.pointTargetId),
              hit.screenAabb.valid ? 1 : 0, boxMin.x, boxMin.y, boxMin.z,
              boxMax.x, boxMax.y, boxMax.z, hit.screenAabb.minX,
              hit.screenAabb.minY, hit.screenAabb.maxX, hit.screenAabb.maxY);
      captureScript.pointHitProxyLogged = true;
    }
    if (captureMode && !captureScript.lineHitProxyLogged &&
        obj.id == captureScript.lineTargetId) {
      SDL_Log("iggy3d_creative: LINE hit proxy objectId=%llu "
              "aabbValid=%d visual=[(%.3f, %.3f, %.3f).."
              "(%.3f, %.3f, %.3f)] screen=[%.1f, %.1f..%.1f, %.1f]",
              static_cast<unsigned long long>(captureScript.lineTargetId),
              hit.screenAabb.valid ? 1 : 0, boxMin.x, boxMin.y, boxMin.z,
              boxMax.x, boxMax.y, boxMax.z, hit.screenAabb.minX,
              hit.screenAabb.minY, hit.screenAabb.maxX, hit.screenAabb.maxY);
      captureScript.lineHitProxyLogged = true;
    }
    if (captureMode && !captureScript.pathHitProxyLogged &&
        obj.id == captureScript.pathTargetId) {
      SDL_Log("iggy3d_creative: PATH hit proxy objectId=%llu "
              "aabbValid=%d visual=[(%.3f, %.3f, %.3f).."
              "(%.3f, %.3f, %.3f)] screen=[%.1f, %.1f..%.1f, %.1f] "
              "pathPointCount=%zu pathPoints='%s'",
              static_cast<unsigned long long>(captureScript.pathTargetId),
              hit.screenAabb.valid ? 1 : 0, boxMin.x, boxMin.y, boxMin.z,
              boxMax.x, boxMax.y, boxMax.z, hit.screenAabb.minX,
              hit.screenAabb.minY, hit.screenAabb.maxX, hit.screenAabb.maxY,
              obj.pathPoints.size(), pathPointsSummary(obj.pathPoints).c_str());
      captureScript.pathHitProxyLogged = true;
    }
  }
  return frame;
}

void applyCreativeEditorPlacementInput(
    iggy3d::SdlWindow& window,
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    iggy3d::Vec3 aimCellCenter,
    bool captureMode) {
  if (!editor.placeMode || captureMode) {
    return;
  }

  const bool* plKeys = SDL_GetKeyboardState(nullptr);
  const bool altHeld =
      plKeys != nullptr && (plKeys[SDL_SCANCODE_LALT] != 0);
  if (altHeld) {
    window.setRelativeMouseMode(false);
    float mx = 0.0F;
    float my = 0.0F;
    const SDL_MouseButtonFlags buttons = SDL_GetMouseState(&mx, &my);
    const bool lDown = (buttons & SDL_BUTTON_LMASK) != 0U;
    if (lDown && !editor.placeButtonDown) {
      editor.placeButtonDown = true;
      (void)placeBrushObjectWithUndo(appState.facade,
                                     editor.undoStack,
                                     editor.placeBrush,
                                     aimCellCenter,
                                     ++editor.placedCount,
                                     "place_interactive");
    } else if (!lDown) {
      editor.placeButtonDown = false;
    }
  } else {
    window.setRelativeMouseMode(true);
    editor.placeButtonDown = false;
  }
}

CreativeEditorSelectionFrame resolveCreativeEditorSelectionFrame(
    const iggy3d::creative::Facade& facade) {
  CreativeEditorSelectionFrame selection;
  const iggy3d::creative::CreativeSelectionState& selectionState =
      facade.selectionState();
  selection.selectedId = selectionState.selectedTarget.value;
  selection.selected =
      selection.selectedId != 0
          ? facade.findObject(static_cast<iggy3d::creative::CreativeObjectId>(
                selection.selectedId))
          : nullptr;
  const std::span<const iggy3d::creative::TargetRef> targets =
      iggy3d::creative::selectedTargetList(selectionState);
  if (targets.empty() && selection.selected != nullptr) {
    selection.selectedObjectIds.push_back(selection.selected->id);
  } else {
    selection.selectedObjectIds.reserve(targets.size());
    for (iggy3d::creative::TargetRef target : targets) {
      if (target.value != iggy3d::creative::kInvalidId) {
        selection.selectedObjectIds.push_back(
            static_cast<iggy3d::creative::CreativeObjectId>(target.value));
      }
    }
  }
  selection.selectionCount = selection.selectedObjectIds.size();

  bool haveBounds = false;
  for (iggy3d::creative::CreativeObjectId objectId :
       selection.selectedObjectIds) {
    const iggy3d::creative::CreativeObject* object = facade.findObject(objectId);
    if (object == nullptr || !object->visible) {
      continue;
    }
    const VisualBounds bounds = visualBoundsForObject(*object);
    if (!haveBounds) {
      selection.boxMin = bounds.min;
      selection.boxMax = bounds.max;
      haveBounds = true;
      continue;
    }
    selection.boxMin.x = std::min(selection.boxMin.x, bounds.min.x);
    selection.boxMin.y = std::min(selection.boxMin.y, bounds.min.y);
    selection.boxMin.z = std::min(selection.boxMin.z, bounds.min.z);
    selection.boxMax.x = std::max(selection.boxMax.x, bounds.max.x);
    selection.boxMax.y = std::max(selection.boxMax.y, bounds.max.y);
    selection.boxMax.z = std::max(selection.boxMax.z, bounds.max.z);
  }
  selection.hasSelection = haveBounds;
  return selection;
}

void logCreativeEditorWorldPickProofFrame(
    const iggy3d::creative::Facade& facade,
    const iggy3d::RenderCameraFrame& camera,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    const CreativeEditorPickFrame& pickFrame,
    iggy3d::creative::CreativeObjectId floorObjectId,
    CreativeEditorState& editor,
    bool captureMode) {
  if (!captureMode) {
    return;
  }

  auto logWorldPickProof =
      [&](const char* label, iggy3d::creative::CreativeObjectId expectedId,
          iggy3d::Vec3 worldPoint, bool& logged) {
        if (logged || expectedId == iggy3d::creative::kInvalidObjectId) {
          return;
        }
        const ScreenPoint screenPoint = projectPointToScreen(
            camera.clipFromWorld, worldPoint, drawableWidth, drawableHeight);
        if (!screenPoint.valid) {
          return;
        }
        const WorldRay ray =
            worldRayFromPixel(camera, screenPoint.x, screenPoint.y,
                              drawableWidth, drawableHeight);
        const ObjectVisualPickResult pick = pickNearestVisualBoundsObject(
            pickFrame.objectPickCandidates, ray);
        SDL_Log("iggy3d_creative: WORLD_PICK_PROOF label='%s' "
                "click=(%.1f, %.1f) rayValid=%d expectedObjectId=%llu "
                "pickedObjectId=%llu matched=%d entryDistance=%.3f "
                "tested=%llu hits=%llu",
                label, screenPoint.x, screenPoint.y, pick.rayValid ? 1 : 0,
                static_cast<unsigned long long>(expectedId),
                static_cast<unsigned long long>(pick.objectId),
                pick.objectId == expectedId ? 1 : 0, pick.entryDistance,
                static_cast<unsigned long long>(pick.testedCount),
                static_cast<unsigned long long>(pick.hitCount));
        logged = true;
      };

  if (!editor.captureWorldPickFloorLogged && pickFrame.haveFloorBounds) {
    const iggy3d::Vec3 floorTopCorner{
        pickFrame.floorBoxMin.x +
            (pickFrame.floorBoxMax.x - pickFrame.floorBoxMin.x) * 0.85F,
        pickFrame.floorBoxMax.y,
        pickFrame.floorBoxMin.z +
            (pickFrame.floorBoxMax.z - pickFrame.floorBoxMin.z) * 0.85F};
    logWorldPickProof("floor_overlap", floorObjectId, floorTopCorner,
                      editor.captureWorldPickFloorLogged);
  }

  auto logObjectCenterPick =
      [&](const char* label, iggy3d::creative::CreativeObjectId expectedId,
          bool& logged) {
        const iggy3d::creative::CreativeObject* object =
            facade.findObject(expectedId);
        if (object == nullptr) {
          return;
        }
        logWorldPickProof(label,
                          expectedId,
                          visualBoundsCenter(visualBoundsForObject(*object)),
                          logged);
      };
  logObjectCenterPick("point_proxy", editor.captureScript.pointTargetId,
                     editor.captureWorldPickPointLogged);
  logObjectCenterPick("line_proxy", editor.captureScript.lineTargetId,
                      editor.captureWorldPickLineLogged);
  logObjectCenterPick("path_proxy", editor.captureScript.pathTargetId,
                     editor.captureWorldPickPathLogged);
}

[[nodiscard]] bool submitCreativeEditorFrame(
    const CreativeEditorSubmitFrameRequest& request) {
  iggy3d::FrameInput& frame = request.frame;
  const iggy3d::creative::CreativeAppState& appState = request.appState;
  CreativeEditorState& editor = request.editor;
  const CreativeEditorSelectionFrame& selection = request.selection;
  const CreativeEditorOverlayFrame& overlayFrame = request.overlayFrame;
  const StandaloneRoomBakePreviewScene& roomBakePreview =
      request.roomBakePreview;
  const iggy3d::creative::Id selectedId = selection.selectedId;
  const iggy3d::creative::CreativeObject* selected = selection.selected;
  const bool hasSelection = selection.hasSelection;

  const StandaloneFrustumCullResult frustumCull =
      cullStandaloneSceneRoomMeshesByFrustum(
          *frame.projections.scene, frame.camera.clipFromWorld);
  frame.projections.scene = &frustumCull.scene;

  const iggy3d::RenderSubmitResult submit =
      request.backend.submitFrame(frame);
  if (!editor.loggedSelection) {
    editor.loggedSelection = true;
    SDL_Log("iggy3d_creative: frame %llu submit outcome=%d reason='%s' "
            "meshes=%zu frustumInputMeshes=%zu frustumKeptMeshes=%zu "
            "frustumCulledMeshes=%zu frustumConservativeMeshes=%zu "
            "selectedTarget=%u hasSelection=%d selBoxLines=%zu "
            "pointMarkerLines=%zu lineMarkerLines=%zu pathHandleLines=%zu "
            "gizmoLines=%zu combinedWireLines=%zu uiRects=%zu glyphs=%zu",
            static_cast<unsigned long long>(editor.frameIndex),
            static_cast<int>(submit.outcome),
            std::string(submit.reason.code).c_str(),
            frustumCull.scene.room.meshes.size(),
            frustumCull.receipt.inputRoomMeshCount,
            frustumCull.receipt.keptRoomMeshCount,
            frustumCull.receipt.culledRoomMeshCount,
            frustumCull.receipt.conservativelyKeptMeshCount, selectedId,
            hasSelection ? 1 : 0,
            overlayFrame.documentWireLineCount,
            overlayFrame.pointMarkerEdgeCount,
            overlayFrame.lineMarkerEdgeCount,
            overlayFrame.pathPointHandleEdgeCount,
            overlayFrame.combinedWireLines.size() -
                overlayFrame.documentWireLineCount -
                overlayFrame.pointMarkerEdgeCount -
                overlayFrame.lineMarkerEdgeCount -
                overlayFrame.pathPointHandleEdgeCount,
            overlayFrame.combinedWireLines.size(),
            overlayFrame.uiRects.size(), overlayFrame.glyphs.size());
  }

  if (request.maxFrames != 0U && editor.frameIndex >= request.maxFrames) {
    logStandaloneRoomBakeFinal(roomBakePreview);
    // Name the SELECTED object + kind so the capture is self-documenting; the
    // capture proof expects this target to be the FLOOR.
    const char* selKind =
        hasSelection
            ? creative::toString(selected->kind).data()
            : "<none>";
    SDL_Log("iggy3d_creative: FINAL frame %llu submit outcome=%d reason='%s' "
            "selectedTarget=%u selectedKind='%s' hasSelection=%d selBoxLines=%zu "
            "pointMarkerLines=%zu lineMarkerLines=%zu pathHandleLines=%zu "
            "gizmoLines=%zu combinedWireLines=%zu placeMode=%d brush='%s' "
            "ghostEdges=%zu placed=%llu objectCount=%llu "
            "frustumInputMeshes=%zu frustumKeptMeshes=%zu "
            "frustumCulledMeshes=%zu frustumConservativeMeshes=%zu",
            static_cast<unsigned long long>(editor.frameIndex),
            static_cast<int>(submit.outcome),
            std::string(submit.reason.code).c_str(), selectedId, selKind,
            hasSelection ? 1 : 0, overlayFrame.documentWireLineCount,
            overlayFrame.pointMarkerEdgeCount,
            overlayFrame.lineMarkerEdgeCount,
            overlayFrame.pathPointHandleEdgeCount,
            overlayFrame.combinedWireLines.size() -
                overlayFrame.documentWireLineCount -
                overlayFrame.pointMarkerEdgeCount -
                overlayFrame.lineMarkerEdgeCount -
                overlayFrame.pathPointHandleEdgeCount,
            overlayFrame.combinedWireLines.size(), editor.placeMode ? 1 : 0,
            std::string(creative::toString(editor.placeBrush)).c_str(),
            overlayFrame.ghostEdgeCount,
            static_cast<unsigned long long>(editor.placedCount),
            static_cast<unsigned long long>(
                appState.facade.document().objectCount()),
            frustumCull.receipt.inputRoomMeshCount,
            frustumCull.receipt.keptRoomMeshCount,
            frustumCull.receipt.culledRoomMeshCount,
            frustumCull.receipt.conservativelyKeptMeshCount);
    return true;
  }
  return false;
}

}  // namespace iggy3d_creative_app
