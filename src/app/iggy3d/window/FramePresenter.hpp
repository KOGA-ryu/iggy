#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/frontend/WorldSetupModel.hpp"
#include "app/iggy3d/world/DefaultWorldTemplate.hpp"
#include "app/iggy3d/Options.hpp"
#include "app/iggy3d/gameplay/MovementTuning.hpp"
#include "app/iggy3d/gameplay/ProjectionRefresh.hpp"
#include "app/iggy3d/menu/DrawList.hpp"
#include "app/iggy3d/window/RendererLifecycle.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"
#include "app/platform/SdlWindow.hpp"
#include "render/FrameInput.hpp"

namespace iggy3d {

struct ProductCreativeWireframeDebugLineList;
namespace creative {
struct CreativeAppState;
}  // namespace creative

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

struct ProductVulkanGameplayFrame {
  FrameInput frame;
  std::vector<RenderUiRect> rects;
  std::vector<DebugHudGlyphQuad> textGlyphQuads;
  std::vector<RenderCreativeWireframeDebugLine> creativeWireframeDebugLines;
  std::uint64_t textGlyphCount = 0;
};

struct ProductCreativeWireframeDebugRenderFrame {
  std::vector<RenderCreativeWireframeDebugLine> lines;
  RenderCreativeWireframeDebugFrame frame;
};

struct ProductWindowFramePresenterRequest {
  const ProductAppOptions& options;
  const ProductWorldTemplate& world;
  const FrontendState& frontend;
  FrontendSettingsTab settingsTab = FrontendSettingsTab::None;
  const WorldSetupDraft& worldSetupDraft;
  ProductAppWindowState& window;
  const ProductSaveBridgeResult& saves;
  SdlWindow& sdlWindow;
  ProductWindowRendererState& renderer;
  const ProductGameplayProjectionFrame& projectionFrame;
  const ProductUiDrawList* creativeUiDrawList = nullptr;
  const ProductCreativeWireframeDebugLineList* creativeWireframeDebugLineList =
      nullptr;
  const creative::CreativeAppState* creativeApp = nullptr;
};

ProductVulkanMenuFrame buildProductVulkanStarterMenuFrame(
    const ProductVulkanMenuFrameRequest& request);
const FrameInput& refreshProductVulkanMenuFrameInput(
    ProductVulkanMenuFrame& menuFrame);
ProductVulkanGameplayFrame buildProductVulkanGameplayFrame(
    const ProductGameplayProjectionFrame& projectionFrame,
    std::uint64_t frameIndex,
    std::uint32_t viewportWidth,
    std::uint32_t viewportHeight,
    float cameraYawDegrees,
    float cameraPitchDegrees,
    const ProductGameplayMovementTuning& movementTuning =
        productGameplayMovementTuning(),
    ProductGameplayMovementTuningField movementTuningField =
        ProductGameplayMovementTuningField::WalkSpeed,
    bool movementTuningVisible = false,
    bool devToolsOverlayVisible = false,
    FrontendDevToolsCategory devToolsCategory = FrontendDevToolsCategory::Session,
    bool creativeEditorOverlayActive = false);
const FrameInput& refreshProductVulkanGameplayFrameInput(
    ProductVulkanGameplayFrame& gameplayFrame);
[[nodiscard]] ProductCreativeWireframeDebugRenderFrame
buildProductCreativeWireframeDebugRenderFrame(
    const ProductCreativeWireframeDebugLineList* lineList);

void appendProductUiOverlay(ProductVulkanGameplayFrame& gameplayFrame,
                            const ProductUiDrawList& overlayUi,
                            std::uint64_t frameIndex,
                            std::uint32_t drawableWidth,
                            std::uint32_t drawableHeight);
void appendCreativeUiOverlay(ProductVulkanGameplayFrame& gameplayFrame,
                             const ProductUiDrawList& overlayUi,
                             std::uint64_t frameIndex,
                             std::uint32_t drawableWidth,
                             std::uint32_t drawableHeight);

// Overlay an in-game menu draw list (e.g. the Journal-themed pause menu) onto a
// gameplay frame's ui overlay, so it composites over the frozen 3D scene in the
// same single submit. Reuses the menu-frame converter, so the draw list's theme
// resolves at the render seam. No-op if the draw list is not ready.
void appendPauseMenuOverlay(ProductVulkanGameplayFrame& gameplayFrame,
                            const ProductUiDrawList& overlayUi,
                            std::uint64_t frameIndex,
                            std::uint32_t drawableWidth,
                            std::uint32_t drawableHeight);
void presentProductWindowFrame(ProductWindowFramePresenterRequest request);

}  // namespace iggy3d
