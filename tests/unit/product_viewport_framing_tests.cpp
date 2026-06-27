#include "app/iggy3d/view/ViewportFraming.hpp"

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
                                      float z,
                                      float markerSize = 20.0F) {
  iggy3d::ProductPrimitiveDrawItem result;
  result.kind = kind;
  result.stableName = std::string{name};
  result.worldPosition = {x, 0.0F, z};
  result.visible = true;
  result.markerSize = markerSize;
  return result;
}

iggy3d::ProductViewportFrameConfig config(float yawDegrees, float pitchDegrees) {
  iggy3d::ProductViewportFrameConfig result;
  result.cameraYawDegrees = yawDegrees;
  result.cameraPitchDegrees = pitchDegrees;
  result.pixelsPerMeter = 100.0F;
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
                            "objective_ahead",
                            0.0F,
                            -4.0F));
  list.items.push_back(item(iggy3d::ProductPrimitiveDrawKind::NpcMarker,
                            "behind",
                            0.0F,
                            2.0F));
  list.items.push_back(item(iggy3d::ProductPrimitiveDrawKind::PickupMarker,
                            "near_pickup",
                            1.0F,
                            -2.0F,
                            20.0F));
  list.items.push_back(item(iggy3d::ProductPrimitiveDrawKind::PickupMarker,
                            "far_pickup",
                            1.0F,
                            -8.0F,
                            20.0F));
  list.items.push_back(item(iggy3d::ProductPrimitiveDrawKind::ObjectiveMarker,
                            "east_ahead_after_yaw",
                            4.0F,
                            0.0F));

  const iggy3d::ProductViewportFrame yawZero =
      iggy3d::buildProductViewportFrame(list, config(0.0F, 0.0F));
  bool ok = true;
  ok &= expect(yawZero.projectionMode == "primitive_first_person",
               "projection mode stays honest");
  ok &= expect(yawZero.gridVisible, "grid visibility copied from draw list");
  ok &= expect(yawZero.playerAnchorFound, "player anchor found");
  ok &= expect(!yawZero.yawApplied, "yaw zero not marked applied");
  ok &= expect(!yawZero.pitchApplied, "pitch zero not marked applied");
  ok &= expect(yawZero.framedItems.size() == 6U, "framed item count");
  ok &= expect(nearlyEqual(yawZero.framedItems[0].screenX, 100.0F) &&
                   nearlyEqual(yawZero.framedItems[0].depthMeters, 0.0F),
               "player anchors at camera depth");
  ok &= expect(!yawZero.framedItems[0].onScreen,
               "player marker is too close for first-person view");
  ok &= expect(nearlyEqual(yawZero.framedItems[1].screenX, 100.0F) &&
                   yawZero.framedItems[1].onScreen,
               "object directly ahead appears centered and on-screen");
  ok &= expect(nearlyEqual(yawZero.framedItems[1].depthMeters, 4.0F),
               "object ahead has positive depth");
  ok &= expect(!yawZero.framedItems[2].onScreen,
               "object behind player is culled");
  ok &= expect(yawZero.framedItems[2].depthMeters < 0.0F,
               "behind object has negative depth");
  ok &= expect(yawZero.framedItems[3].screenX > yawZero.framedItems[1].screenX,
               "right-side object projects right of center");
  ok &= expect(yawZero.framedItems[4].item.markerSize <
                   yawZero.framedItems[3].item.markerSize,
               "farther object has smaller marker size");

  const iggy3d::ProductViewportFrame yawNinety =
      iggy3d::buildProductViewportFrame(list, config(90.0F, 0.0F));
  ok &= expect(yawNinety.yawApplied, "nonzero yaw marked applied");
  ok &= expect(!yawNinety.framedItems[1].onScreen,
               "yaw ninety rotates former forward object out of view");
  ok &= expect(yawNinety.framedItems[5].onScreen,
               "yaw ninety makes world positive x forward");
  ok &= expect(nearlyEqual(yawNinety.framedItems[5].screenX, 100.0F),
               "yaw ninety centers world positive x object");

  const iggy3d::ProductViewportFrame pitchTen =
      iggy3d::buildProductViewportFrame(list, config(0.0F, 10.0F));
  ok &= expect(pitchTen.pitchApplied, "nonzero pitch marked applied");
  ok &= expect(nearlyEqual(pitchTen.framedItems[1].screenY,
                           yawZero.framedItems[1].screenY + 15.0F),
               "pitch shifts horizon with deterministic vertical offset");

  iggy3d::ProductPrimitiveDrawList noPlayer;
  noPlayer.items.push_back(item(iggy3d::ProductPrimitiveDrawKind::ObjectiveMarker,
                                "objective",
                                0.0F,
                                -4.0F));
  const iggy3d::ProductViewportFrame fallback =
      iggy3d::buildProductViewportFrame(noPlayer, config(0.0F, 0.0F));
  ok &= expect(!fallback.playerAnchorFound, "missing player uses fallback anchor");
  ok &= expect(fallback.framedItems.size() == 1U, "missing player still frames item");
  ok &= expect(fallback.framedItems[0].onScreen,
               "fallback origin still frames object in front");
  ok &= expect(nearlyEqual(fallback.framedItems[0].screenX, 100.0F),
               "fallback anchor centers forward object");

  iggy3d::ProductPrimitiveDrawList tooClose;
  tooClose.items.push_back(item(iggy3d::ProductPrimitiveDrawKind::PlayerMarker,
                                "player",
                                0.0F,
                                0.0F));
  tooClose.items.push_back(item(iggy3d::ProductPrimitiveDrawKind::PickupMarker,
                                "too_close",
                                0.0F,
                                -0.05F));
  const iggy3d::ProductViewportFrame nearFrame =
      iggy3d::buildProductViewportFrame(tooClose, config(0.0F, 0.0F));
  ok &= expect(!nearFrame.framedItems[1].onScreen,
               "too-close item is culled by near depth");

  if (!ok) {
    return 1;
  }
  std::cout << "product_viewport_framing_tests=pass\n";
  return 0;
}
