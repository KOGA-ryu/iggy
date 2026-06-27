#include "app/iggy3d/window/FramePresenter.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::FrontendState starterFrontend(iggy3d::FrontendAction selected) {
  iggy3d::FrontendState frontend;
  frontend.screen = iggy3d::FrontendScreen::Starter;
  frontend.childScreen = iggy3d::FrontendScreen::Gameplay;
  frontend.selectedAction = selected;
  frontend.status = "starter_screen_ready";
  return frontend;
}

bool starterDrawListBuildsUiFrame() {
  const iggy3d::FrontendState frontend =
      starterFrontend(iggy3d::FrontendAction::NewWorld);
  const iggy3d::ProductUiDrawList ui =
      iggy3d::buildProductStarterUiDrawList({&frontend, 0U, 1280U, 720U});
  iggy3d::ProductVulkanMenuFrame frame =
      iggy3d::buildProductVulkanStarterMenuFrame({&ui, 7U, 1920U, 1080U});
  const iggy3d::FrameInput& input =
      iggy3d::refreshProductVulkanMenuFrameInput(frame);
  return expect(frame.ready, "menu frame ready") &&
         expect(frame.status == "product_vulkan_menu_frame_ready",
                "menu frame status") &&
         expect(input.viewport.width == 1920U, "viewport width") &&
         expect(input.viewport.height == 1080U, "viewport height") &&
         expect(input.clock.frameIndex == 7U, "frame index") &&
         expect(input.ui.visible, "ui visible") &&
         expect(input.ui.rectCount == ui.rectCount, "rect count copied") &&
         expect(input.ui.primitiveCount == ui.primitiveCount,
                "primitive count copied") &&
         expect(input.ui.rects == frame.rects.data(), "rect pointer refreshed") &&
         expect(input.ui.textGlyphQuads == frame.textGlyphQuads.data(),
                "text pointer refreshed") &&
         expect(input.ui.textGlyphCount > 0U, "text glyph count") &&
         expect(input.ui.textGlyphQuadCount == frame.textGlyphQuads.size(),
                "text quad count") &&
         expect(iggy3d::validateFrameInput(input) == iggy3d::FrameInputStatus::Valid,
                "ui frame validates");
}

bool invalidRequestsFailClosed() {
  const iggy3d::ProductVulkanMenuFrame missing =
      iggy3d::buildProductVulkanStarterMenuFrame({nullptr, 1U, 1280U, 720U});
  const iggy3d::FrontendState frontend =
      starterFrontend(iggy3d::FrontendAction::NewWorld);
  const iggy3d::ProductUiDrawList ui =
      iggy3d::buildProductStarterUiDrawList({&frontend, 0U, 1280U, 720U});
  const iggy3d::ProductVulkanMenuFrame notDrawable =
      iggy3d::buildProductVulkanStarterMenuFrame({&ui, 1U, 0U, 720U});
  return expect(!missing.ready, "missing not ready") &&
         expect(missing.reasonCode == "product_vulkan_menu_frame_missing_ui",
                "missing reason") &&
         expect(!notDrawable.ready, "not drawable not ready") &&
         expect(notDrawable.reasonCode == "product_vulkan_menu_frame_not_drawable",
                "not drawable reason");
}

}  // namespace

int main() {
  const bool ok = starterDrawListBuildsUiFrame() && invalidRequestsFailClosed();
  if (!ok) {
    return 1;
  }
  std::cout << "product_vulkan_menu_frame_tests=pass\n";
  return 0;
}
