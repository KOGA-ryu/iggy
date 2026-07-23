#pragma once

#include "EditorDraftingStyle.hpp"

#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/tools/Select.hpp"
#include "app/iggy3d/creative/world/WorldLayoutPlanHitTest.hpp"
#include "app/iggy3d/creative/world/WorldLayoutPlanProjection.hpp"
#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace iggy3d_creative_app {

struct CreativeEditorWorldLayoutState;
struct CreativeEditorWorldLayoutTopographyState;
struct CreativeEditorWorldLayoutSelection;
enum class CreativeEditorWorldLayoutInspectionSourceKind : std::uint8_t;
enum class CreativeEditorWorldLayoutPreviewValidity : std::uint8_t;

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
  bool lowerLevelContextVisible = true;
  bool upperLevelContextVisible = false;
  bool roofOverheadVisible = true;
  std::uint64_t inspectionContentRevision = 0U;
  CreativeEditorWorldLayoutInspectionSourceKind inspectionSourceKind;
  CreativeEditorWorldLayoutPreviewValidity previewValidity;
  bool volatileSource = false;

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
           lhs.lowerLevelContextVisible == rhs.lowerLevelContextVisible &&
           lhs.upperLevelContextVisible == rhs.upperLevelContextVisible &&
           lhs.roofOverheadVisible == rhs.roofOverheadVisible &&
           lhs.inspectionContentRevision ==
               rhs.inspectionContentRevision &&
           lhs.inspectionSourceKind == rhs.inspectionSourceKind &&
           lhs.previewValidity == rhs.previewValidity &&
           lhs.volatileSource == rhs.volatileSource;
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
  std::size_t sourceLevelIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  iggy3d::creative::CreativeWorldLayoutPlanRole role =
      iggy3d::creative::CreativeWorldLayoutPlanRole::Count;
  double distanceCells = 0.0;
  std::size_t testedPrimitiveCount = 0U;
};

inline constexpr std::size_t kCreativeEditorWorldLayoutPlanHitCapacity = 64U;

struct CreativeEditorWorldLayoutPlanHitStack {
  std::array<CreativeEditorWorldLayoutPlanHit,
             kCreativeEditorWorldLayoutPlanHitCapacity>
      items{};
  std::size_t count = 0U;
  std::size_t testedPrimitiveCount = 0U;
  std::size_t totalHitPrimitiveCount = 0U;
  bool truncated = false;
};

struct CreativeEditorWorldLayoutPlanRegionSelection {
  bool requested = false;
  bool accepted = false;
  bool overflowed = false;
  iggy3d::creative::CreativeWorldLayoutPlanRegionMode mode =
      iggy3d::creative::CreativeWorldLayoutPlanRegionMode::Window;
  std::size_t testedPrimitiveCount = 0U;
  std::size_t matchedPrimitiveCount = 0U;
  std::vector<iggy3d::creative::CreativeWorldLayoutSourceRef> sources;
  std::string_view reasonCode =
      "creative_editor_world_layout_plan_region_not_requested";
};

enum class CreativeEditorSelectionComposition : std::uint8_t {
  Replace,
  Add,
  Toggle,
  Count,
};

struct CreativeEditorObjectSelectionPlan {
  bool requested = false;
  bool accepted = false;
  bool overflowed = false;
  std::size_t sourceCount = 0U;
  std::vector<iggy3d::creative::CreativeObjectId> objectIds;
  iggy3d::creative::CreativeObjectId primaryObjectId =
      iggy3d::creative::kInvalidObjectId;
  std::string_view reasonCode =
      "creative_editor_object_selection_not_requested";
};

// Returns true only when a new semantic projection was built. Ordinary idle
// frames reuse the cached projection. Candidate layouts are intentionally
// rebuilt while an interactive room, transform, or template preview owns
// their data, because those transient candidates do not carry durable
// revisions.
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

// Returns distinct semantic sources in visible paint order, topmost first.
// Multiple glyph primitives owned by the same source occupy one cycle slot.
[[nodiscard]] CreativeEditorWorldLayoutPlanHitStack
hitCreativeEditorWorldLayoutPlanStack(
    const CreativeEditorWorldLayoutPlanViewCache& cache,
    const CreativeEditorWorldLayoutState& state,
    const iggy3d::creative::CreativeWorldLayout& layout,
    iggy3d::creative::CreativeWorldLayoutPlanPoint point,
    double toleranceCells) noexcept;

[[nodiscard]] CreativeEditorWorldLayoutPlanHit
cycleCreativeEditorWorldLayoutPlanHit(
    const CreativeEditorWorldLayoutPlanHitStack& stack,
    CreativeEditorWorldLayoutSelection currentSelection) noexcept;

// Selects deduplicated authored source scopes from the visible plan. A drag to
// the right uses Window semantics; a drag to the left uses Crossing semantics.
// Context layers and suppressed transient primitives are never admitted.
[[nodiscard]] CreativeEditorWorldLayoutPlanRegionSelection
selectCreativeEditorWorldLayoutPlanRegion(
    const CreativeEditorWorldLayoutPlanViewCache& cache,
    const CreativeEditorWorldLayoutState& state,
    const iggy3d::creative::CreativeWorldLayout& layout,
    iggy3d::creative::CreativeWorldLayoutPlanPoint first,
    iggy3d::creative::CreativeWorldLayoutPlanPoint second,
    double toleranceCells = 0.0,
    std::size_t sourceCapacity =
        iggy3d::creative::kCreativeSelectionTargetCapacity);

// Resolves source scopes to the generated document objects they own and then
// composes them with the authoritative object selection. Missing/stale inputs
// and capacity overflow fail atomically; hidden and locked objects remain
// eligible so the plan can reveal and inspect them.
[[nodiscard]] CreativeEditorObjectSelectionPlan
planCreativeEditorWorldLayoutObjectSelection(
    const CreativeEditorWorldLayoutState& state,
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::creative::CreativeSelectionState& currentSelection,
    std::span<const iggy3d::creative::CreativeWorldLayoutSourceRef> sources,
    CreativeEditorSelectionComposition composition,
    std::size_t objectCapacity =
        iggy3d::creative::kCreativeSelectionTargetCapacity);

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
