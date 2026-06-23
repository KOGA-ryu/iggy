#pragma once

#include <string>
#include <vector>

#include "app/iggy3d/ProductPrimitiveDrawList.hpp"

namespace iggy3d {

struct ProductViewportFrameConfig {
  float cameraYawDegrees = 0.0F;
  float cameraPitchDegrees = 0.0F;
  float pixelsPerMeter = 92.0F;
  float centerX = 640.0F;
  float centerY = 394.0F;
};

struct ProductViewportFramedItem {
  ProductPrimitiveDrawItem item;
  float screenX = 0.0F;
  float screenY = 0.0F;
  bool onScreen = false;
  float depthMeters = 0.0F;
};

struct ProductViewportFrame {
  std::vector<ProductViewportFramedItem> framedItems;
  bool gridVisible = false;
  bool yawApplied = false;
  bool pitchApplied = false;
  bool playerAnchorFound = false;
  std::string projectionMode = "primitive_first_person";
};

ProductViewportFrame buildProductViewportFrame(const ProductPrimitiveDrawList& drawList,
                                               const ProductViewportFrameConfig& config);

}  // namespace iggy3d
