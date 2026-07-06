#pragma once

#include <cstdint>
#include <string>

namespace iggy3d {

// Owned creative-UI input mirror state (product window view) -- extracted from the
// ProductAppWindowState god-struct (docs/appkernel_build_map_v0_1.md, L2). App-side (not creative/,
// a separate lane). Behavior-identical.
struct ProductCreativeUiInputState {
  bool requested = false;
  bool clickPresent = false;
  bool drawListAvailable = false;
  bool routed = false;
  bool hit = false;
  bool consumed = false;
  bool enabled = false;
  std::string surface = "none";
  std::string kind = "none";
  std::string action = "none";
  std::uint64_t layerIndex = 0;
  std::uint64_t regionIndex = 0;
  std::string semanticId = "none";
  std::string status = "creative_ui_input_not_requested";
  std::string reasonCode = "creative_ui_input_not_requested";
  bool downstreamClickRequested = false;
  bool downstreamClickPresent = false;
  bool downstreamClickHigherPriority = false;
  bool downstreamClickSuppressed = false;
  std::string downstreamClickStatus = "creative_ui_input_downstream_click_not_requested";
  std::string downstreamClickReasonCode = "creative_ui_input_downstream_click_not_requested";
};

}  // namespace iggy3d
