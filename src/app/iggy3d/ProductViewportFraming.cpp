#include "app/iggy3d/ProductViewportFraming.hpp"

#include <cmath>

namespace iggy3d {
namespace {

constexpr float kPi = 3.14159265358979323846F;

bool isPlayerMarker(const ProductPrimitiveDrawItem& item) {
  return item.kind == ProductPrimitiveDrawKind::PlayerMarker && item.visible;
}

Vec3 cameraAnchorFor(const ProductPrimitiveDrawList& drawList, bool& found) {
  for (const ProductPrimitiveDrawItem& item : drawList.items) {
    if (isPlayerMarker(item)) {
      found = true;
      return item.worldPosition;
    }
  }
  found = false;
  return {};
}

bool itemOnScreen(float x, float y) {
  return x >= 0.0F && x <= 1280.0F && y >= 0.0F && y <= 720.0F;
}

}  // namespace

ProductViewportFrame buildProductViewportFrame(const ProductPrimitiveDrawList& drawList,
                                               const ProductViewportFrameConfig& config) {
  ProductViewportFrame frame;
  frame.gridVisible = drawList.gridVisible;
  frame.yawApplied = config.cameraYawDegrees != 0.0F;
  frame.pitchApplied = config.cameraPitchDegrees != 0.0F;

  const Vec3 anchor = cameraAnchorFor(drawList, frame.playerAnchorFound);
  const float yawRadians = config.cameraYawDegrees * kPi / 180.0F;
  const float cosYaw = std::cos(yawRadians);
  const float sinYaw = std::sin(yawRadians);
  const float pitchOffsetPixels = config.cameraPitchDegrees * 1.5F;

  frame.framedItems.reserve(drawList.items.size());
  for (const ProductPrimitiveDrawItem& item : drawList.items) {
    const float offsetX = item.worldPosition.x - anchor.x;
    const float offsetZ = item.worldPosition.z - anchor.z;
    const float viewX = offsetX * cosYaw - offsetZ * sinYaw;
    const float viewZ = offsetX * sinYaw + offsetZ * cosYaw;

    ProductViewportFramedItem framed;
    framed.item = item;
    framed.screenX = config.centerX + viewX * config.pixelsPerMeter;
    framed.screenY = config.centerY - viewZ * config.pixelsPerMeter + pitchOffsetPixels;
    framed.depthMeters = viewZ;
    framed.onScreen = item.visible && itemOnScreen(framed.screenX, framed.screenY);
    frame.framedItems.push_back(framed);
  }

  return frame;
}

}  // namespace iggy3d
