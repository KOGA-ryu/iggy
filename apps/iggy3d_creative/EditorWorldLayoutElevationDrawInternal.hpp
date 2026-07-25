#pragma once

#include "imgui.h"
#include "EditorMeasurement.hpp"
#include "EditorWorldLayoutElevationPlanner.hpp"

namespace iggy3d_creative_app {

void drawCreativeEditorWorldLayoutElevationSectionControls(
    CreativeEditorWorldLayoutState& state, bool interactionEnabled);

void drawCreativeEditorWorldLayoutElevationProjection(
    ImDrawList& drawList, ImVec2 minimum, ImVec2 maximum,
    const CreativeEditorWorldLayoutElevationCanvasTransform& transform,
    const CreativeEditorWorldLayoutElevationProjection& projection,
    const CreativeEditorWorldLayoutState& state,
    const iggy3d::creative::CreativeGridSettings& grid,
    const iggy3d::creative::CreativeMeasurementAnnotationStore&
        measurementAnnotations,
    const iggy3d::creative::CreativeMeasurementState& measurement);

}  // namespace iggy3d_creative_app
