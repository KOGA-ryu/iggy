#include "app/iggy3d/creative/world/WorldLayout.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"

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

namespace iggy3d::creative {
namespace {

constexpr double kWorldLayoutFloorVerticalScale = 0.05;

void setStatus(CreativeWorldLayoutReceipt& receipt,
               CreativeWorldLayoutStatus status,
               std::string_view reasonCode,
               bool accepted = false) {
  receipt.status = status;
  receipt.reasonCode = std::string(reasonCode);
  receipt.accepted = accepted;
}

[[nodiscard]] bool validDocument(const CreativeDocument& document) noexcept {
  return document.isValid() && document.id() != kInvalidDocumentId &&
         document.nextObjectId() != kInvalidObjectId &&
         document.terrainField().validateInvariants() &&
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
                                std::int32_t baseLayer,
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

[[nodiscard]] bool collectOwnedObjectRemovalOrder(
    const CreativeDocument& document,
    std::string_view layoutTag,
    std::vector<CreativeObjectId>& output) {
  std::unordered_set<CreativeObjectId> remaining;
  std::vector<CreativeObjectId> orderedIds;
  for (const CreativeObject& object : document.objects()) {
    if (hasTag(object.tags, layoutTag)) {
      remaining.insert(object.id);
      orderedIds.push_back(object.id);
    }
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

}  // namespace

std::string_view toString(CreativeWorldLayoutTable table) noexcept {
  switch (table) {
    case CreativeWorldLayoutTable::None: return "None";
    case CreativeWorldLayoutTable::Building: return "Building";
    case CreativeWorldLayoutTable::Room: return "Room";
    case CreativeWorldLayoutTable::Box: return "Box";
    case CreativeWorldLayoutTable::Wall: return "Wall";
    case CreativeWorldLayoutTable::Opening: return "Opening";
    case CreativeWorldLayoutTable::TerrainProfile: return "TerrainProfile";
    case CreativeWorldLayoutTable::TerrainPath: return "TerrainPath";
    case CreativeWorldLayoutTable::TerrainPathPoint: return "TerrainPathPoint";
  }
  return "Unknown";
}

std::string_view toString(CreativeWorldLayoutStatus status) noexcept {
  switch (status) {
    case CreativeWorldLayoutStatus::NotRequested: return "NotRequested";
    case CreativeWorldLayoutStatus::InvalidDocument: return "InvalidDocument";
    case CreativeWorldLayoutStatus::InvalidSchema: return "InvalidSchema";
    case CreativeWorldLayoutStatus::Empty: return "Empty";
    case CreativeWorldLayoutStatus::DuplicateStableKey:
      return "DuplicateStableKey";
    case CreativeWorldLayoutStatus::InvalidSymbol: return "InvalidSymbol";
    case CreativeWorldLayoutStatus::KernelRejected: return "KernelRejected";
    case CreativeWorldLayoutStatus::CapacityExceeded: return "CapacityExceeded";
    case CreativeWorldLayoutStatus::NoChange: return "NoChange";
    case CreativeWorldLayoutStatus::Ready: return "Ready";
    case CreativeWorldLayoutStatus::StalePlan: return "StalePlan";
    case CreativeWorldLayoutStatus::MutationRejected:
      return "MutationRejected";
    case CreativeWorldLayoutStatus::ObjectRejected: return "ObjectRejected";
    case CreativeWorldLayoutStatus::InstallRejected: return "InstallRejected";
    case CreativeWorldLayoutStatus::Applied: return "Applied";
  }
  return "Unknown";
}

std::string creativeWorldLayoutTag(std::string_view layoutKey) {
  return "creative_world_layout:" + std::string(layoutKey);
}

CreativeWorldLayoutCompileResult buildCreativeWorldLayoutPlan(
    const CreativeDocument& document,
    const CreativeWorldLayout& layout) {
  CreativeWorldLayoutCompileResult result;
  result.receipt.requested = true;
  if (!validDocument(document)) {
    setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidDocument,
              "creative_world_layout_document_invalid");
    return result;
  }
  if (layout.schemaVersion != kCreativeWorldLayoutSchemaVersion ||
      !validStableKey(layout.stableKey) ||
      !validTerrainOwnership(layout.terrainOwnership)) {
    setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSchema,
              "creative_world_layout_schema_invalid");
    return result;
  }

  const CreativeWorldLayoutRoomCompileResult roomExpansion =
      expandCreativeWorldLayoutRooms(layout);
  if (!roomExpansion.accepted) {
    result.receipt.failedTable =
        roomExpansion.status ==
                CreativeWorldLayoutRoomCompileStatus::InvalidOpeningHost
            ? CreativeWorldLayoutTable::Opening
            : CreativeWorldLayoutTable::Room;
    result.receipt.failedIndex = roomExpansion.failedIndex;
    setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
              roomExpansion.reasonCode);
    return result;
  }
  const CreativeWorldLayout& expanded = roomExpansion.expanded;

