#pragma once

#include "app/iggy3d/creative/document/TerrainHeightField.hpp"
#include "app/iggy3d/creative/document/TerrainMaterialField.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

inline constexpr std::uint32_t kCreativeTerrainStampVersion = 2U;
inline constexpr std::uint32_t kCreativeTerrainStampRecipeVersion = 1U;
inline constexpr std::size_t kCreativeTerrainStampCellCapacity =
    kCreativeTerrainHeightFieldCellCapacity;
inline constexpr std::size_t kCreativeTerrainStampAssetIdCapacity = 128U;
inline constexpr std::size_t kCreativeTerrainStampLabelCapacity = 64U;
inline constexpr std::int16_t kCreativeTerrainStampMinimumHeightOffsetCells =
    -63;
inline constexpr std::int16_t kCreativeTerrainStampMaximumHeightOffsetCells =
    63;

enum class CreativeTerrainStampMode : std::uint8_t {
  Merge,
  Replace,
  Count,
};

enum class CreativeTerrainStampElevationMode : std::uint8_t {
  Absolute,
  Surface,
  Count,
};

enum class CreativeTerrainStampCopyStatus : std::uint8_t {
  NotRequested,
  InvalidSource,
  InvalidBounds,
  InvalidIdentity,
  CapacityExceeded,
  EmptyRegion,
  Copied,
};

// A stamp is an explicit baked snapshot. Source identity/version is retained
// for diagnostics and future relinking, but replay never silently reads a
// mutable external asset. Heights and materials are source-local row-major
// cells, so generated, painted, and manually authored terrain copy exactly.
struct CreativeTerrainStamp {
  std::uint32_t version = kCreativeTerrainStampVersion;
  std::string assetId;
  std::string label;
  std::uint64_t assetVersion = 1U;
  std::uint64_t sourceDocumentId = 0U;
  std::uint64_t sourceRevision = 0U;
  std::uint64_t contentSignature = 0U;
  CreativeTerrainCoord2 sourceMinimum{};
  std::uint16_t widthCells = 0U;
  std::uint16_t depthCells = 0U;
  std::uint16_t minimumHeightCells = 0U;
  std::vector<std::uint16_t> heights;
  std::vector<CreativeTerrainMaterialWeights> materials;

  [[nodiscard]] std::size_t cellCount() const noexcept {
    return heights.size();
  }

  [[nodiscard]] friend bool operator==(
      const CreativeTerrainStamp&,
      const CreativeTerrainStamp&) noexcept = default;
};

struct CreativeTerrainStampCopyReceipt {
  bool requested = false;
  bool accepted = false;
  CreativeTerrainStampCopyStatus status =
      CreativeTerrainStampCopyStatus::NotRequested;
  CreativeTerrainCoord2 minimumCoord{};
  CreativeTerrainCoord2 maximumCoord{};
  std::uint64_t copiedCellCount = 0U;
  std::uint64_t copiedPresentCellCount = 0U;
  std::uint64_t copiedMaterialCellCount = 0U;
  std::uint16_t minimumHeightCells = 0U;
  std::uint64_t contentSignature = 0U;
  std::string_view reasonCode = "creative_terrain_stamp_copy_not_requested";
};

enum class CreativeTerrainStampPlanStatus : std::uint8_t {
  NotRequested,
  InvalidStamp,
  InvalidDestination,
  InvalidRequest,
  CoordinateOverflow,
  HeightOutOfRange,
  CapacityExceeded,
  OutputRejected,
  NoChange,
  Ready,
};

// Durable terrain-operation payload. The source is baked deliberately; the
// asset id/version records where it came from without making replay depend on
// the current catalog contents.
struct CreativeTerrainStampRecipe {
  std::uint32_t version = kCreativeTerrainStampRecipeVersion;
  CreativeTerrainStamp stamp;
  CreativeTerrainCoord2 targetMinimum{};
  std::uint8_t quarterTurns = 0U;
  bool mirrorX = false;
  bool mirrorZ = false;
  CreativeTerrainStampMode mode = CreativeTerrainStampMode::Merge;
  CreativeTerrainStampElevationMode elevationMode =
      CreativeTerrainStampElevationMode::Surface;
  std::int16_t manualHeightOffsetCells = 0;

