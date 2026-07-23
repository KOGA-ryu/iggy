#pragma once

#include "app/iggy3d/creative/world/WorldLayout.hpp"
#include "runtime/movement/MovementDefaults.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace iggy3d::creative {

inline constexpr double kCreativeWorldLayoutOpeningEndClearanceCells = 0.25;
inline constexpr double kCreativeWorldLayoutOpeningMinimumWidthCells = 0.25;

enum class CreativeWorldLayoutOpeningHostStatus : std::uint8_t {
  NotRequested,
  InvalidOpening,
  MissingHost,
  InvalidHostGeometry,
  UnsupportedOrientation,
  Ready,
};

struct CreativeWorldLayoutOpeningHostFrame {
  bool requested = false;
  bool accepted = false;
  bool exterior = false;
  CreativeWorldLayoutOpeningHostStatus status =
      CreativeWorldLayoutOpeningHostStatus::NotRequested;
  CreativeWorldLayoutOpeningHostKind hostKind =
      CreativeWorldLayoutOpeningHostKind::Wall;
  std::size_t wallIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t roomIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t topologyEdgeIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t buildingIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t levelIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayoutRoomEdge roomEdge = CreativeWorldLayoutRoomEdge::Count;
  CreativeTerrainCoord2 start{};
  CreativeTerrainCoord2 end{};
  double lengthCells = 0.0;
  double baseLayer = 0.0;
  double wallHeightCells = 0.0;
  double wallThicknessCells = 0.0;
  std::size_t owningRoomCount = 0U;
  std::string_view reasonCode =
      "creative_world_layout_opening_host_not_requested";
};

struct CreativeWorldLayoutOpeningHostPoint {
  double x = 0.0;
  double z = 0.0;
};

struct CreativeWorldLayoutOpeningHostHit {
  bool requested = false;
  bool hit = false;
  CreativeWorldLayoutOpeningHostKind hostKind =
      CreativeWorldLayoutOpeningHostKind::Wall;
  std::size_t wallIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t roomIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t topologyEdgeIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayoutRoomEdge roomEdge = CreativeWorldLayoutRoomEdge::Count;
  double centerOffsetCells = 0.0;
  double distanceCells = 0.0;
  CreativeWorldLayoutOpeningHostFrame host;
};

enum class CreativeWorldLayoutOpeningValidationStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  InvalidKind,
  InvalidFacing,
  InvalidDoorSettings,
  InvalidWindowSettings,
  InvalidCutout,
  InvalidInsert,
  InsertDoesNotFit,
  DoorSillInvalid,
  MissingHost,
  UnsupportedHostOrientation,
  EndClearanceInvalid,
  WallHeightExceeded,
  InteriorWindow,
  Overlap,
  DoorSwingObstructed,
  Ready,
};

enum class CreativeWorldLayoutDoorSwingClearanceStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  NotDoor,
  InvalidGeometry,
  WallObstructed,
  DoorObstructed,
  CriticalPathObstructed,
  Ready,
};

struct CreativeWorldLayoutDoorSwingClearanceRequest {
  const CreativeWorldLayout* layout = nullptr;
  const CreativeWorldLayoutOpening* opening = nullptr;
  std::size_t ignoredOpeningIndex = kInvalidCreativeWorldLayoutIndex;
};

struct CreativeWorldLayoutDoorSwingClearanceResult {
  bool requested = false;
  bool accepted = false;
  bool clear = false;
  CreativeWorldLayoutDoorSwingClearanceStatus status =
      CreativeWorldLayoutDoorSwingClearanceStatus::NotRequested;
  CreativeBounds sweepBoundsCells;
  std::size_t conflictingWallIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t conflictingTopologyEdgeIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t conflictingOpeningIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t conflictingVerticalConnectorIndex =
      kInvalidCreativeWorldLayoutIndex;
  std::string_view reasonCode =
      "creative_world_layout_door_swing_not_requested";
};

struct CreativeWorldLayoutOpeningValidationRequest {
  const CreativeWorldLayout* layout = nullptr;
  const CreativeWorldLayoutOpening* opening = nullptr;
  std::size_t ignoredOpeningIndex = kInvalidCreativeWorldLayoutIndex;
  double minimumEndClearanceCells =
      kCreativeWorldLayoutOpeningEndClearanceCells;
  double minimumWidthCells = kCreativeWorldLayoutOpeningMinimumWidthCells;
};

