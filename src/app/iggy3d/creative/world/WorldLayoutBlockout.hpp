#pragma once

#include "app/iggy3d/creative/world/WorldLayoutArchitecture.hpp"
#include "app/iggy3d/creative/world/WorldLayout.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace iggy3d::creative {

enum class CreativeWorldLayoutBuildingBlockoutPattern : std::uint8_t {
  SingleRoom,
  SplitX,
  SplitZ,
  Grid2x2,
  Count,
};

enum class CreativeWorldLayoutBuildingBlockoutStatus : std::uint8_t {
  NotRequested,
  InvalidPattern,
  InvalidFootprint,
  InvalidWallThickness,
  InvalidFloorToFloor,
  InvalidStoreyCount,
  InvalidEntranceEdge,
  InvalidEntranceOffset,
  InvalidVerticalConnector,
  RoomTooSmall,
  VerticalConnectorDoesNotFit,
  OpeningCapacityExceeded,
  Ready,
};

enum class CreativeWorldLayoutBuildingBlockoutOpeningRole : std::uint8_t {
  InteriorConnection,
  Entrance,
  ExteriorWindow,
  Count,
};

struct CreativeWorldLayoutBuildingBlockoutFacadeSettings {
  bool includeEntrance = true;
  CreativeWorldLayoutRoomEdge entranceEdge =
      CreativeWorldLayoutRoomEdge::South;

  // Measured along the selected central exterior room-edge segment from its
  // midpoint. Exact wall fitting remains owned by the opening-placement kernel.
  double entranceOffsetCells = 0.0;

  // Emits one centered window on every eligible exterior room edge except the
  // edge already occupied by the generated entrance.
  bool includeExteriorWindows = true;
};

inline constexpr std::uint16_t
    kCreativeWorldLayoutBuildingBlockoutStoreyCapacity = 8U;

struct CreativeWorldLayoutBuildingBlockoutStoreySettings {
  std::uint16_t count = 1U;
  bool connectStoreys = true;
  CreativeWorldLayoutVerticalConnectorKind connectorKind =
      CreativeWorldLayoutVerticalConnectorKind::Stair;

  // The planner tries this direction first, then the remaining cardinal
  // directions in enum order. The resolved direction is recorded in the plan.
  CreativeWorldLayoutVerticalDirection preferredDirection =
      CreativeWorldLayoutVerticalDirection::PositiveZ;
};

struct CreativeWorldLayoutBuildingBlockoutRequest {
  CreativeWorldLayoutRect footprint;
  CreativeWorldLayoutBuildingBlockoutPattern pattern =
      CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom;
  double wallThicknessCells =
      kDefaultCreativeWorldLayoutWallThicknessCells;
  bool connectRooms = true;
  CreativeWorldLayoutBuildingBlockoutFacadeSettings facade;
  // Distance between consecutive finished-floor datums. Generated walls use
  // the next datum as their initial top constraint; later per-level edits can
  // still override CreativeWorldLayoutLevel::wallHeightCells explicitly.
  std::uint16_t floorToFloorCells =
      kDefaultCreativeWorldLayoutWallHeightCells;
  CreativeWorldLayoutBuildingBlockoutStoreySettings storeys;
};

inline constexpr std::uint32_t
    kCreativeWorldLayoutBuildingBlockoutRecipeVersion = 4U;

