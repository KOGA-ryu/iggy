#include "app/iggy3d/ProductNpcBehaviorDebugHud.hpp"

#include <iostream>

#include "projection/debug/DebugProjection.hpp"

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << "\n";
    return false;
  }
  return true;
}

iggy3d::DebugProjectionResult npcDebugProjection() {
  iggy3d::DebugProjectionResult debug;
  debug.npcBehaviorDebugHudLines.push_back(
      "NPCS world=2 ai=2 resolved=1 failed=1 hostile=1 passive=0");
  debug.npcBehaviorDebugHudLines.push_back(
      "NPC 2 training_dummy default attacking/attack_target tgt=player cd=2");
  debug.npcBehaviorDebugHudLines.push_back(
      "NPC 3 ghost ghost_profile idle/none tgt=none cd=0 "
      "unresolved=profile_missing");
  return debug;
}

bool visibleLine(const iggy3d::ProductNpcBehaviorDebugHud& hud,
                 std::size_t index,
                 const char* text,
                 iggy3d::ProductFeedbackTone tone) {
  return index < hud.lines.size() && hud.lines[index].visible &&
         hud.lines[index].text == text && hud.lines[index].tone == tone;
}

}  // namespace

int main() {
  bool ok = true;
  const iggy3d::DebugProjectionResult debug = npcDebugProjection();

  const iggy3d::ProductNpcBehaviorDebugHud inactive =
      iggy3d::buildProductNpcBehaviorDebugHud(&debug, false, true, true);
  ok &= expect(!inactive.visible, "inactive gameplay hides npc hud");
  ok &= expect(inactive.status == "not_requested", "inactive status");
  ok &= expect(inactive.debugAvailable, "inactive still reports debug available");
  ok &= expect(inactive.lineCount == 3U, "inactive copies line count");
  ok &= expect(inactive.lines.empty(), "inactive emits no lines");

  const iggy3d::ProductNpcBehaviorDebugHud devDisabled =
      iggy3d::buildProductNpcBehaviorDebugHud(&debug, true, false, true);
  ok &= expect(!devDisabled.visible, "developer tools disabled hides npc hud");
  ok &= expect(devDisabled.developerToolsEnabled == false, "dev disabled proof");
  ok &= expect(devDisabled.status == "not_requested", "dev disabled status");
  ok &= expect(devDisabled.lineCount == 3U, "dev disabled line count");

  const iggy3d::ProductNpcBehaviorDebugHud overlayDisabled =
      iggy3d::buildProductNpcBehaviorDebugHud(&debug, true, true, false);
  ok &= expect(!overlayDisabled.visible, "debug overlay disabled hides npc hud");
  ok &= expect(!overlayDisabled.debugOverlayEnabled, "overlay disabled proof");
  ok &= expect(overlayDisabled.status == "not_requested", "overlay disabled status");

  const iggy3d::ProductNpcBehaviorDebugHud missingProjection =
      iggy3d::buildProductNpcBehaviorDebugHud(nullptr, true, true, true);
  ok &= expect(!missingProjection.visible, "missing projection hides npc hud");
  ok &= expect(!missingProjection.debugAvailable, "missing projection proof");
  ok &= expect(missingProjection.status == "projection_missing",
               "missing projection status");
  ok &= expect(missingProjection.reasonCode == "projection_missing",
               "missing projection reason");
  ok &= expect(missingProjection.lineCount == 0U, "missing projection line count");

  iggy3d::DebugProjectionResult emptyDebug;
  const iggy3d::ProductNpcBehaviorDebugHud unavailable =
      iggy3d::buildProductNpcBehaviorDebugHud(&emptyDebug, true, true, true);
  ok &= expect(!unavailable.visible, "empty npc lines hides npc hud");
  ok &= expect(unavailable.debugAvailable, "empty projection available");
  ok &= expect(unavailable.status == "npc_debug_unavailable",
               "empty npc status");
  ok &= expect(unavailable.reasonCode == "npc_debug_unavailable",
               "empty npc reason");
  ok &= expect(unavailable.lineCount == 0U, "empty npc line count");
  ok &= expect(unavailable.lines.empty(), "empty npc emits no lines");

  const iggy3d::ProductNpcBehaviorDebugHud visible =
      iggy3d::buildProductNpcBehaviorDebugHud(&debug, true, true, true);
  ok &= expect(visible.visible, "npc hud visible");
  ok &= expect(visible.developerToolsEnabled, "visible dev tools proof");
  ok &= expect(visible.debugOverlayEnabled, "visible overlay proof");
  ok &= expect(visible.debugAvailable, "visible debug available");
  ok &= expect(visible.status == "npc_debug_ready", "visible status");
  ok &= expect(visible.reasonCode == "npc_debug_ready", "visible reason");
  ok &= expect(visible.lineCount == 3U, "visible line count");
  ok &= expect(visible.lines.size() == 3U, "visible emitted lines");
  ok &= expect(visibleLine(visible,
                           0,
                           "NPCS world=2 ai=2 resolved=1 failed=1 hostile=1 passive=0",
                           iggy3d::ProductFeedbackTone::Neutral),
               "summary neutral");
  ok &= expect(visibleLine(visible,
                           1,
                           "NPC 2 training_dummy default attacking/attack_target "
                           "tgt=player cd=2",
                           iggy3d::ProductFeedbackTone::Neutral),
               "normal row neutral");
  ok &= expect(visibleLine(visible,
                           2,
                           "NPC 3 ghost ghost_profile idle/none tgt=none cd=0 "
                           "unresolved=profile_missing",
                           iggy3d::ProductFeedbackTone::Warn),
               "unresolved row warn");

  if (!ok) {
    return 1;
  }
  std::cout << "product_npc_behavior_debug_hud_tests=pass\n";
  return 0;
}
