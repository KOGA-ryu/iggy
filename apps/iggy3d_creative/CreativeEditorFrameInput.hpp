#pragma once

#include "app/platform/SdlWindow.hpp"

#include "CreativeEditorState.hpp"

namespace iggy3d {

class VulkanBackend;

}  // namespace iggy3d

namespace iggy3d_creative_app {

struct CreativeEditorFrameInputResult {
  bool keepRunning = true;
  bool skipFrame = false;
  iggy3d::SdlDrawableExtent extent{};
  const bool* keyboardState = nullptr;
};

CreativeEditorFrameInputResult beginCreativeEditorFrameInput(
    iggy3d::SdlWindow& window,
    iggy3d::VulkanBackend& backend,
    CreativeEditorState& editor);

}  // namespace iggy3d_creative_app
