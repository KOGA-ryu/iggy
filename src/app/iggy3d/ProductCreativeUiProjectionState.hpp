#pragma once

#include <cstdint>
#include <string>

namespace iggy3d {

// Owned creative-UI-projection mirror state (product window view) -- extracted from the
// ProductAppWindowState god-struct (docs/appkernel_build_map_v0_1.md, L2). Lives app-side (not
// creative/, a separate lane). Behavior-identical.
struct ProductCreativeUiProjectionState {
  bool requested = false;
  bool ready = false;
  bool partial = false;
  std::string status = "creative_ui_projection_not_requested";
  std::string reasonCode = "creative_ui_projection_not_requested";
  bool usedModel = false;
  bool usedFacade = false;
  std::uint32_t virtualWidth = 0;
  std::uint32_t virtualHeight = 0;
  std::string theme = "none";
  std::uint64_t panelCount = 0;
  std::uint64_t modelRowCount = 0;
  std::uint64_t primitiveCount = 0;
  std::uint64_t textCount = 0;
  std::uint64_t rectCount = 0;
  std::uint64_t rowCount = 0;
  std::uint64_t disabledRowCount = 0;
  std::uint64_t hitRegionCount = 0;
};

}  // namespace iggy3d
