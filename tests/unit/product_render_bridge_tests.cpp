#include "app/iggy3d/ProductRenderBridge.hpp"

#include <iostream>

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << "\n";
    return false;
  }
  return true;
}

iggy3d::ProductPrimitiveDrawItem drawItem(iggy3d::ProductPrimitiveDrawKind kind,
                                          std::uint64_t entityId,
                                          const char* name,
                                          bool targetable) {
  iggy3d::ProductPrimitiveDrawItem item;
  item.kind = kind;
  item.entityId = iggy3d::EntityId{entityId};
  item.stableName = name;
  item.visible = true;
  item.targetable = targetable;
  item.worldPosition = {static_cast<float>(entityId), 0.0F, 0.0F};
  return item;
}

}  // namespace

int main() {
  bool ok = true;

  const iggy3d::ProductRenderBridgeFrame empty =
      iggy3d::buildProductRenderBridgeFrame(nullptr, nullptr, nullptr);
  ok &= expect(!empty.ready, "empty bridge not ready");
  ok &= expect(!empty.viewFrameReady, "empty view frame not ready");
  ok &= expect(!empty.feedbackReady, "empty feedback not ready");
  ok &= expect(empty.frameItemCount == 0U, "empty frame item count");

  iggy3d::ProductPrimitiveDrawList drawList;
  drawList.gridVisible = true;
  drawList.playerVisible = true;
  drawList.objectiveVisible = true;
  drawList.playerFocusIndicatorVisible = true;
  drawList.items.push_back(drawItem(iggy3d::ProductPrimitiveDrawKind::PlayerMarker,
                                    1,
                                    "player",
                                    false));
  drawList.items.push_back(drawItem(iggy3d::ProductPrimitiveDrawKind::NpcMarker,
                                    2,
                                    "training_dummy",
                                    true));
  drawList.itemCount = static_cast<std::uint64_t>(drawList.items.size());

  iggy3d::ProductViewportFrame frame =
      iggy3d::buildProductViewportFrame(drawList, {});
  frame.framedItems[0].onScreen = true;
  frame.framedItems[1].onScreen = true;

  iggy3d::ProductGameplayFeedback feedback;
  feedback.visible = true;
  feedback.lines.push_back({"TARGET", "discovered", iggy3d::ProductFeedbackTone::Pass, true});
  feedback.lines.push_back({"REJECT", "none", iggy3d::ProductFeedbackTone::Fail, false});

  const iggy3d::ProductRenderBridgeFrame bridge =
      iggy3d::buildProductRenderBridgeFrame(&drawList, &frame, &feedback);
  ok &= expect(bridge.ready, "bridge ready with draw list and frame");
  ok &= expect(bridge.viewFrameReady, "view frame ready");
  ok &= expect(bridge.feedbackReady, "feedback ready");
  ok &= expect(bridge.drawItemCount == 2U, "draw item count copied");
  ok &= expect(bridge.frameItemCount == 2U, "frame item count");
  ok &= expect(bridge.onScreenItemCount == 2U, "on screen count");
  ok &= expect(bridge.targetItemCount == 1U, "target count");
  ok &= expect(bridge.feedbackLineCount == 1U, "visible feedback line count");
  ok &= expect(bridge.gridVisible, "grid visible copied");
  ok &= expect(bridge.playerVisible, "player visible copied");
  ok &= expect(bridge.objectiveVisible, "objective visible copied");
  ok &= expect(bridge.targetIndicatorVisible, "target indicator copied");
  ok &= expect(bridge.items.size() == 2U, "bridge item metadata count");
  ok &= expect(bridge.items[1].entityId == 2U, "entity id copied");
  ok &= expect(bridge.items[1].stableName == "training_dummy", "stable name copied");
  ok &= expect(bridge.items[1].targetable, "targetable copied");

  if (!ok) {
    return 1;
  }
  std::cout << "product_render_bridge_tests=pass\n";
  return 0;
}
