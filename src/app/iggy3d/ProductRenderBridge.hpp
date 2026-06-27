#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "app/iggy3d/gameplay/ProductGameplayFeedback.hpp"
#include "app/iggy3d/ProductPrimitiveDrawList.hpp"
#include "app/iggy3d/ProductViewportFraming.hpp"

namespace iggy3d {

struct ProductRenderBridgeItem {
  ProductPrimitiveDrawKind kind = ProductPrimitiveDrawKind::DebugMarker;
  std::uint64_t entityId = 0;
  std::string stableName;
  bool visible = false;
  bool onScreen = false;
  bool targetable = false;
  bool interactable = false;
  float screenX = 0.0F;
  float screenY = 0.0F;
  float depthMeters = 0.0F;
};

struct ProductRenderBridgeFrame {
  bool ready = false;
  bool viewFrameReady = false;
  bool feedbackReady = false;
  std::uint64_t drawItemCount = 0;
  std::uint64_t frameItemCount = 0;
  std::uint64_t onScreenItemCount = 0;
  std::uint64_t targetItemCount = 0;
  std::uint64_t feedbackLineCount = 0;
  bool gridVisible = false;
  bool playerVisible = false;
  bool objectiveVisible = false;
  bool targetIndicatorVisible = false;
  bool roomEditorCursorVisible = false;
  std::uint64_t roomEditorCursorCount = 0;
  bool roomEditorPlacementPreviewVisible = false;
  std::uint64_t roomEditorPlacementPreviewCount = 0;
  bool physicsDebugVisible = false;
  std::uint64_t physicsDebugItemCount = 0;
  std::uint64_t physicsAabbDebugCount = 0;
  std::uint64_t physicsContactNormalDebugCount = 0;
  std::uint64_t physicsBroadphasePairDebugCount = 0;
  std::string projectionMode = "primitive_first_person";
  std::vector<ProductRenderBridgeItem> items;
};

ProductRenderBridgeFrame buildProductRenderBridgeFrame(
    const ProductPrimitiveDrawList* drawList,
    const ProductViewportFrame* frame,
    const ProductGameplayFeedback* feedback);

}  // namespace iggy3d
