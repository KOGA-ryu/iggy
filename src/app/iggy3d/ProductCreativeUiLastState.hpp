#pragma once

#include <cstdint>
#include <string>

namespace iggy3d {

// Owned creative-UI last-interaction mirror state (product window view) -- extracted from the
// ProductAppWindowState god-struct (docs/appkernel_build_map_v0_1.md, L2). Lives app-side (not
// creative/, a separate lane). Behavior-identical.
struct ProductCreativeUiLastState {
  bool clickSeen = false;
  std::string clickX = "none";
  std::string clickY = "none";
  bool inputHit = false;
  bool inputConsumed = false;
  std::string inputStatus = "none";
  std::string inputSemanticId = "none";
  std::string commandKind = "none";
  std::string commandStatus = "none";
  bool commandCreateRequested = false;
  bool commandCreateAccepted = false;
  bool commandCreateChanged = false;
  std::uint64_t commandCreateObjectId = 0;
};

}  // namespace iggy3d
