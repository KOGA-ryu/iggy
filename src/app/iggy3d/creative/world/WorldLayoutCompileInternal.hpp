#pragma once

#include "app/iggy3d/creative/world/WorldLayout.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace iggy3d::creative::world_layout_compile {

void setStatus(CreativeWorldLayoutReceipt& receipt,
               CreativeWorldLayoutStatus status,
               std::string_view reasonCode,
               bool accepted = false);

[[nodiscard]] bool validDocument(
    const CreativeDocument& document) noexcept;
[[nodiscard]] bool validStableKey(std::string_view key) noexcept;
[[nodiscard]] bool validTerrainOwnership(
    CreativeWorldLayoutTerrainOwnership ownership) noexcept;
[[nodiscard]] bool validRect(CreativeWorldLayoutRect rect) noexcept;

[[nodiscard]] bool worldCoordinate(double origin,
                                   double cellSize,
                                   long double coordinate,
                                   double& output) noexcept;

[[nodiscard]] bool layoutBounds(const CreativeGridSettings& grid,
                                CreativeWorldLayoutRect rect,
                                double baseLayer,
                                std::uint16_t heightCells,
                                CreativeBounds& output) noexcept;

[[nodiscard]] bool layoutPoint(const CreativeGridSettings& grid,
                               CreativeTerrainCoord2 coord,
                               double layer,
                               CreativeVec3& output) noexcept;

[[nodiscard]] bool layoutPoint(const CreativeGridSettings& grid,
                               CreativeVec3 cells,
                               CreativeVec3& output) noexcept;

[[nodiscard]] bool layoutBounds(const CreativeGridSettings& grid,
                                CreativeBounds cells,
                                CreativeBounds& output) noexcept;

[[nodiscard]] std::string childKey(std::string_view buildingKey,
                                   std::string_view localKey);

[[nodiscard]] bool registerKey(std::unordered_set<std::string>& keys,
                               std::string key,
                               CreativeWorldLayoutTable table,
                               std::size_t index,
                               CreativeWorldLayoutReceipt& receipt);

void appendTagOnce(std::vector<std::string>& tags, std::string tag);

[[nodiscard]] bool collectObjectRemovalOrder(
    const CreativeDocument& document,
    std::span<const CreativeObjectId> removeIds,
    std::vector<CreativeObjectId>& output);

[[nodiscard]] bool stageWorldLayoutTerrain(
    const CreativeDocument& document,
    const CreativeWorldLayout& layout,
    std::unordered_set<std::string>& stableKeys,
    CreativeWorldLayoutCompileResult& result,
    CreativeDocument& staged);

[[nodiscard]] bool shiftBuildingVertically(
    CreativeBuildingRecipeRequest& building,
    double offsetMeters) noexcept;

[[nodiscard]] bool reconcileWorldLayoutRecipes(
    const CreativeDocument& document,
    std::string_view layoutTag,
    CreativeWorldLayoutCompileOptions options,
    std::vector<CreativeRecipePlan>& desiredObjectRecipes,
    CreativeWorldLayoutCompileResult& result);

}  // namespace iggy3d::creative::world_layout_compile
