#pragma once

#include <span>
#include <vector>

#include "app/iggy3d/creative/tools/Volume.hpp"
#include "render/FrameInput.hpp"

namespace iggy3d_creative_app {

void appendCreativeMaterialBrushCellOutlines(
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines,
    std::span<const iggy3d::creative::CreativeGridCoord3> cells,
    const iggy3d::creative::CreativeGridSettings& grid,
    iggy3d::RenderLineColor color,
    float thickness);

void appendCreativeMaterialBrushStampOutline(
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines,
    const iggy3d::creative::CreativeMaterialBrushStampPlan& plan,
    const iggy3d::creative::CreativeGridSettings& grid,
    iggy3d::RenderLineColor color,
    float thickness);

void appendCreativeShapeBrushOutline(
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines,
    const iggy3d::creative::CreativeVolumeSelection& selection,
    iggy3d::creative::CreativeShapeBrushKind kind,
    iggy3d::creative::CreativeShapeBrushAxis axis,
    iggy3d::RenderLineColor color,
    float thickness);

}  // namespace iggy3d_creative_app
