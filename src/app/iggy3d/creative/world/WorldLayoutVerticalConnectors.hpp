#pragma once

#include "app/iggy3d/creative/recipes/RampRecipe.hpp"
#include "app/iggy3d/creative/recipes/StairRecipe.hpp"
#include "app/iggy3d/creative/world/WorldLayout.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace iggy3d::creative {

enum class CreativeWorldLayoutVerticalConnectorStatus : std::uint8_t {
  NotRequested,
  InvalidConnector,
  InvalidOwner,
  InvalidLevels,
  InvalidFootprint,
  InvalidLanding,
  InvalidSlope,
  InvalidMaterial,
  InvalidHeadroom,
  SurfaceAlreadyCut,
  UnrepresentableGeometry,
  Ready,
};

struct CreativeWorldLayoutVerticalConnectorPlan {
  bool accepted = false;
  CreativeWorldLayoutVerticalConnectorStatus status =
      CreativeWorldLayoutVerticalConnectorStatus::NotRequested;
  std::size_t buildingIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t lowerRoomIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t upperRoomIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeObjectKind objectKind = CreativeObjectKind::Unknown;
  CreativeWorldLayoutRect openingFootprint;
  CreativeBounds authoredBounds;
  CreativeVec3 rotationEulerRadians;
  double riseMeters = 0.0;
  double runMeters = 0.0;
  double widthMeters = 0.0;
  std::uint16_t stepCount = 0U;
  CreativeStructuralMaterial material = CreativeStructuralMaterial::Blockout;
  CreativeRampRecipeResult ramp;
  CreativeStairRecipeResult stair;
  std::string_view reasonCode =
      "creative_world_layout_vertical_connector_not_requested";
};

[[nodiscard]] CreativeWorldLayoutVerticalConnectorPlan
planCreativeWorldLayoutVerticalConnector(const CreativeGridSettings& grid,
                                         const CreativeWorldLayout& layout,
                                         std::size_t connectorIndex);

// Evaluates an edited connector against the live layout without copying the
// complete layout. The candidate replaces connectorIndex for this plan only.
[[nodiscard]] CreativeWorldLayoutVerticalConnectorPlan
planCreativeWorldLayoutVerticalConnector(
    const CreativeGridSettings& grid, const CreativeWorldLayout& layout,
    std::size_t connectorIndex,
    const CreativeWorldLayoutVerticalConnector& candidate);

} // namespace iggy3d::creative
