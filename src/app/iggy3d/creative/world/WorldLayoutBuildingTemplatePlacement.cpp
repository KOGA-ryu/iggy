#include "app/iggy3d/creative/world/WorldLayoutBuildingTemplatePlacement.hpp"

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/world/WorldLayoutOpenings.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <utility>

namespace iggy3d::creative {
namespace {

constexpr double kPlacementOverlapEpsilon = 1.0e-9;

struct VerticalSpan {
  bool valid = false;
  double minimum = 0.0;
  double maximum = 0.0;
};

bool shiftedCoordinate(std::int32_t value, std::int32_t offset,
                       std::int32_t& output) noexcept {
  const std::int64_t shifted = static_cast<std::int64_t>(value) + offset;
  if (shifted < std::numeric_limits<std::int32_t>::min() ||
      shifted > std::numeric_limits<std::int32_t>::max()) {
    return false;
  }
  output = static_cast<std::int32_t>(shifted);
  return true;
}

bool shiftedRect(CreativeWorldLayoutRect source, CreativeTerrainCoord2 offset,
                 CreativeWorldLayoutRect& output) noexcept {
  return shiftedCoordinate(source.minimum.x, offset.x, output.minimum.x) &&
         shiftedCoordinate(source.minimum.z, offset.z, output.minimum.z) &&
         shiftedCoordinate(source.maximum.x, offset.x, output.maximum.x) &&
         shiftedCoordinate(source.maximum.z, offset.z, output.maximum.z);
}

bool shiftedBounds(CreativeWorldLayoutBuildingBounds source,
                   CreativeTerrainCoord2 offset,
                   CreativeWorldLayoutBuildingBounds& output) noexcept {
  output.valid = source.valid;
  return source.valid &&
         shiftedCoordinate(source.minimum.x, offset.x, output.minimum.x) &&
         shiftedCoordinate(source.minimum.z, offset.z, output.minimum.z) &&
         shiftedCoordinate(source.maximum.x, offset.x, output.maximum.x) &&
         shiftedCoordinate(source.maximum.z, offset.z, output.maximum.z);
}

bool overlaps(CreativeWorldLayoutRect left,
              CreativeWorldLayoutRect right) noexcept {
  return std::max(left.minimum.x, right.minimum.x) <
             std::min(left.maximum.x, right.maximum.x) &&
         std::max(left.minimum.z, right.minimum.z) <
             std::min(left.maximum.z, right.maximum.z);
}

bool overlaps(VerticalSpan left, VerticalSpan right) noexcept {
  return left.valid && right.valid &&
         std::max(left.minimum, right.minimum) <
             std::min(left.maximum, right.maximum) -
                 kPlacementOverlapEpsilon;
}

void includeSpan(VerticalSpan& span, double minimum, double maximum) noexcept {
  if (!std::isfinite(minimum) || !std::isfinite(maximum) ||
      maximum <= minimum) {
    return;
  }
  if (!span.valid) {
    span = {true, minimum, maximum};
    return;
  }
  span.minimum = std::min(span.minimum, minimum);
  span.maximum = std::max(span.maximum, maximum);
}

VerticalSpan buildingSpan(const CreativeWorldLayout& layout,
                          std::size_t buildingIndex) noexcept {
  VerticalSpan span;
  for (const CreativeWorldLayoutLevel& level : layout.levels) {
    if (level.buildingIndex == buildingIndex) {
      includeSpan(span, level.floorTopLayer,
                  level.floorTopLayer + level.wallHeightCells);
    }
  }
  for (const CreativeWorldLayoutBox& box : layout.boxes) {
    if (box.buildingIndex == buildingIndex) {
      includeSpan(span, box.anchorLayer, box.anchorLayer + box.layerCount);
    }
  }
  for (const CreativeWorldLayoutWall& wall : layout.walls) {
    if (wall.buildingIndex == buildingIndex) {
      includeSpan(span, wall.baseLayer, wall.baseLayer + wall.heightCells);
    }
  }
  if (!span.valid && buildingIndex < layout.buildings.size()) {
    const CreativeWorldLayoutBuilding& building = layout.buildings[buildingIndex];
    includeSpan(span, building.rootBaseLayer,
                static_cast<double>(building.rootBaseLayer) +
                    building.rootHeightCells);
  }
  return span;
}

bool buildingsOverlap(const CreativeWorldLayout& positioned,
                      const CreativeWorldLayout& destination,
                      std::size_t destinationBuildingIndex) noexcept {
  CreativeWorldLayoutBuildingBounds sourceBounds;
  CreativeWorldLayoutBuildingBounds destinationBounds;
  return measureCreativeWorldLayoutBuildingBounds(positioned, 0U,
                                                  sourceBounds) &&
         measureCreativeWorldLayoutBuildingBounds(
             destination, destinationBuildingIndex, destinationBounds) &&
         overlaps({sourceBounds.minimum, sourceBounds.maximum},
                  {destinationBounds.minimum, destinationBounds.maximum}) &&
         overlaps(buildingSpan(positioned, 0U),
                  buildingSpan(destination, destinationBuildingIndex));
}

std::size_t exteriorEntranceCount(const CreativeWorldLayout& layout) noexcept {
  std::size_t count = 0U;
  for (const CreativeWorldLayoutOpening& opening : layout.openings) {
    if (opening.kind != CreativeBuildingOpeningKind::Door) {
      continue;
    }
    const CreativeWorldLayoutOpeningHostFrame host =
        resolveCreativeWorldLayoutOpeningHost(layout, opening);
    if (host.accepted && host.buildingIndex == 0U && host.exterior) {
      ++count;
    }
  }
  return count;
}

CreativeWorldLayoutRect templateFootprint(
    const CreativeWorldLayoutBuildingTemplate& source) noexcept {
  const CreativeWorldLayoutBuilding& building =
      source.normalizedLayout.buildings[0];
  return building.rootMode != CreativeBuildingRootMode::None ||
                 building.groundingMode ==
                     CreativeWorldLayoutGroundingMode::Foundation
             ? building.rootFootprint
             : CreativeWorldLayoutRect{source.bounds.minimum,
                                       source.bounds.maximum};
}

}  // namespace

std::string_view toString(
    CreativeWorldLayoutBuildingTemplateTerrainImpact impact) noexcept {
  switch (impact) {
    case CreativeWorldLayoutBuildingTemplateTerrainImpact::Unknown:
      return "Unknown";
    case CreativeWorldLayoutBuildingTemplateTerrainImpact::None:
      return "None";
    case CreativeWorldLayoutBuildingTemplateTerrainImpact::Grounded:
      return "Grounded";
    case CreativeWorldLayoutBuildingTemplateTerrainImpact::Foundation:
      return "Foundation";
  }
  return "Invalid";
}

std::string_view toString(
    CreativeWorldLayoutBuildingTemplatePlacementStatus status) noexcept {
  switch (status) {
    case CreativeWorldLayoutBuildingTemplatePlacementStatus::NotRequested:
      return "NotRequested";
    case CreativeWorldLayoutBuildingTemplatePlacementStatus::InvalidRequest:
      return "InvalidRequest";
    case CreativeWorldLayoutBuildingTemplatePlacementStatus::InvalidOwnership:
      return "InvalidOwnership";
    case CreativeWorldLayoutBuildingTemplatePlacementStatus::CoordinateOverflow:
      return "CoordinateOverflow";
    case CreativeWorldLayoutBuildingTemplatePlacementStatus::BuildingOverlap:
      return "BuildingOverlap";
    case CreativeWorldLayoutBuildingTemplatePlacementStatus::TerrainRejected:
      return "TerrainRejected";
    case CreativeWorldLayoutBuildingTemplatePlacementStatus::Ready:
      return "Ready";
  }
  return "Invalid";
}

CreativeTerrainGroundingPlan planCreativeWorldLayoutBuildingGrounding(
    const CreativeWorldLayout& layout, std::size_t buildingIndex,
    const CreativeGridSettings& grid, const CreativeTerrainSurfacePlan& terrain,
    CreativeTerrainCoord2 footprintOffset) noexcept {
  CreativeTerrainGroundingPlan invalid;
  invalid.requested = true;
  invalid.status = CreativeTerrainGroundingStatus::InvalidRequest;
  invalid.reasonCode = "creative_world_layout_building_grounding_request_invalid";
  if (buildingIndex >= layout.buildings.size() ||
      !std::isfinite(grid.cellSizeMeters) || grid.cellSizeMeters <= 0.0) {
    return invalid;
  }
  const CreativeWorldLayoutBuilding& building = layout.buildings[buildingIndex];
  if (building.groundingMode != CreativeWorldLayoutGroundingMode::Foundation) {
    return invalid;
  }
  CreativeWorldLayoutRect footprint;
  if (!shiftedRect(building.rootFootprint, footprintOffset, footprint)) {
    return invalid;
  }

  const double floorLayerThickness =
      defaultCreativeStructuralLayerThicknessMeters(CreativeObjectKind::Floor);
  double authoredGroundLayer = std::numeric_limits<double>::infinity();
  for (const CreativeWorldLayoutLevel& level : layout.levels) {
    if (level.buildingIndex == buildingIndex) {
      const double floorThicknessLayers =
          static_cast<double>(level.floorThicknessLayers) *
          floorLayerThickness / grid.cellSizeMeters;
      authoredGroundLayer =
          std::min(authoredGroundLayer,
                   level.floorTopLayer - floorThicknessLayers);
    }
  }
  if (!std::isfinite(authoredGroundLayer)) {
    authoredGroundLayer = static_cast<double>(building.rootBaseLayer);
  }
  return planCreativeTerrainGrounding(
      {&terrain, footprint.minimum, footprint.maximum, authoredGroundLayer,
       building.maximumGroundReliefCells});
}

CreativeWorldLayoutBuildingTemplatePlacementAnalysis
analyzeCreativeWorldLayoutBuildingTemplatePlacement(
    const CreativeWorldLayoutBuildingTemplatePlacementRequest& request) {
  CreativeWorldLayoutBuildingTemplatePlacementAnalysis result;
  result.requested = true;
  if (request.destination == nullptr || request.sourceTemplate == nullptr ||
      !validCreativeWorldLayoutBuildingTemplate(*request.sourceTemplate)) {
    result.status =
        CreativeWorldLayoutBuildingTemplatePlacementStatus::InvalidRequest;
    result.reasonCode =
        "creative_world_layout_building_template_placement_request_invalid";
    return result;
  }
  if (!validCreativeWorldLayoutBuildingOwnership(*request.destination)) {
    result.status =
        CreativeWorldLayoutBuildingTemplatePlacementStatus::InvalidOwnership;
    result.reasonCode =
        "creative_world_layout_building_template_placement_ownership_invalid";
    return result;
  }

  const CreativeWorldLayoutBuildingTemplate& source = *request.sourceTemplate;
  result.templateVersion = source.sourceFingerprint.value;
  result.groundingMode = source.normalizedLayout.buildings[0].groundingMode;
  result.levelCount = static_cast<std::size_t>(std::count_if(
      source.normalizedLayout.levels.begin(),
      source.normalizedLayout.levels.end(),
      [](const CreativeWorldLayoutLevel& level) {
        return level.buildingIndex == 0U;
      }));
  result.entranceCount = exteriorEntranceCount(source.normalizedLayout);
  if (!shiftedBounds(source.bounds, request.anchor, result.bounds) ||
      !shiftedRect(templateFootprint(source), request.anchor,
                   result.footprint)) {
    result.status =
        CreativeWorldLayoutBuildingTemplatePlacementStatus::CoordinateOverflow;
    result.reasonCode =
        "creative_world_layout_building_template_placement_coordinate_overflow";
    return result;
  }

  CreativeWorldLayout positioned = source.normalizedLayout;
  if (request.anchor != CreativeTerrainCoord2{}) {
    CreativeWorldLayoutBuildingEditResult moved = moveCreativeWorldLayoutBuilding(
        positioned, {0U, request.anchor.x, request.anchor.z});
    if (!moved.accepted) {
      result.status = CreativeWorldLayoutBuildingTemplatePlacementStatus::
          CoordinateOverflow;
      result.reasonCode =
          "creative_world_layout_building_template_placement_coordinate_overflow";
      return result;
    }
    positioned = std::move(moved.edited);
  }

  for (std::size_t buildingIndex = 0U;
       buildingIndex < request.destination->buildings.size(); ++buildingIndex) {
    if (buildingsOverlap(positioned, *request.destination, buildingIndex)) {
      result.conflictingBuildingIndex = buildingIndex;
      break;
    }
  }

  if (result.groundingMode == CreativeWorldLayoutGroundingMode::Absolute) {
    result.terrainImpact =
        CreativeWorldLayoutBuildingTemplateTerrainImpact::None;
  } else if (request.terrain == nullptr) {
    result.terrainImpact =
        CreativeWorldLayoutBuildingTemplateTerrainImpact::Unknown;
  } else {
    result.grounding = planCreativeWorldLayoutBuildingGrounding(
        source.normalizedLayout, 0U, request.grid, *request.terrain,
        request.anchor);
    if (result.grounding.accepted) {
      result.terrainImpact =
          result.grounding.reliefCells > 0U
              ? CreativeWorldLayoutBuildingTemplateTerrainImpact::Foundation
              : CreativeWorldLayoutBuildingTemplateTerrainImpact::Grounded;
    }
  }

  if (result.conflictingBuildingIndex != kInvalidCreativeWorldLayoutIndex) {
    result.status =
        CreativeWorldLayoutBuildingTemplatePlacementStatus::BuildingOverlap;
    result.reasonCode =
        "creative_world_layout_building_template_placement_building_overlap";
    return result;
  }
  if (result.groundingMode == CreativeWorldLayoutGroundingMode::Foundation &&
      !result.grounding.accepted) {
    result.status =
        CreativeWorldLayoutBuildingTemplatePlacementStatus::TerrainRejected;
    result.reasonCode =
        request.terrain == nullptr
            ? "creative_world_layout_building_template_placement_terrain_missing"
            : result.grounding.reasonCode;
    return result;
  }
  result.accepted = true;
  result.status = CreativeWorldLayoutBuildingTemplatePlacementStatus::Ready;
  result.reasonCode =
      "creative_world_layout_building_template_placement_ready";
  return result;
}

}  // namespace iggy3d::creative
