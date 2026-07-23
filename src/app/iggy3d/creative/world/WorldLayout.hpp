#pragma once

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/recipes/BridgeRecipe.hpp"
#include "app/iggy3d/creative/recipes/BuildingRecipe.hpp"
#include "app/iggy3d/creative/recipes/ObjectLibraryRecipe.hpp"
#include "app/iggy3d/creative/recipes/RetainingEdgeRecipe.hpp"
#include "app/iggy3d/creative/recipes/StructuralRoofRecipe.hpp"
#include "app/iggy3d/creative/recipes/TerrainLandform.hpp"
#include "app/iggy3d/creative/recipes/TerrainPathSource.hpp"
#include "app/iggy3d/creative/recipes/TerrainRecipe.hpp"
#include "app/iggy3d/creative/recipes/WatercourseRecipe.hpp"
#include "app/iggy3d/creative/world/WorldLayoutReconciliation.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

inline constexpr std::uint32_t kCreativeWorldLayoutSchemaVersion = 29U;
inline constexpr std::size_t kInvalidCreativeWorldLayoutIndex =
    std::numeric_limits<std::size_t>::max();
inline constexpr std::uint16_t kDefaultCreativeWorldLayoutWallHeightCells = 3U;
inline constexpr double kDefaultCreativeWorldLayoutWallThicknessCells = 0.25;
inline constexpr double kMaximumCreativeWorldLayoutRoofOverhangCells = 16.0;

// Layout coordinates are grid-line coordinates. Rect maximums are exclusive,
// so {0, 0}->{10, 8} describes a ten-by-eight-cell floor without half-cell
// conventions leaking into the 2D editor.
struct CreativeWorldLayoutRect {
  CreativeTerrainCoord2 minimum{};
  CreativeTerrainCoord2 maximum{};
};

enum class CreativeWorldLayoutGroundingMode : std::uint8_t {
  Absolute,
  Foundation,
  Count,
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
  CreativeWorldLayoutGroundingMode groundingMode =
      CreativeWorldLayoutGroundingMode::Absolute;
  std::uint16_t maximumGroundReliefCells = 4U;
};

struct CreativeWorldLayoutBox {
  std::size_t buildingIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeObjectKind kind = CreativeObjectKind::Floor;
  std::string stableKey;
  std::string name;
  CreativeWorldLayoutRect footprint;
  // Floor: finished top plane. Ceiling/roof: support plane. Other box kinds:
  // lower volume plane. Values are expressed in document grid layers.
  double anchorLayer = 0.0;
  // Horizontal structural surfaces use descriptor-sized layers. Other box
  // kinds use document grid-cell layers.
  std::uint16_t layerCount = 1U;
};

// Vertical building truth is normalized here instead of repeated on every
// room. Levels remain a flat table so building/template operations can remap
// them deterministically alongside rooms, walls, and openings.
struct CreativeWorldLayoutLevel {
  std::size_t buildingIndex = kInvalidCreativeWorldLayoutIndex;
  std::string stableKey;
  std::string name;
  double floorTopLayer = 0.0;
  std::uint16_t wallHeightCells =
      kDefaultCreativeWorldLayoutWallHeightCells;
  std::uint16_t floorThicknessLayers = 1U;
  std::uint16_t ceilingThicknessLayers = 1U;
  std::uint16_t roofThicknessLayers = 1U;
  CreativeStructuralRoofStyle roofStyle = CreativeStructuralRoofStyle::Flat;
  CreativeStructuralRoofRidgeAxis roofRidgeAxis =
      CreativeStructuralRoofRidgeAxis::X;
  double roofPitchDegrees = kDefaultCreativeStructuralRoofPitchDegrees;
  double roofOverhangCells = 0.0;
  CreativeStructuralRoofSlopeDirection roofSlopeDirection =
      CreativeStructuralRoofSlopeDirection::PositiveZ;
  CreativeStructuralMaterial roofMaterial =
      CreativeStructuralMaterial::Blockout;
};

