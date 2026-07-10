#include "app/iggy3d/window/FramePresenter.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

#include "app/frontend/DevToolsMenu.hpp"
#include "render/debug/DebugHudText.hpp"

namespace iggy3d {

// branch-gate-relocation: BG-1230 from=src/app/iggy3d/window/FramePresenter.cpp

namespace {

constexpr float kVirtualViewportWidth = 1280.0F;
constexpr float kVirtualViewportHeight = 720.0F;

std::int32_t scaledHudOffset(float virtualValue,
                             std::uint32_t viewportExtent,
                             float virtualExtent) {
  return static_cast<std::int32_t>(
      std::lround(virtualValue * static_cast<float>(viewportExtent) / virtualExtent));
}

std::uint32_t scaledHudExtent(float virtualValue,
                              std::uint32_t viewportExtent,
                              float virtualExtent) {
  const long scaled = std::lround(
      virtualValue * static_cast<float>(viewportExtent) / virtualExtent);
  return static_cast<std::uint32_t>(std::max(1L, scaled));
}

std::string fixedHudFloat(float value, int precision) {
  std::ostringstream stream;
  stream << std::fixed << std::setprecision(precision) << value;
  return stream.str();
}

void appendGameplayHudText(ProductVulkanGameplayFrame& frame,
                           std::string_view text,
                           float virtualX,
                           float virtualY,
                           std::uint32_t viewportWidth,
                           std::uint32_t viewportHeight) {
  const DebugHudLayoutResult layout = layoutDebugHudTextAt(
      text,
      scaledHudOffset(virtualX, viewportWidth, kVirtualViewportWidth),
      scaledHudOffset(virtualY, viewportHeight, kVirtualViewportHeight),
      viewportWidth,
      viewportHeight);
  frame.textGlyphCount += layout.glyphCount;
  frame.textGlyphQuads.insert(frame.textGlyphQuads.end(), layout.quads.begin(),
                              layout.quads.end());
}

void appendHudPanel(ProductVulkanGameplayFrame& frame,
                    float virtualX,
                    float virtualY,
                    float virtualWidth,
                    float virtualHeight,
                    std::uint32_t viewportWidth,
                    std::uint32_t viewportHeight,
                    float r = 14.0F / 255.0F,
                    float g = 21.0F / 255.0F,
                    float b = 23.0F / 255.0F,
                    float a = 1.0F) {
  frame.rects.push_back(RenderUiRect{
      scaledHudOffset(virtualX, viewportWidth, kVirtualViewportWidth),
      scaledHudOffset(virtualY, viewportHeight, kVirtualViewportHeight),
      scaledHudExtent(virtualWidth, viewportWidth, kVirtualViewportWidth),
      scaledHudExtent(virtualHeight, viewportHeight, kVirtualViewportHeight),
      r,
      g,
      b,
      a,
  });
}

void appendInteractionModeHudUi(ProductVulkanGameplayFrame& frame,
                                const InteractionModeHud& hud,
                                std::uint32_t viewportWidth,
                                std::uint32_t viewportHeight) {
  // branch-gate: BG-1065
  if (!hud.visible || viewportWidth == 0U || viewportHeight == 0U) {
    return;
  }
  appendHudPanel(frame, 862.0F, 238.0F, 328.0F, 34.0F, viewportWidth,
                 viewportHeight, 18.0F / 255.0F, 24.0F / 255.0F,
                 27.0F / 255.0F);
  appendGameplayHudText(frame, "MODE", 878.0F, 248.0F, viewportWidth,
                        viewportHeight);
  appendGameplayHudText(frame, hud.label, 970.0F, 248.0F, viewportWidth,
                        viewportHeight);
}

void appendGameplayFeedbackUi(ProductVulkanGameplayFrame& frame,
                              const GameplayFeedback& feedback,
                              std::uint32_t viewportWidth,
                              std::uint32_t viewportHeight) {
  // branch-gate: BG-1030
  if (!feedback.visible || viewportWidth == 0U || viewportHeight == 0U) {
    return;
  }
  appendHudPanel(frame, 860.0F, 410.0F, 330.0F, 150.0F, viewportWidth,
                 viewportHeight, 18.0F / 255.0F, 24.0F / 255.0F,
                 27.0F / 255.0F);
  appendGameplayHudText(frame, "ACTION FEEDBACK", 878.0F, 430.0F,
                        viewportWidth, viewportHeight);
  float virtualY = 462.0F;
  for (const GameplayFeedbackLine& line : feedback.lines) {
    // branch-gate: BG-1030
    if (!line.visible) {
      continue;
    }
    appendGameplayHudText(frame, line.label, 878.0F, virtualY, viewportWidth,
                          viewportHeight);
    appendGameplayHudText(frame, line.value, 1010.0F, virtualY, viewportWidth,
                          viewportHeight);
    virtualY += 24.0F;
  }
}

std::string_view vulkanTopDownMapTitle(const TopDownMapOverlay& overlay) {
  // branch-gate: BG-1071
  if (overlay.purpose == "minimap") {
    return "MINIMAP";
  }
  // branch-gate: BG-1071
  if (overlay.purpose == "editor_overview") {
    return "EDITOR OVERVIEW";
  }
  // branch-gate: BG-1071
  if (overlay.purpose == "hidden") {
    return "TOP-DOWN MAP";
  }
  return "TOP-DOWN DEBUG FALLBACK";
}

void appendTopDownMapOverlayUi(ProductVulkanGameplayFrame& frame,
                               const TopDownMapOverlay& overlay,
                               std::uint32_t viewportWidth,
                               std::uint32_t viewportHeight) {
  // branch-gate: BG-1071
  if (!overlay.visible || viewportWidth == 0U || viewportHeight == 0U) {
    return;
  }
  // branch-gate: BG-1071
  if (overlay.size == "compact") {
    appendHudPanel(frame, 874.0F, 74.0F, 318.0F, 190.0F, viewportWidth,
                   viewportHeight, 18.0F / 255.0F, 24.0F / 255.0F,
                   27.0F / 255.0F);
    appendHudPanel(frame, 890.0F, 116.0F, 270.0F, 126.0F, viewportWidth,
                   viewportHeight, 32.0F / 255.0F, 48.0F / 255.0F,
                   48.0F / 255.0F);
    appendGameplayHudText(frame, vulkanTopDownMapTitle(overlay), 890.0F,
                          86.0F, viewportWidth, viewportHeight);
    appendGameplayHudText(frame, "ITEMS", 902.0F, 132.0F, viewportWidth,
                          viewportHeight);
    appendGameplayHudText(frame, std::to_string(overlay.itemCount), 996.0F,
                          132.0F, viewportWidth, viewportHeight);
    return;
  }
  appendHudPanel(frame, 80.0F, 130.0F, 1120.0F, 480.0F, viewportWidth,
                 viewportHeight, 32.0F / 255.0F, 48.0F / 255.0F,
                 48.0F / 255.0F);
  appendGameplayHudText(frame, vulkanTopDownMapTitle(overlay), 100.0F, 150.0F,
                        viewportWidth, viewportHeight);
  appendGameplayHudText(frame, "ITEMS", 100.0F, 184.0F, viewportWidth,
                        viewportHeight);
  appendGameplayHudText(frame, std::to_string(overlay.itemCount), 194.0F,
                        184.0F, viewportWidth, viewportHeight);
}

void appendMovementDebugHudUi(ProductVulkanGameplayFrame& frame,
                              const MovementDebugHud& hud,
                              std::uint32_t viewportWidth,
                              std::uint32_t viewportHeight) {
  // branch-gate: BG-1030
  if (!hud.visible || viewportWidth == 0U || viewportHeight == 0U) {
    return;
  }
  appendHudPanel(frame, 92.0F, 146.0F, 500.0F, 190.0F, viewportWidth,
                 viewportHeight);
  appendGameplayHudText(frame, "MOVEMENT DEBUG", 108.0F, 164.0F,
                        viewportWidth, viewportHeight);
  float virtualY = 198.0F;
  for (const MovementDebugHudLine& line : hud.lines) {
    // branch-gate: BG-1030
    if (!line.visible) {
      continue;
    }
    appendGameplayHudText(frame, line.label, 108.0F, virtualY, viewportWidth,
                          viewportHeight);
    appendGameplayHudText(frame, line.value, 226.0F, virtualY, viewportWidth,
                          viewportHeight);
    virtualY += 22.0F;
  }
}

void appendNpcBehaviorDebugHudUi(ProductVulkanGameplayFrame& frame,
                                 const NpcBehaviorDebugHud& hud,
                                 std::uint32_t viewportWidth,
                                 std::uint32_t viewportHeight) {
  // branch-gate: BG-1030
  if (!hud.visible || viewportWidth == 0U || viewportHeight == 0U) {
    return;
  }
  appendHudPanel(frame, 700.0F, 146.0F, 490.0F, 220.0F, viewportWidth,
                 viewportHeight);
  appendGameplayHudText(frame, "NPC DEBUG", 716.0F, 164.0F, viewportWidth,
                        viewportHeight);
  float virtualY = 198.0F;
  std::uint64_t drawn = 0U;
  for (const NpcBehaviorDebugHudLine& line : hud.lines) {
    // branch-gate: BG-1030
    if (!line.visible) {
      continue;
    }
    // branch-gate: BG-1030
    if (drawn >= 8U) {
      break;
    }
    appendGameplayHudText(frame, line.text, 716.0F, virtualY, viewportWidth,
                          viewportHeight);
    virtualY += 20.0F;
    ++drawn;
  }
}

void appendPhysicsDebugHudUi(ProductVulkanGameplayFrame& frame,
                             const PhysicsDebugHud& hud,
                             std::uint32_t viewportWidth,
                             std::uint32_t viewportHeight) {
  // branch-gate: BG-1110
  if (!hud.visible || viewportWidth == 0U || viewportHeight == 0U) {
    return;
  }
  appendHudPanel(frame, 92.0F, 360.0F, 720.0F, 150.0F, viewportWidth,
                 viewportHeight);
  appendGameplayHudText(frame, "PHYSICS DEBUG", 108.0F, 378.0F,
                        viewportWidth, viewportHeight);
  float virtualY = 408.0F;
  std::uint64_t drawn = 0U;
  for (const PhysicsDebugHudLine& line : hud.lines) {
    // branch-gate: BG-1110
    if (!line.visible) {
      continue;
    }
    // branch-gate: BG-1110
    if (drawn >= 5U) {
      break;
    }
    appendGameplayHudText(frame, line.text, 108.0F, virtualY, viewportWidth,
                          viewportHeight);
    virtualY += 20.0F;
    ++drawn;
  }
}

void appendRoomEditorHudUi(ProductVulkanGameplayFrame& frame,
                           const ProductRoomEditorHud& hud,
                           std::uint32_t viewportWidth,
                           std::uint32_t viewportHeight) {
  // branch-gate: BG-1035
  if (!hud.visible || viewportWidth == 0U || viewportHeight == 0U) {
    return;
  }
  appendHudPanel(frame, 88.0F, 388.0F, 560.0F, 180.0F, viewportWidth,
                 viewportHeight, 18.0F / 255.0F, 24.0F / 255.0F,
                 27.0F / 255.0F);
  appendGameplayHudText(frame, "ROOM EDITOR", 106.0F, 408.0F, viewportWidth,
                        viewportHeight);
  float virtualY = 440.0F;
  std::uint64_t drawn = 0U;
  for (const ProductRoomEditorHudLine& line : hud.lines) {
    // branch-gate: BG-1035
    if (!line.visible) {
      continue;
    }
    // branch-gate: BG-1035
    if (drawn >= 6U) {
      break;
    }
    appendGameplayHudText(frame, line.text, 106.0F, virtualY, viewportWidth,
                          viewportHeight);
    virtualY += 20.0F;
    ++drawn;
  }
}

void appendPositionHudUi(ProductVulkanGameplayFrame& frame,
                         const PositionHud& hud,
                         std::uint32_t viewportWidth,
                         std::uint32_t viewportHeight) {
  // branch-gate: BG-1196
  if (!hud.visible || viewportWidth == 0U || viewportHeight == 0U) {
    return;
  }

  frame.rects.push_back(RenderUiRect{
      scaledHudOffset(88.0F, viewportWidth, kVirtualViewportWidth),
      scaledHudOffset(572.0F, viewportHeight, kVirtualViewportHeight),
      scaledHudExtent(360.0F, viewportWidth, kVirtualViewportWidth),
      scaledHudExtent(54.0F, viewportHeight, kVirtualViewportHeight),
      14.0F / 255.0F,
      21.0F / 255.0F,
      23.0F / 255.0F,
      1.0F,
  });

  const std::int32_t textX =
      scaledHudOffset(100.0F, viewportWidth, kVirtualViewportWidth);
  std::int32_t textY =
      scaledHudOffset(582.0F, viewportHeight, kVirtualViewportHeight);
  const std::int32_t lineStep =
      scaledHudOffset(16.0F, viewportHeight, kVirtualViewportHeight);
  std::uint64_t drawn = 0U;
  for (const PositionHudLine& line : hud.lines) {
    // branch-gate: BG-1196
    if (!line.visible) {
      continue;
    }
    // branch-gate: BG-1196
    if (drawn >= 3U) {
      break;
    }
    const DebugHudLayoutResult layout =
        layoutDebugHudTextAt(line.text, textX, textY, viewportWidth, viewportHeight);
    frame.textGlyphCount += layout.glyphCount;
    frame.textGlyphQuads.insert(frame.textGlyphQuads.end(), layout.quads.begin(),
                                layout.quads.end());
    textY += lineStep;
    ++drawn;
  }
}

void appendMovementTuningHudUi(
    ProductVulkanGameplayFrame& frame,
    const ProductGameplayMovementTuning& tuning,
    ProductGameplayMovementTuningField selectedField,
    bool visible,
    std::uint32_t viewportWidth,
    std::uint32_t viewportHeight) {
  // branch-gate: BG-1213
  if (!visible || viewportWidth == 0U || viewportHeight == 0U) {
    return;
  }

  frame.rects.push_back(RenderUiRect{
      scaledHudOffset(820.0F, viewportWidth, kVirtualViewportWidth),
      scaledHudOffset(330.0F, viewportHeight, kVirtualViewportHeight),
      scaledHudExtent(384.0F, viewportWidth, kVirtualViewportWidth),
      scaledHudExtent(368.0F, viewportHeight, kVirtualViewportHeight),
      14.0F / 255.0F,
      21.0F / 255.0F,
      23.0F / 255.0F,
      1.0F,
  });

  appendGameplayHudText(frame,
                        "MOVEMENT TUNING",
                        838.0F,
                        346.0F,
                        viewportWidth,
                        viewportHeight);
  appendGameplayHudText(frame,
                        "F4 HIDE  ENTER/UP/DOWN FIELD",
                        838.0F,
                        376.0F,
                        viewportWidth,
                        viewportHeight);
  appendGameplayHudText(frame,
                        "LEFT/RIGHT VALUE",
                        838.0F,
                        392.0F,
                        viewportWidth,
                        viewportHeight);

  float virtualY = 418.0F;
  std::uint64_t drawn = 0U;
  for (const ProductGameplayMovementTuningFieldDescriptor& descriptor :
       kProductGameplayMovementTuningFields) {
    // branch-gate: BG-1213
    if (drawn >= productGameplayMovementTuningFieldCount()) {
      break;
    }
    const bool selected = descriptor.field == selectedField;
    // branch-gate: BG-1213
    if (selected) {
      frame.rects.push_back(RenderUiRect{
          scaledHudOffset(834.0F, viewportWidth, kVirtualViewportWidth),
          scaledHudOffset(virtualY - 2.0F, viewportHeight, kVirtualViewportHeight),
          scaledHudExtent(348.0F, viewportWidth, kVirtualViewportWidth),
          scaledHudExtent(14.0F, viewportHeight, kVirtualViewportHeight),
          63.0F / 255.0F,
          71.0F / 255.0F,
          58.0F / 255.0F,
          1.0F,
      });
    }
    std::string row = selected ? "> " : "  ";  // branch-gate: BG-1213
    row += descriptor.label;
    row += " ";
    // branch-gate: BG-1213
    if (descriptor.kind == ProductGameplayMovementTuningFieldKind::Toggle) {
      row += (tuning.*(descriptor.value) >= 0.5F ? "ON" : "OFF");
    } else {
      row += fixedHudFloat(tuning.*(descriptor.value),
                           descriptor.step < 0.05F ? 2 : 1);  // branch-gate: BG-1213
    }
    appendGameplayHudText(frame,
                          row,
                          838.0F,
                          virtualY,
                          viewportWidth,
                          viewportHeight);
    virtualY += 16.0F;
    ++drawn;
  }
}

void appendDevToolsOverlayUi(ProductVulkanGameplayFrame& frame,
                             bool visible,
                             FrontendDevToolsCategory selectedCategory,
                             std::uint32_t viewportWidth,
                             std::uint32_t viewportHeight) {
  // branch-gate: BG-1214
  if (!visible || viewportWidth == 0U || viewportHeight == 0U) {
    return;
  }

  frame.rects.push_back(RenderUiRect{
      scaledHudOffset(414.0F, viewportWidth, kVirtualViewportWidth),
      scaledHudOffset(100.0F, viewportHeight, kVirtualViewportHeight),
      scaledHudExtent(760.0F, viewportWidth, kVirtualViewportWidth),
      scaledHudExtent(438.0F, viewportHeight, kVirtualViewportHeight),
      14.0F / 255.0F,
      21.0F / 255.0F,
      23.0F / 255.0F,
      1.0F,
  });
  appendGameplayHudText(frame,
                        "DEV TOOLS",
                        450.0F,
                        128.0F,
                        viewportWidth,
                        viewportHeight);
  appendGameplayHudText(frame,
                        devToolsFunctionKeyHintLabel(),
                        452.0F,
                        185.0F,
                        viewportWidth,
                        viewportHeight);

  float virtualY = 230.0F;
  for (const FrontendDevToolsCategory category : devToolsCategoryOrder()) {
    const bool selected = category == selectedCategory;
    // branch-gate: BG-1214
    if (selected) {
      frame.rects.push_back(RenderUiRect{
          scaledHudOffset(444.0F, viewportWidth, kVirtualViewportWidth),
          scaledHudOffset(virtualY - 6.0F, viewportHeight, kVirtualViewportHeight),
          scaledHudExtent(320.0F, viewportWidth, kVirtualViewportWidth),
          scaledHudExtent(26.0F, viewportHeight, kVirtualViewportHeight),
          63.0F / 255.0F,
          71.0F / 255.0F,
          58.0F / 255.0F,
          1.0F,
      });
    }
    std::string row = selected ? "> " : "  ";  // branch-gate: BG-1214
    row += devToolsCategoryLabel(category);
    appendGameplayHudText(frame,
                          row,
                          458.0F,
                          virtualY,
                          viewportWidth,
                          viewportHeight);
    virtualY += 34.0F;
  }

  appendGameplayHudText(frame,
                        "READOUTS",
                        850.0F,
                        230.0F,
                        viewportWidth,
                        viewportHeight);
  appendGameplayHudText(frame,
                        "RUNTIME STATE",
                        850.0F,
                        272.0F,
                        viewportWidth,
                        viewportHeight);
  appendGameplayHudText(frame,
                        "INPUT OWNER",
                        850.0F,
                        304.0F,
                        viewportWidth,
                        viewportHeight);
  appendGameplayHudText(frame,
                        "RENDERER STATUS",
                        850.0F,
                        336.0F,
                        viewportWidth,
                        viewportHeight);
  appendGameplayHudText(frame,
                        "BACK",
                        850.0F,
                        394.0F,
                        viewportWidth,
                        viewportHeight);
}

void appendMapMakerCubePreviewUi(ProductVulkanGameplayFrame& frame,
                                  const ProductViewportFrame& viewportFrame,
                                  std::uint32_t viewportWidth,
                                  std::uint32_t viewportHeight) {
  // branch-gate: BG-1206
  if (viewportWidth == 0U || viewportHeight == 0U) {
    return;
  }
  for (const ProductViewportFramedItem& framed : viewportFrame.framedItems) {
    // branch-gate: BG-1206
    if (!framed.onScreen ||
        framed.item.kind != ProductPrimitiveDrawKind::MapMakerCubePreview) {
      continue;
    }
    const float size = std::clamp(framed.item.markerSize, 18.0F, 72.0F);
    frame.rects.push_back(RenderUiRect{
        scaledHudOffset(framed.screenX - size * 0.5F, viewportWidth,
                        kVirtualViewportWidth),
        scaledHudOffset(framed.screenY - size * 0.5F, viewportHeight,
                        kVirtualViewportHeight),
        scaledHudExtent(size, viewportWidth, kVirtualViewportWidth),
        scaledHudExtent(size, viewportHeight, kVirtualViewportHeight),
        static_cast<float>(framed.item.color.r) / 255.0F,
        static_cast<float>(framed.item.color.g) / 255.0F,
        static_cast<float>(framed.item.color.b) / 255.0F,
        1.0F,
    });
    // branch-gate: BG-1206
    if (size > 12.0F) {
      const float inset = 4.0F;
      frame.rects.push_back(RenderUiRect{
          scaledHudOffset(framed.screenX - size * 0.5F + inset, viewportWidth,
                          kVirtualViewportWidth),
          scaledHudOffset(framed.screenY - size * 0.5F + inset, viewportHeight,
                          kVirtualViewportHeight),
          scaledHudExtent(size - inset * 2.0F, viewportWidth, kVirtualViewportWidth),
          scaledHudExtent(size - inset * 2.0F, viewportHeight,
                          kVirtualViewportHeight),
          48.0F / 255.0F,
          65.0F / 255.0F,
          72.0F / 255.0F,
          0.92F,
      });
    }
  }
}

void appendMapMakerHudUi(ProductVulkanGameplayFrame& frame,
                         const ProductMapMakerHud& hud,
                         std::uint32_t viewportWidth,
                         std::uint32_t viewportHeight) {
  // branch-gate: BG-1205
  if (!hud.visible || viewportWidth == 0U || viewportHeight == 0U) {
    return;
  }
  frame.rects.push_back(RenderUiRect{
      scaledHudOffset(88.0F, viewportWidth, kVirtualViewportWidth),
      scaledHudOffset(636.0F, viewportHeight, kVirtualViewportHeight),
      scaledHudExtent(300.0F, viewportWidth, kVirtualViewportWidth),
      scaledHudExtent(26.0F, viewportHeight, kVirtualViewportHeight),
      14.0F / 255.0F,
      21.0F / 255.0F,
      23.0F / 255.0F,
      1.0F,
  });
  const std::int32_t textX =
      scaledHudOffset(100.0F, viewportWidth, kVirtualViewportWidth);
  std::int32_t textY =
      scaledHudOffset(644.0F, viewportHeight, kVirtualViewportHeight);
  for (const ProductMapMakerHudLine& line : hud.lines) {
    // branch-gate: BG-1205
    if (!line.visible) {
      continue;
    }
    const DebugHudLayoutResult layout =
        layoutDebugHudTextAt(line.text, textX, textY, viewportWidth, viewportHeight);
    frame.textGlyphCount += layout.glyphCount;
    frame.textGlyphQuads.insert(frame.textGlyphQuads.end(), layout.quads.begin(),
                                layout.quads.end());
    break;
  }
}

}  // namespace

ProductVulkanGameplayFrame buildProductVulkanGameplayFrame(
    const ProductGameplayProjectionFrame& projectionFrame,
    std::uint64_t frameIndex,
    std::uint32_t viewportWidth,
    std::uint32_t viewportHeight,
    float cameraYawDegrees,
    float cameraPitchDegrees,
    const ProductGameplayMovementTuning& movementTuning,
    ProductGameplayMovementTuningField movementTuningField,
    bool movementTuningVisible,
    bool devToolsOverlayVisible,
    FrontendDevToolsCategory devToolsCategory,
    bool creativeEditorOverlayActive) {
  ProductVulkanGameplayFrame frame;
  frame.frame = makeProductVulkanFrame(projectionFrame.scene, projectionFrame.debug,
                                       frameIndex, viewportWidth, viewportHeight,
                                       cameraYawDegrees, cameraPitchDegrees,
                                       projectionFrame.cameraAnchorOverrideAvailable,
                                       projectionFrame.cameraAnchorOverrideMeters);
  if (!creativeEditorOverlayActive) {
    appendMapMakerCubePreviewUi(frame, projectionFrame.viewportFrame, viewportWidth,
                                viewportHeight);
    appendTopDownMapOverlayUi(frame, projectionFrame.topDownMapOverlay,
                              viewportWidth, viewportHeight);
    appendMapMakerHudUi(frame, projectionFrame.mapMakerHud, viewportWidth,
                        viewportHeight);
    appendInteractionModeHudUi(frame,
                               projectionFrame.interactionModeHud,
                               viewportWidth,
                               viewportHeight);
    appendMovementDebugHudUi(frame, projectionFrame.movementHud, viewportWidth,
                             viewportHeight);
    appendNpcBehaviorDebugHudUi(frame,
                                projectionFrame.npcBehaviorHud,
                                viewportWidth,
                                viewportHeight);
    appendPhysicsDebugHudUi(frame, projectionFrame.physicsHud, viewportWidth,
                            viewportHeight);
    appendRoomEditorHudUi(frame, projectionFrame.roomEditorHud, viewportWidth,
                          viewportHeight);
    appendPositionHudUi(frame, projectionFrame.positionHud, viewportWidth,
                        viewportHeight);
    appendMovementTuningHudUi(frame,
                              movementTuning,
                              movementTuningField,
                              movementTuningVisible,
                              viewportWidth,
                              viewportHeight);
  }
  // TV1-H (TL-5): the DevTools overlay is creative-aware — it stays off while
  // the creative document editor overlay owns the viewport, alongside the other
  // HUDs gated above. (The pause journal stays topmost by design.)
  if (!creativeEditorOverlayActive) {
    appendDevToolsOverlayUi(frame,
                            devToolsOverlayVisible,
                            devToolsCategory,
                            viewportWidth,
                            viewportHeight);
    appendGameplayFeedbackUi(frame, projectionFrame.feedback, viewportWidth,
                             viewportHeight);
  }
  return frame;
}

const FrameInput& refreshProductVulkanGameplayFrameInput(
    ProductVulkanGameplayFrame& gameplayFrame) {
  gameplayFrame.frame.ui.visible =
      !gameplayFrame.rects.empty() || !gameplayFrame.textGlyphQuads.empty();
  gameplayFrame.frame.ui.rects = gameplayFrame.rects.data();
  gameplayFrame.frame.ui.rectCount = gameplayFrame.rects.size();
  gameplayFrame.frame.ui.textGlyphQuads = gameplayFrame.textGlyphQuads.data();
  gameplayFrame.frame.ui.textGlyphQuadCount = gameplayFrame.textGlyphQuads.size();
  gameplayFrame.frame.ui.textGlyphCount = gameplayFrame.textGlyphCount;
  gameplayFrame.frame.ui.primitiveCount =
      gameplayFrame.rects.size() + gameplayFrame.textGlyphCount;
  gameplayFrame.frame.creativeWireframeDebug.lines =
      gameplayFrame.creativeWireframeDebugLines.data();
  gameplayFrame.frame.creativeWireframeDebug.lineCount =
      gameplayFrame.creativeWireframeDebugLines.size();
  gameplayFrame.frame.creativeWireframeDebug.visible =
      gameplayFrame.frame.creativeWireframeDebug.available &&
      !gameplayFrame.creativeWireframeDebugLines.empty();
  return gameplayFrame.frame;
}

}  // namespace iggy3d
