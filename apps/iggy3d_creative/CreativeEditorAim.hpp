#pragma once

#include "app/iggy3d/creative/tools/Tools.hpp"
#include "core/math/Vec3.hpp"
#include "render/FrameInput.hpp"

namespace iggy3d_creative_app {

[[nodiscard]] iggy3d::creative::CreativeToolWorldPoint
resolveCreativeEditorGroundPoint(const iggy3d::RenderCameraFrame& camera);

[[nodiscard]] iggy3d::Vec3 resolveCreativeEditorAimCell(
    const iggy3d::RenderCameraFrame& camera,
    double placeCellSize);

}  // namespace iggy3d_creative_app
