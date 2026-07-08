#pragma once

#include <vector>

#include "core/math/Vec3.hpp"
#include "render/FrameInput.hpp"

namespace iggy3d_creative_app {

void appendStandaloneWireframeBoxEdges(
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& out,
    iggy3d::Vec3 boxMin,
    iggy3d::Vec3 boxMax,
    iggy3d::RenderLineColor color,
    float thickness);

}  // namespace iggy3d_creative_app