struct CreativeWorldLayoutOpeningValidationResult {
  bool requested = false;
  bool accepted = false;
  CreativeWorldLayoutOpeningValidationStatus status =
      CreativeWorldLayoutOpeningValidationStatus::NotRequested;
  std::size_t conflictingOpeningIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t conflictingWallIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t conflictingTopologyEdgeIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t conflictingVerticalConnectorIndex =
      kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayoutOpeningHostFrame host;
  CreativeBounds doorSweepBoundsCells;
  double sillTopCells = 0.0;
  double lintelBottomCells = 0.0;
  double lintelHeightCells = 0.0;
  std::string_view reasonCode =
      "creative_world_layout_opening_validation_not_requested";
};

enum class CreativeWorldLayoutOpeningClearanceStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  NotPassage,
  WidthObstructed,
  HeightObstructed,
  ThresholdObstructed,
  Ready,
};

struct CreativeWorldLayoutOpeningClearanceRequest {
  const CreativeWorldLayoutOpening* opening = nullptr;
  double gridCellSizeMeters = 1.0;
  double actorRadiusMeters = iggy3d::kDefaultPlayerBodyRadiusMeters;
  double actorHeightMeters = iggy3d::kDefaultPlayerStandingHeightMeters;
  double maximumStepMeters = iggy3d::kDefaultPlayerStepHeightMeters;
  double skinMeters = iggy3d::kDefaultPlayerSkinMeters;
};

struct CreativeWorldLayoutOpeningClearanceResult {
  bool requested = false;
  bool accepted = false;
  bool traversable = false;
  CreativeWorldLayoutOpeningClearanceStatus status =
      CreativeWorldLayoutOpeningClearanceStatus::NotRequested;
  double clearWidthMeters = 0.0;
  double clearHeightMeters = 0.0;
  double thresholdMeters = 0.0;
  double minimumWidthMeters = 0.0;
  double minimumHeightMeters = 0.0;
  std::string_view reasonCode =
      "creative_world_layout_opening_clearance_not_requested";
};

[[nodiscard]] CreativeWorldLayoutOpeningHostFrame
resolveCreativeWorldLayoutOpeningHost(
    const CreativeWorldLayout& layout,
    const CreativeWorldLayoutOpening& opening) noexcept;

[[nodiscard]] double creativeWorldLayoutOpeningHostOffset(
    const CreativeWorldLayoutOpeningHostFrame& host,
    CreativeWorldLayoutOpeningHostPoint point) noexcept;

[[nodiscard]] CreativeWorldLayoutOpeningHostPoint
creativeWorldLayoutOpeningHostPoint(
    const CreativeWorldLayoutOpeningHostFrame& host,
    double offsetCells) noexcept;

// Control-path nearest-host query. Explicit walls win exact distance ties;
// canonical topology edges follow stable table order. No allocations occur.
[[nodiscard]] CreativeWorldLayoutOpeningHostHit
findNearestCreativeWorldLayoutOpeningHost(
    const CreativeWorldLayout& layout,
    CreativeWorldLayoutOpeningHostPoint point,
    double toleranceCells,
    std::size_t activeLevelIndex = kInvalidCreativeWorldLayoutIndex) noexcept;

// O(O + B) for O openings and B room-boundary rows. The ignored row supports
// editing an existing opening without treating it as its own conflict.
[[nodiscard]] CreativeWorldLayoutOpeningValidationResult
validateCreativeWorldLayoutOpening(
    const CreativeWorldLayoutOpeningValidationRequest& request) noexcept;

// Validates every authored opening against the complete layout, including
// mutual overlap and door-swing clearance. Runs without allocating.
[[nodiscard]] bool validCreativeWorldLayoutOpenings(
    const CreativeWorldLayout& layout) noexcept;

[[nodiscard]] CreativeWorldLayoutOpeningClearanceResult
evaluateCreativeWorldLayoutOpeningClearance(
    const CreativeWorldLayoutOpeningClearanceRequest& request) noexcept;

// Plans the exact procedural leaf assembly in grid-cell coordinates, then tests
// its full swept bounds against authored walls, other door sweeps, and vertical
// connector footprints. Invalid inputs fail closed; no document is mutated.
[[nodiscard]] CreativeWorldLayoutDoorSwingClearanceResult
evaluateCreativeWorldLayoutDoorSwingClearance(
    const CreativeWorldLayoutDoorSwingClearanceRequest& request) noexcept;

}  // namespace iggy3d::creative