enum class CreativeWorldLayoutRoomType : std::uint8_t {
  Generic,
  Living,
  Kitchen,
  Bedroom,
  Bathroom,
  Corridor,
  Storage,
  Utility,
  Count,
};

struct CreativeWorldLayoutRoom {
  std::size_t buildingIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t levelIndex = kInvalidCreativeWorldLayoutIndex;
  std::string stableKey;
  std::string name;
  CreativeWorldLayoutRect footprint;
  double wallThicknessCells =
      kDefaultCreativeWorldLayoutWallThicknessCells;
  CreativeWorldLayoutRoomType type = CreativeWorldLayoutRoomType::Generic;
};

// Rooms with explicit topology share vertices and edges through these flat
// tables. room.footprint remains a validated bounds cache and the compatibility
// representation for layouts authored before schema 12.
struct CreativeWorldLayoutTopologyVertex {
  std::size_t levelIndex = kInvalidCreativeWorldLayoutIndex;
  std::string stableKey;
  CreativeTerrainCoord2 position{};
};

enum class CreativeWorldLayoutWallProfile : std::uint8_t {
  Automatic,
  Exterior,
  Interior,
  Count,
};

// Straight orthogonal wall spans use a square overlap at shared vertices. The
// enum is explicit so future join geometry can extend the contract without
// changing the meaning of existing source records.
enum class CreativeWorldLayoutWallJoinStyle : std::uint8_t {
  Square,
  Count,
};

struct CreativeWorldLayoutTopologyEdge {
  std::size_t levelIndex = kInvalidCreativeWorldLayoutIndex;
  std::string stableKey;
  std::size_t startVertexIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t endVertexIndex = kInvalidCreativeWorldLayoutIndex;
  double wallThicknessCells =
      kDefaultCreativeWorldLayoutWallThicknessCells;
  // Zero inherits the owning level height. A positive value is an intentional
  // per-wall exception and remains attached to this stable edge identity.
  std::uint16_t wallHeightCells = 0U;
  CreativeWorldLayoutWallProfile profile =
      CreativeWorldLayoutWallProfile::Automatic;
  CreativeStructuralMaterial material =
      CreativeStructuralMaterial::Blockout;
  CreativeWorldLayoutWallJoinStyle joinStyle =
      CreativeWorldLayoutWallJoinStyle::Square;
};

struct CreativeWorldLayoutRoomBoundary {
  std::size_t roomIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t topologyEdgeIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t order = 0U;
  bool reversed = false;
};

enum class CreativeWorldLayoutVerticalConnectorKind : std::uint8_t {
  Stair,
  Ramp,
  Count,
};

// Direction points from the low end on the lower level to the high end on the
// upper level. The authored footprint is also the opening cut from both slabs.
enum class CreativeWorldLayoutVerticalDirection : std::uint8_t {
  PositiveX,
  NegativeX,
  PositiveZ,
  NegativeZ,
  Count,
};

struct CreativeWorldLayoutVerticalConnector {
  std::size_t buildingIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t lowerRoomIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t upperRoomIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayoutVerticalConnectorKind kind =
      CreativeWorldLayoutVerticalConnectorKind::Stair;
  CreativeWorldLayoutVerticalDirection direction =
      CreativeWorldLayoutVerticalDirection::PositiveZ;
  std::string stableKey;
  std::string name;
  CreativeWorldLayoutRect footprint;
  CreativeStructuralMaterial material = CreativeStructuralMaterial::Blockout;
};

enum class CreativeWorldLayoutRoomEdge : std::uint8_t {
  North,
  East,
  South,
  West,
  Count,

  // Serialized aliases retained for source compatibility. Cardinal names are
  // the authored identity; their numeric values remain schema-stable.
  MinimumZ = North,
  MaximumX = East,
  MaximumZ = South,
  MinimumX = West,
};

enum class CreativeWorldLayoutOpeningHostKind : std::uint8_t {
  Wall,
  RoomEdge,
  Count,
};

