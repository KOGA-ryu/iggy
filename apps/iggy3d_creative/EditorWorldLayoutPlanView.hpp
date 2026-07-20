#pragma once

#include "EditorDraftingStyle.hpp"

#include "app/iggy3d/creative/world/WorldLayoutPlanProjection.hpp"

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace iggy3d_creative_app {

struct CreativeEditorWorldLayoutState;
struct CreativeEditorWorldLayoutTopographyState;

struct CreativeEditorWorldLayoutPlanViewKey {
  std::uint64_t sourceEpoch = 0U;
  std::uint64_t sourceRevision = 0U;
  std::size_t activeLevelIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::uint64_t topographyBuildCount = 0U;
  iggy3d::creative::CreativeVec3 gridOrigin;
  iggy3d::creative::CreativeGridSize3 gridSize;
  double gridCellSizeMeters = 0.0;
  bool topographyVisible = false;
  bool transientCandidate = false;

  [[nodiscard]] friend constexpr bool operator==(
      CreativeEditorWorldLayoutPlanViewKey lhs,
      CreativeEditorWorldLayoutPlanViewKey rhs) noexcept {
    return lhs.sourceEpoch == rhs.sourceEpoch &&
           lhs.sourceRevision == rhs.sourceRevision &&
           lhs.activeLevelIndex == rhs.activeLevelIndex &&
           lhs.topographyBuildCount == rhs.topographyBuildCount &&
           lhs.gridOrigin.x == rhs.gridOrigin.x &&
           lhs.gridOrigin.y == rhs.gridOrigin.y &&
           lhs.gridOrigin.z == rhs.gridOrigin.z &&
           lhs.gridSize.width == rhs.gridSize.width &&
           lhs.gridSize.height == rhs.gridSize.height &&
           lhs.gridSize.depth == rhs.gridSize.depth &&
           lhs.gridCellSizeMeters == rhs.gridCellSizeMeters &&
           lhs.topographyVisible == rhs.topographyVisible &&
           lhs.transientCandidate == rhs.transientCandidate;
  }
};

struct CreativeEditorWorldLayoutPlanViewCache {
  bool valid = false;
  CreativeEditorWorldLayoutPlanViewKey key;
  iggy3d::creative::CreativeWorldLayoutPlanProjection projection;
  std::vector<std::size_t> paintOrder;
  std::uint64_t buildCount = 0U;
};

struct CreativeEditorWorldLayoutPlanHit {
  bool hit = false;
  std::size_t primitiveIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  iggy3d::creative::CreativeWorldLayoutTable table =
      iggy3d::creative::CreativeWorldLayoutTable::None;
  std::size_t sourceIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  iggy3d::creative::CreativeWorldLayoutPlanRole role =
      iggy3d::creative::CreativeWorldLayoutPlanRole::Count;
  double distanceCells = 0.0;
  std::size_t testedPrimitiveCount = 0U;
};

// Returns true only when a new semantic projection was built. Ordinary idle
// frames reuse the cached projection. Candidate layouts are intentionally
// rebuilt while an interactive transform/template preview owns their data,
// because those transient candidates do not carry durable revisions.
[[nodiscard]] bool refreshCreativeEditorWorldLayoutPlanView(
    CreativeEditorWorldLayoutPlanViewCache& cache,
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutTopographyState& topography,
    iggy3d::creative::CreativeGridSettings grid);

void invalidateCreativeEditorWorldLayoutPlanView(
    CreativeEditorWorldLayoutPlanViewCache& cache) noexcept;

// Scans cached primitives in reverse paint order so the visible topmost active
// symbol wins. Context and overhead layers are presentation-only here. Source
// provenance, not reconstructed coordinates, determines the returned target.
[[nodiscard]] CreativeEditorWorldLayoutPlanHit
hitCreativeEditorWorldLayoutPlan(
    const CreativeEditorWorldLayoutPlanViewCache& cache,
    const CreativeEditorWorldLayoutState& state,
    const iggy3d::creative::CreativeWorldLayout& layout,
    iggy3d::creative::CreativeWorldLayoutPlanPoint point,
    double toleranceCells) noexcept;

// Exhaustive semantic adapter: projection roles and their metadata resolve to
// the visual table here, never in the ImGui renderer.
[[nodiscard]] CreativeEditorDraftingRole
creativeEditorWorldLayoutPlanDraftingRole(
    const iggy3d::creative::CreativeWorldLayoutPlanPrimitive& primitive)
    noexcept;

[[nodiscard]] bool creativeEditorWorldLayoutPlanPrimitiveSelected(
    const CreativeEditorWorldLayoutState& state,
    const iggy3d::creative::CreativeWorldLayoutPlanPrimitive& primitive)
    noexcept;

[[nodiscard]] std::size_t creativeEditorWorldLayoutPlanPrimitiveBuildingIndex(
    const iggy3d::creative::CreativeWorldLayout& layout,
    const iggy3d::creative::CreativeWorldLayoutPlanPrimitive& primitive)
    noexcept;

[[nodiscard]] std::pair<double, double>
creativeEditorWorldLayoutPlanPrimitiveOffset(
    const CreativeEditorWorldLayoutState& state,
    const iggy3d::creative::CreativeWorldLayout& layout,
    const iggy3d::creative::CreativeWorldLayoutPlanPrimitive& primitive)
    noexcept;

[[nodiscard]] bool creativeEditorWorldLayoutPlanPrimitiveSuppressed(
    const CreativeEditorWorldLayoutState& state,
    const iggy3d::creative::CreativeWorldLayout& layout,
    const iggy3d::creative::CreativeWorldLayoutPlanPrimitive& primitive)
    noexcept;

}  // namespace iggy3d_creative_app
