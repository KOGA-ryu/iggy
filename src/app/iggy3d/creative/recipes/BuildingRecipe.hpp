#pragma once

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/recipes/CreativeRecipe.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace iggy3d::creative {

enum class CreativeBuildingRootMode : std::uint8_t {
  None,
  CreateRoom,
  ExistingRoom,
};

enum class CreativeBuildingOpeningKind : std::uint8_t {
  Door,
  Window,
};

enum class CreativeBuildingOpeningPose : std::uint8_t {
  Closed,
  OpenFromStartNegativeNormal,
  OpenFromStartPositiveNormal,
  OpenFromEndNegativeNormal,
  OpenFromEndPositiveNormal,
};

enum class CreativeBuildingRecipeStatus : std::uint8_t {
  NotRequested,
  InvalidRoot,
  Empty,
  InvalidBox,
  InvalidWall,
  UnsupportedWallOrientation,
  InvalidOpening,
  OverlappingOpenings,
  InvalidPlan,
  Ready,
};

enum class CreativeRectangularRoomGeometryStatus : std::uint8_t {
  NotRequested,
  InvalidCorner,
  UnevenFloorPlane,
  InvalidDimension,
  DegenerateFootprint,
  WallConsumesFootprint,
  Ready,
};

// Corners describe the outer floor boundary on its finished top plane. The
// generated floor extends downward; generated walls begin on that exact plane.
struct CreativeRectangularRoomGeometryRequest {
  CreativeVec3 firstFloorCorner;
  CreativeVec3 oppositeFloorCorner;
  double wallHeightMeters = defaultCreativeWallGeometry().heightMeters;
  double wallThicknessMeters = defaultCreativeWallGeometry().thicknessMeters;
  double floorThicknessMeters = 0.25;
};

struct CreativeRectangularRoomGeometryPlan {
  CreativeRectangularRoomGeometryStatus status =
      CreativeRectangularRoomGeometryStatus::NotRequested;
  CreativeBounds rootBounds;
  CreativeBounds floorBounds;
  std::array<CreativeVec3, 4U> wallStarts{};
  std::array<CreativeVec3, 4U> wallEnds{};
  std::array<CreativeBounds, 4U> wallBounds{};
  bool accepted = false;
  std::string_view reasonCode =
      "creative_rectangular_room_geometry_not_requested";
};

static_assert(
    std::is_trivially_copyable_v<CreativeRectangularRoomGeometryRequest>);
static_assert(
    std::is_trivially_copyable_v<CreativeRectangularRoomGeometryPlan>);

struct CreativeRectangularRoomRecipeRequest {
  std::string stableKey = "room";
  std::string name = "Room";
  CreativeRectangularRoomGeometryRequest geometry;
  bool visible = true;
  std::vector<std::string> tags;
};

// Ordered axis-aligned building boxes. Their order is preserved in the output,
// which keeps deterministic ids for callers such as map templates.
struct CreativeBuildingBoxSpec {
  CreativeObjectKind kind = CreativeObjectKind::Unknown;
  std::string stableKey;
  std::string name;
  CreativeBounds bounds;
  CreativeVec3 scale{1.0, 1.0, 1.0};
};

struct CreativeBuildingOpeningSpec {
  CreativeBuildingOpeningKind kind = CreativeBuildingOpeningKind::Door;
  CreativeBuildingOpeningPose pose = CreativeBuildingOpeningPose::Closed;
  std::string stableKey;
  std::string name;

  // Horizontal values are measured along the wall from start to end.
  double centerOffsetMeters = 0.0;
  double widthMeters = 1.0;

  // The cutout is removed from wall geometry. Door cutouts normally begin at
  // zero; windows normally have a positive bottom.
  double cutoutBottomMeters = 0.0;
  double cutoutHeightMeters = 2.1;

  // Inserts are optional. Zero dimensions inherit their corresponding cutout
  // dimensions; zero thickness inherits wall thickness.
  bool includeInsert = true;
  double insertBottomMeters = 0.0;
  double insertHeightMeters = 0.0;
  double insertWidthMeters = 0.0;
  double insertThicknessMeters = 0.0;
};

struct CreativeBuildingWallSpec {
  std::string stableKey;
  std::string name;
  CreativeVec3 start;
  CreativeVec3 end;
  double heightMeters = defaultCreativeWallGeometry().heightMeters;
  double thicknessMeters = defaultCreativeWallGeometry().thicknessMeters;
  std::vector<CreativeBuildingOpeningSpec> openings;

  // Optional product-facing names for each full-height span, in start-to-end
  // order. When present there must be openings.size() + 1 names.
  std::vector<std::string> segmentNames;
};

struct CreativeBuildingRecipeRequest {
  std::string stableKey = "building";
  std::string name = "Building";
  CreativeBuildingRootMode rootMode = CreativeBuildingRootMode::CreateRoom;
  CreativeBounds rootBounds;
  CreativeObjectId existingRoomObjectId = kInvalidObjectId;
  bool visible = true;
  std::vector<std::string> tags;
  std::vector<CreativeBuildingBoxSpec> boxes;
  std::vector<CreativeBuildingWallSpec> walls;
};

struct CreativeBuildingRecipeReceipt {
  bool requested = false;
  bool accepted = false;
  CreativeBuildingRecipeStatus status =
      CreativeBuildingRecipeStatus::NotRequested;
  std::uint64_t rootObjectCount = 0U;
  std::uint64_t boxObjectCount = 0U;
  std::uint64_t wallObjectCount = 0U;
  std::uint64_t doorObjectCount = 0U;
  std::uint64_t windowObjectCount = 0U;
  std::uint64_t generatedObjectCount = 0U;
  std::size_t failedBoxIndex = 0U;
  std::size_t failedWallIndex = 0U;
  std::size_t failedOpeningIndex = 0U;
  std::string reasonCode = "creative_building_recipe_not_requested";
};

struct CreativeBuildingRecipeResult {
  CreativeRecipePlan plan;
  CreativeBuildingRecipeReceipt receipt;
};

[[nodiscard]] std::string_view toString(
    CreativeBuildingRootMode mode) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeBuildingOpeningKind kind) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeBuildingOpeningPose pose) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeBuildingRecipeStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeRectangularRoomGeometryStatus status) noexcept;

[[nodiscard]] CreativeRectangularRoomGeometryPlan
planCreativeRectangularRoomGeometry(
    const CreativeRectangularRoomGeometryRequest& request) noexcept;

[[nodiscard]] CreativeBuildingOpeningSpec makeCreativeBuildingDoorOpening(
    std::string stableKey,
    std::string name,
    double centerOffsetMeters,
    double widthMeters = 1.0,
    double heightMeters = 2.1);
[[nodiscard]] CreativeBuildingOpeningSpec makeCreativeBuildingWindowOpening(
    std::string stableKey,
    std::string name,
    double centerOffsetMeters,
    double widthMeters = 1.5,
    double sillHeightMeters = 0.9,
    double heightMeters = 1.1);

[[nodiscard]] CreativeBuildingRecipeResult buildCreativeBuildingRecipe(
    const CreativeBuildingRecipeRequest& request);
[[nodiscard]] CreativeBuildingRecipeResult buildCreativeRectangularRoomRecipe(
    const CreativeRectangularRoomRecipeRequest& request);

}  // namespace iggy3d::creative
