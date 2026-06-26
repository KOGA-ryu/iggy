#pragma once

#include <string_view>

struct SDL_Renderer;

#include "app/iggy3d/ProductAppOptions.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/platform/SdlWindow.hpp"
#include "render/RendererApi.hpp"

namespace iggy3d {

struct ProductWindowRendererRequest {
  ProductRendererRequest rendererRequest = ProductRendererRequest::Null;
  const SdlWindowCreateInfo* createInfo = nullptr;
  SdlWindow* sdlWindow = nullptr;
  ProductAppWindowState* window = nullptr;
};

struct ProductWindowRendererState {
  bool useVulkanRenderer = false;
  bool ready = false;
  SDL_Renderer* sdlRenderer = nullptr;
  RendererApi vulkanRenderer;
};

bool productWindowRendererUsesVulkan(ProductRendererRequest rendererRequest);

void recordProductVulkanRendererUnavailable(ProductAppWindowState& window,
                                            std::string_view reasonCode);
void recordProductVulkanRendererReady(ProductAppWindowState& window,
                                      const RendererApi& renderer);
void recordProductVulkanSubmit(ProductAppWindowState& window,
                               const RenderSubmitResult& submit);

ProductWindowRendererState createProductWindowRenderer(
    const ProductWindowRendererRequest& request);
void shutdownProductWindowRenderer(ProductWindowRendererState& renderer);
void finalizeProductWindowRendererStatus(const ProductWindowRendererState& renderer,
                                         ProductAppWindowState& window);

}  // namespace iggy3d
