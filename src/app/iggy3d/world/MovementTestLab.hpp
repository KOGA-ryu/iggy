#pragma once

#include <string_view>
#include <vector>

#include "app/iggy3d/ascii_room/AsciiRoomGrid.hpp"
#include "runtime/save/SaveEnvelope.hpp"

namespace iggy3d {

struct ProductMovementTestLabBuildConfig {
  float tileSizeMeters = 1.0F;
  bool centerOnOrigin = true;
  std::int32_t storyIndex = 0;
  bool enabled = false;
};

bool productMovementTestLabRoomId(std::string_view roomId);

std::vector<SaveAuthoredRoomObjectRecord> buildProductMovementTestLabObjects(
    const AsciiRoomGrid& grid,
    const ProductMovementTestLabBuildConfig& config = {});

}  // namespace iggy3d
