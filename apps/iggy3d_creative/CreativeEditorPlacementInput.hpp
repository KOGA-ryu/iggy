#pragma once

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/platform/SdlWindow.hpp"
#include "core/math/Vec3.hpp"

#include "CreativeEditorState.hpp"

namespace iggy3d_creative_app {

void applyCreativeEditorPlacementInput(
    iggy3d::SdlWindow& window,
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    iggy3d::Vec3 aimCellCenter,
    bool captureMode);

}  // namespace iggy3d_creative_app
