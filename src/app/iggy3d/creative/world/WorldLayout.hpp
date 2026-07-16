#pragma once

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/recipes/BuildingRecipe.hpp"
#include "app/iggy3d/creative/recipes/TerrainRecipe.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

inline constexpr std::uint32_t kCreativeWorldLayoutSchemaVersion = 1U;
inline constexpr std::size_t kInvalidCreativeWorldLayoutIndex =
    std::numeric_limits<std::size_t>::max();

// Layout coordinates are grid-line coordinates. Rect maximums are exclusive,
// so {0, 0}->{10, 8} describes a ten-by-eight-cell floor without half-cell
// conventions leaking into the 2D editor.
struct CreativeWorldLayoutRect {
  CreativeTerrainCoord2 minimum{};
  CreativeTerrainCoord2 maximum{};
};

struct CreativeWorldLayoutBuilding {
  std::string stableKey;
  std::string name;
  CreativeBuildingRootMode rootMode = CreativeBuildingRootMode::None;
  CreativeWorldLayoutRect rootFootprint;
  std::int32_t rootBaseLayer = 0;
  std::uint16_t rootHeightCells = 3U;
  bool visible = true;
  std::vector<std::string> tags;
};

struct CreativeWorldLayoutBox {
  std::size_t buildingIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeObjectKind kind = CreativeObjectKind::Floor;
  std::string stableKey;
  std::string name;
  CreativeWorldLayoutRect footprint;
  std::int32_t baseLayer = 0;
  std::uint16_t heightCells = 1U;
};

struct CreativeWorldLayoutWall {
  std::size_t buildingIndex = kInvalidCreativeWorldLayoutIndex;
  std::string stableKey;
  std::string name;
  CreativeTerrainCoord2 start{};
  CreativeTerrainCoord2 end{};
  std::int32_t baseLayer = 0;
  std::uint16_t heightCells = 3U;
  double thicknessCells = 0.25;
};

struct CreativeWorldLayoutOpening {
  std::size_t wallIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeBuildingOpeningKind kind = CreativeBuildingOpeningKind::Door;
  CreativeBuildingOpeningPose pose = CreativeBuildingOpeningPose::Closed;
  std::string stableKey;
  std::string name;
  double centerOffsetCells = 0.0;
  double widthCells = 1.0;
  double cutoutBottomCells = 0.0;
  double cutoutHeightCells = 2.1;
  bool includeInsert = true;
  double insertBottomCells = 0.0;
  double insertHeightCells = 0.0;
  double insertWidthCells = 0.0;
  double insertThicknessCells = 0.0;
};

