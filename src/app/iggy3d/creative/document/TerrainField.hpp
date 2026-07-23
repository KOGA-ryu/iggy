#pragma once

#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/document/VoxelField.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

class CreativeTerrainMaterialField;

inline constexpr std::size_t kCreativeTerrainControlCapacity = 256U;
inline constexpr std::size_t kCreativeTerrainRenderPatchCapacity = 8192U;
inline constexpr std::size_t kCreativeTerrainHardEdgeCapacity =
    kCreativeTerrainRenderPatchCapacity * 4U;
inline constexpr std::uint16_t kCreativeTerrainMinimumHeightCells = 1U;
inline constexpr std::uint16_t kCreativeTerrainMaximumHeightCells = 64U;
inline constexpr std::uint16_t kCreativeTerrainMinimumRadiusCells = 1U;
inline constexpr std::uint16_t kCreativeTerrainMaximumRadiusCells = 16U;

struct CreativeTerrainCoord2 {
  std::int32_t x = 0;
  std::int32_t z = 0;

  [[nodiscard]] friend constexpr bool operator==(
      CreativeTerrainCoord2,
      CreativeTerrainCoord2) noexcept = default;
};

// Canonical topology seam between two cardinally adjacent terrain cells. The
// endpoints are sorted by Z then X, making a bounded vector of these edges a
// deterministic set. Height samples stay independent: this record says that
// the shared boundary must not be smoothed.
struct CreativeTerrainHardEdge {
  CreativeTerrainCoord2 first{};
  CreativeTerrainCoord2 second{};

  [[nodiscard]] friend constexpr bool operator==(
      CreativeTerrainHardEdge,
      CreativeTerrainHardEdge) noexcept = default;
};

struct CreativeTerrainColumn;

[[nodiscard]] CreativeTerrainHardEdge canonicalCreativeTerrainHardEdge(
    CreativeTerrainCoord2 first,
    CreativeTerrainCoord2 second) noexcept;
[[nodiscard]] bool isValidCreativeTerrainHardEdge(
    CreativeTerrainHardEdge edge) noexcept;
[[nodiscard]] bool validateCreativeTerrainHardEdges(
    std::span<const CreativeTerrainHardEdge> edges) noexcept;
[[nodiscard]] bool validateCreativeTerrainHardEdgesForSurface(
    std::span<const CreativeTerrainColumn> columns,
    std::span<const CreativeTerrainHardEdge> edges) noexcept;

struct CreativeTerrainPatchRegion {
  CreativeTerrainCoord2 minimum{};
  CreativeTerrainCoord2 maximum{};

  [[nodiscard]] friend constexpr bool operator==(
      CreativeTerrainPatchRegion,
      CreativeTerrainPatchRegion) noexcept = default;
};

enum class CreativeTerrainMaterial : std::uint8_t {
  Grass,
  Dirt,
  Stone,
  Sand,
  Count,
};

struct CreativeTerrainControlPoint {
  CreativeTerrainCoord2 coord{};
  std::uint16_t heightCells = 4U;
  std::uint16_t radiusCells = 4U;

  [[nodiscard]] friend constexpr bool operator==(
      CreativeTerrainControlPoint,
      CreativeTerrainControlPoint) noexcept = default;
};

enum class CreativeTerrainEditKind : std::uint8_t {
  Upsert,
  Remove,
  Count,
};

struct CreativeTerrainControlEdit {
  CreativeTerrainEditKind kind = CreativeTerrainEditKind::Upsert;
  CreativeTerrainControlPoint control{};
};

enum class CreativeTerrainMutationStatus : std::uint8_t {
  NotRequested,
  InvalidField,
  InvalidEdit,
  DuplicateCoordinate,
  CapacityExceeded,
  NoChange,
  Applied,
};

struct CreativeTerrainMutationReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeTerrainMutationStatus status =
      CreativeTerrainMutationStatus::NotRequested;
  std::uint64_t revisionBefore = 0;
  std::uint64_t revisionAfter = 0;
  std::uint64_t attemptedEditCount = 0;
  std::uint64_t controlCountBefore = 0;
  std::uint64_t controlCountAfter = 0;
  std::uint64_t changedControlCount = 0;
  std::string_view reasonCode = "creative_terrain_mutation_not_requested";
};

class CreativeTerrainField {
 public:
  [[nodiscard]] bool isValid() const noexcept;
  [[nodiscard]] bool validateInvariants() const noexcept;
  [[nodiscard]] std::uint64_t revision() const noexcept;
  [[nodiscard]] std::uint64_t controlCount() const noexcept;
  [[nodiscard]] std::span<const CreativeTerrainControlPoint> controls()
      const noexcept;
  [[nodiscard]] const CreativeTerrainControlPoint* controlAt(
      CreativeTerrainCoord2 coord) const noexcept;

