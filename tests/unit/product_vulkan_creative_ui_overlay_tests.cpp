#include "app/iggy3d/window/FramePresenter.hpp"

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/bridge/UiWindowFrame.hpp"

#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    return false;
  }
  return true;
}

void markCreativeAppIdentity(cr::CreativeAppState& app) {
  app.identity.saveId = "creative_save";
  app.identity.worldId = "world_001";
  app.identity.documentId = 42U;
}

void markCreativeDocumentWindow(iggy3d::ProductAppWindowState& window) {
  window.interactionMode = iggy3d::ProductInteractionMode::Creative;
  window.activeCreative.saveId = "creative_save";
  window.activeCreative.worldId = "world_001";
  window.activeCreative.documentId = 42U;
}

iggy3d::ProductCreativeUiFrame readyCreativeUiFrame() {
  iggy3d::ProductAppWindowState window;
  markCreativeDocumentWindow(window);
  cr::CreativeAppState app;
  markCreativeAppIdentity(app);
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  return iggy3d::buildProductCreativeUiWindowFrame(
      iggy3d::ProductCreativeUiWindowFrameRequest{
          &window, &app, 1280, 720, 1280, 720,
          iggy3d::ProductUiThemeId::System});
}

bool legacyCreativeWindowDoesNotBuildDocumentOverlay() {
  iggy3d::ProductAppWindowState window;
  window.interactionMode = iggy3d::ProductInteractionMode::Creative;
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();

  const iggy3d::ProductCreativeUiFrame frame =
      iggy3d::buildProductCreativeUiWindowFrame(
          iggy3d::ProductCreativeUiWindowFrameRequest{
              &window, &app, 1280, 720, 1280, 720,
              iggy3d::ProductUiThemeId::System});

  return expect(!frame.projection.drawList.ready,
                "legacy creative draw list inactive") &&
         expect(frame.receipt.status == "product_creative_ui_frame_inactive",
                "legacy creative frame inactive status");
}

iggy3d::ProductGameplayProjectionFrame legacyGameplayHudProjectionFrame() {
  iggy3d::ProductGameplayProjectionFrame frame;
  frame.topDownMapOverlay.visible = true;
  frame.topDownMapOverlay.purpose = "editor_overview";
  frame.topDownMapOverlay.size = "full";
  frame.topDownMapOverlay.itemCount = 14U;
  frame.interactionModeHud.visible = true;
  frame.interactionModeHud.label = "CREATIVE";
  frame.feedback.visible = true;
  frame.feedback.lines.push_back(
      iggy3d::GameplayFeedbackLine{"TARGET", "NOT_ATTEMPTED",
                                   iggy3d::FeedbackTone::Neutral, true});
  return frame;
}

bool readyCreativeOverlayAddsRectsAndGlyphs() {
  const iggy3d::ProductCreativeUiFrame creativeFrame = readyCreativeUiFrame();
  const iggy3d::ProductUiDrawList& drawList =
      creativeFrame.projection.drawList;
  iggy3d::ProductVulkanGameplayFrame gameplayFrame;

  iggy3d::appendCreativeUiOverlay(gameplayFrame, drawList, 1U, 1280U, 720U);

  return expect(drawList.ready, "creative draw list ready") &&
         expect(!gameplayFrame.rects.empty(), "creative overlay adds rects") &&
         expect(!gameplayFrame.textGlyphQuads.empty(),
                "creative overlay adds glyph quads") &&
         expect(gameplayFrame.textGlyphCount > 0U,
                "creative overlay tracks glyph count");
}

bool creativeOverlayAppendsToExistingHudOverlay() {
  const iggy3d::ProductCreativeUiFrame creativeFrame = readyCreativeUiFrame();
  const iggy3d::ProductUiDrawList& drawList =
      creativeFrame.projection.drawList;
  iggy3d::ProductVulkanGameplayFrame gameplayFrame;
  gameplayFrame.rects.push_back(iggy3d::RenderUiRect{});
  gameplayFrame.textGlyphQuads.push_back(iggy3d::DebugHudGlyphQuad{});
  gameplayFrame.textGlyphCount = 1U;
  const std::size_t rectsBefore = gameplayFrame.rects.size();
  const std::size_t glyphQuadsBefore = gameplayFrame.textGlyphQuads.size();

  iggy3d::appendCreativeUiOverlay(gameplayFrame, drawList, 2U, 1280U, 720U);

  return expect(gameplayFrame.rects.size() > rectsBefore,
                "existing rect preserved and creative rects appended") &&
         expect(gameplayFrame.textGlyphQuads.size() > glyphQuadsBefore,
                "existing glyph preserved and creative glyphs appended") &&
         expect(gameplayFrame.textGlyphCount > 1U,
                "creative glyph count accumulates");
}

