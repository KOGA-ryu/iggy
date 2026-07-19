#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/world/WorldLayout.hpp"
#include "app/iggy3d/creative/world/WorldLayoutBuildingOps.hpp"
#include "app/iggy3d/creative/world/WorldLayoutTerrainReconciliation.hpp"

#include "EditorWorldLayoutElevation.hpp"
#include "EditorWorldLayoutDiagnostics.hpp"

namespace iggy3d::creative {
struct CreativeCatalogEntry;
}

namespace iggy3d_creative_app {

namespace cr = iggy3d::creative;

enum class CreativeEditorWorldLayoutTool : std::uint8_t {
  Select,
  Room,
  Floor,
  Wall,
  Door,
  Window,
  Stair,
  Ramp,
  Plateau,
  Road,
  Ditch,
  Bridge,
  CatalogAsset,
  PlayerSpawn,
  NpcSpawn,
  BuildingShell,
  Count,
};

enum class CreativeEditorWorldLayoutAssetCategory : std::uint8_t {
  Architecture,
  Nature,
  Cover,
  Props,
  Gameplay,
  Count,
};

enum class CreativeEditorWorldLayoutCatalogSnapMode : std::uint8_t {
  Grid,
  Floor,
  Wall,
  Count,
};

enum class CreativeEditorWorldLayoutCatalogSnapHostKind : std::uint8_t {
  None,
  LevelFloor,
  ExplicitWall,
  RoomEdge,
  Count,
};

enum class CreativeEditorWorldLayoutOpeningInsertOperation : std::uint8_t {
  FitAssetToOpening,
  ResizeOpeningToAsset,
  UseProceduralInsert,
  Count,
};

enum class CreativeEditorWorldLayoutAssetBoundsTarget : std::uint8_t {
  Object,
  OpeningInsert,
  Count,
};

struct CreativeEditorWorldLayoutAssetBoundsUpdate {
  CreativeEditorWorldLayoutAssetBoundsTarget target =
      CreativeEditorWorldLayoutAssetBoundsTarget::Object;
  std::size_t index = cr::kInvalidCreativeWorldLayoutIndex;
  std::string assetId;
  cr::CreativeBounds sourceBoundsMeters;
};

inline constexpr double
    kCreativeEditorWorldLayoutCatalogWallSnapToleranceCells = 1.25;

enum class CreativeEditorWorldLayoutPaletteCategory : std::uint8_t {
  Structure,
  Terrain,
  Object,
  Gameplay,
  Count,
};

enum class CreativeEditorWorldLayoutPaletteActivation : std::uint8_t {
  Tool,
  BuildingTemplate,
  Count,
};

struct CreativeEditorWorldLayoutPaletteEntry {
  CreativeEditorWorldLayoutPaletteCategory category =
      CreativeEditorWorldLayoutPaletteCategory::Structure;
  std::string_view label;
  CreativeEditorWorldLayoutPaletteActivation activation =
      CreativeEditorWorldLayoutPaletteActivation::Tool;
  CreativeEditorWorldLayoutTool tool = CreativeEditorWorldLayoutTool::Select;
  std::string_view buildingTemplateId;
};

enum class CreativeEditorWorldLayoutSelectionKind : std::uint8_t {
  None,
  Level,
  Room,
  VerticalConnector,
  Box,
  Wall,
  Opening,
  Building,
  TerrainProfile,
  TerrainPath,
  Object,
};

struct CreativeEditorWorldLayoutSelection {
  CreativeEditorWorldLayoutSelectionKind kind =
      CreativeEditorWorldLayoutSelectionKind::None;
  std::size_t index = cr::kInvalidCreativeWorldLayoutIndex;
};

struct CreativeEditorWorldLayoutPoint {
  double x = 0.0;
  double z = 0.0;
};

struct CreativeEditorWorldLayoutRoomSettings {
  cr::CreativeWorldLayoutRect footprint;
  double floorTopLayer = 0.0;
  std::uint16_t wallHeightCells =
      cr::kDefaultCreativeWorldLayoutWallHeightCells;
  double wallThicknessCells =
      cr::kDefaultCreativeWorldLayoutWallThicknessCells;
  std::uint16_t floorThicknessLayers = 1U;
  std::uint16_t roofThicknessLayers = 1U;
  cr::CreativeStructuralRoofStyle roofStyle =
      cr::CreativeStructuralRoofStyle::Flat;
  cr::CreativeStructuralRoofRidgeAxis roofRidgeAxis =
      cr::CreativeStructuralRoofRidgeAxis::X;
  double roofPitchDegrees =
      cr::kDefaultCreativeStructuralRoofPitchDegrees;
  double roofOverhangCells = 0.0;
};

struct CreativeEditorWorldLayoutRoomSettingsDraft {
  bool active = false;
  std::size_t roomIndex = cr::kInvalidCreativeWorldLayoutIndex;
  std::uint64_t sourceRevision = 0U;
  CreativeEditorWorldLayoutRoomSettings settings;
};

enum class CreativeEditorWorldLayoutRectHandle : std::uint8_t {
  None,
  Move,
  North,
  East,
  South,
  West,
  NorthWest,
  NorthEast,
  SouthEast,
  SouthWest,
  Count,
};

using CreativeEditorWorldLayoutRoomHandle =
    CreativeEditorWorldLayoutRectHandle;
using CreativeEditorWorldLayoutBoxHandle = CreativeEditorWorldLayoutRectHandle;

struct CreativeEditorWorldLayoutRoomTarget {
  std::size_t roomIndex = cr::kInvalidCreativeWorldLayoutIndex;
  CreativeEditorWorldLayoutRoomHandle handle =
      CreativeEditorWorldLayoutRoomHandle::None;
};

enum class CreativeEditorWorldLayoutRectManipulationPhase : std::uint8_t {
  Begin,
  Update,
  Commit,
  Cancel,
  Count,
};

using CreativeEditorWorldLayoutRoomManipulationPhase =
    CreativeEditorWorldLayoutRectManipulationPhase;
using CreativeEditorWorldLayoutBoxManipulationPhase =
    CreativeEditorWorldLayoutRectManipulationPhase;
using CreativeEditorWorldLayoutVerticalConnectorManipulationPhase =
    CreativeEditorWorldLayoutRectManipulationPhase;

struct CreativeEditorWorldLayoutRoomManipulationState {
  bool active = false;
  std::uint64_t sourceRevision = 0U;
  CreativeEditorWorldLayoutRoomTarget target;
  CreativeEditorWorldLayoutPoint startPoint;
  cr::CreativeWorldLayoutRect originalFootprint;
  cr::CreativeWorldLayoutRect previewFootprint;
  bool previewValid = false;
  std::string reasonCode =
      "creative_editor_world_layout_room_manipulation_inactive";
};

inline constexpr double
    kCreativeEditorWorldLayoutVerticalConnectorDirectionHandleOffsetCells =
        0.65;

struct CreativeEditorWorldLayoutVerticalConnectorSettings {
  cr::CreativeWorldLayoutRect footprint;
  cr::CreativeWorldLayoutVerticalConnectorKind kind =
      cr::CreativeWorldLayoutVerticalConnectorKind::Stair;
  cr::CreativeWorldLayoutVerticalDirection direction =
      cr::CreativeWorldLayoutVerticalDirection::PositiveZ;
};

struct CreativeEditorWorldLayoutVerticalConnectorSettingsDraft {
  bool active = false;
  std::size_t connectorIndex = cr::kInvalidCreativeWorldLayoutIndex;
  std::uint64_t sourceRevision = 0U;
  CreativeEditorWorldLayoutVerticalConnectorSettings settings;
};

struct CreativeEditorWorldLayoutVerticalConnectorTarget {
  std::size_t connectorIndex = cr::kInvalidCreativeWorldLayoutIndex;
  CreativeEditorWorldLayoutRectHandle handle =
      CreativeEditorWorldLayoutRectHandle::None;
  bool directionHandle = false;
};

struct CreativeEditorWorldLayoutVerticalConnectorManipulationState {
  bool active = false;
  std::uint64_t sourceRevision = 0U;
  CreativeEditorWorldLayoutVerticalConnectorTarget target;
  CreativeEditorWorldLayoutPoint startPoint;
  cr::CreativeWorldLayoutRect originalFootprint;
  cr::CreativeWorldLayoutVerticalConnectorKind originalKind =
      cr::CreativeWorldLayoutVerticalConnectorKind::Stair;
  cr::CreativeWorldLayoutVerticalDirection originalDirection =
      cr::CreativeWorldLayoutVerticalDirection::PositiveZ;
  cr::CreativeWorldLayoutRect previewFootprint;
  cr::CreativeWorldLayoutVerticalConnectorKind previewKind =
      cr::CreativeWorldLayoutVerticalConnectorKind::Stair;
  cr::CreativeWorldLayoutVerticalDirection previewDirection =
      cr::CreativeWorldLayoutVerticalDirection::PositiveZ;
  bool previewValid = false;
  std::string reasonCode =
      "creative_editor_world_layout_vertical_connector_manipulation_inactive";
};

struct CreativeEditorWorldLayoutBoxSettings {
  cr::CreativeWorldLayoutRect footprint;
  double anchorLayer = 0.0;
  std::uint16_t layerCount = 1U;
};

struct CreativeEditorWorldLayoutBoxSettingsDraft {
  bool active = false;
  std::size_t boxIndex = cr::kInvalidCreativeWorldLayoutIndex;
  std::uint64_t sourceRevision = 0U;
  CreativeEditorWorldLayoutBoxSettings settings;
};

struct CreativeEditorWorldLayoutBoxTarget {
  std::size_t boxIndex = cr::kInvalidCreativeWorldLayoutIndex;
  CreativeEditorWorldLayoutBoxHandle handle =
      CreativeEditorWorldLayoutBoxHandle::None;
};

struct CreativeEditorWorldLayoutBoxManipulationState {
  bool active = false;
  std::uint64_t sourceRevision = 0U;
  CreativeEditorWorldLayoutBoxTarget target;
  CreativeEditorWorldLayoutPoint startPoint;
  cr::CreativeWorldLayoutRect originalFootprint;
  cr::CreativeWorldLayoutRect previewFootprint;
  bool previewValid = false;
  std::string reasonCode =
      "creative_editor_world_layout_box_manipulation_inactive";
};

struct CreativeEditorWorldLayoutWallSettings {
  cr::CreativeTerrainCoord2 start;
  cr::CreativeTerrainCoord2 end;
  double baseLayer = 0.0;
  std::uint16_t heightCells =
      cr::kDefaultCreativeWorldLayoutWallHeightCells;
  double thicknessCells =
      cr::kDefaultCreativeWorldLayoutWallThicknessCells;
};

struct CreativeEditorWorldLayoutWallSettingsDraft {
  bool active = false;
  std::size_t wallIndex = cr::kInvalidCreativeWorldLayoutIndex;
  std::uint64_t sourceRevision = 0U;
  CreativeEditorWorldLayoutWallSettings settings;
};

enum class CreativeEditorWorldLayoutWallHandle : std::uint8_t {
  None,
  Move,
  Start,
  End,
  Count,
};

struct CreativeEditorWorldLayoutWallTarget {
  std::size_t wallIndex = cr::kInvalidCreativeWorldLayoutIndex;
  CreativeEditorWorldLayoutWallHandle handle =
      CreativeEditorWorldLayoutWallHandle::None;
};

enum class CreativeEditorWorldLayoutWallManipulationPhase : std::uint8_t {
  Begin,
  Update,
  Commit,
  Cancel,
  Count,
};

struct CreativeEditorWorldLayoutWallManipulationState {
  bool active = false;
  std::uint64_t sourceRevision = 0U;
  CreativeEditorWorldLayoutWallTarget target;
  CreativeEditorWorldLayoutPoint startPoint;
  cr::CreativeTerrainCoord2 originalStart;
  cr::CreativeTerrainCoord2 originalEnd;
  cr::CreativeTerrainCoord2 previewStart;
  cr::CreativeTerrainCoord2 previewEnd;
  double previewOpeningOffsetDeltaCells = 0.0;
  bool previewValid = false;
  std::string reasonCode =
      "creative_editor_world_layout_wall_manipulation_inactive";
};

using CreativeEditorWorldLayoutBuildingBounds =
    cr::CreativeWorldLayoutBuildingBounds;

enum class CreativeEditorWorldLayoutBuildingManipulationPhase : std::uint8_t {
  Begin,
  Update,
  Commit,
  Cancel,
  Count,
};

struct CreativeEditorWorldLayoutBuildingManipulationState {
  bool active = false;
  std::uint64_t sourceRevision = 0U;
  std::size_t buildingIndex = cr::kInvalidCreativeWorldLayoutIndex;
  CreativeEditorWorldLayoutPoint startPoint;
  CreativeEditorWorldLayoutBuildingBounds originalBounds;
  std::int64_t previewDeltaXCells = 0;
  std::int64_t previewDeltaZCells = 0;
  bool previewValid = false;
  std::string reasonCode =
      "creative_editor_world_layout_building_manipulation_inactive";
};

enum class CreativeEditorWorldLayoutBuildingTransformPhase : std::uint8_t {
  Preview,
  Commit,
  Cancel,
  Count,
};

struct CreativeEditorWorldLayoutBuildingTransformState {
  bool active = false;
  std::uint64_t sourceRevision = 0U;
  std::size_t buildingIndex = cr::kInvalidCreativeWorldLayoutIndex;
  cr::CreativeWorldLayoutBuildingTransformOperation operation =
      cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90;
  CreativeEditorWorldLayoutBuildingBounds sourceBounds;
  CreativeEditorWorldLayoutBuildingBounds previewBounds;
  cr::CreativeWorldLayout candidate;
  std::string reasonCode =
      "creative_editor_world_layout_building_transform_inactive";
};

enum class CreativeEditorWorldLayoutGeneratedBuildingOperation
    : std::uint8_t {
  Move,
  RotateLeft90,
  RotateRight90,
  MirrorX,
  MirrorZ,
  Duplicate,
  Count,
};

struct CreativeEditorWorldLayoutGeneratedBuildingDraft {
  bool active = false;
  std::size_t buildingIndex = cr::kInvalidCreativeWorldLayoutIndex;
  std::uint64_t sourceRevision = 0U;
  CreativeEditorWorldLayoutGeneratedBuildingOperation operation =
      CreativeEditorWorldLayoutGeneratedBuildingOperation::Move;
  std::int64_t deltaXCells = 0;
  std::int64_t deltaZCells = 0;
  bool previewReady = false;
};

inline constexpr std::size_t
    kCreativeEditorWorldLayoutBuildingTemplateCapacity = 256U;

struct CreativeEditorWorldLayoutBuildingTemplateLibrary {
  std::filesystem::path root;
  std::vector<cr::CreativeWorldLayoutBuildingTemplate> templates;
  std::uint64_t nextTemplateOrdinal = 1U;
  std::size_t selectedIndex = cr::kInvalidCreativeWorldLayoutIndex;
  std::string statusMessage = "building template library not loaded";
};

struct CreativeEditorWorldLayoutBuildingTemplateLoadReceipt {
  bool requested = false;
  bool accepted = false;
  std::size_t loadedCount = 0U;
  std::size_t rejectedCount = 0U;
  std::string reasonCode =
      "creative_editor_world_layout_building_template_load_not_requested";
};

struct CreativeEditorWorldLayoutBuildingTemplateInstallReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  std::size_t templateIndex = cr::kInvalidCreativeWorldLayoutIndex;
  std::string reasonCode =
      "creative_editor_world_layout_building_template_install_not_requested";
};

