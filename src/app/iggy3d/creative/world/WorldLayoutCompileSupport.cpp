#include "app/iggy3d/creative/world/WorldLayoutCompileInternal.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace iggy3d::creative::world_layout_compile {

void setStatus(CreativeWorldLayoutReceipt& receipt,
               CreativeWorldLayoutStatus status,
               std::string_view reasonCode,
               bool accepted) {
  receipt.status = status;
  receipt.reasonCode = std::string(reasonCode);
  receipt.accepted = accepted;
}

[[nodiscard]] bool validDocument(const CreativeDocument& document) noexcept {
  return document.isValid() && document.id() != kInvalidDocumentId &&
         document.nextObjectId() != kInvalidObjectId &&
         document.terrainField().validateInvariants() &&
         document.terrainHeightField().validateInvariants() &&
         document.terrainMaterialField().validateInvariants();
}

[[nodiscard]] bool validStableKey(std::string_view key) noexcept {
  if (key.empty() || key.size() > 128U) {
    return false;
  }
  return std::all_of(key.begin(), key.end(), [](char value) {
    const unsigned char character = static_cast<unsigned char>(value);
    return std::isalnum(character) != 0 || value == '_' || value == '-' ||
           value == '.';
  });
}

[[nodiscard]] bool validTerrainOwnership(
    CreativeWorldLayoutTerrainOwnership ownership) noexcept {
  return ownership == CreativeWorldLayoutTerrainOwnership::PreserveExisting ||
         ownership == CreativeWorldLayoutTerrainOwnership::ReplaceAll;
}

[[nodiscard]] bool coordLess(CreativeTerrainCoord2 lhs,
                             CreativeTerrainCoord2 rhs) noexcept {
  return lhs.z < rhs.z || (lhs.z == rhs.z && lhs.x < rhs.x);
}

[[nodiscard]] bool validRect(CreativeWorldLayoutRect rect) noexcept {
  return rect.minimum.x < rect.maximum.x &&
         rect.minimum.z < rect.maximum.z;
}

[[nodiscard]] bool worldCoordinate(double origin,
                                   double cellSize,
                                   long double coordinate,
                                   double& output) noexcept {
  const long double value = static_cast<long double>(origin) +
                            static_cast<long double>(cellSize) * coordinate;
  if (!std::isfinite(value) ||
      value < -std::numeric_limits<double>::max() ||
      value > std::numeric_limits<double>::max()) {
    return false;
  }
  output = static_cast<double>(value);
  return std::isfinite(output);
}

[[nodiscard]] bool layoutBounds(const CreativeGridSettings& grid,
                                CreativeWorldLayoutRect rect,
                                double baseLayer,
                                std::uint16_t heightCells,
                                CreativeBounds& output) noexcept {
  if (!validRect(rect) || heightCells == 0U ||
      !std::isfinite(grid.cellSizeMeters) || grid.cellSizeMeters <= 0.0) {
    return false;
  }
  return worldCoordinate(grid.origin.x, grid.cellSizeMeters, rect.minimum.x,
                         output.min.x) &&
         worldCoordinate(grid.origin.z, grid.cellSizeMeters, rect.minimum.z,
                         output.min.z) &&
         worldCoordinate(grid.origin.y, grid.cellSizeMeters, baseLayer,
                         output.min.y) &&
         worldCoordinate(grid.origin.x, grid.cellSizeMeters, rect.maximum.x,
                         output.max.x) &&
         worldCoordinate(grid.origin.z, grid.cellSizeMeters, rect.maximum.z,
                         output.max.z) &&
         worldCoordinate(grid.origin.y, grid.cellSizeMeters,
                         static_cast<long double>(baseLayer) + heightCells,
                         output.max.y);
}

[[nodiscard]] bool layoutPoint(const CreativeGridSettings& grid,
                               CreativeTerrainCoord2 coord,
                               double layer,
                               CreativeVec3& output) noexcept {
  return worldCoordinate(grid.origin.x, grid.cellSizeMeters, coord.x,
                         output.x) &&
         worldCoordinate(grid.origin.y, grid.cellSizeMeters, layer,
                         output.y) &&
         worldCoordinate(grid.origin.z, grid.cellSizeMeters, coord.z,
                         output.z);
}

[[nodiscard]] bool layoutPoint(const CreativeGridSettings& grid,
                               CreativeVec3 cells,
                               CreativeVec3& output) noexcept {
  return worldCoordinate(grid.origin.x, grid.cellSizeMeters, cells.x,
                         output.x) &&
         worldCoordinate(grid.origin.y, grid.cellSizeMeters, cells.y,
                         output.y) &&
         worldCoordinate(grid.origin.z, grid.cellSizeMeters, cells.z,
                         output.z);
}

