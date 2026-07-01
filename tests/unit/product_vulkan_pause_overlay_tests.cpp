#include "app/iggy3d/window/FramePresenter.hpp"

#include <cstddef>
#include <iostream>
#include <string_view>

#include "app/iggy3d/menu/PauseUi.hpp"

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    return false;
  }
  return true;
}

iggy3d::PauseMenuModel samplePauseModel() {
  iggy3d::PauseMenuContext context;
  context.pauseOpen = true;
  context.runtimeSessionAvailable = true;
  context.saveRootWritable = true;
  context.compatibleSaveCount = 1U;
  context.developerToolsEnabled = true;
  return iggy3d::buildPauseMenuModel(context, iggy3d::FrontendAction::Resume);
}

// The pause menu wears the Journal theme and overlays onto a gameplay frame's ui
// overlay as rects + glyphs — this is what puts the Moleskine menu over the frozen
// scene in the single Vulkan submit.
bool pauseOverlayAddsJournalRectsAndGlyphsToGameplayFrame() {
  const iggy3d::PauseMenuModel model = samplePauseModel();
  iggy3d::ProductPauseUiRequest request;
  request.model = &model;
  const iggy3d::ProductUiDrawList pauseUi =
      iggy3d::buildProductPauseUiDrawList(request);

  iggy3d::ProductVulkanGameplayFrame frame;  // as if the HUD overlay were empty
  iggy3d::appendPauseMenuOverlay(frame, pauseUi, 1U, 1280U, 720U);

  bool ok = true;
  ok &= expect(pauseUi.ready, "pause draw list ready");
  ok &= expect(pauseUi.theme == iggy3d::ProductUiThemeId::Journal,
               "pause overlay carries the journal theme");
  ok &= expect(!frame.rects.empty(),
               "overlay contributes rects (page + selected highlight)");
  ok &= expect(!frame.textGlyphQuads.empty(),
               "overlay contributes text glyphs (title + rows)");
  ok &= expect(frame.textGlyphCount > 0U, "overlay glyph count tracked");
  return ok;
}

bool pauseOverlayPreservesExistingHudOverlay() {
  const iggy3d::PauseMenuModel model = samplePauseModel();
  iggy3d::ProductPauseUiRequest request;
  request.model = &model;
  const iggy3d::ProductUiDrawList pauseUi =
      iggy3d::buildProductPauseUiDrawList(request);

  // Pretend a HUD already put one rect + one glyph on the frame; the overlay must
  // ADD to it, not replace it (menu-over-scene compositing in one frame).
  iggy3d::ProductVulkanGameplayFrame frame;
  frame.rects.push_back(iggy3d::RenderUiRect{});
  frame.textGlyphQuads.push_back(iggy3d::DebugHudGlyphQuad{});
  frame.textGlyphCount = 1U;
  const std::size_t rectsBefore = frame.rects.size();
  const std::size_t glyphsBefore = frame.textGlyphQuads.size();

  iggy3d::appendPauseMenuOverlay(frame, pauseUi, 1U, 1280U, 720U);

  bool ok = true;
  ok &= expect(frame.rects.size() > rectsBefore, "hud rect preserved, overlay added");
  ok &= expect(frame.textGlyphQuads.size() > glyphsBefore,
               "hud glyph preserved, overlay added");
  ok &= expect(frame.textGlyphCount > 1U, "glyph count accumulates over the hud");
  return ok;
}

bool pauseOverlayNoopsOnUnreadyDrawList() {
  iggy3d::ProductUiDrawList notReady;  // ready == false by default
  iggy3d::ProductVulkanGameplayFrame frame;
  iggy3d::appendPauseMenuOverlay(frame, notReady, 1U, 1280U, 720U);
  bool ok = true;
  ok &= expect(frame.rects.empty() && frame.textGlyphQuads.empty(),
               "an unready overlay draw list adds nothing");
  return ok;
}

}  // namespace

int main() {
  bool ok = true;
  ok &= pauseOverlayAddsJournalRectsAndGlyphsToGameplayFrame();
  ok &= pauseOverlayPreservesExistingHudOverlay();
  ok &= pauseOverlayNoopsOnUnreadyDrawList();
  if (!ok) {
    return 1;
  }
  std::cout << "product_vulkan_pause_overlay_tests=pass\n";
  return 0;
}
