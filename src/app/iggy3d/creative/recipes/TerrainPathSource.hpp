#pragma once

#include "app/iggy3d/creative/document/TerrainHeightField.hpp"
#include "app/iggy3d/creative/document/TerrainMaterialField.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/tools/TerrainPath.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

using CreativeTerrainPathSourcePointId = std::uint32_t;

inline constexpr CreativeTerrainPathSourcePointId
    kInvalidCreativeTerrainPathSourcePointId = 0U;
inline constexpr std::uint32_t kCreativeTerrainPathSourceVersion = 3U;
inline constexpr std::uint16_t kCreativeTerrainPathSourceMaximumHalfWidthCells =
    16U;
inline constexpr std::uint16_t kCreativeTerrainPathSourceMaximumFalloffCells =
    16U;
inline constexpr std::int32_t kCreativeTerrainPathSourceMaximumBankPermille =
    1000;
inline constexpr std::uint16_t kCreativeTerrainRoadMaximumShoulderWidthCells =
    8U;
inline constexpr std::uint16_t kCreativeTerrainRoadMaximumGradePermille =
    1000U;
inline constexpr double kCreativeTerrainRoadMinimumEdgeDimensionMeters = 0.01;
inline constexpr double kCreativeTerrainRoadMaximumEdgeDimensionMeters = 2.0;
inline constexpr std::uint16_t
    kCreativeTerrainWatercourseMaximumBankSlopeCells = 16U;
inline constexpr std::uint16_t
    kCreativeTerrainWatercourseMaximumSurfaceInsetCells = 16U;
inline constexpr std::uint16_t
    kCreativeTerrainWatercourseMaximumCrossingClearanceCells = 16U;
inline constexpr std::size_t kCreativeTerrainWatercourseCrossingCapacity = 32U;
inline constexpr std::size_t
    kCreativeTerrainPathGeneratedCenterlineCapacity =
        kCreativeTerrainHeightFieldCellCapacity;

enum class CreativeTerrainPathCurvePolicy : std::uint8_t {
  Linear,
  CatmullRom,
  Count,
};

enum class CreativeTerrainPathCrossSection : std::uint8_t {
  Flat,
  Crowned,
  Channel,
  Berm,
  Cut,
  Count,
};

enum class CreativeTerrainPathEndpointJoin : std::uint8_t {
  Open,
  Blend,
  Intersection,
  Bridge,
  BuildingPad,
  Count,
};

enum class CreativeTerrainRoadEdgeTreatment : std::uint8_t {
  None,
  Curb,
  Count,
};

struct CreativeTerrainRoadSettings {
  // Authored point half-width is the travel surface. Shoulders extend the
  // graded and painted corridor on both sides without changing that profile.
  std::uint16_t shoulderWidthCells = 0U;
  // Zero keeps pre-v2 sources behavior-compatible: no authored grade cap.
  std::uint16_t maximumGradePermille = 0U;
  CreativeTerrainRoadEdgeTreatment edgeTreatment =
      CreativeTerrainRoadEdgeTreatment::None;
  double edgeWidthMeters = 0.15;
  double edgeHeightMeters = 0.15;
  CreativeStructuralMaterial edgeMaterial = CreativeStructuralMaterial::Stone;

  [[nodiscard]] friend constexpr bool operator==(
      CreativeTerrainRoadSettings,
      CreativeTerrainRoadSettings) noexcept = default;
};

enum class CreativeTerrainWatercourseDrainageDirection : std::uint8_t {
  Unspecified,
  StartToEnd,
  EndToStart,
  Count,
};

enum class CreativeTerrainWaterSurfacePolicy : std::uint8_t {
  None,
  Reserved,
  Count,
};

using CreativeTerrainWatercourseCrossingId = std::uint32_t;

inline constexpr CreativeTerrainWatercourseCrossingId
    kInvalidCreativeTerrainWatercourseCrossingId = 0U;

