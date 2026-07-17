#pragma once

#include "app/iggy3d/creative/world/WorldLayout.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace iggy3d::creative {

enum class CreativeWorldLayoutRoofStatus : std::uint8_t {
  NotRequested,
  InvalidLevel,
  NotTopmost,
  InvalidFootprint,
  NonRectangularFootprint,
  InvalidGrid,
  InvalidSettings,
  RecipeRejected,
  Ready,
};

struct CreativeWorldLayoutRoofPlan {
  bool accepted = false;
  CreativeWorldLayoutRoofStatus status =
      CreativeWorldLayoutRoofStatus::NotRequested;
  std::size_t buildingIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t levelIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayoutRect footprint;
  CreativeStructuralRoofRecipeResult geometry;
  std::string_view reasonCode = "creative_world_layout_roof_not_requested";
};

// Returns one exact rectangular roof footprint only when the rooms on the
// level tile that rectangle without gaps or overlap.
[[nodiscard]] bool creativeWorldLayoutLevelRoofFootprint(
    const CreativeWorldLayout& layout,
    std::size_t levelIndex,
    CreativeWorldLayoutRect& output) noexcept;

[[nodiscard]] CreativeWorldLayoutRoofPlan planCreativeWorldLayoutRoof(
    const CreativeGridSettings& grid,
    const CreativeWorldLayout& layout,
    std::size_t levelIndex) noexcept;

}  // namespace iggy3d::creative
