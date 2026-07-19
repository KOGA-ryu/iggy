#pragma once

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/document/TerrainMaterialField.hpp"
#include "app/iggy3d/creative/tools/TerrainPath.hpp"
#include "app/iggy3d/creative/tools/TerrainProfile.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

enum class CreativeTerrainRecipeKind : std::uint8_t {
  Hill = 0U,
  Valley = 1U,
  Crater = 2U,
  Ridge = 3U,
  Road = 4U,
  River = 5U,
  Ditch = 6U,
  RidgeLine = 7U,
  Plateau = 8U,
  Count = 9U,
};

enum class CreativeTerrainRecipeStatus : std::uint8_t {
  NotRequested,
  InvalidDocument,
  InvalidKind,
  KernelRejected,
  MaterialPlanRejected,
  NoChange,
  Ready,
  StalePlan,
  MutationRejected,
  InstallRejected,
  Applied,
};

struct CreativeTerrainProfileRecipeRequest {
  const CreativeDocument* document = nullptr;
  CreativeTerrainRecipeKind kind = CreativeTerrainRecipeKind::Hill;
  CreativeTerrainCoord2 center{};
  std::uint16_t baseHeightCells = 4U;
  std::uint16_t radiusCells = 4U;
  std::uint16_t amplitudeCells = 4U;
  std::uint16_t spacingCells = 1U;
  CreativeTerrainProfileBlend blend = CreativeTerrainProfileBlend::Set;
  CreativeTerrainProfileRodPolicy rodPolicy =
      CreativeTerrainProfileRodPolicy::Fill;
  CreativeTerrainProfileDirection direction =
      CreativeTerrainProfileDirection::PositiveX;
  std::uint8_t frequency = 1U;
};

struct CreativeTerrainPathRecipeRequest {
  const CreativeDocument* document = nullptr;
  CreativeTerrainRecipeKind kind = CreativeTerrainRecipeKind::Road;
  std::span<const CreativeTerrainPathPoint> points;
  CreativeTerrainPathElevation elevation =
      CreativeTerrainPathElevation::Follow;
  std::uint16_t halfWidthCells = 1U;
  std::uint16_t amplitudeCells = 1U;
  bool paintSurface = true;
  // Count selects the semantic default: Dirt for roads, Sand for waterways,
  // and no material override for ridgelines.
  CreativeTerrainMaterial material = CreativeTerrainMaterial::Count;
};

struct CreativeTerrainRecipePlan {
  CreativeTerrainRecipeKind kind = CreativeTerrainRecipeKind::Count;
  CreativeDocumentId sourceDocumentId = kInvalidDocumentId;
  std::uint64_t sourceDocumentRevision = 0U;
  std::uint64_t sourceTerrainRevision = 0U;
  std::uint64_t sourceMaterialRevision = 0U;
  CreativeTerrainCoord2 minimumCoord{};
  CreativeTerrainCoord2 maximumCoord{};
  std::vector<CreativeTerrainControlEdit> controlEdits;
  std::vector<CreativeTerrainMaterialEdit> materialEdits;
  // Complete semantic outputs, including values already present in the source
  // document. Transient attribution can therefore distinguish a no-op from an
  // absent source without reimplementing terrain geometry.
  std::vector<CreativeTerrainControlPoint> controlOutputs;
  std::vector<CreativeTerrainMaterialOverride> materialOutputs;
};

struct CreativeTerrainRecipeReceipt {
  bool requested = false;
  bool accepted = false;
  CreativeTerrainRecipeStatus status = CreativeTerrainRecipeStatus::NotRequested;
  CreativeTerrainRecipeKind kind = CreativeTerrainRecipeKind::Count;
  std::uint64_t controlEditCount = 0U;
  std::uint64_t materialEditCount = 0U;
  std::string reasonCode = "creative_terrain_recipe_not_requested";
  std::string kernelReasonCode = "creative_terrain_recipe_kernel_not_requested";
};

struct CreativeTerrainRecipeResult {
  CreativeTerrainRecipePlan plan;
  CreativeTerrainRecipeReceipt receipt;
};

struct CreativeTerrainRecipePreviewResult {
  bool requested = false;
  bool accepted = false;
  CreativeTerrainRecipeStatus status = CreativeTerrainRecipeStatus::NotRequested;
  CreativeTerrainMutationReceipt terrainReceipt;
  CreativeTerrainMaterialMutationReceipt materialReceipt;
  CreativeTerrainRenderPlan renderPlan;
  std::string reasonCode = "creative_terrain_recipe_preview_not_requested";
};

struct CreativeTerrainRecipeApplyReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeTerrainRecipeStatus status = CreativeTerrainRecipeStatus::NotRequested;
  CreativeTerrainMutationReceipt terrainReceipt;
  CreativeTerrainMaterialMutationReceipt materialReceipt;
  CreativeFacadeDocumentInstallReceipt installReceipt;
  CreativeHistoryRecordReceipt historyReceipt;
  std::string reasonCode = "creative_terrain_recipe_apply_not_requested";
};

[[nodiscard]] std::string_view toString(
    CreativeTerrainRecipeKind kind) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainRecipeStatus status) noexcept;

[[nodiscard]] CreativeTerrainRecipeResult buildCreativeTerrainProfileRecipe(
    const CreativeTerrainProfileRecipeRequest& request);
[[nodiscard]] CreativeTerrainRecipeResult buildCreativeTerrainPathRecipe(
    const CreativeTerrainPathRecipeRequest& request);

// Preview and apply consume the exact edit vectors produced by the recipe.
// There is no second geometry path for UI or 2D-symbol prediction.
[[nodiscard]] CreativeTerrainRecipePreviewResult previewCreativeTerrainRecipe(
    const CreativeDocument& document,
    const CreativeTerrainRecipePlan& plan);
[[nodiscard]] CreativeTerrainRecipeApplyReceipt applyCreativeTerrainRecipe(
    Facade& facade,
    const CreativeTerrainRecipePlan& plan);
[[nodiscard]] CreativeTerrainRecipeApplyReceipt
applyCreativeTerrainRecipeWithHistory(CreativeAppState& appState,
                                      const CreativeTerrainRecipePlan& plan,
                                      std::string_view source);

}  // namespace iggy3d::creative
