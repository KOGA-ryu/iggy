#pragma once

#include <string_view>

#include "EditorDesktopCommands.hpp"
#include "EditorState.hpp"

namespace iggy3d_creative_app {

void drawCreativeEditorWorldLayoutElevationCanvas(
    CreativeEditorState& editor,
    const iggy3d::creative::CreativeGridSettings& grid,
    const iggy3d::creative::CreativeMeasurementAnnotationStore&
        measurementAnnotations,
    const iggy3d::creative::CreativeMeasurementState& measurement,
    CreativeDesktopCommandFrame& commands,
    bool interactionEnabled);

}  // namespace iggy3d_creative_app
