#pragma once

#include "EditorDesktopCommands.hpp"
#include "EditorState.hpp"

namespace iggy3d_creative_app {

void drawCreativeEditorWorldLayoutElevationCanvas(
    CreativeEditorState& editor,
    const iggy3d::creative::CreativeGridSettings& grid,
    CreativeDesktopCommandFrame& commands,
    bool interactionEnabled);

}  // namespace iggy3d_creative_app
