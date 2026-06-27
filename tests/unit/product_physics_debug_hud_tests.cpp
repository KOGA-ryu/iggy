#include "app/iggy3d/ProductPhysicsDebugHud.hpp"

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

iggy3d::DebugProjectionResult physicsDebugProjection() {
  iggy3d::DebugProjectionResult debug;
  debug.physicsDebugHudLines.push_back(
      "PHYS packets=2 failed=0 bodies=6 colliders=7 contacts=2 sensors=1");
  debug.physicsDebugHudLines.push_back(
      "PHYS BP cells=3 entries=12 bucket=4 candidates=8 tested=5 dup=1 overlaps=2");
  debug.physicsDebugHudLines.push_back(
      "PHYS SOLVE plans=2 pos=1 vel=2 fric=1 pen=0.125 "
      "ni=3.500/2.250 fi=1.000/0.750");
  debug.physicsDebugHudLines.push_back(
      "PHYS MOVE kin_iter=3 kin_hits=1 player_iter=2 player_hits=1 baked=5 skipped=2");
  return debug;
}

iggy3d::DebugProjectionResult physicsWarningProjection() {
  iggy3d::DebugProjectionResult debug = physicsDebugProjection();
  debug.physicsDebugHudLines.push_back(
      "PHYS WARN status=physics_debug_snapshot_stats_failed "
      "upstream=physics_frame_stats_nonfinite_scalar bp=1 pen=1 impulse=1");
  return debug;
}

bool visibleLine(const iggy3d::ProductPhysicsDebugHud& hud,
                 std::size_t index,
                 const char* text,
                 iggy3d::ProductFeedbackTone tone) {
  return index < hud.lines.size() && hud.lines[index].visible &&
         hud.lines[index].text == text && hud.lines[index].tone == tone;
}

}  // namespace

