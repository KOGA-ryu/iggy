#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "app/iggy3d/menu/ProductUiDrawList.hpp"
#include "render/FrameInput.hpp"

namespace iggy3d {

struct ProductVulkanMenuFrameRequest {
  const ProductUiDrawList* uiDrawList = nullptr;
  std::uint64_t frameIndex = 0;
  std::uint32_t drawableWidth = 0;
  std::uint32_t drawableHeight = 0;
};

struct ProductVulkanMenuFrame {
  bool ready = false;
  std::string status = "product_vulkan_menu_frame_not_ready";
  std::string reasonCode = "product_vulkan_menu_frame_not_ready";
  FrameInput frame;
  std::vector<RenderUiRect> rects;
  std::vector<DebugHudGlyphQuad> textGlyphQuads;
  std::uint64_t textGlyphCount = 0;
};

ProductVulkanMenuFrame buildProductVulkanStarterMenuFrame(
    const ProductVulkanMenuFrameRequest& request);
const FrameInput& refreshProductVulkanMenuFrameInput(ProductVulkanMenuFrame& menuFrame);

}  // namespace iggy3d
