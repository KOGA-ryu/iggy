#pragma once

#include <cstdint>

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/platform/SdlWindow.hpp"
#include "render/FrameInput.hpp"

#include "CreativeEditorPickFrame.hpp"
#include "CreativeEditorState.hpp"

namespace iggy3d_creative_app {

void applyCreativeEditorClickSelection(
    iggy3d::SdlWindow& window,
    iggy3d::creative::CreativeAppState& appState,
    const iggy3d::RenderCameraFrame& camera,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    const CreativeEditorPickFrame& pickFrame,
    CreativeEditorState& editor,
    bool captureMode);

}  // namespace iggy3d_creative_app