int main() {
  bool ok = true;
  const iggy3d::DebugProjectionResult debug = physicsDebugProjection();
  const iggy3d::DebugProjectionResult warningDebug = physicsWarningProjection();

  const iggy3d::ProductPhysicsDebugHud inactive =
      iggy3d::buildProductPhysicsDebugHud(&debug, false, true, true);
  ok &= expect(!inactive.visible, "inactive gameplay hides physics hud");
  ok &= expect(inactive.status == "not_requested", "inactive status");
  ok &= expect(inactive.reasonCode == "not_requested", "inactive reason");
  ok &= expect(inactive.debugAvailable, "inactive debug available");
  ok &= expect(inactive.lineCount == 4U, "inactive line count");
  ok &= expect(inactive.lines.empty(), "inactive emits no lines");

  const iggy3d::ProductPhysicsDebugHud devDisabled =
      iggy3d::buildProductPhysicsDebugHud(&debug, true, false, true);
  ok &= expect(!devDisabled.visible, "developer tools disabled hides physics hud");
  ok &= expect(!devDisabled.developerToolsEnabled, "dev disabled proof");
  ok &= expect(devDisabled.status == "not_requested", "dev disabled status");
  ok &= expect(devDisabled.lineCount == 4U, "dev disabled line count");

  const iggy3d::ProductPhysicsDebugHud overlayDisabled =
      iggy3d::buildProductPhysicsDebugHud(&debug, true, true, false);
  ok &= expect(!overlayDisabled.visible, "debug overlay disabled hides physics hud");
  ok &= expect(!overlayDisabled.debugOverlayEnabled, "overlay disabled proof");
  ok &= expect(overlayDisabled.status == "not_requested", "overlay disabled status");
  ok &= expect(overlayDisabled.lineCount == 4U, "overlay disabled line count");

  const iggy3d::ProductPhysicsDebugHud missingProjection =
      iggy3d::buildProductPhysicsDebugHud(nullptr, true, true, true);
  ok &= expect(!missingProjection.visible, "missing projection hides physics hud");
  ok &= expect(!missingProjection.debugAvailable, "missing projection proof");
  ok &= expect(missingProjection.status == "projection_missing",
               "missing projection status");
  ok &= expect(missingProjection.reasonCode == "projection_missing",
               "missing projection reason");
  ok &= expect(missingProjection.lineCount == 0U, "missing projection line count");
  ok &= expect(!missingProjection.hasWarnings, "missing projection no warnings");

  iggy3d::DebugProjectionResult emptyDebug;
  const iggy3d::ProductPhysicsDebugHud unavailable =
      iggy3d::buildProductPhysicsDebugHud(&emptyDebug, true, true, true);
  ok &= expect(!unavailable.visible, "empty physics lines hides physics hud");
  ok &= expect(unavailable.debugAvailable, "empty projection available");
  ok &= expect(unavailable.status == "physics_debug_unavailable",
               "empty physics status");
  ok &= expect(unavailable.reasonCode == "physics_debug_unavailable",
               "empty physics reason");
  ok &= expect(unavailable.lineCount == 0U, "empty physics line count");
  ok &= expect(unavailable.lines.empty(), "empty physics emits no lines");

  const iggy3d::ProductPhysicsDebugHud visible =
      iggy3d::buildProductPhysicsDebugHud(&debug, true, true, true);
  ok &= expect(visible.visible, "physics hud visible");
  ok &= expect(visible.developerToolsEnabled, "visible dev tools proof");
  ok &= expect(visible.debugOverlayEnabled, "visible overlay proof");
  ok &= expect(visible.debugAvailable, "visible debug available");
  ok &= expect(visible.status == "physics_debug_ready", "visible status");
  ok &= expect(visible.reasonCode == "physics_debug_ready", "visible reason");
  ok &= expect(visible.lineCount == 4U, "visible line count");
  ok &= expect(visible.lines.size() == 4U, "visible emitted lines");
  ok &= expect(!visible.hasWarnings, "visible normal no warnings");
  ok &= expect(visibleLine(visible,
                           0,
                           "PHYS packets=2 failed=0 bodies=6 colliders=7 contacts=2 sensors=1",
                           iggy3d::ProductFeedbackTone::Neutral),
               "summary neutral");
  ok &= expect(visibleLine(visible,
                           1,
                           "PHYS BP cells=3 entries=12 bucket=4 candidates=8 tested=5 dup=1 overlaps=2",
                           iggy3d::ProductFeedbackTone::Neutral),
               "broadphase neutral");
  ok &= expect(visibleLine(visible,
                           2,
                           "PHYS SOLVE plans=2 pos=1 vel=2 fric=1 pen=0.125 "
                           "ni=3.500/2.250 fi=1.000/0.750",
                           iggy3d::ProductFeedbackTone::Neutral),
               "solver neutral");
  ok &= expect(visibleLine(visible,
                           3,
                           "PHYS MOVE kin_iter=3 kin_hits=1 player_iter=2 player_hits=1 baked=5 skipped=2",
                           iggy3d::ProductFeedbackTone::Neutral),
               "movement neutral");

  const iggy3d::ProductPhysicsDebugHud warning =
      iggy3d::buildProductPhysicsDebugHud(&warningDebug, true, true, true);
  ok &= expect(warning.visible, "warning hud visible");
  ok &= expect(warning.hasWarnings, "warning hud has warnings");
  ok &= expect(warning.lineCount == 5U, "warning line count");
  ok &= expect(warning.lines.size() == 5U, "warning emitted line count");
  ok &= expect(visibleLine(warning,
                           4,
                           "PHYS WARN status=physics_debug_snapshot_stats_failed "
                           "upstream=physics_frame_stats_nonfinite_scalar bp=1 pen=1 impulse=1",
                           iggy3d::ProductFeedbackTone::Warn),
               "warning line warn tone");

  iggy3d::DebugProjectionResult statusFailureDebug = debug;
  statusFailureDebug.physicsDebugHudLines.push_back(
      "PHYS status=physics_debug_snapshot_stats_failed diagnostics");
  const iggy3d::ProductPhysicsDebugHud statusFailure =
      iggy3d::buildProductPhysicsDebugHud(&statusFailureDebug, true, true, true);
  ok &= expect(statusFailure.hasWarnings, "status failed line has warnings");
  ok &= expect(statusFailure.lines.back().tone == iggy3d::ProductFeedbackTone::Warn,
               "status failed line warn tone");

  const iggy3d::ProductPhysicsDebugHud hiddenWarning =
      iggy3d::buildProductPhysicsDebugHud(&warningDebug, false, true, true);
  ok &= expect(!hiddenWarning.visible, "hidden warning hud hidden");
  ok &= expect(hiddenWarning.hasWarnings, "hidden warning still tracked");
  ok &= expect(hiddenWarning.lineCount == 5U, "hidden warning line count");
  ok &= expect(hiddenWarning.lines.empty(), "hidden warning emits no lines");

  if (!ok) {
    return 1;
  }
  std::cout << "product_physics_debug_hud_tests=pass\n";
  return 0;
}
