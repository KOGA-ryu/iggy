#include "app/iggy3d/ProductViewportFraming.hpp"

#include <algorithm>
#include <cmath>

namespace iggy3d {
namespace {

constexpr float kPi = 3.14159265358979323846F;
constexpr float kNearDepthMeters = 0.15F;
constexpr float kDepthScreenDropPixelsPerMeter = 4.0F;
constexpr float kReferenceMarkerDepthMeters = 4.0F;
constexpr float kMinPerspectiveMarkerScale = 0.35F;
constexpr float kMaxPerspectiveMarkerScale = 1.25F;

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

float perspectiveMarkerSize(float markerSize, float depthMeters) {
  const float scale = std::clamp(kReferenceMarkerDepthMeters / depthMeters,
                                 kMinPerspectiveMarkerScale,
                                 kMaxPerspectiveMarkerScale);
  return markerSize * scale;
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
  const Vec3 forward{sinYaw, 0.0F, -cosYaw};
  const Vec3 right{cosYaw, 0.0F, sinYaw};
  const float focalPixels = std::max(1.0F, config.pixelsPerMeter);

  frame.framedItems.reserve(drawList.items.size());
  for (const ProductPrimitiveDrawItem& item : drawList.items) {
    const Vec3 offset = item.worldPosition - anchor;
    const float lateralMeters = offset.x * right.x + offset.z * right.z;
    const float depthMeters = offset.x * forward.x + offset.z * forward.z;
    const float safeDepthMeters = std::max(depthMeters, kNearDepthMeters);
    const float relativeHeightMeters = item.worldPosition.y - anchor.y;

    ProductViewportFramedItem framed;
    framed.item = item;
    framed.item.markerSize = perspectiveMarkerSize(item.markerSize, safeDepthMeters);
    framed.screenX = config.centerX + (lateralMeters / safeDepthMeters) * focalPixels;
    framed.screenY = config.centerY + pitchOffsetPixels +
                     (depthMeters * kDepthScreenDropPixelsPerMeter) -
                     (relativeHeightMeters / safeDepthMeters) * focalPixels;
    framed.depthMeters = depthMeters;
    framed.onScreen = item.visible && depthMeters > kNearDepthMeters &&
                      itemOnScreen(framed.screenX, framed.screenY);
    frame.framedItems.push_back(framed);
  }

  return frame;
}

}  // namespace iggy3d
