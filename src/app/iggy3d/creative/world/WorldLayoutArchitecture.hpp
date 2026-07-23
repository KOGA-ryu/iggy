#pragma once

#include "app/iggy3d/creative/world/WorldLayoutDimensions.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace iggy3d::creative {

enum class CreativeWorldLayoutArchitecturalProfileKind : std::uint8_t {
  Residential,
  Grand,
  Custom,
  Count,
};

struct CreativeWorldLayoutArchitecturalProfile {
  CreativeWorldLayoutArchitecturalProfileKind kind =
      CreativeWorldLayoutArchitecturalProfileKind::Residential;
  double floorToFloorMeters = 3.0;
  std::uint16_t floorThicknessLayers = 4U;
  std::uint16_t ceilingThicknessLayers = 1U;
  std::uint16_t roofThicknessLayers = 1U;
};

enum class CreativeWorldLayoutArchitectureStatus : std::uint8_t {
  NotRequested,
  InvalidGrid,
  InvalidRequest,
  InvalidOwnership,
  EmptyBuilding,
  DuplicateLevelElevation,
  UnrepresentableProfile,
  UnalignedStructure,
  OpeningDoesNotFit,
  VerticalConnectorInvalid,
  NoChange,
  Ready,
};

struct CreativeWorldLayoutArchitectureRequest {
  std::size_t buildingIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayoutArchitecturalProfile profile;
};

struct CreativeWorldLayoutArchitectureReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  bool blockoutWasCurrent = false;
  bool preservedBlockoutLink = false;
  CreativeWorldLayoutArchitectureStatus status =
      CreativeWorldLayoutArchitectureStatus::NotRequested;
  std::size_t buildingIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t updatedLevelCount = 0U;
  std::size_t updatedWallCount = 0U;
  std::size_t updatedStructuralBoxCount = 0U;
  std::size_t updatedVerticalConnectorCount = 0U;
  std::size_t validatedOpeningCount = 0U;
  std::size_t validatedVerticalConnectorCount = 0U;
  std::uint16_t resolvedWallHeightCells = 0U;
  double resolvedFloorToFloorMeters = 0.0;
  CreativeWorldLayoutBuildingDimensions before;
  CreativeWorldLayoutBuildingDimensions after;
  std::string_view reasonCode =
      "creative_world_layout_architecture_not_requested";
};

struct CreativeWorldLayoutArchitectureResult {
  CreativeWorldLayoutArchitectureReceipt receipt;
  CreativeWorldLayout edited;
};

[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutArchitecturalProfileKind kind) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutArchitectureStatus status) noexcept;

[[nodiscard]] CreativeWorldLayoutArchitecturalProfile
defaultCreativeWorldLayoutArchitecturalProfile(
    CreativeWorldLayoutArchitecturalProfileKind kind) noexcept;
[[nodiscard]] bool validCreativeWorldLayoutArchitecturalProfile(
    const CreativeWorldLayoutArchitecturalProfile& profile) noexcept;
[[nodiscard]] bool resolveCreativeWorldLayoutArchitecturalProfileHeightCells(
    const CreativeGridSettings& grid,
    const CreativeWorldLayoutArchitecturalProfile& profile,
    std::uint16_t& output) noexcept;

// Normalizes one building in a single candidate copy. Horizontal footprints,
// names, stable keys, opening-local dimensions, neighboring buildings, and
// terrain remain unchanged. Level order is preserved while elevations become
// a contiguous low-to-high sequence. Level-aligned walls and structural boxes
// follow their mapped planes; ambiguous custom vertical geometry rejects.
// Complexity is O(levels^2 + walls*levels + boxes*levels + openings +
// connectors), with deterministic source-order traversal.
[[nodiscard]] CreativeWorldLayoutArchitectureResult
normalizeCreativeWorldLayoutBuildingArchitecture(
    const CreativeGridSettings& grid,
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutArchitectureRequest& request);

}  // namespace iggy3d::creative