[[nodiscard]] bool layoutBounds(const CreativeGridSettings& grid,
                                CreativeBounds cells,
                                CreativeBounds& output) noexcept {
  return layoutPoint(grid, cells.min, output.min) &&
         layoutPoint(grid, cells.max, output.max);
}

[[nodiscard]] std::string childKey(std::string_view buildingKey,
                                   std::string_view localKey) {
  return std::string(buildingKey) + "." + std::string(localKey);
}

[[nodiscard]] bool registerKey(std::unordered_set<std::string>& keys,
                               std::string key,
                               CreativeWorldLayoutTable table,
                               std::size_t index,
                               CreativeWorldLayoutReceipt& receipt) {
  if (!validStableKey(key)) {
    receipt.failedTable = table;
    receipt.failedIndex = index;
    setStatus(receipt, CreativeWorldLayoutStatus::InvalidSymbol,
              "creative_world_layout_stable_key_invalid");
    return false;
  }
  if (!keys.insert(std::move(key)).second) {
    receipt.failedTable = table;
    receipt.failedIndex = index;
    setStatus(receipt, CreativeWorldLayoutStatus::DuplicateStableKey,
              "creative_world_layout_stable_key_duplicate");
    return false;
  }
  return true;
}

[[nodiscard]] bool hasTag(std::span<const std::string> tags,
                          std::string_view tag) noexcept {
  return std::any_of(tags.begin(), tags.end(),
                     [tag](const std::string& value) { return value == tag; });
}

void appendTagOnce(std::vector<std::string>& tags, std::string tag) {
  if (!tag.empty() && !hasTag(tags, tag)) {
    tags.push_back(std::move(tag));
  }
}

[[nodiscard]] bool collectObjectRemovalOrder(
    const CreativeDocument& document,
    std::span<const CreativeObjectId> removeIds,
    std::vector<CreativeObjectId>& output) {
  std::unordered_set<CreativeObjectId> remaining(removeIds.begin(),
                                                  removeIds.end());
  if (remaining.size() != removeIds.size()) {
    return false;
  }
  std::vector<CreativeObjectId> orderedIds(removeIds.begin(), removeIds.end());
  if (std::any_of(orderedIds.begin(), orderedIds.end(),
                  [&](CreativeObjectId id) {
                    return document.findObject(id) == nullptr;
                  })) {
    return false;
  }
  output.reserve(remaining.size());
  while (!remaining.empty()) {
    const auto leaf = std::find_if(
        orderedIds.begin(), orderedIds.end(), [&](CreativeObjectId candidate) {
          if (!remaining.contains(candidate)) {
            return false;
          }
          return std::none_of(
              document.objects().begin(), document.objects().end(),
              [&](const CreativeObject& object) {
                return remaining.contains(object.id) &&
                       object.parentId == candidate;
              });
        });
    if (leaf == orderedIds.end()) {
      output.clear();
      return false;
    }
    output.push_back(*leaf);
    remaining.erase(*leaf);
  }
  return true;
}

[[nodiscard]] bool clearTerrain(CreativeDocument& document) {
  std::vector<CreativeTerrainControlEdit> terrain;
  terrain.reserve(document.terrainField().controls().size());
  for (const CreativeTerrainControlPoint& control :
       document.terrainField().controls()) {
    terrain.push_back({CreativeTerrainEditKind::Remove, control});
  }
  if (!terrain.empty() &&
      !document.applyTerrainControlEdits(terrain).accepted) {
    return false;
  }

  std::vector<CreativeTerrainMaterialEdit> materials;
  materials.reserve(document.terrainMaterialField().overrides().size());
  for (const CreativeTerrainMaterialOverride& value :
       document.terrainMaterialField().overrides()) {
    materials.push_back(
        {CreativeTerrainMaterialEditKind::Clear, value.coord, value.material});
  }
  return materials.empty() ||
         document.applyTerrainMaterialEdits(materials).accepted;
}

[[nodiscard]] std::vector<CreativeTerrainControlEdit> terrainDiff(
    std::span<const CreativeTerrainControlPoint> before,
    std::span<const CreativeTerrainControlPoint> after) {
  std::vector<CreativeTerrainControlEdit> edits;
  std::size_t beforeIndex = 0U;
  std::size_t afterIndex = 0U;
  while (beforeIndex < before.size() || afterIndex < after.size()) {
    if (afterIndex >= after.size() ||
        (beforeIndex < before.size() &&
         coordLess(before[beforeIndex].coord, after[afterIndex].coord))) {
      edits.push_back(
          {CreativeTerrainEditKind::Remove, before[beforeIndex++]});
      continue;
    }
    if (beforeIndex >= before.size() ||
        coordLess(after[afterIndex].coord, before[beforeIndex].coord)) {
      edits.push_back(
          {CreativeTerrainEditKind::Upsert, after[afterIndex++]});
      continue;
    }
    if (!(before[beforeIndex] == after[afterIndex])) {
      edits.push_back(
          {CreativeTerrainEditKind::Upsert, after[afterIndex]});
    }
    ++beforeIndex;
    ++afterIndex;
  }
  return edits;
}

