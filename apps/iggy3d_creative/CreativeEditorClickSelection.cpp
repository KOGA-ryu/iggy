#include "CreativeEditorClickSelection.hpp"

#include <SDL3/SDL.h>

#include "app/iggy3d/creative/tools/Tools.hpp"

#include "StandalonePicking.hpp"

namespace iggy3d_creative_app {

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

}  // namespace iggy3d_creative_app