  [[nodiscard]] friend bool operator==(
      const CreativeTerrainStampRecipe&,
      const CreativeTerrainStampRecipe&) noexcept = default;
};

using CreativeTerrainStampRequest = CreativeTerrainStampRecipe;

struct CreativeTerrainStampPlan {
  bool requested = false;
  bool accepted = false;
  CreativeTerrainStampPlanStatus status =
      CreativeTerrainStampPlanStatus::NotRequested;
  CreativeTerrainStampRecipe recipe{};
  bool targetSurfacePresent = false;
  std::uint16_t targetSurfaceHeightCells = 0U;
  std::int32_t appliedHeightOffsetCells = 0;
  CreativeTerrainCoord2 targetMinimum{};
  CreativeTerrainCoord2 targetMaximum{};
  std::uint16_t transformedWidthCells = 0U;
  std::uint16_t transformedDepthCells = 0U;
  std::uint64_t affectedCellCount = 0U;
  std::uint64_t presentStampCellCount = 0U;
  std::uint64_t changedHeightCellCount = 0U;
  std::uint64_t changedMaterialCellCount = 0U;
  CreativeTerrainHeightField heightField;
  CreativeTerrainMaterialField materialField;
  std::string_view reasonCode = "creative_terrain_stamp_not_requested";
};

[[nodiscard]] std::string_view toString(
    CreativeTerrainStampMode mode) noexcept;
[[nodiscard]] bool parseCreativeTerrainStampMode(
    std::string_view value,
    CreativeTerrainStampMode& output) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainStampElevationMode mode) noexcept;
[[nodiscard]] bool parseCreativeTerrainStampElevationMode(
    std::string_view value,
    CreativeTerrainStampElevationMode& output) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainStampCopyStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainStampPlanStatus status) noexcept;

[[nodiscard]] std::uint64_t creativeTerrainStampContentSignature(
    const CreativeTerrainStamp& stamp) noexcept;
[[nodiscard]] bool isValidCreativeTerrainStamp(
    const CreativeTerrainStamp& stamp) noexcept;
[[nodiscard]] bool isValidCreativeTerrainStampRecipe(
    const CreativeTerrainStampRecipe& recipe) noexcept;
[[nodiscard]] bool creativeTerrainStampEmpty(
    const CreativeTerrainStamp& stamp) noexcept;
void clearCreativeTerrainStamp(CreativeTerrainStamp& stamp) noexcept;

// Captures the exact visible surface inside inclusive bounds. Empty cells are
// retained as holes and every present cell records its exact material weights.
// Rejection is transactional and leaves outStamp untouched.
[[nodiscard]] CreativeTerrainStampCopyReceipt copyCreativeTerrainRegionToStamp(
    std::uint64_t sourceDocumentId,
    std::uint64_t sourceRevision,
    const CreativeTerrainSurfacePlan& sourceSurface,
    const CreativeTerrainMaterialField& sourceMaterials,
    CreativeTerrainCoord2 minimumCoord,
    CreativeTerrainCoord2 maximumCoord,
    std::string_view assetId,
    std::string_view label,
    std::uint64_t assetVersion,
    CreativeTerrainStamp& outStamp);

// O((D + S) log D), bounded by 8192 destination cells and 8192 stamp cells.
// Rotation and mirrors are exact integer-lattice transforms. Surface elevation
// aligns the copied minimum present height to the destination sample; Absolute
// preserves source heights. Merge writes only present stamp cells. Replace
// also preserves stamp holes by clearing destination terrain in the footprint.
// The returned height/material fields are the immutable preview/commit truth.
[[nodiscard]] CreativeTerrainStampPlan buildCreativeTerrainStampPlan(
    const CreativeTerrainHeightField& destinationHeight,
    const CreativeTerrainMaterialField& destinationMaterial,
    const CreativeTerrainSurfacePlan& destinationSurface,
    const CreativeTerrainStampRequest& request);

}  // namespace iggy3d::creative
