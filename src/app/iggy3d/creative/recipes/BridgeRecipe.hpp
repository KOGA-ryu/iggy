#pragma once

#include "app/iggy3d/creative/recipes/CreativeRecipe.hpp"
#include "app/iggy3d/creative/recipes/TerrainGradeRecipe.hpp"
#include "app/iggy3d/creative/recipes/WatercourseRecipe.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

inline constexpr std::uint32_t kCreativeBridgeRecipeVersion = 1U;
inline constexpr std::size_t kCreativeBridgeSupportStationCapacity = 64U;
inline constexpr std::size_t kCreativeBridgeGeneratedObjectCapacity = 132U;

enum class CreativeBridgeAttachmentKind : std::uint8_t {
  WatercourseCrossing,
  Count,
};

enum class CreativeBridgeSupportStyle : std::uint8_t {
  None,
  PierPairs,
  Count,
};

struct CreativeBridgeMaterialKit {
  CreativeStructuralMaterial deck = CreativeStructuralMaterial::Timber;
  CreativeStructuralMaterial supports = CreativeStructuralMaterial::Stone;
  CreativeStructuralMaterial rails = CreativeStructuralMaterial::Timber;

  [[nodiscard]] friend constexpr bool operator==(
      CreativeBridgeMaterialKit,
      CreativeBridgeMaterialKit) noexcept = default;
};

struct CreativeBridgeSettings {
  double deckWidthMeters = 2.0;
  double deckThicknessMeters = 0.35;
  double deckElevationOffsetMeters = 0.0;
  double maximumSpanMeters = 64.0;
  double minimumClearanceMeters = 0.5;
  CreativeBridgeSupportStyle supportStyle =
      CreativeBridgeSupportStyle::PierPairs;
  double supportSpacingMeters = 4.0;
  double supportWidthMeters = 0.4;
  double supportDepthMeters = 0.4;
  bool rails = true;
  double railHeightMeters = 1.0;
  double railThicknessMeters = 0.12;
  std::uint16_t maximumApproachGradePermille = 500U;
  std::uint16_t approachFalloffCells = 2U;
  CreativeBridgeMaterialKit materials{};

  [[nodiscard]] friend constexpr bool operator==(
      CreativeBridgeSettings,
      CreativeBridgeSettings) noexcept = default;
};

// Durable World Layout source. The stable path key and crossing id are the
// attachment contract; generated positions and member objects are transient.
struct CreativeBridgeSourceRecipe {
  std::uint32_t version = kCreativeBridgeRecipeVersion;
  CreativeBridgeAttachmentKind attachment =
      CreativeBridgeAttachmentKind::WatercourseCrossing;
  std::string watercoursePathKey;
  CreativeTerrainWatercourseCrossingId crossingId =
      kInvalidCreativeTerrainWatercourseCrossingId;
  CreativeBridgeSettings settings{};

  [[nodiscard]] friend bool operator==(
      const CreativeBridgeSourceRecipe&,
      const CreativeBridgeSourceRecipe&) noexcept = default;
};

struct CreativeBridgeRecipeRequest {
  std::uint32_t version = kCreativeBridgeRecipeVersion;
  std::string instanceKey;
  std::string name = "Bridge";
  double gridCellSizeMeters = 1.0;
  CreativeBridgeSourceRecipe source{};
  CreativeWatercourseCrossingFrame crossing{};
  std::vector<std::string> tags;
};

enum class CreativeBridgeRecipeStatus : std::uint8_t {
  NotRequested,
  UnsupportedVersion,
  InvalidRequest,
  InvalidAttachment,
  SpanExceeded,
  ClearanceInsufficient,
  ApproachGradeExceeded,
  ApproachRejected,
  StructureCapacityExceeded,
  InvalidStructure,
  Ready,
};

struct CreativeBridgeRecipeReceipt {
  bool requested = false;
  bool accepted = false;
  CreativeBridgeRecipeStatus status =
      CreativeBridgeRecipeStatus::NotRequested;
  std::uint64_t deckCount = 0U;
  std::uint64_t supportStationCount = 0U;
  std::uint64_t supportObjectCount = 0U;
  std::uint64_t railCount = 0U;
  std::uint64_t approachGradeCount = 0U;
  std::uint64_t generatedObjectCount = 0U;
  std::uint64_t definitionFingerprint = 0U;
  double spanMeters = 0.0;
  double underDeckClearanceMeters = 0.0;
  double maximumApproachGradePermille = 0.0;
  std::string reasonCode = "creative_bridge_recipe_not_requested";
};

struct CreativeBridgeRecipeResult {
  CreativeRecipePlan structure;
  std::array<CreativeTerrainGradeRecipe, 2U> approachGrades{};
  std::size_t approachGradeCount = 0U;
  CreativeBridgeRecipeReceipt receipt{};
};

[[nodiscard]] bool isValidCreativeBridgeSettings(
    const CreativeBridgeSettings& settings) noexcept;
[[nodiscard]] bool isValidCreativeBridgeSourceRecipe(
    const CreativeBridgeSourceRecipe& source) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeBridgeRecipeStatus status) noexcept;

// Pure semantic planner. It consumes one canonical S3 crossing frame and emits
// generated structure plus exact terrain-grade recipes; it mutates neither the
// document nor terrain.
[[nodiscard]] CreativeBridgeRecipeResult planCreativeBridge(
    const CreativeBridgeRecipeRequest& request);

}  // namespace iggy3d::creative
