#include "CreativeEditorFrameInput.hpp"

#include <algorithm>
#include <chrono>
#include <thread>

#include <SDL3/SDL.h>

#include "app/iggy3d/creative/camera/Fly.hpp"
#include "render/FrameInput.hpp"
#include "render/vulkan/VulkanBackend.hpp"

namespace iggy3d_creative_app {
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

}  // namespace iggy3d_creative_app
