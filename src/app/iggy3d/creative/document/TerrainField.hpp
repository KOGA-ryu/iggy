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

inline constexpr std::size_t kCreativeTerrainControlCapacity = 256U;
inline constexpr std::size_t kCreativeTerrainRenderPatchCapacity = 8192U;
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
  std::string_view reasonCode = "creative_terrain_surface_not_requested";
};

struct CreativeTerrainSurfacePatch {
  CreativeTerrainCoord2 coord{};
  CreativeVec3 center{};
  // Counter-clockwise from the minimum X/Z corner when viewed from above.
  std::array<CreativeVec3, 4U> corners{};
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
  std::uint64_t sourceColumnCount = 0;
  std::vector<CreativeTerrainSurfacePatch> patches;
  std::string_view reasonCode = "creative_terrain_render_not_requested";
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

}  // namespace iggy3d::creative
