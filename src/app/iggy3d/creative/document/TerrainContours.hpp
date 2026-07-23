#pragma once

#include "app/iggy3d/creative/document/TerrainField.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

inline constexpr std::size_t kCreativeTerrainContourSegmentCapacity = 4096U;
inline constexpr std::uint16_t kCreativeTerrainContourMaximumIntervalCells =
    16U;
inline constexpr std::uint16_t kCreativeTerrainContourMaximumMajorEvery = 16U;
inline constexpr std::size_t kCreativeTerrainContourLabelCapacity =
    kCreativeTerrainMaximumHeightCells;
inline constexpr std::size_t kCreativeTerrainAnalysisCellCapacity =
    kCreativeTerrainRenderPatchCapacity;

struct CreativeTerrainContourPoint {
  double x = 0.0;
  double z = 0.0;

  [[nodiscard]] friend constexpr bool operator==(
      CreativeTerrainContourPoint,
      CreativeTerrainContourPoint) noexcept = default;
};

struct CreativeTerrainContourSegment {
  CreativeTerrainContourPoint start{};
  CreativeTerrainContourPoint end{};
  std::uint16_t levelCells = 0U;
  bool major = false;

  [[nodiscard]] friend constexpr bool operator==(
      CreativeTerrainContourSegment,
      CreativeTerrainContourSegment) noexcept = default;
};

enum class CreativeTerrainContourPlanStatus : std::uint8_t {
  NotRequested,
  InvalidSurface,
  InvalidRequest,
  CapacityExceeded,
  Empty,
  Ready,
};

struct CreativeTerrainContourRequest {
  std::uint16_t intervalCells = 2U;
  std::uint16_t majorEvery = 5U;
  std::size_t maxSegmentCount = kCreativeTerrainContourSegmentCapacity;
};

struct CreativeTerrainContourPlan {
  bool requested = false;
  bool accepted = false;
  CreativeTerrainContourPlanStatus status =
      CreativeTerrainContourPlanStatus::NotRequested;
  std::uint64_t sourceRevision = 0U;
  std::uint64_t sourceColumnCount = 0U;
  std::uint64_t evaluatedSquareCount = 0U;
  std::uint64_t contourLevelCount = 0U;
  std::uint64_t ambiguousCaseCount = 0U;
  std::vector<CreativeTerrainContourSegment> segments;
  std::string_view reasonCode = "creative_terrain_contours_not_requested";
};

struct CreativeTerrainContourLabel {
  CreativeTerrainContourPoint point{};
  std::uint16_t levelCells = 0U;

  [[nodiscard]] friend constexpr bool operator==(
      CreativeTerrainContourLabel,
      CreativeTerrainContourLabel) noexcept = default;
};

enum class CreativeTerrainSlopeBand : std::uint8_t {
  Unavailable,
  Flat,
  Gentle,
  Steep,
  Extreme,
  Count,
};

enum class CreativeTerrainCutFillKind : std::uint8_t {
  Unchanged,
  Cut,
  Fill,
  Count,
};

struct CreativeTerrainAnalysisCell {
  CreativeTerrainCoord2 coord{};
  bool terrainPresent = false;
  bool referencePresent = false;
  std::uint16_t heightCells = 0U;
  std::uint16_t referenceHeightCells = 0U;
  std::int16_t deltaCells = 0;
  double slopeXCellsPerCell = 0.0;
  double slopeZCellsPerCell = 0.0;
  double slopeDegrees = 0.0;
  std::uint8_t neighborSampleCount = 0U;
  CreativeTerrainSlopeBand slopeBand = CreativeTerrainSlopeBand::Unavailable;
  CreativeTerrainCutFillKind cutFill =
      CreativeTerrainCutFillKind::Unchanged;

  [[nodiscard]] friend constexpr bool operator==(
      CreativeTerrainAnalysisCell,
      CreativeTerrainAnalysisCell) noexcept = default;
};

