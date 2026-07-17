#pragma once

#include <cstdint>

#include "EditorPlacement.hpp"
#include "core/spatial/AabbGridIndex.hpp"

namespace iggy3d_creative_app {

struct CreativePlacementClearanceCache {
  iggy3d::creative::CreativeDocumentId documentId =
      iggy3d::creative::kInvalidDocumentId;
  std::uint64_t documentRevision = 0;
  std::uint64_t rebuildCount = 0;
  std::uint64_t indexedObjectCount = 0;
  iggy3d::AabbGridIndex authoredObstacleIndex;
  bool complete = false;
  bool valid = false;
};

[[nodiscard]] bool refreshCreativePlacementClearanceCache(
    CreativePlacementClearanceCache& cache,
    const iggy3d::creative::CreativeDocument& document);
void invalidateCreativePlacementClearanceCache(
    CreativePlacementClearanceCache& cache) noexcept;

[[nodiscard]] iggy3d::creative::CreativePlacementClearanceResult
evaluateCreativeBrushPlacementClearance(
    const iggy3d::creative::CreativeDocument& document,
    const CreativeBrushPlacementPlan& plan,
    const CreativePlacementClearanceCache* cache = nullptr) noexcept;

void applyCreativeBrushPlacementClearance(
    CreativeBrushPlacementAdmission& admission,
    const iggy3d::creative::CreativeDocument& document,
    const CreativePlacementClearanceCache* cache = nullptr) noexcept;

}  // namespace iggy3d_creative_app