struct CreativeWorldLayoutTerrainProfile {
  std::string stableKey;
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

struct CreativeWorldLayoutTerrainPath {
  std::string stableKey;
  CreativeTerrainRecipeKind kind = CreativeTerrainRecipeKind::Road;
  std::size_t firstPointIndex = 0U;
  std::size_t pointCount = 0U;
  CreativeTerrainPathElevation elevation =
      CreativeTerrainPathElevation::Follow;
  std::uint16_t halfWidthCells = 1U;
  std::uint16_t amplitudeCells = 1U;
  bool paintSurface = true;
  CreativeTerrainMaterial material = CreativeTerrainMaterial::Count;
};

enum class CreativeWorldLayoutTerrainOwnership : std::uint8_t {
  PreserveExisting,
  ReplaceAll,
};

// Flat symbol tables keep 2D editing, selection, serialization, and diffing
// independent. Indices express ownership instead of nested mutable objects.
struct CreativeWorldLayout {
  std::uint32_t schemaVersion = kCreativeWorldLayoutSchemaVersion;
  std::string stableKey = "world_layout";
  CreativeWorldLayoutTerrainOwnership terrainOwnership =
      CreativeWorldLayoutTerrainOwnership::PreserveExisting;
  std::vector<CreativeWorldLayoutBuilding> buildings;
  std::vector<CreativeWorldLayoutBox> boxes;
  std::vector<CreativeWorldLayoutWall> walls;
  std::vector<CreativeWorldLayoutOpening> openings;
  std::vector<CreativeWorldLayoutTerrainProfile> terrainProfiles;
  std::vector<CreativeWorldLayoutTerrainPath> terrainPaths;
  std::vector<CreativeTerrainPathPoint> terrainPathPoints;
};

enum class CreativeWorldLayoutTable : std::uint8_t {
  None,
  Building,
  Box,
  Wall,
  Opening,
  TerrainProfile,
  TerrainPath,
  TerrainPathPoint,
};

enum class CreativeWorldLayoutStatus : std::uint8_t {
  NotRequested,
  InvalidDocument,
  InvalidSchema,
  Empty,
  DuplicateStableKey,
  InvalidSymbol,
  KernelRejected,
  CapacityExceeded,
  NoChange,
  Ready,
  StalePlan,
  MutationRejected,
  ObjectRejected,
  InstallRejected,
  Applied,
};

struct CreativeWorldLayoutPlan {
  std::uint32_t schemaVersion = kCreativeWorldLayoutSchemaVersion;
  std::string layoutKey;
  CreativeDocumentId sourceDocumentId = kInvalidDocumentId;
  std::uint64_t sourceDocumentRevision = 0U;
  std::uint64_t sourceTerrainRevision = 0U;
  std::uint64_t sourceMaterialRevision = 0U;
  std::vector<CreativeObjectId> objectRemoveIds;
  std::vector<CreativeRecipePlan> objectRecipes;
  std::vector<CreativeTerrainControlEdit> terrainEdits;
  std::vector<CreativeTerrainMaterialEdit> materialEdits;
};

struct CreativeWorldLayoutReceipt {
  bool requested = false;
  bool accepted = false;
  CreativeWorldLayoutStatus status = CreativeWorldLayoutStatus::NotRequested;
  CreativeWorldLayoutTable failedTable = CreativeWorldLayoutTable::None;
  std::size_t failedIndex = kInvalidCreativeWorldLayoutIndex;
  std::uint64_t buildingCount = 0U;
  std::uint64_t objectRecipeCount = 0U;
  std::uint64_t objectCount = 0U;
  std::uint64_t objectRemoveCount = 0U;
  std::uint64_t terrainControlEditCount = 0U;
  std::uint64_t terrainMaterialEditCount = 0U;
  std::string reasonCode = "creative_world_layout_not_requested";
  std::string kernelReasonCode = "creative_world_layout_kernel_not_requested";
};

struct CreativeWorldLayoutCompileResult {
  CreativeWorldLayoutPlan plan;
  CreativeWorldLayoutReceipt receipt;
};

struct CreativeWorldLayoutPreviewResult {
  bool requested = false;
  bool accepted = false;
  CreativeWorldLayoutStatus status = CreativeWorldLayoutStatus::NotRequested;
  bool hasTerrainRenderPlan = false;
  CreativeDocument document;
  CreativeTerrainRenderPlan terrainRenderPlan;
  std::size_t failedRecipeIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t failedObjectIndex = kInvalidCreativeWorldLayoutIndex;
  std::string reasonCode = "creative_world_layout_preview_not_requested";
};

struct CreativeWorldLayoutApplyReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeWorldLayoutStatus status = CreativeWorldLayoutStatus::NotRequested;
  CreativeTerrainMutationReceipt terrainReceipt;
  CreativeTerrainMaterialMutationReceipt materialReceipt;
  CreativeFacadeDocumentInstallReceipt installReceipt;
  CreativeHistoryRecordReceipt historyReceipt;
  std::size_t failedRecipeIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t failedObjectIndex = kInvalidCreativeWorldLayoutIndex;
  std::string reasonCode = "creative_world_layout_apply_not_requested";
};

[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutTable table) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutStatus status) noexcept;
[[nodiscard]] std::string creativeWorldLayoutTag(std::string_view layoutKey);

[[nodiscard]] CreativeWorldLayoutCompileResult buildCreativeWorldLayoutPlan(
    const CreativeDocument& document,
    const CreativeWorldLayout& layout);
[[nodiscard]] CreativeWorldLayoutPreviewResult previewCreativeWorldLayoutPlan(
    const CreativeDocument& document,
    const CreativeWorldLayoutPlan& plan);
[[nodiscard]] CreativeWorldLayoutApplyReceipt applyCreativeWorldLayoutPlan(
    Facade& facade,
    const CreativeWorldLayoutPlan& plan);
[[nodiscard]] CreativeWorldLayoutApplyReceipt
applyCreativeWorldLayoutPlanWithHistory(CreativeAppState& appState,
                                        const CreativeWorldLayoutPlan& plan,
                                        std::string_view source);

}  // namespace iggy3d::creative
