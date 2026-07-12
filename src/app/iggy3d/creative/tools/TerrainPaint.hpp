#pragma once

#include "app/iggy3d/creative/document/TerrainMaterialField.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace iggy3d::creative {

enum class CreativeTerrainPaintRadius : std::uint8_t {
  OneCell,
  TwoCells,
  FourCells,
  EightCells,
  Count,
};

inline constexpr std::size_t kCreativeTerrainPaintEditCapacity = 256U;

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
  CreativeTerrainCoord2 center{};
  CreativeTerrainMaterial material = CreativeTerrainMaterial::Grass;
  std::uint16_t radiusCells = 1U;
};

struct CreativeTerrainPaintPlan {
  bool requested = false;
  bool accepted = false;
  CreativeTerrainPaintPlanStatus status =
      CreativeTerrainPaintPlanStatus::NotRequested;
  CreativeTerrainCoord2 center{};
  CreativeTerrainMaterial material = CreativeTerrainMaterial::Grass;
  std::uint16_t radiusCells = 1U;
  std::array<CreativeTerrainMaterialEdit, kCreativeTerrainPaintEditCapacity>
      edits{};
  std::uint16_t editCount = 0U;
  std::uint16_t surfaceCellCount = 0U;
  std::string_view reasonCode = "creative_terrain_paint_not_requested";

  [[nodiscard]] std::span<const CreativeTerrainMaterialEdit> items()
      const noexcept {
    return {edits.data(), editCount};
  }
};

[[nodiscard]] std::uint16_t creativeTerrainPaintRadiusCells(
    CreativeTerrainPaintRadius radius) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainPaintRadius radius) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainPaintPlanStatus status) noexcept;
[[nodiscard]] CreativeTerrainPaintPlan buildCreativeTerrainPaintPlan(
    const CreativeTerrainPaintRequest& request) noexcept;

}  // namespace iggy3d::creative