  [[nodiscard]] CreativeTerrainMutationReceipt apply(
      std::span<const CreativeTerrainControlEdit> edits);
  void clear() noexcept;

 private:
  std::vector<CreativeTerrainControlPoint> controls_;
  std::uint64_t revision_ = 0;
  bool valid_ = true;
};

struct CreativeTerrainColumn {
  CreativeTerrainCoord2 coord{};
  std::uint16_t heightCells = 0;

  [[nodiscard]] friend constexpr bool operator==(
      CreativeTerrainColumn,
      CreativeTerrainColumn) noexcept = default;
};

struct CreativeTerrainHeightSample {
  bool present = false;
  CreativeTerrainCoord2 coord{};
  std::uint16_t heightCells = 0;
  std::uint16_t contributingControlCount = 0;
  std::uint64_t totalWeight = 0;
};

enum class CreativeTerrainRaycastStatus : std::uint8_t {
  NotRequested,
  InvalidField,
  InvalidRequest,
  TraversalLimitExceeded,
  Miss,
  Hit,
};

struct CreativeTerrainRaycastRequest {
  CreativeVec3 rayOrigin{};
  CreativeVec3 rayDirection{};
  CreativeVec3 gridOrigin{};
  double cellSize = 1.0;
  double maxDistance = 256.0;
  std::uint32_t maxVisitedCells = 4096U;
};

struct CreativeTerrainRaycastReceipt {
  bool requested = false;
  bool accepted = false;
  bool hit = false;
  bool startInside = false;
  CreativeTerrainRaycastStatus status =
      CreativeTerrainRaycastStatus::NotRequested;
  CreativeTerrainCoord2 cell{};
  std::uint16_t heightCells = 0;
  CreativeVec3 hitPoint{};
  CreativeVec3 faceNormal{};
  double distance = 0.0;
  std::uint32_t visitedCellCount = 0;
  std::string_view reasonCode = "creative_terrain_raycast_not_requested";
};

enum class CreativeTerrainSurfacePlanStatus : std::uint8_t {
  NotRequested,
  InvalidField,
  Empty,
  Ready,
};

struct CreativeTerrainSurfacePlan {
  bool requested = false;
  bool accepted = false;
  CreativeTerrainSurfacePlanStatus status =
      CreativeTerrainSurfacePlanStatus::NotRequested;
  std::uint64_t sourceRevision = 0;
  std::uint64_t contributionCount = 0;
  std::vector<CreativeTerrainColumn> columns;
  std::vector<CreativeVoxelCuboid> cuboids;
  std::vector<CreativeTerrainHardEdge> hardEdges;
  std::string_view reasonCode = "creative_terrain_surface_not_requested";
};

struct CreativeTerrainSurfacePatch {
  CreativeTerrainCoord2 coord{};
  CreativeTerrainMaterial material = CreativeTerrainMaterial::Grass;
  CreativeVec3 center{};
  // Counter-clockwise from the minimum X/Z corner when viewed from above.
  std::array<CreativeVec3, 4U> corners{};
  std::array<std::uint8_t,
             static_cast<std::size_t>(CreativeTerrainMaterial::Count)>
      materialWeights{255U, 0U, 0U, 0U};
  CreativeVec3 materialColor{0.22, 0.52, 0.20};
  // South, east, north, west bits. A bit is owned only by the higher patch;
  // the lower neighboring patch supplies the bottom edge at render/bake time.
  std::uint8_t hardEdgeMask = 0U;
};

enum class CreativeTerrainRenderPlanStatus : std::uint8_t {
  NotRequested,
  InvalidField,
  InvalidRequest,
  SurfacePlanFailed,
  CapacityExceeded,
  ArithmeticOverflow,
  Empty,
  Ready,
};

struct CreativeTerrainRenderPlan {
  bool requested = false;
  bool accepted = false;
  CreativeTerrainRenderPlanStatus status =
      CreativeTerrainRenderPlanStatus::NotRequested;
  std::uint64_t sourceRevision = 0;
  std::uint64_t sourceMaterialRevision = 0;
  std::uint64_t sourceColumnCount = 0;
  std::vector<CreativeTerrainSurfacePatch> patches;
  std::string_view reasonCode = "creative_terrain_render_not_requested";
};

enum class CreativeTerrainMutationPreviewStatus : std::uint8_t {
  NotRequested,
  InvalidField,
  MutationRejected,
  RenderRejected,
  Ready,
};

