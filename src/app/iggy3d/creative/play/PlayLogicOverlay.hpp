#pragma once

#include <cstddef>
#include <vector>

#include "app/iggy3d/creative/play/RuntimeSandbox.hpp"
#include "render/FrameInput.hpp"

namespace iggy3d_creative_app {

struct CreativePlayLogicOverlay {
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> lines;
  std::size_t sourceEdgeCount = 0U;
  std::size_t linkShaftCount = 0U;
  std::size_t linkArrowEdgeCount = 0U;
  std::size_t targetEdgeCount = 0U;
  std::size_t invalidLinkCount = 0U;
};

[[nodiscard]] CreativePlayLogicOverlay buildCreativePlayLogicOverlay(
    const iggy3d::creative::CreativeRuntimeSandbox& sandbox,
    iggy3d::creative::CreativeObjectId highlightedLogicSourceObjectId);

}  // namespace iggy3d_creative_app