bool unreadyCreativeOverlayNoops() {
  iggy3d::ProductUiDrawList notReady;
  iggy3d::ProductVulkanGameplayFrame gameplayFrame;

  iggy3d::appendCreativeUiOverlay(gameplayFrame, notReady, 3U, 1280U, 720U);

  return expect(gameplayFrame.rects.empty(), "unready adds no rects") &&
         expect(gameplayFrame.textGlyphQuads.empty(),
                "unready adds no glyphs") &&
         expect(gameplayFrame.textGlyphCount == 0U,
                "unready glyph count stays zero");
}

bool creativeOverlayRefreshesGameplayFrameInput() {
  const iggy3d::ProductCreativeUiFrame creativeFrame = readyCreativeUiFrame();
  const iggy3d::ProductUiDrawList& drawList =
      creativeFrame.projection.drawList;
  iggy3d::ProductVulkanGameplayFrame gameplayFrame;

  iggy3d::appendProductUiOverlay(gameplayFrame, drawList, 4U, 1280U, 720U);
  const iggy3d::FrameInput& input =
      iggy3d::refreshProductVulkanGameplayFrameInput(gameplayFrame);

  return expect(drawList.theme == iggy3d::ProductUiThemeId::System,
                "creative draw list carries system theme") &&
         expect(input.ui.visible, "refreshed ui visible") &&
         expect(input.ui.rects == gameplayFrame.rects.data(),
                "refreshed rect pointer") &&
         expect(input.ui.rectCount == gameplayFrame.rects.size(),
                "refreshed rect count") &&
         expect(input.ui.textGlyphQuads ==
                    gameplayFrame.textGlyphQuads.data(),
                "refreshed glyph pointer") &&
         expect(input.ui.textGlyphQuadCount ==
                    gameplayFrame.textGlyphQuads.size(),
                "refreshed glyph quad count") &&
         expect(input.ui.textGlyphCount == gameplayFrame.textGlyphCount,
                "refreshed glyph count") &&
         expect(input.ui.primitiveCount ==
                    gameplayFrame.rects.size() +
                        gameplayFrame.textGlyphCount,
                "refreshed primitive count");
}

bool creativeEditorFrameSuppressesLegacyGameplayHudPanels() {
  const iggy3d::ProductGameplayProjectionFrame projection =
      legacyGameplayHudProjectionFrame();
  const iggy3d::ProductVulkanGameplayFrame normal =
      iggy3d::buildProductVulkanGameplayFrame(
          projection,
          5U,
          1280U,
          720U,
          0.0F,
          0.0F,
          iggy3d::productGameplayMovementTuning(),
          iggy3d::ProductGameplayMovementTuningField::WalkSpeed,
          false,
          false,
          iggy3d::FrontendDevToolsCategory::Session,
          false);
  const iggy3d::ProductVulkanGameplayFrame creative =
      iggy3d::buildProductVulkanGameplayFrame(
          projection,
          5U,
          1280U,
          720U,
          0.0F,
          0.0F,
          iggy3d::productGameplayMovementTuning(),
          iggy3d::ProductGameplayMovementTuningField::WalkSpeed,
          false,
          false,
          iggy3d::FrontendDevToolsCategory::Session,
          true);

  return expect(!normal.rects.empty(), "normal gameplay hud rects") &&
         expect(!normal.textGlyphQuads.empty(), "normal gameplay hud text") &&
         expect(creative.rects.empty(), "creative suppresses legacy rects") &&
         expect(creative.textGlyphQuads.empty(),
                "creative suppresses legacy text") &&
         expect(creative.textGlyphCount == 0U,
                "creative suppresses legacy glyph count");
}

}  // namespace

int main() {
  bool ok = true;
  ok &= readyCreativeOverlayAddsRectsAndGlyphs();
  ok &= legacyCreativeWindowDoesNotBuildDocumentOverlay();
  ok &= creativeOverlayAppendsToExistingHudOverlay();
  ok &= unreadyCreativeOverlayNoops();
  ok &= creativeOverlayRefreshesGameplayFrameInput();
  ok &= creativeEditorFrameSuppressesLegacyGameplayHudPanels();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