enum class CreativeEditorWorldLayoutBuildingTemplatePlacementPhase
    : std::uint8_t {
  Begin,
  Update,
  Transform,
  Commit,
  Cancel,
  Count,
};

struct CreativeEditorWorldLayoutBuildingTemplatePlacementState {
  bool active = false;
  std::uint64_t sourceRevision = 0U;
  std::size_t templateIndex = cr::kInvalidCreativeWorldLayoutIndex;
  cr::CreativeWorldLayoutBuildingTemplate orientedTemplate;
  cr::CreativeTerrainCoord2 anchor;
  cr::CreativeWorldLayoutBuildingBounds previewBounds;
  cr::CreativeWorldLayout candidate;
  std::size_t resultBuildingIndex = cr::kInvalidCreativeWorldLayoutIndex;
  std::uint64_t nextStableOrdinal = 1U;
  bool previewValid = false;
  std::string reasonCode =
      "creative_editor_world_layout_building_template_placement_inactive";
};

struct CreativeEditorWorldLayoutOpeningSettings {
  double centerOffsetCells = 0.0;
  double widthCells = 1.0;
  double sillHeightCells = 0.0;
  double heightCells = 2.1;
  cr::CreativeBuildingOpeningPose pose =
      cr::CreativeBuildingOpeningPose::Closed;
  bool includeInsert = true;
};

