#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "app/iggy3d/gameplay/GameplayFeedback.hpp"

namespace iggy3d {

struct InteractionModeHud {
  bool visible = false;
  std::string status = "interaction_mode_hud_hidden";
  std::string reasonCode = "interaction_mode_hud_hidden";
  std::string mode = "player";
  std::string label = "player";
  FeedbackTone tone = FeedbackTone::Neutral;
  bool roomEditingReady = false;
};

struct NpcBehaviorDebugHudLine {
  std::string text;
  FeedbackTone tone = FeedbackTone::Neutral;
  bool visible = false;
};

struct NpcBehaviorDebugHud {
  bool visible = false;
  bool developerToolsEnabled = false;
  bool debugOverlayEnabled = false;
  bool debugAvailable = false;
  std::size_t lineCount = 0;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  std::vector<NpcBehaviorDebugHudLine> lines;
};

struct PhysicsDebugHudLine {
  std::string text;
  FeedbackTone tone = FeedbackTone::Neutral;
  bool visible = false;
};

struct PhysicsDebugHud {
  bool visible = false;
  bool developerToolsEnabled = false;
  bool debugOverlayEnabled = false;
  bool debugAvailable = false;
  std::size_t lineCount = 0U;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  bool hasWarnings = false;
  std::vector<PhysicsDebugHudLine> lines;
};

// Owned state for the top-down map debug overlay -- the first cluster extracted out of the
// ProductAppWindowState god-struct (docs/appkernel_build_map_v0_1.md, L2). A new top-down-map field
// now lands HERE, in its domain, instead of adding a flat field to the 615-member struct. Populated
// by copyTopDownMapOverlay (ProjectionRefresh), serialized by ReceiptBuilder. Behavior-identical.
struct ProductTopDownMapState {
  bool visible = false;
  std::string purpose = "hidden";
  std::string size = "hidden";
  std::string status = "top_down_map_hidden";
  std::string reasonCode = "top_down_map_hidden";
  std::uint64_t itemCount = 0;
};

// Owned dev collision-overlay state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: debug. Behavior-identical.
struct ProductDevCollisionOverlayState {
  bool visible = false;
  std::string status = "dev_collision_overlay_hidden";
  std::string reasonCode = "dev_collision_overlay_hidden";
};

// Owned NPC-behavior debug-HUD state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: debug. Behavior-identical.
struct ProductNpcBehaviorDebugHudState {
  bool visible = false;
  bool debugAvailable = false;
  std::uint64_t lineCount = 0;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  bool hasUnresolvedProfile = false;
};

}  // namespace iggy3d
