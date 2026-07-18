#include "app/iggy3d/creative/world/WorldLayout.hpp"
#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"
#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRoofs.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"

#include "app/iggy3d/creative/world/WorldLayoutVerticalConnectors.hpp"

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/recipes/StructuralSurfaceRecipe.hpp"

#include <algorithm>
#include <array>
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
    case CreativeWorldLayoutTable::Level: return "Level";
    case CreativeWorldLayoutTable::Room: return "Room";
    case CreativeWorldLayoutTable::VerticalConnector:
      return "VerticalConnector";
    case CreativeWorldLayoutTable::Box:
      return "Box";
    case CreativeWorldLayoutTable::Wall: return "Wall";
    case CreativeWorldLayoutTable::Opening: return "Opening";
    case CreativeWorldLayoutTable::Object: return "Object";
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

std::string_view toString(CreativeWorldLayoutRoomEdge edge) noexcept {
  switch (edge) {
    case CreativeWorldLayoutRoomEdge::North: return "North";
    case CreativeWorldLayoutRoomEdge::East: return "East";
    case CreativeWorldLayoutRoomEdge::South: return "South";
    case CreativeWorldLayoutRoomEdge::West: return "West";
    case CreativeWorldLayoutRoomEdge::Count: break;
  }
  return "Unknown";
}

std::string_view creativeWorldLayoutRoomEdgeKey(
    CreativeWorldLayoutRoomEdge edge) noexcept {
  switch (edge) {
    case CreativeWorldLayoutRoomEdge::North: return "north";
    case CreativeWorldLayoutRoomEdge::East: return "east";
    case CreativeWorldLayoutRoomEdge::South: return "south";
    case CreativeWorldLayoutRoomEdge::West: return "west";
    case CreativeWorldLayoutRoomEdge::Count: break;
  }
  return "unknown";
}

std::string creativeWorldLayoutTag(std::string_view layoutKey) {
  return "creative_world_layout:" + std::string(layoutKey);
}

bool validCreativeWorldLayoutStableKey(std::string_view key) noexcept {
  return validStableKey(key);
}

bool creativeWorldLayoutStableKeyExists(const CreativeWorldLayout& layout,
                                        std::string_view key) noexcept {
  const auto matches = [&](const auto& value) {
    return value.stableKey == key;
  };
  return std::any_of(layout.buildings.begin(), layout.buildings.end(),
                     matches) ||
         std::any_of(layout.levels.begin(), layout.levels.end(), matches) ||
         std::any_of(layout.rooms.begin(), layout.rooms.end(), matches) ||
         std::any_of(layout.verticalConnectors.begin(),
                     layout.verticalConnectors.end(), matches) ||
         std::any_of(layout.boxes.begin(), layout.boxes.end(), matches) ||
         std::any_of(layout.walls.begin(), layout.walls.end(), matches) ||
         std::any_of(layout.openings.begin(), layout.openings.end(), matches) ||
         std::any_of(layout.objects.begin(), layout.objects.end(), matches) ||
         std::any_of(layout.terrainProfiles.begin(),
                     layout.terrainProfiles.end(), matches) ||
         std::any_of(layout.terrainPaths.begin(), layout.terrainPaths.end(),
                     matches);
}