struct CreativeEditorWorldLayoutOpeningSettingsDraft {
  bool active = false;
  std::size_t openingIndex = cr::kInvalidCreativeWorldLayoutIndex;
  std::uint64_t sourceRevision = 0U;
  CreativeEditorWorldLayoutOpeningSettings settings;
};

struct CreativeEditorWorldLayoutLevelSettings {
  std::string name;
  double floorTopLayer = 0.0;
  std::uint16_t wallHeightCells =
      cr::kDefaultCreativeWorldLayoutWallHeightCells;
  std::uint16_t floorThicknessLayers = 1U;
  std::uint16_t ceilingThicknessLayers = 1U;
  std::uint16_t roofThicknessLayers = 1U;
  cr::CreativeStructuralRoofStyle roofStyle =
      cr::CreativeStructuralRoofStyle::Flat;
  cr::CreativeStructuralRoofRidgeAxis roofRidgeAxis =
      cr::CreativeStructuralRoofRidgeAxis::X;
  double roofPitchDegrees = cr::kDefaultCreativeStructuralRoofPitchDegrees;
  double roofOverhangCells = 0.0;

  [[nodiscard]] friend bool operator==(
      const CreativeEditorWorldLayoutLevelSettings&,
      const CreativeEditorWorldLayoutLevelSettings&) = default;
};

struct CreativeEditorWorldLayoutLevelSettingsDraft {
  bool active = false;
  std::size_t levelIndex = cr::kInvalidCreativeWorldLayoutIndex;
  std::uint64_t sourceRevision = 0U;
  CreativeEditorWorldLayoutLevelSettings settings;
};

struct CreativeEditorWorldLayoutTerrainProfileSettings {
  cr::CreativeTerrainRecipeKind kind = cr::CreativeTerrainRecipeKind::Hill;
  cr::CreativeTerrainCoord2 center;
  std::uint16_t baseHeightCells = 4U;
  std::uint16_t radiusCells = 4U;
  std::uint16_t amplitudeCells = 4U;
  std::uint16_t spacingCells = 1U;
  cr::CreativeTerrainProfileDirection direction =
      cr::CreativeTerrainProfileDirection::PositiveX;
  std::uint8_t frequency = 1U;

  [[nodiscard]] friend bool operator==(
      CreativeEditorWorldLayoutTerrainProfileSettings,
      CreativeEditorWorldLayoutTerrainProfileSettings) noexcept = default;
};

struct CreativeEditorWorldLayoutTerrainProfileSettingsDraft {
  bool active = false;
  std::size_t profileIndex = cr::kInvalidCreativeWorldLayoutIndex;
  std::uint64_t sourceRevision = 0U;
  CreativeEditorWorldLayoutTerrainProfileSettings settings;
};

struct CreativeEditorWorldLayoutTerrainPathSettings {
  cr::CreativeTerrainRecipeKind kind = cr::CreativeTerrainRecipeKind::Road;
  cr::CreativeTerrainPathElevation elevation =
      cr::CreativeTerrainPathElevation::Level;
  std::uint16_t halfWidthCells = 1U;
  std::uint16_t amplitudeCells = 1U;
  bool paintSurface = true;
  cr::CreativeTerrainMaterial material = cr::CreativeTerrainMaterial::Count;
  std::vector<cr::CreativeTerrainPathPoint> points;

  [[nodiscard]] friend bool operator==(
      const CreativeEditorWorldLayoutTerrainPathSettings&,
      const CreativeEditorWorldLayoutTerrainPathSettings&) = default;
};

struct CreativeEditorWorldLayoutTerrainPathSettingsDraft {
  bool active = false;
  std::size_t pathIndex = cr::kInvalidCreativeWorldLayoutIndex;
  std::uint64_t sourceRevision = 0U;
  CreativeEditorWorldLayoutTerrainPathSettings settings;
};

struct CreativeEditorWorldLayoutObjectSettings {
  cr::CreativeObjectKind kind = cr::CreativeObjectKind::Unknown;
  cr::CreativeObjectLibraryPlacementMode mode =
      cr::CreativeObjectLibraryPlacementMode::Bounds;
  std::string name;
  std::string assetId;
  cr::CreativeBounds boundsCells;
  cr::CreativeVec3 pointCells;
  cr::CreativeBounds assetSourceBoundsMeters;
  bool hasAssetSourceBounds = false;
  double yawRadians = 0.0;
  cr::CreativeVec3 scale{1.0, 1.0, 1.0};
  bool visible = true;