struct CreativeTerrainWatercourseCrossing {
  CreativeTerrainWatercourseCrossingId id =
      kInvalidCreativeTerrainWatercourseCrossingId;
  CreativeTerrainPathSourcePointId pointId =
      kInvalidCreativeTerrainPathSourcePointId;
  std::uint16_t bankClearanceCells = 1U;
  std::uint16_t deckClearanceCells = 1U;
  std::uint16_t approachLengthCells = 2U;

  [[nodiscard]] friend constexpr bool operator==(
      CreativeTerrainWatercourseCrossing,
      CreativeTerrainWatercourseCrossing) noexcept = default;
};

struct CreativeTerrainWatercourseSettings {
  // Zero preserves the pre-v3 bowl/cut profile. Positive values treat the
  // authored point half-width as the flat bed and grade outward to the bank.
  std::uint16_t bankSlopeCells = 0U;
  CreativeTerrainWatercourseDrainageDirection drainageDirection =
      CreativeTerrainWatercourseDrainageDirection::Unspecified;
  // Reserved records future water intent and elevation without generating a
  // renderable, collider, or simulated water object in this recipe version.
  CreativeTerrainWaterSurfacePolicy surfacePolicy =
      CreativeTerrainWaterSurfacePolicy::None;
  std::uint16_t surfaceInsetCells = 1U;
  CreativeTerrainWatercourseCrossingId nextCrossingId = 1U;
  std::vector<CreativeTerrainWatercourseCrossing> crossings;

  [[nodiscard]] friend bool operator==(
      const CreativeTerrainWatercourseSettings&,
      const CreativeTerrainWatercourseSettings&) noexcept = default;
};

struct CreativeTerrainPathSourcePoint {
  CreativeTerrainPathSourcePointId id =
      kInvalidCreativeTerrainPathSourcePointId;
  CreativeTerrainCoord2 coord{};
  std::uint16_t heightCells = 4U;
  std::uint16_t halfWidthCells = 1U;
  std::uint16_t amplitudeCells = 0U;
  std::int32_t bankPermille = 0;

  [[nodiscard]] friend constexpr bool operator==(
      CreativeTerrainPathSourcePoint,
      CreativeTerrainPathSourcePoint) noexcept = default;
};

// One durable Road/River/Ridge/Trench source. Point ids survive insertion,
// deletion, and reordering; vector order is the authored traversal order.
struct CreativeTerrainPathSourceRecipe {
  std::uint32_t version = kCreativeTerrainPathSourceVersion;
  CreativeTerrainPathKind kind = CreativeTerrainPathKind::Road;
  CreativeTerrainPathElevation elevation = CreativeTerrainPathElevation::Grade;
  CreativeTerrainPathCurvePolicy curve =
      CreativeTerrainPathCurvePolicy::Linear;
  CreativeTerrainPathCrossSection crossSection =
      CreativeTerrainPathCrossSection::Flat;
  CreativeTerrainPathEndpointJoin startJoin =
      CreativeTerrainPathEndpointJoin::Open;
  CreativeTerrainPathEndpointJoin endJoin =
      CreativeTerrainPathEndpointJoin::Open;
  std::uint16_t falloffCells = 2U;
  bool paintSurface = true;
  CreativeTerrainMaterial material = CreativeTerrainMaterial::Dirt;
  CreativeTerrainRoadSettings road{};
  CreativeTerrainWatercourseSettings watercourse{};
  CreativeTerrainPathSourcePointId nextPointId = 1U;
  std::vector<CreativeTerrainPathSourcePoint> points;

  [[nodiscard]] friend bool operator==(
      const CreativeTerrainPathSourceRecipe&,
      const CreativeTerrainPathSourceRecipe&) noexcept = default;
};