// The complete semantic input required to regenerate a blockout. This remains
// independent of editor UI state so both 2D and 3D frontends can load and
// update the same authored building recipe.
struct CreativeWorldLayoutBuildingBlockoutRecipe {
  std::uint32_t version =
      kCreativeWorldLayoutBuildingBlockoutRecipeVersion;
  CreativeWorldLayoutBuildingBlockoutRequest request;
  double floorTopLayer = 0.0;
  std::uint16_t floorThicknessLayers = 1U;
  std::uint16_t ceilingThicknessLayers = 1U;
  std::uint16_t roofThicknessLayers = 1U;
  CreativeWorldLayoutArchitecturalProfileKind architecturalProfileKind =
      CreativeWorldLayoutArchitecturalProfileKind::Custom;
  CreativeStructuralMaterial exteriorWallMaterial =
      CreativeStructuralMaterial::Blockout;
  CreativeStructuralMaterial interiorWallMaterial =
      CreativeStructuralMaterial::Blockout;
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

struct CreativeWorldLayoutBuildingBlockoutFingerprint {
  bool valid = false;
  std::uint64_t value = 0U;
};

struct CreativeWorldLayoutBuildingBlockoutProvenance {
  bool present = false;
  bool valid = false;
  CreativeWorldLayoutBuildingBlockoutRecipe recipe;
  std::uint64_t instanceBaselineFingerprint = 0U;
};

enum class CreativeWorldLayoutBuildingBlockoutSyncState : std::uint8_t {
  Unlinked,
  Current,
  LocallyModified,
  Invalid,
};

struct CreativeWorldLayoutBuildingBlockoutSyncReceipt {
  bool requested = false;
  bool accepted = false;
  std::size_t buildingIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayoutBuildingBlockoutSyncState state =
      CreativeWorldLayoutBuildingBlockoutSyncState::Unlinked;
  CreativeWorldLayoutBuildingBlockoutProvenance provenance;
  CreativeWorldLayoutBuildingBlockoutFingerprint instanceFingerprint;
  std::string_view reasonCode =
      "creative_world_layout_building_blockout_sync_not_requested";
};

inline constexpr std::size_t kCreativeWorldLayoutBuildingBlockoutRoomCapacity =
    4U;
inline constexpr std::size_t
    kCreativeWorldLayoutBuildingBlockoutOpeningCapacity = 16U;

// One semantic opening intent. Interior intents name both adjacent room edges;
// facade intents leave the adjacent-room fields invalid. The world-space point
// is consumed by the existing opening-placement kernel, which remains the sole
// owner of dimensions, wall clearance, snapping, and host validation.
struct CreativeWorldLayoutBuildingBlockoutOpening {
  CreativeWorldLayoutBuildingBlockoutOpeningRole role =
      CreativeWorldLayoutBuildingBlockoutOpeningRole::InteriorConnection;
  CreativeBuildingOpeningKind kind = CreativeBuildingOpeningKind::Door;
  std::size_t roomIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayoutRoomEdge roomEdge =
      CreativeWorldLayoutRoomEdge::North;
  std::size_t adjacentRoomIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayoutRoomEdge adjacentRoomEdge =
      CreativeWorldLayoutRoomEdge::North;
  double xCells = 0.0;
  double zCells = 0.0;
};

// One shaft intent repeated between each adjacent pair of generated levels.
// The footprint is centered inside one repeated room with at least one cell of
// landing and side clearance, widened when walls require it. It includes the
// run, not the landings. Exact slope, cut, and generated geometry remains owned
// by WorldLayoutVerticalConnectors.
struct CreativeWorldLayoutBuildingBlockoutVerticalConnector {
  std::size_t roomIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayoutVerticalConnectorKind kind =
      CreativeWorldLayoutVerticalConnectorKind::Stair;
  CreativeWorldLayoutVerticalDirection direction =
      CreativeWorldLayoutVerticalDirection::PositiveZ;
  CreativeWorldLayoutRect footprint;
};

struct CreativeWorldLayoutBuildingBlockoutPlan {
  bool requested = false;
  bool accepted = false;
  CreativeWorldLayoutBuildingBlockoutStatus status =
      CreativeWorldLayoutBuildingBlockoutStatus::NotRequested;
  CreativeWorldLayoutBuildingBlockoutPattern pattern =
      CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom;
  CreativeWorldLayoutRect footprint;
  std::array<CreativeWorldLayoutRect,
             kCreativeWorldLayoutBuildingBlockoutRoomCapacity>
      rooms{};
  std::size_t roomCount = 0U;
  std::array<CreativeWorldLayoutBuildingBlockoutOpening,
             kCreativeWorldLayoutBuildingBlockoutOpeningCapacity>
      openings{};
  std::size_t openingCount = 0U;
  std::uint16_t storeyCount = 0U;
  bool hasVerticalConnector = false;
  CreativeWorldLayoutBuildingBlockoutVerticalConnector verticalConnector;
  std::string_view reasonCode =
      "creative_world_layout_building_blockout_not_requested";
};

static_assert(std::is_trivially_copyable_v<
              CreativeWorldLayoutBuildingBlockoutFacadeSettings>);
static_assert(std::is_trivially_copyable_v<
              CreativeWorldLayoutBuildingBlockoutStoreySettings>);
static_assert(
    std::is_trivially_copyable_v<CreativeWorldLayoutBuildingBlockoutRequest>);
static_assert(
    std::is_trivially_copyable_v<CreativeWorldLayoutBuildingBlockoutRecipe>);
static_assert(std::is_trivially_copyable_v<
              CreativeWorldLayoutBuildingBlockoutProvenance>);
static_assert(std::is_trivially_copyable_v<
              CreativeWorldLayoutBuildingBlockoutOpening>);
static_assert(std::is_trivially_copyable_v<
              CreativeWorldLayoutBuildingBlockoutVerticalConnector>);
static_assert(
    std::is_trivially_copyable_v<CreativeWorldLayoutBuildingBlockoutPlan>);

[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutBuildingBlockoutPattern pattern) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutBuildingBlockoutStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutBuildingBlockoutSyncState state) noexcept;

