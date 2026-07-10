#include "app/iggy3d/debug/DebugHudPanels.hpp"

#include <array>
#include <string>
#include <string_view>

#include "projection/debug/DebugProjection.hpp"

namespace iggy3d {
namespace {

struct InteractionModeHudDescriptor {
  ProductInteractionMode mode;
  std::string_view label;
  FeedbackTone tone;
};

constexpr std::array<InteractionModeHudDescriptor, 2> kModeHudDescriptors{
    InteractionModeHudDescriptor{
        ProductInteractionMode::Player,
        "player",
        FeedbackTone::Neutral,
    },
    InteractionModeHudDescriptor{
        ProductInteractionMode::Creative,
        "creative",
        FeedbackTone::Warn,
    },
};

const InteractionModeHudDescriptor* interactionModeDescriptorFor(
    ProductInteractionMode mode) {
  for (const InteractionModeHudDescriptor& descriptor :
       kModeHudDescriptors) {
    // branch-gate: BG-1064
    if (descriptor.mode == mode) {
      return &descriptor;
    }
  }
  return nullptr;
}

FeedbackTone npcToneForLine(std::string_view line) {
  // branch-gate: BG-1223
  if (line.find("unresolved=") != std::string_view::npos) {
    return FeedbackTone::Warn;
  }
  return FeedbackTone::Neutral;
}

void copyVisibleNpcLines(NpcBehaviorDebugHud& hud,
                         const DebugProjectionResult& debug) {
  hud.lines.reserve(debug.npcBehaviorDebugHudLines.size());
  for (const std::string& line : debug.npcBehaviorDebugHudLines) {
    hud.lines.push_back(NpcBehaviorDebugHudLine{
        line, npcToneForLine(line), hud.visible});
  }
}

FeedbackTone physicsToneForLine(std::string_view line) {
  const bool warningLine = line.starts_with("PHYS WARN") ||
                           line.find("status=physics_debug_snapshot_stats_failed") !=
                               std::string_view::npos;
  // branch-gate: BG-1108
  if (warningLine) {
    return FeedbackTone::Warn;
  }
  return FeedbackTone::Neutral;
}

bool physicsHasWarningLine(const DebugProjectionResult& debug) {
  for (const std::string& line : debug.physicsDebugHudLines) {
    // branch-gate: BG-1108
    if (physicsToneForLine(line) == FeedbackTone::Warn) {
      return true;
    }
  }
  return false;
}

void copyVisiblePhysicsLines(PhysicsDebugHud& hud,
                             const DebugProjectionResult& debug) {
  hud.lines.reserve(debug.physicsDebugHudLines.size());
  for (const std::string& line : debug.physicsDebugHudLines) {
    hud.lines.push_back(
        PhysicsDebugHudLine{line, physicsToneForLine(line), hud.visible});
  }
}

}  // namespace

InteractionModeHud buildInteractionModeHud(
    InteractionModeHudRequest request) {
  InteractionModeHud hud;
  hud.roomEditingReady = request.roomEditingReady;

  const InteractionModeHudDescriptor* descriptor =
      interactionModeDescriptorFor(request.mode);
  // branch-gate: BG-1064
  if (descriptor == nullptr) {
    hud.status = "interaction_mode_hud_unknown_mode";
    hud.reasonCode = hud.status;
    hud.mode = "unknown";
    hud.label = "unknown";
    return hud;
  }

  hud.mode = std::string(productInteractionModeName(request.mode));
  hud.label = std::string(descriptor->label);
  hud.tone = descriptor->tone;

  // branch-gate: BG-1064
  if (!request.gameplayActive) {
    return hud;
  }

  hud.visible = true;
  hud.status = "interaction_mode_hud_ready";
  hud.reasonCode = hud.status;
  return hud;
}

NpcBehaviorDebugHud buildNpcBehaviorDebugHud(
    const DebugProjectionResult* debug,
    bool gameplayActive,
    bool developerToolsEnabled,
    bool debugOverlayEnabled) {
  NpcBehaviorDebugHud hud;
  hud.developerToolsEnabled = developerToolsEnabled;
  hud.debugOverlayEnabled = debugOverlayEnabled;
  hud.debugAvailable = debug != nullptr;

  // branch-gate: BG-1223
  if (debug != nullptr) {
    hud.lineCount = debug->npcBehaviorDebugHudLines.size();
  }

  // branch-gate: BG-1223
  if (!gameplayActive) {
    hud.status = "not_requested";
    hud.reasonCode = hud.status;
    return hud;
  }
  // branch-gate: BG-1223
  if (!developerToolsEnabled || !debugOverlayEnabled) {
    hud.status = "not_requested";
    hud.reasonCode = hud.status;
    return hud;
  }
  // branch-gate: BG-1223
  if (debug == nullptr) {
    hud.status = "projection_missing";
    hud.reasonCode = hud.status;
    return hud;
  }
  // branch-gate: BG-1223
  if (debug->npcBehaviorDebugHudLines.empty()) {
    hud.status = "npc_debug_unavailable";
    hud.reasonCode = hud.status;
    return hud;
  }

  hud.visible = true;
  hud.status = "npc_debug_ready";
  hud.reasonCode = hud.status;
  copyVisibleNpcLines(hud, *debug);
  return hud;
}

PhysicsDebugHud buildPhysicsDebugHud(
    const DebugProjectionResult* debug,
    bool gameplayActive,
    bool developerToolsEnabled,
    bool debugOverlayEnabled) {
  PhysicsDebugHud hud;
  hud.developerToolsEnabled = developerToolsEnabled;
  hud.debugOverlayEnabled = debugOverlayEnabled;
  hud.debugAvailable = debug != nullptr;

  // branch-gate: BG-1108
  if (debug != nullptr) {
    hud.lineCount = debug->physicsDebugHudLines.size();
    hud.hasWarnings = physicsHasWarningLine(*debug);
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
  copyVisiblePhysicsLines(hud, *debug);
  return hud;
}

}  // namespace iggy3d
