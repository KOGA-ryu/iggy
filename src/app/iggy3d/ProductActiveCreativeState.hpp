#pragma once

#include <cstdint>
#include <string>

namespace iggy3d {

// Owned active-creative-document/save mirror state (product window view) -- extracted from the
// ProductAppWindowState god-struct (docs/appkernel_build_map_v0_1.md, L2). Lives app-side (not
// creative/, a separate lane). Behavior-identical.
struct ProductActiveCreativeState {
  std::string saveId = "none";
  std::string savePath = "none";
  std::string worldId = "none";
  std::uint64_t documentId = 0;
  std::uint64_t objectCount = 0;
  std::uint64_t nextObjectId = 0;
  std::string saveStatus = "creative_world_save_not_requested";
  std::string saveReasonCode = "creative_world_save_not_requested";
  std::uint64_t saveDirtyFlagsBefore = 0;
  std::uint64_t saveDirtyFlagsDrained = 0;
  std::uint64_t saveDirtyFlagsAfter = 0;
  std::string saveSavedAtUtc = "none";
};

}  // namespace iggy3d