[[nodiscard]] bool validCreativeWorldLayoutBuildingBlockoutRecipe(
    const CreativeWorldLayoutBuildingBlockoutRecipe& recipe) noexcept;
[[nodiscard]] bool isCreativeWorldLayoutBuildingBlockoutProvenanceTag(
    std::string_view tag) noexcept;
[[nodiscard]] CreativeWorldLayoutBuildingBlockoutFingerprint
fingerprintCreativeWorldLayoutBuildingBlockout(
    const CreativeWorldLayout& layout, std::size_t buildingIndex);
[[nodiscard]] CreativeWorldLayoutBuildingBlockoutProvenance
creativeWorldLayoutBuildingBlockoutProvenance(
    const CreativeWorldLayout& layout, std::size_t buildingIndex);
[[nodiscard]] bool creativeWorldLayoutBuildingBlockoutWallMaterial(
    const CreativeWorldLayout& layout, std::size_t buildingIndex,
    CreativeWorldLayoutWallProfile profile,
    CreativeStructuralMaterial& output);
[[nodiscard]] bool setCreativeWorldLayoutBuildingBlockoutProvenance(
    CreativeWorldLayout& layout, std::size_t buildingIndex,
    const CreativeWorldLayoutBuildingBlockoutProvenance& provenance);
[[nodiscard]] CreativeWorldLayoutBuildingBlockoutSyncReceipt
inspectCreativeWorldLayoutBuildingBlockoutSync(
    const CreativeWorldLayout& layout, std::size_t buildingIndex);

// Produces row-major room footprints and bounded opening intents on integer grid
// lines in this order: minimal interior tree, entrance, then room-major facade
// windows in cardinal-edge order. For connected multi-storey requests, one
// reusable shaft is selected by preferred direction, then largest/most-central
// eligible room. Odd spans give the extra cell to the positive-X or positive-Z
// room. This is O(room capacity * (edge count + direction count)), performs no
// allocation, and never mutates source layout.
[[nodiscard]] CreativeWorldLayoutBuildingBlockoutPlan
planCreativeWorldLayoutBuildingBlockout(
    const CreativeWorldLayoutBuildingBlockoutRequest& request) noexcept;

}  // namespace iggy3d::creative