struct CreativeWorldLayoutWall {
  std::size_t buildingIndex = kInvalidCreativeWorldLayoutIndex;
  std::string stableKey;
  std::string name;
  CreativeTerrainCoord2 start{};
  CreativeTerrainCoord2 end{};
  double baseLayer = 0.0;
  std::uint16_t heightCells = kDefaultCreativeWorldLayoutWallHeightCells;
  double thicknessCells = kDefaultCreativeWorldLayoutWallThicknessCells;
  CreativeWorldLayoutWallProfile profile =
      CreativeWorldLayoutWallProfile::Automatic;
  CreativeStructuralMaterial material =
      CreativeStructuralMaterial::Blockout;
  CreativeWorldLayoutWallJoinStyle joinStyle =
      CreativeWorldLayoutWallJoinStyle::Square;
};

struct CreativeWorldLayoutOpening {
  CreativeWorldLayoutOpeningHostKind hostKind =
      CreativeWorldLayoutOpeningHostKind::Wall;
  std::size_t wallIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t roomIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayoutRoomEdge roomEdge =
      CreativeWorldLayoutRoomEdge::North;
  CreativeBuildingOpeningKind kind = CreativeBuildingOpeningKind::Door;
  CreativeDoorSettings door;
  CreativeWindowSettings window;
  CreativeBuildingOpeningFacing facing =
      CreativeBuildingOpeningFacing::PositiveNormal;
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
  // Empty keeps the descriptor-backed procedural insert. Catalog-backed
  // openings retain imported meter-space bounds so compilation can fit the
  // asset to the semantic cutout without creating an unrelated prop.
  std::string insertAssetId;
  CreativeBounds insertAssetSourceBoundsMeters;
  bool hasInsertAssetSourceBounds = false;
  // Schema-12 rooms host openings directly on a shared topology edge. The
  // legacy roomIndex/roomEdge pair remains valid when this index is absent.
  std::size_t roomTopologyEdgeIndex = kInvalidCreativeWorldLayoutIndex;
};

// Roof apertures remain plan-space source records owned by one level. The
// roof recipe maps these grid-cell bounds onto the exact panel plane, so style
// or pitch changes preserve author intent without storing generated pieces.
struct CreativeWorldLayoutRoofAperture {
  std::size_t levelIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeStructuralRoofApertureKind kind =
      CreativeStructuralRoofApertureKind::Skylight;
  std::string stableKey;
  std::string name;
  double minimumXCells = 0.0;
  double maximumXCells = 0.0;
  double minimumZCells = 0.0;
  double maximumZCells = 0.0;
};

// Reusable props and gameplay anchors remain semantic layout symbols instead
// of being applied as an unrelated post-layout batch. Coordinates are in grid
// cells; the compiler converts them through the document's grid settings.
struct CreativeWorldLayoutObject {
  CreativeObjectKind kind = CreativeObjectKind::Unknown;
  CreativeObjectLibraryPlacementMode mode =
      CreativeObjectLibraryPlacementMode::Bounds;
  std::string stableKey;
  std::string name;
  std::string assetId;
  CreativeBounds boundsCells;
  CreativeVec3 pointCells;
  // Point-anchored catalog assets retain the imported bounds relative to the
  // asset pivot. The compiler combines these meter-space bounds with the
  // grid-derived point so moving an object never desynchronizes its footprint.
  CreativeBounds assetSourceBoundsMeters;
  bool hasAssetSourceBounds = false;
  double yawRadians = 0.0;
  CreativeVec3 scale{1.0, 1.0, 1.0};
  bool visible = true;
  std::vector<std::string> tags;
  // False preserves the legacy one-box ObjectLibrary bridge. True makes this
  // row the durable source for one generated bridge attached to a stable
  // watercourse crossing; bounds remain its 2D selection footprint.
  bool usesBridgeRecipe = false;
  CreativeBridgeSourceRecipe bridge;
  CreativePlayerSpawnSettings playerSpawn{};
  CreativeNpcSpawnSettings npcSpawn{};
  CreativeLootPointSettings lootPoint{};
  CreativeExitPointSettings exitPoint{};
};