[[nodiscard]] std::vector<CreativeTerrainMaterialEdit> materialDiff(
    std::span<const CreativeTerrainMaterialOverride> before,
    std::span<const CreativeTerrainMaterialOverride> after) {
  std::vector<CreativeTerrainMaterialEdit> edits;
  std::size_t beforeIndex = 0U;
  std::size_t afterIndex = 0U;
  while (beforeIndex < before.size() || afterIndex < after.size()) {
    if (afterIndex >= after.size() ||
        (beforeIndex < before.size() &&
         coordLess(before[beforeIndex].coord, after[afterIndex].coord))) {
      edits.push_back({CreativeTerrainMaterialEditKind::Clear,
                       before[beforeIndex].coord,
                       before[beforeIndex].material});
      ++beforeIndex;
      continue;
    }
    if (beforeIndex >= before.size() ||
        coordLess(after[afterIndex].coord, before[beforeIndex].coord)) {
      edits.push_back({CreativeTerrainMaterialEditKind::Set,
                       after[afterIndex].coord,
                       after[afterIndex].material});
      ++afterIndex;
      continue;
    }
    if (!(before[beforeIndex] == after[afterIndex])) {
      edits.push_back({CreativeTerrainMaterialEditKind::Set,
                       after[afterIndex].coord,
                       after[afterIndex].material});
    }
    ++beforeIndex;
    ++afterIndex;
  }
  return edits;
}