std::string mintCreativeWorldLayoutStableKey(
    const CreativeWorldLayout& layout,
    std::uint64_t& nextOrdinal,
    std::string_view prefix) {
  for (;;) {
    const std::string candidate =
        std::string(prefix) + "_" + std::to_string(nextOrdinal++);
    if (!creativeWorldLayoutStableKeyExists(layout, candidate)) {
      return candidate;
    }
  }
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
    result.receipt.failedTable = CreativeWorldLayoutTable::Room;
    if (roomExpansion.status ==
        CreativeWorldLayoutRoomCompileStatus::InvalidOpeningHost) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Opening;
    } else if (roomExpansion.status ==
               CreativeWorldLayoutRoomCompileStatus::InvalidLevel) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Level;
    }
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
    appendTagOnce(building.tags, layoutTag);
    appendTagOnce(building.tags, creativeWorldLayoutProvenanceTag(
                                     layout,
                                     CreativeWorldLayoutTable::Building,
                                     index));
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

  for (std::size_t index = 0U; index < layout.levels.size(); ++index) {
    const CreativeWorldLayoutLevel& symbol = layout.levels[index];
    if (symbol.buildingIndex >= buildings.size() || symbol.name.empty() ||
        !std::isfinite(symbol.floorTopLayer) ||
        symbol.wallHeightCells == 0U || symbol.floorThicknessLayers == 0U ||
        symbol.ceilingThicknessLayers == 0U ||
        symbol.roofThicknessLayers == 0U) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Level;
      result.receipt.failedIndex = index;
      setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                "creative_world_layout_level_invalid");
      return result;
    }
    const std::string key = childKey(
        layout.buildings[symbol.buildingIndex].stableKey, symbol.stableKey);
    if (!registerKey(stableKeys, key, CreativeWorldLayoutTable::Level, index,
                     result.receipt)) {
      return result;
    }
  }

  for (std::size_t index = 0U; index < layout.rooms.size(); ++index) {
    const CreativeWorldLayoutRoom& symbol = layout.rooms[index];
    if (symbol.buildingIndex >= buildings.size() ||
        symbol.levelIndex >= layout.levels.size() ||
        layout.levels[symbol.levelIndex].buildingIndex !=
            symbol.buildingIndex ||
        symbol.name.empty()) {
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

  std::vector<CreativeWorldLayoutVerticalConnectorPlan> connectorPlans;
  connectorPlans.reserve(layout.verticalConnectors.size());
  for (std::size_t index = 0U; index < layout.verticalConnectors.size();
       ++index) {
    const CreativeWorldLayoutVerticalConnector& symbol =
        layout.verticalConnectors[index];
    const CreativeWorldLayoutVerticalConnectorPlan connector =
        planCreativeWorldLayoutVerticalConnector(grid, layout, index);
    if (!connector.accepted) {
      result.receipt.failedTable = CreativeWorldLayoutTable::VerticalConnector;
      result.receipt.failedIndex = index;
      result.receipt.kernelReasonCode = connector.reasonCode;
      setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                "creative_world_layout_vertical_connector_rejected");
      return result;
    }
    const std::string key = childKey(
        layout.buildings[symbol.buildingIndex].stableKey, symbol.stableKey);
    if (!registerKey(stableKeys, key,
                     CreativeWorldLayoutTable::VerticalConnector, index,
                     result.receipt)) {
      return result;
    }
    connectorPlans.push_back(connector);
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
    const CreativeStructuralSurfaceAnchor surfaceAnchor =
        creativeStructuralSurfaceAnchor(symbol.kind);
    if (surfaceAnchor == CreativeStructuralSurfaceAnchor::None) {
      if (!layoutBounds(grid, symbol.footprint, symbol.anchorLayer,
                        symbol.layerCount, bounds)) {
        result.receipt.failedTable = CreativeWorldLayoutTable::Box;
        result.receipt.failedIndex = index;
        setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                  "creative_world_layout_box_bounds_invalid");
        return result;
      }
    } else {
      CreativeStructuralSurfaceRecipeRequest surface;
      surface.kind = symbol.kind;
      surface.layerCount = symbol.layerCount;
      if (!validRect(symbol.footprint) ||
          !std::isfinite(grid.cellSizeMeters) || grid.cellSizeMeters <= 0.0 ||
          !worldCoordinate(grid.origin.x, grid.cellSizeMeters,
                           symbol.footprint.minimum.x, surface.minimumX) ||
          !worldCoordinate(grid.origin.x, grid.cellSizeMeters,
                           symbol.footprint.maximum.x, surface.maximumX) ||
          !worldCoordinate(grid.origin.z, grid.cellSizeMeters,
                           symbol.footprint.minimum.z, surface.minimumZ) ||
          !worldCoordinate(grid.origin.z, grid.cellSizeMeters,
                           symbol.footprint.maximum.z, surface.maximumZ) ||
          !worldCoordinate(grid.origin.y, grid.cellSizeMeters,
                           symbol.anchorLayer, surface.anchorPlaneMeters)) {
        result.receipt.failedTable = CreativeWorldLayoutTable::Box;
        result.receipt.failedIndex = index;
        setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                  "creative_world_layout_box_bounds_invalid");
        return result;
      }
      const CreativeStructuralSurfaceRecipeResult planned =
          planCreativeStructuralSurface(surface);
      if (!planned.accepted) {
        result.receipt.failedTable = CreativeWorldLayoutTable::Box;
        result.receipt.failedIndex = index;
        result.receipt.kernelReasonCode = planned.reasonCode;
        setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                  "creative_world_layout_box_geometry_rejected");
        return result;
      }
      bounds = planned.bounds;
    }
    CreativeBuildingBoxSpec box{symbol.kind, key, symbol.name, bounds};
    appendTagOnce(box.tags, creativeWorldLayoutProvenanceTag(
                                layout, CreativeWorldLayoutTable::Box,
                                index));
    buildings[symbol.buildingIndex].boxes.push_back(std::move(box));
  }

  for (std::size_t index = 0U; index < layout.rooms.size(); ++index) {
    const CreativeWorldLayoutRoom& symbol = layout.rooms[index];
    const CreativeRectangularRoomGeometryPlan geometry =
        planCreativeWorldLayoutRoomGeometry(grid, layout, index);
    const CreativeWorldLayoutResolvedRoomGeometry resolved =
        resolveCreativeWorldLayoutRoomGeometry(layout, index);
    if (!geometry.accepted || !resolved.valid) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Room;
      result.receipt.failedIndex = index;
      result.receipt.kernelReasonCode = geometry.reasonCode;
      setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                "creative_world_layout_room_geometry_rejected");
      return result;
    }
    const CreativeWorldLayoutVerticalConnectorPlan* floorCutout = nullptr;
    const CreativeWorldLayoutVerticalConnectorPlan* upperCutout = nullptr;
    for (const CreativeWorldLayoutVerticalConnectorPlan& connector :
         connectorPlans) {
      if (connector.upperRoomIndex == index) {
        floorCutout = &connector;
      }
      if (connector.lowerRoomIndex == index) {
        upperCutout = &connector;
      }
    }

    const auto appendSurface =
        [&](CreativeObjectKind kind, double anchorLayer,
            std::uint16_t layerCount, std::string_view suffix,
            std::string_view label,
            const CreativeWorldLayoutVerticalConnectorPlan* cutout) {
          CreativeStructuralSurfaceRecipeRequest surface;
          surface.kind = kind;
          surface.layerCount = layerCount;
          if (!worldCoordinate(grid.origin.x, grid.cellSizeMeters,
                               symbol.footprint.minimum.x, surface.minimumX) ||
              !worldCoordinate(grid.origin.x, grid.cellSizeMeters,
                               symbol.footprint.maximum.x, surface.maximumX) ||
              !worldCoordinate(grid.origin.z, grid.cellSizeMeters,
                               symbol.footprint.minimum.z, surface.minimumZ) ||
              !worldCoordinate(grid.origin.z, grid.cellSizeMeters,
                               symbol.footprint.maximum.z, surface.maximumZ) ||
              !worldCoordinate(grid.origin.y, grid.cellSizeMeters, anchorLayer,
                               surface.anchorPlaneMeters)) {
            result.receipt.failedTable = CreativeWorldLayoutTable::Room;
            result.receipt.failedIndex = index;
            setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                      "creative_world_layout_room_surface_invalid");
            return false;
          }

          std::array<CreativeStructuralSurfaceRecipeResult,
                     kCreativeStructuralSurfaceCutoutPieceCapacity>
              pieces{};
          std::size_t pieceCount = 1U;
          if (cutout == nullptr) {
            pieces[0] = planCreativeStructuralSurface(surface);
            if (!pieces[0].accepted) {
              result.receipt.kernelReasonCode = pieces[0].reasonCode;
              result.receipt.failedTable = CreativeWorldLayoutTable::Room;
              result.receipt.failedIndex = index;
              setStatus(result.receipt,
                        CreativeWorldLayoutStatus::KernelRejected,
                        "creative_world_layout_room_surface_rejected");
              return false;
            }
          } else {
            CreativeStructuralSurfaceCutoutRequest cutoutRequest;
            cutoutRequest.surface = surface;
            if (!worldCoordinate(grid.origin.x, grid.cellSizeMeters,
                                 cutout->openingFootprint.minimum.x,
                                 cutoutRequest.cutoutMinimumX) ||
                !worldCoordinate(grid.origin.x, grid.cellSizeMeters,
                                 cutout->openingFootprint.maximum.x,
                                 cutoutRequest.cutoutMaximumX) ||
                !worldCoordinate(grid.origin.z, grid.cellSizeMeters,
                                 cutout->openingFootprint.minimum.z,
                                 cutoutRequest.cutoutMinimumZ) ||
                !worldCoordinate(grid.origin.z, grid.cellSizeMeters,
                                 cutout->openingFootprint.maximum.z,
                                 cutoutRequest.cutoutMaximumZ)) {
              result.receipt.failedTable =
                  CreativeWorldLayoutTable::VerticalConnector;
              result.receipt.failedIndex =
                  static_cast<std::size_t>(cutout - connectorPlans.data());
              setStatus(
                  result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                  "creative_world_layout_vertical_connector_cutout_invalid");
              return false;
            }
            const CreativeStructuralSurfaceCutoutResult cut =
                planCreativeStructuralSurfaceCutout(cutoutRequest);
            if (!cut.accepted || cut.pieceCount == 0U) {
              result.receipt.failedTable =
                  CreativeWorldLayoutTable::VerticalConnector;
              result.receipt.failedIndex =
                  static_cast<std::size_t>(cutout - connectorPlans.data());
              result.receipt.kernelReasonCode = cut.reasonCode;
              setStatus(
                  result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                  "creative_world_layout_vertical_connector_cutout_rejected");
              return false;
            }
            pieces = cut.pieces;
            pieceCount = cut.pieceCount;
          }

          for (std::size_t pieceIndex = 0U; pieceIndex < pieceCount;
               ++pieceIndex) {
            const std::string localKey =
                symbol.stableKey + std::string(suffix) +
                (cutout == nullptr
                     ? std::string{}
                     : ".part." + std::to_string(pieceIndex + 1U));
            const std::string key = childKey(
                layout.buildings[symbol.buildingIndex].stableKey, localKey);
            if (!registerKey(stableKeys, key, CreativeWorldLayoutTable::Room,
                             index, result.receipt)) {
              return false;
            }
            CreativeBuildingBoxSpec box{
                kind, key,
                symbol.name + std::string(label) +
                    (cutout == nullptr
                         ? std::string{}
                         : " Part " + std::to_string(pieceIndex + 1U)),
                pieces[pieceIndex].bounds};
            appendTagOnce(box.tags,
                          creativeWorldLayoutProvenanceTag(
                              layout, CreativeWorldLayoutTable::Room, index));
            buildings[symbol.buildingIndex].boxes.push_back(std::move(box));
          }
          return true;
        };

    if (!appendSurface(CreativeObjectKind::Floor, resolved.floorTopLayer,
                       resolved.floorThicknessLayers, ".floor", " Floor",
                       floorCutout)) {
      return result;
    }
    const bool roof = resolved.upperSurfaceKind == CreativeObjectKind::Roof;
    const CreativeWorldLayoutLevel& level =
        layout.levels[resolved.levelIndex];
    const bool usesAuthoredLevelRoof =
        level.roofStyle == CreativeStructuralRoofStyle::Gable ||
        level.roofOverhangCells > 0.0;
    if (roof && usesAuthoredLevelRoof) {
      const bool firstRoomForLevel = std::none_of(
          layout.rooms.begin(), layout.rooms.begin() +
                                    static_cast<std::ptrdiff_t>(index),
          [&](const CreativeWorldLayoutRoom& room) {
            return room.levelIndex == resolved.levelIndex;
          });
      if (!firstRoomForLevel) {
        continue;
      }
      const CreativeWorldLayoutRoofPlan roofPlan =
          planCreativeWorldLayoutRoof(grid, layout, resolved.levelIndex);
      if (!roofPlan.accepted) {
        result.receipt.failedTable = CreativeWorldLayoutTable::Level;
        result.receipt.failedIndex = resolved.levelIndex;
        result.receipt.kernelReasonCode = roofPlan.reasonCode;
        setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                  "creative_world_layout_roof_rejected");
        return result;
      }
      constexpr std::array<std::string_view,
                           kCreativeStructuralRoofPartCapacity>
          kPartSuffixes{".roof.base", ".roof.slope.first",
                        ".roof.slope.second"};
      constexpr std::array<std::string_view,
                           kCreativeStructuralRoofPartCapacity>
          kPartLabels{" Roof Base", " Roof Slope 1", " Roof Slope 2"};
      for (std::size_t partIndex = 0U;
           partIndex < roofPlan.geometry.partCount; ++partIndex) {
        const CreativeStructuralRoofPart& part =
            roofPlan.geometry.parts[partIndex];
        const std::string key = childKey(
            layout.buildings[level.buildingIndex].stableKey,
            level.stableKey + std::string(kPartSuffixes[partIndex]));
        if (!registerKey(stableKeys, key, CreativeWorldLayoutTable::Level,
                         resolved.levelIndex, result.receipt)) {
          return result;
        }
        CreativeBuildingBoxSpec box{
            part.kind, key, level.name + std::string(kPartLabels[partIndex]),
            part.bounds, {1.0, 1.0, 1.0}, {},
            part.rotationEulerRadians};
        appendTagOnce(
            box.tags,
            creativeWorldLayoutProvenanceTag(
                layout, CreativeWorldLayoutTable::Level,
                resolved.levelIndex));
        buildings[level.buildingIndex].boxes.push_back(std::move(box));
      }
      continue;
    }
    if (!appendSurface(resolved.upperSurfaceKind,
                       resolved.floorTopLayer + resolved.wallHeightCells,
                       resolved.upperSurfaceThicknessLayers,
                       roof ? ".roof" : ".ceiling", roof ? " Roof" : " Ceiling",
                       upperCutout)) {
      return result;
    }
  }

  for (std::size_t index = 0U; index < connectorPlans.size(); ++index) {
    const CreativeWorldLayoutVerticalConnector& symbol =
        layout.verticalConnectors[index];
    const CreativeWorldLayoutVerticalConnectorPlan& connector =
        connectorPlans[index];
    const std::string key = childKey(
        layout.buildings[symbol.buildingIndex].stableKey, symbol.stableKey);
    CreativeBuildingBoxSpec box{connector.objectKind,
                                key,
                                symbol.name,
                                connector.authoredBounds,
                                {1.0, 1.0, 1.0},
                                {},
                                connector.rotationEulerRadians};
    appendTagOnce(
        box.tags,
        creativeWorldLayoutProvenanceTag(
            layout, CreativeWorldLayoutTable::VerticalConnector, index));
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
    if (index < layout.walls.size()) {
      appendTagOnce(wall.tags, creativeWorldLayoutProvenanceTag(
                                   layout, CreativeWorldLayoutTable::Wall,
                                   index));
    } else if (index < roomExpansion.wallProvenance.size()) {
      for (const auto& contributor :
           roomExpansion.wallProvenance[index].contributors) {
        appendTagOnce(wall.tags, creativeWorldLayoutRoomEdgeProvenanceTag(
                                     layout, contributor.roomIndex,
                                     contributor.roomEdge));
      }
    }
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
    opening.insertAssetId = symbol.insertAssetId;
    opening.insertAssetSourceBoundsMeters =
        symbol.insertAssetSourceBoundsMeters;
    opening.hasInsertAssetSourceBounds =
        symbol.hasInsertAssetSourceBounds;
    appendTagOnce(opening.tags, creativeWorldLayoutProvenanceTag(
                                    layout,
                                    CreativeWorldLayoutTable::Opening,
                                    index));
    const CreativeWorldLayoutOpening& sourceOpening = layout.openings[index];
    if (sourceOpening.hostKind ==
            CreativeWorldLayoutOpeningHostKind::RoomEdge &&
        sourceOpening.roomIndex < layout.rooms.size()) {
      appendTagOnce(opening.tags,
                    creativeWorldLayoutRoomEdgeProvenanceTag(
                        layout, sourceOpening.roomIndex,
                        sourceOpening.roomEdge));
    } else if (sourceOpening.hostKind ==
                   CreativeWorldLayoutOpeningHostKind::Wall &&
               sourceOpening.wallIndex < layout.walls.size()) {
      appendTagOnce(opening.tags, creativeWorldLayoutProvenanceTag(
                                      layout,
                                      CreativeWorldLayoutTable::Wall,
                                      sourceOpening.wallIndex));
    }
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

  if (!layout.objects.empty()) {
    CreativeObjectLibraryRecipeRequest objectRequest;
    objectRequest.stableKey = layout.stableKey + ".objects";
    objectRequest.name = "World Layout Objects";
    objectRequest.placements.reserve(layout.objects.size());
    for (std::size_t index = 0U; index < layout.objects.size(); ++index) {
      const CreativeWorldLayoutObject& symbol = layout.objects[index];
      if (!registerKey(stableKeys, symbol.stableKey,
                       CreativeWorldLayoutTable::Object, index,
                       result.receipt)) {
        return result;
      }
      if (symbol.name.empty() ||
          symbol.mode >= CreativeObjectLibraryPlacementMode::Count) {
        result.receipt.failedTable = CreativeWorldLayoutTable::Object;
        result.receipt.failedIndex = index;
        setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                  "creative_world_layout_object_invalid");
        return result;
      }
      CreativeObjectLibraryPlacementSpec placement;
      placement.kind = symbol.kind;
      placement.mode = symbol.mode;
      placement.stableKey = symbol.stableKey;
      placement.name = symbol.name;
      placement.assetId = symbol.assetId;
      placement.assetSourceBounds = symbol.assetSourceBoundsMeters;
      placement.hasAssetSourceBounds = symbol.hasAssetSourceBounds;
      placement.yawRadians = symbol.yawRadians;
      placement.scale = symbol.scale;
      placement.visible = symbol.visible;
      placement.tags = symbol.tags;
      appendTagOnce(placement.tags, layoutTag);
      appendTagOnce(placement.tags, creativeWorldLayoutProvenanceTag(
                                        layout,
                                        CreativeWorldLayoutTable::Object,
                                        index));
      const bool positionReady =
          symbol.mode == CreativeObjectLibraryPlacementMode::Bounds
              ? layoutBounds(grid, symbol.boundsCells, placement.bounds)
              : layoutPoint(grid, symbol.pointCells, placement.point);
      if (!positionReady) {
        result.receipt.failedTable = CreativeWorldLayoutTable::Object;
        result.receipt.failedIndex = index;
        setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                  "creative_world_layout_object_coordinate_invalid");
        return result;
      }
      objectRequest.placements.push_back(std::move(placement));
    }
    const CreativeObjectLibraryRecipeResult objects =
        buildCreativeObjectLibraryRecipe(objectRequest);
    if (!objects.receipt.accepted) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Object;
      result.receipt.failedIndex = objects.receipt.failedPlacementIndex;
      result.receipt.kernelReasonCode = objects.receipt.reasonCode;
      setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                "creative_world_layout_object_recipe_rejected");
      return result;
    }
    const CreativeRecipeMaterializeResult validated =
        materializeCreativeRecipe(objects.plan, nextObjectId);
    if (!validated.receipt.accepted) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Object;
      result.receipt.kernelReasonCode = validated.receipt.reasonCode;
      setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                "creative_world_layout_object_materialize_rejected");
      return result;
    }
    nextObjectId +=
        static_cast<CreativeObjectId>(validated.createRequests.size());
    result.receipt.objectCount += validated.createRequests.size();
    result.plan.objectRecipes.push_back(objects.plan);
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
      !layout.buildings.empty() || !layout.levels.empty() ||
      !layout.rooms.empty() || !layout.boxes.empty() ||
      !layout.walls.empty() || !layout.openings.empty() ||
      !layout.objects.empty() ||
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
