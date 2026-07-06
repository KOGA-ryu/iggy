#pragma once

namespace iggy3d {

// Owned Vulkan-renderer lifecycle-flag state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: window/vulkan. Behavior-identical.
struct ProductVulkanRendererState {
  bool requested = false;
  bool created = false;
  bool ready = false;
};

}  // namespace iggy3d
