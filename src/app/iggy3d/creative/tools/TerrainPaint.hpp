#pragma once

#include "app/iggy3d/creative/document/TerrainMaterialField.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

enum class CreativeTerrainPaintRadius : std::uint8_t {
  OneCell,
  TwoCells,
  FourCells,
  EightCells,
  Count,
};

enum class CreativeTerrainPaintMode : std::uint8_t {
  Brush,
  Connected,
  Region,
  Count,
};

enum class CreativeTerrainPaintSource : std::uint8_t {
  Any,
  Grass,
  Dirt,
  Stone,
  Sand,
  Count,
};

enum class CreativeTerrainPaintHardness : std::uint8_t {
  Soft,
  Balanced,
  Firm,
  Solid,
  Count,
};

enum class CreativeTerrainPaintOpacity : std::uint8_t {
  Percent25,
  Percent50,
  Percent75,
  Percent100,
  Count,
};

enum class CreativeTerrainPaintMask : std::uint8_t {
  Circle,
  Square,
  Count,
};

enum class CreativeTerrainPaintBlend : std::uint8_t {
  Replace,
  Additive,
  Count,
};

enum class CreativeTerrainPaintSlopeFilter : std::uint8_t {
  Any,
  UpTo5Degrees,
  UpTo15Degrees,
  UpTo30Degrees,
  UpTo45Degrees,
  Above45Degrees,
  Count,
};

enum class CreativeTerrainPaintHeightFilter : std::uint8_t {
  Any,
  Cells1To8,
  Cells9To16,
  Cells17To32,
  Cells33To64,
  Count,
};

inline constexpr std::size_t kCreativeTerrainPaintBrushCellCapacity = 256U;
inline constexpr std::size_t kCreativeTerrainPaintCellCapacity =
    kCreativeTerrainMaterialOverrideCapacity;

enum class CreativeTerrainPaintPlanStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  CapacityExceeded,
  NoSurface,
  NoChange,
  Ready,
};

struct CreativeTerrainPaintRequest {
  std::span<const CreativeTerrainColumn> surfaceColumns;
  const CreativeTerrainMaterialField* materialField = nullptr;
  CreativeTerrainPaintMode mode = CreativeTerrainPaintMode::Brush;
  CreativeTerrainCoord2 center{};
  CreativeTerrainCoord2 minimumCoord{};
  CreativeTerrainCoord2 maximumCoord{};
  CreativeTerrainMaterial material = CreativeTerrainMaterial::Grass;
  CreativeTerrainPaintSource source = CreativeTerrainPaintSource::Any;
  std::uint16_t radiusCells = 1U;
  CreativeTerrainPaintHardness hardness =
      CreativeTerrainPaintHardness::Solid;
  CreativeTerrainPaintOpacity opacity =
      CreativeTerrainPaintOpacity::Percent100;
  CreativeTerrainPaintMask mask = CreativeTerrainPaintMask::Circle;
  CreativeTerrainPaintBlend blend = CreativeTerrainPaintBlend::Replace;
  CreativeTerrainPaintSlopeFilter slopeFilter =
      CreativeTerrainPaintSlopeFilter::Any;
  CreativeTerrainPaintHeightFilter heightFilter =
      CreativeTerrainPaintHeightFilter::Any;
  std::size_t maxAffectedCellCount = kCreativeTerrainPaintCellCapacity;
};

struct CreativeTerrainPaintCellPreview {
  CreativeTerrainCoord2 coord{};
  CreativeTerrainMaterialWeights beforeWeights{};
  CreativeTerrainMaterialWeights afterWeights{};
  std::uint16_t heightCells = 0U;
  std::uint8_t influence = 0U;
  double slopeDegrees = 0.0;
};

struct CreativeTerrainPaintPlan {
  bool requested = false;
  bool accepted = false;
  CreativeTerrainPaintPlanStatus status =
      CreativeTerrainPaintPlanStatus::NotRequested;
  CreativeTerrainPaintMode mode = CreativeTerrainPaintMode::Brush;
  CreativeTerrainCoord2 center{};
  CreativeTerrainCoord2 minimumCoord{};
  CreativeTerrainCoord2 maximumCoord{};
  CreativeTerrainMaterial material = CreativeTerrainMaterial::Grass;
  CreativeTerrainPaintSource source = CreativeTerrainPaintSource::Any;
  std::uint16_t radiusCells = 1U;
  CreativeTerrainPaintHardness hardness =
      CreativeTerrainPaintHardness::Solid;
  CreativeTerrainPaintOpacity opacity =
      CreativeTerrainPaintOpacity::Percent100;
  CreativeTerrainPaintMask mask = CreativeTerrainPaintMask::Circle;
  CreativeTerrainPaintBlend blend = CreativeTerrainPaintBlend::Replace;
  CreativeTerrainPaintSlopeFilter slopeFilter =
      CreativeTerrainPaintSlopeFilter::Any;
  CreativeTerrainPaintHeightFilter heightFilter =
      CreativeTerrainPaintHeightFilter::Any;
  std::vector<CreativeTerrainCoord2> affectedCells;
  std::vector<CreativeTerrainPaintCellPreview> previewCells;
  std::vector<CreativeTerrainMaterialEdit> edits;
  std::string_view reasonCode = "creative_terrain_paint_not_requested";

  [[nodiscard]] std::span<const CreativeTerrainCoord2> cells()
      const noexcept {
    return affectedCells;
  }
  [[nodiscard]] std::span<const CreativeTerrainMaterialEdit> items()
      const noexcept {
    return edits;
  }
  [[nodiscard]] std::span<const CreativeTerrainPaintCellPreview> previews()
      const noexcept {
    return previewCells;
  }
};

[[nodiscard]] std::uint16_t creativeTerrainPaintRadiusCells(
    CreativeTerrainPaintRadius radius) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainPaintRadius radius) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainPaintMode mode) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainPaintSource source) noexcept;
[[nodiscard]] bool creativeTerrainPaintSourceMatches(
    CreativeTerrainPaintSource source,
    CreativeTerrainMaterial material) noexcept;
[[nodiscard]] std::uint8_t creativeTerrainPaintHardnessPercent(
    CreativeTerrainPaintHardness hardness) noexcept;
[[nodiscard]] std::uint8_t creativeTerrainPaintOpacityPercent(
    CreativeTerrainPaintOpacity opacity) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainPaintHardness hardness) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainPaintOpacity opacity) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainPaintMask mask) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainPaintBlend blend) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainPaintSlopeFilter filter) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainPaintHeightFilter filter) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainPaintPlanStatus status) noexcept;
[[nodiscard]] CreativeTerrainPaintPlan buildCreativeTerrainPaintPlan(
    const CreativeTerrainPaintRequest& request);

}  // namespace iggy3d::creative