  [[nodiscard]] friend bool operator==(
      const CreativeEditorWorldLayoutObjectSettings& lhs,
      const CreativeEditorWorldLayoutObjectSettings& rhs) noexcept {
    return lhs.kind == rhs.kind && lhs.mode == rhs.mode &&
           lhs.name == rhs.name && lhs.assetId == rhs.assetId &&
           lhs.boundsCells.min.x == rhs.boundsCells.min.x &&
           lhs.boundsCells.min.y == rhs.boundsCells.min.y &&
           lhs.boundsCells.min.z == rhs.boundsCells.min.z &&
           lhs.boundsCells.max.x == rhs.boundsCells.max.x &&
           lhs.boundsCells.max.y == rhs.boundsCells.max.y &&
           lhs.boundsCells.max.z == rhs.boundsCells.max.z &&
           lhs.pointCells.x == rhs.pointCells.x &&
           lhs.pointCells.y == rhs.pointCells.y &&
           lhs.pointCells.z == rhs.pointCells.z &&
           lhs.assetSourceBoundsMeters.min.x ==
               rhs.assetSourceBoundsMeters.min.x &&
           lhs.assetSourceBoundsMeters.min.y ==
               rhs.assetSourceBoundsMeters.min.y &&
           lhs.assetSourceBoundsMeters.min.z ==
               rhs.assetSourceBoundsMeters.min.z &&
           lhs.assetSourceBoundsMeters.max.x ==
               rhs.assetSourceBoundsMeters.max.x &&
           lhs.assetSourceBoundsMeters.max.y ==
               rhs.assetSourceBoundsMeters.max.y &&
           lhs.assetSourceBoundsMeters.max.z ==
               rhs.assetSourceBoundsMeters.max.z &&
           lhs.hasAssetSourceBounds == rhs.hasAssetSourceBounds &&
           lhs.yawRadians == rhs.yawRadians && lhs.scale.x == rhs.scale.x &&
           lhs.scale.y == rhs.scale.y && lhs.scale.z == rhs.scale.z &&
           lhs.visible == rhs.visible;
  }
};

struct CreativeEditorWorldLayoutCatalogPlacementState {
  bool active = false;
  cr::CreativeObjectKind kind = cr::CreativeObjectKind::Unknown;
  std::string assetId;
  std::string label;
  std::string categoryId;
  cr::CreativeBounds sourceBoundsMeters;
  CreativeEditorWorldLayoutCatalogSnapMode snapMode =
      CreativeEditorWorldLayoutCatalogSnapMode::Grid;
  bool wallSideFlipped = false;
  double elevationCells = 0.0;
  double yawDegrees = 0.0;
  cr::CreativeVec3 scale{1.0, 1.0, 1.0};
};

struct CreativeEditorWorldLayoutObjectFootprint {
  bool valid = false;
  std::array<CreativeEditorWorldLayoutPoint, 4U> corners{};
  cr::CreativeBounds axisAlignedBoundsCells;
};

struct CreativeEditorWorldLayoutOpeningHost {
  bool valid = false;
  CreativeEditorWorldLayoutPoint start;
  CreativeEditorWorldLayoutPoint end;
  double lengthCells = 0.0;
  double wallHeightCells = 0.0;
};

struct CreativeEditorWorldLayoutOpeningPlacementPlan {
  bool accepted = false;
  cr::CreativeWorldLayoutOpening opening;
  CreativeEditorWorldLayoutOpeningHost host;
  CreativeEditorWorldLayoutPoint centerPoint;
  CreativeEditorWorldLayoutPoint startPoint;
  CreativeEditorWorldLayoutPoint endPoint;
  double pointerDistanceCells = 0.0;
  std::string_view message = "Choose a wall target";
  std::string_view reasonCode =
      "creative_editor_world_layout_opening_placement_not_requested";
};

struct CreativeEditorWorldLayoutOpeningPlacementRequest {
  CreativeEditorWorldLayoutPoint point;
  cr::CreativeBuildingOpeningKind kind =
      cr::CreativeBuildingOpeningKind::Door;
  double widthCells = 0.0;
  double cutoutBottomCells = 0.0;
  double cutoutHeightCells = 0.0;
  double insertThicknessCells = 0.0;
  bool useExplicitDimensions = false;
  std::string_view insertAssetId;
  cr::CreativeBounds insertAssetSourceBoundsMeters;
  bool hasInsertAssetSourceBounds = false;
  std::string_view label;
};

static_assert(std::is_trivially_copyable_v<
              CreativeEditorWorldLayoutOpeningPlacementRequest>);

struct CreativeEditorWorldLayoutOpeningInsertRequest {
  std::size_t openingIndex = cr::kInvalidCreativeWorldLayoutIndex;
  CreativeEditorWorldLayoutOpeningInsertOperation operation =
      CreativeEditorWorldLayoutOpeningInsertOperation::FitAssetToOpening;
  cr::CreativeBuildingOpeningKind assetKind =
      cr::CreativeBuildingOpeningKind::Door;
  std::string_view assetId;
  cr::CreativeBounds assetSourceBoundsMeters;
  cr::CreativeVec3 assetScale{1.0, 1.0, 1.0};
  double gridCellSizeMeters = 1.0;
};

struct CreativeEditorWorldLayoutOpeningAssetGeometryRequest {
  cr::CreativeBuildingOpeningKind kind =
      cr::CreativeBuildingOpeningKind::Door;
  cr::CreativeBounds sourceBoundsMeters;
  cr::CreativeVec3 scale{1.0, 1.0, 1.0};
  double gridCellSizeMeters = 1.0;
};

struct CreativeEditorWorldLayoutOpeningAssetGeometryPlan {
  bool accepted = false;
  double widthCells = 0.0;
  double cutoutBottomCells = 0.0;
  double cutoutHeightCells = 0.0;
  double insertThicknessCells = 0.0;
  std::string_view reasonCode =
      "creative_editor_world_layout_opening_asset_geometry_not_requested";
};

struct CreativeEditorWorldLayoutOpeningInsertPlan {
  bool accepted = false;
  cr::CreativeWorldLayoutOpening candidate;
  std::string message = "Choose an opening insert";
  std::string reasonCode =
      "creative_editor_world_layout_opening_insert_not_requested";
};

static_assert(std::is_trivially_copyable_v<
              CreativeEditorWorldLayoutOpeningInsertRequest>);
static_assert(std::is_trivially_copyable_v<
              CreativeEditorWorldLayoutOpeningAssetGeometryRequest>);
static_assert(std::is_trivially_copyable_v<
              CreativeEditorWorldLayoutOpeningAssetGeometryPlan>);

struct CreativeEditorWorldLayoutCatalogPlacementPlan {
  bool accepted = false;
  bool hostedOpening = false;
  cr::CreativeWorldLayoutObject object;
  CreativeEditorWorldLayoutObjectFootprint footprint;
  CreativeEditorWorldLayoutOpeningPlacementPlan openingPlacement;
  CreativeEditorWorldLayoutCatalogSnapMode snapMode =
      CreativeEditorWorldLayoutCatalogSnapMode::Grid;
  CreativeEditorWorldLayoutCatalogSnapHostKind snapHostKind =
      CreativeEditorWorldLayoutCatalogSnapHostKind::None;
  std::size_t snapHostIndex = cr::kInvalidCreativeWorldLayoutIndex;
  cr::CreativeWorldLayoutRoomEdge snapRoomEdge =
      cr::CreativeWorldLayoutRoomEdge::Count;
  CreativeEditorWorldLayoutPoint snapSurfacePoint;
  CreativeEditorWorldLayoutPoint snapNormal;
  double snapWallThicknessCells = 0.0;
  double snapContactOffsetCells = 0.0;
  double snapDistanceCells = 0.0;
  std::string message = "Choose a placement target";
  std::string reasonCode =
      "creative_editor_world_layout_catalog_placement_not_requested";
};

struct CreativeEditorWorldLayoutObjectSettingsDraft {
  bool active = false;
  std::size_t objectIndex = cr::kInvalidCreativeWorldLayoutIndex;
  std::uint64_t sourceRevision = 0U;
  CreativeEditorWorldLayoutObjectSettings settings;
};