struct CreativeWorldLayoutTerrainProfile {
  std::string stableKey;
  CreativeTerrainRecipeKind kind = CreativeTerrainRecipeKind::Hill;
  // Schema-21 and older Plateau records retain their original radial control
  // recipe. New Plateau/Terrace/Cliff sources opt into the exact bounded
  // landform operation explicitly, so loading an old map never changes shape.
  bool usesLandformRecipe = false;
  CreativeTerrainLandformRecipe landform;
  // Optional generated structure over this profile's exact final hard seams.
  // The landform remains the sole terrain owner; this source only decorates
  // the composed result and retains stable member identity through recipes.
  bool usesRetainingEdgeRecipe = false;
  CreativeRetainingEdgeSourceRecipe retainingEdge;
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
  CreativeTerrainPathSourceRecipe recipe;
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
  std::vector<CreativeWorldLayoutLevel> levels;
  std::vector<CreativeWorldLayoutRoom> rooms;
  std::vector<CreativeWorldLayoutVerticalConnector> verticalConnectors;
  std::vector<CreativeWorldLayoutBox> boxes;
  std::vector<CreativeWorldLayoutWall> walls;
  std::vector<CreativeWorldLayoutOpening> openings;
  std::vector<CreativeWorldLayoutRoofAperture> roofApertures;
  std::vector<CreativeWorldLayoutObject> objects;
  std::vector<CreativeWorldLayoutTerrainProfile> terrainProfiles;
  std::vector<CreativeWorldLayoutTerrainPath> terrainPaths;
  std::vector<CreativeWorldLayoutTopologyVertex> topologyVertices;
  std::vector<CreativeWorldLayoutTopologyEdge> topologyEdges;
  std::vector<CreativeWorldLayoutRoomBoundary> roomBoundaries;
};

enum class CreativeWorldLayoutTable : std::uint8_t {
  None,
  Building,
  Level,
  Room,
  VerticalConnector,
  Box,
  Wall,
  Opening,
  Object,
  TerrainProfile,
  TerrainPath,
  // Retained as a serialized provenance value for pre-schema-21 layouts.
  TerrainPathPoint,
  TopologyEdge,
  // Appended to preserve every existing serialized provenance table value.
  RoofAperture,
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
  RefinementConflict,
};

struct CreativeWorldLayoutCompileOptions {
  std::span<const CreativeWorldLayoutConflictDecision> conflictDecisions;
};

struct CreativeWorldLayoutRecipePatch {
  CreativeRecipePlan recipe;
  // Exact final ids aligned with recipe.objects. Existing members retain their
  // ids; Create members receive deterministic ids during compilation.
  std::vector<CreativeObjectId> objectIds;
  std::vector<CreativeWorldLayoutRecipeMemberAction> memberActions;
};

