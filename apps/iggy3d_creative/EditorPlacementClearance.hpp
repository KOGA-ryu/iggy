#pragma once

#include <cstdint>
#include <span>

#include "EditorPlacement.hpp"
#include "core/spatial/AabbGridIndex.hpp"

namespace iggy3d {

struct StaticMeshAssetCatalog;

}  // namespace iggy3d

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

struct CreativeObjectSetClearanceResult {
  iggy3d::creative::CreativePlacementClearanceResult clearance{};
  iggy3d::creative::CreativeObjectId candidateObjectId =
      iggy3d::creative::kInvalidObjectId;
  std::uint64_t candidateObjectCount = 0U;
};

// One ownership rule for authored geometry that participates in placement
// clearance. Pattern previews and ordinary brush placement must classify the
// same objects as blockers.
[[nodiscard]] bool creativeObjectBlocksPlacementClearance(
    const iggy3d::creative::CreativeObject& object) noexcept;

[[nodiscard]] bool refreshCreativePlacementClearanceCache(
    CreativePlacementClearanceCache& cache,
    const iggy3d::creative::CreativeDocument& document);
void invalidateCreativePlacementClearanceCache(
    CreativePlacementClearanceCache& cache) noexcept;

[[nodiscard]] iggy3d::creative::CreativePlacementClearanceResult
evaluateCreativeBrushPlacementClearance(
    const iggy3d::creative::CreativeDocument& document,
    const CreativeBrushPlacementPlan& plan,
    const CreativePlacementClearanceCache* cache = nullptr,
    std::span<const iggy3d::creative::CreativeObjectId> ignoredObjectIds = {})
    noexcept;

// Evaluates already-transformed candidate objects without re-planning their
// geometry. ignoredObjectIds are the original source objects for a move; copy
// requests pass an empty span so overlapping the source is rejected.
[[nodiscard]] CreativeObjectSetClearanceResult
evaluateCreativeObjectSetPlacementClearance(
    const iggy3d::creative::CreativeDocument& document,
    std::span<const iggy3d::creative::CreativeObject> candidates,
    std::span<const iggy3d::creative::CreativeObjectId> ignoredObjectIds = {},
    const CreativePlacementClearanceCache* cache = nullptr) noexcept;

// Attachment hosts may have hollow compound collision (for example a door
// frame), so host validation must use catalog collision parts rather than the
// host's aggregate visual bounds. Exact face contact is accepted; penetration
// into any source/host collision part fails closed.
[[nodiscard]] CreativeObjectSetClearanceResult
evaluateCreativeAttachmentPlacementClearance(
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::StaticMeshAssetCatalog& assetCatalog,
    std::span<const iggy3d::creative::CreativeObject> candidates,
    std::span<const iggy3d::creative::CreativeObjectId> sourceHierarchyIds,
    iggy3d::creative::CreativeObjectId sourceObjectId,
    iggy3d::creative::CreativeObjectId targetObjectId,
    const CreativePlacementClearanceCache* cache = nullptr);

void applyCreativeBrushPlacementClearance(
    CreativeBrushPlacementAdmission& admission,
    const iggy3d::creative::CreativeDocument& document,
    const CreativePlacementClearanceCache* cache = nullptr,
    std::span<const iggy3d::creative::CreativeObjectId> ignoredObjectIds = {})
    noexcept;

}  // namespace iggy3d_creative_app