struct CreativeEditorWorldLayoutObjectManipulationState {
  bool active = false;
  std::uint64_t sourceRevision = 0U;
  std::size_t objectIndex = cr::kInvalidCreativeWorldLayoutIndex;
  std::string stableKey;
  CreativeEditorWorldLayoutPoint startPoint;
  CreativeEditorWorldLayoutObjectSettings originalSettings;
  CreativeEditorWorldLayoutObjectSettings previewSettings;
  bool previewValid = false;
  std::string reasonCode =
      "creative_editor_world_layout_object_manipulation_inactive";
};

enum class CreativeEditorWorldLayoutOpeningHandle : std::uint8_t {
  None,
  Move,
  Start,
  End,
  Count,
};

struct CreativeEditorWorldLayoutOpeningTarget {
  std::size_t openingIndex = cr::kInvalidCreativeWorldLayoutIndex;
  CreativeEditorWorldLayoutOpeningHandle handle =
      CreativeEditorWorldLayoutOpeningHandle::None;
};

enum class CreativeEditorWorldLayoutOpeningManipulationPhase : std::uint8_t {
  Begin,
  Update,
  Commit,
  Cancel,
  Count,
};

struct CreativeEditorWorldLayoutOpeningManipulationState {
  bool active = false;
  std::uint64_t sourceRevision = 0U;
  CreativeEditorWorldLayoutOpeningTarget target;
  double startPointerOffsetCells = 0.0;
  double originalCenterOffsetCells = 0.0;
  double originalWidthCells = 0.0;
  double previewCenterOffsetCells = 0.0;
  double previewWidthCells = 0.0;
  bool previewValid = false;
  std::string reasonCode =
      "creative_editor_world_layout_opening_manipulation_inactive";
};

struct CreativeEditorWorldLayoutElevationCache {
  bool valid = false;
  std::uint64_t sourceRevision = 0U;
  std::size_t buildingIndex = cr::kInvalidCreativeWorldLayoutIndex;
  CreativeEditorWorldLayoutElevationAxis axis =
      CreativeEditorWorldLayoutElevationAxis::X;
  cr::CreativeVec3 gridOrigin;
  double gridCellSizeMeters = 0.0;
  CreativeEditorWorldLayoutElevationProjection projection;
};

struct CreativeEditorWorldLayoutElevationManipulationState {
  bool active = false;
  std::uint64_t sourceRevision = 0U;
  CreativeEditorWorldLayoutElevationHandle handle;
  CreativeEditorWorldLayoutElevationEditResult preview;
};

enum class CreativeEditorWorldLayoutGesturePhase : std::uint8_t {
  Begin,
  Commit,
  Cancel,
  Count,
};

enum class CreativeEditorWorldLayoutLevelOperation : std::uint8_t {
  Select,
  Add,
  Duplicate,
  MoveEarlier,
  MoveLater,
  Delete,
  Count,
};

struct CreativeEditorWorldLayoutSnapshot {
  cr::CreativeWorldLayout source;
  std::uint64_t revision = 1U;
  std::uint64_t savedRevision = 1U;
  std::uint64_t generatedRevision = 1U;
  std::uint64_t nextStableOrdinal = 1U;
};

struct CreativeEditorWorldLayoutSourceHistoryEntry {
  CreativeEditorWorldLayoutSnapshot snapshot;
  CreativeEditorWorldLayoutSelection selection;
  std::size_t activeLevelIndex = cr::kInvalidCreativeWorldLayoutIndex;
  std::string source;
};

struct CreativeEditorWorldLayoutSourceHistory {
  std::vector<CreativeEditorWorldLayoutSourceHistoryEntry> undoEntries;
  std::vector<CreativeEditorWorldLayoutSourceHistoryEntry> redoEntries;
  CreativeEditorWorldLayoutSourceHistoryEntry current;
  std::size_t maxDepth = 32U;
};

struct CreativeEditorWorldLayoutDeferredSourceHistory {
  bool active = false;
  CreativeEditorWorldLayoutSourceHistory history;
};

struct CreativeEditorWorldLayoutConflictReviewState {
  std::uint64_t diagnosticBuildCount = 0U;
  std::vector<cr::CreativeWorldLayoutConflictDecision> decisions;
  std::vector<cr::CreativeWorldLayoutTerrainConflictDecision> terrainDecisions;
};

struct CreativeEditorWorldLayoutState {
  cr::CreativeWorldLayout source;
  // Transient identity for the installed source. A replacement may reuse the
  // same numeric revision, so revision-owned UI caches key on both values.
  std::uint64_t sourceEpoch = 1U;
  std::uint64_t revision = 1U;
  std::uint64_t savedRevision = 1U;
  std::uint64_t generatedRevision = 1U;
  std::uint64_t nextStableOrdinal = 1U;
  CreativeEditorWorldLayoutSnapshot generatedBaseline;
  CreativeEditorWorldLayoutSourceHistory sourceHistory;
  CreativeEditorWorldLayoutDeferredSourceHistory deferredSourceHistory;

  CreativeEditorWorldLayoutTool tool = CreativeEditorWorldLayoutTool::Select;
  std::size_t activeLevelIndex = cr::kInvalidCreativeWorldLayoutIndex;
  CreativeEditorWorldLayoutSelection selection;
  bool anchorActive = false;
  cr::CreativeTerrainCoord2 anchor{};
  CreativeEditorWorldLayoutRoomManipulationState roomManipulation;
  CreativeEditorWorldLayoutVerticalConnectorManipulationState
      verticalConnectorManipulation;
  CreativeEditorWorldLayoutBoxManipulationState boxManipulation;
  CreativeEditorWorldLayoutWallManipulationState wallManipulation;
  CreativeEditorWorldLayoutBuildingManipulationState buildingManipulation;
  CreativeEditorWorldLayoutBuildingTransformState buildingTransform;
  CreativeEditorWorldLayoutGeneratedBuildingDraft generatedBuildingDraft;
  CreativeEditorWorldLayoutBuildingTemplateLibrary buildingTemplates;
  CreativeEditorWorldLayoutBuildingTemplatePlacementState
      buildingTemplatePlacement;
  CreativeEditorWorldLayoutOpeningManipulationState openingManipulation;
  CreativeEditorWorldLayoutRoomSettingsDraft roomSettingsDraft;
  CreativeEditorWorldLayoutVerticalConnectorSettingsDraft
      verticalConnectorSettingsDraft;
  CreativeEditorWorldLayoutBoxSettingsDraft boxSettingsDraft;
  CreativeEditorWorldLayoutWallSettingsDraft wallSettingsDraft;
  CreativeEditorWorldLayoutOpeningSettingsDraft openingSettingsDraft;
  CreativeEditorWorldLayoutLevelSettingsDraft levelSettingsDraft;
  CreativeEditorWorldLayoutLevelSettingsDraft generatedLevelSettingsDraft;
  CreativeEditorWorldLayoutTerrainProfileSettingsDraft
      terrainProfileSettingsDraft;
  CreativeEditorWorldLayoutTerrainPathSettingsDraft terrainPathSettingsDraft;
  CreativeEditorWorldLayoutObjectSettingsDraft objectSettingsDraft;
  CreativeEditorWorldLayoutObjectManipulationState objectManipulation;
  CreativeEditorWorldLayoutAssetCategory assetCategory =
      CreativeEditorWorldLayoutAssetCategory::Architecture;
  std::string assetQuery;
  CreativeEditorWorldLayoutCatalogPlacementState catalogPlacement;