enum class CreativeTerrainAnalysisPlanStatus : std::uint8_t {
  NotRequested,
  InvalidSurface,
  InvalidReference,
  InvalidRequest,
  CapacityExceeded,
  Empty,
  Ready,
};

struct CreativeTerrainAnalysisRequest {
  CreativeTerrainContourRequest contours{};
  double flatMaximumDegrees = 5.0;
  double gentleMaximumDegrees = 15.0;
  double steepMaximumDegrees = 30.0;
  std::size_t maxCellCount = kCreativeTerrainAnalysisCellCapacity;
  std::size_t maxLabelCount = kCreativeTerrainContourLabelCapacity;
};

struct CreativeTerrainAnalysisPlan {
  bool requested = false;
  bool accepted = false;
  CreativeTerrainAnalysisPlanStatus status =
      CreativeTerrainAnalysisPlanStatus::NotRequested;
  std::uint64_t sourceRevision = 0U;
  std::uint64_t referenceRevision = 0U;
  bool hasReference = false;
  std::uint16_t minimumHeightCells = 0U;
  std::uint16_t maximumHeightCells = 0U;
  double maximumSlopeDegrees = 0.0;
  std::uint64_t cutCellCount = 0U;
  std::uint64_t fillCellCount = 0U;
  CreativeTerrainContourPlan contours;
  std::vector<CreativeTerrainContourLabel> labels;
  std::vector<CreativeTerrainAnalysisCell> cells;
  std::string_view reasonCode = "creative_terrain_analysis_not_requested";
};

enum class CreativeTerrainAnalysisHitKind : std::uint8_t {
  None,
  Contour,
  HeightHandle,
  Count,
};

enum class CreativeTerrainAnalysisHitMode : std::uint8_t {
  ContourOnly,
  HeightHandleOnly,
  ContourThenHeightHandle,
  Count,
};

struct CreativeTerrainAnalysisHit {
  bool requested = false;
  bool accepted = false;
  CreativeTerrainAnalysisHitKind kind = CreativeTerrainAnalysisHitKind::None;
  CreativeTerrainCoord2 coord{};
  CreativeTerrainContourPoint point{};
  std::uint16_t targetHeightCells = 0U;
  std::size_t contourSegmentIndex = 0U;
  double distanceCells = 0.0;
  std::string_view reasonCode = "creative_terrain_analysis_hit_not_requested";
};

[[nodiscard]] std::string_view toString(
    CreativeTerrainContourPlanStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainSlopeBand band) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainCutFillKind kind) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainAnalysisPlanStatus status) noexcept;

// Marching squares over adjacent terrain-column centers. A contour at level N
// lies at N - 0.5 cells, avoiding equality ambiguity for quantized heights.
// Squares touching a terrain hole are omitted rather than bridged.
[[nodiscard]] CreativeTerrainContourPlan buildCreativeTerrainContourPlan(
    const CreativeTerrainSurfacePlan& surface,
    const CreativeTerrainContourRequest& request = {});

// Produces bounded drafting analysis from canonical composed terrain. The
// optional reference is used only for signed cut/fill deltas; slope and contour
// truth always come from the source surface. Major labels choose the longest
// segment at each index level, making placement deterministic and uncluttered.
[[nodiscard]] CreativeTerrainAnalysisPlan buildCreativeTerrainAnalysisPlan(
    const CreativeTerrainSurfacePlan& surface,
    const CreativeTerrainAnalysisRequest& request = {},
    const CreativeTerrainSurfacePlan* reference = nullptr);

// Hit mode makes contour-vs-height intent explicit. ContourThenHeightHandle
// gives contours precedence inside tolerance and otherwise falls back to the
// containing terrain cell.
[[nodiscard]] CreativeTerrainAnalysisHit hitCreativeTerrainAnalysis(
    const CreativeTerrainAnalysisPlan& plan,
    CreativeTerrainContourPoint point,
    double contourToleranceCells,
    CreativeTerrainAnalysisHitMode mode =
        CreativeTerrainAnalysisHitMode::ContourThenHeightHandle) noexcept;

}  // namespace iggy3d::creative