struct CreativeTerrainMutationPreviewReceipt {
  bool requested = false;
  bool accepted = false;
  CreativeTerrainMutationPreviewStatus status =
      CreativeTerrainMutationPreviewStatus::NotRequested;
  CreativeTerrainMutationReceipt mutation{};
  CreativeTerrainRenderPlan render{};
  bool regionLimited = false;
  CreativeTerrainCoord2 regionMinimum{};
  CreativeTerrainCoord2 regionMaximum{};
  std::uint64_t sampledColumnCoordinateCount = 0U;
  std::uint64_t candidatePatchCoordinateCount = 0U;
  std::string_view reasonCode = "creative_terrain_preview_not_requested";
};

[[nodiscard]] bool isValidCreativeTerrainControlPoint(
    CreativeTerrainControlPoint control) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainMutationStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainSurfacePlanStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainRaycastStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainRenderPlanStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainMutationPreviewStatus status) noexcept;

[[nodiscard]] CreativeTerrainHeightSample sampleCreativeTerrainHeight(
    const CreativeTerrainField& field,
    CreativeTerrainCoord2 coord) noexcept;

// Bounded 2D grid DDA over derived vertical terrain columns. The direction
// need not be normalized. Sampling uses the same integer blend as the surface
// planner, so editor picking cannot drift from generated geometry.
[[nodiscard]] CreativeTerrainRaycastReceipt raycastCreativeTerrainField(
    const CreativeTerrainField& field,
    const CreativeTerrainRaycastRequest& request) noexcept;

// Each control contributes inside a compact circular influence disk. Overlap is
// blended with deterministic integer weights, so save/load and cross-platform
// rebuilds produce identical column heights. The generated cuboids are derived
// cache data; controls remain the authored truth.
[[nodiscard]] CreativeTerrainSurfacePlan buildCreativeTerrainSurfacePlan(
    const CreativeTerrainField& field);

// Builds a bounded visual height mesh from the deterministic column plan.
// Adjacent cells resolve shared corner heights from the same neighboring
// columns, preventing cracks without changing authored controls or collision.
[[nodiscard]] CreativeTerrainRenderPlan buildCreativeTerrainRenderPlan(
    const CreativeTerrainField& field,
    CreativeVec3 gridOrigin,
    double cellSize,
    std::size_t maxPatchCount = kCreativeTerrainRenderPatchCapacity);
[[nodiscard]] CreativeTerrainRenderPlan buildCreativeTerrainRenderPlan(
    const CreativeTerrainSurfacePlan& surface,
    CreativeVec3 gridOrigin,
    double cellSize,
    std::size_t maxPatchCount = kCreativeTerrainRenderPatchCapacity);
[[nodiscard]] CreativeTerrainRenderPlan buildCreativeTerrainRenderPlan(
    const CreativeTerrainSurfacePlan& surface,
    const CreativeTerrainMaterialField& materials,
    CreativeVec3 gridOrigin,
    double cellSize,
    std::size_t maxPatchCount = kCreativeTerrainRenderPatchCapacity);

// Exact bounded render of only the inclusive patch region. Support columns one
// cell beyond the region are sampled so boundary corner heights match a full
// terrain render, while work remains proportional to the requested area.
[[nodiscard]] CreativeTerrainRenderPlan buildCreativeTerrainRenderPlan(
    const CreativeTerrainField& field,
    CreativeTerrainPatchRegion region,
    CreativeVec3 gridOrigin,
    double cellSize,
    std::size_t maxPatchCount = kCreativeTerrainRenderPatchCapacity);

// Applies a proposed edit batch to a copy of the field and renders that copy.
// The source field is never mutated, so editor previews and commit plans can
// share one fail-closed apply-and-render contract.
[[nodiscard]] CreativeTerrainMutationPreviewReceipt
buildCreativeTerrainMutationPreview(
    const CreativeTerrainField& field,
    std::span<const CreativeTerrainControlEdit> edits,
    CreativeVec3 gridOrigin,
    double cellSize,
    std::size_t maxPatchCount = kCreativeTerrainRenderPatchCapacity);

[[nodiscard]] CreativeTerrainMutationPreviewReceipt
buildCreativeTerrainMutationPreview(
    const CreativeTerrainField& field,
    std::span<const CreativeTerrainControlEdit> edits,
    CreativeTerrainPatchRegion region,
    CreativeVec3 gridOrigin,
    double cellSize,
    std::size_t maxPatchCount = kCreativeTerrainRenderPatchCapacity);

}  // namespace iggy3d::creative
