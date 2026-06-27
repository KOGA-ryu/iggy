#include "app/iggy3d/ProductPhysicsDebugHud.hpp"

#include <string>
#include <string_view>

#include "projection/debug/DebugProjection.hpp"

namespace iggy3d {
namespace {

ProductFeedbackTone toneForLine(std::string_view line) {
  const bool warningLine = line.starts_with("PHYS WARN") ||
                           line.find("status=physics_debug_snapshot_stats_failed") !=
                               std::string_view::npos;
  // branch-gate: BG-1108
  if (warningLine) {
    return ProductFeedbackTone::Warn;
  }
  return ProductFeedbackTone::Neutral;
}

bool hasWarningLine(const DebugProjectionResult& debug) {
  for (const std::string& line : debug.physicsDebugHudLines) {
    // branch-gate: BG-1108
    if (toneForLine(line) == ProductFeedbackTone::Warn) {
      return true;
    }
  }
  return false;
}

void copyVisibleLines(ProductPhysicsDebugHud& hud,
                      const DebugProjectionResult& debug) {
  hud.lines.reserve(debug.physicsDebugHudLines.size());
  for (const std::string& line : debug.physicsDebugHudLines) {
    hud.lines.push_back(
        ProductPhysicsDebugHudLine{line, toneForLine(line), hud.visible});
  }
}

}  // namespace

ProductPhysicsDebugHud buildProductPhysicsDebugHud(
    const DebugProjectionResult* debug,
    bool gameplayActive,
    bool developerToolsEnabled,
    bool debugOverlayEnabled) {
  ProductPhysicsDebugHud hud;
  hud.developerToolsEnabled = developerToolsEnabled;
  hud.debugOverlayEnabled = debugOverlayEnabled;
  hud.debugAvailable = debug != nullptr;

  // branch-gate: BG-1108
  if (debug != nullptr) {
    hud.lineCount = debug->physicsDebugHudLines.size();
    hud.hasWarnings = hasWarningLine(*debug);
  }

  // branch-gate: BG-1108
  if (!gameplayActive) {
    hud.status = "not_requested";
    hud.reasonCode = hud.status;
    return hud;
  }
  // branch-gate: BG-1108
  if (!developerToolsEnabled || !debugOverlayEnabled) {
    hud.status = "not_requested";
    hud.reasonCode = hud.status;
    return hud;
  }
  // branch-gate: BG-1108
  if (debug == nullptr) {
    hud.status = "projection_missing";
    hud.reasonCode = hud.status;
    return hud;
  }
  // branch-gate: BG-1108
  if (debug->physicsDebugHudLines.empty()) {
    hud.status = "physics_debug_unavailable";
    hud.reasonCode = hud.status;
    return hud;
  }

  hud.visible = true;
  hud.status = "physics_debug_ready";
  hud.reasonCode = hud.status;
  copyVisibleLines(hud, *debug);
  return hud;
}

}  // namespace iggy3d
