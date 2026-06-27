#pragma once

#include <string>
#include <string_view>

struct SDL_Renderer;

#include "app/iggy3d/Options.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/menu/DrawList.hpp"
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

struct ProductVulkanGameplayReadiness {
  bool backendBuilt = false;
  bool ready = false;
  std::string status = "product_vulkan_not_requested";
  std::string reasonCode = "product_vulkan_not_requested";
};

bool productWindowRendererUsesVulkan(ProductRendererRequest rendererRequest);
bool productWindowVulkanBackendBuilt();
ProductVulkanGameplayReadiness evaluateProductVulkanGameplayReadiness(
    const ProductAppWindowState& window);

void recordProductVulkanRendererUnavailable(ProductAppWindowState& window,
                                            std::string_view reasonCode);
void recordProductVulkanRendererReady(ProductAppWindowState& window,
                                      const RendererApi& renderer);
void recordProductVulkanSubmit(ProductAppWindowState& window,
                               const RenderSubmitResult& submit);
void recordProductVulkanMenuUnsupported(ProductAppWindowState& window,
                                        std::string_view menuSurface);
void recordProductVulkanMenuUiDrawList(ProductAppWindowState& window,
                                       std::string_view menuSurface,
                                       const ProductUiDrawList& uiDrawList);

ProductWindowRendererState createProductWindowRenderer(
    const ProductWindowRendererRequest& request);
void shutdownProductWindowRenderer(ProductWindowRendererState& renderer);
void finalizeProductWindowRendererStatus(const ProductWindowRendererState& renderer,
                                         ProductAppWindowState& window);

}  // namespace iggy3d