struct CreativeTerrainPathSegmentReceipt {
  CreativeTerrainPathSourcePointId startPointId =
      kInvalidCreativeTerrainPathSourcePointId;
  CreativeTerrainPathSourcePointId endPointId =
      kInvalidCreativeTerrainPathSourcePointId;
  CreativeTerrainHeightFieldBounds impactBounds{};
  std::uint64_t sourceHash = 0U;
  std::uint32_t centerlineCellCount = 0U;
  std::uint32_t generatedCellCount = 0U;
};

struct CreativeTerrainPathDirtySegments {
  bool changed = false;
  bool allSegments = false;
  std::size_t firstSegment = 0U;
  std::size_t segmentCount = 0U;

  [[nodiscard]] friend constexpr bool operator==(
      CreativeTerrainPathDirtySegments,
      CreativeTerrainPathDirtySegments) noexcept = default;
};

// Transient sampled geometry for one durable source segment. These records are
// deliberately separate from document state: callers may retain them between
// previews, but save/load continues to persist only the authored recipe.
struct CreativeTerrainPathSourceSample {
  CreativeTerrainCoord2 coord{};
  std::size_t segmentIndex = 0U;
  double progress = 0.0;
  double heightCells = 0.0;
  // Authored flat travel/bed half-width before shoulders or bank slopes.
  double profileHalfWidthCells = 0.0;
  double halfWidthCells = 0.0;
  double amplitudeCells = 0.0;
  double bankPermille = 0.0;
  double terrainWeight = 1.0;
  double profileWeight = 1.0;
  std::int32_t tangentX = 1;
  std::int32_t tangentZ = 0;

  [[nodiscard]] friend constexpr bool operator==(
      CreativeTerrainPathSourceSample,
      CreativeTerrainPathSourceSample) noexcept = default;
};

struct CreativeTerrainPathSourceSegmentCache {
  CreativeTerrainPathSourcePointId startPointId =
      kInvalidCreativeTerrainPathSourcePointId;
  CreativeTerrainPathSourcePointId endPointId =
      kInvalidCreativeTerrainPathSourcePointId;
  std::uint64_t sourceHash = 0U;
  std::vector<CreativeTerrainPathSourceSample> samples;

  [[nodiscard]] friend bool operator==(
      const CreativeTerrainPathSourceSegmentCache&,
      const CreativeTerrainPathSourceSegmentCache&) noexcept = default;
};

struct CreativeTerrainPathSourceCache {
  bool valid = false;
  CreativeTerrainPathSourceRecipe recipe{};
  CreativeTerrainPathDirtySegments dirtySegments{};
  std::uint64_t rebuiltSegmentCount = 0U;
  std::uint64_t reusedSegmentCount = 0U;
  std::uint64_t generatedControlCount = 0U;
  std::vector<CreativeTerrainPathSourceSegmentCache> segments;

  [[nodiscard]] friend bool operator==(
      const CreativeTerrainPathSourceCache&,
      const CreativeTerrainPathSourceCache&) noexcept = default;
};

enum class CreativeTerrainPathSourceStatus : std::uint8_t {
  NotRequested,
  UnsupportedVersion,
  InvalidRecipe,
  InvalidSource,
  CoordinateOverflow,
  CapacityExceeded,
  HeightOutOfRange,
  MaterialRejected,
  OutputRejected,
  Ready,
};

struct CreativeTerrainPathSourceSamplingResult {
  bool accepted = false;
  CreativeTerrainPathSourceStatus status =
      CreativeTerrainPathSourceStatus::NotRequested;
  std::vector<CreativeTerrainPathSourceSample> samples;
  std::vector<CreativeTerrainPathSegmentReceipt> segments;
  CreativeTerrainPathSourceCache cache;
  std::string_view reasonCode =
      "creative_terrain_path_source_sampling_not_requested";
};