struct CreativeWorldLayoutPlan {
  std::uint32_t schemaVersion = kCreativeWorldLayoutSchemaVersion;
  std::string layoutKey;
  std::uint64_t sourceLayoutFingerprint = 0U;
  CreativeDocumentId sourceDocumentId = kInvalidDocumentId;
  std::uint64_t sourceDocumentRevision = 0U;
  std::uint64_t sourceTerrainRevision = 0U;
  std::uint64_t sourceTerrainHeightRevision = 0U;
  std::uint64_t sourceMaterialRevision = 0U;
  std::vector<CreativeObjectId> objectDetachIds;
  std::vector<CreativeObjectId> objectRemoveIds;
  std::vector<CreativeWorldLayoutRecipePatch> objectRecipePatches;
  std::vector<CreativeRecipePlan> objectRecipes;
  // Transient semantic output for crossing consumers and a future water owner.
  // These plans never become anonymous document objects or persisted water.
  std::vector<CreativeWatercoursePlan> watercoursePlans;
  // Bridge plans retain the exact crossing, approach-grade, and generated
  // structure decision used by this compile. Only their recipe objects and
  // terrain operations are materialized.
  std::vector<CreativeBridgeRecipeResult> bridgePlans;
  // Exact retaining-edge decisions compiled from final staged terrain.
  std::vector<CreativeRetainingEdgeRecipeResult> retainingEdgePlans;
  std::vector<CreativeTerrainOperationMutationRequest>
      terrainOperationMutations;
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
  std::uint64_t groundedBuildingCount = 0U;
  std::uint64_t foundationObjectCount = 0U;
  std::uint64_t watercourseCount = 0U;
  std::uint64_t watercourseCrossingCount = 0U;
  std::uint64_t bridgeRecipeCount = 0U;
  std::uint64_t bridgeGeneratedObjectCount = 0U;
  std::uint64_t bridgeApproachGradeCount = 0U;
  std::uint64_t retainingEdgeRecipeCount = 0U;
  std::uint64_t retainingEdgeGeneratedObjectCount = 0U;
  // Recipes and objects scheduled by this compile, not total source output.
  std::uint64_t objectRecipeCount = 0U;
  std::uint64_t objectRecipeCreateCount = 0U;
  // Safe member-level reconciliation that retains matching object ids.
  std::uint64_t objectRecipePatchCount = 0U;
  // Explicit destructive regeneration selected during conflict review.
  std::uint64_t objectRecipeReplaceCount = 0U;
  // Desired groups retained byte-for-byte in the destination document.
  std::uint64_t objectRecipeKeepCount = 0U;
  // Desired groups whose source is unchanged but live 3D output was refined.
  std::uint64_t objectRecipeRefinedCount = 0U;
  std::uint64_t objectRecipeConflictCount = 0U;
  std::uint64_t objectRecipeDetachCount = 0U;
  std::uint64_t objectCount = 0U;
  std::uint64_t objectDetachCount = 0U;
  std::uint64_t objectRemoveCount = 0U;
  std::uint64_t terrainControlEditCount = 0U;
  std::uint64_t terrainMaterialEditCount = 0U;
  std::uint64_t terrainOperationMutationCount = 0U;
  std::string reasonCode = "creative_world_layout_not_requested";
  std::string kernelReasonCode = "creative_world_layout_kernel_not_requested";
};

struct CreativeWorldLayoutCompileResult {
  CreativeWorldLayoutPlan plan;
  CreativeWorldLayoutReceipt receipt;
  std::vector<CreativeWorldLayoutRecipeChange> recipeChanges;
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
[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutRoomEdge edge) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutRoomType type) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutWallProfile profile) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutWallJoinStyle joinStyle) noexcept;
[[nodiscard]] std::string_view creativeWorldLayoutRoomEdgeKey(
    CreativeWorldLayoutRoomEdge edge) noexcept;
[[nodiscard]] std::string creativeWorldLayoutTag(std::string_view layoutKey);
[[nodiscard]] std::string creativeWorldLayoutTerrainPathSourceKey(
    std::string_view layoutKey,
    std::string_view pathKey);
[[nodiscard]] std::string creativeWorldLayoutTerrainLandformSourceKey(
    std::string_view layoutKey,
    std::string_view profileKey);
[[nodiscard]] bool validCreativeWorldLayoutStableKey(
    std::string_view key) noexcept;
[[nodiscard]] bool creativeWorldLayoutStableKeyExists(
    const CreativeWorldLayout& layout,
    std::string_view key) noexcept;
[[nodiscard]] std::string mintCreativeWorldLayoutStableKey(
    const CreativeWorldLayout& layout,
    std::uint64_t& nextOrdinal,
    std::string_view prefix);

[[nodiscard]] CreativeWorldLayoutCompileResult buildCreativeWorldLayoutPlan(
    const CreativeDocument& document,
    const CreativeWorldLayout& layout,
    CreativeWorldLayoutCompileOptions options = {});
[[nodiscard]] std::uint64_t fingerprintCreativeWorldLayoutPlanSource(
    const CreativeWorldLayoutPlan& plan) noexcept;
[[nodiscard]] std::uint64_t creativeWorldLayoutPlanAffectedMemberCount(
    const CreativeWorldLayoutPlan& plan) noexcept;
[[nodiscard]] std::optional<CreativeAuthoringOperationRecord>
makeCreativeWorldLayoutOperationRecord(const CreativeWorldLayoutPlan& plan);
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
