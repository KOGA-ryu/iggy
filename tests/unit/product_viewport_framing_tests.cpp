#include "app/iggy3d/ProductViewportFraming.hpp"

#include <cmath>
#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << "\n";
    return false;
  }
  return true;
}

bool nearlyEqual(float lhs, float rhs, float epsilon = 0.001F) {
  return std::fabs(lhs - rhs) <= epsilon;
}

iggy3d::ProductPrimitiveDrawItem item(iggy3d::ProductPrimitiveDrawKind kind,
                                      std::string_view name,
                                      float x,
                                      float z) {
  iggy3d::ProductPrimitiveDrawItem result;
  result.kind = kind;
  result.stableName = std::string{name};
  result.worldPosition = {x, 0.0F, z};
  result.visible = true;
  return result;
}

iggy3d::ProductViewportFrameConfig config(float yawDegrees, float pitchDegrees) {
  iggy3d::ProductViewportFrameConfig result;
  result.cameraYawDegrees = yawDegrees;
  result.cameraPitchDegrees = pitchDegrees;
  result.pixelsPerMeter = 10.0F;
  result.centerX = 100.0F;
  result.centerY = 100.0F;
  return result;
}

}  // namespace

int main() {
  iggy3d::ProductPrimitiveDrawList list;
  list.gridVisible = true;
  list.items.push_back(item(iggy3d::ProductPrimitiveDrawKind::PlayerMarker,
                            "player",
                            0.0F,
                            0.0F));
  list.items.push_back(item(iggy3d::ProductPrimitiveDrawKind::ObjectiveMarker,
                            "objective",
                            0.0F,
                            1.0F));

  const iggy3d::ProductViewportFrame yawZero =
      iggy3d::buildProductViewportFrame(list, config(0.0F, 0.0F));
  bool ok = true;
  ok &= expect(yawZero.projectionMode == "primitive_first_person",
               "projection mode stays honest");
  ok &= expect(yawZero.gridVisible, "grid visibility copied from draw list");
  ok &= expect(yawZero.playerAnchorFound, "player anchor found");
  ok &= expect(!yawZero.yawApplied, "yaw zero not marked applied");
  ok &= expect(!yawZero.pitchApplied, "pitch zero not marked applied");
  ok &= expect(yawZero.framedItems.size() == 2U, "framed item count");
  ok &= expect(nearlyEqual(yawZero.framedItems[0].screenX, 100.0F) &&
                   nearlyEqual(yawZero.framedItems[0].screenY, 100.0F),
               "player anchors at frame center");
  ok &= expect(nearlyEqual(yawZero.framedItems[1].screenX, 100.0F) &&
                   yawZero.framedItems[1].screenY < yawZero.framedItems[0].screenY,
               "yaw zero maps world positive z upward");

  const iggy3d::ProductViewportFrame yawNinety =
      iggy3d::buildProductViewportFrame(list, config(90.0F, 0.0F));
  ok &= expect(yawNinety.yawApplied, "nonzero yaw marked applied");
  ok &= expect(yawNinety.framedItems[1].screenX < yawNinety.framedItems[0].screenX,
               "yaw ninety rotates world positive z left");
  ok &= expect(!nearlyEqual(yawNinety.framedItems[1].screenX,
                            yawZero.framedItems[1].screenX),
               "yaw changes horizontal placement");

  const iggy3d::ProductViewportFrame pitchTen =
      iggy3d::buildProductViewportFrame(list, config(0.0F, 10.0F));
  ok &= expect(pitchTen.pitchApplied, "nonzero pitch marked applied");
  ok &= expect(nearlyEqual(pitchTen.framedItems[1].screenY,
                           yawZero.framedItems[1].screenY + 15.0F),
               "pitch applies deterministic vertical offset");

  iggy3d::ProductPrimitiveDrawList noPlayer;
  noPlayer.items.push_back(item(iggy3d::ProductPrimitiveDrawKind::ObjectiveMarker,
                                "objective",
                                1.0F,
                                0.0F));
  const iggy3d::ProductViewportFrame fallback =
      iggy3d::buildProductViewportFrame(noPlayer, config(0.0F, 0.0F));
  ok &= expect(!fallback.playerAnchorFound, "missing player uses fallback anchor");
  ok &= expect(fallback.framedItems.size() == 1U, "missing player still frames item");
  ok &= expect(nearlyEqual(fallback.framedItems[0].screenX, 110.0F),
               "fallback anchor is world origin");

  if (!ok) {
    return 1;
  }
  std::cout << "product_viewport_framing_tests=pass\n";
  return 0;
}
