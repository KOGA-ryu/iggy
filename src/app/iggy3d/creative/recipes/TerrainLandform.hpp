#pragma once

#include "app/iggy3d/creative/document/TerrainHeightField.hpp"
#include "app/iggy3d/creative/document/TerrainMaterialField.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

inline constexpr std::uint32_t kCreativeTerrainLandformRecipeVersion = 1U;
inline constexpr std::uint8_t kCreativeTerrainLandformMaximumTerraceCount =
    16U;
inline constexpr std::uint16_t kCreativeTerrainLandformMaximumEdgeCells = 16U;
inline constexpr std::uint16_t kCreativeTerrainLandformMaximumErosionReliefCells =
    4U;

enum class CreativeTerrainLandformKind : std::uint8_t {
  Plateau,
  Terrace,
  Cliff,
  Count,
};

enum class CreativeTerrainLandformDirection : std::uint8_t {
  PositiveX,
  PositiveZ,
  NegativeX,
  NegativeZ,
  Count,
};

enum class CreativeTerrainLandformEdge : std::uint8_t {
  Slope,
  Retaining,
  Count,
};

enum class CreativeTerrainLandformErosion : std::uint8_t {
  Clean,
  Weathered,
  Count,
};

// One bounded, editable site-shaping source. Bounds are dense terrain tile
// coordinates. Retaining edges are vertical; slope edges consume edgeWidthCells
// inside the authored bounds. Weathering perturbs transition cells only, so the
// usable plateau or terrace surfaces remain exact.
struct CreativeTerrainLandformRecipe {
  std::uint32_t version = kCreativeTerrainLandformRecipeVersion;
  CreativeTerrainLandformKind kind = CreativeTerrainLandformKind::Plateau;
  CreativeTerrainHeightFieldBounds bounds{{0, 0}, 8U, 8U};
  std::uint16_t baseHeightCells = 1U;
  std::uint16_t targetHeightCells = 4U;
  std::uint8_t terraceCount = 4U;
  CreativeTerrainLandformDirection direction =
      CreativeTerrainLandformDirection::PositiveX;
  CreativeTerrainLandformEdge edge = CreativeTerrainLandformEdge::Slope;
  std::uint16_t edgeWidthCells = 2U;
  std::uint16_t featherCells = 0U;
  bool paintSurface = true;
  CreativeTerrainMaterial material = CreativeTerrainMaterial::Grass;
  CreativeTerrainLandformErosion erosion =
      CreativeTerrainLandformErosion::Clean;
  std::uint16_t erosionReliefCells = 0U;
  std::uint64_t seed = 1U;

  [[nodiscard]] friend bool operator==(
      const CreativeTerrainLandformRecipe&,
      const CreativeTerrainLandformRecipe&) noexcept = default;
};

enum class CreativeTerrainLandformStatus : std::uint8_t {
  NotRequested,
  UnsupportedVersion,
  InvalidRecipe,
  InvalidSource,
  CapacityExceeded,
  OutputRejected,
  MaterialRejected,
  Ready,
};

struct CreativeTerrainLandformReceipt {
  bool requested = false;
  bool accepted = false;
  CreativeTerrainLandformStatus status =
      CreativeTerrainLandformStatus::NotRequested;
  std::uint64_t evaluatedCellCount = 0U;
  std::uint64_t modifiedCellCount = 0U;
  std::uint64_t transitionCellCount = 0U;
  std::uint64_t weatheredCellCount = 0U;
  std::uint64_t materialEditCount = 0U;
  std::uint64_t hardEdgeCount = 0U;
  std::uint64_t outputCellCount = 0U;
  std::uint64_t heightHash = 0U;
  std::string_view reasonCode = "creative_terrain_landform_not_requested";
};

struct CreativeTerrainLandformResult {
  CreativeTerrainLandformRecipe recipe{};
  CreativeTerrainHeightField heightField;
  std::vector<CreativeTerrainMaterialEdit> materialEdits;
  std::vector<CreativeTerrainHardEdge> hardEdges;
  CreativeTerrainLandformReceipt receipt{};
};

[[nodiscard]] bool isValidCreativeTerrainLandformRecipe(
    const CreativeTerrainLandformRecipe& recipe) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainLandformKind value) noexcept;
[[nodiscard]] bool parseCreativeTerrainLandformKind(
    std::string_view text,
    CreativeTerrainLandformKind& output) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainLandformDirection value) noexcept;
[[nodiscard]] bool parseCreativeTerrainLandformDirection(
    std::string_view text,
    CreativeTerrainLandformDirection& output) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainLandformEdge value) noexcept;
[[nodiscard]] bool parseCreativeTerrainLandformEdge(
    std::string_view text,
    CreativeTerrainLandformEdge& output) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainLandformErosion value) noexcept;
[[nodiscard]] bool parseCreativeTerrainLandformErosion(
    std::string_view text,
    CreativeTerrainLandformErosion& output) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainLandformStatus value) noexcept;

// O(n) in the union of the source and authored bounds, hard-bounded by the
// dense terrain capacity. Preview and operation replay consume this exact
// height field, material edits, and intentional non-smoothed topology.
[[nodiscard]] CreativeTerrainLandformResult buildCreativeTerrainLandform(
    const CreativeTerrainHeightField& existingAuthored,
    const CreativeTerrainSurfacePlan& canonicalSource,
    const CreativeTerrainMaterialField& existingMaterial,
    const CreativeTerrainLandformRecipe& recipe);

}  // namespace iggy3d::creative
