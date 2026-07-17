#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include "app/iggy3d/creative/world/WorldLayout.hpp"

namespace iggy3d_creative_app {

namespace cr = iggy3d::creative;

enum class CreativeEditorWorldLayoutViewMode : std::uint8_t {
  Plan,
  Elevation,
  Count,
};

// The world coordinate shown on the horizontal elevation axis. Vertical is
// always document Y, expressed in grid cells.
enum class CreativeEditorWorldLayoutElevationAxis : std::uint8_t {
  X,
  Z,
  Count,
};

enum class CreativeEditorWorldLayoutElevationStatus : std::uint8_t {
  NotRequested,
  InvalidGrid,
  InvalidBuilding,
  InvalidLayout,
  EmptyBuilding,
  RecipeRejected,
  Ready,
};

enum class CreativeEditorWorldLayoutElevationSourceKind : std::uint8_t {
  None,
  Room,
  Box,
  Wall,
  Opening,
  VerticalConnector,
  Count,
};

enum class CreativeEditorWorldLayoutElevationItemKind : std::uint8_t {
  FloorSlab,
  WallEnvelope,
  CeilingSlab,
  RoofBase,
  Door,
  Window,
  Stair,
  Ramp,
  Volume,
  Count,
};

enum class CreativeEditorWorldLayoutElevationLineKind : std::uint8_t {
  RoofSlope,
  RoofRidge,
  ConnectorRise,
  Count,
};

enum class CreativeEditorWorldLayoutElevationHandleKind : std::uint8_t {
  None,
  LevelFloor,
  WallTop,
  RoofRidge,
  OpeningBottom,
  OpeningTop,
  Count,
};

struct CreativeEditorWorldLayoutElevationPoint {
  double horizontal = 0.0;
  double vertical = 0.0;
};

struct CreativeEditorWorldLayoutElevationBounds {
  bool valid = false;
  double minimumHorizontal = 0.0;
  double maximumHorizontal = 0.0;
  double minimumVertical = 0.0;
  double maximumVertical = 0.0;
};

struct CreativeEditorWorldLayoutElevationItem {
  CreativeEditorWorldLayoutElevationItemKind kind =
      CreativeEditorWorldLayoutElevationItemKind::WallEnvelope;
  CreativeEditorWorldLayoutElevationSourceKind sourceKind =
      CreativeEditorWorldLayoutElevationSourceKind::None;
  std::size_t sourceIndex = cr::kInvalidCreativeWorldLayoutIndex;
  std::size_t levelIndex = cr::kInvalidCreativeWorldLayoutIndex;
  double minimumHorizontal = 0.0;
  double maximumHorizontal = 0.0;
  double minimumVertical = 0.0;
  double maximumVertical = 0.0;
};

struct CreativeEditorWorldLayoutElevationLine {
  CreativeEditorWorldLayoutElevationLineKind kind =
      CreativeEditorWorldLayoutElevationLineKind::RoofSlope;
  CreativeEditorWorldLayoutElevationSourceKind sourceKind =
      CreativeEditorWorldLayoutElevationSourceKind::None;
  std::size_t sourceIndex = cr::kInvalidCreativeWorldLayoutIndex;
  std::size_t levelIndex = cr::kInvalidCreativeWorldLayoutIndex;
  CreativeEditorWorldLayoutElevationPoint start;
  CreativeEditorWorldLayoutElevationPoint end;
};

struct CreativeEditorWorldLayoutElevationHandle {
  CreativeEditorWorldLayoutElevationHandleKind kind =
      CreativeEditorWorldLayoutElevationHandleKind::None;
  CreativeEditorWorldLayoutElevationSourceKind sourceKind =
      CreativeEditorWorldLayoutElevationSourceKind::None;
  std::size_t sourceIndex = cr::kInvalidCreativeWorldLayoutIndex;
  std::size_t levelIndex = cr::kInvalidCreativeWorldLayoutIndex;
  CreativeEditorWorldLayoutElevationPoint position;
};

struct CreativeEditorWorldLayoutElevationProjection {
  bool accepted = false;
  CreativeEditorWorldLayoutElevationStatus status =
      CreativeEditorWorldLayoutElevationStatus::NotRequested;
  std::size_t buildingIndex = cr::kInvalidCreativeWorldLayoutIndex;
  CreativeEditorWorldLayoutElevationAxis axis =
      CreativeEditorWorldLayoutElevationAxis::X;
  CreativeEditorWorldLayoutElevationBounds bounds;
  std::vector<CreativeEditorWorldLayoutElevationItem> items;
  std::vector<CreativeEditorWorldLayoutElevationLine> lines;
  std::vector<CreativeEditorWorldLayoutElevationHandle> handles;
  std::string_view reasonCode =
      "creative_editor_world_layout_elevation_not_requested";
};

struct CreativeEditorWorldLayoutElevationRequest {
  const cr::CreativeWorldLayout* layout = nullptr;
  cr::CreativeGridSettings grid;
  std::size_t buildingIndex = cr::kInvalidCreativeWorldLayoutIndex;
  CreativeEditorWorldLayoutElevationAxis axis =
      CreativeEditorWorldLayoutElevationAxis::X;
};

struct CreativeEditorWorldLayoutElevationEditResult {
  bool accepted = false;
  CreativeEditorWorldLayoutElevationHandle handle;
  double floorTopLayer = 0.0;
  std::uint16_t wallHeightCells = 0U;
  double roofPitchDegrees = 0.0;
  double openingSillCells = 0.0;
  double openingHeightCells = 0.0;
  std::string_view reasonCode =
      "creative_editor_world_layout_elevation_edit_not_requested";
};

[[nodiscard]] CreativeEditorWorldLayoutElevationProjection
planCreativeEditorWorldLayoutElevation(
    const CreativeEditorWorldLayoutElevationRequest& request);

[[nodiscard]] CreativeEditorWorldLayoutElevationHandle
findCreativeEditorWorldLayoutElevationHandle(
    const CreativeEditorWorldLayoutElevationProjection& projection,
    CreativeEditorWorldLayoutElevationPoint point,
    double toleranceCells) noexcept;

[[nodiscard]] const CreativeEditorWorldLayoutElevationItem*
findCreativeEditorWorldLayoutElevationItem(
    const CreativeEditorWorldLayoutElevationProjection& projection,
    CreativeEditorWorldLayoutElevationPoint point,
    double toleranceCells) noexcept;

// Resolves one snapped vertical handle edit without mutating layout source.
// The caller commits the returned values through the existing room/opening
// settings command on pointer release.
[[nodiscard]] CreativeEditorWorldLayoutElevationEditResult
planCreativeEditorWorldLayoutElevationEdit(
    const cr::CreativeWorldLayout& layout,
    const CreativeEditorWorldLayoutElevationProjection& projection,
    CreativeEditorWorldLayoutElevationHandle handle,
    double requestedVerticalCells) noexcept;

}  // namespace iggy3d_creative_app