  CreativeEditorWorldLayoutViewMode viewMode =
      CreativeEditorWorldLayoutViewMode::Plan;
  CreativeEditorWorldLayoutElevationAxis elevationAxis =
      CreativeEditorWorldLayoutElevationAxis::X;
  CreativeEditorWorldLayoutElevationCache elevationCache;
  CreativeEditorWorldLayoutElevationManipulationState elevationManipulation;
  CreativeEditorWorldLayoutDiagnosticCache diagnosticCache;
  CreativeEditorWorldLayoutConflictReviewState conflictReview;

  bool previewVisible = false;
  std::uint64_t previewLayoutRevision = 0U;
  cr::CreativeWorldLayoutPreviewResult preview;
  std::string statusMessage = "layout ready";

  // Canvas-only view state. It is neither document nor layout truth.
  float canvasPixelsPerCell = 28.0F;
  float canvasPanX = 0.0F;
  float canvasPanZ = 0.0F;
  float elevationPixelsPerCell = 28.0F;
  float elevationPanHorizontal = 0.0F;
  float elevationPanY = 0.0F;
};

struct CreativeEditorWorldLayoutEditReceipt {
  bool accepted = false;
  bool changed = false;
  std::string reasonCode = "creative_editor_world_layout_not_requested";
};

struct CreativeEditorWorldLayoutPreviewReceipt {
  bool accepted = false;
  bool changed = false;
  cr::CreativeWorldLayoutStatus status =
      cr::CreativeWorldLayoutStatus::NotRequested;
  std::string reasonCode = "creative_editor_world_layout_preview_not_requested";
};

struct CreativeEditorWorldLayoutApplyReceipt {
  bool accepted = false;
  bool changed = false;
  cr::CreativeWorldLayoutApplyReceipt apply;
  std::string reasonCode = "creative_editor_world_layout_apply_not_requested";
};

struct CreativeEditorWorldLayoutAdoptionReceipt {
  bool accepted = false;
  bool changed = false;
  cr::CreativeWorldLayoutTable table = cr::CreativeWorldLayoutTable::None;
  std::size_t index = cr::kInvalidCreativeWorldLayoutIndex;
  cr::CreativeWorldLayoutApplyReceipt apply;
  std::string reasonCode =
      "creative_editor_world_layout_adoption_not_requested";
};

[[nodiscard]] const char* creativeEditorWorldLayoutToolLabel(
    CreativeEditorWorldLayoutTool tool) noexcept;
[[nodiscard]] bool creativeEditorWorldLayoutToolIsVerticalConnector(
    CreativeEditorWorldLayoutTool tool) noexcept;
[[nodiscard]] const char* creativeEditorWorldLayoutPaletteCategoryLabel(
    CreativeEditorWorldLayoutPaletteCategory category) noexcept;
[[nodiscard]] std::span<const CreativeEditorWorldLayoutPaletteEntry>
creativeEditorWorldLayoutPaletteEntries() noexcept;
[[nodiscard]] const char* creativeEditorWorldLayoutAssetCategoryLabel(
    CreativeEditorWorldLayoutAssetCategory category) noexcept;
[[nodiscard]] const char* creativeEditorWorldLayoutCatalogSnapModeLabel(
    CreativeEditorWorldLayoutCatalogSnapMode mode) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutAssetCategory
classifyCreativeEditorWorldLayoutAsset(
    const cr::CreativeCatalogEntry& entry) noexcept;
[[nodiscard]] bool creativeEditorWorldLayoutCatalogAssetSupportsWallSnap(
    std::string_view categoryId) noexcept;
[[nodiscard]] bool creativeEditorWorldLayoutCatalogAssetIsHostedOpening(
    std::string_view categoryId) noexcept;
[[nodiscard]] bool creativeEditorWorldLayoutCatalogAssetMatchesOpening(
    std::string_view categoryId,
    cr::CreativeBuildingOpeningKind kind) noexcept;
[[nodiscard]] bool creativeEditorWorldLayoutAssetMatchesQuery(
    const cr::CreativeCatalogEntry& entry, std::string_view query);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
selectCreativeEditorWorldLayoutCatalogAsset(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeCatalogEntry& entry);
[[nodiscard]] CreativeEditorWorldLayoutObjectFootprint
planCreativeEditorWorldLayoutObjectFootprint(
    const cr::CreativeWorldLayoutObject& object,
    cr::CreativeGridSettings grid) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutCatalogPlacementPlan
planCreativeEditorWorldLayoutCatalogPlacement(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point,
    cr::CreativeGridSettings grid);

void resetCreativeEditorWorldLayout(CreativeEditorWorldLayoutState& state,
                                    std::string layoutKey = "world_layout");
void installCreativeEditorWorldLayout(CreativeEditorWorldLayoutState& state,
                                      cr::CreativeWorldLayout layout);
void markCreativeEditorWorldLayoutSaved(
    CreativeEditorWorldLayoutState& state) noexcept;

