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
#include "StandaloneDelete.hpp"
#include "StandalonePersistenceProof.hpp"
#include "StandalonePicking.hpp"
#include "EditorPreviewProxies.hpp"
#include "StandaloneUndo.hpp"

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
      if ((buttons & SDL_BUTTON_LMASK) != 0U) {
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
      }
    } else {
      window.setRelativeMouseMode(true);
    }
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
    if (pickedId != iggy3d::creative::kInvalidObjectId) {
      packet.pointer.target =
          iggy3d::creative::TargetRef{
              static_cast<iggy3d::creative::Id>(pickedId)};
    }
    (void)appState.facade.dispatchToolInput(packet);
  }
}

namespace creative = iggy3d::creative;

void applyCreativeEditorCommandInput(
    const bool* keys,
    bool captureMode,
    creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    const std::filesystem::path& saveRoot,
    const std::string& saveId) {
  if (captureMode || keys == nullptr) {
    return;
  }

  const bool key1 = keys[SDL_SCANCODE_1] != 0;
  const bool key2 = keys[SDL_SCANCODE_2] != 0;
  const bool key3 = keys[SDL_SCANCODE_3] != 0;
  const bool keyB = keys[SDL_SCANCODE_B] != 0;
  const bool keyDelete = keys[SDL_SCANCODE_DELETE] != 0;
  const bool keyBackspace = keys[SDL_SCANCODE_BACKSPACE] != 0;
  const bool keyZ = keys[SDL_SCANCODE_Z] != 0;
  const SDL_Keymod modState = SDL_GetModState();
  const bool undoModifier =
      (modState & (SDL_KMOD_GUI | SDL_KMOD_CTRL)) != 0U;
  if (key1 && !editor.prevKey1) {
    editor.placeMode = false;  // '1' Select leaves Place mode.
    const bool ok = appState.facade.setActiveTool(creative::Tool::Select);
    SDL_Log("iggy3d_creative: setActiveTool(Select) accepted=%d placeMode=0",
            ok ? 1 : 0);
  }
  if (key2 && !editor.prevKey2) {
    editor.placeMode = false;  // '2' Move leaves Place mode.
    const bool ok = appState.facade.setActiveTool(creative::Tool::Move);
    SDL_Log("iggy3d_creative: setActiveTool(Move) accepted=%d placeMode=0",
            ok ? 1 : 0);
  }
  if (key3 && !editor.prevKey3) {
    editor.placeMode = true;  // '3' Place: app-level mode, not a kernel Tool.
    SDL_Log("iggy3d_creative: placeMode=1 brush='%s'",
            std::string(creative::toString(editor.placeBrush)).c_str());
  }
  if (keyB && !editor.prevKeyB) {
    editor.placeBrush = nextBrushKind(editor.brushPalette, editor.placeBrush);
    SDL_Log("iggy3d_creative: brush cycled -> '%s'",
            std::string(creative::toString(editor.placeBrush)).c_str());
  }
  if ((keyDelete && !editor.prevKeyDelete) ||
      (keyBackspace && !editor.prevKeyBackspace)) {
    (void)deleteSelectedObject(appState,
                               keyDelete ? "delete_key" : "backspace_key",
                               &editor.undoStack);
  }
  if (keyZ && !editor.prevKeyZ && undoModifier) {
    (void)undoLastSnapshot(appState, editor.undoStack, "keyboard_undo");
  }
  const bool keyF5 = keys[SDL_SCANCODE_F5] != 0;
  const bool keyF6 = keys[SDL_SCANCODE_F6] != 0;
  const bool keyF9 = keys[SDL_SCANCODE_F9] != 0;
  if (keyF5 && !editor.prevKeyF5) {
    const iggy3d::CreativeWorldSaveResult saveResult =
        saveStandaloneScene(appState.facade, saveRoot, saveId);
    if (saveResult.accepted && saveResult.saved) {
      clearUndoStack(editor.undoStack, "save_success");
    }
  }
  if (keyF6 && !editor.prevKeyF6) {
    clearToBlankScene(appState);
    clearUndoStack(editor.undoStack, "new_clear");
  }
  if (keyF9 && !editor.prevKeyF9) {
    const bool loaded = loadStandaloneScene(appState, saveRoot, saveId);
    if (loaded) {
      clearUndoStack(editor.undoStack, "load_success");
    }
  }
  editor.prevKey1 = key1;
  editor.prevKey2 = key2;
  editor.prevKey3 = key3;
  editor.prevKeyB = keyB;
  editor.prevKeyDelete = keyDelete;
  editor.prevKeyBackspace = keyBackspace;
  editor.prevKeyZ = keyZ;
  editor.prevKeyF5 = keyF5;
  editor.prevKeyF6 = keyF6;
  editor.prevKeyF9 = keyF9;
}

namespace {

constexpr float kMouseSensitivity = 0.12F;

}  // namespace

CreativeEditorFrameInputResult beginCreativeEditorFrameInput(
    iggy3d::SdlWindow& window,
    iggy3d::VulkanBackend& backend,
    CreativeEditorState& editor) {
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
  result.keyboardState = keys;
  iggy3d::ProductCreativeFlyInput flyInput;
  if (keys != nullptr) {
    const float moveX = (keys[SDL_SCANCODE_D] ? 1.0F : 0.0F) -
                        (keys[SDL_SCANCODE_A] ? 1.0F : 0.0F);
    const float moveY = (keys[SDL_SCANCODE_W] ? 1.0F : 0.0F) -
                        (keys[SDL_SCANCODE_S] ? 1.0F : 0.0F);
    const float moveZ = (keys[SDL_SCANCODE_SPACE] ? 1.0F : 0.0F) -
                        (keys[SDL_SCANCODE_LCTRL] ? 1.0F : 0.0F);
    flyInput.moveX = moveX;
    flyInput.moveY = moveY;
    flyInput.moveZ = moveZ;
    flyInput.sprinting = keys[SDL_SCANCODE_LSHIFT];
  }

  float mouseDx = 0.0F;
  float mouseDy = 0.0F;
  SDL_GetRelativeMouseState(&mouseDx, &mouseDy);
  editor.yawDegrees += mouseDx * kMouseSensitivity;
  editor.pitchDegrees = std::clamp(
      editor.pitchDegrees - mouseDy * kMouseSensitivity, -80.0F, 80.0F);
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
  selection.selectedId = facade.selectionState().selectedTarget.value;
  selection.selected =
      selection.selectedId != 0
          ? facade.findObject(static_cast<iggy3d::creative::CreativeObjectId>(
                selection.selectedId))
          : nullptr;
  selection.hasSelection =
      selection.selected != nullptr && selection.selected->visible;
  if (selection.hasSelection) {
    const VisualBounds selectedVisualBounds =
        visualBoundsForObject(*selection.selected);
    selection.boxMin = selectedVisualBounds.min;
    selection.boxMax = selectedVisualBounds.max;
  }
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

}  // namespace iggy3d_creative_app
