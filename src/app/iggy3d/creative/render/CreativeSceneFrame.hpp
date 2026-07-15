#pragma once

#include <cstdint>

#include "core/math/Vec3.hpp"
#include "render/FrameInput.hpp"

namespace iggy3d {

struct DebugProjectionResult;
struct SceneProjectionResult;

FrameInput makeCreativeVulkanFrame(
    const SceneProjectionResult& scene,
    const DebugProjectionResult& debug,
    std::uint64_t frameIndex,
    std::uint32_t viewportWidth,
    std::uint32_t viewportHeight,
    float cameraYawDegrees,
    float cameraPitchDegrees,
    bool cameraAnchorOverrideAvailable = false,
    Vec3 cameraAnchorOverrideMeters = {},
    // Sub-rectangle the 3D scene occupies (drawable px). The all-zero sentinel
    // means full-frame, so viewport stays swapchain-truth and the camera aspect
    // is unchanged; an explicit rect drives clipFromView by its own aspect.
    RenderContentViewport contentViewport = {});

}  // namespace iggy3d
