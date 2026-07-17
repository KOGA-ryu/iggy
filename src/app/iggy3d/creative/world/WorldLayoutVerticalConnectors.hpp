#pragma once

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
  std::string_view reasonCode =
      "creative_world_layout_vertical_connector_not_requested";
};

[[nodiscard]] CreativeWorldLayoutVerticalConnectorPlan
planCreativeWorldLayoutVerticalConnector(const CreativeGridSettings& grid,
                                         const CreativeWorldLayout& layout,
                                         std::size_t connectorIndex) noexcept;

} // namespace iggy3d::creative