[[nodiscard]] bool stageWorldLayoutTerrain(
    const CreativeDocument& document,
    const CreativeWorldLayout& layout,
    std::unordered_set<std::string>& stableKeys,
    CreativeWorldLayoutCompileResult& result,
    CreativeDocument& staged) {
  if (layout.terrainOwnership ==
          CreativeWorldLayoutTerrainOwnership::ReplaceAll &&
      !clearTerrain(staged)) {
    setStatus(result.receipt, CreativeWorldLayoutStatus::MutationRejected,
              "creative_world_layout_terrain_clear_rejected");
    return false;
  }

  for (std::size_t index = 0U; index < layout.terrainProfiles.size(); ++index) {
    const CreativeWorldLayoutTerrainProfile& symbol =
        layout.terrainProfiles[index];
    if (!registerKey(stableKeys, symbol.stableKey,
                     CreativeWorldLayoutTable::TerrainProfile, index,
                     result.receipt)) {
      return false;
    }
    if (symbol.blend != CreativeTerrainProfileBlend::Set ||
        symbol.rodPolicy != CreativeTerrainProfileRodPolicy::Fill) {
      result.receipt.failedTable = CreativeWorldLayoutTable::TerrainProfile;
      result.receipt.failedIndex = index;
      setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                "creative_world_layout_profile_not_absolute");
      return false;
    }
    CreativeTerrainProfileRecipeRequest request;
    request.document = &staged;
    request.kind = symbol.kind;
    request.center = symbol.center;
    request.baseHeightCells = symbol.baseHeightCells;
    request.radiusCells = symbol.radiusCells;
    request.amplitudeCells = symbol.amplitudeCells;
    request.spacingCells = symbol.spacingCells;
    request.blend = symbol.blend;
    request.rodPolicy = symbol.rodPolicy;
    request.direction = symbol.direction;
    request.frequency = symbol.frequency;
    const CreativeTerrainRecipeResult recipe =
        buildCreativeTerrainProfileRecipe(request);
    if (!recipe.receipt.accepted) {
      result.receipt.failedTable = CreativeWorldLayoutTable::TerrainProfile;
      result.receipt.failedIndex = index;
      result.receipt.kernelReasonCode = recipe.receipt.kernelReasonCode;
      setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                recipe.receipt.reasonCode);
      return false;
    }
    if (!recipe.plan.controlEdits.empty() &&
        !staged.applyTerrainControlEdits(recipe.plan.controlEdits).accepted) {
      result.receipt.failedTable = CreativeWorldLayoutTable::TerrainProfile;
      result.receipt.failedIndex = index;
      setStatus(result.receipt, CreativeWorldLayoutStatus::MutationRejected,
                "creative_world_layout_profile_stage_rejected");
      return false;
    }
  }

  std::vector<bool> pointOwned(layout.terrainPathPoints.size(), false);
  for (std::size_t index = 0U; index < layout.terrainPaths.size(); ++index) {
    const CreativeWorldLayoutTerrainPath& symbol = layout.terrainPaths[index];
    if (!registerKey(stableKeys, symbol.stableKey,
                     CreativeWorldLayoutTable::TerrainPath, index,
                     result.receipt)) {
      return false;
    }
    if (symbol.pointCount < 2U ||
        symbol.firstPointIndex > layout.terrainPathPoints.size() ||
        symbol.pointCount >
            layout.terrainPathPoints.size() - symbol.firstPointIndex ||
        symbol.elevation == CreativeTerrainPathElevation::Follow) {
      result.receipt.failedTable = CreativeWorldLayoutTable::TerrainPath;
      result.receipt.failedIndex = index;
      setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                "creative_world_layout_path_invalid");
      return false;
    }
    for (std::size_t pointIndex = symbol.firstPointIndex;
         pointIndex < symbol.firstPointIndex + symbol.pointCount;
         ++pointIndex) {
      if (pointOwned[pointIndex]) {
        result.receipt.failedTable =
            CreativeWorldLayoutTable::TerrainPathPoint;
        result.receipt.failedIndex = pointIndex;
        setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                  "creative_world_layout_path_point_shared");
        return false;
      }
      pointOwned[pointIndex] = true;
    }
  }
  const auto unownedPoint =
      std::find(pointOwned.begin(), pointOwned.end(), false);
  if (unownedPoint != pointOwned.end()) {
    result.receipt.failedTable = CreativeWorldLayoutTable::TerrainPathPoint;
    result.receipt.failedIndex =
        static_cast<std::size_t>(unownedPoint - pointOwned.begin());
    setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
              "creative_world_layout_path_point_unowned");
    return false;
  }

  for (std::size_t index = 0U; index < layout.terrainPaths.size(); ++index) {
    const CreativeWorldLayoutTerrainPath& symbol = layout.terrainPaths[index];
    CreativeTerrainPathRecipeRequest request;
    request.document = &staged;
    request.kind = symbol.kind;
    request.points = std::span{layout.terrainPathPoints}.subspan(
        symbol.firstPointIndex, symbol.pointCount);
    request.elevation = symbol.elevation;
    request.halfWidthCells = symbol.halfWidthCells;
    request.amplitudeCells = symbol.amplitudeCells;
    request.paintSurface = symbol.paintSurface;
    request.material = symbol.material;
    const CreativeTerrainRecipeResult recipe =
        buildCreativeTerrainPathRecipe(request);
    if (!recipe.receipt.accepted) {
      result.receipt.failedTable = CreativeWorldLayoutTable::TerrainPath;
      result.receipt.failedIndex = index;
      result.receipt.kernelReasonCode = recipe.receipt.kernelReasonCode;
      setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                recipe.receipt.reasonCode);
      return false;
    }
    if ((!recipe.plan.controlEdits.empty() &&
         !staged.applyTerrainControlEdits(recipe.plan.controlEdits).accepted) ||
        (!recipe.plan.materialEdits.empty() &&
         !staged.applyTerrainMaterialEdits(recipe.plan.materialEdits)
              .accepted)) {
      result.receipt.failedTable = CreativeWorldLayoutTable::TerrainPath;
      result.receipt.failedIndex = index;
      setStatus(result.receipt, CreativeWorldLayoutStatus::MutationRejected,
                "creative_world_layout_path_stage_rejected");
      return false;
    }
  }

  result.plan.terrainEdits = terrainDiff(
      document.terrainField().controls(), staged.terrainField().controls());
  result.plan.materialEdits = materialDiff(
      document.terrainMaterialField().overrides(),
      staged.terrainMaterialField().overrides());
  if (result.plan.terrainEdits.size() > kCreativeTerrainControlCapacity ||
      result.plan.materialEdits.size() >
          kCreativeTerrainMaterialOverrideCapacity) {
    setStatus(result.receipt, CreativeWorldLayoutStatus::CapacityExceeded,
              "creative_world_layout_diff_capacity_exceeded");
    result.plan = {};
    return false;
  }
  return true;
}

[[nodiscard]] bool shiftBuildingVertically(
    CreativeBuildingRecipeRequest& building,
    double offsetMeters) noexcept {
  if (!std::isfinite(offsetMeters)) {
    return false;
  }
  const auto shift = [offsetMeters](double& value) {
    value += offsetMeters;
    return std::isfinite(value);
  };
  if (building.rootMode == CreativeBuildingRootMode::CreateRoom &&
      (!shift(building.rootBounds.min.y) ||
       !shift(building.rootBounds.max.y))) {
    return false;
  }
  for (CreativeBuildingBoxSpec& box : building.boxes) {
    if (!shift(box.bounds.min.y) || !shift(box.bounds.max.y)) {
      return false;
    }
  }
  for (CreativeBuildingWallSpec& wall : building.walls) {
    if (!shift(wall.start.y) || !shift(wall.end.y)) {
      return false;
    }
  }
  return true;
}

}  // namespace iggy3d::creative::world_layout_compile
