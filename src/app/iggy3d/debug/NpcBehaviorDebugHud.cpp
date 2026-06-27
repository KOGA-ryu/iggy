#include "app/iggy3d/debug/NpcBehaviorDebugHud.hpp"

#include <string>
#include <string_view>

#include "projection/debug/DebugProjection.hpp"

namespace iggy3d {
namespace {

ProductFeedbackTone toneForLine(std::string_view line) {
  if (line.find("unresolved=") != std::string_view::npos) {
    return ProductFeedbackTone::Warn;
  }
  return ProductFeedbackTone::Neutral;
}

void copyVisibleLines(NpcBehaviorDebugHud& hud,
                      const DebugProjectionResult& debug) {
  hud.lines.reserve(debug.npcBehaviorDebugHudLines.size());
  for (const std::string& line : debug.npcBehaviorDebugHudLines) {
    hud.lines.push_back(NpcBehaviorDebugHudLine{
        line, toneForLine(line), hud.visible});
  }
}

}  // namespace

NpcBehaviorDebugHud buildNpcBehaviorDebugHud(
    const DebugProjectionResult* debug,
    bool gameplayActive,
    bool developerToolsEnabled,
    bool debugOverlayEnabled) {
  NpcBehaviorDebugHud hud;
  hud.developerToolsEnabled = developerToolsEnabled;
  hud.debugOverlayEnabled = debugOverlayEnabled;
  hud.debugAvailable = debug != nullptr;

  if (debug != nullptr) {
    hud.lineCount = debug->npcBehaviorDebugHudLines.size();
  }

  if (!gameplayActive) {
    hud.status = "not_requested";
    hud.reasonCode = hud.status;
    return hud;
  }
  if (!developerToolsEnabled || !debugOverlayEnabled) {
    hud.status = "not_requested";
    hud.reasonCode = hud.status;
    return hud;
  }
  if (debug == nullptr) {
    hud.status = "projection_missing";
    hud.reasonCode = hud.status;
    return hud;
  }
  if (debug->npcBehaviorDebugHudLines.empty()) {
    hud.status = "npc_debug_unavailable";
    hud.reasonCode = hud.status;
    return hud;
  }

  hud.visible = true;
  hud.status = "npc_debug_ready";
  hud.reasonCode = hud.status;
  copyVisibleLines(hud, *debug);
  return hud;
}

}  // namespace iggy3d
