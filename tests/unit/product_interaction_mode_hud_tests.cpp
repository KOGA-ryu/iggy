#include "app/iggy3d/debug/DebugHudPanels.hpp"

#include <iostream>

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << "\n";
    return false;
  }
  return true;
}

}  // namespace

int main() {
  bool ok = true;

  const iggy3d::InteractionModeHud inactive =
      iggy3d::buildInteractionModeHud({
          iggy3d::ProductInteractionMode::Player,
          false,
          false,
      });
  ok &= expect(!inactive.visible, "inactive hides mode hud");
  ok &= expect(inactive.status == "interaction_mode_hud_hidden",
               "inactive status");
  ok &= expect(inactive.reasonCode == "interaction_mode_hud_hidden",
               "inactive reason");
  ok &= expect(inactive.mode == "player", "inactive mode copied");
  ok &= expect(inactive.label == "player", "inactive label copied");

  const iggy3d::InteractionModeHud player =
      iggy3d::buildInteractionModeHud({
          iggy3d::ProductInteractionMode::Player,
          true,
          false,
      });
  ok &= expect(player.visible, "player mode hud visible");
  ok &= expect(player.status == "interaction_mode_hud_ready",
               "player ready status");
  ok &= expect(player.reasonCode == "interaction_mode_hud_ready",
               "player ready reason");
  ok &= expect(player.mode == "player", "player mode name");
  ok &= expect(player.label == "player", "player label");
  ok &= expect(player.tone == iggy3d::FeedbackTone::Neutral,
               "player neutral tone");

  const iggy3d::InteractionModeHud creative =
      iggy3d::buildInteractionModeHud({
          iggy3d::ProductInteractionMode::Creative,
          true,
          true,
      });
  ok &= expect(creative.visible, "creative mode hud visible");
  ok &= expect(creative.status == "interaction_mode_hud_ready",
               "creative ready status");
  ok &= expect(creative.mode == "creative", "creative mode name");
  ok &= expect(creative.label == "creative", "creative label");
  ok &= expect(creative.roomEditingReady, "creative room editing ready copied");
  ok &= expect(creative.tone == iggy3d::FeedbackTone::Warn,
               "creative accent tone");

  const iggy3d::InteractionModeHud unknown =
      iggy3d::buildInteractionModeHud({
          static_cast<iggy3d::ProductInteractionMode>(255U),
          true,
          true,
      });
  ok &= expect(!unknown.visible, "unknown mode hides hud");
  ok &= expect(unknown.status == "interaction_mode_hud_unknown_mode",
               "unknown status");
  ok &= expect(unknown.reasonCode == "interaction_mode_hud_unknown_mode",
               "unknown reason");
  ok &= expect(unknown.mode == "unknown", "unknown mode name");
  ok &= expect(unknown.label == "unknown", "unknown label");

  if (!ok) {
    return 1;
  }
  std::cout << "product_interaction_mode_hud_tests=pass\n";
  return 0;
}