struct CreativeTerrainPathSourceReceipt {
  bool requested = false;
  bool accepted = false;
  CreativeTerrainPathSourceStatus status =
      CreativeTerrainPathSourceStatus::NotRequested;
  std::uint64_t sourceColumnCount = 0U;
  std::uint64_t segmentCount = 0U;
  std::uint64_t centerlineCellCount = 0U;
  std::uint64_t generatedControlCount = 0U;
  std::uint64_t rebuiltSegmentCount = 0U;
  std::uint64_t reusedSegmentCount = 0U;
  std::uint64_t evaluatedCellCount = 0U;
  std::uint64_t modifiedCellCount = 0U;
  std::uint64_t materialEditCount = 0U;
  std::uint64_t outputCellCount = 0U;
  std::uint64_t outputHeightHash = 0U;
  std::uint64_t recipeHash = 0U;
  std::string_view reasonCode = "creative_terrain_path_source_not_requested";
};

struct CreativeTerrainPathSourceResult {
  CreativeTerrainHeightField heightField;
  std::vector<CreativeTerrainMaterialEdit> materialEdits;
  std::vector<CreativeTerrainPathSegmentReceipt> segments;
  // Joined, endpoint-adjusted samples used by terrain generation. This is
  // transient output for structural road recipes and previews, never save data.
  std::vector<CreativeTerrainPathSourceSample> samples;
  CreativeTerrainPathSourceReceipt receipt{};
};

[[nodiscard]] bool isValidCreativeTerrainPathSourceRecipe(
    const CreativeTerrainPathSourceRecipe& recipe) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainPathCurvePolicy value) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainPathCrossSection value) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainPathEndpointJoin value) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainRoadEdgeTreatment value) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainWatercourseDrainageDirection value) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainWaterSurfacePolicy value) noexcept;
[[nodiscard]] bool parseCreativeTerrainPathKind(
    std::string_view text,
    CreativeTerrainPathKind& output) noexcept;
[[nodiscard]] bool parseCreativeTerrainPathElevation(
    std::string_view text,
    CreativeTerrainPathElevation& output) noexcept;
[[nodiscard]] bool parseCreativeTerrainPathCurvePolicy(
    std::string_view text,
    CreativeTerrainPathCurvePolicy& output) noexcept;
[[nodiscard]] bool parseCreativeTerrainPathCrossSection(
    std::string_view text,
    CreativeTerrainPathCrossSection& output) noexcept;
[[nodiscard]] bool parseCreativeTerrainPathEndpointJoin(
    std::string_view text,
    CreativeTerrainPathEndpointJoin& output) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainPathSourceStatus value) noexcept;
[[nodiscard]] std::uint64_t hashCreativeTerrainPathSourceRecipe(
    const CreativeTerrainPathSourceRecipe& recipe) noexcept;

// Returns the minimal source-segment interval that must be regenerated.
// Neighboring segments are included because Catmull-Rom tangents span them.
[[nodiscard]] CreativeTerrainPathDirtySegments
diffCreativeTerrainPathSourceSegments(
    const CreativeTerrainPathSourceRecipe& before,
    const CreativeTerrainPathSourceRecipe& after) noexcept;

// Samples authored geometry without evaluating the dense terrain field. Road
// structure generation consumes this exact joined centerline, so visual edge
// members and terrain replay cannot drift onto separate spline kernels.
[[nodiscard]] CreativeTerrainPathSourceSamplingResult
sampleCreativeTerrainPathSourceRecipe(
    const CreativeTerrainPathSourceRecipe& recipe,
    const CreativeTerrainPathSourceCache* previousCache = nullptr);

// O(h * r^2 + c log c), where h is the bounded output heightfield cell count,
// r is at most 32 cells, and c is at most 8192 centerline cells. Ordering is
// row-major and every failure returns zero material edits and no partial field.
[[nodiscard]] CreativeTerrainPathSourceResult
buildCreativeTerrainPathSourceRecipe(
    const CreativeTerrainHeightField& existingAuthored,
    const CreativeTerrainSurfacePlan& canonicalSource,
    const CreativeTerrainMaterialField& materialSource,
    const CreativeTerrainPathSourceRecipe& recipe,
    CreativeTerrainPathSourceCache* cache = nullptr);

}  // namespace iggy3d::creative
