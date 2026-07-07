#pragma once

#include <cstdint>
#include <vector>

#include "render/FrameInput.hpp"
#include "render/vulkan/CommandRecording.hpp"

namespace iggy3d::vulkan {

struct ProjectileOverlayLayout {
  std::vector<OverlayRect> rects;
  std::uint32_t projectileCount = 0;
  std::uint32_t markerCount = 0;
  std::uint32_t trailRectCount = 0;
  bool projected = false;
  bool impactVisible = false;
};

[[nodiscard]] ProjectileOverlayLayout projectileOverlayLayoutFor(
    const FrameInput& frame);

}  // namespace iggy3d::vulkan
