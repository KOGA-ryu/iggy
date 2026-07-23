#pragma once

#include "app/iggy3d/creative/recipes/TerrainGrounding.hpp"
#include "app/iggy3d/creative/world/WorldLayoutBuildingOps.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace iggy3d::creative {

enum class CreativeWorldLayoutBuildingTemplateTerrainImpact : std::uint8_t {
  Unknown,
  None,
  Grounded,
  Foundation,
};

enum class CreativeWorldLayoutBuildingTemplatePlacementStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  InvalidOwnership,
  CoordinateOverflow,
  BuildingOverlap,
  TerrainRejected,
  Ready,
};

struct CreativeWorldLayoutBuildingTemplatePlacementRequest {
  const CreativeWorldLayout* destination = nullptr;
  const CreativeWorldLayoutBuildingTemplate* sourceTemplate = nullptr;
  CreativeTerrainCoord2 anchor{};
  CreativeGridSettings grid{};
  const CreativeTerrainSurfacePlan* terrain = nullptr;
};

struct CreativeWorldLayoutBuildingTemplatePlacementAnalysis {
  bool requested = false;
  bool accepted = false;
  CreativeWorldLayoutBuildingTemplatePlacementStatus status =
      CreativeWorldLayoutBuildingTemplatePlacementStatus::NotRequested;
  CreativeWorldLayoutBuildingBounds bounds;
  CreativeWorldLayoutRect footprint;
  std::uint64_t templateVersion = 0U;
  std::size_t levelCount = 0U;
  std::size_t entranceCount = 0U;
  std::size_t conflictingBuildingIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayoutGroundingMode groundingMode =
      CreativeWorldLayoutGroundingMode::Absolute;
  CreativeWorldLayoutBuildingTemplateTerrainImpact terrainImpact =
      CreativeWorldLayoutBuildingTemplateTerrainImpact::Unknown;
  CreativeTerrainGroundingPlan grounding;
  std::string_view reasonCode =
      "creative_world_layout_building_template_placement_not_requested";
};

[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutBuildingTemplateTerrainImpact impact) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutBuildingTemplatePlacementStatus status) noexcept;

// Shared by placement analysis and the compiler so previewed terrain impact is
// exactly the impact applied when the building is materialized in 3D.
[[nodiscard]] CreativeTerrainGroundingPlan
planCreativeWorldLayoutBuildingGrounding(
    const CreativeWorldLayout& layout, std::size_t buildingIndex,
    const CreativeGridSettings& grid, const CreativeTerrainSurfacePlan& terrain,
    CreativeTerrainCoord2 footprintOffset = {}) noexcept;

// One allocation-bounded control-path query. It reports the complete template
// placement contract without mutating the layout: content version, footprint,
// exterior entrances, levels, overlap, and exact terrain grounding outcome.
[[nodiscard]] CreativeWorldLayoutBuildingTemplatePlacementAnalysis
analyzeCreativeWorldLayoutBuildingTemplatePlacement(
    const CreativeWorldLayoutBuildingTemplatePlacementRequest& request);

}  // namespace iggy3d::creative