[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
focusCreativeEditorWorldLayoutSource(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
selectCreativeEditorWorldLayoutSource(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index);
[[nodiscard]] cr::CreativeWorldLayoutTable
creativeEditorWorldLayoutSelectionTable(
    CreativeEditorWorldLayoutSelectionKind kind) noexcept;

[[nodiscard]] bool creativeEditorWorldLayoutSourceCanRename(
    cr::CreativeWorldLayoutTable table) noexcept;
[[nodiscard]] bool creativeEditorWorldLayoutSourceCanDuplicate(
    cr::CreativeWorldLayoutTable table) noexcept;
[[nodiscard]] bool creativeEditorWorldLayoutSourceCanDelete(
    cr::CreativeWorldLayoutTable table) noexcept;
[[nodiscard]] bool creativeEditorWorldLayoutSourceStableKeyMatches(
    const CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index,
    std::string_view stableKey) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
renameCreativeEditorWorldLayoutSource(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index,
    std::string name);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
duplicateCreativeEditorWorldLayoutSource(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
deleteCreativeEditorWorldLayoutSource(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index);

// Links a picked generated object back to the synchronized 2D source symbol.
// The ordinary document selection remains available to 3D tools.
[[nodiscard]] bool selectCreativeEditorWorldLayoutObjectSource(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeObject& object);
[[nodiscard]] bool selectCreativeEditorWorldLayoutObjectSource(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeObject& object,
    cr::CreativeGridSettings grid,
    cr::CreativeVec3 worldPoint);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
focusCreativeEditorWorldLayoutObjectSource(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeObject& object);
[[nodiscard]] CreativeEditorWorldLayoutAdoptionReceipt
adoptCreativeEditorWorldLayoutObjectSource(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeAppState& appState,
    cr::CreativeObjectId objectId);

[[nodiscard]] bool creativeEditorWorldLayoutDirty(
    const CreativeEditorWorldLayoutState& state) noexcept;
[[nodiscard]] bool creativeEditorWorldLayoutPreviewActive(
    const CreativeEditorWorldLayoutState& state) noexcept;
[[nodiscard]] const cr::CreativeDocument&
creativeEditorWorldLayoutRenderDocument(
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& liveDocument) noexcept;
[[nodiscard]] const cr::CreativeWorldLayout&
creativeEditorWorldLayoutDisplaySource(
    const CreativeEditorWorldLayoutState& state) noexcept;

[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutTool(CreativeEditorWorldLayoutState& state,
                                 CreativeEditorWorldLayoutTool tool);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutPoint(CreativeEditorWorldLayoutState& state,
                                    CreativeEditorWorldLayoutPoint point);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutPoint(CreativeEditorWorldLayoutState& state,
                                    CreativeEditorWorldLayoutPoint point,
                                    cr::CreativeGridSettings grid);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutGesture(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutGesturePhase phase,
    CreativeEditorWorldLayoutPoint point = {});
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
createCreativeEditorWorldLayoutBuildingShell(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutRoomSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
createCreativeEditorWorldLayoutRoom(
    CreativeEditorWorldLayoutState& state, std::size_t levelIndex,
    CreativeEditorWorldLayoutRoomSettings settings);
[[nodiscard]] bool readCreativeEditorWorldLayoutRoomSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t roomIndex,
    CreativeEditorWorldLayoutRoomSettings& output) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutRoomSettings(
    CreativeEditorWorldLayoutState& state, std::size_t roomIndex,
    CreativeEditorWorldLayoutRoomSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutApplyReceipt
applyCreativeEditorWorldLayoutRoomSettingsToDocument(
    CreativeEditorWorldLayoutState& state, cr::CreativeAppState& appState,
    std::size_t roomIndex, CreativeEditorWorldLayoutRoomSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutPreviewReceipt
previewCreativeEditorWorldLayoutRoomSettings(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document, std::size_t roomIndex,
    CreativeEditorWorldLayoutRoomSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutRoomTarget
findCreativeEditorWorldLayoutRoomTarget(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutRoomManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutRoomManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point = {},
    double toleranceCells = 0.25);
[[nodiscard]] bool creativeEditorWorldLayoutVerticalConnectorOnActiveLevel(
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeWorldLayout& source,
    std::size_t connectorIndex) noexcept;
[[nodiscard]] bool resolveCreativeEditorWorldLayoutVerticalConnectorAxis(
    cr::CreativeWorldLayoutRect footprint,
    cr::CreativeWorldLayoutVerticalDirection direction,
    CreativeEditorWorldLayoutPoint& low,
    CreativeEditorWorldLayoutPoint& high) noexcept;
[[nodiscard]] bool
resolveCreativeEditorWorldLayoutVerticalConnectorDirectionHandle(
    cr::CreativeWorldLayoutRect footprint,
    cr::CreativeWorldLayoutVerticalDirection direction,
    CreativeEditorWorldLayoutPoint& output) noexcept;
[[nodiscard]] bool readCreativeEditorWorldLayoutVerticalConnectorSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t connectorIndex,
    CreativeEditorWorldLayoutVerticalConnectorSettings& output) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutVerticalConnectorSettings(
    CreativeEditorWorldLayoutState& state, std::size_t connectorIndex,
    CreativeEditorWorldLayoutVerticalConnectorSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutApplyReceipt
applyCreativeEditorWorldLayoutVerticalConnectorSettingsToDocument(
    CreativeEditorWorldLayoutState& state, cr::CreativeAppState& appState,
    std::size_t connectorIndex,
    CreativeEditorWorldLayoutVerticalConnectorSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutPreviewReceipt
previewCreativeEditorWorldLayoutVerticalConnectorSettings(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document, std::size_t connectorIndex,
    CreativeEditorWorldLayoutVerticalConnectorSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutVerticalConnectorTarget
findCreativeEditorWorldLayoutVerticalConnectorTarget(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutVerticalConnectorManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutVerticalConnectorManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point = {},
    double toleranceCells = 0.25);
[[nodiscard]] bool readCreativeEditorWorldLayoutBoxSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t boxIndex,
    CreativeEditorWorldLayoutBoxSettings& output) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutBoxSettings(
    CreativeEditorWorldLayoutState& state, std::size_t boxIndex,
    CreativeEditorWorldLayoutBoxSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutBoxTarget
findCreativeEditorWorldLayoutBoxTarget(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutBoxManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutBoxManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point = {},
    double toleranceCells = 0.25);
[[nodiscard]] bool readCreativeEditorWorldLayoutWallSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t wallIndex,
    CreativeEditorWorldLayoutWallSettings& output) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutWallSettings(
    CreativeEditorWorldLayoutState& state, std::size_t wallIndex,
    CreativeEditorWorldLayoutWallSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutApplyReceipt
applyCreativeEditorWorldLayoutWallSettingsToDocument(
    CreativeEditorWorldLayoutState& state, cr::CreativeAppState& appState,
    std::size_t wallIndex, CreativeEditorWorldLayoutWallSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutPreviewReceipt
previewCreativeEditorWorldLayoutWallSettings(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document, std::size_t wallIndex,
    CreativeEditorWorldLayoutWallSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutWallTarget
findCreativeEditorWorldLayoutWallTarget(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutWallManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutWallManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point = {},
    double toleranceCells = 0.25);
[[nodiscard]] std::size_t creativeEditorWorldLayoutSelectedBuilding(
    const CreativeEditorWorldLayoutState& state) noexcept;
[[nodiscard]] bool readCreativeEditorWorldLayoutBuildingBounds(
    const CreativeEditorWorldLayoutState& state, std::size_t buildingIndex,
    CreativeEditorWorldLayoutBuildingBounds& output) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
selectCreativeEditorWorldLayoutBuilding(CreativeEditorWorldLayoutState& state,
                                        std::size_t buildingIndex);
void repairCreativeEditorWorldLayoutActiveLevel(
    CreativeEditorWorldLayoutState& state,
    std::size_t preferredBuildingIndex =
        cr::kInvalidCreativeWorldLayoutIndex) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutLevelOperation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutLevelOperation operation,
    std::size_t buildingIndex = cr::kInvalidCreativeWorldLayoutIndex,
    std::size_t levelIndex = cr::kInvalidCreativeWorldLayoutIndex);
[[nodiscard]] bool readCreativeEditorWorldLayoutLevelSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t levelIndex,
    CreativeEditorWorldLayoutLevelSettings& output);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutLevelSettings(
    CreativeEditorWorldLayoutState& state, std::size_t levelIndex,
    CreativeEditorWorldLayoutLevelSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutApplyReceipt
applyCreativeEditorWorldLayoutLevelSettingsToDocument(
    CreativeEditorWorldLayoutState& state, cr::CreativeAppState& appState,
    std::size_t levelIndex, CreativeEditorWorldLayoutLevelSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutPreviewReceipt
previewCreativeEditorWorldLayoutLevelSettings(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document, std::size_t levelIndex,
    CreativeEditorWorldLayoutLevelSettings settings);
[[nodiscard]] bool readCreativeEditorWorldLayoutTerrainProfileSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t profileIndex,
    CreativeEditorWorldLayoutTerrainProfileSettings& output) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutTerrainProfileSettings(
    CreativeEditorWorldLayoutState& state, std::size_t profileIndex,
    CreativeEditorWorldLayoutTerrainProfileSettings settings);
[[nodiscard]] bool readCreativeEditorWorldLayoutTerrainPathSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t pathIndex,
    CreativeEditorWorldLayoutTerrainPathSettings& output);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutTerrainPathSettings(
    CreativeEditorWorldLayoutState& state, std::size_t pathIndex,
    CreativeEditorWorldLayoutTerrainPathSettings settings);
[[nodiscard]] bool readCreativeEditorWorldLayoutObjectSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t objectIndex,
    CreativeEditorWorldLayoutObjectSettings& output);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutObjectSettings(
    CreativeEditorWorldLayoutState& state, std::size_t objectIndex,
    CreativeEditorWorldLayoutObjectSettings settings);
[[nodiscard]] std::size_t findCreativeEditorWorldLayoutObjectAt(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point) noexcept;
[[nodiscard]] std::size_t findCreativeEditorWorldLayoutObjectAt(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point,
    cr::CreativeGridSettings grid) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
beginCreativeEditorWorldLayoutObjectManipulation(
    CreativeEditorWorldLayoutState& state, std::size_t objectIndex,
    CreativeEditorWorldLayoutPoint point);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
updateCreativeEditorWorldLayoutObjectManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
cancelCreativeEditorWorldLayoutObjectManipulation(
    CreativeEditorWorldLayoutState& state) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
clearCreativeEditorWorldLayoutSelection(
    CreativeEditorWorldLayoutState& state);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutBuildingManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutBuildingManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point = {},
    double toleranceCells = 0.25);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutBuildingTransform(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutBuildingTransformPhase phase,
    cr::CreativeWorldLayoutBuildingTransformOperation operation =
        cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90);
[[nodiscard]] CreativeEditorWorldLayoutPreviewReceipt
previewCreativeEditorWorldLayoutGeneratedBuildingOperation(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document, std::size_t buildingIndex,
    CreativeEditorWorldLayoutGeneratedBuildingOperation operation,
    std::int64_t deltaXCells = 0, std::int64_t deltaZCells = 0);
[[nodiscard]] CreativeEditorWorldLayoutApplyReceipt
applyCreativeEditorWorldLayoutGeneratedBuildingOperationToDocument(
    CreativeEditorWorldLayoutState& state, cr::CreativeAppState& appState,
    std::size_t buildingIndex,
    CreativeEditorWorldLayoutGeneratedBuildingOperation operation,
    std::int64_t deltaXCells = 0, std::int64_t deltaZCells = 0);
[[nodiscard]] bool defaultCreativeEditorWorldLayoutBuildingDuplicateOffset(
    const CreativeEditorWorldLayoutState& state, std::size_t buildingIndex,
    std::int64_t& deltaXCells, std::int64_t& deltaZCells) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
duplicateCreativeEditorWorldLayoutBuilding(
    CreativeEditorWorldLayoutState& state, std::size_t buildingIndex,
    std::int64_t deltaXCells, std::int64_t deltaZCells);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
deleteCreativeEditorWorldLayoutBuilding(CreativeEditorWorldLayoutState& state,
                                        std::size_t buildingIndex);
[[nodiscard]] CreativeEditorWorldLayoutBuildingTemplateLoadReceipt
loadCreativeEditorWorldLayoutBuildingTemplateLibrary(
    CreativeEditorWorldLayoutBuildingTemplateLibrary& library,
    const std::filesystem::path& creativeSaveRoot);
[[nodiscard]] CreativeEditorWorldLayoutBuildingTemplateInstallReceipt
installCreativeEditorWorldLayoutBuildingTemplate(
    CreativeEditorWorldLayoutBuildingTemplateLibrary& library,
    const cr::CreativeWorldLayoutBuildingTemplate& sourceTemplate);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
captureCreativeEditorWorldLayoutBuildingTemplate(
    CreativeEditorWorldLayoutState& state,
    std::size_t buildingIndex,
    std::string label = {});
[[nodiscard]] cr::CreativeWorldLayoutBuildingTemplateSyncReceipt
inspectCreativeEditorWorldLayoutBuildingTemplateSync(
    const CreativeEditorWorldLayoutState& state,
    std::size_t buildingIndex);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
updateCreativeEditorWorldLayoutBuildingTemplateFromInstance(
    CreativeEditorWorldLayoutState& state,
    std::size_t buildingIndex);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
refreshCreativeEditorWorldLayoutBuildingTemplateInstances(
    CreativeEditorWorldLayoutState& state,
    std::size_t buildingIndex,
    cr::CreativeWorldLayoutBuildingTemplateRefreshMode mode);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
selectCreativeEditorWorldLayoutBuildingTemplate(
    CreativeEditorWorldLayoutState& state,
    std::size_t templateIndex);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutBuildingTemplatePlacement(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutBuildingTemplatePlacementPhase phase,
    CreativeEditorWorldLayoutPoint point = {},
    cr::CreativeWorldLayoutBuildingTransformOperation operation =
        cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90);
[[nodiscard]] bool readCreativeEditorWorldLayoutOpeningSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t openingIndex,
    CreativeEditorWorldLayoutOpeningSettings& output) noexcept;
// Hover-time query: O(walls + room edges + openings), stable first-host ties,
// and no layout copies or container allocations.
[[nodiscard]] CreativeEditorWorldLayoutOpeningPlacementPlan
planCreativeEditorWorldLayoutOpeningPlacement(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point,
    cr::CreativeBuildingOpeningKind kind);
[[nodiscard]] CreativeEditorWorldLayoutOpeningPlacementPlan
planCreativeEditorWorldLayoutOpeningPlacement(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutOpeningPlacementRequest& request);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutOpeningPlacement(
    CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutOpeningPlacementRequest& request);
[[nodiscard]] CreativeEditorWorldLayoutOpeningHost
resolveCreativeEditorWorldLayoutOpeningHost(
    const CreativeEditorWorldLayoutState& state,
    std::size_t openingIndex) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutOpeningTarget
findCreativeEditorWorldLayoutOpeningTarget(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutOpeningSettings(
    CreativeEditorWorldLayoutState& state, std::size_t openingIndex,
    CreativeEditorWorldLayoutOpeningSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutApplyReceipt
applyCreativeEditorWorldLayoutOpeningSettingsToDocument(
    CreativeEditorWorldLayoutState& state, cr::CreativeAppState& appState,
    std::size_t openingIndex,
    CreativeEditorWorldLayoutOpeningSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutPreviewReceipt
previewCreativeEditorWorldLayoutOpeningSettings(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document, std::size_t openingIndex,
    CreativeEditorWorldLayoutOpeningSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutOpeningInsertPlan
planCreativeEditorWorldLayoutOpeningInsert(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutOpeningInsertRequest& request);
[[nodiscard]] CreativeEditorWorldLayoutOpeningAssetGeometryPlan
planCreativeEditorWorldLayoutOpeningAssetGeometry(
    const CreativeEditorWorldLayoutOpeningAssetGeometryRequest& request)
    noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutOpeningInsert(
    CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutOpeningInsertRequest& request);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutAssetBoundsUpdates(
    CreativeEditorWorldLayoutState& state,
    std::span<const CreativeEditorWorldLayoutAssetBoundsUpdate> updates);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutOpeningManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutOpeningManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point = {},
    double toleranceCells = 0.25);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
deleteCreativeEditorWorldLayoutSelection(CreativeEditorWorldLayoutState& state);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
cancelCreativeEditorWorldLayoutPreview(
    CreativeEditorWorldLayoutState& state) noexcept;

[[nodiscard]] CreativeEditorWorldLayoutPreviewReceipt
previewCreativeEditorWorldLayout(CreativeEditorWorldLayoutState& state,
                                 const cr::CreativeDocument& document);
[[nodiscard]] cr::CreativeWorldLayoutTerrainReconciliationResult
reconcileCreativeEditorWorldLayoutTerrain(
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document,
    const cr::CreativeWorldLayout& desiredLayout,
    std::span<const cr::CreativeWorldLayoutTerrainConflictDecision> decisions =
        {});
[[nodiscard]] CreativeEditorWorldLayoutApplyReceipt
confirmCreativeEditorWorldLayout(CreativeEditorWorldLayoutState& state,
                                 cr::CreativeAppState& appState,
                                 std::span<const
                                     cr::CreativeWorldLayoutConflictDecision>
                                     conflictDecisions = {},
                                 std::span<const
                                     cr::CreativeWorldLayoutTerrainConflictDecision>
                                     terrainConflictDecisions = {});

}  // namespace iggy3d_creative_app
