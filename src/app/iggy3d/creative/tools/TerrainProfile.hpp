#pragma once

#include "app/iggy3d/creative/document/TerrainHeightField.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

namespace iggy3d::creative {

enum class CreativeTerrainProfileKind : std::uint8_t {
  Hill,
  Basin,
  Ring,
  Crater,
  Ridge,
  Wave,
  Ripple,
  Count,
};

enum class CreativeTerrainProfileBlend : std::uint8_t {
  Set,
  Add,
  Count,
};

enum class CreativeTerrainProfileRodPolicy : std::uint8_t {
  Fill,
  Existing,
  Count,
};

enum class CreativeTerrainProfileRadius : std::uint8_t {
  TwoCells,
  FourCells,
  EightCells,
  Count,
};

enum class CreativeTerrainProfileAmplitude : std::uint8_t {
  OneCell,
  TwoCells,
  FourCells,
  EightCells,
  SixteenCells,
  Count,
};

enum class CreativeTerrainProfileSpacing : std::uint8_t {
  OneCell,
  TwoCells,
  FourCells,
  Count,
};

enum class CreativeTerrainProfileDirection : std::uint8_t {
  PositiveX,
  PositiveXPositiveZ,
  PositiveZ,
  NegativeXPositiveZ,
  NegativeX,
  NegativeXNegativeZ,
  NegativeZ,
  PositiveXNegativeZ,
  Count,
};

enum class CreativeTerrainProfileFrequency : std::uint8_t {
  OneCycle,
  TwoCycles,
  Count,
};

struct CreativeTerrainProfileRequest {
  const CreativeTerrainField* field = nullptr;
  CreativeTerrainCoord2 center{};
  std::uint16_t baseHeightCells = 4U;
  CreativeTerrainProfileKind profile = CreativeTerrainProfileKind::Hill;
  CreativeTerrainProfileBlend blend = CreativeTerrainProfileBlend::Set;
  CreativeTerrainProfileRodPolicy rodPolicy =
      CreativeTerrainProfileRodPolicy::Fill;
  CreativeTerrainProfileDirection direction =
      CreativeTerrainProfileDirection::PositiveX;
  std::uint16_t radiusCells = 4U;
  std::uint16_t amplitudeCells = 4U;
  std::uint16_t spacingCells = 1U;
  std::uint8_t frequency = 1U;
  std::uint64_t seed = 0U;
};

enum class CreativeTerrainProfilePlanStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  CoordinateOverflow,
  CapacityExceeded,
  SamplingUnproven,
  UnderSampled,
  NoControlsInBrush,
  NoChange,
  Ready,
};

struct CreativeTerrainProfilePlan {
  bool requested = false;
  bool accepted = false;
  CreativeTerrainProfilePlanStatus status =
      CreativeTerrainProfilePlanStatus::NotRequested;
  std::array<CreativeTerrainControlEdit, kCreativeTerrainControlCapacity> edits{};
  std::uint16_t candidateCount = 0U;
  std::uint16_t editCount = 0U;
  std::string_view reasonCode = "creative_terrain_profile_not_requested";

  [[nodiscard]] std::span<const CreativeTerrainControlEdit> items()
      const noexcept {
    return {edits.data(), editCount};
  }
};

static_assert(std::is_trivially_copyable_v<CreativeTerrainProfilePlan>);
static_assert(std::is_standard_layout_v<CreativeTerrainProfilePlan>);

[[nodiscard]] std::string_view toString(
    CreativeTerrainProfileKind value) noexcept;
[[nodiscard]] bool parseCreativeTerrainProfileKind(
    std::string_view text,
    CreativeTerrainProfileKind& output) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainProfileBlend value) noexcept;
[[nodiscard]] bool parseCreativeTerrainProfileBlend(
    std::string_view text,
    CreativeTerrainProfileBlend& output) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainProfileRodPolicy value) noexcept;
[[nodiscard]] bool parseCreativeTerrainProfileRodPolicy(
    std::string_view text,
    CreativeTerrainProfileRodPolicy& output) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainProfileRadius value) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainProfileAmplitude value) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainProfileSpacing value) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainProfileDirection value) noexcept;
[[nodiscard]] bool parseCreativeTerrainProfileDirection(
    std::string_view text,
    CreativeTerrainProfileDirection& output) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainProfileFrequency value) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainProfilePlanStatus value) noexcept;

[[nodiscard]] std::uint16_t creativeTerrainProfileRadiusCells(
    CreativeTerrainProfileRadius value) noexcept;
[[nodiscard]] std::uint16_t creativeTerrainProfileAmplitudeCells(
    CreativeTerrainProfileAmplitude value) noexcept;