  result.plan.layoutKey = layout.stableKey;
  result.plan.sourceDocumentId = document.id();
  result.plan.sourceDocumentRevision = document.revision();
  result.plan.sourceTerrainRevision = document.terrainField().revision();
  result.plan.sourceMaterialRevision =
      document.terrainMaterialField().revision();

  const std::string layoutTag = creativeWorldLayoutTag(layout.stableKey);
  if (!collectOwnedObjectRemovalOrder(
          document, layoutTag, result.plan.objectRemoveIds)) {
    setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidDocument,
              "creative_world_layout_owned_object_graph_invalid");
    return result;
  }

  std::unordered_set<std::string> stableKeys;
  std::vector<CreativeBuildingRecipeRequest> buildings(layout.buildings.size());
  const CreativeGridSettings grid = document.gridSettings();
  for (std::size_t index = 0U; index < layout.buildings.size(); ++index) {
    const CreativeWorldLayoutBuilding& symbol = layout.buildings[index];
    if (!registerKey(stableKeys, symbol.stableKey,
                     CreativeWorldLayoutTable::Building, index,
                     result.receipt)) {
      return result;
    }
    if (symbol.name.empty() ||
        (symbol.rootMode != CreativeBuildingRootMode::None &&
         symbol.rootMode != CreativeBuildingRootMode::CreateRoom)) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Building;
      result.receipt.failedIndex = index;
      setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                "creative_world_layout_building_invalid");
      return result;
    }
    CreativeBuildingRecipeRequest& building = buildings[index];
    building.stableKey = childKey(layout.stableKey, symbol.stableKey);
    building.name = symbol.name;
    building.rootMode = symbol.rootMode;
    building.visible = symbol.visible;
    building.tags = symbol.tags;
    if (!hasTag(building.tags, layoutTag)) {
      building.tags.push_back(layoutTag);
    }
    if (symbol.rootMode == CreativeBuildingRootMode::CreateRoom &&
        !layoutBounds(grid, symbol.rootFootprint, symbol.rootBaseLayer,
                      symbol.rootHeightCells, building.rootBounds)) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Building;
      result.receipt.failedIndex = index;
      setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                "creative_world_layout_building_bounds_invalid");
      return result;
    }
  }

  for (std::size_t index = 0U; index < layout.rooms.size(); ++index) {
    const CreativeWorldLayoutRoom& symbol = layout.rooms[index];
    if (symbol.buildingIndex >= buildings.size() || symbol.name.empty()) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Room;
      result.receipt.failedIndex = index;
      setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                "creative_world_layout_room_invalid");
      return result;
    }
    const std::string key = childKey(
        layout.buildings[symbol.buildingIndex].stableKey, symbol.stableKey);
    if (!registerKey(stableKeys, key, CreativeWorldLayoutTable::Room, index,
                     result.receipt)) {
      return result;
    }
  }

  for (std::size_t index = 0U; index < expanded.boxes.size(); ++index) {
    const CreativeWorldLayoutBox& symbol = expanded.boxes[index];
    if (symbol.buildingIndex >= buildings.size() || symbol.name.empty()) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Box;
      result.receipt.failedIndex = index;
      setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                "creative_world_layout_box_invalid");
      return result;
    }
    const std::string key = childKey(
        layout.buildings[symbol.buildingIndex].stableKey, symbol.stableKey);
    if (!registerKey(stableKeys, key, CreativeWorldLayoutTable::Box, index,
                     result.receipt)) {
      return result;
    }
    CreativeBounds bounds;
    if (!layoutBounds(grid, symbol.footprint, symbol.baseLayer,
                      symbol.heightCells, bounds)) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Box;
      result.receipt.failedIndex = index;
      setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                "creative_world_layout_box_bounds_invalid");
      return result;
    }
    CreativeBuildingBoxSpec box{symbol.kind, key, symbol.name, bounds};
    if (symbol.kind == CreativeObjectKind::Floor) {
      box.scale.y = kWorldLayoutFloorVerticalScale;
    }
    buildings[symbol.buildingIndex].boxes.push_back(std::move(box));
  }

  std::vector<std::size_t> localWallIndices(
      expanded.walls.size(), kInvalidCreativeWorldLayoutIndex);
  for (std::size_t index = 0U; index < expanded.walls.size(); ++index) {
    const CreativeWorldLayoutWall& symbol = expanded.walls[index];
    if (symbol.buildingIndex >= buildings.size() || symbol.name.empty() ||
        !std::isfinite(symbol.baseLayer) ||
        symbol.heightCells == 0U || !std::isfinite(symbol.thicknessCells) ||
        symbol.thicknessCells <= 0.0) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Wall;
      result.receipt.failedIndex = index;
      setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                "creative_world_layout_wall_invalid");
      return result;
    }
    const std::string key = childKey(
        layout.buildings[symbol.buildingIndex].stableKey, symbol.stableKey);
    if (!registerKey(stableKeys, key, CreativeWorldLayoutTable::Wall, index,
                     result.receipt)) {
      return result;
    }
    CreativeBuildingWallSpec wall;
    wall.stableKey = key;
    wall.name = symbol.name;
    wall.heightMeters = symbol.heightCells * grid.cellSizeMeters;
    wall.thicknessMeters = symbol.thicknessCells * grid.cellSizeMeters;
    if (!layoutPoint(grid, symbol.start, symbol.baseLayer, wall.start) ||
        !layoutPoint(grid, symbol.end, symbol.baseLayer, wall.end)) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Wall;
      result.receipt.failedIndex = index;
      setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                "creative_world_layout_wall_coordinate_invalid");
      return result;
    }
    localWallIndices[index] = buildings[symbol.buildingIndex].walls.size();
    buildings[symbol.buildingIndex].walls.push_back(std::move(wall));
  }

  for (std::size_t index = 0U; index < expanded.openings.size(); ++index) {
    const CreativeWorldLayoutOpening& symbol = expanded.openings[index];
    if (symbol.hostKind != CreativeWorldLayoutOpeningHostKind::Wall ||
        symbol.wallIndex >= expanded.walls.size() || symbol.name.empty()) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Opening;
      result.receipt.failedIndex = index;
      setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                "creative_world_layout_opening_invalid");
      return result;
    }
    const CreativeWorldLayoutWall& wallSymbol = expanded.walls[symbol.wallIndex];
    const std::size_t buildingIndex = wallSymbol.buildingIndex;
    const std::string key = childKey(
        layout.buildings[buildingIndex].stableKey, symbol.stableKey);
    if (!registerKey(stableKeys, key, CreativeWorldLayoutTable::Opening, index,
                     result.receipt)) {
      return result;
    }
    CreativeBuildingOpeningSpec opening;
    opening.kind = symbol.kind;
    opening.pose = symbol.pose;
    opening.stableKey = key;
    opening.name = symbol.name;
    opening.centerOffsetMeters =
        symbol.centerOffsetCells * grid.cellSizeMeters;
    opening.widthMeters = symbol.widthCells * grid.cellSizeMeters;
    opening.cutoutBottomMeters =
        symbol.cutoutBottomCells * grid.cellSizeMeters;
    opening.cutoutHeightMeters =
        symbol.cutoutHeightCells * grid.cellSizeMeters;
    opening.includeInsert = symbol.includeInsert;
    opening.insertBottomMeters =
        (symbol.kind == CreativeBuildingOpeningKind::Window &&
         symbol.insertBottomCells == 0.0)
            ? opening.cutoutBottomMeters
            : symbol.insertBottomCells * grid.cellSizeMeters;
    opening.insertHeightMeters =
        symbol.insertHeightCells * grid.cellSizeMeters;
    opening.insertWidthMeters = symbol.insertWidthCells * grid.cellSizeMeters;
    opening.insertThicknessMeters =
        symbol.insertThicknessCells * grid.cellSizeMeters;
    buildings[buildingIndex]
        .walls[localWallIndices[symbol.wallIndex]]
        .openings.push_back(std::move(opening));
  }

  CreativeObjectId nextObjectId = document.nextObjectId();
  for (std::size_t index = 0U; index < buildings.size(); ++index) {
    const CreativeBuildingRecipeResult built =
        buildCreativeBuildingRecipe(buildings[index]);
    if (!built.receipt.accepted) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Building;
      result.receipt.failedIndex = index;
      result.receipt.kernelReasonCode = built.receipt.reasonCode;
      setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                "creative_world_layout_building_rejected");
      return result;
    }
    const CreativeRecipeMaterializeResult validated =
        materializeCreativeRecipe(built.plan, nextObjectId);
    if (!validated.receipt.accepted) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Building;
      result.receipt.failedIndex = index;
      result.receipt.kernelReasonCode = validated.receipt.reasonCode;
      setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                "creative_world_layout_building_materialize_rejected");
      return result;
    }
    nextObjectId +=
        static_cast<CreativeObjectId>(validated.createRequests.size());
    result.receipt.objectCount += validated.createRequests.size();
    result.plan.objectRecipes.push_back(built.plan);
  }

  CreativeDocument terrainStaged = document;
  if (layout.terrainOwnership ==
          CreativeWorldLayoutTerrainOwnership::ReplaceAll &&
      !clearTerrain(terrainStaged)) {
    setStatus(result.receipt, CreativeWorldLayoutStatus::MutationRejected,
              "creative_world_layout_terrain_clear_rejected");
    return result;
  }

  for (std::size_t index = 0U; index < layout.terrainProfiles.size(); ++index) {
    const CreativeWorldLayoutTerrainProfile& symbol =
        layout.terrainProfiles[index];
    if (!registerKey(stableKeys, symbol.stableKey,
                     CreativeWorldLayoutTable::TerrainProfile, index,
                     result.receipt)) {
      return result;
    }
    if (symbol.blend != CreativeTerrainProfileBlend::Set ||
        symbol.rodPolicy != CreativeTerrainProfileRodPolicy::Fill) {
      result.receipt.failedTable = CreativeWorldLayoutTable::TerrainProfile;
      result.receipt.failedIndex = index;
      setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                "creative_world_layout_profile_not_absolute");
      return result;
    }
    CreativeTerrainProfileRecipeRequest request;
    request.document = &terrainStaged;
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
      return result;
    }
    if (!recipe.plan.controlEdits.empty() &&
        !terrainStaged.applyTerrainControlEdits(recipe.plan.controlEdits)
             .accepted) {
      result.receipt.failedTable = CreativeWorldLayoutTable::TerrainProfile;
      result.receipt.failedIndex = index;
      setStatus(result.receipt, CreativeWorldLayoutStatus::MutationRejected,
                "creative_world_layout_profile_stage_rejected");
      return result;
    }
  }

  std::vector<bool> pointOwned(layout.terrainPathPoints.size(), false);
  for (std::size_t index = 0U; index < layout.terrainPaths.size(); ++index) {
    const CreativeWorldLayoutTerrainPath& symbol = layout.terrainPaths[index];
    if (!registerKey(stableKeys, symbol.stableKey,
                     CreativeWorldLayoutTable::TerrainPath, index,
                     result.receipt)) {
      return result;
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
      return result;
    }
    for (std::size_t pointIndex = symbol.firstPointIndex;
         pointIndex < symbol.firstPointIndex + symbol.pointCount;
         ++pointIndex) {
      if (pointOwned[pointIndex]) {
        result.receipt.failedTable = CreativeWorldLayoutTable::TerrainPathPoint;
        result.receipt.failedIndex = pointIndex;
        setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                  "creative_world_layout_path_point_shared");
        return result;
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
    return result;
  }

  for (std::size_t index = 0U; index < layout.terrainPaths.size(); ++index) {
    const CreativeWorldLayoutTerrainPath& symbol = layout.terrainPaths[index];
    CreativeTerrainPathRecipeRequest request;
    request.document = &terrainStaged;
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
      return result;
    }
    if ((!recipe.plan.controlEdits.empty() &&
         !terrainStaged.applyTerrainControlEdits(recipe.plan.controlEdits)
              .accepted) ||
        (!recipe.plan.materialEdits.empty() &&
         !terrainStaged.applyTerrainMaterialEdits(recipe.plan.materialEdits)
              .accepted)) {
      result.receipt.failedTable = CreativeWorldLayoutTable::TerrainPath;
      result.receipt.failedIndex = index;
      setStatus(result.receipt, CreativeWorldLayoutStatus::MutationRejected,
                "creative_world_layout_path_stage_rejected");
      return result;
    }
  }

  result.plan.terrainEdits = terrainDiff(
      document.terrainField().controls(), terrainStaged.terrainField().controls());
  result.plan.materialEdits = materialDiff(
      document.terrainMaterialField().overrides(),
      terrainStaged.terrainMaterialField().overrides());
  if (result.plan.terrainEdits.size() > kCreativeTerrainControlCapacity ||
      result.plan.materialEdits.size() >
          kCreativeTerrainMaterialOverrideCapacity) {
    setStatus(result.receipt, CreativeWorldLayoutStatus::CapacityExceeded,
              "creative_world_layout_diff_capacity_exceeded");
    result.plan = {};
    return result;
  }

  const bool hasSourceSymbols =
      !layout.buildings.empty() || !layout.boxes.empty() ||
      !layout.walls.empty() || !layout.openings.empty() ||
      !layout.terrainProfiles.empty() || !layout.terrainPaths.empty();
  const bool hasOperations = !result.plan.objectRemoveIds.empty() ||
                             !result.plan.objectRecipes.empty() ||
                             !result.plan.terrainEdits.empty() ||
                             !result.plan.materialEdits.empty();
  if (!hasSourceSymbols && !hasOperations) {
    setStatus(result.receipt, CreativeWorldLayoutStatus::Empty,
              "creative_world_layout_empty");
    result.plan = {};
    return result;
  }

  const CreativeWorldLayoutPreviewResult preview =
      previewCreativeWorldLayoutPlan(document, result.plan);
  if (!preview.accepted) {
    setStatus(result.receipt, preview.status, preview.reasonCode);
    result.plan = {};
    return result;
  }

  result.receipt.buildingCount = layout.buildings.size();
  result.receipt.objectRecipeCount = result.plan.objectRecipes.size();
  result.receipt.objectRemoveCount = result.plan.objectRemoveIds.size();
  result.receipt.terrainControlEditCount = result.plan.terrainEdits.size();
  result.receipt.terrainMaterialEditCount = result.plan.materialEdits.size();
  const bool changed = preview.status == CreativeWorldLayoutStatus::Ready;
  setStatus(result.receipt, preview.status,
            changed ? "creative_world_layout_ready"
                    : "creative_world_layout_no_change",
            true);
  return result;
}

}  // namespace iggy3d::creative
