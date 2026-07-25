#pragma once

#include <cstddef>
#include <string>

#include "app/iggy3d/creative/recipes/TerrainOperation.hpp"

namespace iggy3d_creative_app {

struct CreativeDesktopTerrainOperationPayload {
  iggy3d::creative::CreativeTerrainOperationId operationId =
      iggy3d::creative::kInvalidCreativeTerrainOperationId;
  bool enabled = true;
  std::size_t targetIndex = 0U;
};

struct CreativeDesktopTerrainStampPayload {
  std::string assetId;
  std::string label;
  iggy3d::creative::CreativeTerrainOperationId operationId =
      iggy3d::creative::kInvalidCreativeTerrainOperationId;
  bool replaceExisting = false;
};

}  // namespace iggy3d_creative_app