[[nodiscard]] std::uint16_t creativeTerrainProfileSpacingCells(
    CreativeTerrainProfileSpacing value) noexcept;
[[nodiscard]] std::uint8_t creativeTerrainProfileFrequencyCycles(
    CreativeTerrainProfileFrequency value) noexcept;
[[nodiscard]] bool creativeTerrainProfileUsesDirection(
    CreativeTerrainProfileKind value) noexcept;
[[nodiscard]] bool creativeTerrainProfileUsesFrequency(
    CreativeTerrainProfileKind value) noexcept;
[[nodiscard]] bool creativeTerrainProfileUsesSeed(
    CreativeTerrainProfileKind value) noexcept;

// O(a*n) in the bounded field, where a is at most 256 candidates and n is at
// most 256 controls. Every rejected plan exposes zero edits.
[[nodiscard]] CreativeTerrainProfilePlan buildCreativeTerrainProfilePlan(
    const CreativeTerrainProfileRequest& request) noexcept;

inline constexpr std::uint32_t kCreativeTerrainProfileRecipeVersion = 1U;
inline constexpr std::uint16_t kCreativeTerrainProfileMaximumRadiusCells = 64U;
inline constexpr std::uint16_t kCreativeTerrainProfileMaximumSpacingCells = 16U;
inline constexpr std::uint8_t kCreativeTerrainProfileMaximumFrequency = 8U;

// One durable mathematical elevation source. Dense operation replay evaluates
// every affected tile from this record; spacing controls deterministic profile
// sampling and seed controls oscillatory phase without changing the footprint.
struct CreativeTerrainProfileRecipe {
  std::uint32_t version = kCreativeTerrainProfileRecipeVersion;
  CreativeTerrainCoord2 center{};
  std::uint16_t baseHeightCells = 4U;
  CreativeTerrainProfileKind profile = CreativeTerrainProfileKind::Hill;
  CreativeTerrainProfileBlend blend = CreativeTerrainProfileBlend::Set;
  CreativeTerrainProfileRodPolicy rodPolicy =
      CreativeTerrainProfileRodPolicy::Fill;
  CreativeTerrainProfileDirection direction =
      CreativeTerrainProfileDirection::PositiveX;
  std::uint16_t radiusCells = 4U;
  std::uint16_t amplitudeCells = 4U;
  std::uint16_t spacingCells = 1U;
  std::uint8_t frequency = 1U;
  std::uint64_t seed = 0U;

  [[nodiscard]] friend constexpr bool operator==(
      CreativeTerrainProfileRecipe,
      CreativeTerrainProfileRecipe) noexcept = default;
};

enum class CreativeTerrainProfileRecipeStatus : std::uint8_t {
  NotRequested,
  UnsupportedVersion,
  InvalidRecipe,
  InvalidSource,
  CoordinateOverflow,
  CapacityExceeded,
  UnderSampled,
  NoSourceInFootprint,
  OutputRejected,
  Ready,
};

struct CreativeTerrainProfileRecipeReceipt {
  bool requested = false;
  bool accepted = false;
  bool boundsExpanded = false;
  CreativeTerrainProfileRecipeStatus status =
      CreativeTerrainProfileRecipeStatus::NotRequested;
  std::uint64_t sourceColumnCount = 0U;
  std::uint64_t evaluatedCellCount = 0U;
  std::uint64_t affectedCellCount = 0U;
  std::uint64_t modifiedCellCount = 0U;
  std::uint64_t materializedCellCount = 0U;
  std::uint64_t outputCellCount = 0U;
  std::uint64_t heightHash = 0U;
  std::string_view reasonCode =
      "creative_terrain_profile_recipe_not_requested";
};

struct CreativeTerrainProfileRecipeResult {
  CreativeTerrainProfileRecipe recipe{};
  CreativeTerrainHeightField heightField;
  CreativeTerrainProfileRecipeReceipt receipt{};
};

[[nodiscard]] bool isValidCreativeTerrainProfileRecipe(
    const CreativeTerrainProfileRecipe& recipe) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainProfileRecipeStatus status) noexcept;

// O(n + r^2), bounded by the 8192-cell dense terrain capacity. The immutable
// canonical source and authored heightfield produce the exact preview/commit
// candidate used by operation replay; rejected recipes return no output field.
[[nodiscard]] CreativeTerrainProfileRecipeResult
buildCreativeTerrainProfileRecipe(
    const CreativeTerrainHeightField& existingAuthored,
    const CreativeTerrainSurfacePlan& canonicalSource,
    const CreativeTerrainProfileRecipe& recipe);

}  // namespace iggy3d::creative
