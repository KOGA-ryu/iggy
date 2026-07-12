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
  std::size_t maxAffectedCellCount = kCreativeTerrainPaintCellCapacity;
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
  std::vector<CreativeTerrainCoord2> affectedCells;
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
[[nodiscard]] std::string_view toString(
    CreativeTerrainPaintPlanStatus status) noexcept;
[[nodiscard]] CreativeTerrainPaintPlan buildCreativeTerrainPaintPlan(
    const CreativeTerrainPaintRequest& request);

}  // namespace iggy3d::creative
